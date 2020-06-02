/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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

#import "config.h"
#import "RemoteLayerTreeDisplayRefreshMonitor.h"

namespace WebKit {
using namespace WebCore;

RemoteLayerTreeDisplayRefreshMonitor::RemoteLayerTreeDisplayRefreshMonitor(PlatformDisplayID displayID, RemoteLayerTreeDrawingArea& drawingArea)
    : DisplayRefreshMonitor(displayID)
    , m_drawingArea(makeWeakPtr(drawingArea))
{
}

RemoteLayerTreeDisplayRefreshMonitor::~RemoteLayerTreeDisplayRefreshMonitor()
{
    if (m_drawingArea)
        m_drawingArea->willDestroyDisplayRefreshMonitor(this);
}

void RemoteLayerTreeDisplayRefreshMonitor::setPreferredFramesPerSecond(FramesPerSecond preferredFramesPerSecond)
{
    if (m_drawingArea)
        m_drawingArea->setPreferredFramesPerSecond(preferredFramesPerSecond);
}

bool RemoteLayerTreeDisplayRefreshMonitor::requestRefreshCallback()
{
    if (!m_drawingArea || !isActive())
        return false;

    if (!isScheduled())
        static_cast<DrawingArea&>(*m_drawingArea.get()).scheduleRenderingUpdate();

    setIsActive(true);
    setIsScheduled(true);
    return true;
}

void RemoteLayerTreeDisplayRefreshMonitor::didUpdateLayers()
{
    setIsScheduled(false);

    if (!isPreviousFrameDone())
        return;

    setIsPreviousFrameDone(false);
    handleDisplayRefreshedNotificationOnMainThread(this);
}

void RemoteLayerTreeDisplayRefreshMonitor::updateDrawingArea(RemoteLayerTreeDrawingArea& drawingArea)
{
    m_drawingArea = makeWeakPtr(drawingArea);
}

}
