/*
 * Copyright (C) 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#import "config.h"
#import "DumpRenderTree.h"
#import "TestRunner.h"

#import "EditingDelegate.h"
#import "MockGeolocationProvider.h"
#import "MockWebNotificationProvider.h"
#import "PolicyDelegate.h"
#import "StorageTrackerDelegate.h"
#import "UIDelegate.h"
#import "WorkQueue.h"
#import "WorkQueueItem.h"
#import <Foundation/Foundation.h>
#import <JavaScriptCore/JSRetainPtr.h>
#import <JavaScriptCore/JSStringRef.h>
#import <JavaScriptCore/JSStringRefCF.h>
#import <WebKit/DOMDocument.h>
#import <WebKit/DOMElement.h>
#import <WebKit/WebApplicationCache.h>
#import <WebKit/WebBackForwardList.h>
#import <WebKit/WebCoreStatistics.h>
#import <WebKit/WebDOMOperationsPrivate.h>
#import <WebKit/WebDataSource.h>
#import <WebKit/WebDatabaseManagerPrivate.h>
#import <WebKit/WebDeviceOrientation.h>
#import <WebKit/WebDeviceOrientationProviderMock.h>
#import <WebKit/WebFrame.h>
#import <WebKit/WebFrameViewPrivate.h>
#import <WebKit/WebGeolocationPosition.h>
#import <WebKit/WebHTMLRepresentation.h>
#import <WebKit/WebHTMLViewPrivate.h>
#import <WebKit/WebHistory.h>
#import <WebKit/WebHistoryPrivate.h>
#import <WebKit/WebIconDatabasePrivate.h>
#import <WebKit/WebInspectorPrivate.h>
#import <WebKit/WebNSURLExtras.h>
#import <WebKit/WebKitErrors.h>
#import <WebKit/WebPreferences.h>
#import <WebKit/WebPreferencesPrivate.h>
#import <WebKit/WebQuotaManager.h>
#import <WebKit/WebScriptWorld.h>
#import <WebKit/WebSecurityOriginPrivate.h>
#import <WebKit/WebStorageManagerPrivate.h>
#import <WebKit/WebTypesInternal.h>
#import <WebKit/WebView.h>
#import <WebKit/WebViewPrivate.h>
#import <WebKit/WebWorkersPrivate.h>
#import <wtf/CurrentTime.h>
#import <wtf/HashMap.h>
#import <wtf/RetainPtr.h>

@interface CommandValidationTarget : NSObject <NSValidatedUserInterfaceItem>
{
    SEL _action;
}
- (id)initWithAction:(SEL)action;
@end

@implementation CommandValidationTarget

- (id)initWithAction:(SEL)action
{
    self = [super init];
    if (!self)
        return nil;

    _action = action;
    return self;
}

- (SEL)action
{
    return _action;
}

- (NSInteger)tag
{
    return 0;
}

@end

TestRunner::~TestRunner()
{
}

void TestRunner::addDisallowedURL(JSStringRef url)
{
    RetainPtr<CFStringRef> urlCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, url));

    if (!disallowedURLs)
        disallowedURLs = CFSetCreateMutable(kCFAllocatorDefault, 0, NULL);

    // Canonicalize the URL
    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:(NSString *)urlCF.get()]];
    request = [NSURLProtocol canonicalRequestForRequest:request];

    CFSetAddValue(disallowedURLs, [request URL]);
}

bool TestRunner::callShouldCloseOnWebView()
{
    return [[mainFrame webView] shouldClose];
}

void TestRunner::clearAllApplicationCaches()
{
    [WebApplicationCache deleteAllApplicationCaches];
}

long long TestRunner::applicationCacheDiskUsageForOrigin(JSStringRef url)
{
    RetainPtr<CFStringRef> urlCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, url));
    WebSecurityOrigin *origin = [[WebSecurityOrigin alloc] initWithURL:[NSURL URLWithString:(NSString *)urlCF.get()]];
    long long usage = [WebApplicationCache diskUsageForOrigin:origin];
    [origin release];
    return usage;
}

void TestRunner::syncLocalStorage()
{
    [[WebStorageManager sharedWebStorageManager] syncLocalStorage];
}

long long TestRunner::localStorageDiskUsageForOrigin(JSStringRef url)
{
    RetainPtr<CFStringRef> urlCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, url));
    WebSecurityOrigin *origin = [[WebSecurityOrigin alloc] initWithURL:[NSURL URLWithString:(NSString *)urlCF.get()]];
    long long usage = [[WebStorageManager sharedWebStorageManager] diskUsageForOrigin:origin];
    [origin release];
    return usage;
}

void TestRunner::observeStorageTrackerNotifications(unsigned number)
{
    [storageDelegate logNotifications:number controller:this];
}

void TestRunner::clearApplicationCacheForOrigin(JSStringRef url)
{
    RetainPtr<CFStringRef> urlCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, url));

    WebSecurityOrigin *origin = [[WebSecurityOrigin alloc] initWithURL:[NSURL URLWithString:(NSString *)urlCF.get()]];
    [WebApplicationCache deleteCacheForOrigin:origin];
    [origin release];
}

JSValueRef originsArrayToJS(JSContextRef context, NSArray *origins)
{
    NSUInteger count = [origins count];

    JSValueRef jsOriginsArray[count];
    for (NSUInteger i = 0; i < count; i++) {
        NSString *origin = [[origins objectAtIndex:i] databaseIdentifier];
        JSRetainPtr<JSStringRef> originJS(Adopt, JSStringCreateWithCFString((CFStringRef)origin));
        jsOriginsArray[i] = JSValueMakeString(context, originJS.get());
    }

    return JSObjectMakeArray(context, count, jsOriginsArray, NULL);
}

JSValueRef TestRunner::originsWithApplicationCache(JSContextRef context)
{
    return originsArrayToJS(context, [WebApplicationCache originsWithCache]);
}

void TestRunner::clearAllDatabases()
{
    [[WebDatabaseManager sharedWebDatabaseManager] deleteAllDatabases];
}

void TestRunner::deleteAllLocalStorage()
{
    [[WebStorageManager sharedWebStorageManager] deleteAllOrigins];
}

void TestRunner::setStorageDatabaseIdleInterval(double interval)
{
    [WebStorageManager setStorageDatabaseIdleInterval:interval];
}

JSValueRef TestRunner::originsWithLocalStorage(JSContextRef context)
{
    return originsArrayToJS(context, [[WebStorageManager sharedWebStorageManager] origins]);
}

void TestRunner::deleteLocalStorageForOrigin(JSStringRef URL)
{
    RetainPtr<CFStringRef> urlCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, URL));
    
    WebSecurityOrigin *origin = [[WebSecurityOrigin alloc] initWithURL:[NSURL URLWithString:(NSString *)urlCF.get()]];
    [[WebStorageManager sharedWebStorageManager] deleteOrigin:origin];
    [origin release];
}

void TestRunner::clearBackForwardList()
{
    WebBackForwardList *backForwardList = [[mainFrame webView] backForwardList];
    WebHistoryItem *item = [[backForwardList currentItem] retain];

    // We clear the history by setting the back/forward list's capacity to 0
    // then restoring it back and adding back the current item.
    int capacity = [backForwardList capacity];
    [backForwardList setCapacity:0];
    [backForwardList setCapacity:capacity];
    [backForwardList addItem:item];
    [backForwardList goToItem:item];
    [item release];
}

JSStringRef TestRunner::copyDecodedHostName(JSStringRef name)
{
    RetainPtr<CFStringRef> nameCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, name));
    NSString *nameNS = (NSString *)nameCF.get();
    return JSStringCreateWithCFString((CFStringRef)[nameNS _web_decodeHostName]);
}

JSStringRef TestRunner::copyEncodedHostName(JSStringRef name)
{
    RetainPtr<CFStringRef> nameCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, name));
    NSString *nameNS = (NSString *)nameCF.get();
    return JSStringCreateWithCFString((CFStringRef)[nameNS _web_encodeHostName]);
}

void TestRunner::display()
{
    displayWebView();
}

void TestRunner::keepWebHistory()
{
    if (![WebHistory optionalSharedHistory]) {
        WebHistory *history = [[WebHistory alloc] init];
        [WebHistory setOptionalSharedHistory:history];
        [history release];
    }
}

JSValueRef TestRunner::computedStyleIncludingVisitedInfo(JSContextRef context, JSValueRef value)
{   
    return [[mainFrame webView] _computedStyleIncludingVisitedInfo:context forElement:value];
}

JSRetainPtr<JSStringRef> TestRunner::layerTreeAsText() const
{
    JSRetainPtr<JSStringRef> string(Adopt, JSStringCreateWithCFString((CFStringRef)[mainFrame _layerTreeAsText]));
    return string;
}

JSRetainPtr<JSStringRef> TestRunner::markerTextForListItem(JSContextRef context, JSValueRef nodeObject) const
{
    DOMElement *element = [DOMElement _DOMElementFromJSContext:context value:nodeObject];
    if (!element)
        return JSRetainPtr<JSStringRef>();

    JSRetainPtr<JSStringRef> markerText(Adopt, JSStringCreateWithCFString((CFStringRef)[element _markerTextForListItem]));
    return markerText;
}

int TestRunner::numberOfPendingGeolocationPermissionRequests()
{
    return [[[mainFrame webView] UIDelegate] numberOfPendingGeolocationPermissionRequests];
}

size_t TestRunner::webHistoryItemCount()
{
    return [[[WebHistory optionalSharedHistory] allItems] count];
}

unsigned TestRunner::workerThreadCount() const
{
    return [WebWorkersPrivate workerThreadCount];
}

JSRetainPtr<JSStringRef> TestRunner::platformName() const
{
    JSRetainPtr<JSStringRef> platformName(Adopt, JSStringCreateWithUTF8CString("mac"));
    return platformName;
}

void TestRunner::notifyDone()
{
    if (m_waitToDump && !topLoadingFrame && !WorkQueue::shared()->count())
        dump();
    m_waitToDump = false;
}

static inline std::string stringFromJSString(JSStringRef jsString)
{
    size_t maxBufferSize = JSStringGetMaximumUTF8CStringSize(jsString);
    char* utf8Buffer = new char[maxBufferSize];
    size_t bytesWrittenToUTF8Buffer = JSStringGetUTF8CString(jsString, utf8Buffer, maxBufferSize);
    std::string stdString(utf8Buffer, bytesWrittenToUTF8Buffer - 1); // bytesWrittenToUTF8Buffer includes a trailing \0 which std::string doesn't need.
    delete[] utf8Buffer;
    return stdString;
}

static inline size_t indexOfSeparatorAfterDirectoryName(const std::string& directoryName, const std::string& fullPath)
{
    std::string searchKey = "/" + directoryName + "/";
    size_t indexOfSearchKeyStart = fullPath.rfind(searchKey);
    if (indexOfSearchKeyStart == std::string::npos) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    // Callers expect the return value not to end in "/", so searchKey.length() - 1.
    return indexOfSearchKeyStart + searchKey.length() - 1;
}

static inline std::string resourceRootAbsolutePath(const std::string& testPathOrURL, const std::string& expectedRootName)
{
    char* localResourceRootEnv = getenv("LOCAL_RESOURCE_ROOT");
    if (localResourceRootEnv)
        return std::string(localResourceRootEnv);

    // This fallback approach works for non-http tests and is useful
    // in the case when we're running DRT directly from the command line.
    return testPathOrURL.substr(0, indexOfSeparatorAfterDirectoryName(expectedRootName, testPathOrURL));
}

JSStringRef TestRunner::pathToLocalResource(JSContextRef context, JSStringRef localResourceJSString)
{
    // The passed in path will be an absolute path to the resource starting
    // with "/tmp" or "/tmp/LayoutTests", optionally starting with the explicit file:// protocol.
    // /tmp maps to DUMPRENDERTREE_TEMP, and /tmp/LayoutTests maps to LOCAL_RESOURCE_ROOT.
    // FIXME: This code should work on all *nix platforms and can be moved into TestRunner.cpp.
    std::string expectedRootName;
    std::string absolutePathToResourceRoot;
    std::string localResourceString = stringFromJSString(localResourceJSString);

    if (localResourceString.find("LayoutTests") != std::string::npos) {
        expectedRootName = "LayoutTests";
        absolutePathToResourceRoot = resourceRootAbsolutePath(m_testPathOrURL, expectedRootName);
    } else if (localResourceString.find("tmp") != std::string::npos) {
        expectedRootName = "tmp";
        absolutePathToResourceRoot = getenv("DUMPRENDERTREE_TEMP");
    } else {
        ASSERT_NOT_REACHED(); // pathToLocalResource was passed a path it doesn't know how to map.
    }
    ASSERT(!absolutePathToResourceRoot.empty());
    size_t indexOfSeparatorAfterRootName = indexOfSeparatorAfterDirectoryName(expectedRootName, localResourceString);
    std::string absolutePathToLocalResource = absolutePathToResourceRoot + localResourceString.substr(indexOfSeparatorAfterRootName);

    // Note: It's important that we keep the file:// or http tests will get confused.
    if (localResourceString.find("file://") != std::string::npos) {
        ASSERT(absolutePathToLocalResource[0] == '/');
        absolutePathToLocalResource = std::string("file://") + absolutePathToLocalResource;
    }
    return JSStringCreateWithUTF8CString(absolutePathToLocalResource.c_str());
}

void TestRunner::queueLoad(JSStringRef url, JSStringRef target)
{
    RetainPtr<CFStringRef> urlCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, url));
    NSString *urlNS = (NSString *)urlCF.get();

    NSURL *nsurl = [NSURL URLWithString:urlNS relativeToURL:[[[mainFrame dataSource] response] URL]];
    NSString *nsurlString = [nsurl absoluteString];

    JSRetainPtr<JSStringRef> absoluteURL(Adopt, JSStringCreateWithUTF8CString([nsurlString UTF8String]));
    WorkQueue::shared()->queue(new LoadItem(absoluteURL.get(), target));
}

void TestRunner::setAcceptsEditing(bool newAcceptsEditing)
{
    [(EditingDelegate *)[[mainFrame webView] editingDelegate] setAcceptsEditing:newAcceptsEditing];
}

void TestRunner::setAlwaysAcceptCookies(bool alwaysAcceptCookies)
{
    if (alwaysAcceptCookies == m_alwaysAcceptCookies)
        return;

    m_alwaysAcceptCookies = alwaysAcceptCookies;
    NSHTTPCookieAcceptPolicy cookieAcceptPolicy = alwaysAcceptCookies ? NSHTTPCookieAcceptPolicyAlways : NSHTTPCookieAcceptPolicyOnlyFromMainDocumentDomain;
    [WebPreferences _setCurrentNetworkLoaderSessionCookieAcceptPolicy:cookieAcceptPolicy];
}

void TestRunner::setAppCacheMaximumSize(unsigned long long size)
{
    [WebApplicationCache setMaximumSize:size];
}

void TestRunner::setApplicationCacheOriginQuota(unsigned long long quota)
{
    WebSecurityOrigin *origin = [[WebSecurityOrigin alloc] initWithURL:[NSURL URLWithString:@"http://127.0.0.1:8000"]];
    [[origin applicationCacheQuotaManager] setQuota:quota];
    [origin release];
}

void TestRunner::setAuthorAndUserStylesEnabled(bool flag)
{
    [[[mainFrame webView] preferences] setAuthorAndUserStylesEnabled:flag];
}

void TestRunner::setAutofilled(JSContextRef context, JSValueRef nodeObject, bool autofilled)
{
    DOMElement *element = [DOMElement _DOMElementFromJSContext:context value:nodeObject];
    if (!element || ![element isKindOfClass:[DOMHTMLInputElement class]])
        return;

    [(DOMHTMLInputElement *)element _setAutofilled:autofilled];
}

void TestRunner::setCustomPolicyDelegate(bool setDelegate, bool permissive)
{
    if (setDelegate) {
        [policyDelegate setPermissive:permissive];
        [[mainFrame webView] setPolicyDelegate:policyDelegate];
    } else
        [[mainFrame webView] setPolicyDelegate:nil];
}

void TestRunner::setDatabaseQuota(unsigned long long quota)
{    
    WebSecurityOrigin *origin = [[WebSecurityOrigin alloc] initWithURL:[NSURL URLWithString:@"file:///"]];
    [[origin databaseQuotaManager] setQuota:quota];
    [origin release];
}

void TestRunner::goBack()
{
    [[mainFrame webView] goBack];
}

void TestRunner::setDefersLoading(bool defers)
{
    [[mainFrame webView] setDefersCallbacks:defers];
}

void TestRunner::setDomainRelaxationForbiddenForURLScheme(bool forbidden, JSStringRef scheme)
{
    RetainPtr<CFStringRef> schemeCFString(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, scheme));
    [WebView _setDomainRelaxationForbidden:forbidden forURLScheme:(NSString *)schemeCFString.get()];
}

void TestRunner::setMockDeviceOrientation(bool canProvideAlpha, double alpha, bool canProvideBeta, double beta, bool canProvideGamma, double gamma)
{
    // DumpRenderTree configured the WebView to use WebDeviceOrientationProviderMock.
    id<WebDeviceOrientationProvider> provider = [[mainFrame webView] _deviceOrientationProvider];
    WebDeviceOrientationProviderMock *mockProvider = static_cast<WebDeviceOrientationProviderMock*>(provider);
    WebDeviceOrientation *orientation = [[WebDeviceOrientation alloc] initWithCanProvideAlpha:canProvideAlpha alpha:alpha canProvideBeta:canProvideBeta beta:beta canProvideGamma:canProvideGamma gamma:gamma];
    [mockProvider setOrientation:orientation];
    [orientation release];
}

void TestRunner::setMockGeolocationPosition(double latitude, double longitude, double accuracy)
{
    WebGeolocationPosition *position = [[WebGeolocationPosition alloc] initWithTimestamp:currentTime() latitude:latitude longitude:longitude accuracy:accuracy];
    [[MockGeolocationProvider shared] setPosition:position];
    [position release];
}

void TestRunner::setMockGeolocationError(int code, JSStringRef message)
{
    RetainPtr<CFStringRef> messageCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, message));
    NSString *messageNS = (NSString *)messageCF.get();
    NSError *error = [NSError errorWithDomain:WebKitErrorDomain code:code userInfo:[NSDictionary dictionaryWithObject:messageNS forKey:NSLocalizedDescriptionKey]];
    [[MockGeolocationProvider shared] setError:error];
}

void TestRunner::setGeolocationPermission(bool allow)
{
    setGeolocationPermissionCommon(allow);
    [[[mainFrame webView] UIDelegate] didSetMockGeolocationPermission];
}

void TestRunner::addMockSpeechInputResult(JSStringRef result, double confidence, JSStringRef language)
{
    // FIXME: Implement for speech input layout tests.
    // See https://bugs.webkit.org/show_bug.cgi?id=39485.
}

void TestRunner::setMockSpeechInputDumpRect(bool flag)
{
    // FIXME: Implement for speech input layout tests.
    // See https://bugs.webkit.org/show_bug.cgi?id=39485.
}

void TestRunner::startSpeechInput(JSContextRef inputElement)
{
    // FIXME: Implement for speech input layout tests.
    // See https://bugs.webkit.org/show_bug.cgi?id=39485.
}

void TestRunner::setIconDatabaseEnabled(bool iconDatabaseEnabled)
{
    // FIXME: Workaround <rdar://problem/6480108>
    static WebIconDatabase *sharedWebIconDatabase = NULL;
    if (!sharedWebIconDatabase) {
        if (!iconDatabaseEnabled)
            return;
        sharedWebIconDatabase = [WebIconDatabase sharedIconDatabase];
        if ([sharedWebIconDatabase isEnabled] == iconDatabaseEnabled)
            return;
    }
    [sharedWebIconDatabase setEnabled:iconDatabaseEnabled];
}

void TestRunner::setMainFrameIsFirstResponder(bool flag)
{
    NSView *documentView = [[mainFrame frameView] documentView];
    
    NSResponder *firstResponder = flag ? documentView : nil;
    [[[mainFrame webView] window] makeFirstResponder:firstResponder];
}

void TestRunner::setPrivateBrowsingEnabled(bool privateBrowsingEnabled)
{
    [[[mainFrame webView] preferences] setPrivateBrowsingEnabled:privateBrowsingEnabled];
}

void TestRunner::setXSSAuditorEnabled(bool enabled)
{
    [[[mainFrame webView] preferences] setXSSAuditorEnabled:enabled];
}

void TestRunner::setFrameFlatteningEnabled(bool enabled)
{
    [[[mainFrame webView] preferences] setFrameFlatteningEnabled:enabled];
}

void TestRunner::setSpatialNavigationEnabled(bool enabled)
{
    [[[mainFrame webView] preferences] setSpatialNavigationEnabled:enabled];
}

void TestRunner::setAllowUniversalAccessFromFileURLs(bool enabled)
{
    [[[mainFrame webView] preferences] setAllowUniversalAccessFromFileURLs:enabled];
}

void TestRunner::setAllowFileAccessFromFileURLs(bool enabled)
{
    [[[mainFrame webView] preferences] setAllowFileAccessFromFileURLs:enabled];
}

void TestRunner::setPopupBlockingEnabled(bool popupBlockingEnabled)
{
    [[[mainFrame webView] preferences] setJavaScriptCanOpenWindowsAutomatically:!popupBlockingEnabled];
}

void TestRunner::setPluginsEnabled(bool pluginsEnabled)
{
    [[[mainFrame webView] preferences] setPlugInsEnabled:pluginsEnabled];
}

void TestRunner::setJavaScriptCanAccessClipboard(bool enabled)
{
    [[[mainFrame webView] preferences] setJavaScriptCanAccessClipboard:enabled];
}

void TestRunner::setAutomaticLinkDetectionEnabled(bool enabled)
{
    [[mainFrame webView] setAutomaticLinkDetectionEnabled:enabled];
}

void TestRunner::setTabKeyCyclesThroughElements(bool cycles)
{
    [[mainFrame webView] setTabKeyCyclesThroughElements:cycles];
}

void TestRunner::setUseDashboardCompatibilityMode(bool flag)
{
    [[mainFrame webView] _setDashboardBehavior:WebDashboardBehaviorUseBackwardCompatibilityMode to:flag];
}

void TestRunner::setUserStyleSheetEnabled(bool flag)
{
    [[WebPreferences standardPreferences] setUserStyleSheetEnabled:flag];
}

void TestRunner::setUserStyleSheetLocation(JSStringRef path)
{
    RetainPtr<CFStringRef> pathCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, path));
    NSURL *url = [NSURL URLWithString:(NSString *)pathCF.get()];
    [[WebPreferences standardPreferences] setUserStyleSheetLocation:url];
}

void TestRunner::setValueForUser(JSContextRef context, JSValueRef nodeObject, JSStringRef value)
{
    DOMElement *element = [DOMElement _DOMElementFromJSContext:context value:nodeObject];
    if (!element || ![element isKindOfClass:[DOMHTMLInputElement class]])
        return;

    RetainPtr<CFStringRef> valueCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, value));
    [(DOMHTMLInputElement *)element _setValueForUser:(NSString *)valueCF.get()];
}

void TestRunner::setViewModeMediaFeature(JSStringRef mode)
{
    // FIXME: implement
}

void TestRunner::disableImageLoading()
{
    [[WebPreferences standardPreferences] setLoadsImagesAutomatically:NO];
}

void TestRunner::dispatchPendingLoadRequests()
{
    [[mainFrame webView] _dispatchPendingLoadRequests];
}

void TestRunner::overridePreference(JSStringRef key, JSStringRef value)
{
    RetainPtr<CFStringRef> keyCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, key));
    NSString *keyNS = (NSString *)keyCF.get();

    RetainPtr<CFStringRef> valueCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, value));
    NSString *valueNS = (NSString *)valueCF.get();

    [[WebPreferences standardPreferences] _setPreferenceForTestWithValue:valueNS forKey:keyNS];
}

void TestRunner::removeAllVisitedLinks()
{
    [WebHistory _removeAllVisitedLinks];
}

void TestRunner::setPersistentUserStyleSheetLocation(JSStringRef jsURL)
{
    RetainPtr<CFStringRef> urlString(AdoptCF, JSStringCopyCFString(0, jsURL));
    ::setPersistentUserStyleSheetLocation(urlString.get());
}

void TestRunner::clearPersistentUserStyleSheet()
{
    ::setPersistentUserStyleSheetLocation(0);
}

void TestRunner::setWindowIsKey(bool windowIsKey)
{
    m_windowIsKey = windowIsKey;
    [[mainFrame webView] _updateActiveState];
}

void TestRunner::setSmartInsertDeleteEnabled(bool flag)
{
    [[mainFrame webView] setSmartInsertDeleteEnabled:flag];
}

void TestRunner::setSelectTrailingWhitespaceEnabled(bool flag)
{
    [[mainFrame webView] setSelectTrailingWhitespaceEnabled:flag];
}

static const CFTimeInterval waitToDumpWatchdogInterval = 30.0;

static void waitUntilDoneWatchdogFired(CFRunLoopTimerRef timer, void* info)
{
    gTestRunner->waitToDumpWatchdogTimerFired();
}

void TestRunner::setWaitToDump(bool waitUntilDone)
{
    m_waitToDump = waitUntilDone;
    if (m_waitToDump && shouldSetWaitToDumpWatchdog())
        setWaitToDumpWatchdog(CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent() + waitToDumpWatchdogInterval, 0, 0, 0, waitUntilDoneWatchdogFired, NULL));
}

int TestRunner::windowCount()
{
    return CFArrayGetCount(openWindowsRef);
}

bool TestRunner::elementDoesAutoCompleteForElementWithId(JSStringRef jsString)
{
    RetainPtr<CFStringRef> idCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, jsString));
    NSString *idNS = (NSString *)idCF.get();
    
    DOMElement *element = [[mainFrame DOMDocument] getElementById:idNS];
    id rep = [[mainFrame dataSource] representation];
    
    if ([rep class] == [WebHTMLRepresentation class])
        return [(WebHTMLRepresentation *)rep elementDoesAutoComplete:element];

    return false;
}

void TestRunner::execCommand(JSStringRef name, JSStringRef value)
{
    RetainPtr<CFStringRef> nameCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, name));
    NSString *nameNS = (NSString *)nameCF.get();

    RetainPtr<CFStringRef> valueCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, value));
    NSString *valueNS = (NSString *)valueCF.get();

    [[mainFrame webView] _executeCoreCommandByName:nameNS value:valueNS];
}

bool TestRunner::findString(JSContextRef context, JSStringRef target, JSObjectRef optionsArray)
{
    WebFindOptions options = 0;

    JSRetainPtr<JSStringRef> lengthPropertyName(Adopt, JSStringCreateWithUTF8CString("length"));
    JSValueRef lengthValue = JSObjectGetProperty(context, optionsArray, lengthPropertyName.get(), 0);
    if (!JSValueIsNumber(context, lengthValue))
        return false;

    RetainPtr<CFStringRef> targetCFString(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, target));

    size_t length = static_cast<size_t>(JSValueToNumber(context, lengthValue, 0));
    for (size_t i = 0; i < length; ++i) {
        JSValueRef value = JSObjectGetPropertyAtIndex(context, optionsArray, i, 0);
        if (!JSValueIsString(context, value))
            continue;

        JSRetainPtr<JSStringRef> optionName(Adopt, JSValueToStringCopy(context, value, 0));

        if (JSStringIsEqualToUTF8CString(optionName.get(), "CaseInsensitive"))
            options |= WebFindOptionsCaseInsensitive;
        else if (JSStringIsEqualToUTF8CString(optionName.get(), "AtWordStarts"))
            options |= WebFindOptionsAtWordStarts;
        else if (JSStringIsEqualToUTF8CString(optionName.get(), "TreatMedialCapitalAsWordStart"))
            options |= WebFindOptionsTreatMedialCapitalAsWordStart;
        else if (JSStringIsEqualToUTF8CString(optionName.get(), "Backwards"))
            options |= WebFindOptionsBackwards;
        else if (JSStringIsEqualToUTF8CString(optionName.get(), "WrapAround"))
            options |= WebFindOptionsWrapAround;
        else if (JSStringIsEqualToUTF8CString(optionName.get(), "StartInSelection"))
            options |= WebFindOptionsStartInSelection;
    }

    return [[mainFrame webView] findString:(NSString *)targetCFString.get() options:options];
}

void TestRunner::setCacheModel(int cacheModel)
{
    [[WebPreferences standardPreferences] setCacheModel:cacheModel];
}

bool TestRunner::isCommandEnabled(JSStringRef name)
{
    RetainPtr<CFStringRef> nameCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, name));
    NSString *nameNS = (NSString *)nameCF.get();

    // Accept command strings with capital letters for first letter without trailing colon.
    if (![nameNS hasSuffix:@":"] && [nameNS length]) {
        nameNS = [[[[nameNS substringToIndex:1] lowercaseString]
            stringByAppendingString:[nameNS substringFromIndex:1]]
            stringByAppendingString:@":"];
    }

    SEL selector = NSSelectorFromString(nameNS);
    RetainPtr<CommandValidationTarget> target(AdoptNS, [[CommandValidationTarget alloc] initWithAction:selector]);
    id validator = [NSApp targetForAction:selector to:[mainFrame webView] from:target.get()];
    if (!validator)
        return false;
    if (![validator respondsToSelector:selector])
        return false;
    if (![validator respondsToSelector:@selector(validateUserInterfaceItem:)])
        return true;
    return [validator validateUserInterfaceItem:target.get()];
}

bool TestRunner::pauseAnimationAtTimeOnElementWithId(JSStringRef animationName, double time, JSStringRef elementId)
{
    RetainPtr<CFStringRef> idCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, elementId));
    NSString *idNS = (NSString *)idCF.get();
    RetainPtr<CFStringRef> nameCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, animationName));
    NSString *nameNS = (NSString *)nameCF.get();
    
    return [mainFrame _pauseAnimation:nameNS onNode:[[mainFrame DOMDocument] getElementById:idNS] atTime:time];
}

bool TestRunner::pauseTransitionAtTimeOnElementWithId(JSStringRef propertyName, double time, JSStringRef elementId)
{
    RetainPtr<CFStringRef> idCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, elementId));
    NSString *idNS = (NSString *)idCF.get();
    RetainPtr<CFStringRef> nameCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, propertyName));
    NSString *nameNS = (NSString *)nameCF.get();
    
    return [mainFrame _pauseTransitionOfProperty:nameNS onNode:[[mainFrame DOMDocument] getElementById:idNS] atTime:time];
}

unsigned TestRunner::numberOfActiveAnimations() const
{
    return [mainFrame _numberOfActiveAnimations];
}

void TestRunner::waitForPolicyDelegate()
{
    setWaitToDump(true);
    [policyDelegate setControllerToNotifyDone:this];
    [[mainFrame webView] setPolicyDelegate:policyDelegate];
}

void TestRunner::addOriginAccessWhitelistEntry(JSStringRef sourceOrigin, JSStringRef destinationProtocol, JSStringRef destinationHost, bool allowDestinationSubdomains)
{
    RetainPtr<CFStringRef> sourceOriginCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, sourceOrigin));
    NSString *sourceOriginNS = (NSString *)sourceOriginCF.get();
    RetainPtr<CFStringRef> protocolCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, destinationProtocol));
    NSString *destinationProtocolNS = (NSString *)protocolCF.get();
    RetainPtr<CFStringRef> hostCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, destinationHost));
    NSString *destinationHostNS = (NSString *)hostCF.get();
    [WebView _addOriginAccessWhitelistEntryWithSourceOrigin:sourceOriginNS destinationProtocol:destinationProtocolNS destinationHost:destinationHostNS allowDestinationSubdomains:allowDestinationSubdomains];
}

void TestRunner::removeOriginAccessWhitelistEntry(JSStringRef sourceOrigin, JSStringRef destinationProtocol, JSStringRef destinationHost, bool allowDestinationSubdomains)
{
    RetainPtr<CFStringRef> sourceOriginCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, sourceOrigin));
    NSString *sourceOriginNS = (NSString *)sourceOriginCF.get();
    RetainPtr<CFStringRef> protocolCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, destinationProtocol));
    NSString *destinationProtocolNS = (NSString *)protocolCF.get();
    RetainPtr<CFStringRef> hostCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, destinationHost));
    NSString *destinationHostNS = (NSString *)hostCF.get();
    [WebView _removeOriginAccessWhitelistEntryWithSourceOrigin:sourceOriginNS destinationProtocol:destinationProtocolNS destinationHost:destinationHostNS allowDestinationSubdomains:allowDestinationSubdomains];
}

void TestRunner::setScrollbarPolicy(JSStringRef orientation, JSStringRef policy)
{
    // FIXME: implement
}

void TestRunner::addUserScript(JSStringRef source, bool runAtStart, bool allFrames)
{
    RetainPtr<CFStringRef> sourceCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, source));
    NSString *sourceNS = (NSString *)sourceCF.get();
    [WebView _addUserScriptToGroup:@"org.webkit.DumpRenderTree" world:[WebScriptWorld world] source:sourceNS url:nil whitelist:nil blacklist:nil injectionTime:(runAtStart ? WebInjectAtDocumentStart : WebInjectAtDocumentEnd) injectedFrames:(allFrames ? WebInjectInAllFrames : WebInjectInTopFrameOnly)];
}

void TestRunner::addUserStyleSheet(JSStringRef source, bool allFrames)
{
    RetainPtr<CFStringRef> sourceCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, source));
    NSString *sourceNS = (NSString *)sourceCF.get();
    [WebView _addUserStyleSheetToGroup:@"org.webkit.DumpRenderTree" world:[WebScriptWorld world] source:sourceNS url:nil whitelist:nil blacklist:nil injectedFrames:(allFrames ? WebInjectInAllFrames : WebInjectInTopFrameOnly)];
}

void TestRunner::setDeveloperExtrasEnabled(bool enabled)
{
    [[[mainFrame webView] preferences] setDeveloperExtrasEnabled:enabled];
}

void TestRunner::setAsynchronousSpellCheckingEnabled(bool enabled)
{
    [[[mainFrame webView] preferences] setAsynchronousSpellCheckingEnabled:enabled];
}

void TestRunner::showWebInspector()
{
    [[[mainFrame webView] inspector] show:nil];
}

void TestRunner::closeWebInspector()
{
    [[[mainFrame webView] inspector] close:nil];
}

void TestRunner::evaluateInWebInspector(long callId, JSStringRef script)
{
    RetainPtr<CFStringRef> scriptCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, script));
    NSString *scriptNS = (NSString *)scriptCF.get();
    [[[mainFrame webView] inspector] evaluateInFrontend:nil callId:callId script:scriptNS];
}

typedef HashMap<unsigned, RetainPtr<WebScriptWorld> > WorldMap;
static WorldMap& worldMap()
{
    static WorldMap& map = *new WorldMap;
    return map;
}

unsigned worldIDForWorld(WebScriptWorld *world)
{
    WorldMap::const_iterator end = worldMap().end();
    for (WorldMap::const_iterator it = worldMap().begin(); it != end; ++it) {
        if (it->second == world)
            return it->first;
    }

    return 0;
}

void TestRunner::evaluateScriptInIsolatedWorldAndReturnValue(unsigned worldID, JSObjectRef globalObject, JSStringRef script)
{
    // FIXME: Implement this.
}

void TestRunner::evaluateScriptInIsolatedWorld(unsigned worldID, JSObjectRef globalObject, JSStringRef script)
{
    RetainPtr<CFStringRef> scriptCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, script));
    NSString *scriptNS = (NSString *)scriptCF.get();

    // A worldID of 0 always corresponds to a new world. Any other worldID corresponds to a world
    // that is created once and cached forever.
    WebScriptWorld *world;
    if (!worldID)
        world = [WebScriptWorld world];
    else {
        RetainPtr<WebScriptWorld>& worldSlot = worldMap().add(worldID, 0).iterator->second;
        if (!worldSlot)
            worldSlot.adoptNS([[WebScriptWorld alloc] init]);
        world = worldSlot.get();
    }

    [mainFrame _stringByEvaluatingJavaScriptFromString:scriptNS withGlobalObject:globalObject inScriptWorld:world];
}

@interface APITestDelegate : NSObject
{
    bool* m_condition;
}
@end

@implementation APITestDelegate

- (id)initWithCompletionCondition:(bool*)condition
{
    [super init];
    ASSERT(condition);
    m_condition = condition;
    *m_condition = false;
    return self;
}

- (void)webView:(WebView *)sender didFailLoadWithError:(NSError *)error forFrame:(WebFrame *)frame
{
    printf("API Test load failed\n");
    *m_condition = true;
}

- (void)webView:(WebView *)sender didFailProvisionalLoadWithError:(NSError *)error forFrame:(WebFrame *)frame
{
    printf("API Test load failed provisional\n");
    *m_condition = true;
}

- (void)webView:(WebView *)sender didFinishLoadForFrame:(WebFrame *)frame
{
    printf("API Test load succeeded\n");
    *m_condition = true;
}

@end

void TestRunner::apiTestNewWindowDataLoadBaseURL(JSStringRef utf8Data, JSStringRef baseURL)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    RetainPtr<CFStringRef> utf8DataCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, utf8Data));
    RetainPtr<CFStringRef> baseURLCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, baseURL));
    
    WebView *webView = [[WebView alloc] initWithFrame:NSZeroRect frameName:@"" groupName:@""];

    bool done = false;
    APITestDelegate *delegate = [[APITestDelegate alloc] initWithCompletionCondition:&done];
    [webView setFrameLoadDelegate:delegate];

    [[webView mainFrame] loadData:[(NSString *)utf8DataCF.get() dataUsingEncoding:NSUTF8StringEncoding] MIMEType:@"text/html" textEncodingName:@"utf-8" baseURL:[NSURL URLWithString:(NSString *)baseURLCF.get()]];
    
    while (!done) {
        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantPast]];
        [pool release];
    }
        
    [webView close];
    [webView release];
    [delegate release];
    [pool release];
}

void TestRunner::apiTestGoToCurrentBackForwardItem()
{
    WebView *view = [mainFrame webView];
    [view goToBackForwardItem:[[view backForwardList] currentItem]];
}

void TestRunner::setWebViewEditable(bool editable)
{
    WebView *view = [mainFrame webView];
    [view setEditable:editable];
}

static NSString *SynchronousLoaderRunLoopMode = @"DumpRenderTreeSynchronousLoaderRunLoopMode";

#if __MAC_OS_X_VERSION_MIN_REQUIRED <= 1060
@protocol NSURLConnectionDelegate <NSObject>
@end
#endif

@interface SynchronousLoader : NSObject <NSURLConnectionDelegate>
{
    NSString *m_username;
    NSString *m_password;
    BOOL m_isDone;
}
+ (void)makeRequest:(NSURLRequest *)request withUsername:(NSString *)username password:(NSString *)password;
@end

@implementation SynchronousLoader : NSObject
- (void)dealloc
{
    [m_username release];
    [m_password release];

    [super dealloc];
}

- (BOOL)connectionShouldUseCredentialStorage:(NSURLConnection *)connection
{
    return YES;
}

- (void)connection:(NSURLConnection *)connection didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge
{
    if ([challenge previousFailureCount] == 0) {
        RetainPtr<NSURLCredential> credential(AdoptNS, [[NSURLCredential alloc]  initWithUser:m_username password:m_password persistence:NSURLCredentialPersistenceForSession]);
        [[challenge sender] useCredential:credential.get() forAuthenticationChallenge:challenge];
        return;
    }
    [[challenge sender] cancelAuthenticationChallenge:challenge];
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
    printf("SynchronousLoader failed: %s\n", [[error description] UTF8String]);
    m_isDone = YES;
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection
{
    m_isDone = YES;
}

+ (void)makeRequest:(NSURLRequest *)request withUsername:(NSString *)username password:(NSString *)password
{
    ASSERT(![[request URL] user]);
    ASSERT(![[request URL] password]);

    SynchronousLoader *delegate = [[SynchronousLoader alloc] init];
    delegate->m_username = [username copy];
    delegate->m_password = [password copy];

    NSURLConnection *connection = [[NSURLConnection alloc] initWithRequest:request delegate:delegate startImmediately:NO];
    [connection scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:SynchronousLoaderRunLoopMode];
    [connection start];
    
    while (!delegate->m_isDone)
        [[NSRunLoop currentRunLoop] runMode:SynchronousLoaderRunLoopMode beforeDate:[NSDate distantFuture]];

    [connection cancel];
    
    [connection release];
    [delegate release];
}

@end

void TestRunner::authenticateSession(JSStringRef url, JSStringRef username, JSStringRef password)
{
    // See <rdar://problem/7880699>.
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1060
    RetainPtr<CFStringRef> urlStringCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, url));
    RetainPtr<CFStringRef> usernameCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, username));
    RetainPtr<CFStringRef> passwordCF(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, password));

    RetainPtr<NSURLRequest> request(AdoptNS, [[NSURLRequest alloc] initWithURL:[NSURL URLWithString:(NSString *)urlStringCF.get()]]);

    [SynchronousLoader makeRequest:request.get() withUsername:(NSString *)usernameCF.get() password:(NSString *)passwordCF.get()];
#endif
}

void TestRunner::abortModal()
{
    [NSApp abortModal];
}

void TestRunner::setSerializeHTTPLoads(bool serialize)
{
    [WebView _setLoadResourcesSerially:serialize];
}

void TestRunner::setMinimumTimerInterval(double minimumTimerInterval)
{
    [[mainFrame webView] _setMinimumTimerInterval:minimumTimerInterval];
}

void TestRunner::setTextDirection(JSStringRef directionName)
{
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1060
    if (JSStringIsEqualToUTF8CString(directionName, "ltr"))
        [[mainFrame webView] makeBaseWritingDirectionLeftToRight:0];
    else if (JSStringIsEqualToUTF8CString(directionName, "rtl"))
        [[mainFrame webView] makeBaseWritingDirectionRightToLeft:0];
    else
        ASSERT_NOT_REACHED();
#endif
}

void TestRunner::addChromeInputField()
{
    NSTextField *textField = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 100, 20)];
    textField.tag = 1;
    [[[[mainFrame webView] window] contentView] addSubview:textField];
    [textField release];
    
    [textField setNextKeyView:[mainFrame webView]];
    [[mainFrame webView] setNextKeyView:textField];
}

void TestRunner::removeChromeInputField()
{
    NSView* textField = [[[[mainFrame webView] window] contentView] viewWithTag:1];
    if (textField) {
        [textField removeFromSuperview];
        focusWebView();
    }
}

void TestRunner::focusWebView()
{
    [[[mainFrame webView] window] makeFirstResponder:[mainFrame webView]];
}

void TestRunner::setBackingScaleFactor(double backingScaleFactor)
{
    [[mainFrame webView] _setCustomBackingScaleFactor:backingScaleFactor];
}

void TestRunner::resetPageVisibility()
{
    // FIXME: Implement.
}

void TestRunner::setPageVisibility(const char*)
{
    // FIXME: Implement.
}

void TestRunner::sendWebIntentResponse(JSStringRef)
{
    // FIXME: Implement.
}

void TestRunner::deliverWebIntent(JSStringRef, JSStringRef, JSStringRef)
{
    // FIXME: Implement.
}

void TestRunner::grantWebNotificationPermission(JSStringRef jsOrigin)
{
    RetainPtr<CFStringRef> cfOrigin(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, jsOrigin));
    ASSERT([[mainFrame webView] _notificationProvider] == [MockWebNotificationProvider shared]);
    [[MockWebNotificationProvider shared] setWebNotificationOrigin:(NSString *)cfOrigin.get() permission:TRUE];
}

void TestRunner::denyWebNotificationPermission(JSStringRef jsOrigin)
{
    RetainPtr<CFStringRef> cfOrigin(AdoptCF, JSStringCopyCFString(kCFAllocatorDefault, jsOrigin));
    ASSERT([[mainFrame webView] _notificationProvider] == [MockWebNotificationProvider shared]);
    [[MockWebNotificationProvider shared] setWebNotificationOrigin:(NSString *)cfOrigin.get() permission:FALSE];
}

void TestRunner::removeAllWebNotificationPermissions()
{
    [[MockWebNotificationProvider shared] removeAllWebNotificationPermissions];
}

void TestRunner::simulateWebNotificationClick(JSValueRef jsNotification)
{
    uint64_t notificationID = [[mainFrame webView] _notificationIDForTesting:jsNotification];
    m_hasPendingWebNotificationClick = true;
    dispatch_async(dispatch_get_main_queue(), ^{
        if (!m_hasPendingWebNotificationClick)
            return;

        [[MockWebNotificationProvider shared] simulateWebNotificationClick:notificationID];
        m_hasPendingWebNotificationClick = false;
    });
}

void TestRunner::simulateLegacyWebNotificationClick(JSStringRef jsTitle)
{
}

