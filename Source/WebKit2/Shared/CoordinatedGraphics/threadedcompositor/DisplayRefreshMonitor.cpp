/*
 * Copyright (C) 2014 Igalia S.L.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DisplayRefreshMonitor.h"

#if USE(COORDINATED_GRAPHICS_THREADED) && USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)

#include "CompositingRunLoop.h"
#include "ThreadedCompositor.h"
#include <glib.h>

namespace WebKit {

DisplayRefreshMonitor::DisplayRefreshMonitor(ThreadedCompositor& compositor)
    : WebCore::DisplayRefreshMonitor(0)
    , m_displayRefreshTimer(RunLoop::main(), this, &DisplayRefreshMonitor::displayRefreshCallback)
    , m_compositor(&compositor)
{
    m_displayRefreshTimer.setPriority(G_PRIORITY_HIGH + 30);
}

bool DisplayRefreshMonitor::requestRefreshCallback()
{
    LockHolder locker(mutex());
    setIsScheduled(true);

    if (isPreviousFrameDone() && m_compositor) {
        if (!m_compositor->m_compositingRunLoop->isActive())
            m_compositor->m_compositingRunLoop->scheduleUpdate();
    }

    return true;
}

bool DisplayRefreshMonitor::requiresDisplayRefreshCallback()
{
    LockHolder locker(mutex());
    return isScheduled() && isPreviousFrameDone();
}

void DisplayRefreshMonitor::dispatchDisplayRefreshCallback()
{
    m_displayRefreshTimer.startOneShot(0);
}

void DisplayRefreshMonitor::invalidate()
{
    m_compositor = nullptr;
}

void DisplayRefreshMonitor::displayRefreshCallback()
{
    bool shouldHandleDisplayRefreshNotification = false;
    bool wasRescheduled = false;
    {
        LockHolder locker(mutex());
        shouldHandleDisplayRefreshNotification = isScheduled() && isPreviousFrameDone();
        if (shouldHandleDisplayRefreshNotification) {
            setIsPreviousFrameDone(false);
            setMonotonicAnimationStartTime(monotonicallyIncreasingTime());
        }
    }

    if (shouldHandleDisplayRefreshNotification)
        DisplayRefreshMonitor::handleDisplayRefreshedNotificationOnMainThread(this);
    {
        LockHolder holder(mutex());
        wasRescheduled = isScheduled();
    }

    if (m_compositor) {
        if (m_compositor->m_clientRendersNextFrame.compareExchangeStrong(true, false))
            m_compositor->m_scene->renderNextFrame();
        if (m_compositor->m_coordinateUpdateCompletionWithClient.compareExchangeStrong(true, false))
            m_compositor->m_compositingRunLoop->updateCompleted();
        if (wasRescheduled) {
            if (!m_compositor->m_compositingRunLoop->isActive())
                m_compositor->m_compositingRunLoop->scheduleUpdate();
        }
    }
}

} // namespace WebKit

#endif // USE(COORDINATED_GRAPHICS_THREADED) && USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
