/*
 * Copyright (C) 2008 Kevin Ollivier <kevino@theolliviers.com>
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
#include "LayoutTestController.h"

#include <wtf/text/CString.h>
#include "DumpRenderTree.h"
#include "JSStringUtils.h"
#include "NotImplemented.h"
#include "WorkQueue.h"
#include "WorkQueueItem.h"
#include <JavaScriptCore/JSRetainPtr.h>
#include <JavaScriptCore/JSStringRef.h>
#include <JavaScriptCore/OpaqueJSString.h>

void notifyDoneFired();

LayoutTestController::~LayoutTestController()
{
}

void LayoutTestController::addDisallowedURL(JSStringRef)
{
    notImplemented();
}

void LayoutTestController::clearBackForwardList()
{
    notImplemented();
}

JSStringRef LayoutTestController::copyDecodedHostName(JSStringRef)
{
    notImplemented();
    return 0;
}

JSStringRef LayoutTestController::copyEncodedHostName(JSStringRef)
{
    notImplemented();
    return 0;
}

void LayoutTestController::dispatchPendingLoadRequests()
{
    // FIXME: Implement for testing fix for 6727495
    notImplemented();
}

void LayoutTestController::display()
{
    notImplemented();
}

JSRetainPtr<JSStringRef> LayoutTestController::counterValueForElementById(JSStringRef id)
{
    notImplemented();
    return JSRetainPtr<JSStringRef>(Adopt, JSStringCreateWithUTF8CString(""));
}

void LayoutTestController::keepWebHistory()
{
    notImplemented();
}

JSValueRef LayoutTestController::computedStyleIncludingVisitedInfo(JSContextRef context, JSValueRef value)
{
    notImplemented();
}

JSRetainPtr<JSStringRef> LayoutTestController::layerTreeAsText() const
{
    notImplemented();
    return JSRetainPtr<JSStringRef>(Adopt, JSStringCreateWithUTF8CString(""));
}

int LayoutTestController::pageNumberForElementById(JSStringRef id, float pageWidth, float pageHeight)
{
    notImplemented();	
    return 0;
}

int LayoutTestController::numberOfPages(float pageWidth, float pageHeight)
{
	notImplemented();
    return 0;
}

JSRetainPtr<JSStringRef> LayoutTestController::pageProperty(const char* propertyName, int pageNumber) const
{
    notImplemented();
    return 0;
}

bool LayoutTestController::isPageBoxVisible(int pageIndex) const
{
    notImplemented();
    return false;
}

JSRetainPtr<JSStringRef> LayoutTestController::pageSizeAndMarginsInPixels(int pageNumber, int width, int height, int marginTop, int marginRight, int marginBottom, int marginLeft) const
{
    notImplemented();
    return 0;
}

size_t LayoutTestController::webHistoryItemCount()
{
    notImplemented();
    return 0;
}

unsigned LayoutTestController::workerThreadCount() const
{
    notImplemented();
    return 0;
}

void LayoutTestController::notifyDone()
{
    if (m_waitToDump && !WorkQueue::shared()->count())
        notifyDoneFired();
    m_waitToDump = false;
}

JSStringRef LayoutTestController::pathToLocalResource(JSContextRef context, JSStringRef url)
{
    // Function introduced in r28690. This may need special-casing on Windows.
    return JSStringRetain(url); // Do nothing on Unix.
}

void LayoutTestController::queueLoad(JSStringRef url, JSStringRef target)
{
    // FIXME: We need to resolve relative URLs here
    WorkQueue::shared()->queue(new LoadItem(url, target));
}

void LayoutTestController::setAcceptsEditing(bool acceptsEditing)
{
    notImplemented();
}

void LayoutTestController::setAlwaysAcceptCookies(bool alwaysAcceptCookies)
{
    notImplemented();
}

void LayoutTestController::setCustomPolicyDelegate(bool, bool)
{
    notImplemented();
}

void LayoutTestController::waitForPolicyDelegate()
{
    notImplemented();
}

void LayoutTestController::setScrollbarPolicy(JSStringRef, JSStringRef)
{
    notImplemented();
}

void LayoutTestController::addOriginAccessWhitelistEntry(JSStringRef sourceOrigin, JSStringRef protocol, JSStringRef host, bool includeSubdomains)
{
    notImplemented();
}

void LayoutTestController::removeOriginAccessWhitelistEntry(JSStringRef sourceOrigin, JSStringRef protocol, JSStringRef host, bool includeSubdomains)
{
    notImplemented();
}

void LayoutTestController::setMainFrameIsFirstResponder(bool)
{
    notImplemented();
}

void LayoutTestController::setTabKeyCyclesThroughElements(bool)
{
    notImplemented();
}

void LayoutTestController::setUseDashboardCompatibilityMode(bool)
{
    notImplemented();
}

void LayoutTestController::setUserStyleSheetEnabled(bool flag)
{
    notImplemented();
}

void LayoutTestController::setUserStyleSheetLocation(JSStringRef path)
{
    notImplemented();
}

void LayoutTestController::setValueForUser(JSContextRef context, JSValueRef nodeObject, JSStringRef value)
{
    notImplemented();
}

void LayoutTestController::setViewModeMediaFeature(JSStringRef mode)
{
    notImplemented();
}

void LayoutTestController::setWindowIsKey(bool)
{
    notImplemented();
}

void LayoutTestController::setSmartInsertDeleteEnabled(bool flag)
{
    notImplemented();
}

void LayoutTestController::setWaitToDump(bool waitUntilDone)
{
    static const int timeoutSeconds = 10;
    m_waitToDump = waitUntilDone;
}

int LayoutTestController::windowCount()
{
	notImplemented();
    return 1;
}

void LayoutTestController::setPrivateBrowsingEnabled(bool flag)
{
    notImplemented();
}

void LayoutTestController::setJavaScriptCanAccessClipboard(bool)
{
    notImplemented();
}

void LayoutTestController::setXSSAuditorEnabled(bool flag)
{
    notImplemented();
}

void LayoutTestController::setFrameFlatteningEnabled(bool flag)
{
    notImplemented();
}

void LayoutTestController::setSpatialNavigationEnabled(bool flag)
{
    notImplemented();
}

void LayoutTestController::setAllowUniversalAccessFromFileURLs(bool)
{
    notImplemented();
}

void LayoutTestController::setAllowFileAccessFromFileURLs(bool)
{
    notImplemented();
}

void LayoutTestController::setAuthorAndUserStylesEnabled(bool flag)
{
    notImplemented();
}

void LayoutTestController::setAutofilled(JSContextRef context, JSValueRef nodeObject, bool autofilled)
{
    notImplemented();
}

void LayoutTestController::disableImageLoading()
{
    notImplemented();
}

void LayoutTestController::setMockDeviceOrientation(bool, double, bool, double, bool, double)
{
    // FIXME: Implement for DeviceOrientation layout tests.
    // See https://bugs.webkit.org/show_bug.cgi?id=30335.
    notImplemented();
}

void LayoutTestController::setMockGeolocationPosition(double, double, double)
{
    // FIXME: Implement for Geolocation layout tests.
    // See https://bugs.webkit.org/show_bug.cgi?id=28264.
    notImplemented();
}

void LayoutTestController::setMockGeolocationError(int, JSStringRef)
{
    // FIXME: Implement for Geolocation layout tests.
    // See https://bugs.webkit.org/show_bug.cgi?id=28264.
    notImplemented();
}

void LayoutTestController::setGeolocationPermission(bool allow)
{
    // FIXME: Implement for Geolocation layout tests.
    setGeolocationPermissionCommon(allow);
}

int LayoutTestController::numberOfPendingGeolocationPermissionRequests()
{
    // FIXME: Implement for Geolocation layout tests.
    return -1;
}

void LayoutTestController::addMockSpeechInputResult(JSStringRef, double, JSStringRef)
{
    // FIXME: Implement for speech input layout tests.
    // See https://bugs.webkit.org/show_bug.cgi?id=39485.
    notImplemented();
}

void LayoutTestController::setMockSpeechInputDumpRect(bool)
{
    // FIXME: Implement for speech input layout tests.
    // See https://bugs.webkit.org/show_bug.cgi?id=39485.
    notImplemented();
}

void LayoutTestController::startSpeechInput(JSContextRef inputElement)
{
    // FIXME: Implement for speech input layout tests.
    // See https://bugs.webkit.org/show_bug.cgi?id=39485.
    notImplemented();
}

void LayoutTestController::setIconDatabaseEnabled(bool enabled)
{
    notImplemented();
}

void LayoutTestController::setJavaScriptProfilingEnabled(bool enabled)
{
    notImplemented();
}

void LayoutTestController::setSelectTrailingWhitespaceEnabled(bool flag)
{
    notImplemented();
}

void LayoutTestController::setPopupBlockingEnabled(bool flag)
{
    notImplemented();
}

void LayoutTestController::setPluginsEnabled(bool flag)
{
    notImplemented();
}

bool LayoutTestController::elementDoesAutoCompleteForElementWithId(JSStringRef id)
{
    notImplemented();
    return false;
}

void LayoutTestController::execCommand(JSStringRef, JSStringRef)
{
    notImplemented();
}

bool LayoutTestController::findString(JSContextRef context, JSStringRef target, JSObjectRef optionsArray)
{
    notImplemented();
}

bool LayoutTestController::isCommandEnabled(JSStringRef name)
{
    return false;
}

void LayoutTestController::setCacheModel(int)
{
    notImplemented();
}

void LayoutTestController::setPersistentUserStyleSheetLocation(JSStringRef)
{
    notImplemented();
}

void LayoutTestController::clearPersistentUserStyleSheet()
{
    notImplemented();
}

void LayoutTestController::clearAllApplicationCaches()
{
    notImplemented();
}

void LayoutTestController::setApplicationCacheOriginQuota(unsigned long long)
{
    // FIXME: Implement to support application cache quotas.
    notImplemented();
}

void LayoutTestController::clearApplicationCacheForOrigin(OpaqueJSString*)
{
    // FIXME: Implement to support deleting all application caches for an origin.
    notImplemented();
}

long long LayoutTestController::localStorageDiskUsageForOrigin(JSStringRef)
{
    // FIXME: Implement to support getting disk usage in bytes for an origin.
    notImplemented();
    return 0;
}

JSValueRef LayoutTestController::originsWithApplicationCache(JSContextRef context)
{
    // FIXME: Implement to get origins that contain application caches.
    notImplemented();
    return JSValueMakeUndefined(context);
}

long long LayoutTestController::applicationCacheDiskUsageForOrigin(JSStringRef)
{
    notImplemented();
    return 0;
}

void LayoutTestController::clearAllDatabases()
{
    notImplemented();
}

void LayoutTestController::setDatabaseQuota(unsigned long long quota)
{
    notImplemented();
}

JSValueRef LayoutTestController::originsWithLocalStorage(JSContextRef context)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

void LayoutTestController::deleteAllLocalStorage()
{
    notImplemented();
}

void LayoutTestController::deleteLocalStorageForOrigin(JSStringRef)
{
    notImplemented();
}

void LayoutTestController::observeStorageTrackerNotifications(unsigned)
{
    notImplemented();
}

void LayoutTestController::syncLocalStorage()
{
    notImplemented();
}

void LayoutTestController::setDomainRelaxationForbiddenForURLScheme(bool, JSStringRef)
{
    notImplemented();
}

void LayoutTestController::goBack()
{
    notImplemented();
}

void LayoutTestController::setDefersLoading(bool defers)
{
    notImplemented();
}

void LayoutTestController::setAppCacheMaximumSize(unsigned long long size)
{
    notImplemented();
}

bool LayoutTestController::pauseAnimationAtTimeOnElementWithId(JSStringRef animationName, double time, JSStringRef elementId)
{
    notImplemented();
    return false;
}

bool LayoutTestController::pauseTransitionAtTimeOnElementWithId(JSStringRef propertyName, double time, JSStringRef elementId)
{
	notImplemented();
    return false;
}

unsigned LayoutTestController::numberOfActiveAnimations() const
{
	notImplemented();
    return 0;
}

void LayoutTestController::suspendAnimations() const
{
    notImplemented();
}

void LayoutTestController::resumeAnimations() const
{
    notImplemented();
}

static inline bool toBool(JSStringRef value)
{
    return equals(value, "true") || equals(value, "1");
}

static inline int toInt(JSStringRef value)
{
    return atoi(value->ustring().utf8().data());
}

void LayoutTestController::overridePreference(JSStringRef key, JSStringRef value)
{
    notImplemented();
}

void LayoutTestController::addUserScript(JSStringRef, bool, bool)
{
    notImplemented();
}

void LayoutTestController::addUserStyleSheet(JSStringRef source, bool allFrames)
{
    notImplemented();
}

void LayoutTestController::setDeveloperExtrasEnabled(bool enabled)
{
    notImplemented();
}

void LayoutTestController::setAsynchronousSpellCheckingEnabled(bool)
{
    notImplemented();
}

void LayoutTestController::showWebInspector()
{
    notImplemented();
}

void LayoutTestController::closeWebInspector()
{
    notImplemented();
}

void LayoutTestController::evaluateInWebInspector(long, JSStringRef)
{
    notImplemented();
}

void LayoutTestController::evaluateScriptInIsolatedWorldAndReturnValue(unsigned, JSObjectRef, JSStringRef)
{
    notImplemented();
}

void LayoutTestController::evaluateScriptInIsolatedWorld(unsigned, JSObjectRef, JSStringRef)
{
    notImplemented();
}

void LayoutTestController::removeAllVisitedLinks()
{
    notImplemented();
}

bool LayoutTestController::callShouldCloseOnWebView()
{
    notImplemented();
    return false;
}

void LayoutTestController::apiTestNewWindowDataLoadBaseURL(JSStringRef, JSStringRef)
{
    notImplemented();
}

void LayoutTestController::apiTestGoToCurrentBackForwardItem()
{
    notImplemented();
}

void LayoutTestController::setWebViewEditable(bool)
{
    notImplemented();
}

JSRetainPtr<JSStringRef> LayoutTestController::markerTextForListItem(JSContextRef context, JSValueRef nodeObject) const
{
    notImplemented();
}

void LayoutTestController::authenticateSession(JSStringRef, JSStringRef, JSStringRef)
{
    notImplemented();
}

void LayoutTestController::setEditingBehavior(const char* editingBehavior)
{
    notImplemented();
}

void LayoutTestController::abortModal()
{
    notImplemented();
}

void LayoutTestController::dumpConfigurationForViewport(int deviceDPI, int deviceWidth, int deviceHeight, int availableWidth, int availableHeight)
{
    notImplemented();
}

void LayoutTestController::setSerializeHTTPLoads(bool)
{
    // FIXME: Implement if needed for https://bugs.webkit.org/show_bug.cgi?id=50758.
    notImplemented();
}

void LayoutTestController::setMinimumTimerInterval(double minimumTimerInterval)
{
    notImplemented();
}

void LayoutTestController::setTextDirection(JSStringRef)
{
    notImplemented();
}

void LayoutTestController::allowRoundingHacks()
{
    notImplemented();
}

void LayoutTestController::addChromeInputField()
{
    notImplemented();
}

void LayoutTestController::removeChromeInputField()
{
    notImplemented();
}

void LayoutTestController::focusWebView()
{
    notImplemented();
}

void LayoutTestController::setBackingScaleFactor(double)
{
    notImplemented();
}

void LayoutTestController::simulateDesktopNotificationClick(JSStringRef title)
{
}


