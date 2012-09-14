/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#include "InjectedBundle.h"

#include "Arguments.h"
#include "ImmutableArray.h"
#include "InjectedBundleMessageKinds.h"
#include "InjectedBundleScriptWorld.h"
#include "InjectedBundleUserMessageCoders.h"
#include "LayerTreeHost.h"
#include "NotificationPermissionRequestManager.h"
#include "WKAPICast.h"
#include "WKBundleAPICast.h"
#include "WebApplicationCacheManager.h"
#include "WebContextMessageKinds.h"
#include "WebCookieManager.h"
#include "WebCoreArgumentCoders.h"
#include "WebDatabaseManager.h"
#include "WebFrame.h"
#include "WebPage.h"
#include "WebPreferencesStore.h"
#include "WebProcess.h"
#include <JavaScriptCore/APICast.h>
#include <JavaScriptCore/JSLock.h>
#include <WebCore/ApplicationCache.h>
#include <WebCore/ApplicationCacheStorage.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameView.h>
#include <WebCore/GCController.h>
#include <WebCore/GeolocationClient.h>
#include <WebCore/GeolocationClientMock.h>
#include <WebCore/GeolocationController.h>
#include <WebCore/GeolocationPosition.h>
#include <WebCore/JSDOMWindow.h>
#include <WebCore/JSNotification.h>
#include <WebCore/Page.h>
#include <WebCore/PageGroup.h>
#include <WebCore/PageVisibilityState.h>
#include <WebCore/PrintContext.h>
#include <WebCore/ResourceHandle.h>
#include <WebCore/ScriptController.h>
#include <WebCore/SecurityOrigin.h>
#include <WebCore/SecurityPolicy.h>
#include <WebCore/Settings.h>
#include <WebCore/UserGestureIndicator.h>
#include <WebCore/WorkerThread.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/PassOwnArrayPtr.h>

using namespace WebCore;
using namespace JSC;

namespace WebKit {

InjectedBundle::InjectedBundle(const String& path)
    : m_path(path)
    , m_platformBundle(0)
{
    initializeClient(0);
}

InjectedBundle::~InjectedBundle()
{
}

void InjectedBundle::initializeClient(WKBundleClient* client)
{
    m_client.initialize(client);
}

void InjectedBundle::postMessage(const String& messageName, APIObject* messageBody)
{
    WebProcess::shared().connection()->deprecatedSend(WebContextLegacyMessage::PostMessage, 0, CoreIPC::In(messageName, InjectedBundleUserMessageEncoder(messageBody)));
}

void InjectedBundle::postSynchronousMessage(const String& messageName, APIObject* messageBody, RefPtr<APIObject>& returnData)
{
    RefPtr<APIObject> returnDataTmp;
    InjectedBundleUserMessageDecoder messageDecoder(returnDataTmp);
    
    bool succeeded = WebProcess::shared().connection()->deprecatedSendSync(WebContextLegacyMessage::PostSynchronousMessage, 0, CoreIPC::In(messageName, InjectedBundleUserMessageEncoder(messageBody)), CoreIPC::Out(messageDecoder));

    if (!succeeded)
        return;

    returnData = returnDataTmp;
}

WebConnection* InjectedBundle::webConnectionToUIProcess() const
{
    return WebProcess::shared().webConnectionToUIProcess();
}

void InjectedBundle::setShouldTrackVisitedLinks(bool shouldTrackVisitedLinks)
{
    WebProcess::shared().setShouldTrackVisitedLinks(shouldTrackVisitedLinks);
}

void InjectedBundle::setAlwaysAcceptCookies(bool accept)
{
    WebCookieManager::shared().setHTTPCookieAcceptPolicy(accept ? HTTPCookieAcceptPolicyAlways : HTTPCookieAcceptPolicyOnlyFromMainDocumentDomain);
}

void InjectedBundle::removeAllVisitedLinks()
{
    PageGroup::removeAllVisitedLinks();
}

void InjectedBundle::overrideBoolPreferenceForTestRunner(WebPageGroupProxy* pageGroup, const String& preference, bool enabled)
{
    const HashSet<Page*>& pages = PageGroup::pageGroup(pageGroup->identifier())->pages();

    if (preference == "WebKitTabToLinksPreferenceKey") {
       WebPreferencesStore::overrideBoolValueForKey(WebPreferencesKey::tabsToLinksKey(), enabled);
       for (HashSet<Page*>::iterator i = pages.begin(); i != pages.end(); ++i) {
            WebPage* webPage = static_cast<WebFrameLoaderClient*>((*i)->mainFrame()->loader()->client())->webFrame()->page();
            webPage->setTabToLinksEnabled(enabled);
        }
    }

    if (preference == "WebKit2AsynchronousPluginInitializationEnabled") {
        WebPreferencesStore::overrideBoolValueForKey(WebPreferencesKey::asynchronousPluginInitializationEnabledKey(), enabled);
        for (HashSet<Page*>::iterator i = pages.begin(); i != pages.end(); ++i) {
            WebPage* webPage = static_cast<WebFrameLoaderClient*>((*i)->mainFrame()->loader()->client())->webFrame()->page();
            webPage->setAsynchronousPluginInitializationEnabled(enabled);
        }
    }

    if (preference == "WebKit2AsynchronousPluginInitializationEnabledForAllPlugins") {
        WebPreferencesStore::overrideBoolValueForKey(WebPreferencesKey::asynchronousPluginInitializationEnabledForAllPluginsKey(), enabled);
        for (HashSet<Page*>::iterator i = pages.begin(); i != pages.end(); ++i) {
            WebPage* webPage = static_cast<WebFrameLoaderClient*>((*i)->mainFrame()->loader()->client())->webFrame()->page();
            webPage->setAsynchronousPluginInitializationEnabledForAllPlugins(enabled);
        }
    }

    if (preference == "WebKit2ArtificialPluginInitializationDelayEnabled") {
        WebPreferencesStore::overrideBoolValueForKey(WebPreferencesKey::artificialPluginInitializationDelayEnabledKey(), enabled);
        for (HashSet<Page*>::iterator i = pages.begin(); i != pages.end(); ++i) {
            WebPage* webPage = static_cast<WebFrameLoaderClient*>((*i)->mainFrame()->loader()->client())->webFrame()->page();
            webPage->setArtificialPluginInitializationDelayEnabled(enabled);
        }
    }

    // Map the names used in LayoutTests with the names used in WebCore::Settings and WebPreferencesStore.
#define FOR_EACH_OVERRIDE_BOOL_PREFERENCE(macro) \
    macro(WebKitAcceleratedCompositingEnabled, AcceleratedCompositingEnabled, acceleratedCompositingEnabled) \
    macro(WebKitCSSCustomFilterEnabled, CSSCustomFilterEnabled, cssCustomFilterEnabled) \
    macro(WebKitCSSRegionsEnabled, CSSRegionsEnabled, cssRegionsEnabled) \
    macro(WebKitCSSGridLayoutEnabled, CSSGridLayoutEnabled, cssGridLayoutEnabled) \
    macro(WebKitJavaEnabled, JavaEnabled, javaEnabled) \
    macro(WebKitJavaScriptEnabled, ScriptEnabled, javaScriptEnabled) \
    macro(WebKitLoadSiteIconsKey, LoadsSiteIconsIgnoringImageLoadingSetting, loadsSiteIconsIgnoringImageLoadingPreference) \
    macro(WebKitOfflineWebApplicationCacheEnabled, OfflineWebApplicationCacheEnabled, offlineWebApplicationCacheEnabled) \
    macro(WebKitPageCacheSupportsPluginsPreferenceKey, PageCacheSupportsPlugins, pageCacheSupportsPlugins) \
    macro(WebKitPluginsEnabled, PluginsEnabled, pluginsEnabled) \
    macro(WebKitUsesPageCachePreferenceKey, UsesPageCache, usesPageCache) \
    macro(WebKitWebAudioEnabled, WebAudioEnabled, webAudioEnabled) \
    macro(WebKitWebGLEnabled, WebGLEnabled, webGLEnabled) \
    macro(WebKitXSSAuditorEnabled, XSSAuditorEnabled, xssAuditorEnabled) \
    macro(WebKitShouldRespectImageOrientation, ShouldRespectImageOrientation, shouldRespectImageOrientation) \
    macro(WebKitEnableCaretBrowsing, CaretBrowsingEnabled, caretBrowsingEnabled)

    if (preference == "WebKitAcceleratedCompositingEnabled")
        enabled = enabled && LayerTreeHost::supportsAcceleratedCompositing();

#define OVERRIDE_PREFERENCE_AND_SET_IN_EXISTING_PAGES(TestRunnerName, SettingsName, WebPreferencesName) \
    if (preference == #TestRunnerName) { \
        WebPreferencesStore::overrideBoolValueForKey(WebPreferencesKey::WebPreferencesName##Key(), enabled); \
        for (HashSet<Page*>::iterator iter = pages.begin(); iter != pages.end(); ++iter) \
            (*iter)->settings()->set##SettingsName(enabled); \
        return; \
    }

    FOR_EACH_OVERRIDE_BOOL_PREFERENCE(OVERRIDE_PREFERENCE_AND_SET_IN_EXISTING_PAGES)

#undef OVERRIDE_PREFERENCE_AND_SET_IN_EXISTING_PAGES
#undef FOR_EACH_OVERRIDE_BOOL_PREFERENCE
}

void InjectedBundle::overrideXSSAuditorEnabledForTestRunner(WebPageGroupProxy* pageGroup, bool enabled)
{
    // Override the preference for all future pages.
    WebPreferencesStore::overrideBoolValueForKey(WebPreferencesKey::xssAuditorEnabledKey(), enabled);

    // Change the setting for existing ones.
    const HashSet<Page*>& pages = PageGroup::pageGroup(pageGroup->identifier())->pages();
    for (HashSet<Page*>::iterator iter = pages.begin(); iter != pages.end(); ++iter)
        (*iter)->settings()->setXSSAuditorEnabled(enabled);
}

void InjectedBundle::setAllowUniversalAccessFromFileURLs(WebPageGroupProxy* pageGroup, bool enabled)
{
    const HashSet<Page*>& pages = PageGroup::pageGroup(pageGroup->identifier())->pages();
    for (HashSet<Page*>::iterator iter = pages.begin(); iter != pages.end(); ++iter)
        (*iter)->settings()->setAllowUniversalAccessFromFileURLs(enabled);
}

void InjectedBundle::setAllowFileAccessFromFileURLs(WebPageGroupProxy* pageGroup, bool enabled)
{
    const HashSet<Page*>& pages = PageGroup::pageGroup(pageGroup->identifier())->pages();
    for (HashSet<Page*>::iterator iter = pages.begin(); iter != pages.end(); ++iter)
        (*iter)->settings()->setAllowFileAccessFromFileURLs(enabled);
}

void InjectedBundle::setMinimumLogicalFontSize(WebPageGroupProxy* pageGroup, int size)
{
    const HashSet<Page*>& pages = PageGroup::pageGroup(pageGroup->identifier())->pages();
    for (HashSet<Page*>::iterator iter = pages.begin(); iter != pages.end(); ++iter)
        (*iter)->settings()->setMinimumLogicalFontSize(size);
}

void InjectedBundle::setFrameFlatteningEnabled(WebPageGroupProxy* pageGroup, bool enabled)
{
    const HashSet<Page*>& pages = PageGroup::pageGroup(pageGroup->identifier())->pages();
    for (HashSet<Page*>::iterator iter = pages.begin(); iter != pages.end(); ++iter)
        (*iter)->settings()->setFrameFlatteningEnabled(enabled);
}

void InjectedBundle::setPluginsEnabled(WebPageGroupProxy* pageGroup, bool enabled)
{
    const HashSet<Page*>& pages = PageGroup::pageGroup(pageGroup->identifier())->pages();
    for (HashSet<Page*>::iterator iter = pages.begin(); iter != pages.end(); ++iter)
        (*iter)->settings()->setPluginsEnabled(enabled);
}

void InjectedBundle::setGeoLocationPermission(WebPageGroupProxy* pageGroup, bool enabled)
{
#if ENABLE(GEOLOCATION)
    const HashSet<Page*>& pages = PageGroup::pageGroup(pageGroup->identifier())->pages();
    for (HashSet<Page*>::iterator iter = pages.begin(); iter != pages.end(); ++iter)
        static_cast<GeolocationClientMock*>(GeolocationController::from(*iter)->client())->setPermission(enabled);
#endif // ENABLE(GEOLOCATION)
}

void InjectedBundle::setJavaScriptCanAccessClipboard(WebPageGroupProxy* pageGroup, bool enabled)
{
    const HashSet<Page*>& pages = PageGroup::pageGroup(pageGroup->identifier())->pages();
    for (HashSet<Page*>::iterator iter = pages.begin(); iter != pages.end(); ++iter)
        (*iter)->settings()->setJavaScriptCanAccessClipboard(enabled);
}

void InjectedBundle::setPrivateBrowsingEnabled(WebPageGroupProxy* pageGroup, bool enabled)
{
    const HashSet<Page*>& pages = PageGroup::pageGroup(pageGroup->identifier())->pages();
    for (HashSet<Page*>::iterator iter = pages.begin(); iter != pages.end(); ++iter)
        (*iter)->settings()->setPrivateBrowsingEnabled(enabled);
}

void InjectedBundle::setPopupBlockingEnabled(WebPageGroupProxy* pageGroup, bool enabled)
{
    const HashSet<Page*>& pages = PageGroup::pageGroup(pageGroup->identifier())->pages();
    HashSet<Page*>::const_iterator end = pages.end();
    for (HashSet<Page*>::const_iterator iter = pages.begin(); iter != end; ++iter)
        (*iter)->settings()->setJavaScriptCanOpenWindowsAutomatically(!enabled);
}

void InjectedBundle::switchNetworkLoaderToNewTestingSession()
{
#if USE(CFURLSTORAGESESSIONS)
    // Set a private session for testing to avoid interfering with global cookies. This should be different from private browsing session.
    RetainPtr<CFURLStorageSessionRef> session = ResourceHandle::createPrivateBrowsingStorageSession(CFSTR("Private WebKit Session"));
    ResourceHandle::setDefaultStorageSession(session.get());
#endif
}

void InjectedBundle::setAuthorAndUserStylesEnabled(WebPageGroupProxy* pageGroup, bool enabled)
{
    const HashSet<Page*>& pages = PageGroup::pageGroup(pageGroup->identifier())->pages();
    for (HashSet<Page*>::iterator iter = pages.begin(); iter != pages.end(); ++iter)
        (*iter)->settings()->setAuthorAndUserStylesEnabled(enabled);
}

void InjectedBundle::setSpatialNavigationEnabled(WebPageGroupProxy* pageGroup, bool enabled)
{
    const HashSet<Page*>& pages = PageGroup::pageGroup(pageGroup->identifier())->pages();
    for (HashSet<Page*>::iterator iter = pages.begin(); iter != pages.end(); ++iter)
        (*iter)->settings()->setSpatialNavigationEnabled(enabled);
}

void InjectedBundle::addOriginAccessWhitelistEntry(const String& sourceOrigin, const String& destinationProtocol, const String& destinationHost, bool allowDestinationSubdomains)
{
    SecurityPolicy::addOriginAccessWhitelistEntry(*SecurityOrigin::createFromString(sourceOrigin), destinationProtocol, destinationHost, allowDestinationSubdomains);
}

void InjectedBundle::removeOriginAccessWhitelistEntry(const String& sourceOrigin, const String& destinationProtocol, const String& destinationHost, bool allowDestinationSubdomains)
{
    SecurityPolicy::removeOriginAccessWhitelistEntry(*SecurityOrigin::createFromString(sourceOrigin), destinationProtocol, destinationHost, allowDestinationSubdomains);
}

void InjectedBundle::resetOriginAccessWhitelists()
{
    SecurityPolicy::resetOriginAccessWhitelists();
}

void InjectedBundle::clearAllDatabases()
{
#if ENABLE(SQL_DATABASE)
    WebDatabaseManager::shared().deleteAllDatabases();
#endif
}

void InjectedBundle::setDatabaseQuota(uint64_t quota)
{
#if ENABLE(SQL_DATABASE)
    // Historically, we've used the following (somewhat non-sensical) string
    // for the databaseIdentifier of local files.
    WebDatabaseManager::shared().setQuotaForOrigin("file__0", quota);
#endif
}

void InjectedBundle::clearApplicationCache()
{
    WebApplicationCacheManager::shared().deleteAllEntries();
}

void InjectedBundle::clearApplicationCacheForOrigin(const String& originString)
{
    RefPtr<SecurityOrigin> origin = SecurityOrigin::createFromString(originString);
    ApplicationCache::deleteCacheForOrigin(origin.get());
}

void InjectedBundle::setAppCacheMaximumSize(uint64_t size)
{
    WebApplicationCacheManager::shared().setAppCacheMaximumSize(size);
}

uint64_t InjectedBundle::appCacheUsageForOrigin(const String& originString)
{
    RefPtr<SecurityOrigin> origin = SecurityOrigin::createFromString(originString);
    return ApplicationCache::diskUsageForOrigin(origin.get());
}

void InjectedBundle::setApplicationCacheOriginQuota(const String& originString, uint64_t bytes)
{
    RefPtr<SecurityOrigin> origin = SecurityOrigin::createFromString(originString);
    cacheStorage().storeUpdatedQuotaForOrigin(origin.get(), bytes);
}

void InjectedBundle::resetApplicationCacheOriginQuota(const String& originString)
{
    RefPtr<SecurityOrigin> origin = SecurityOrigin::createFromString(originString);
    cacheStorage().storeUpdatedQuotaForOrigin(origin.get(), cacheStorage().defaultOriginQuota());
}

PassRefPtr<ImmutableArray> InjectedBundle::originsWithApplicationCache()
{
    HashSet<RefPtr<SecurityOrigin>, SecurityOriginHash> origins;
    cacheStorage().getOriginsWithCache(origins);
    Vector< RefPtr<APIObject> > originsVector;

    HashSet<RefPtr<SecurityOrigin>, SecurityOriginHash>::iterator it = origins.begin();
    HashSet<RefPtr<SecurityOrigin>, SecurityOriginHash>::iterator end = origins.end();
    for ( ; it != end; ++it)
        originsVector.append(WebString::create((*it)->databaseIdentifier()));

    return ImmutableArray::adopt(originsVector);
}

int InjectedBundle::numberOfPages(WebFrame* frame, double pageWidthInPixels, double pageHeightInPixels)
{
    Frame* coreFrame = frame ? frame->coreFrame() : 0;
    if (!coreFrame)
        return -1;
    if (!pageWidthInPixels)
        pageWidthInPixels = coreFrame->view()->width();
    if (!pageHeightInPixels)
        pageHeightInPixels = coreFrame->view()->height();

    return PrintContext::numberOfPages(coreFrame, FloatSize(pageWidthInPixels, pageHeightInPixels));
}

int InjectedBundle::pageNumberForElementById(WebFrame* frame, const String& id, double pageWidthInPixels, double pageHeightInPixels)
{
    Frame* coreFrame = frame ? frame->coreFrame() : 0;
    if (!coreFrame)
        return -1;

    Element* element = coreFrame->document()->getElementById(AtomicString(id));
    if (!element)
        return -1;

    if (!pageWidthInPixels)
        pageWidthInPixels = coreFrame->view()->width();
    if (!pageHeightInPixels)
        pageHeightInPixels = coreFrame->view()->height();

    return PrintContext::pageNumberForElement(element, FloatSize(pageWidthInPixels, pageHeightInPixels));
}

String InjectedBundle::pageSizeAndMarginsInPixels(WebFrame* frame, int pageIndex, int width, int height, int marginTop, int marginRight, int marginBottom, int marginLeft)
{
    Frame* coreFrame = frame ? frame->coreFrame() : 0;
    if (!coreFrame)
        return String();

    return PrintContext::pageSizeAndMarginsInPixels(coreFrame, pageIndex, width, height, marginTop, marginRight, marginBottom, marginLeft);
}

bool InjectedBundle::isPageBoxVisible(WebFrame* frame, int pageIndex)
{
    Frame* coreFrame = frame ? frame->coreFrame() : 0;
    if (!coreFrame)
        return false;

    return PrintContext::isPageBoxVisible(coreFrame, pageIndex);
}

bool InjectedBundle::isProcessingUserGesture()
{
    return ScriptController::processingUserGesture();
}

static PassOwnPtr<Vector<String> > toStringVector(ImmutableArray* patterns)
{
    if (!patterns)
        return nullptr;

    size_t size =  patterns->size();
    if (!size)
        return nullptr;

    OwnPtr<Vector<String> > patternsVector = adoptPtr(new Vector<String>);
    patternsVector->reserveInitialCapacity(size);
    for (size_t i = 0; i < size; ++i) {
        WebString* entry = patterns->at<WebString>(i);
        if (entry)
            patternsVector->uncheckedAppend(entry->string());
    }
    return patternsVector.release();
}

void InjectedBundle::addUserScript(WebPageGroupProxy* pageGroup, InjectedBundleScriptWorld* scriptWorld, const String& source, const String& url, ImmutableArray* whitelist, ImmutableArray* blacklist, WebCore::UserScriptInjectionTime injectionTime, WebCore::UserContentInjectedFrames injectedFrames)
{
    // url is not from KURL::string(), i.e. it has not already been parsed by KURL, so we have to use the relative URL constructor for KURL instead of the ParsedURLStringTag version.
    PageGroup::pageGroup(pageGroup->identifier())->addUserScriptToWorld(scriptWorld->coreWorld(), source, KURL(KURL(), url), toStringVector(whitelist), toStringVector(blacklist), injectionTime, injectedFrames);
}

void InjectedBundle::addUserStyleSheet(WebPageGroupProxy* pageGroup, InjectedBundleScriptWorld* scriptWorld, const String& source, const String& url, ImmutableArray* whitelist, ImmutableArray* blacklist, WebCore::UserContentInjectedFrames injectedFrames)
{
    // url is not from KURL::string(), i.e. it has not already been parsed by KURL, so we have to use the relative URL constructor for KURL instead of the ParsedURLStringTag version.
    PageGroup::pageGroup(pageGroup->identifier())->addUserStyleSheetToWorld(scriptWorld->coreWorld(), source, KURL(KURL(), url), toStringVector(whitelist), toStringVector(blacklist), injectedFrames);
}

void InjectedBundle::removeUserScript(WebPageGroupProxy* pageGroup, InjectedBundleScriptWorld* scriptWorld, const String& url)
{
    // url is not from KURL::string(), i.e. it has not already been parsed by KURL, so we have to use the relative URL constructor for KURL instead of the ParsedURLStringTag version.
    PageGroup::pageGroup(pageGroup->identifier())->removeUserScriptFromWorld(scriptWorld->coreWorld(), KURL(KURL(), url));
}

void InjectedBundle::removeUserStyleSheet(WebPageGroupProxy* pageGroup, InjectedBundleScriptWorld* scriptWorld, const String& url)
{
    // url is not from KURL::string(), i.e. it has not already been parsed by KURL, so we have to use the relative URL constructor for KURL instead of the ParsedURLStringTag version.
    PageGroup::pageGroup(pageGroup->identifier())->removeUserStyleSheetFromWorld(scriptWorld->coreWorld(), KURL(KURL(), url));
}

void InjectedBundle::removeUserScripts(WebPageGroupProxy* pageGroup, InjectedBundleScriptWorld* scriptWorld)
{
    PageGroup::pageGroup(pageGroup->identifier())->removeUserScriptsFromWorld(scriptWorld->coreWorld());
}

void InjectedBundle::removeUserStyleSheets(WebPageGroupProxy* pageGroup, InjectedBundleScriptWorld* scriptWorld)
{
    PageGroup::pageGroup(pageGroup->identifier())->removeUserStyleSheetsFromWorld(scriptWorld->coreWorld());
}

void InjectedBundle::removeAllUserContent(WebPageGroupProxy* pageGroup)
{
    PageGroup::pageGroup(pageGroup->identifier())->removeAllUserContent();
}

void InjectedBundle::garbageCollectJavaScriptObjects()
{
    gcController().garbageCollectNow();
}

void InjectedBundle::garbageCollectJavaScriptObjectsOnAlternateThreadForDebugging(bool waitUntilDone)
{
    gcController().garbageCollectOnAlternateThreadForDebugging(waitUntilDone);
}

size_t InjectedBundle::javaScriptObjectsCount()
{
    JSLockHolder lock(JSDOMWindow::commonJSGlobalData());
    return JSDOMWindow::commonJSGlobalData()->heap.objectCount();
}

void InjectedBundle::reportException(JSContextRef context, JSValueRef exception)
{
    if (!context || !exception)
        return;

    JSC::ExecState* execState = toJS(context);
    JSLockHolder lock(execState);

    // Make sure the context has a DOMWindow global object, otherwise this context didn't originate from a Page.
    if (!toJSDOMWindow(execState->lexicalGlobalObject()))
        return;

    WebCore::reportException(execState, toJS(execState, exception));
}

void InjectedBundle::didCreatePage(WebPage* page)
{
    m_client.didCreatePage(this, page);
}

void InjectedBundle::willDestroyPage(WebPage* page)
{
    m_client.willDestroyPage(this, page);
}

void InjectedBundle::didInitializePageGroup(WebPageGroupProxy* pageGroup)
{
    m_client.didInitializePageGroup(this, pageGroup);
}

void InjectedBundle::didReceiveMessage(const String& messageName, APIObject* messageBody)
{
    m_client.didReceiveMessage(this, messageName, messageBody);
}

void InjectedBundle::didReceiveMessageToPage(WebPage* page, const String& messageName, APIObject* messageBody)
{
    m_client.didReceiveMessageToPage(this, page, messageName, messageBody);
}

void InjectedBundle::didReceiveMessage(CoreIPC::Connection* connection, CoreIPC::MessageID messageID, CoreIPC::ArgumentDecoder* arguments)
{
    switch (messageID.get<InjectedBundleMessage::Kind>()) {
        case InjectedBundleMessage::PostMessage: {
            String messageName;            
            RefPtr<APIObject> messageBody;
            InjectedBundleUserMessageDecoder messageDecoder(messageBody);
            if (!arguments->decode(CoreIPC::Out(messageName, messageDecoder)))
                return;

            didReceiveMessage(messageName, messageBody.get());
            return;
        }

        case InjectedBundleMessage::PostMessageToPage: {
            uint64_t pageID = arguments->destinationID();
            if (!pageID)
                return;
            
            WebPage* page = WebProcess::shared().webPage(pageID);
            if (!page)
                return;

            String messageName;
            RefPtr<APIObject> messageBody;
            InjectedBundleUserMessageDecoder messageDecoder(messageBody);
            if (!arguments->decode(CoreIPC::Out(messageName, messageDecoder)))
                return;

            didReceiveMessageToPage(page, messageName, messageBody.get());
            return;
        }
    }

    ASSERT_NOT_REACHED();
}

void InjectedBundle::setPageVisibilityState(WebPage* page, int state, bool isInitialState)
{
#if ENABLE(PAGE_VISIBILITY_API)
    page->corePage()->setVisibilityState(static_cast<PageVisibilityState>(state), isInitialState);
#endif
}

size_t InjectedBundle::workerThreadCount()
{
#if ENABLE(WORKERS)
    return WebCore::WorkerThread::workerThreadCount();
#else
    return 0;
#endif
}

void InjectedBundle::setUserStyleSheetLocation(WebPageGroupProxy* pageGroup, const String& location)
{
    const HashSet<Page*>& pages = PageGroup::pageGroup(pageGroup->identifier())->pages();
    for (HashSet<Page*>::iterator iter = pages.begin(); iter != pages.end(); ++iter)
        (*iter)->settings()->setUserStyleSheetLocation(KURL(KURL(), location));
}

void InjectedBundle::setMinimumTimerInterval(WebPageGroupProxy* pageGroup, double seconds)
{
    const HashSet<Page*>& pages = PageGroup::pageGroup(pageGroup->identifier())->pages();
    for (HashSet<Page*>::iterator iter = pages.begin(); iter != pages.end(); ++iter)
        (*iter)->settings()->setMinDOMTimerInterval(seconds);
}

void InjectedBundle::setWebNotificationPermission(WebPage* page, const String& originString, bool allowed)
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    page->notificationPermissionRequestManager()->setPermissionLevelForTesting(originString, allowed);
#else
    UNUSED_PARAM(page);
    UNUSED_PARAM(originString);
    UNUSED_PARAM(allowed);
#endif
}

void InjectedBundle::removeAllWebNotificationPermissions(WebPage* page)
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    page->notificationPermissionRequestManager()->removeAllPermissionsForTesting();
#else
    UNUSED_PARAM(page);
#endif
}

uint64_t InjectedBundle::webNotificationID(JSContextRef jsContext, JSValueRef jsNotification)
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    WebCore::Notification* notification = toNotification(toJS(toJS(jsContext), jsNotification));
    if (!notification)
        return 0;
    return WebProcess::shared().notificationManager().notificationIDForTesting(notification);
#else
    return 0;
#endif
}

void InjectedBundle::setTabKeyCyclesThroughElements(WebPage* page, bool enabled)
{
    page->corePage()->setTabKeyCyclesThroughElements(enabled);
}

} // namespace WebKit
