/*
 * Copyright (C) 2011 Samsung Electronics
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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
#include "PageClientLegacyImpl.h"

#include "EwkViewImpl.h"
#include "LayerTreeCoordinatorProxy.h"
#include "NotImplemented.h"

using namespace WebCore;
using namespace EwkViewCallbacks;

namespace WebKit {

PageClientLegacyImpl::PageClientLegacyImpl(EwkViewImpl* viewImpl)
    : PageClientBase(viewImpl)
{
}

void PageClientLegacyImpl::didCommitLoad()
{
    m_viewImpl->update();
}

void PageClientLegacyImpl::updateViewportSize(const WebCore::IntSize& size)
{
#if USE(TILED_BACKING_STORE)
    m_viewImpl->page()->drawingArea()->setVisibleContentsRect(IntRect(m_viewImpl->discretePagePosition(), size), m_viewImpl->scaleFactor(), FloatPoint());
#else
    UNUSED_PARAM(size);
#endif
}

FloatRect PageClientLegacyImpl::convertToDeviceSpace(const FloatRect& viewRect)
{
    notImplemented();
    return viewRect;
}

FloatRect PageClientLegacyImpl::convertToUserSpace(const FloatRect& viewRect)
{
    notImplemented();
    return viewRect;
}

void PageClientLegacyImpl::didChangeViewportProperties(const WebCore::ViewportAttributes&)
{
    m_viewImpl->update();
}

void PageClientLegacyImpl::didChangeContentsSize(const WebCore::IntSize& size)
{
#if USE(TILED_BACKING_STORE)
    // m_viewImpl->informContentSizeChanged will be called as a result of setContentsSize
    m_viewImpl->page()->drawingArea()->layerTreeCoordinatorProxy()->setContentsSize(FloatSize(size.width(), size.height()));
    m_viewImpl->update();
#else
    m_viewImpl->informContentsSizeChange(size);
#endif
}

#if USE(TILED_BACKING_STORE)
void PageClientLegacyImpl::pageDidRequestScroll(const IntPoint& position)
{
    m_viewImpl->setPagePosition(FloatPoint(position));
    m_viewImpl->update();
}

void PageClientLegacyImpl::didRenderFrame(const WebCore::IntSize&, const WebCore::IntRect&)
{
    m_viewImpl->update();
}

void PageClientLegacyImpl::pageTransitionViewportReady()
{
    m_viewImpl->update();
}
#endif

} // namespace WebKit
