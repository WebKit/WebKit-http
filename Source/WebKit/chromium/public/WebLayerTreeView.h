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

#ifndef WebLayerTreeView_h
#define WebLayerTreeView_h

#include "WebCommon.h"
#include "WebPrivatePtr.h"

namespace WebCore {
class CCLayerTreeHost;
struct CCSettings;
}

namespace WebKit {
class WebLayer;
class WebLayerTreeViewClient;
struct WebSize;

class WebLayerTreeView {
public:
    struct Settings {
        Settings()
            : acceleratePainting(false)
            , compositeOffscreen(false)
            , enableCompositorThread(false) { }

        bool acceleratePainting;
        bool compositeOffscreen;
        bool enableCompositorThread;
#if WEBKIT_IMPLEMENTATION
        operator WebCore::CCSettings() const;
#endif
    };

    static WebLayerTreeView create(WebLayerTreeViewClient*, const WebLayer& root, const Settings&);

    WebLayerTreeView() { }
    WebLayerTreeView(const WebLayerTreeView& layer) { assign(layer); }
    ~WebLayerTreeView() { reset(); }
    WebLayerTreeView& operator=(const WebLayerTreeView& layer)
    {
        assign(layer);
        return *this;
    }
    WEBKIT_EXPORT void reset();
    WEBKIT_EXPORT void assign(const WebLayerTreeView&);
    WEBKIT_EXPORT bool equals(const WebLayerTreeView&) const;

    // Triggers a compositing pass. If the compositor thread is disabled
    // (enableCompositorThread == false), the compositing pass happens
    // immediately. If it is enabled, the compositing pass will happen at a
    // later time. Before the compositing pass happens (i.e. before composite()
    // returns when the compositor thread is disabled), WebContentLayers will be
    // asked to paint their dirty region, through
    // WebContentLayerClient::paintContents.
    WEBKIT_EXPORT void composite();

    WEBKIT_EXPORT void setViewportSize(const WebSize&);
    WEBKIT_EXPORT WebSize viewportSize() const;

#if WEBKIT_IMPLEMENTATION
    WebLayerTreeView(const WTF::PassRefPtr<WebCore::CCLayerTreeHost>&);
    WebLayerTreeView& operator=(const WTF::PassRefPtr<WebCore::CCLayerTreeHost>&);
    operator WTF::PassRefPtr<WebCore::CCLayerTreeHost>() const;
#endif

protected:
    WebPrivatePtr<WebCore::CCLayerTreeHost> m_private;
};

inline bool operator==(const WebLayerTreeView& a, const WebLayerTreeView& b)
{
    return a.equals(b);
}

inline bool operator!=(const WebLayerTreeView& a, const WebLayerTreeView& b)
{
    return !(a == b);
}

} // namespace WebKit

#endif // WebLayerTreeView_h
