/*
 * Copyright (C) 2009, 2010, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "TestRunner.h"

#include "CString.h"
#include "DatabaseTracker.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "DocumentMarker.h"
#include "DumpRenderTree.h"
#include "DumpRenderTreeBlackBerry.h"
#include "DumpRenderTreeSupport.h"
#include "EditingBehaviorTypes.h"
#include "EditorClientBlackBerry.h"
#include "Element.h"
#include "Frame.h"
#include "HTMLInputElement.h"
#include "JSElement.h"
#include "KURL.h"
#include "NotImplemented.h"
#include "OwnArrayPtr.h"
#include "Page.h"
#include "RenderTreeAsText.h"
#include "SchemeRegistry.h"
#include "SecurityOrigin.h"
#include "SecurityPolicy.h"
#include "Settings.h"
#include "UnusedParam.h"
#include "WorkQueue.h"
#include "WorkQueueItem.h"
#include "WorkerThread.h"

#include <JavaScriptCore/APICast.h>
#include <SharedPointer.h>
#include <WebPage.h>
#include <WebSettings.h>

using WebCore::toElement;
using WebCore::toJS;

TestRunner::~TestRunner()
{
}

void TestRunner::addDisallowedURL(JSStringRef url)
{
    UNUSED_PARAM(url);
    notImplemented();
}

void TestRunner::clearAllDatabases()
{
#if ENABLE(DATABASE)
    WebCore::DatabaseTracker::tracker().deleteAllDatabases();
#endif
}

void TestRunner::clearBackForwardList()
{
    BlackBerry::WebKit::DumpRenderTree::currentInstance()->page()->clearBackForwardList(true);
}

void TestRunner::clearPersistentUserStyleSheet()
{
    notImplemented();
}

JSStringRef TestRunner::copyDecodedHostName(JSStringRef name)
{
    UNUSED_PARAM(name);
    notImplemented();
    return 0;
}

JSStringRef TestRunner::copyEncodedHostName(JSStringRef name)
{
    UNUSED_PARAM(name);
    notImplemented();
    return 0;
}

void TestRunner::dispatchPendingLoadRequests()
{
    notImplemented();
}

void TestRunner::display()
{
    notImplemented();
}

static String jsStringRefToWebCoreString(JSStringRef str)
{
    size_t strArrSize = JSStringGetMaximumUTF8CStringSize(str);
    OwnArrayPtr<char> strArr = adoptArrayPtr(new char[strArrSize]);
    JSStringGetUTF8CString(str, strArr.get(), strArrSize);
    return String::fromUTF8(strArr.get());
}

void TestRunner::execCommand(JSStringRef name, JSStringRef value)
{
    if (!mainFrame)
        return;

    String nameStr = jsStringRefToWebCoreString(name);
    String valueStr = jsStringRefToWebCoreString(value);

    mainFrame->editor()->command(nameStr).execute(valueStr);
}

bool TestRunner::isCommandEnabled(JSStringRef name)
{
    if (!mainFrame)
        return false;

    String nameStr = jsStringRefToWebCoreString(name);

    return mainFrame->editor()->command(nameStr).isEnabled();
}

void TestRunner::keepWebHistory()
{
    notImplemented();
}

void TestRunner::notifyDone()
{
    if (m_waitToDump && (!topLoadingFrame || BlackBerry::WebKit::DumpRenderTree::currentInstance()->loadFinished()) && !WorkQueue::shared()->count())
        dump();

    m_waitToDump = false;
    waitForPolicy = false;
}

JSStringRef TestRunner::pathToLocalResource(JSContextRef, JSStringRef url)
{
    return JSStringRetain(url);
}

void TestRunner::queueLoad(JSStringRef url, JSStringRef target)
{
    size_t urlArrSize = JSStringGetMaximumUTF8CStringSize(url);
    OwnArrayPtr<char> urlArr = adoptArrayPtr(new char[urlArrSize]);
    JSStringGetUTF8CString(url, urlArr.get(), urlArrSize);

    WebCore::KURL base = mainFrame->loader()->documentLoader()->response().url();
    WebCore::KURL absolute(base, urlArr.get());

    JSRetainPtr<JSStringRef> absoluteURL(Adopt, JSStringCreateWithUTF8CString(absolute.string().utf8().data()));
    WorkQueue::shared()->queue(new LoadItem(absoluteURL.get(), target));
}

void TestRunner::setAcceptsEditing(bool acceptsEditing)
{
    BlackBerry::WebKit::DumpRenderTree::currentInstance()->setAcceptsEditing(acceptsEditing);
}

void TestRunner::setAppCacheMaximumSize(unsigned long long quota)
{
    UNUSED_PARAM(quota);
    notImplemented();
}

void TestRunner::setAuthorAndUserStylesEnabled(bool enable)
{
    mainFrame->page()->settings()->setAuthorAndUserStylesEnabled(enable);
}

void TestRunner::setCacheModel(int)
{
    notImplemented();
}

void TestRunner::setCustomPolicyDelegate(bool setDelegate, bool permissive)
{
    UNUSED_PARAM(setDelegate);
    UNUSED_PARAM(permissive);
    notImplemented();
}

void TestRunner::clearApplicationCacheForOrigin(OpaqueJSString*)
{
    // FIXME: Implement to support deleting all application caches for an origin.
    notImplemented();
}

long long TestRunner::localStorageDiskUsageForOrigin(JSStringRef)
{
    // FIXME: Implement to support getting disk usage in bytes for an origin.
    notImplemented();
    return 0;
}

JSValueRef TestRunner::originsWithApplicationCache(JSContextRef context)
{
    // FIXME: Implement to get origins that contain application caches.
    notImplemented();
    return JSValueMakeUndefined(context);
}

void TestRunner::setDatabaseQuota(unsigned long long quota)
{
    if (!mainFrame)
        return;

    WebCore::DatabaseTracker::tracker().setQuota(mainFrame->document()->securityOrigin(), quota);
}

void TestRunner::setDomainRelaxationForbiddenForURLScheme(bool forbidden, JSStringRef scheme)
{
    WebCore::SchemeRegistry::setDomainRelaxationForbiddenForURLScheme(forbidden, jsStringRefToWebCoreString(scheme));
}

void TestRunner::setIconDatabaseEnabled(bool iconDatabaseEnabled)
{
    UNUSED_PARAM(iconDatabaseEnabled);
    notImplemented();
}

void TestRunner::setMainFrameIsFirstResponder(bool flag)
{
    UNUSED_PARAM(flag);
    notImplemented();
}

void TestRunner::setPersistentUserStyleSheetLocation(JSStringRef path)
{
    UNUSED_PARAM(path);
    notImplemented();
}

void TestRunner::setPopupBlockingEnabled(bool flag)
{
    BlackBerry::WebKit::DumpRenderTree::currentInstance()->page()->settings()->setJavaScriptOpenWindowsAutomatically(!flag);
}

void TestRunner::setPrivateBrowsingEnabled(bool flag)
{
    UNUSED_PARAM(flag);
    notImplemented();
}

void TestRunner::setXSSAuditorEnabled(bool flag)
{
    BlackBerry::WebKit::DumpRenderTree::currentInstance()->page()->settings()->setXSSAuditorEnabled(flag);
}

void TestRunner::setSelectTrailingWhitespaceEnabled(bool flag)
{
    BlackBerry::WebKit::DumpRenderTree::currentInstance()->setSelectTrailingWhitespaceEnabled(flag);
}

void TestRunner::setSmartInsertDeleteEnabled(bool flag)
{
    UNUSED_PARAM(flag);
    notImplemented();
}

void TestRunner::setTabKeyCyclesThroughElements(bool cycles)
{
    if (!mainFrame)
        return;

    mainFrame->page()->setTabKeyCyclesThroughElements(cycles);
}

void TestRunner::setUseDashboardCompatibilityMode(bool flag)
{
    UNUSED_PARAM(flag);
    notImplemented();
}

void TestRunner::setUserStyleSheetEnabled(bool flag)
{
    UNUSED_PARAM(flag);
    notImplemented();
}

void TestRunner::setUserStyleSheetLocation(JSStringRef path)
{
    String pathStr = jsStringRefToWebCoreString(path);
    BlackBerry::WebKit::DumpRenderTree::currentInstance()->page()->settings()->setUserStyleSheetLocation(pathStr.utf8().data());
}

void TestRunner::waitForPolicyDelegate()
{
    setWaitToDump(true);
    waitForPolicy = true;
}

size_t TestRunner::webHistoryItemCount()
{
    SharedArray<BlackBerry::WebKit::WebPage::BackForwardEntry> backForwardList;
    BlackBerry::WebKit::DumpRenderTree::currentInstance()->page()->getBackForwardList(backForwardList);
    return backForwardList.length();
}

int TestRunner::windowCount()
{
    notImplemented();
    return 0;
}

bool TestRunner::elementDoesAutoCompleteForElementWithId(JSStringRef id)
{
    UNUSED_PARAM(id);
    notImplemented();
    return false;
}

void TestRunner::setWaitToDump(bool waitToDump)
{
    // Change from 30s to 35s because some test cases in multipart need 30 seconds,
    // refer to http/tests/multipart/resources/multipart-wait-before-boundary.php please.
    static const double kWaitToDumpWatchdogInterval = 35.0;
    m_waitToDump = waitToDump;
    if (m_waitToDump)
        BlackBerry::WebKit::DumpRenderTree::currentInstance()->setWaitToDumpWatchdog(kWaitToDumpWatchdogInterval);
}

void TestRunner::setWindowIsKey(bool windowIsKey)
{
    m_windowIsKey = windowIsKey;
    notImplemented();
}

bool TestRunner::pauseAnimationAtTimeOnElementWithId(JSStringRef animationName, double time, JSStringRef elementId)
{
    if (!mainFrame)
        return false;

    int nameLen = JSStringGetMaximumUTF8CStringSize(animationName);
    int idLen = JSStringGetMaximumUTF8CStringSize(elementId);
    OwnArrayPtr<char> name = adoptArrayPtr(new char[nameLen]);
    OwnArrayPtr<char> eId = adoptArrayPtr(new char[idLen]);

    JSStringGetUTF8CString(animationName, name.get(), nameLen);
    JSStringGetUTF8CString(elementId, eId.get(), idLen);

    WebCore::AnimationController* animationController = mainFrame->animation();
    if (!animationController)
        return false;

    WebCore::Node* node = mainFrame->document()->getElementById(eId.get());
    if (!node || !node->renderer())
        return false;

    return animationController->pauseAnimationAtTime(node->renderer(), name.get(), time);
}

bool TestRunner::pauseTransitionAtTimeOnElementWithId(JSStringRef propertyName, double time, JSStringRef elementId)
{
    if (!mainFrame)
        return false;

    int nameLen = JSStringGetMaximumUTF8CStringSize(propertyName);
    int idLen = JSStringGetMaximumUTF8CStringSize(elementId);
    OwnArrayPtr<char> name = adoptArrayPtr(new char[nameLen]);
    OwnArrayPtr<char> eId = adoptArrayPtr(new char[idLen]);

    JSStringGetUTF8CString(propertyName, name.get(), nameLen);
    JSStringGetUTF8CString(elementId, eId.get(), idLen);

    WebCore::AnimationController* animationController = mainFrame->animation();
    if (!animationController)
        return false;

    WebCore::Node* node = mainFrame->document()->getElementById(eId.get());
    if (!node || !node->renderer())
        return false;

    return animationController->pauseTransitionAtTime(node->renderer(), name.get(), time);
}

unsigned TestRunner::numberOfActiveAnimations() const
{
    if (!mainFrame)
        return false;

    WebCore::AnimationController* animationController = mainFrame->animation();
    if (!animationController)
        return false;

    return animationController->numberOfActiveAnimations(mainFrame->document());
}

unsigned int TestRunner::workerThreadCount() const
{
#if ENABLE_WORKERS
    return WebCore::WorkerThread::workerThreadCount();
#else
    return 0;
#endif
}

void TestRunner::removeAllVisitedLinks()
{
    notImplemented();
}

void TestRunner::disableImageLoading()
{
    BlackBerry::WebKit::DumpRenderTree::currentInstance()->page()->settings()->setLoadsImagesAutomatically(false);
}

void TestRunner::overridePreference(JSStringRef key, JSStringRef value)
{
    if (!mainFrame)
        return;

    String keyStr = jsStringRefToWebCoreString(key);
    String valueStr = jsStringRefToWebCoreString(value);

    if (keyStr == "WebKitUsesPageCachePreferenceKey")
        BlackBerry::WebKit::DumpRenderTree::currentInstance()->page()->settings()->setMaximumPagesInCache(1);
    else if (keyStr == "WebKitUsePreHTML5ParserQuirks")
        mainFrame->page()->settings()->setUsePreHTML5ParserQuirks(true);
    else if (keyStr == "WebKitTabToLinksPreferenceKey")
        DumpRenderTreeSupport::setLinksIncludedInFocusChain(valueStr == "true" || valueStr == "1");
    else if (keyStr == "WebKitHyperlinkAuditingEnabled")
        mainFrame->page()->settings()->setHyperlinkAuditingEnabled(valueStr == "true" || valueStr == "1");
}

void TestRunner::setAlwaysAcceptCookies(bool alwaysAcceptCookies)
{
    UNUSED_PARAM(alwaysAcceptCookies);
    notImplemented();
}

void TestRunner::setMockGeolocationPosition(double latitude, double longitude, double accuracy)
{
    DumpRenderTreeSupport::setMockGeolocationPosition(BlackBerry::WebKit::DumpRenderTree::currentInstance()->page(), latitude, longitude, accuracy);
}

void TestRunner::setMockGeolocationError(int code, JSStringRef message)
{
    String messageStr = jsStringRefToWebCoreString(message);
    DumpRenderTreeSupport::setMockGeolocationError(BlackBerry::WebKit::DumpRenderTree::currentInstance()->page(), code, messageStr);
}

void TestRunner::showWebInspector()
{
    notImplemented();
}

void TestRunner::closeWebInspector()
{
    notImplemented();
}

void TestRunner::evaluateInWebInspector(long callId, JSStringRef script)
{
    UNUSED_PARAM(callId);
    UNUSED_PARAM(script);
    notImplemented();
}

void TestRunner::evaluateScriptInIsolatedWorldAndReturnValue(unsigned worldID, JSObjectRef globalObject, JSStringRef script)
{
    UNUSED_PARAM(worldID);
    UNUSED_PARAM(globalObject);
    UNUSED_PARAM(script);
    notImplemented();
}

void TestRunner::evaluateScriptInIsolatedWorld(unsigned worldID, JSObjectRef globalObject, JSStringRef script)
{
    UNUSED_PARAM(worldID);
    UNUSED_PARAM(globalObject);
    UNUSED_PARAM(script);
    notImplemented();
}

void TestRunner::addUserScript(JSStringRef source, bool runAtStart, bool allFrames)
{
    UNUSED_PARAM(source);
    UNUSED_PARAM(runAtStart);
    UNUSED_PARAM(allFrames);
    notImplemented();
}

void TestRunner::addUserStyleSheet(JSStringRef, bool)
{
    notImplemented();
}

void TestRunner::setScrollbarPolicy(JSStringRef, JSStringRef)
{
    notImplemented();
}

void TestRunner::setWebViewEditable(bool)
{
    notImplemented();
}

void TestRunner::authenticateSession(JSStringRef url, JSStringRef username, JSStringRef password)
{
    notImplemented();
}

bool TestRunner::callShouldCloseOnWebView()
{
    notImplemented();
    return false;
}

void TestRunner::setFrameFlatteningEnabled(bool enable)
{
    BlackBerry::WebKit::DumpRenderTree::currentInstance()->page()->settings()->setFrameFlatteningEnabled(enable);
}

void TestRunner::setSpatialNavigationEnabled(bool enable)
{
    notImplemented();
}

void TestRunner::addOriginAccessWhitelistEntry(JSStringRef sourceOrigin, JSStringRef destinationProtocol, JSStringRef destinationHost, bool allowDestinationSubdomains)
{
    WebCore::SecurityPolicy::addOriginAccessWhitelistEntry(*WebCore::SecurityOrigin::createFromString(jsStringRefToWebCoreString(sourceOrigin)),
                                                  jsStringRefToWebCoreString(destinationProtocol),
                                                  jsStringRefToWebCoreString(destinationHost),
                                                  allowDestinationSubdomains);
}

void TestRunner::removeOriginAccessWhitelistEntry(JSStringRef sourceOrigin, JSStringRef destinationProtocol, JSStringRef destinationHost, bool allowDestinationSubdomains)
{
    WebCore::SecurityPolicy::removeOriginAccessWhitelistEntry(*WebCore::SecurityOrigin::createFromString(jsStringRefToWebCoreString(sourceOrigin)),
                                                     jsStringRefToWebCoreString(destinationProtocol),
                                                     jsStringRefToWebCoreString(destinationHost),
                                                     allowDestinationSubdomains);
}

void TestRunner::setAllowFileAccessFromFileURLs(bool enabled)
{
    if (!mainFrame)
        return;

    mainFrame->page()->settings()->setAllowFileAccessFromFileURLs(enabled);
}

void TestRunner::setAllowUniversalAccessFromFileURLs(bool enabled)
{
    if (!mainFrame)
        return;

    mainFrame->page()->settings()->setAllowUniversalAccessFromFileURLs(enabled);
}

void TestRunner::apiTestNewWindowDataLoadBaseURL(JSStringRef utf8Data, JSStringRef baseURL)
{
    notImplemented();
}

void TestRunner::apiTestGoToCurrentBackForwardItem()
{
    notImplemented();
}

void TestRunner::setJavaScriptCanAccessClipboard(bool flag)
{
    BlackBerry::WebKit::DumpRenderTree::currentInstance()->page()->setJavaScriptCanAccessClipboard(flag);
}

JSValueRef TestRunner::computedStyleIncludingVisitedInfo(JSContextRef context, JSValueRef value)
{
    return DumpRenderTreeSupport::computedStyleIncludingVisitedInfo(context, value);
}

JSRetainPtr<JSStringRef> TestRunner::layerTreeAsText() const
{
    notImplemented();
    return 0;
}

JSRetainPtr<JSStringRef> TestRunner::markerTextForListItem(JSContextRef context, JSValueRef nodeObject) const
{
    WebCore::Element* element = toElement(toJS(toJS(context), nodeObject));
    if (!element)
        return 0;

    JSRetainPtr<JSStringRef> markerText(Adopt, JSStringCreateWithUTF8CString(WebCore::markerTextForListItem(element).utf8().data()));
    return markerText;
}

void TestRunner::setPluginsEnabled(bool flag)
{
    notImplemented();
}

void TestRunner::abortModal()
{
    notImplemented();
}

void TestRunner::clearAllApplicationCaches()
{
    notImplemented();
}

void TestRunner::setApplicationCacheOriginQuota(unsigned long long quota)
{
    notImplemented();
}

void TestRunner::setMockDeviceOrientation(bool canProvideAlpha, double alpha, bool canProvideBeta, double beta, bool canProvideGamma, double gamma)
{
    notImplemented();
}

void TestRunner::addMockSpeechInputResult(JSStringRef result, double confidence, JSStringRef language)
{
    notImplemented();
}

void TestRunner::setGeolocationPermission(bool allow)
{
    setGeolocationPermissionCommon(allow);
    DumpRenderTreeSupport::setMockGeolocationPermission(BlackBerry::WebKit::DumpRenderTree::currentInstance()->page(), allow);
}

void TestRunner::setViewModeMediaFeature(const JSStringRef mode)
{
    notImplemented();
}

void TestRunner::setSerializeHTTPLoads(bool)
{
    // FIXME: Implement if needed for https://bugs.webkit.org/show_bug.cgi?id=50758.
    notImplemented();
}

void TestRunner::setMinimumTimerInterval(double)
{
    notImplemented();
}

void TestRunner::setTextDirection(JSStringRef)
{
    notImplemented();
}

void TestRunner::goBack()
{
    // FIXME: implement to enable loader/navigation-while-deferring-loads.html
    notImplemented();
}

void TestRunner::setDefersLoading(bool)
{
    // FIXME: implement to enable loader/navigation-while-deferring-loads.html
    notImplemented();
}

JSValueRef TestRunner::originsWithLocalStorage(JSContextRef context)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

void TestRunner::observeStorageTrackerNotifications(unsigned)
{
    notImplemented();
}

void TestRunner::syncLocalStorage()
{
    notImplemented();
}

void TestRunner::deleteAllLocalStorage()
{
    notImplemented();
}

void TestRunner::setAsynchronousSpellCheckingEnabled(bool)
{
    notImplemented();
}

void TestRunner::setAutofilled(JSContextRef context, JSValueRef nodeObject, bool autofilled)
{
    JSC::ExecState* exec = toJS(context);
    WebCore::Element* element = toElement(toJS(exec, nodeObject));
    if (!element)
        return;
    WebCore::HTMLInputElement* inputElement = element->toInputElement();
    if (!inputElement)
        return;

    inputElement->setAutofilled(autofilled);
}

int TestRunner::numberOfPendingGeolocationPermissionRequests()
{
    return DumpRenderTreeSupport::numberOfPendingGeolocationPermissionRequests(BlackBerry::WebKit::DumpRenderTree::currentInstance()->page());
}

bool TestRunner::findString(JSContextRef context, JSStringRef target, JSObjectRef optionsArray)
{
    WebCore::FindOptions options = 0;

    String nameStr = jsStringRefToWebCoreString(target);

    JSRetainPtr<JSStringRef> lengthPropertyName(Adopt, JSStringCreateWithUTF8CString("length"));
    size_t length = 0;
    if (optionsArray) {
        JSValueRef lengthValue = JSObjectGetProperty(context, optionsArray, lengthPropertyName.get(), 0);
        if (!JSValueIsNumber(context, lengthValue))
            return false;
        length = static_cast<size_t>(JSValueToNumber(context, lengthValue, 0));
    }

    for (size_t i = 0; i < length; ++i) {
        JSValueRef value = JSObjectGetPropertyAtIndex(context, optionsArray, i, 0);
        if (!JSValueIsString(context, value))
            continue;

        JSRetainPtr<JSStringRef> optionName(Adopt, JSValueToStringCopy(context, value, 0));

        if (JSStringIsEqualToUTF8CString(optionName.get(), "CaseInsensitive"))
            options |= WebCore::CaseInsensitive;
        else if (JSStringIsEqualToUTF8CString(optionName.get(), "AtWordStarts"))
            options |= WebCore::AtWordStarts;
        else if (JSStringIsEqualToUTF8CString(optionName.get(), "TreatMedialCapitalAsWordStart"))
            options |= WebCore::TreatMedialCapitalAsWordStart;
        else if (JSStringIsEqualToUTF8CString(optionName.get(), "Backwards"))
            options |= WebCore::Backwards;
        else if (JSStringIsEqualToUTF8CString(optionName.get(), "WrapAround"))
            options |= WebCore::WrapAround;
        else if (JSStringIsEqualToUTF8CString(optionName.get(), "StartInSelection"))
            options |= WebCore::StartInSelection;
    }

    // Our layout tests assume find will wrap and highlight all matches.
    return BlackBerry::WebKit::DumpRenderTree::currentInstance()->page()->findNextString(nameStr.utf8().data(),
        !(options & WebCore::Backwards), !(options & WebCore::CaseInsensitive), true /* wrap */, true /* highlightAllMatches */);
}

void TestRunner::deleteLocalStorageForOrigin(JSStringRef URL)
{
    // FIXME: Implement.
}

void TestRunner::setValueForUser(JSContextRef context, JSValueRef nodeObject, JSStringRef value)
{
    JSC::ExecState* exec = toJS(context);
    WebCore::Element* element = toElement(toJS(exec, nodeObject));
    if (!element)
        return;
    WebCore::HTMLInputElement* inputElement = element->toInputElement();
    if (!inputElement)
        return;

    inputElement->setValueForUser(jsStringRefToWebCoreString(value));
}

long long TestRunner::applicationCacheDiskUsageForOrigin(JSStringRef origin)
{
    // FIXME: Implement to support getting disk usage by all application caches for an origin.
    return 0;
}

void TestRunner::addChromeInputField()
{
}

void TestRunner::removeChromeInputField()
{
}

void TestRunner::focusWebView()
{
}

void TestRunner::setBackingScaleFactor(double)
{
}

void TestRunner::setMockSpeechInputDumpRect(bool)
{
}

void TestRunner::grantWebNotificationPermission(JSStringRef origin)
{
}

void TestRunner::denyWebNotificationPermission(JSStringRef jsOrigin)
{
}

void TestRunner::removeAllWebNotificationPermissions()
{
}

void TestRunner::simulateWebNotificationClick(JSValueRef jsNotification)
{
}

void TestRunner::simulateLegacyWebNotificationClick(JSStringRef title)
{
}

void TestRunner::resetPageVisibility()
{
    notImplemented();
}

void TestRunner::setPageVisibility(const char*)
{
    notImplemented();
}

void TestRunner::setAutomaticLinkDetectionEnabled(bool)
{
    notImplemented();
}

void TestRunner::sendWebIntentResponse(JSStringRef)
{
    notImplemented();
}

void TestRunner::deliverWebIntent(JSStringRef, JSStringRef, JSStringRef)
{
    notImplemented();
}
