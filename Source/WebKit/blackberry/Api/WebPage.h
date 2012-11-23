/*
 * Copyright (C) 2009, 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
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

#ifndef WebPage_h
#define WebPage_h

#include "BlackBerryGlobal.h"
#include "JavaScriptVariant.h"

#include <BlackBerryPlatformGuardedPointer.h>
#include <BlackBerryPlatformInputEvents.h>
#include <BlackBerryPlatformString.h>
#include <BlackBerryPlatformWebContext.h>
#include <imf/input_data.h>
#include <network/NetworkRequest.h>
#include <string>
#include <vector>

struct OpaqueJSContext;
typedef const struct OpaqueJSContext* JSContextRef;

struct OpaqueJSValue;
typedef const struct OpaqueJSValue* JSValueRef;

namespace WebCore {
class ChromeClientBlackBerry;
class Frame;
class FrameLoaderClientBlackBerry;
class PagePopupBlackBerry;
}

class WebDOMDocument;
class WebDOMNode;
template<typename T> class SharedArray;

namespace BlackBerry {
namespace Platform {
class FloatPoint;
class IntPoint;
class IntRect;
class IntSize;
class KeyboardEvent;
class MouseEvent;
class TouchEvent;
class TouchPoint;
class ViewportAccessor;
}

namespace WebKit {

class BackingStore;
class BackingStoreClient;
class BackingStorePrivate;
class InRegionScroller;
class RenderQueue;
class WebCookieJar;
class WebOverlay;
class WebPageClient;
class WebPageCompositor;
class WebPageGroupLoadDeferrer;
class WebPagePrivate;
class WebSettings;
class WebTapHighlight;
class WebViewportArguments;

enum JavaScriptDataType { JSUndefined = 0, JSNull, JSBoolean, JSNumber, JSString, JSObject, JSException, JSDataTypeMax };

enum ActivationStateType { ActivationActive, ActivationInactive, ActivationStandby };

enum TargetDetectionStrategy {PointBased, RectBased, FocusBased};

class BLACKBERRY_EXPORT WebPage : public Platform::GuardedPointerBase {
public:
    WebPage(WebPageClient*, const BlackBerry::Platform::String& pageGroupName, const Platform::IntRect&);
    void destroy();

    WebPageClient* client() const;

    void load(const BlackBerry::Platform::String& url, const BlackBerry::Platform::String& networkToken, bool isInitial = false);

    void loadExtended(const char* url, const char* networkToken, const char* method, Platform::NetworkRequest::CachePolicy = Platform::NetworkRequest::UseProtocolCachePolicy, const char* data = 0, size_t dataLength = 0, const char* const* headers = 0, size_t headersLength = 0, bool mustHandleInternally = false);

    void loadFile(const BlackBerry::Platform::String& path, const BlackBerry::Platform::String& overrideContentType = "");

    void loadString(const BlackBerry::Platform::String&, const BlackBerry::Platform::String& baseURL, const BlackBerry::Platform::String& contentType = "text/html", const BlackBerry::Platform::String& failingURL = BlackBerry::Platform::String::emptyString());

    void download(const Platform::NetworkRequest&);

    bool executeJavaScript(const BlackBerry::Platform::String& script, JavaScriptDataType& returnType, BlackBerry::Platform::String& returnValue);

    // This will execute the script even if in-page JavaScript is disabled.
    bool executeJavaScriptInIsolatedWorld(const BlackBerry::Platform::String& script, JavaScriptDataType& returnType, BlackBerry::Platform::String& returnValue);

    // Takes a UTF16 encoded script that is used explicitly by the pattern matching code
    bool executeJavaScriptInIsolatedWorld(const std::wstring& script, JavaScriptDataType& returnType, BlackBerry::Platform::String& returnValue);

    void executeJavaScriptFunction(const std::vector<BlackBerry::Platform::String> &function, const std::vector<JavaScriptVariant> &args, JavaScriptVariant& returnValue);

    void initializeIconDataBase();

    void stopLoading();

    bool dispatchBeforeUnloadEvent();

    // This will force any unload handlers to run.
    void prepareToDestroy();

    void enableCrossSiteXHR();
    void addOriginAccessWhitelistEntry(const BlackBerry::Platform::String& sourceOrigin, const BlackBerry::Platform::String& destinationOrigin, bool allowDestinationSubdomains);
    void removeOriginAccessWhitelistEntry(const BlackBerry::Platform::String& sourceOrigin, const BlackBerry::Platform::String& destinationOrigin, bool allowDestinationSubdomains);

    void reload();
    void reloadFromCache();

    WebSettings* settings() const;

    WebCookieJar* cookieJar() const;

    void setVisible(bool);
    bool isVisible() const;

    void setScreenOrientation(int);
    void applyPendingOrientationIfNeeded();

    Platform::ViewportAccessor* webkitThreadViewportAccessor() const;
    Platform::IntSize viewportSize() const;
    void setViewportSize(const Platform::IntSize&, bool ensureFocusElementVisible = true);

    void resetVirtualViewportOnCommitted(bool reset);
    void setVirtualViewportSize(const Platform::IntSize&);

    // Used for default layout size unless overridden by web content or by other APIs.
    void setDefaultLayoutSize(const Platform::IntSize&);

    bool mouseEvent(const Platform::MouseEvent&, bool* wheelDeltaAccepted = 0);

    // Handles native javascript touch events.
    bool touchEvent(const Platform::TouchEvent&);

    // For conversion to mouse events.
    void touchEventCancel();
    void touchPointAsMouseEvent(const Platform::TouchPoint&);

    void playSoundIfAnchorIsTarget() const;

    // Returns true if the key stroke was handled by WebKit.
    bool keyEvent(const Platform::KeyboardEvent&);

    BlackBerry::Platform::String title() const;
    BlackBerry::Platform::String selectedText() const;
    BlackBerry::Platform::String cutSelectedText();
    void insertText(const BlackBerry::Platform::String&);
    void clearCurrentInputField();

    void cut();
    void copy();
    void paste();
    void selectAll();

    // Text encoding.
    BlackBerry::Platform::String textEncoding();
    BlackBerry::Platform::String forcedTextEncoding();
    void setForcedTextEncoding(const BlackBerry::Platform::String&);

    // Scroll position provided should be in document coordinates.
    // Use webkitThreadViewportAccessor() to retrieve the scroll position.
    void setDocumentScrollPosition(const Platform::IntPoint&);
    void notifyInRegionScrollStopped();
    void setDocumentScrollOriginPoint(const Platform::IntPoint&);

    BackingStore* backingStore() const;

    InRegionScroller* inRegionScroller() const;

    bool blockZoom(const Platform::IntPoint& documentTargetPoint);
    void blockZoomAnimationFinished();
    void resetBlockZoom();
    bool isAtInitialZoom() const;
    bool isMaxZoomed() const;
    bool isMinZoomed() const;
    bool pinchZoomAboutPoint(double scale, const Platform::FloatPoint& documentFocalPoint);

    bool isUserScalable() const;
    void setUserScalable(bool);
    double currentScale() const;
    double initialScale() const;
    double zoomToFitScale() const;
    void setInitialScale(double);
    double minimumScale() const;
    void setMinimumScale(double);
    double maximumScale() const;
    void setMaximumScale(double);

    void assignFocus(Platform::FocusDirection);

    void setFocused(bool);

    void clearBrowsingData();
    void clearHistory();
    void clearCookies();
    void clearCache();
    void clearLocalStorage();
    void clearCredentials();
    void clearAutofillData();
    void clearNeverRememberSites();

    void runLayoutTests();

    // Find the next utf8 string in the given direction.
    // Case sensitivity, wrapping, and highlighting all matches are also toggleable.
    bool findNextString(const char*, bool forward, bool caseSensitive, bool wrap, bool highlightAllMatches);

    JSContextRef scriptContext() const;
    JSValueRef windowObject() const;

    unsigned timeoutForJavaScriptExecution() const;
    void setTimeoutForJavaScriptExecution(unsigned ms);

    void setCaretHighlightStyle(Platform::CaretHighlightStyle);

    // IMF functions.
    bool setBatchEditingActive(bool);
    bool setInputSelection(unsigned start, unsigned end);
    int inputCaretPosition() const;
    bool deleteTextRelativeToCursor(unsigned leftOffset, unsigned rightOffset);
    spannable_string_t* selectedText(int32_t flags);
    spannable_string_t* textBeforeCursor(int32_t length, int32_t flags);
    spannable_string_t* textAfterCursor(int32_t length, int32_t flags);
    extracted_text_t* extractedTextRequest(extracted_text_request_t*, int32_t flags);
    int32_t setComposingRegion(int32_t start, int32_t end);
    int32_t finishComposition();
    int32_t setComposingText(spannable_string_t*, int32_t relativeCursorPosition);
    int32_t commitText(spannable_string_t*, int32_t relativeCursorPosition);

    void setSpellCheckingEnabled(bool);
    void spellCheckingRequestProcessed(int32_t transactionId, spannable_string_t*);
    void spellCheckingRequestCancelled(int32_t transactionId);

    void setDocumentSelection(const Platform::IntPoint& documentStartPoint, const Platform::IntPoint& documentEndPoint);
    void setDocumentCaretPosition(const Platform::IntPoint&);
    void selectAtDocumentPoint(const Platform::IntPoint&);
    void selectionCancelled();
    bool selectionContainsDocumentPoint(const Platform::IntPoint&);

    void popupListClosed(int size, const bool* selecteds);
    void popupListClosed(int index);
    void setDateTimeInput(const BlackBerry::Platform::String& value);
    void setColorInput(const BlackBerry::Platform::String& value);

    void onInputLocaleChanged(bool isRTL);
    static void onNetworkAvailabilityChanged(bool available);
    static void onCertificateStoreLocationSet(const BlackBerry::Platform::String& caPath);

    BlackBerry::Platform::String textHasAttribute(const BlackBerry::Platform::String& query) const;

    Platform::WebContext webContext(TargetDetectionStrategy) const;

    typedef intptr_t BackForwardId;
    struct BackForwardEntry {
        BlackBerry::Platform::String url;
        BlackBerry::Platform::String originalUrl;
        BlackBerry::Platform::String title;
        BlackBerry::Platform::String networkToken;
        BackForwardId id;
        bool lastVisitWasHTTPNonGet;
    };

    bool canGoBackOrForward(int delta) const;
    // Returns false if there is no page for the given delta (eg.
    // attempt to go back with -1 when on the first page).
    bool goBackOrForward(int delta);
    void goToBackForwardEntry(BackForwardId);

    int backForwardListLength() const;
    void getBackForwardList(SharedArray<BackForwardEntry>& result) const;
    void releaseBackForwardEntry(BackForwardId) const;
    void clearBackForwardList(bool keepCurrentPage) const;

    void addVisitedLink(const unsigned short* url, unsigned length);

#if defined(ENABLE_WEBDOM) && ENABLE_WEBDOM
    WebDOMDocument document() const;
    WebDOMNode nodeAtDocumentPoint(const Platform::IntPoint&);
    bool getNodeRect(const WebDOMNode&, Platform::IntRect& result);
    bool setNodeFocus(const WebDOMNode&, bool on);
    bool setNodeHovered(const WebDOMNode&, bool on);
    bool nodeHasHover(const WebDOMNode&);
#endif

    bool defersLoading() const;

    bool isEnableLocalAccessToAllCookies() const;
    void setEnableLocalAccessToAllCookies(bool);

    void enableDNSPrefetch();
    void disableDNSPrefetch();
    bool isDNSPrefetchEnabled() const;
    void enableWebInspector();
    void disableWebInspector();
    bool isWebInspectorEnabled();
    void enablePasswordEcho();
    void disablePasswordEcho();
    void dispatchInspectorMessage(const BlackBerry::Platform::String& message);
    void inspectCurrentContextElement();

    // FIXME: Needs API review on this header. See PR #120402.
    void notifyPagePause();
    void notifyPageResume();
    void notifyPageBackground();
    void notifyPageForeground();
    void notifyPageFullScreenAllowed();
    void notifyPageFullScreenExit();
    void notifyDeviceIdleStateChange(bool enterIdle);
    void notifyAppActivationStateChange(ActivationStateType);
    void notifySwipeEvent();
    void notifyScreenPowerStateChanged(bool powered);
    void notifyFullScreenVideoExited(bool done);
    void clearPluginSiteData();
    void setJavaScriptCanAccessClipboard(bool);
    bool isWebGLEnabled() const;
    void setWebGLEnabled(bool);

    void destroyWebPageCompositor();

    void setUserViewportArguments(const WebViewportArguments&);
    void resetUserViewportArguments();

    WebTapHighlight* tapHighlight() const;

    // Adds an overlay that can be modified on the WebKit thread, and
    // whose attributes can be overridden on the compositing thread.
    void addOverlay(WebOverlay*);
    void removeOverlay(WebOverlay*);

    // Adds an overlay that can only be modified on the compositing thread.
    void addCompositingThreadOverlay(WebOverlay*);
    void removeCompositingThreadOverlay(WebOverlay*);

    // Popup client
    void initPopupWebView(BlackBerry::WebKit::WebPage*);
    void popupOpened(WebCore::PagePopupBlackBerry* webPopup);
    void popupClosed();
    bool hasOpenedPopup() const;
    WebCore::PagePopupBlackBerry* popup();

    void autofillTextField(const BlackBerry::Platform::String&);

    void enableQnxJavaScriptObject(bool);

    BlackBerry::Platform::String renderTreeAsText();

    void updateNotificationPermission(const BlackBerry::Platform::String& requestId, bool allowed);
    void notificationClicked(const BlackBerry::Platform::String& notificationId);
    void notificationClosed(const BlackBerry::Platform::String& notificationId);
    void notificationError(const BlackBerry::Platform::String& notificationId);
    void notificationShown(const BlackBerry::Platform::String& notificationId);

private:
    virtual ~WebPage();

    friend class WebKit::BackingStore;
    friend class WebKit::BackingStoreClient;
    friend class WebKit::BackingStorePrivate;
    friend class WebKit::RenderQueue;
    friend class WebKit::WebPageCompositor;
    friend class WebKit::WebPageGroupLoadDeferrer;
    friend class WebKit::WebPagePrivate;
    friend class WebCore::ChromeClientBlackBerry;
    friend class WebCore::FrameLoaderClientBlackBerry;
    friend class WebCore::PagePopupBlackBerry;
    WebPagePrivate* d;
};
}
}

#endif // WebPage_h
