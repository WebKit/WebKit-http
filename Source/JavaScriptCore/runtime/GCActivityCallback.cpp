/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "GCActivityCallback.h"

#include "APIShims.h"
#include "Heap.h"
#include "JSGlobalData.h"
#include "JSLock.h"
#include "JSObject.h"

#include <wtf/RetainPtr.h>
#include <wtf/WTFThreadData.h>

namespace JSC {

#if USE(CF)

const double gcTimeSlicePerMB = 0.01; // Percentage of CPU time we will spend to reclaim 1 MB
const double maxGCTimeSlice = 0.05; // The maximum amount of CPU time we want to use for opportunistic timer-triggered collections.
const double timerSlop = 2.0; // Fudge factor to avoid performance cost of resetting timer.
const double pagingTimeOut = 0.1; // Time in seconds to allow opportunistic timer to iterate over all blocks to see if the Heap is paged out.
const CFTimeInterval hour = 60 * 60;

DefaultGCActivityCallback::DefaultGCActivityCallback(Heap* heap)
    : GCActivityCallback(heap->globalData(), CFRunLoopGetCurrent())
    , m_delay(s_decade)
{
}

DefaultGCActivityCallback::DefaultGCActivityCallback(Heap* heap, CFRunLoopRef runLoop)
    : GCActivityCallback(heap->globalData(), runLoop)
    , m_delay(s_decade)
{
}

void DefaultGCActivityCallback::doWork()
{
    Heap* heap = &m_globalData->heap;
    if (!isEnabled())
        return;
    
    APIEntryShim shim(m_globalData);
#if !PLATFORM(IOS)
    double startTime = WTF::monotonicallyIncreasingTime();
    if (heap->isPagedOut(startTime + pagingTimeOut)) {
        heap->activityCallback()->cancel();
        heap->increaseLastGCLength(pagingTimeOut);
        return;
    }
#endif
    heap->collect(Heap::DoNotSweep);
}
    
void DefaultGCActivityCallback::scheduleTimer(double newDelay)
{
    if (newDelay * timerSlop > m_delay)
        return;
    double delta = m_delay - newDelay;
    m_delay = newDelay;
    CFRunLoopTimerSetNextFireDate(m_timer.get(), CFRunLoopTimerGetNextFireDate(m_timer.get()) - delta);
}

void DefaultGCActivityCallback::cancelTimer()
{
    m_delay = s_decade;
    CFRunLoopTimerSetNextFireDate(m_timer.get(), CFAbsoluteTimeGetCurrent() + s_decade);
}

void DefaultGCActivityCallback::didAllocate(size_t bytes)
{
    // The first byte allocated in an allocation cycle will report 0 bytes to didAllocate. 
    // We pretend it's one byte so that we don't ignore this allocation entirely.
    if (!bytes)
        bytes = 1;
    Heap* heap = static_cast<Heap*>(&m_globalData->heap);
    double gcTimeSlice = std::min((static_cast<double>(bytes) / MB) * gcTimeSlicePerMB, maxGCTimeSlice);
    double newDelay = heap->lastGCLength() / gcTimeSlice;
    scheduleTimer(newDelay);
}

void DefaultGCActivityCallback::willCollect()
{
    cancelTimer();
}

void DefaultGCActivityCallback::cancel()
{
    cancelTimer();
}

#else

DefaultGCActivityCallback::DefaultGCActivityCallback(Heap* heap)
    : GCActivityCallback(heap->globalData())
{
}

void DefaultGCActivityCallback::doWork()
{
}

void DefaultGCActivityCallback::didAllocate(size_t)
{
}

void DefaultGCActivityCallback::willCollect()
{
}

void DefaultGCActivityCallback::cancel()
{
}

#endif

}

