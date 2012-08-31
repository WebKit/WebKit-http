/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
#include "WebIOSurfaceLayerImpl.h"

#include "IOSurfaceLayerChromium.h"
#include "WebLayerImpl.h"

using WebCore::IOSurfaceLayerChromium;

namespace WebKit {

WebIOSurfaceLayer* WebIOSurfaceLayer::create()
{
    return new WebIOSurfaceLayerImpl();
}

WebIOSurfaceLayerImpl::WebIOSurfaceLayerImpl()
    : m_layer(adoptPtr(new WebLayerImpl(IOSurfaceLayerChromium::create())))
{
    m_layer->layer()->setIsDrawable(true);
}

WebIOSurfaceLayerImpl::~WebIOSurfaceLayerImpl()
{
}

void WebIOSurfaceLayerImpl::setIOSurfaceProperties(unsigned ioSurfaceId, WebSize size)
{
    static_cast<IOSurfaceLayerChromium*>(m_layer->layer())->setIOSurfaceProperties(ioSurfaceId, size);
}

WebLayer* WebIOSurfaceLayerImpl::layer()
{
    return m_layer.get();
}

} // namespace WebKit
