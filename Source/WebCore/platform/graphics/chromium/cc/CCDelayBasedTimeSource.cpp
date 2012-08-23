/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "CCDelayBasedTimeSource.h"

#include "TraceEvent.h"
#include <algorithm>
#include <wtf/CurrentTime.h>
#include <wtf/MathExtras.h>

namespace WebCore {

namespace {

// doubleTickThreshold prevents ticks from running within the specified fraction of an interval.
// This helps account for jitter in the timebase as well as quick timer reactivation.
const double doubleTickThreshold = 0.25;

// intervalChangeThreshold is the fraction of the interval that will trigger an immediate interval change.
// phaseChangeThreshold is the fraction of the interval that will trigger an immediate phase change.
// If the changes are within the thresholds, the change will take place on the next tick.
// If either change is outside the thresholds, the next tick will be canceled and reissued immediately.
const double intervalChangeThreshold = 0.25;
const double phaseChangeThreshold = 0.25;

}


PassRefPtr<CCDelayBasedTimeSource> CCDelayBasedTimeSource::create(double interval, CCThread* thread)
{
    return adoptRef(new CCDelayBasedTimeSource(interval, thread));
}

CCDelayBasedTimeSource::CCDelayBasedTimeSource(double intervalSeconds, CCThread* thread)
    : m_client(0)
    , m_hasTickTarget(false)
    , m_lastTickTime(0)
    , m_currentParameters(intervalSeconds, 0)
    , m_nextParameters(intervalSeconds, 0)
    , m_state(STATE_INACTIVE)
    , m_timer(thread, this)
{
}

void CCDelayBasedTimeSource::setActive(bool active)
{
    TRACE_EVENT1("cc", "CCDelayBasedTimeSource::setActive", "active", active);
    if (!active) {
        m_state = STATE_INACTIVE;
        m_timer.stop();
        return;
    }

    if (m_state == STATE_STARTING || m_state == STATE_ACTIVE)
        return;

    if (!m_hasTickTarget) {
        // Becoming active the first time is deferred: we post a 0-delay task. When
        // it runs, we use that to establish the timebase, become truly active, and
        // fire the first tick.
        m_state = STATE_STARTING;
        m_timer.startOneShot(0);
        return;
    }

    m_state = STATE_ACTIVE;

    double now = monotonicTimeNow();
    postNextTickTask(now);
}

double CCDelayBasedTimeSource::lastTickTime()
{
    return m_lastTickTime;
}

double CCDelayBasedTimeSource::nextTickTimeIfActivated()
{
    return active() ? m_currentParameters.tickTarget : nextTickTarget(monotonicTimeNow());
}

void CCDelayBasedTimeSource::onTimerFired()
{
    ASSERT(m_state != STATE_INACTIVE);

    double now = monotonicTimeNow();
    m_lastTickTime = now;

    if (m_state == STATE_STARTING) {
        setTimebaseAndInterval(now, m_currentParameters.interval);
        m_state = STATE_ACTIVE;
    }

    postNextTickTask(now);

    // Fire the tick
    if (m_client)
        m_client->onTimerTick();
}

void CCDelayBasedTimeSource::setTimebaseAndInterval(double timebase, double intervalSeconds)
{
    m_nextParameters.interval = intervalSeconds;
    m_nextParameters.tickTarget = timebase;
    m_hasTickTarget = true;

    if (m_state != STATE_ACTIVE) {
        // If we aren't active, there's no need to reset the timer.
        return;
    }

    // If the change in interval is larger than the change threshold,
    // request an immediate reset.
    double intervalDelta = std::abs(intervalSeconds - m_currentParameters.interval);
    double intervalChange = intervalDelta / intervalSeconds;
    if (intervalChange > intervalChangeThreshold) {
        setActive(false);
        setActive(true);
        return;
    }

    // If the change in phase is greater than the change threshold in either
    // direction, request an immediate reset. This logic might result in a false
    // negative if there is a simultaneous small change in the interval and the
    // fmod just happens to return something near zero. Assuming the timebase
    // is very recent though, which it should be, we'll still be ok because the
    // old clock and new clock just happen to line up.
    double targetDelta = std::abs(timebase - m_currentParameters.tickTarget);
    double phaseChange = fmod(targetDelta, intervalSeconds) / intervalSeconds;
    if (phaseChange > phaseChangeThreshold && phaseChange < (1.0 - phaseChangeThreshold)) {
        setActive(false);
        setActive(true);
        return;
    }
}

double CCDelayBasedTimeSource::monotonicTimeNow() const
{
    return monotonicallyIncreasingTime();
}

// This code tries to achieve an average tick rate as close to m_intervalMs as possible.
// To do this, it has to deal with a few basic issues:
//   1. postDelayedTask can delay only at a millisecond granularity. So, 16.666 has to
//      posted as 16 or 17.
//   2. A delayed task may come back a bit late (a few ms), or really late (frames later)
//
// The basic idea with this scheduler here is to keep track of where we *want* to run in
// m_tickTarget. We update this with the exact interval.
//
// Then, when we post our task, we take the floor of (m_tickTarget and now()). If we
// started at now=0, and 60FPs:
//      now=0    target=16.667   postDelayedTask(16)
//
// When our callback runs, we figure out how far off we were from that goal. Because of the flooring
// operation, and assuming our timer runs exactly when it should, this yields:
//      now=16   target=16.667
//
// Since we can't post a 0.667 ms task to get to now=16, we just treat this as a tick. Then,
// we update target to be 33.333. We now post another task based on the difference between our target
// and now:
//      now=16   tickTarget=16.667  newTarget=33.333   --> postDelayedTask(floor(33.333 - 16)) --> postDelayedTask(17)
//
// Over time, with no late tasks, this leads to us posting tasks like this:
//      now=0    tickTarget=0       newTarget=16.667   --> tick(), postDelayedTask(16)
//      now=16   tickTarget=16.667  newTarget=33.333   --> tick(), postDelayedTask(17)
//      now=33   tickTarget=33.333  newTarget=50.000   --> tick(), postDelayedTask(17)
//      now=50   tickTarget=50.000  newTarget=66.667   --> tick(), postDelayedTask(16)
//
// We treat delays in tasks differently depending on the amount of delay we encounter. Suppose we
// posted a task with a target=16.667:
//   Case 1: late but not unrecoverably-so
//      now=18 tickTarget=16.667
//
//   Case 2: so late we obviously missed the tick
//      now=25.0 tickTarget=16.667
//
// We treat the first case as a tick anyway, and assume the delay was
// unusual. Thus, we compute the newTarget based on the old timebase:
//      now=18   tickTarget=16.667  newTarget=33.333   --> tick(), postDelayedTask(floor(33.333-18)) --> postDelayedTask(15)
// This brings us back to 18+15 = 33, which was where we would have been if the task hadn't been late.
//
// For the really late delay, we we move to the next logical tick. The timebase is not reset.
//      now=37   tickTarget=16.667  newTarget=50.000  --> tick(), postDelayedTask(floor(50.000-37)) --> postDelayedTask(13)
//
// Note, that in the above discussion, times are expressed in milliseconds, but in the code, seconds are used.
double CCDelayBasedTimeSource::nextTickTarget(double now)
{
    double newInterval = m_nextParameters.interval;
    double intervalsElapsed = floor((now - m_nextParameters.tickTarget) / newInterval);
    double lastEffectiveTick = m_nextParameters.tickTarget + newInterval * intervalsElapsed;
    double newTickTarget = lastEffectiveTick + newInterval;
    ASSERT(newTickTarget > now);

    // Avoid double ticks when:
    // 1) Turning off the timer and turning it right back on.
    // 2) Jittery data is passed to setTimebaseAndInterval().
    if (newTickTarget - m_lastTickTime <= newInterval * doubleTickThreshold)
        newTickTarget += newInterval;

    return newTickTarget;
}

void CCDelayBasedTimeSource::postNextTickTask(double now)
{
    double newTickTarget = nextTickTarget(now);

    // Post another task *before* the tick and update state
    double delay = newTickTarget - now;
    ASSERT(delay <= m_nextParameters.interval * (1.0 + doubleTickThreshold));
    m_timer.startOneShot(delay);

    m_nextParameters.tickTarget = newTickTarget;
    m_currentParameters = m_nextParameters;
}

}
