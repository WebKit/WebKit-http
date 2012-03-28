/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
#include "WebLayerTreeViewImpl.h"

#include "CCThreadImpl.h"
#include "GraphicsContext3DPrivate.h"
#include "LayerChromium.h"
#include "cc/CCThreadProxy.h"
#include "platform/WebGraphicsContext3D.h"
#include "platform/WebLayer.h"
#include "platform/WebLayerTreeView.h"
#include "platform/WebLayerTreeViewClient.h"
#include "platform/WebSize.h"
#include "platform/WebThread.h"

using namespace WebCore;

namespace WebKit {

PassRefPtr<WebLayerTreeViewImpl> WebLayerTreeViewImpl::create(WebLayerTreeViewClient* client, const WebLayer& root, const WebLayerTreeView::Settings& settings)
{
    RefPtr<WebLayerTreeViewImpl> host = adoptRef(new WebLayerTreeViewImpl(client, settings));
    if (!host->initialize())
        return 0;
    host->setRootLayer(root);
    return host;
}

WebLayerTreeViewImpl::WebLayerTreeViewImpl(WebLayerTreeViewClient* client, const WebLayerTreeView::Settings& settings) 
    : CCLayerTreeHost(this, settings)
    , m_client(client)
{
}

WebLayerTreeViewImpl::~WebLayerTreeViewImpl()
{
}

void WebLayerTreeViewImpl::willBeginFrame()
{
    if (m_client)
        m_client->willBeginFrame();
}

void WebLayerTreeViewImpl::updateAnimations(double monotonicFrameBeginTime)
{
    if (m_client)
        m_client->updateAnimations(monotonicFrameBeginTime);
}

void WebLayerTreeViewImpl::layout()
{
    if (m_client)
        m_client->layout();
}

void WebLayerTreeViewImpl::applyScrollAndScale(const WebCore::IntSize& scrollDelta, float pageScale)
{
    if (m_client)
        m_client->applyScrollAndScale(WebSize(scrollDelta), pageScale);
}

PassRefPtr<GraphicsContext3D> WebLayerTreeViewImpl::createContext()
{
    if (!m_client)
        return 0;
    OwnPtr<WebGraphicsContext3D> webContext = adoptPtr(m_client->createContext3D());
    if (!webContext)
        return 0;

    return GraphicsContext3DPrivate::createGraphicsContextFromWebContext(webContext.release(), GraphicsContext3D::RenderDirectlyToHostWindow, false /* preserveDrawingBuffer */ );
}

void WebLayerTreeViewImpl::didRecreateContext(bool success)
{
    if (m_client)
        m_client->didRebindGraphicsContext(success);
}

void WebLayerTreeViewImpl::didCommit()
{
    if (m_client)
        m_client->didCommit();
}

void WebLayerTreeViewImpl::didCommitAndDrawFrame()
{
    if (m_client)
        m_client->didCommitAndDrawFrame();
}

void WebLayerTreeViewImpl::didCompleteSwapBuffers()
{
    if (m_client)
        m_client->didCompleteSwapBuffers();
}

void WebLayerTreeViewImpl::scheduleComposite()
{
    if (m_client)
        m_client->scheduleComposite();
}

} // namespace WebKit
