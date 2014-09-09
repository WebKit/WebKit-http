/*
 * Copyright (C) 2010, 2014 Apple Inc. All rights reserved.
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
#include "DrawingAreaProxy.h"

#include "DrawingAreaMessages.h"
#include "DrawingAreaProxyMessages.h"
#include "WebPageProxy.h"
#include "WebProcessProxy.h"

using namespace WebCore;

namespace WebKit {

DrawingAreaProxy::DrawingAreaProxy(DrawingAreaType type, WebPageProxy& webPageProxy)
    : m_type(type)
    , m_webPageProxy(webPageProxy)
    , m_size(webPageProxy.viewSize())
#if PLATFORM(MAC)
    , m_exposedRectChangedTimer(RunLoop::main(), this, &DrawingAreaProxy::exposedRectChangedTimerFired)
#endif
{
    m_webPageProxy.process().addMessageReceiver(Messages::DrawingAreaProxy::messageReceiverName(), m_webPageProxy.pageID(), *this);
}

DrawingAreaProxy::~DrawingAreaProxy()
{
    m_webPageProxy.process().removeMessageReceiver(Messages::DrawingAreaProxy::messageReceiverName(), m_webPageProxy.pageID());
}

void DrawingAreaProxy::setSize(const IntSize& size, const IntSize& layerPosition, const IntSize& scrollOffset)
{ 
    if (m_size == size && m_layerPosition == layerPosition && scrollOffset.isZero())
        return;

    m_size = size;
    m_layerPosition = layerPosition;
    m_scrollOffset += scrollOffset;
    sizeDidChange();
}

#if PLATFORM(MAC)
void DrawingAreaProxy::setExposedRect(const FloatRect& exposedRect)
{
    if (!m_webPageProxy.isValid())
        return;

    m_exposedRect = exposedRect;

    if (!m_exposedRectChangedTimer.isActive())
        m_exposedRectChangedTimer.startOneShot(0);
}

void DrawingAreaProxy::exposedRectChangedTimerFired()
{
    if (!m_webPageProxy.isValid())
        return;

    if (m_exposedRect == m_lastSentExposedRect)
        return;

    m_webPageProxy.process().send(Messages::DrawingArea::SetExposedRect(m_exposedRect), m_webPageProxy.pageID());
    m_lastSentExposedRect = m_exposedRect;
}
#endif // PLATFORM(MAC)

} // namespace WebKit
