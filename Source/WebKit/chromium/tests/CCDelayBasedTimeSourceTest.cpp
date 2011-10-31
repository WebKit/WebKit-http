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

#include "cc/CCDelayBasedTimeSource.h"

#include "CCSchedulerTestCommon.h"
#include "cc/CCThread.h"
#include <gtest/gtest.h>
#include <wtf/RefPtr.h>

using namespace WTF;
using namespace WebCore;
using namespace WebKitTests;

namespace {

TEST(CCDelayBasedTimeSourceTest, TaskPostedAndTickCalled)
{
    FakeCCThread thread;
    FakeCCTimeSourceClient client;
    RefPtr<FakeCCDelayBasedTimeSource> timer = FakeCCDelayBasedTimeSource::create(1000.0 / 60.0, &thread);
    timer->setClient(&client);

    timer->setMonotonicallyIncreasingTimeMs(0);
    timer->setActive(true);
    EXPECT_TRUE(thread.hasPendingTask());

    timer->setMonotonicallyIncreasingTimeMs(16);
    thread.runPendingTask();
    EXPECT_TRUE(client.tickCalled());
}

TEST(CCDelayBasedTimeSource, TickNotCalledWithTaskPosted)
{
    FakeCCThread thread;
    FakeCCTimeSourceClient client;
    RefPtr<FakeCCDelayBasedTimeSource> timer = FakeCCDelayBasedTimeSource::create(1000.0 / 60.0, &thread);
    timer->setClient(&client);
    timer->setActive(true);
    EXPECT_TRUE(thread.hasPendingTask());
    timer->setActive(false);
    thread.runPendingTask();
    EXPECT_FALSE(client.tickCalled());
}

TEST(CCDelayBasedTimeSource, StartTwiceEnqueuesOneTask)
{
    FakeCCThread thread;
    FakeCCTimeSourceClient client;
    RefPtr<FakeCCDelayBasedTimeSource> timer = FakeCCDelayBasedTimeSource::create(1000.0 / 60.0, &thread);
    timer->setClient(&client);
    timer->setActive(true);
    EXPECT_TRUE(thread.hasPendingTask());
    thread.reset();
    timer->setActive(true);
    EXPECT_FALSE(thread.hasPendingTask());
}

TEST(CCDelayBasedTimeSource, StartWhenRunningDoesntTick)
{
    FakeCCThread thread;
    FakeCCTimeSourceClient client;
    RefPtr<FakeCCDelayBasedTimeSource> timer = FakeCCDelayBasedTimeSource::create(1000.0 / 60.0, &thread);
    timer->setClient(&client);
    timer->setActive(true);
    thread.runPendingTask();
    thread.reset();
    timer->setActive(true);
    EXPECT_FALSE(thread.hasPendingTask());
}

TEST(CCDelayBasedTimeSourceTest, AchievesTargetRateWithNoNoise)
{
    int numIterations = 1000;

    FakeCCThread thread;
    FakeCCTimeSourceClient client;
    RefPtr<FakeCCDelayBasedTimeSource> timer = FakeCCDelayBasedTimeSource::create(1000.0 / 60.0, &thread);
    timer->setClient(&client);
    timer->setActive(true);

    double totalFrameTime = 0;
    for (int i = 0; i < numIterations; ++i) {
        long long delay = thread.pendingDelay();

        // accumulate the "delay"
        totalFrameTime += delay;

        // Run the callback exactly when asked
        double now = timer->monotonicallyIncreasingTimeMs() + delay;
        timer->setMonotonicallyIncreasingTimeMs(now);
        thread.runPendingTask();
    }
    double averageInterval = totalFrameTime / static_cast<double>(numIterations);
    EXPECT_NEAR(1000.0 / 60.0, averageInterval, 0.1);
}

TEST(CCDelayBasedTimeSource, TestUnrefWhilePending)
{
    FakeCCThread thread;
    FakeCCTimeSourceClient client;
    RefPtr<FakeCCDelayBasedTimeSource> timer = FakeCCDelayBasedTimeSource::create(1000.0 / 60.0, &thread);
    timer->setClient(&client);
    timer->setActive(true); // Should post a task.
    timer->setActive(false);
    timer.clear();
    thread.runPendingTask(); // Should run the posted task, and delete the timer object.
}

}
