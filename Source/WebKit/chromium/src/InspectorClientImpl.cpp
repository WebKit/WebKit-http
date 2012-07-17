/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#include "config.h"
#include "InspectorClientImpl.h"

#include "DOMWindow.h"
#include "FloatRect.h"
#include "InspectorInstrumentation.h"
#include "NotImplemented.h"
#include "Page.h"
#include "WebDevToolsAgentImpl.h"
#include "platform/WebRect.h"
#include "platform/WebURL.h"
#include "platform/WebURLRequest.h"
#include "WebViewClient.h"
#include "WebViewImpl.h"
#include <public/Platform.h>
#include <wtf/Vector.h>

using namespace WebCore;

namespace WebKit {

InspectorClientImpl::InspectorClientImpl(WebViewImpl* webView)
    : m_inspectedWebView(webView)
{
    ASSERT(m_inspectedWebView);
}

InspectorClientImpl::~InspectorClientImpl()
{
}

void InspectorClientImpl::inspectorDestroyed()
{
    if (WebDevToolsAgentImpl* agent = devToolsAgent())
        agent->inspectorDestroyed();
}

InspectorFrontendChannel* InspectorClientImpl::openInspectorFrontend(InspectorController* controller)
{
    if (WebDevToolsAgentImpl* agent = devToolsAgent())
        return agent->openInspectorFrontend(controller);
    return 0;
}

void InspectorClientImpl::closeInspectorFrontend()
{
    if (WebDevToolsAgentImpl* agent = devToolsAgent())
        agent->closeInspectorFrontend();
}

void InspectorClientImpl::bringFrontendToFront()
{
    if (WebDevToolsAgentImpl* agent = devToolsAgent())
        agent->bringFrontendToFront();
}

void InspectorClientImpl::highlight()
{
    if (WebDevToolsAgentImpl* agent = devToolsAgent())
        agent->highlight();
}

void InspectorClientImpl::hideHighlight()
{
    if (WebDevToolsAgentImpl* agent = devToolsAgent())
        agent->hideHighlight();
}

bool InspectorClientImpl::sendMessageToFrontend(const WTF::String& message)
{
    if (WebDevToolsAgentImpl* agent = devToolsAgent())
        return agent->sendMessageToFrontend(message);
    return false;
}

void InspectorClientImpl::updateInspectorStateCookie(const WTF::String& inspectorState)
{
    if (WebDevToolsAgentImpl* agent = devToolsAgent())
        agent->updateInspectorStateCookie(inspectorState);
}

bool InspectorClientImpl::canClearBrowserCache()
{
    return true;
}

void InspectorClientImpl::clearBrowserCache()
{
    if (WebDevToolsAgentImpl* agent = devToolsAgent())
        agent->clearBrowserCache();
}

bool InspectorClientImpl::canClearBrowserCookies()
{
    return true;
}

void InspectorClientImpl::clearBrowserCookies()
{
    if (WebDevToolsAgentImpl* agent = devToolsAgent())
        agent->clearBrowserCookies();
}

void InspectorClientImpl::startMainThreadMonitoring()
{
    WebKit::Platform::current()->currentThread()->addTaskObserver(this);
}

void InspectorClientImpl::stopMainThreadMonitoring()
{
    WebKit::Platform::current()->currentThread()->removeTaskObserver(this);
}

bool InspectorClientImpl::canOverrideDeviceMetrics()
{
    return true;
}

void InspectorClientImpl::overrideDeviceMetrics(int width, int height, float fontScaleFactor, bool fitWindow)
{
    if (WebDevToolsAgentImpl* agent = devToolsAgent())
        agent->overrideDeviceMetrics(width, height, fontScaleFactor, fitWindow);
}

void InspectorClientImpl::autoZoomPageToFitWidth()
{
    if (WebDevToolsAgentImpl* agent = devToolsAgent())
        agent->autoZoomPageToFitWidth();
}

bool InspectorClientImpl::supportsFrameInstrumentation()
{
    return true;
}

void InspectorClientImpl::willProcessTask()
{
    InspectorInstrumentation::willProcessTask(m_inspectedWebView->page());
}

void InspectorClientImpl::didProcessTask()
{
    InspectorInstrumentation::didProcessTask(m_inspectedWebView->page());
}

WebDevToolsAgentImpl* InspectorClientImpl::devToolsAgent()
{
    return static_cast<WebDevToolsAgentImpl*>(m_inspectedWebView->devToolsAgent());
}

} // namespace WebKit
