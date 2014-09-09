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

#include "APIURLRequest.h"
#include "WebFramePolicyListenerProxy.h"
#include "WebFrameProxy.h"
#include "WebInspectorMessages.h"
#include "WebInspectorProxyMessages.h"
#include "WebPageCreationParameters.h"
#include "WebPageGroup.h"
#include "WebPageProxy.h"
#include "WebPreferences.h"
#include "WebProcessProxy.h"
#include <WebCore/SchemeRegistry.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/text/StringBuilder.h>

#if ENABLE(INSPECTOR_SERVER)
#include "WebInspectorServer.h"
#endif

using namespace WebCore;

namespace WebKit {

const unsigned WebInspectorProxy::minimumWindowWidth = 750;
const unsigned WebInspectorProxy::minimumWindowHeight = 400;

const unsigned WebInspectorProxy::initialWindowWidth = 1000;
const unsigned WebInspectorProxy::initialWindowHeight = 650;

const unsigned WebInspectorProxy::minimumAttachedWidth = 750;
const unsigned WebInspectorProxy::minimumAttachedHeight = 250;

class WebInspectorPageGroups {
public:
    static WebInspectorPageGroups& shared()
    {
        static NeverDestroyed<WebInspectorPageGroups> instance;
        return instance;
    }

    unsigned inspectorLevel(WebPageGroup& inspectedPageGroup)
    {
        return isInspectorPageGroup(inspectedPageGroup) ? inspectorPageGroupLevel(inspectedPageGroup) + 1 : 1;
    }

    bool isInspectorPageGroup(WebPageGroup& group)
    {
        return m_pageGroupLevel.contains(&group);
    }

    unsigned inspectorPageGroupLevel(WebPageGroup& group)
    {
        ASSERT(isInspectorPageGroup(group));
        return m_pageGroupLevel.get(&group);
    }

    WebPageGroup* inspectorPageGroupForLevel(unsigned level)
    {
        // The level is the key of the HashMap, so it cannot be 0.
        ASSERT(level);

        auto iterator = m_pageGroupByLevel.find(level);
        if (iterator != m_pageGroupByLevel.end())
            return iterator->value.get();

        RefPtr<WebPageGroup> group = createInspectorPageGroup(level);
        m_pageGroupByLevel.set(level, group.get());
        m_pageGroupLevel.set(group.get(), level);
        return group.get();
    }

private:
    static PassRefPtr<WebPageGroup> createInspectorPageGroup(unsigned level)
    {
        RefPtr<WebPageGroup> pageGroup = WebPageGroup::create(String::format("__WebInspectorPageGroupLevel%u__", level), false, false);

#ifndef NDEBUG
        // Allow developers to inspect the Web Inspector in debug builds.
        pageGroup->preferences().setDeveloperExtrasEnabled(true);
        pageGroup->preferences().setLogsPageMessagesToSystemConsoleEnabled(true);
#endif

        pageGroup->preferences().setApplicationChromeModeEnabled(true);

        return pageGroup.release();
    }

    typedef HashMap<unsigned, RefPtr<WebPageGroup> > PageGroupByLevelMap;
    typedef HashMap<WebPageGroup*, unsigned> PageGroupLevelMap;

    PageGroupByLevelMap m_pageGroupByLevel;
    PageGroupLevelMap m_pageGroupLevel;
};

WebInspectorProxy::WebInspectorProxy(WebPageProxy* page)
    : m_page(page)
    , m_isVisible(false)
    , m_isAttached(false)
    , m_isDebuggingJavaScript(false)
    , m_isProfilingJavaScript(false)
    , m_isProfilingPage(false)
    , m_showMessageSent(false)
    , m_createdInspectorPage(false)
    , m_ignoreFirstBringToFront(false)
    , m_attachmentSide(AttachmentSideBottom)
#if PLATFORM(GTK) || PLATFORM(EFL)
    , m_inspectorView(0)
    , m_inspectorWindow(0)
#endif
#if ENABLE(INSPECTOR_SERVER)
    , m_remoteInspectionPageId(0)
#endif
{
    m_level = WebInspectorPageGroups::shared().inspectorLevel(m_page->pageGroup());
    m_page->process().addMessageReceiver(Messages::WebInspectorProxy::messageReceiverName(), m_page->pageID(), *this);
}

WebInspectorProxy::~WebInspectorProxy()
{
}

WebPageGroup* WebInspectorProxy::inspectorPageGroup() const
{
    return WebInspectorPageGroups::shared().inspectorPageGroupForLevel(m_level);
}

void WebInspectorProxy::invalidate()
{
#if ENABLE(INSPECTOR_SERVER)
    if (m_remoteInspectionPageId)
        WebInspectorServer::shared().unregisterPage(m_remoteInspectionPageId);
#endif

    m_page->process().removeMessageReceiver(Messages::WebInspectorProxy::messageReceiverName(), m_page->pageID());

    didClose();

    m_page = 0;
}

// Public APIs
bool WebInspectorProxy::isFront()
{
    if (!m_page)
        return false;

    return platformIsFront();
}

void WebInspectorProxy::connect()
{
    if (!m_page)
        return;

    if (m_showMessageSent)
        return;

    m_showMessageSent = true;
    m_ignoreFirstBringToFront = true;

    m_page->process().send(Messages::WebInspector::Show(), m_page->pageID());
}

void WebInspectorProxy::show()
{
    if (!m_page)
        return;

    if (isConnected()) {
        bringToFront();
        return;
    }

    connect();

    // Don't ignore the first bringToFront so it opens the Inspector.
    m_ignoreFirstBringToFront = false;
}

void WebInspectorProxy::hide()
{
    if (!m_page)
        return;

    m_isVisible = false;

    platformHide();
}

void WebInspectorProxy::close()
{
    if (!m_page)
        return;

    m_page->process().send(Messages::WebInspector::Close(), m_page->pageID());

    didClose();
}

void WebInspectorProxy::showConsole()
{
    if (!m_page)
        return;

    m_page->process().send(Messages::WebInspector::ShowConsole(), m_page->pageID());
}

void WebInspectorProxy::showResources()
{
    if (!m_page)
        return;

    m_page->process().send(Messages::WebInspector::ShowResources(), m_page->pageID());
}

void WebInspectorProxy::showMainResourceForFrame(WebFrameProxy* frame)
{
    if (!m_page)
        return;

    m_page->process().send(Messages::WebInspector::ShowMainResourceForFrame(frame->frameID()), m_page->pageID());
}

void WebInspectorProxy::attachBottom()
{
    attach(AttachmentSideBottom);
}

void WebInspectorProxy::attachRight()
{
    attach(AttachmentSideRight);
}

void WebInspectorProxy::attach(AttachmentSide side)
{
    if (!m_page || !canAttach())
        return;

    m_isAttached = true;
    m_attachmentSide = side;

    inspectorPageGroup()->preferences().setInspectorAttachmentSide(side);

    if (m_isVisible)
        inspectorPageGroup()->preferences().setInspectorStartsAttached(true);

    switch (m_attachmentSide) {
    case AttachmentSideBottom:
        m_page->process().send(Messages::WebInspector::AttachedBottom(), m_page->pageID());
        break;

    case AttachmentSideRight:
        m_page->process().send(Messages::WebInspector::AttachedRight(), m_page->pageID());
        break;
    }

    platformAttach();
}

void WebInspectorProxy::detach()
{
    if (!m_page)
        return;

    m_isAttached = false;

    if (m_isVisible)
        inspectorPageGroup()->preferences().setInspectorStartsAttached(false);

    m_page->process().send(Messages::WebInspector::Detached(), m_page->pageID());

    platformDetach();
}

void WebInspectorProxy::setAttachedWindowHeight(unsigned height)
{
    inspectorPageGroup()->preferences().setInspectorAttachedHeight(height);
    platformSetAttachedWindowHeight(height);
}

void WebInspectorProxy::setAttachedWindowWidth(unsigned width)
{
    inspectorPageGroup()->preferences().setInspectorAttachedWidth(width);
    platformSetAttachedWindowWidth(width);
}

void WebInspectorProxy::toggleJavaScriptDebugging()
{
    if (!m_page)
        return;

    if (m_isDebuggingJavaScript)
        m_page->process().send(Messages::WebInspector::StopJavaScriptDebugging(), m_page->pageID());
    else
        m_page->process().send(Messages::WebInspector::StartJavaScriptDebugging(), m_page->pageID());

    // FIXME: have the WebProcess notify us on state changes.
    m_isDebuggingJavaScript = !m_isDebuggingJavaScript;
}

void WebInspectorProxy::toggleJavaScriptProfiling()
{
    if (!m_page)
        return;

    if (m_isProfilingJavaScript)
        m_page->process().send(Messages::WebInspector::StopJavaScriptProfiling(), m_page->pageID());
    else
        m_page->process().send(Messages::WebInspector::StartJavaScriptProfiling(), m_page->pageID());

    // FIXME: have the WebProcess notify us on state changes.
    m_isProfilingJavaScript = !m_isProfilingJavaScript;
}

void WebInspectorProxy::togglePageProfiling()
{
    if (!m_page)
        return;

    if (m_isProfilingPage)
        m_page->process().send(Messages::WebInspector::StopPageProfiling(), m_page->pageID());
    else
        m_page->process().send(Messages::WebInspector::StartPageProfiling(), m_page->pageID());

    // FIXME: have the WebProcess notify us on state changes.
    m_isProfilingPage = !m_isProfilingPage;
}

bool WebInspectorProxy::isInspectorPage(WebPageProxy& page)
{
    return WebInspectorPageGroups::shared().isInspectorPageGroup(page.pageGroup());
}

static bool isMainOrTestInspectorPage(const WebInspectorProxy* webInspectorProxy, WKURLRequestRef requestRef)
{
    URL requestURL(URL(), toImpl(requestRef)->resourceRequest().url());
    if (!WebCore::SchemeRegistry::shouldTreatURLSchemeAsLocal(requestURL.protocol()))
        return false;

    // Use URL so we can compare just the paths.
    URL mainPageURL(URL(), webInspectorProxy->inspectorPageURL());
    ASSERT(WebCore::SchemeRegistry::shouldTreatURLSchemeAsLocal(mainPageURL.protocol()));
    if (decodeURLEscapeSequences(requestURL.path()) == decodeURLEscapeSequences(mainPageURL.path()))
        return true;

    // We might not have a Test URL in Production builds.
    String testPageURLString = webInspectorProxy->inspectorTestPageURL();
    if (testPageURLString.isNull())
        return false;

    URL testPageURL(URL(), webInspectorProxy->inspectorTestPageURL());
    ASSERT(WebCore::SchemeRegistry::shouldTreatURLSchemeAsLocal(testPageURL.protocol()));
    return decodeURLEscapeSequences(requestURL.path()) == decodeURLEscapeSequences(testPageURL.path());
}

static void decidePolicyForNavigationAction(WKPageRef, WKFrameRef frameRef, WKFrameNavigationType, WKEventModifiers, WKEventMouseButton, WKFrameRef, WKURLRequestRef requestRef, WKFramePolicyListenerRef listenerRef, WKTypeRef, const void* clientInfo)
{
    // Allow non-main frames to navigate anywhere.
    if (!toImpl(frameRef)->isMainFrame()) {
        toImpl(listenerRef)->use();
        return;
    }

    const WebInspectorProxy* webInspectorProxy = static_cast<const WebInspectorProxy*>(clientInfo);
    ASSERT(webInspectorProxy);

    // Allow loading of the main inspector file.
    if (isMainOrTestInspectorPage(webInspectorProxy, requestRef)) {
        toImpl(listenerRef)->use();
        return;
    }

    // Prevent everything else from loading in the inspector's page.
    toImpl(listenerRef)->ignore();

    // And instead load it in the inspected page.
    webInspectorProxy->page()->loadRequest(toImpl(requestRef)->resourceRequest());
}

#if ENABLE(INSPECTOR_SERVER)
void WebInspectorProxy::enableRemoteInspection()
{
    if (!m_remoteInspectionPageId)
        m_remoteInspectionPageId = WebInspectorServer::shared().registerPage(this);
}

void WebInspectorProxy::remoteFrontendConnected()
{
    m_page->process().send(Messages::WebInspector::RemoteFrontendConnected(), m_page->pageID());
}

void WebInspectorProxy::remoteFrontendDisconnected()
{
    m_page->process().send(Messages::WebInspector::RemoteFrontendDisconnected(), m_page->pageID());
}

void WebInspectorProxy::dispatchMessageFromRemoteFrontend(const String& message)
{
    m_page->process().send(Messages::WebInspector::DispatchMessageFromRemoteFrontend(message), m_page->pageID());
}
#endif

// Called by WebInspectorProxy messages
void WebInspectorProxy::createInspectorPage(uint64_t& inspectorPageID, WebPageCreationParameters& inspectorPageParameters)
{
    inspectorPageID = 0;

    if (!m_page)
        return;

    m_isAttached = shouldOpenAttached();
    m_attachmentSide = static_cast<AttachmentSide>(inspectorPageGroup()->preferences().inspectorAttachmentSide());

    WebPageProxy* inspectorPage = platformCreateInspectorPage();
    ASSERT(inspectorPage);
    if (!inspectorPage)
        return;

    inspectorPageID = inspectorPage->pageID();
    inspectorPageParameters = inspectorPage->creationParameters();

    WKPagePolicyClientV1 policyClient = {
        { 1, this },
        0, /* decidePolicyForNavigationAction_deprecatedForUseWithV0 */
        0, /* decidePolicyForNewWindowAction */
        0, /* decidePolicyForResponse_deprecatedForUseWithV0 */
        0, /* unableToImplementPolicy */
        decidePolicyForNavigationAction,
        0, /* decidePolicyForResponse */
    };

    WKPageSetPagePolicyClient(toAPI(inspectorPage), &policyClient.base);

    StringBuilder url;

    url.append(inspectorPageURL());

    url.appendLiteral("?dockSide=");

    if (m_isAttached) {
        switch (m_attachmentSide) {
        case AttachmentSideBottom:
            url.appendLiteral("bottom");
            m_page->process().send(Messages::WebInspector::AttachedBottom(), m_page->pageID());
            break;
        case AttachmentSideRight:
            url.appendLiteral("right");
            m_page->process().send(Messages::WebInspector::AttachedRight(), m_page->pageID());
            break;
        }
    } else
        url.appendLiteral("undocked");

    m_page->process().assumeReadAccessToBaseURL(inspectorBaseURL());

    inspectorPage->loadRequest(URL(URL(), url.toString()));

    m_createdInspectorPage = true;
}

void WebInspectorProxy::createInspectorPageForTest(uint64_t& inspectorPageID, WebPageCreationParameters& inspectorPageParameters)
{
    inspectorPageID = 0;

    if (!m_page)
        return;

    m_isAttached = false;

    WebPageProxy* inspectorPage = platformCreateInspectorPage();
    ASSERT(inspectorPage);
    if (!inspectorPage)
        return;

    inspectorPageID = inspectorPage->pageID();
    inspectorPageParameters = inspectorPage->creationParameters();

    WKPagePolicyClientV1 policyClient = {
        { 1, this },
        0, /* decidePolicyForNavigationAction_deprecatedForUseWithV0 */
        0, /* decidePolicyForNewWindowAction */
        0, /* decidePolicyForResponse_deprecatedForUseWithV0 */
        0, /* unableToImplementPolicy */
        decidePolicyForNavigationAction,
        0, /* decidePolicyForResponse */
    };

    WKPageSetPagePolicyClient(toAPI(inspectorPage), &policyClient.base);

    m_page->process().assumeReadAccessToBaseURL(inspectorBaseURL());

    inspectorPage->loadRequest(URL(URL(), inspectorTestPageURL()));

    m_createdInspectorPage = true;
}

void WebInspectorProxy::open()
{
    m_isVisible = true;

    platformOpen();
}

void WebInspectorProxy::didClose()
{
    if (!m_createdInspectorPage)
        return;

    m_isVisible = false;
    m_isDebuggingJavaScript = false;
    m_isProfilingJavaScript = false;
    m_isProfilingPage = false;
    m_createdInspectorPage = false;
    m_showMessageSent = false;
    m_ignoreFirstBringToFront = false;

    if (m_isAttached)
        platformDetach();
    m_isAttached = false;

    platformDidClose();
}

void WebInspectorProxy::bringToFront()
{
    // WebCore::InspectorFrontendClientLocal tells us to do this on load. We want to
    // ignore it once if we only wanted to connect. This allows the Inspector to later
    // request to be brought to the front when a breakpoint is hit or some other action.
    if (m_ignoreFirstBringToFront) {
        m_ignoreFirstBringToFront = false;
        return;
    }

    if (m_isVisible)
        platformBringToFront();
    else
        open();
}

void WebInspectorProxy::attachAvailabilityChanged(bool available)
{
    platformAttachAvailabilityChanged(available);
}

void WebInspectorProxy::inspectedURLChanged(const String& urlString)
{
    platformInspectedURLChanged(urlString);
}

void WebInspectorProxy::save(const String& filename, const String& content, bool base64Encoded, bool forceSaveAs)
{
    platformSave(filename, content, base64Encoded, forceSaveAs);
}

void WebInspectorProxy::append(const String& filename, const String& content)
{
    platformAppend(filename, content);
}

bool WebInspectorProxy::canAttach()
{
    // Keep this in sync with InspectorFrontendClientLocal::canAttachWindow. There are two implementations
    // to make life easier in the multi-process world we have. WebInspectorProxy uses canAttach to decide if
    // we can attach on open (on the UI process side). And InspectorFrontendClientLocal::canAttachWindow is
    // used to decide if we can attach when the attach button is pressed (on the WebProcess side).

    // If we are already attached, allow attaching again to allow switching sides.
    if (m_isAttached)
        return true;

    // Don't allow attaching to another inspector -- two inspectors in one window is too much!
    if (m_level > 1)
        return false;

    // Don't allow the attach if the window would be too small to accommodate the minimum inspector height.
    unsigned inspectedPageHeight = platformInspectedWindowHeight();
    unsigned inspectedPageWidth = platformInspectedWindowWidth();
    unsigned maximumAttachedHeight = inspectedPageHeight * 3 / 4;
    return minimumAttachedHeight <= maximumAttachedHeight && minimumAttachedWidth <= inspectedPageWidth;
}

bool WebInspectorProxy::shouldOpenAttached()
{
    return inspectorPageGroup()->preferences().inspectorStartsAttached() && canAttach();
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
