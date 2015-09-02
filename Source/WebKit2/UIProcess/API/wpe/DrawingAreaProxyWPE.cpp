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
#include "DrawingAreaProxyWPE.h"

#include "DrawingAreaMessages.h"
#include "WPEView.h"
#include "WebPageProxy.h"
#include "WebProcessProxy.h"

namespace WebKit {

DrawingAreaProxyWPE::DrawingAreaProxyWPE(WKWPE::View& view)
    : DrawingAreaProxy(DrawingAreaTypeWPE, view.page())
    , m_viewBackend(view.viewBackend())
{
    m_viewBackend.setClient(this);
}

DrawingAreaProxyWPE::~DrawingAreaProxyWPE()
{
}

void DrawingAreaProxyWPE::deviceScaleFactorDidChange()
{
    fprintf(stderr, "DrawingAreaProxyWPE: %s\n", __func__);
}

void DrawingAreaProxyWPE::sizeDidChange()
{
    fprintf(stderr, "DrawingAreaProxyWPE: %s\n", __func__);
}

void DrawingAreaProxyWPE::releaseBuffer(uint32_t handle)
{
    m_webPageProxy.process().send(Messages::DrawingArea::ReleaseBuffer(handle), m_webPageProxy.pageID());
}

void DrawingAreaProxyWPE::frameComplete()
{
    m_webPageProxy.process().send(Messages::DrawingArea::FrameComplete(), m_webPageProxy.pageID());
}

void DrawingAreaProxyWPE::update(uint64_t backingStoreStateID, const UpdateInfo&)
{
    fprintf(stderr, "DrawingAreaProxyWPE: %s\n", __func__);
}

void DrawingAreaProxyWPE::didUpdateBackingStoreState(uint64_t backingStoreStateID, const UpdateInfo&, const LayerTreeContext&)
{
    fprintf(stderr, "DrawingAreaProxyWPE: %s\n", __func__);
}

void DrawingAreaProxyWPE::enterAcceleratedCompositingMode(uint64_t backingStoreStateID, const LayerTreeContext& layerTreeContext)
{
    fprintf(stderr, "DrawingAreaProxyWPE: %s\n", __func__);
    ASSERT(!backingStoreID);

    m_layerTreeContext = layerTreeContext;
    m_webPageProxy.enterAcceleratedCompositingMode(layerTreeContext);
}

void DrawingAreaProxyWPE::exitAcceleratedCompositingMode(uint64_t backingStoreStateID, const UpdateInfo&)
{
    fprintf(stderr, "DrawingAreaProxyWPE: %s\n", __func__);
}

void DrawingAreaProxyWPE::updateAcceleratedCompositingMode(uint64_t backingStoreStateID, const LayerTreeContext&)
{
    fprintf(stderr, "DrawingAreaProxyWPE: %s\n", __func__);
}

void DrawingAreaProxyWPE::commitPrimeFD(uint32_t handle, uint32_t width, uint32_t height, uint32_t stride, uint32_t format, IPC::Attachment fd)
{
    m_viewBackend.commitPrimeFD(fd.fileDescriptor(), handle, width, height, stride, format);
}

void DrawingAreaProxyWPE::destroyPrimeBuffer(uint32_t handle)
{
    m_viewBackend.destroyPrimeBuffer(handle);
}

} // namespace WebKit
