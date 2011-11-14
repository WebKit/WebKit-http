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

#ifndef WebLayerTreeViewImpl_h
#define WebLayerTreeViewImpl_h

#include "WebLayerTreeView.h"
#include "cc/CCLayerTreeHost.h"
#include <wtf/PassRefPtr.h>

namespace WebKit {
class WebLayer;
class WebLayerTreeViewClient;

class WebLayerTreeViewImpl : public WebCore::CCLayerTreeHost, public WebCore::CCLayerTreeHostClient {
public:
    static PassRefPtr<WebLayerTreeViewImpl> create(WebLayerTreeViewClient*, const WebLayer& root, const WebLayerTreeView::Settings&);

private:
    WebLayerTreeViewImpl(WebLayerTreeViewClient*, const WebLayer& root, const WebLayerTreeView::Settings&);
    virtual ~WebLayerTreeViewImpl();
    virtual void animateAndLayout(double frameBeginTime);
    virtual void applyScrollAndScale(const WebCore::IntSize& scrollDelta, float pageScale);
    virtual PassRefPtr<WebCore::GraphicsContext3D> createLayerTreeHostContext3D();
    virtual void didRecreateGraphicsContext(bool success);
    virtual void didCommitAndDrawFrame();
    virtual void didCompleteSwapBuffers();

    // Only used in the single threaded path.
    virtual void scheduleComposite();

    WebLayerTreeViewClient* m_client;
};

} // namespace WebKit

#endif // WebLayerTreeViewImpl_h
