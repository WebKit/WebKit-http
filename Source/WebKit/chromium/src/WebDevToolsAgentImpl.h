/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebDevToolsAgentImpl_h
#define WebDevToolsAgentImpl_h

#include "InspectorClient.h"

#include "PageOverlay.h"
#include "WebDevToolsAgentPrivate.h"

#include <wtf/Forward.h>
#include <wtf/OwnPtr.h>

namespace WebCore {
class Document;
class Frame;
class GraphicsContext;
class InspectorClient;
class InspectorController;
class Node;
}

namespace WebKit {

class DebuggerAgentImpl;
class WebDevToolsAgentClient;
class WebFrame;
class WebFrameImpl;
class WebString;
class WebURLRequest;
class WebURLResponse;
class WebViewImpl;
struct WebURLError;
struct WebDevToolsMessageData;

class WebDevToolsAgentImpl : public WebDevToolsAgentPrivate,
                             public WebCore::InspectorClient,
                             public PageOverlay::PageOverlayClient {
public:
    WebDevToolsAgentImpl(WebViewImpl* webViewImpl, WebDevToolsAgentClient* client);
    virtual ~WebDevToolsAgentImpl();

    // WebDevToolsAgentPrivate implementation.
    virtual void didClearWindowObject(WebFrameImpl* frame);

    // WebDevToolsAgent implementation.
    virtual void attach();
    virtual void detach();
    virtual void frontendLoaded();
    virtual void didNavigate();
    virtual void dispatchOnInspectorBackend(const WebString& message);
    virtual void inspectElementAt(const WebPoint& point);
    virtual void evaluateInWebInspector(long callId, const WebString& script);
    virtual void setJavaScriptProfilingEnabled(bool);
    virtual void setRuntimeProperty(const WebString& name, const WebString& value);
    virtual void setProcessId(long);

    // InspectorClient implementation.
    virtual void inspectorDestroyed();
    virtual void openInspectorFrontend(WebCore::InspectorController*);
    virtual void highlight();
    virtual void hideHighlight();
    virtual void updateInspectorStateCookie(const WTF::String&);
    virtual bool sendMessageToFrontend(const WTF::String&);

    virtual void clearBrowserCache();
    virtual void clearBrowserCookies();

    int hostId() { return m_hostId; }

    // PageOverlayClient
    virtual void paintPageOverlay(WebCore::GraphicsContext&);

private:
    WebCore::InspectorController* inspectorController();
    WebCore::Frame* mainFrame();

    int m_hostId;
    WebDevToolsAgentClient* m_client;
    WebViewImpl* m_webViewImpl;
    OwnPtr<DebuggerAgentImpl> m_debuggerAgentImpl;
    bool m_attached;
};

} // namespace WebKit

#endif
