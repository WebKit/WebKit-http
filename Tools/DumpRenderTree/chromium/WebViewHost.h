/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef WebViewHost_h
#define WebViewHost_h

#include "MockSpellCheck.h"
#include "Task.h"
#include "TestDelegate.h"
#include "TestNavigationController.h"
#include "WebAccessibilityNotification.h"
#include "WebCursorInfo.h"
#include "WebFrameClient.h"
#include "WebIntentRequest.h"
#include "WebPrerendererClient.h"
#include "WebSpellCheckClient.h"
#include "WebViewClient.h"
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

class DRTTestRunner;
class MockWebSpeechInputController;
class MockWebSpeechRecognizer;
class SkCanvas;
class TestShell;

namespace WebKit {
class WebFrame;
class WebDeviceOrientationClient;
class WebDeviceOrientationClientMock;
class WebGeolocationClient;
class WebGeolocationClientMock;
class WebGeolocationServiceMock;
class WebIntentServiceInfo;
class WebSerializedScriptValue;
class WebSharedWorkerClient;
class WebSpeechInputController;
class WebSpeechInputListener;
class WebURL;
class WebUserMediaClientMock;
struct WebRect;
struct WebURLError;
struct WebWindowFeatures;
}

namespace webkit_support {
class MediaStreamUtil;
class TestMediaStreamClient;
}

class WebViewHost : public WebKit::WebViewClient, public WebKit::WebFrameClient, public NavigationHost,
                    public WebKit::WebPrerendererClient, public WebKit::WebSpellCheckClient,
                    public TestDelegate {
 public:
    WebViewHost(TestShell*);
    virtual ~WebViewHost();
    void setWebWidget(WebKit::WebWidget*);
    WebKit::WebView* webView() const;
    WebKit::WebWidget* webWidget() const;
    void reset();
    void setSelectTrailingWhitespaceEnabled(bool);
    void setSmartInsertDeleteEnabled(bool);
    void setLogConsoleOutput(bool);
    void waitForPolicyDelegate();
    void setCustomPolicyDelegate(bool, bool);
    WebKit::WebFrame* topLoadingFrame() { return m_topLoadingFrame; }
    void setBlockRedirects(bool block) { m_blocksRedirects = block; }
    void setRequestReturnNull(bool returnNull) { m_requestReturnNull = returnNull; }
    virtual void setEditCommand(const std::string& name, const std::string& value) OVERRIDE;
    virtual void clearEditCommand() OVERRIDE;
    void setPendingExtraData(PassOwnPtr<TestShellExtraData>);
    void setDeviceScaleFactor(float);

    virtual void setGamepadData(const WebKit::WebGamepads&);

    void paintRect(const WebKit::WebRect&);
    void updatePaintRect(const WebKit::WebRect&);
    void paintInvalidatedRegion();
    void paintPagesWithBoundaries();
    SkCanvas* canvas();
    void displayRepaintMask();

    void loadURLForFrame(const WebKit::WebURL&, const WebKit::WebString& frameName);
    TestNavigationController* navigationController() { return m_navigationController.get(); }

    void addClearHeader(const WTF::String& header) { m_clearHeaders.add(header); }
    const HashSet<WTF::String>& clearHeaders() const { return m_clearHeaders; }
    void closeWidget();

    virtual WebKit::WebContextMenuData* lastContextMenuData() const OVERRIDE;
    virtual void clearContextMenuData() OVERRIDE;

#if ENABLE(INPUT_SPEECH)
    MockWebSpeechInputController* speechInputControllerMock() { return m_speechInputControllerMock.get(); }
#endif

#if ENABLE(SCRIPTED_SPEECH)
    MockWebSpeechRecognizer* mockSpeechRecognizer() { return m_mockSpeechRecognizer.get(); }
#endif

#if ENABLE(POINTER_LOCK)
    void didAcquirePointerLock();
    void didNotAcquirePointerLock();
    void didLosePointerLock();
    void setPointerLockWillRespondAsynchronously() { m_pointerLockPlannedResult = PointerLockWillRespondAsync; }
    void setPointerLockWillFailSynchronously() { m_pointerLockPlannedResult = PointerLockWillFailSync; }
#endif

    // NavigationHost
    virtual bool navigate(const TestNavigationEntry&, bool reload);

    // WebKit::WebPrerendererClient
    virtual void willAddPrerender(WebKit::WebPrerender*) OVERRIDE;

    // WebKit::WebSpellCheckClient
    virtual void spellCheck(const WebKit::WebString&, int& offset, int& length, WebKit::WebVector<WebKit::WebString>* optionalSuggestions);
    virtual void checkTextOfParagraph(const WebKit::WebString&, WebKit::WebTextCheckingTypeMask, WebKit::WebVector<WebKit::WebTextCheckingResult>*);
    virtual void requestCheckingOfText(const WebKit::WebString&, WebKit::WebTextCheckingCompletion*);
    virtual WebKit::WebString autoCorrectWord(const WebKit::WebString&);

    // WebKit::WebViewClient
    virtual WebKit::WebView* createView(WebKit::WebFrame*, const WebKit::WebURLRequest&, const WebKit::WebWindowFeatures&, const WebKit::WebString&, WebKit::WebNavigationPolicy);
    virtual WebKit::WebWidget* createPopupMenu(WebKit::WebPopupType);
    virtual WebKit::WebWidget* createPopupMenu(const WebKit::WebPopupMenuInfo&);
    virtual WebKit::WebStorageNamespace* createSessionStorageNamespace(unsigned quota);
    virtual WebKit::WebCompositorOutputSurface* createOutputSurface();
    virtual void didAddMessageToConsole(const WebKit::WebConsoleMessage&, const WebKit::WebString& sourceName, unsigned sourceLine);
    virtual void didStartLoading();
    virtual void didStopLoading();
    virtual bool shouldBeginEditing(const WebKit::WebRange&);
    virtual bool shouldEndEditing(const WebKit::WebRange&);
    virtual bool shouldInsertNode(const WebKit::WebNode&, const WebKit::WebRange&, WebKit::WebEditingAction);
    virtual bool shouldInsertText(const WebKit::WebString&, const WebKit::WebRange&, WebKit::WebEditingAction);
    virtual bool shouldChangeSelectedRange(const WebKit::WebRange& from, const WebKit::WebRange& to, WebKit::WebTextAffinity, bool stillSelecting);
    virtual bool shouldDeleteRange(const WebKit::WebRange&);
    virtual bool shouldApplyStyle(const WebKit::WebString& style, const WebKit::WebRange&);
    virtual bool isSmartInsertDeleteEnabled();
    virtual bool isSelectTrailingWhitespaceEnabled();
    virtual void didBeginEditing();
    virtual void didChangeSelection(bool isSelectionEmpty);
    virtual void didChangeContents();
    virtual void didEndEditing();
    virtual bool handleCurrentKeyboardEvent();
    virtual void runModalAlertDialog(WebKit::WebFrame*, const WebKit::WebString&);
    virtual bool runModalConfirmDialog(WebKit::WebFrame*, const WebKit::WebString&);
    virtual bool runModalPromptDialog(WebKit::WebFrame*, const WebKit::WebString& message, const WebKit::WebString& defaultValue, WebKit::WebString* actualValue);
    virtual bool runModalBeforeUnloadDialog(WebKit::WebFrame*, const WebKit::WebString&);
    virtual void showContextMenu(WebKit::WebFrame*, const WebKit::WebContextMenuData&);
    virtual void setStatusText(const WebKit::WebString&);
    virtual void startDragging(WebKit::WebFrame*, const WebKit::WebDragData&, WebKit::WebDragOperationsMask, const WebKit::WebImage&, const WebKit::WebPoint&);
    virtual void didUpdateLayout();
    virtual void navigateBackForwardSoon(int offset);
    virtual int historyBackListCount();
    virtual int historyForwardListCount();
    virtual void postAccessibilityNotification(const WebKit::WebAccessibilityObject&, WebKit::WebAccessibilityNotification);
#if ENABLE(NOTIFICATIONS)
    virtual WebKit::WebNotificationPresenter* notificationPresenter();
#endif
    virtual WebKit::WebGeolocationClient* geolocationClient();
#if ENABLE(INPUT_SPEECH)
    virtual WebKit::WebSpeechInputController* speechInputController(WebKit::WebSpeechInputListener*);
#endif
#if ENABLE(SCRIPTED_SPEECH)
    virtual WebKit::WebSpeechRecognizer* speechRecognizer() OVERRIDE;
#endif
    virtual WebKit::WebDeviceOrientationClient* deviceOrientationClient() OVERRIDE;
#if ENABLE(MEDIA_STREAM)
    virtual WebKit::WebUserMediaClient* userMediaClient();
#endif
    virtual void printPage(WebKit::WebFrame*);

    // WebKit::WebWidgetClient
    virtual void didInvalidateRect(const WebKit::WebRect&);
    virtual void didScrollRect(int dx, int dy, const WebKit::WebRect&);
    virtual void didAutoResize(const WebKit::WebSize& newSize);
    virtual void scheduleComposite();
#if ENABLE(REQUEST_ANIMATION_FRAME)
    virtual void serviceAnimation();
    virtual void scheduleAnimation();
#endif
    virtual void didFocus();
    virtual void didBlur();
    virtual void didChangeCursor(const WebKit::WebCursorInfo&);
    virtual void closeWidgetSoon();
    virtual void show(WebKit::WebNavigationPolicy);
    virtual void runModal();
    virtual bool enterFullScreen();
    virtual void exitFullScreen();
    virtual WebKit::WebRect windowRect();
    virtual void setWindowRect(const WebKit::WebRect&);
    virtual WebKit::WebRect rootWindowRect();
    virtual WebKit::WebRect windowResizerRect();
    virtual WebKit::WebScreenInfo screenInfo();
#if ENABLE(POINTER_LOCK)
    virtual bool requestPointerLock();
    virtual void requestPointerUnlock();
    virtual bool isPointerLocked();
#endif

    // WebKit::WebFrameClient
    virtual WebKit::WebPlugin* createPlugin(WebKit::WebFrame*, const WebKit::WebPluginParams&);
    virtual WebKit::WebMediaPlayer* createMediaPlayer(WebKit::WebFrame*, const WebKit::WebURL&, WebKit::WebMediaPlayerClient*);
    virtual WebKit::WebApplicationCacheHost* createApplicationCacheHost(WebKit::WebFrame*, WebKit::WebApplicationCacheHostClient*);
    virtual void loadURLExternally(WebKit::WebFrame*, const WebKit::WebURLRequest&, WebKit::WebNavigationPolicy);
    virtual void loadURLExternally(WebKit::WebFrame*, const WebKit::WebURLRequest&, WebKit::WebNavigationPolicy, const WebKit::WebString& downloadName);
    virtual WebKit::WebNavigationPolicy decidePolicyForNavigation(
        WebKit::WebFrame*, const WebKit::WebURLRequest&,
        WebKit::WebNavigationType, const WebKit::WebNode&,
        WebKit::WebNavigationPolicy, bool isRedirect);
    virtual bool canHandleRequest(WebKit::WebFrame*, const WebKit::WebURLRequest&);
    virtual WebKit::WebURLError cannotHandleRequestError(WebKit::WebFrame*, const WebKit::WebURLRequest&);
    virtual WebKit::WebURLError cancelledError(WebKit::WebFrame*, const WebKit::WebURLRequest&);
    virtual void unableToImplementPolicyWithError(WebKit::WebFrame*, const WebKit::WebURLError&);
    virtual void willPerformClientRedirect(
        WebKit::WebFrame*, const WebKit::WebURL& from, const WebKit::WebURL& to,
        double interval, double fireTime);
    virtual void didCancelClientRedirect(WebKit::WebFrame*);
    virtual void didCreateDataSource(WebKit::WebFrame*, WebKit::WebDataSource*);
    virtual void didStartProvisionalLoad(WebKit::WebFrame*);
    virtual void didReceiveServerRedirectForProvisionalLoad(WebKit::WebFrame*);
    virtual void didFailProvisionalLoad(WebKit::WebFrame*, const WebKit::WebURLError&);
    virtual void didCommitProvisionalLoad(WebKit::WebFrame*, bool isNewNavigation);
    virtual void didClearWindowObject(WebKit::WebFrame*);
    virtual void didReceiveTitle(WebKit::WebFrame*, const WebKit::WebString&, WebKit::WebTextDirection);
    virtual void didFinishDocumentLoad(WebKit::WebFrame*);
    virtual void didHandleOnloadEvents(WebKit::WebFrame*);
    virtual void didFailLoad(WebKit::WebFrame*, const WebKit::WebURLError&);
    virtual void didFinishLoad(WebKit::WebFrame*);
    virtual void didNavigateWithinPage(WebKit::WebFrame*, bool isNewNavigation);
    virtual void didChangeLocationWithinPage(WebKit::WebFrame*);
    virtual void assignIdentifierToRequest(WebKit::WebFrame*, unsigned identifier, const WebKit::WebURLRequest&);
    virtual void removeIdentifierForRequest(unsigned identifier);
    virtual void willSendRequest(WebKit::WebFrame*, unsigned identifier, WebKit::WebURLRequest&, const WebKit::WebURLResponse&);
    virtual void didReceiveResponse(WebKit::WebFrame*, unsigned identifier, const WebKit::WebURLResponse&);
    virtual void didFinishResourceLoad(WebKit::WebFrame*, unsigned identifier);
    virtual void didFailResourceLoad(WebKit::WebFrame*, unsigned identifier, const WebKit::WebURLError&);
    virtual void didDisplayInsecureContent(WebKit::WebFrame*);
    virtual void didRunInsecureContent(WebKit::WebFrame*, const WebKit::WebSecurityOrigin&, const WebKit::WebURL&);
    virtual void didDetectXSS(WebKit::WebFrame*, const WebKit::WebURL&, bool didBlockEntirePage);
    virtual void openFileSystem(WebKit::WebFrame*, WebKit::WebFileSystem::Type, long long size, bool create, WebKit::WebFileSystemCallbacks*);
    virtual void deleteFileSystem(WebKit::WebFrame*, WebKit::WebFileSystem::Type, WebKit::WebFileSystemCallbacks*);
    virtual bool willCheckAndDispatchMessageEvent(WebKit::WebFrame* source, WebKit::WebSecurityOrigin target, WebKit::WebDOMMessageEvent);
    virtual void registerIntentService(WebKit::WebFrame*, const WebKit::WebIntentServiceInfo&);
    virtual void dispatchIntent(WebKit::WebFrame*, const WebKit::WebIntentRequest&);
    virtual void deliveredIntentResult(WebKit::WebFrame*, int, const WebKit::WebSerializedScriptValue&);
    virtual void deliveredIntentFailure(WebKit::WebFrame*, int, const WebKit::WebSerializedScriptValue&);

    WebKit::WebDeviceOrientationClientMock* deviceOrientationClientMock();
    
    // Spellcheck related helper APIs
    MockSpellCheck* mockSpellCheck();
    void finishLastTextCheck();
    virtual void fillSpellingSuggestionList(const WebKit::WebString& word, Vector<WebKit::WebString>* suggestions) OVERRIDE;

    // Geolocation client mocks for DRTTestRunner
    WebKit::WebGeolocationClientMock* geolocationClientMock();

    // Pending task list, Note taht the method is referred from MethodTask class.
    TaskList* taskList() { return &m_taskList; }

    // The current web intents request.
    WebKit::WebIntentRequest* currentIntentRequest() { return &m_currentRequest; }

private:

    class HostMethodTask : public MethodTask<WebViewHost> {
    public:
        typedef void (WebViewHost::*CallbackMethodType)();
        HostMethodTask(WebViewHost* object, CallbackMethodType callback)
            : MethodTask<WebViewHost>(object)
            , m_callback(callback)
        { }

        virtual void runIfValid() { (m_object->*m_callback)(); }

    private:
        CallbackMethodType m_callback;
    };

    DRTTestRunner* testRunner() const;

    // Called the title of the page changes.
    // Can be used to update the title of the window.
    void setPageTitle(const WebKit::WebString&);

    // Called when the URL of the page changes.
    // Extracts the URL and forwards on to SetAddressBarURL().
    void updateAddressBar(WebKit::WebView*);

    // Called when the URL of the page changes.
    // Should be used to update the text of the URL bar.
    void setAddressBarURL(const WebKit::WebURL&);

    void enterFullScreenNow();
    void exitFullScreenNow();

    // In the Mac code, this is called to trigger the end of a test after the
    // page has finished loading. From here, we can generate the dump for the
    // test.
    void locationChangeDone(WebKit::WebFrame*);

    void updateForCommittedLoad(WebKit::WebFrame*, bool isNewNavigation);
    void updateURL(WebKit::WebFrame*);
    void updateSessionHistory(WebKit::WebFrame*);

    // Dumping a frame to the console.
    void printFrameDescription(WebKit::WebFrame*);

    // Dumping the user gesture status to the console.
    void printFrameUserGestureStatus(WebKit::WebFrame*, const char*);

    bool hasWindow() const { return m_hasWindow; }
    void resetScrollRect();
    void discardBackingStore();

#if ENABLE(MEDIA_STREAM)
    WebKit::WebUserMediaClientMock* userMediaClientMock();
    webkit_support::TestMediaStreamClient* testMediaStreamClient();
#endif

    // Causes navigation actions just printout the intended navigation instead
    // of taking you to the page. This is used for cases like mailto, where you
    // don't actually want to open the mail program.
    bool m_policyDelegateEnabled;

    // Toggles the behavior of the policy delegate. If true, then navigations
    // will be allowed. Otherwise, they will be ignored (dropped).
    bool m_policyDelegateIsPermissive;

    // If true, the policy delegate will signal layout test completion.
    bool m_policyDelegateShouldNotifyDone;

    // Non-owning pointer. The WebViewHost instance is owned by this TestShell instance.
    TestShell* m_shell;

    // This delegate works for the following widget.
    WebKit::WebWidget* m_webWidget;

    // This is non-0 IFF a load is in progress.
    WebKit::WebFrame* m_topLoadingFrame;

    // For tracking session history. See RenderView.
    int m_pageId;
    int m_lastPageIdUpdated;

    OwnPtr<TestShellExtraData> m_pendingExtraData;

    // Maps resource identifiers to a descriptive string.
    typedef HashMap<unsigned, std::string> ResourceMap;
    ResourceMap m_resourceIdentifierMap;
    void printResourceDescription(unsigned identifier);

    WebKit::WebCursorInfo m_currentCursor;

    bool m_hasWindow;
    bool m_inModalLoop;
    WebKit::WebRect m_windowRect;

    // true if we want to enable smart insert/delete.
    bool m_smartInsertDeleteEnabled;

    // true if we want to enable selection of trailing whitespaces
    bool m_selectTrailingWhitespaceEnabled;

    // true if whatever is sent to the console should be logged to stdout.
    bool m_logConsoleOutput;

    // Set of headers to clear in willSendRequest.
    HashSet<WTF::String> m_clearHeaders;

    // true if we should block any redirects
    bool m_blocksRedirects;

    // true if we should block (set an empty request for) any requests
    bool m_requestReturnNull;

    // Edit command associated to the current keyboard event.
    std::string m_editCommandName;
    std::string m_editCommandValue;

    // The mock spellchecker used in spellCheck().
    MockSpellCheck m_spellcheck;

    // Painting.
    OwnPtr<SkCanvas> m_canvas;
    WebKit::WebRect m_paintRect;
    bool m_isPainting;

    OwnPtr<WebKit::WebContextMenuData> m_lastContextMenuData;

    // Geolocation
    OwnPtr<WebKit::WebGeolocationClientMock> m_geolocationClientMock;

    OwnPtr<WebKit::WebDeviceOrientationClientMock> m_deviceOrientationClientMock;
#if ENABLE(INPUT_SPEECH)
    OwnPtr<MockWebSpeechInputController> m_speechInputControllerMock;
#endif

#if ENABLE(SCRIPTED_SPEECH)
    OwnPtr<MockWebSpeechRecognizer> m_mockSpeechRecognizer;
#endif

#if ENABLE(MEDIA_STREAM)
    OwnPtr<WebKit::WebUserMediaClientMock> m_userMediaClientMock;
    OwnPtr<webkit_support::TestMediaStreamClient> m_testMediaStreamClient;
#endif

    OwnPtr<TestNavigationController> m_navigationController;

    WebKit::WebString m_lastRequestedTextCheckString;
    WebKit::WebTextCheckingCompletion* m_lastRequestedTextCheckingCompletion;

    TaskList m_taskList;
    Vector<WebKit::WebWidget*> m_popupmenus;

#if ENABLE(POINTER_LOCK)
    bool m_pointerLocked;
    enum {
        PointerLockWillSucceed,
        PointerLockWillRespondAsync,
        PointerLockWillFailSync
    } m_pointerLockPlannedResult;
#endif

    // For web intents: holds the current request, if any.
    WebKit::WebIntentRequest m_currentRequest;
};

#endif // WebViewHost_h
