/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2011 Motorola Mobility, Inc.  All rights reserved.
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
#include "WebInspectorProxy.h"

#if ENABLE(INSPECTOR)

#include "WebFramePolicyListenerProxy.h"
#include "WebFrameProxy.h"
#include "WebInspectorMessages.h"
#include "WebPageCreationParameters.h"
#include "WebPageGroup.h"
#include "WebPageProxy.h"
#include "WebPreferences.h"
#include "WebProcessProxy.h"
#include "WebURLRequest.h"

#if ENABLE(INSPECTOR_SERVER)
#include "WebInspectorServer.h"
#endif
#if PLATFORM(WIN)
#include "WebView.h"
#endif

using namespace WebCore;

namespace WebKit {

const unsigned WebInspectorProxy::minimumWindowWidth = 500;
const unsigned WebInspectorProxy::minimumWindowHeight = 400;

const unsigned WebInspectorProxy::initialWindowWidth = 750;
const unsigned WebInspectorProxy::initialWindowHeight = 650;

const unsigned WebInspectorProxy::minimumAttachedHeight = 250;

static PassRefPtr<WebPageGroup> createInspectorPageGroup()
{
    RefPtr<WebPageGroup> pageGroup = WebPageGroup::create("__WebInspectorPageGroup__", false, false);

#ifndef NDEBUG
    // Allow developers to inspect the Web Inspector in debug builds.
    pageGroup->preferences()->setDeveloperExtrasEnabled(true);
#endif

    pageGroup->preferences()->setApplicationChromeModeEnabled(true);
    pageGroup->preferences()->setSuppressesIncrementalRendering(true);

    return pageGroup.release();
}

WebPageGroup* WebInspectorProxy::inspectorPageGroup()
{
    static WebPageGroup* pageGroup = createInspectorPageGroup().leakRef();
    return pageGroup;
}

WebInspectorProxy::WebInspectorProxy(WebPageProxy* page)
    : m_page(page)
    , m_isVisible(false)
    , m_isAttached(false)
    , m_isDebuggingJavaScript(false)
    , m_isProfilingJavaScript(false)
    , m_isProfilingPage(false)
#if PLATFORM(WIN)
    , m_inspectorWindow(0)
#elif PLATFORM(GTK) || PLATFORM(EFL)
    , m_inspectorView(0)
    , m_inspectorWindow(0)
#endif
#if ENABLE(INSPECTOR_SERVER)
    , m_remoteInspectionPageId(0)
#endif
{
}

WebInspectorProxy::~WebInspectorProxy()
{
}

void WebInspectorProxy::invalidate()
{
#if ENABLE(INSPECTOR_SERVER)
    if (m_remoteInspectionPageId)
        WebInspectorServer::shared().unregisterPage(m_remoteInspectionPageId);
#endif

    m_page->close();
    didClose();

    m_page = 0;

    m_isVisible = false;
    m_isDebuggingJavaScript = false;
    m_isProfilingJavaScript = false;
    m_isProfilingPage = false;
}

// Public APIs
bool WebInspectorProxy::isFront()
{
    if (!m_page)
        return false;

    return platformIsFront();
}

void WebInspectorProxy::show()
{
    if (!m_page)
        return;

    m_page->process()->send(Messages::WebInspector::Show(), m_page->pageID());
}

void WebInspectorProxy::close()
{
    if (!m_page)
        return;

    m_page->process()->send(Messages::WebInspector::Close(), m_page->pageID());
}

void WebInspectorProxy::showConsole()
{
    if (!m_page)
        return;

    m_page->process()->send(Messages::WebInspector::ShowConsole(), m_page->pageID());
}

void WebInspectorProxy::showResources()
{
    if (!m_page)
        return;

    m_page->process()->send(Messages::WebInspector::ShowResources(), m_page->pageID());
}

void WebInspectorProxy::showMainResourceForFrame(WebFrameProxy* frame)
{
    if (!m_page)
        return;
    
    m_page->process()->send(Messages::WebInspector::ShowMainResourceForFrame(frame->frameID()), m_page->pageID());
}

void WebInspectorProxy::attach()
{
    if (!canAttach())
        return;

    m_isAttached = true;

    if (m_isVisible)
        inspectorPageGroup()->preferences()->setInspectorStartsAttached(true);

    platformAttach();
}

void WebInspectorProxy::detach()
{
    m_isAttached = false;
    
    if (m_isVisible)
        inspectorPageGroup()->preferences()->setInspectorStartsAttached(false);

    platformDetach();
}

void WebInspectorProxy::setAttachedWindowHeight(unsigned height)
{
    inspectorPageGroup()->preferences()->setInspectorAttachedHeight(height);
    platformSetAttachedWindowHeight(height);
}

void WebInspectorProxy::toggleJavaScriptDebugging()
{
    if (!m_page)
        return;

    if (m_isDebuggingJavaScript)
        m_page->process()->send(Messages::WebInspector::StopJavaScriptDebugging(), m_page->pageID());
    else
        m_page->process()->send(Messages::WebInspector::StartJavaScriptDebugging(), m_page->pageID());

    // FIXME: have the WebProcess notify us on state changes.
    m_isDebuggingJavaScript = !m_isDebuggingJavaScript;
}

void WebInspectorProxy::toggleJavaScriptProfiling()
{
    if (!m_page)
        return;

    if (m_isProfilingJavaScript)
        m_page->process()->send(Messages::WebInspector::StopJavaScriptProfiling(), m_page->pageID());
    else
        m_page->process()->send(Messages::WebInspector::StartJavaScriptProfiling(), m_page->pageID());

    // FIXME: have the WebProcess notify us on state changes.
    m_isProfilingJavaScript = !m_isProfilingJavaScript;
}

void WebInspectorProxy::togglePageProfiling()
{
    if (!m_page)
        return;

    if (m_isProfilingPage)
        m_page->process()->send(Messages::WebInspector::StopPageProfiling(), m_page->pageID());
    else
        m_page->process()->send(Messages::WebInspector::StartPageProfiling(), m_page->pageID());

    // FIXME: have the WebProcess notify us on state changes.
    m_isProfilingPage = !m_isProfilingPage;
}

bool WebInspectorProxy::isInspectorPage(WebPageProxy* page)
{
    return page->pageGroup() == inspectorPageGroup();
}

static void decidePolicyForNavigationAction(WKPageRef, WKFrameRef frameRef, WKFrameNavigationType, WKEventModifiers, WKEventMouseButton, WKURLRequestRef requestRef, WKFramePolicyListenerRef listenerRef, WKTypeRef, const void* clientInfo)
{
    // Allow non-main frames to navigate anywhere.
    if (!toImpl(frameRef)->isMainFrame()) {
        toImpl(listenerRef)->use();
        return;
    }

    const WebInspectorProxy* webInspectorProxy = static_cast<const WebInspectorProxy*>(clientInfo);
    ASSERT(webInspectorProxy);

    // Use KURL so we can compare just the fileSystemPaths.
    KURL inspectorURL(KURL(), webInspectorProxy->inspectorPageURL());
    KURL requestURL(KURL(), toImpl(requestRef)->url());

    ASSERT(inspectorURL.isLocalFile());

    // Allow loading of the main inspector file.
    if (requestURL.isLocalFile() && requestURL.fileSystemPath() == inspectorURL.fileSystemPath()) {
        toImpl(listenerRef)->use();
        return;
    }

    // Prevent everything else from loading in the inspector's page.
    toImpl(listenerRef)->ignore();

    // And instead load it in the inspected page.
    webInspectorProxy->page()->loadURLRequest(toImpl(requestRef));
}

#if ENABLE(INSPECTOR_SERVER)
void WebInspectorProxy::enableRemoteInspection()
{
    if (!m_remoteInspectionPageId)
        m_remoteInspectionPageId = WebInspectorServer::shared().registerPage(this);
}

void WebInspectorProxy::remoteFrontendConnected()
{
    m_page->process()->send(Messages::WebInspector::RemoteFrontendConnected(), m_page->pageID());
}

void WebInspectorProxy::remoteFrontendDisconnected()
{
    m_page->process()->send(Messages::WebInspector::RemoteFrontendDisconnected(), m_page->pageID());
}

void WebInspectorProxy::dispatchMessageFromRemoteFrontend(const String& message)
{
    m_page->process()->send(Messages::WebInspector::DispatchMessageFromRemoteFrontend(message), m_page->pageID());
}
#endif

// Called by WebInspectorProxy messages
void WebInspectorProxy::createInspectorPage(uint64_t& inspectorPageID, WebPageCreationParameters& inspectorPageParameters)
{
    inspectorPageID = 0;

    if (!m_page)
        return;

    m_isAttached = shouldOpenAttached();

    WebPageProxy* inspectorPage = platformCreateInspectorPage();
    ASSERT(inspectorPage);
    if (!inspectorPage)
        return;

    inspectorPageID = inspectorPage->pageID();
    inspectorPageParameters = inspectorPage->creationParameters();

    WKPagePolicyClient policyClient = {
        kWKPagePolicyClientCurrentVersion,
        this, /* clientInfo */
        decidePolicyForNavigationAction,
        0, /* decidePolicyForNewWindowAction */
        0, /* decidePolicyForResponse */
        0 /* unableToImplementPolicy */
    };

    inspectorPage->initializePolicyClient(&policyClient);

    String url = inspectorPageURL();
    if (m_isAttached)
        url.append("?docked=true");

    m_page->process()->assumeReadAccessToBaseURL(inspectorBaseURL());

    inspectorPage->loadURL(url);
}

void WebInspectorProxy::didLoadInspectorPage()
{
    m_isVisible = true;

    // platformOpen is responsible for rendering attached mode depending on m_isAttached.
    platformOpen();
}

void WebInspectorProxy::didClose()
{
    m_isVisible = false;
    m_isDebuggingJavaScript = false;
    m_isProfilingJavaScript = false;
    m_isProfilingPage = false;

    if (m_isAttached) {
        // Detach here so we only need to have one code path that is responsible for cleaning up the inspector
        // state.
        detach();
    }

    platformDidClose();
}

void WebInspectorProxy::bringToFront()
{
    platformBringToFront();
}

void WebInspectorProxy::inspectedURLChanged(const String& urlString)
{
    platformInspectedURLChanged(urlString);
}

bool WebInspectorProxy::canAttach()
{
    // Keep this in sync with InspectorFrontendClientLocal::canAttachWindow. There are two implementations
    // to make life easier in the multi-process world we have. WebInspectorProxy uses canAttach to decide if
    // we can attach on open (on the UI process side). And InspectorFrontendClientLocal::canAttachWindow is
    // used to decide if we can attach when the attach button is pressed (on the WebProcess side).

    // Don't allow the attach if the window would be too small to accommodate the minimum inspector height.
    // Also don't allow attaching to another inspector -- two inspectors in one window is too much!
    bool isInspectorPage = m_page->pageGroup() == inspectorPageGroup();
    unsigned inspectedPageHeight = platformInspectedWindowHeight();
    unsigned maximumAttachedHeight = inspectedPageHeight * 3 / 4;
    return minimumAttachedHeight <= maximumAttachedHeight && !isInspectorPage;
}

bool WebInspectorProxy::shouldOpenAttached()
{
    return inspectorPageGroup()->preferences()->inspectorStartsAttached() && canAttach();
}

#if ENABLE(INSPECTOR_SERVER)
void WebInspectorProxy::sendMessageToRemoteFrontend(const String& message)
{
    ASSERT(m_remoteInspectionPageId);
    WebInspectorServer::shared().sendMessageOverConnection(m_remoteInspectionPageId, message);
}
#endif

} // namespace WebKit

#endif // ENABLE(INSPECTOR)
