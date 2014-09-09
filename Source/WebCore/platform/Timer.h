/*
 * Copyright (C) 2006 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef Timer_h
#define Timer_h

#include <chrono>
#include <functional>
#include <wtf/Noncopyable.h>
#include <wtf/Threading.h>
#include <wtf/Vector.h>

#if PLATFORM(IOS)
#include "WebCoreThread.h"
#endif

namespace WebCore {

// Time intervals are all in seconds.

class TimerHeapElement;

class TimerBase {
    WTF_MAKE_NONCOPYABLE(TimerBase);
    WTF_MAKE_FAST_ALLOCATED;
public:
    TimerBase();
    virtual ~TimerBase();

    void start(double nextFireInterval, double repeatInterval);

    void startRepeating(double repeatInterval) { start(repeatInterval, repeatInterval); }
    void startRepeating(std::chrono::milliseconds repeatInterval) { startRepeating(repeatInterval.count() * 0.001); }
    void startOneShot(double interval) { start(interval, 0); }
    void startOneShot(std::chrono::milliseconds interval) { startOneShot(interval.count() * 0.001); }

    void stop();
    bool isActive() const;

    double nextFireInterval() const;
    double nextUnalignedFireInterval() const;
    double repeatInterval() const { return m_repeatInterval; }

    void augmentFireInterval(double delta) { setNextFireTime(m_nextFireTime + delta); }
    void augmentRepeatInterval(double delta) { augmentFireInterval(delta); m_repeatInterval += delta; }

    void didChangeAlignmentInterval();

    static void fireTimersInNestedEventLoop();

private:
    virtual void fired() = 0;

    virtual double alignedFireTime(double fireTime) const { return fireTime; }

    void checkConsistency() const;
    void checkHeapIndex() const;

    void setNextFireTime(double);

    bool inHeap() const { return m_heapIndex != -1; }

    bool hasValidHeapPosition() const;
    void updateHeapIfNeeded(double oldTime);

    void heapDecreaseKey();
    void heapDelete();
    void heapDeleteMin();
    void heapIncreaseKey();
    void heapInsert();
    void heapPop();
    void heapPopMin();

    Vector<TimerBase*>& timerHeap() const { ASSERT(m_cachedThreadGlobalTimerHeap); return *m_cachedThreadGlobalTimerHeap; }

    double m_nextFireTime; // 0 if inactive
    double m_unalignedNextFireTime; // m_nextFireTime not considering alignment interval
    double m_repeatInterval; // 0 if not repeating
    int m_heapIndex; // -1 if not in heap
    unsigned m_heapInsertionOrder; // Used to keep order among equal-fire-time timers
    Vector<TimerBase*>* m_cachedThreadGlobalTimerHeap;

#ifndef NDEBUG
    ThreadIdentifier m_thread;
    bool m_wasDeleted;
#endif

    friend class ThreadTimers;
    friend class TimerHeapLessThanFunction;
    friend class TimerHeapReference;
};

template <typename TimerFiredClass> class Timer : public TimerBase {
public:
    typedef void (TimerFiredClass::*TimerFiredFunction)(Timer&);
    typedef void (TimerFiredClass::*DeprecatedTimerFiredFunction)(Timer*);

    Timer(TimerFiredClass* object, TimerFiredFunction function)
        : m_function(std::bind(function, object, std::ref(*this)))
    {
    }

    Timer(TimerFiredClass* object, DeprecatedTimerFiredFunction function)
        : m_function(std::bind(function, object, this))
    {
    }

private:
    virtual void fired() override
    {
        m_function();
    }

    std::function<void ()> m_function;
};

inline bool TimerBase::isActive() const
{
    // FIXME: Write this in terms of USE(WEB_THREAD) instead of PLATFORM(IOS).
#if !PLATFORM(IOS)
    ASSERT(m_thread == currentThread());
#else
    ASSERT(WebThreadIsCurrent() || pthread_main_np() || m_thread == currentThread());
#endif // PLATFORM(IOS)
    return m_nextFireTime;
}

class DeferrableOneShotTimer : protected TimerBase {
public:
    template<typename TimerFiredClass>
    DeferrableOneShotTimer(TimerFiredClass* object, void (TimerFiredClass::*function)(), std::chrono::milliseconds delay)
        : DeferrableOneShotTimer(std::bind(function, object), delay)
    {
    }

    DeferrableOneShotTimer(std::function<void ()> function, std::chrono::milliseconds delay)
        : m_function(WTF::move(function))
        , m_delay(delay)
        , m_shouldRestartWhenTimerFires(false)
    {
    }

    void restart()
    {
        // Setting this boolean is much more efficient than calling startOneShot
        // again, which might result in rescheduling the system timer which
        // can be quite expensive.

        if (isActive()) {
            m_shouldRestartWhenTimerFires = true;
            return;
        }
        startOneShot(m_delay);
    }

    void stop()
    {
        m_shouldRestartWhenTimerFires = false;
        TimerBase::stop();
    }

    using TimerBase::isActive;

private:
    virtual void fired() override
    {
        if (m_shouldRestartWhenTimerFires) {
            m_shouldRestartWhenTimerFires = false;
            startOneShot(m_delay);
            return;
        }

        m_function();
    }

    std::function<void ()> m_function;

    std::chrono::milliseconds m_delay;
    bool m_shouldRestartWhenTimerFires;
};

}

#endif
