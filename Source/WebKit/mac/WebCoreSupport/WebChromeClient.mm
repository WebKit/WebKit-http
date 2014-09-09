/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2010 Nokia Corporation and/or its subsidiary(-ies)
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
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
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

#import "WebChromeClient.h"

#import "DOMElementInternal.h"
#import "DOMNodeInternal.h"
#import "PopupMenuMac.h"
#import "SearchPopupMenuMac.h"
#import "WebBasePluginPackage.h"
#import "WebDefaultUIDelegate.h"
#import "WebDelegateImplementationCaching.h"
#import "WebElementDictionary.h"
#import "WebFrameInternal.h"
#import "WebFrameView.h"
#import "WebHTMLViewInternal.h"
#import "WebHistoryInternal.h"
#import "WebKitFullScreenListener.h"
#import "WebKitPrefix.h"
#import "WebKitSystemInterface.h"
#import "WebNSURLRequestExtras.h"
#import "WebOpenPanelResultListener.h"
#import "WebPlugin.h"
#import "WebQuotaManager.h"
#import "WebSecurityOriginInternal.h"
#import "WebUIDelegatePrivate.h"
#import "WebView.h"
#import "WebViewInternal.h"
#import <Foundation/Foundation.h>
#import <WebCore/BlockExceptions.h>
#import <WebCore/ColorChooser.h>
#import <WebCore/ContextMenu.h>
#import <WebCore/ContextMenuController.h>
#import <WebCore/Cursor.h>
#import <WebCore/Element.h>
#import <WebCore/FileChooser.h>
#import <WebCore/FileIconLoader.h>
#import <WebCore/FloatRect.h>
#import <WebCore/Frame.h>
#import <WebCore/FrameLoadRequest.h>
#import <WebCore/FrameView.h>
#import <WebCore/GraphicsLayer.h>
#import <WebCore/HTMLInputElement.h>
#import <WebCore/HTMLNames.h>
#import <WebCore/HTMLPlugInImageElement.h>
#import <WebCore/HitTestResult.h>
#import <WebCore/Icon.h>
#import <WebCore/IntPoint.h>
#import <WebCore/IntRect.h>
#import <WebCore/NavigationAction.h>
#import <WebCore/NotImplemented.h>
#import <WebCore/Page.h>
#import <WebCore/PlatformScreen.h>
#import <WebCore/ResourceRequest.h>
#import <WebCore/SerializedCryptoKeyWrap.h>
#import <WebCore/Widget.h>
#import <WebCore/WindowFeatures.h>
#import <wtf/PassRefPtr.h>
#import <wtf/Vector.h>
#import <wtf/text/WTFString.h>

#if USE(PLUGIN_HOST_PROCESS) && ENABLE(NETSCAPE_PLUGIN_API)
#import "NetscapePluginHostManager.h"
#endif

#if PLATFORM(IOS) && ENABLE(GEOLOCATION)
#import <WebCore/Geolocation.h>
#endif

#if PLATFORM(IOS)
#import <WebCore/WAKClipView.h>
#import <WebCore/WAKWindow.h>
#import <WebCore/WebCoreThreadMessage.h>
#endif

NSString *WebConsoleMessageXMLMessageSource = @"XMLMessageSource";
NSString *WebConsoleMessageJSMessageSource = @"JSMessageSource";
NSString *WebConsoleMessageNetworkMessageSource = @"NetworkMessageSource";
NSString *WebConsoleMessageConsoleAPIMessageSource = @"ConsoleAPIMessageSource";
NSString *WebConsoleMessageStorageMessageSource = @"StorageMessageSource";
NSString *WebConsoleMessageAppCacheMessageSource = @"AppCacheMessageSource";
NSString *WebConsoleMessageRenderingMessageSource = @"RenderingMessageSource";
NSString *WebConsoleMessageCSSMessageSource = @"CSSMessageSource";
NSString *WebConsoleMessageSecurityMessageSource = @"SecurityMessageSource";
NSString *WebConsoleMessageOtherMessageSource = @"OtherMessageSource";

NSString *WebConsoleMessageDebugMessageLevel = @"DebugMessageLevel";
NSString *WebConsoleMessageLogMessageLevel = @"LogMessageLevel";
NSString *WebConsoleMessageWarningMessageLevel = @"WarningMessageLevel";
NSString *WebConsoleMessageErrorMessageLevel = @"ErrorMessageLevel";


#if !PLATFORM(IOS)
@interface NSApplication (WebNSApplicationDetails)
- (NSCursor *)_cursorRectCursor;
@end
#endif

@interface NSView (WebNSViewDetails)
- (NSView *)_findLastViewInKeyViewLoop;
@end

// For compatibility with old SPI.
@interface NSView (WebOldWebKitPlugInDetails)
- (void)setIsSelected:(BOOL)isSelected;
@end

#if !PLATFORM(IOS)
@interface NSWindow (AppKitSecretsIKnowAbout)
- (NSRect)_growBoxRect;
@end
#endif

using namespace WebCore;
using namespace HTMLNames;

WebChromeClient::WebChromeClient(WebView *webView) 
    : m_webView(webView)
{
}

void WebChromeClient::chromeDestroyed()
{
    delete this;
}

// These functions scale between window and WebView coordinates because JavaScript/DOM operations 
// assume that the WebView and the window share the same coordinate system.

void WebChromeClient::setWindowRect(const FloatRect& rect)
{
#if !PLATFORM(IOS)
    NSRect windowRect = toDeviceSpace(rect, [m_webView window]);
    [[m_webView _UIDelegateForwarder] webView:m_webView setFrame:windowRect];
#endif
}

FloatRect WebChromeClient::windowRect()
{
#if !PLATFORM(IOS)
    NSRect windowRect = [[m_webView _UIDelegateForwarder] webViewFrame:m_webView];
    return toUserSpace(windowRect, [m_webView window]);
#else
    return FloatRect();
#endif
}

// FIXME: We need to add API for setting and getting this.
FloatRect WebChromeClient::pageRect()
{
    return [m_webView frame];
}

void WebChromeClient::focus()
{
    [[m_webView _UIDelegateForwarder] webViewFocus:m_webView];
}

void WebChromeClient::unfocus()
{
    [[m_webView _UIDelegateForwarder] webViewUnfocus:m_webView];
}

bool WebChromeClient::canTakeFocus(FocusDirection)
{
    // There's unfortunately no way to determine if we will become first responder again
    // once we give it up, so we just have to guess that we won't.
    return true;
}

void WebChromeClient::takeFocus(FocusDirection direction)
{
#if !PLATFORM(IOS)
    if (direction == FocusDirectionForward) {
        // Since we're trying to move focus out of m_webView, and because
        // m_webView may contain subviews within it, we ask it for the next key
        // view of the last view in its key view loop. This makes m_webView
        // behave as if it had no subviews, which is the behavior we want.
        NSView *lastView = [m_webView _findLastViewInKeyViewLoop];
        // avoid triggering assertions if the WebView is the only thing in the key loop
        if ([m_webView _becomingFirstResponderFromOutside] && m_webView == [lastView nextValidKeyView])
            return;
        [[m_webView window] selectKeyViewFollowingView:lastView];
    } else {
        // avoid triggering assertions if the WebView is the only thing in the key loop
        if ([m_webView _becomingFirstResponderFromOutside] && m_webView == [m_webView previousValidKeyView])
            return;
        [[m_webView window] selectKeyViewPrecedingView:m_webView];
    }
#endif
}

void WebChromeClient::focusedElementChanged(Element* element)
{
    if (!element)
        return;
    if (!isHTMLInputElement(element))
        return;

    HTMLInputElement* inputElement = toHTMLInputElement(element);
    if (!inputElement->isText())
        return;

    CallFormDelegate(m_webView, @selector(didFocusTextField:inFrame:), kit(inputElement), kit(inputElement->document().frame()));
}

void WebChromeClient::focusedFrameChanged(Frame*)
{
}

Page* WebChromeClient::createWindow(Frame* frame, const FrameLoadRequest&, const WindowFeatures& features, const NavigationAction&)
{
    id delegate = [m_webView UIDelegate];
    WebView *newWebView;

#if ENABLE(FULLSCREEN_API)
    if (frame->document() && frame->document()->webkitCurrentFullScreenElement())
        frame->document()->webkitCancelFullScreen();
#endif
    
    if ([delegate respondsToSelector:@selector(webView:createWebViewWithRequest:windowFeatures:)]) {
        NSNumber *x = features.xSet ? [[NSNumber alloc] initWithFloat:features.x] : nil;
        NSNumber *y = features.ySet ? [[NSNumber alloc] initWithFloat:features.y] : nil;
        NSNumber *width = features.widthSet ? [[NSNumber alloc] initWithFloat:features.width] : nil;
        NSNumber *height = features.heightSet ? [[NSNumber alloc] initWithFloat:features.height] : nil;
        NSNumber *menuBarVisible = [[NSNumber alloc] initWithBool:features.menuBarVisible];
        NSNumber *statusBarVisible = [[NSNumber alloc] initWithBool:features.statusBarVisible];
        NSNumber *toolBarVisible = [[NSNumber alloc] initWithBool:features.toolBarVisible];
        NSNumber *scrollbarsVisible = [[NSNumber alloc] initWithBool:features.scrollbarsVisible];
        NSNumber *resizable = [[NSNumber alloc] initWithBool:features.resizable];
        NSNumber *fullscreen = [[NSNumber alloc] initWithBool:features.fullscreen];
        NSNumber *dialog = [[NSNumber alloc] initWithBool:features.dialog];
        
        NSMutableDictionary *dictFeatures = [[NSMutableDictionary alloc] initWithObjectsAndKeys:
                                             menuBarVisible, @"menuBarVisible", 
                                             statusBarVisible, @"statusBarVisible",
                                             toolBarVisible, @"toolBarVisible",
                                             scrollbarsVisible, @"scrollbarsVisible",
                                             resizable, @"resizable",
                                             fullscreen, @"fullscreen",
                                             dialog, @"dialog",
                                             nil];
        
        if (x)
            [dictFeatures setObject:x forKey:@"x"];
        if (y)
            [dictFeatures setObject:y forKey:@"y"];
        if (width)
            [dictFeatures setObject:width forKey:@"width"];
        if (height)
            [dictFeatures setObject:height forKey:@"height"];
        
        newWebView = CallUIDelegate(m_webView, @selector(webView:createWebViewWithRequest:windowFeatures:), nil, dictFeatures);
        
        [dictFeatures release];
        [x release];
        [y release];
        [width release];
        [height release];
        [menuBarVisible release];
        [statusBarVisible release];
        [toolBarVisible release];
        [scrollbarsVisible release];
        [resizable release];
        [fullscreen release];
        [dialog release];
    } else if (features.dialog && [delegate respondsToSelector:@selector(webView:createWebViewModalDialogWithRequest:)]) {
        newWebView = CallUIDelegate(m_webView, @selector(webView:createWebViewModalDialogWithRequest:), nil);
    } else {
        newWebView = CallUIDelegate(m_webView, @selector(webView:createWebViewWithRequest:), nil);
    }

#if USE(PLUGIN_HOST_PROCESS) && ENABLE(NETSCAPE_PLUGIN_API)
    if (newWebView)
        WebKit::NetscapePluginHostManager::shared().didCreateWindow();
#endif
    
    return core(newWebView);
}

void WebChromeClient::show()
{
    [[m_webView _UIDelegateForwarder] webViewShow:m_webView];
}

bool WebChromeClient::canRunModal()
{
    return [[m_webView UIDelegate] respondsToSelector:@selector(webViewRunModal:)];
}

void WebChromeClient::runModal()
{
    CallUIDelegate(m_webView, @selector(webViewRunModal:));
}

void WebChromeClient::setToolbarsVisible(bool b)
{
    [[m_webView _UIDelegateForwarder] webView:m_webView setToolbarsVisible:b];
}

bool WebChromeClient::toolbarsVisible()
{
    return CallUIDelegateReturningBoolean(NO, m_webView, @selector(webViewAreToolbarsVisible:));
}

void WebChromeClient::setStatusbarVisible(bool b)
{
    [[m_webView _UIDelegateForwarder] webView:m_webView setStatusBarVisible:b];
}

bool WebChromeClient::statusbarVisible()
{
    return CallUIDelegateReturningBoolean(NO, m_webView, @selector(webViewIsStatusBarVisible:));
}

void WebChromeClient::setScrollbarsVisible(bool b)
{
    [[[m_webView mainFrame] frameView] setAllowsScrolling:b];
}

bool WebChromeClient::scrollbarsVisible()
{
    return [[[m_webView mainFrame] frameView] allowsScrolling];
}

void WebChromeClient::setMenubarVisible(bool)
{
    // The menubar is always visible in Mac OS X.
    return;
}

bool WebChromeClient::menubarVisible()
{
    // The menubar is always visible in Mac OS X.
    return true;
}

void WebChromeClient::setResizable(bool b)
{
    [[m_webView _UIDelegateForwarder] webView:m_webView setResizable:b];
}

inline static NSString *stringForMessageSource(MessageSource source)
{
    switch (source) {
    case MessageSource::XML:
        return WebConsoleMessageXMLMessageSource;
    case MessageSource::JS:
        return WebConsoleMessageJSMessageSource;
    case MessageSource::Network:
        return WebConsoleMessageNetworkMessageSource;
    case MessageSource::ConsoleAPI:
        return WebConsoleMessageConsoleAPIMessageSource;
    case MessageSource::Storage:
        return WebConsoleMessageStorageMessageSource;
    case MessageSource::AppCache:
        return WebConsoleMessageAppCacheMessageSource;
    case MessageSource::Rendering:
        return WebConsoleMessageRenderingMessageSource;
    case MessageSource::CSS:
        return WebConsoleMessageCSSMessageSource;
    case MessageSource::Security:
        return WebConsoleMessageSecurityMessageSource;
    case MessageSource::Other:
        return WebConsoleMessageOtherMessageSource;
    }
    ASSERT_NOT_REACHED();
    return @"";
}

inline static NSString *stringForMessageLevel(MessageLevel level)
{
    switch (level) {
    case MessageLevel::Debug:
        return WebConsoleMessageDebugMessageLevel;
    case MessageLevel::Log:
        return WebConsoleMessageLogMessageLevel;
    case MessageLevel::Warning:
        return WebConsoleMessageWarningMessageLevel;
    case MessageLevel::Error:
        return WebConsoleMessageErrorMessageLevel;
    }
    ASSERT_NOT_REACHED();
    return @"";
}

void WebChromeClient::addMessageToConsole(MessageSource source, MessageLevel level, const String& message, unsigned lineNumber, unsigned columnNumber, const String& sourceURL)
{
#if !PLATFORM(IOS)
    id delegate = [m_webView UIDelegate];
#else
    if (![m_webView _allowsMessaging])
        return;

    id delegate = [m_webView _UIKitDelegate];
    // No delegate means nothing to send this data to so bail.
    if (!delegate)
        return;
#endif

    BOOL respondsToNewSelector = NO;

    SEL selector = @selector(webView:addMessageToConsole:withSource:);
    if ([delegate respondsToSelector:selector])
        respondsToNewSelector = YES;
    else {
        // The old selector only takes JSMessageSource messages.
        if (source != MessageSource::JS)
            return;
        selector = @selector(webView:addMessageToConsole:);
        if (![delegate respondsToSelector:selector])
            return;
    }

    NSString *messageSource = stringForMessageSource(source);
    NSDictionary *dictionary = [[NSDictionary alloc] initWithObjectsAndKeys:
        (NSString *)message, @"message",
        [NSNumber numberWithUnsignedInt:lineNumber], @"lineNumber",
        [NSNumber numberWithUnsignedInt:columnNumber], @"columnNumber",
        (NSString *)sourceURL, @"sourceURL",
        messageSource, @"MessageSource",
        stringForMessageLevel(level), @"MessageLevel",
        NULL];

#if PLATFORM(IOS)
    [[[m_webView _UIKitDelegateForwarder] asyncForwarder] webView:m_webView addMessageToConsole:dictionary withSource:messageSource];
#else
    if (respondsToNewSelector)
        CallUIDelegate(m_webView, selector, dictionary, messageSource);
    else
        CallUIDelegate(m_webView, selector, dictionary);
#endif

    [dictionary release];
}

bool WebChromeClient::canRunBeforeUnloadConfirmPanel()
{
    return [[m_webView UIDelegate] respondsToSelector:@selector(webView:runBeforeUnloadConfirmPanelWithMessage:initiatedByFrame:)];
}

bool WebChromeClient::runBeforeUnloadConfirmPanel(const String& message, Frame* frame)
{
    return CallUIDelegateReturningBoolean(true, m_webView, @selector(webView:runBeforeUnloadConfirmPanelWithMessage:initiatedByFrame:), message, kit(frame));
}

void WebChromeClient::closeWindowSoon()
{
    // We need to remove the parent WebView from WebViewSets here, before it actually
    // closes, to make sure that JavaScript code that executes before it closes
    // can't find it. Otherwise, window.open will select a closed WebView instead of 
    // opening a new one <rdar://problem/3572585>.

    // We also need to stop the load to prevent further parsing or JavaScript execution
    // after the window has torn down <rdar://problem/4161660>.
  
    // FIXME: This code assumes that the UI delegate will respond to a webViewClose
    // message by actually closing the WebView. Safari guarantees this behavior, but other apps might not.
    // This approach is an inherent limitation of not making a close execute immediately
    // after a call to window.close.

    [m_webView setGroupName:nil];
    [m_webView stopLoading:nil];
    [m_webView performSelector:@selector(_closeWindow) withObject:nil afterDelay:0.0];
}

void WebChromeClient::runJavaScriptAlert(Frame* frame, const String& message)
{
    id delegate = [m_webView UIDelegate];
    SEL selector = @selector(webView:runJavaScriptAlertPanelWithMessage:initiatedByFrame:);
    if ([delegate respondsToSelector:selector]) {
        CallUIDelegate(m_webView, selector, message, kit(frame));
        return;
    }

    // Call the old version of the delegate method if it is implemented.
    selector = @selector(webView:runJavaScriptAlertPanelWithMessage:);
    if ([delegate respondsToSelector:selector]) {
        CallUIDelegate(m_webView, selector, message);
        return;
    }
}

bool WebChromeClient::runJavaScriptConfirm(Frame* frame, const String& message)
{
    id delegate = [m_webView UIDelegate];
    SEL selector = @selector(webView:runJavaScriptConfirmPanelWithMessage:initiatedByFrame:);
    if ([delegate respondsToSelector:selector])
        return CallUIDelegateReturningBoolean(NO, m_webView, selector, message, kit(frame));

    // Call the old version of the delegate method if it is implemented.
    selector = @selector(webView:runJavaScriptConfirmPanelWithMessage:);
    if ([delegate respondsToSelector:selector])
        return CallUIDelegateReturningBoolean(NO, m_webView, selector, message);

    return NO;
}

bool WebChromeClient::runJavaScriptPrompt(Frame* frame, const String& prompt, const String& defaultText, String& result)
{
    id delegate = [m_webView UIDelegate];
    SEL selector = @selector(webView:runJavaScriptTextInputPanelWithPrompt:defaultText:initiatedByFrame:);
    NSString *defaultString = defaultText;
    if ([delegate respondsToSelector:selector]) {
        result = (NSString *)CallUIDelegate(m_webView, selector, prompt, defaultString, kit(frame));
        return !result.isNull();
    }

    // Call the old version of the delegate method if it is implemented.
    selector = @selector(webView:runJavaScriptTextInputPanelWithPrompt:defaultText:);
    if ([delegate respondsToSelector:selector]) {
        result = (NSString *)CallUIDelegate(m_webView, selector, prompt, defaultString);
        return !result.isNull();
    }

    result = [[WebDefaultUIDelegate sharedUIDelegate] webView:m_webView runJavaScriptTextInputPanelWithPrompt:prompt defaultText:defaultString initiatedByFrame:kit(frame)];
    return !result.isNull();
}

bool WebChromeClient::shouldInterruptJavaScript()
{
    return CallUIDelegateReturningBoolean(NO, m_webView, @selector(webViewShouldInterruptJavaScript:));
}

void WebChromeClient::setStatusbarText(const String& status)
{
    // We want the temporaries allocated here to be released even before returning to the 
    // event loop; see <http://bugs.webkit.org/show_bug.cgi?id=9880>.
    NSAutoreleasePool* localPool = [[NSAutoreleasePool alloc] init];
    CallUIDelegate(m_webView, @selector(webView:setStatusText:), (NSString *)status);
    [localPool drain];
}

IntRect WebChromeClient::windowResizerRect() const
{
#if !PLATFORM(IOS)
    return enclosingIntRect([[m_webView window] _growBoxRect]);
#else
    return IntRect();
#endif
}

bool WebChromeClient::supportsImmediateInvalidation()
{
    return true;
}

void WebChromeClient::invalidateRootView(const IntRect&)
{
}

void WebChromeClient::invalidateContentsAndRootView(const IntRect& rect)
{
}

void WebChromeClient::invalidateContentsForSlowScroll(const IntRect& rect)
{
    invalidateContentsAndRootView(rect);
}

void WebChromeClient::scroll(const IntSize&, const IntRect&, const IntRect&)
{
}

IntPoint WebChromeClient::screenToRootView(const IntPoint& p) const
{
    // FIXME: Implement this.
    return p;
}

IntRect WebChromeClient::rootViewToScreen(const IntRect& r) const
{
    // FIXME: Implement this.
    return r;
}

#if PLATFORM(IOS)
IntPoint WebChromeClient::accessibilityScreenToRootView(const IntPoint& p) const
{
    return p;
}

IntRect WebChromeClient::rootViewToAccessibilityScreen(const IntRect& r) const
{
    return r;
}
#endif

PlatformPageClient WebChromeClient::platformPageClient() const
{
    return 0;
}

void WebChromeClient::contentsSizeChanged(Frame*, const IntSize&) const
{
}

void WebChromeClient::scrollRectIntoView(const IntRect& r) const
{
    // FIXME: This scrolling behavior should be under the control of the embedding client,
    // perhaps in a delegate method, rather than something WebKit does unconditionally.
    NSView *coordinateView = [[[m_webView mainFrame] frameView] documentView];
    NSRect rect = r;
    for (NSView *view = m_webView; view; view = [view superview]) {
        if ([view isKindOfClass:[NSClipView class]]) {
            NSClipView *clipView = (NSClipView *)view;
            NSView *documentView = [clipView documentView];
            [documentView scrollRectToVisible:[documentView convertRect:rect fromView:coordinateView]];
        }
    }
}

// End host window methods.

bool WebChromeClient::shouldUnavailablePluginMessageBeButton(RenderEmbeddedObject::PluginUnavailabilityReason pluginUnavailabilityReason) const
{
    if (pluginUnavailabilityReason == RenderEmbeddedObject::PluginMissing)
        return [[m_webView UIDelegate] respondsToSelector:@selector(webView:didPressMissingPluginButton:)];

    return false;
}

void WebChromeClient::unavailablePluginButtonClicked(Element* element, RenderEmbeddedObject::PluginUnavailabilityReason pluginUnavailabilityReason) const
{
    ASSERT(element->hasTagName(objectTag) || element->hasTagName(embedTag) || element->hasTagName(appletTag));

    ASSERT(pluginUnavailabilityReason == RenderEmbeddedObject::PluginMissing);
    CallUIDelegate(m_webView, @selector(webView:didPressMissingPluginButton:), kit(element));
}

void WebChromeClient::mouseDidMoveOverElement(const HitTestResult& result, unsigned modifierFlags)
{
    WebElementDictionary *element = [[WebElementDictionary alloc] initWithHitTestResult:result];
    [m_webView _mouseDidMoveOverElement:element modifierFlags:modifierFlags];
    [element release];
}

void WebChromeClient::setToolTip(const String& toolTip, TextDirection)
{
    NSView<WebDocumentView> *documentView = [[[m_webView _selectedOrMainFrame] frameView] documentView];
    if ([documentView isKindOfClass:[WebHTMLView class]])
        [(WebHTMLView *)documentView _setToolTip:toolTip];
}

void WebChromeClient::print(Frame* frame)
{
    WebFrame *webFrame = kit(frame);
    if ([[m_webView UIDelegate] respondsToSelector:@selector(webView:printFrame:)])
        CallUIDelegate(m_webView, @selector(webView:printFrame:), webFrame);
    else
        CallUIDelegate(m_webView, @selector(webView:printFrameView:), [webFrame frameView]);
}

#if ENABLE(SQL_DATABASE)

void WebChromeClient::exceededDatabaseQuota(Frame* frame, const String& databaseName, DatabaseDetails)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    WebSecurityOrigin *webOrigin = [[WebSecurityOrigin alloc] _initWithWebCoreSecurityOrigin:frame->document()->securityOrigin()];
    CallUIDelegate(m_webView, @selector(webView:frame:exceededDatabaseQuotaForSecurityOrigin:database:), kit(frame), webOrigin, (NSString *)databaseName);
    [webOrigin release];

    END_BLOCK_OBJC_EXCEPTIONS;
}

#endif

void WebChromeClient::reachedMaxAppCacheSize(int64_t spaceNeeded)
{
    // FIXME: Free some space.
}

void WebChromeClient::reachedApplicationCacheOriginQuota(SecurityOrigin* origin, int64_t totalSpaceNeeded)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    WebSecurityOrigin *webOrigin = [[WebSecurityOrigin alloc] _initWithWebCoreSecurityOrigin:origin];
    CallUIDelegate(m_webView, @selector(webView:exceededApplicationCacheOriginQuotaForSecurityOrigin:totalSpaceNeeded:), webOrigin, static_cast<NSUInteger>(totalSpaceNeeded));
    [webOrigin release];

    END_BLOCK_OBJC_EXCEPTIONS;
}

void WebChromeClient::populateVisitedLinks()
{
    if ([m_webView historyDelegate]) {
        WebHistoryDelegateImplementationCache* implementations = WebViewGetHistoryDelegateImplementations(m_webView);
        
        if (implementations->populateVisitedLinksFunc)
            CallHistoryDelegate(implementations->populateVisitedLinksFunc, m_webView, @selector(populateVisitedLinksForWebView:));

        return;
    }

    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    [[WebHistory optionalSharedHistory] _addVisitedLinksToPageGroup:[m_webView page]->group()];
    END_BLOCK_OBJC_EXCEPTIONS;
}

#if ENABLE(DASHBOARD_SUPPORT)

void WebChromeClient::annotatedRegionsChanged()
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    CallUIDelegate(m_webView, @selector(webView:dashboardRegionsChanged:), [m_webView _dashboardRegions]);
    END_BLOCK_OBJC_EXCEPTIONS;
}

#endif

#if ENABLE(INPUT_TYPE_COLOR)
PassOwnPtr<ColorChooser> WebChromeClient::createColorChooser(ColorChooserClient* client, const Color& initialColor)
{
    // FIXME: Implement <input type='color'> for WK1 (Bug 119094).
    ASSERT_NOT_REACHED();
    return nullptr;
}
#endif

void WebChromeClient::runOpenPanel(Frame*, PassRefPtr<FileChooser> chooser)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    BOOL allowMultipleFiles = chooser->settings().allowsMultipleFiles;
    WebOpenPanelResultListener *listener = [[WebOpenPanelResultListener alloc] initWithChooser:chooser];
    id delegate = [m_webView UIDelegate];
    if ([delegate respondsToSelector:@selector(webView:runOpenPanelForFileButtonWithResultListener:allowMultipleFiles:)])
        CallUIDelegate(m_webView, @selector(webView:runOpenPanelForFileButtonWithResultListener:allowMultipleFiles:), listener, allowMultipleFiles);
    else if ([delegate respondsToSelector:@selector(webView:runOpenPanelForFileButtonWithResultListener:)])
        CallUIDelegate(m_webView, @selector(webView:runOpenPanelForFileButtonWithResultListener:), listener);
    else
        [listener cancel];
    [listener release];
    END_BLOCK_OBJC_EXCEPTIONS;
}

void WebChromeClient::loadIconForFiles(const Vector<String>& filenames, FileIconLoader* iconLoader)
{
    iconLoader->notifyFinished(Icon::createIconForFiles(filenames));
}

#if !PLATFORM(IOS)

void WebChromeClient::setCursor(const WebCore::Cursor& cursor)
{
    // FIXME: Would be nice to share this code with WebKit2's PageClientImpl.

    if ([NSApp _cursorRectCursor])
        return;

    if (!m_webView)
        return;

    NSWindow *window = [m_webView window];
    if (!window)
        return;

    if ([window windowNumber] != [NSWindow windowNumberAtPoint:[NSEvent mouseLocation] belowWindowWithWindowNumber:0])
        return;

    NSCursor *platformCursor = cursor.platformCursor();
    if ([NSCursor currentCursor] == platformCursor)
        return;

    [platformCursor set];
}

void WebChromeClient::setCursorHiddenUntilMouseMoves(bool hiddenUntilMouseMoves)
{
    [NSCursor setHiddenUntilMouseMoves:hiddenUntilMouseMoves];
}

#endif

KeyboardUIMode WebChromeClient::keyboardUIMode()
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    return [m_webView _keyboardUIMode];
    END_BLOCK_OBJC_EXCEPTIONS;
    return KeyboardAccessDefault;
}

NSResponder *WebChromeClient::firstResponder()
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    return [[m_webView _UIDelegateForwarder] webViewFirstResponder:m_webView];
    END_BLOCK_OBJC_EXCEPTIONS;
    return nil;
}

void WebChromeClient::makeFirstResponder(NSResponder *responder)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    [m_webView _pushPerformingProgrammaticFocus];
    [[m_webView _UIDelegateForwarder] webView:m_webView makeFirstResponder:responder];
    [m_webView _popPerformingProgrammaticFocus];
    END_BLOCK_OBJC_EXCEPTIONS;
}

void WebChromeClient::enableSuddenTermination()
{
#if !PLATFORM(IOS)
    [[NSProcessInfo processInfo] enableSuddenTermination];
#endif
}

void WebChromeClient::disableSuddenTermination()
{
#if !PLATFORM(IOS)
    [[NSProcessInfo processInfo] disableSuddenTermination];
#endif
}

bool WebChromeClient::shouldReplaceWithGeneratedFileForUpload(const String& path, String& generatedFilename)
{
    NSString* filename;
    if (![[m_webView _UIDelegateForwarder] webView:m_webView shouldReplaceUploadFile:path usingGeneratedFilename:&filename])
        return false;
    generatedFilename = filename;
    return true;
}

String WebChromeClient::generateReplacementFile(const String& path)
{
    return [[m_webView _UIDelegateForwarder] webView:m_webView generateReplacementFile:path];
}

void WebChromeClient::elementDidFocus(const WebCore::Node* node)
{
    CallUIDelegate(m_webView, @selector(webView:formDidFocusNode:), kit(const_cast<WebCore::Node*>(node)));
}

void WebChromeClient::elementDidBlur(const WebCore::Node* node)
{
    CallUIDelegate(m_webView, @selector(webView:formDidBlurNode:), kit(const_cast<WebCore::Node*>(node)));
}

bool WebChromeClient::selectItemWritingDirectionIsNatural()
{
    return false;
}

bool WebChromeClient::selectItemAlignmentFollowsMenuWritingDirection()
{
    return true;
}

bool WebChromeClient::hasOpenedPopup() const
{
    notImplemented();
    return false;
}

PassRefPtr<WebCore::PopupMenu> WebChromeClient::createPopupMenu(WebCore::PopupMenuClient* client) const
{
#if !PLATFORM(IOS)
    return adoptRef(new PopupMenuMac(client));
#else
    return nullptr;
#endif
}

PassRefPtr<WebCore::SearchPopupMenu> WebChromeClient::createSearchPopupMenu(WebCore::PopupMenuClient* client) const
{
#if !PLATFORM(IOS)
    return adoptRef(new SearchPopupMenuMac(client));
#else
    return nullptr;
#endif
}

bool WebChromeClient::shouldPaintEntireContents() const
{
#if PLATFORM(IOS)
    return false;
#else
    NSView *documentView = [[[m_webView mainFrame] frameView] documentView];
    return [documentView layer];
#endif
}

void WebChromeClient::attachRootGraphicsLayer(Frame* frame, GraphicsLayer* graphicsLayer)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    NSView *documentView = [[kit(frame) frameView] documentView];
    if (![documentView isKindOfClass:[WebHTMLView class]]) {
        // We should never be attaching when we don't have a WebHTMLView.
        ASSERT(!graphicsLayer);
        return;
    }

    WebHTMLView *webHTMLView = (WebHTMLView *)documentView;
    if (graphicsLayer)
        [webHTMLView attachRootLayer:graphicsLayer->platformLayer()];
    else
        [webHTMLView detachRootLayer];
    END_BLOCK_OBJC_EXCEPTIONS;
}

void WebChromeClient::setNeedsOneShotDrawingSynchronization()
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    [m_webView _setNeedsOneShotDrawingSynchronization:YES];
    END_BLOCK_OBJC_EXCEPTIONS;
}

void WebChromeClient::scheduleCompositingLayerFlush()
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    [m_webView _scheduleCompositingLayerFlush];
    END_BLOCK_OBJC_EXCEPTIONS;
}

#if ENABLE(VIDEO)

bool WebChromeClient::supportsFullscreenForNode(const Node* node)
{
#if PLATFORM(IOS)
    if (!Settings::avKitEnabled())
        return false;
#endif
    return isHTMLVideoElement(node);
}

void WebChromeClient::enterFullscreenForNode(Node* node)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    [m_webView _enterFullscreenForNode:node];
    END_BLOCK_OBJC_EXCEPTIONS;
}

void WebChromeClient::exitFullscreenForNode(Node*)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    [m_webView _exitFullscreen];
    END_BLOCK_OBJC_EXCEPTIONS;    
}

#endif

#if ENABLE(FULLSCREEN_API)

bool WebChromeClient::supportsFullScreenForElement(const Element* element, bool withKeyboard)
{
    SEL selector = @selector(webView:supportsFullScreenForElement:withKeyboard:);
    if ([[m_webView UIDelegate] respondsToSelector:selector])
        return CallUIDelegateReturningBoolean(false, m_webView, selector, kit(const_cast<WebCore::Element*>(element)), withKeyboard);
#if !PLATFORM(IOS)
    return [m_webView _supportsFullScreenForElement:const_cast<WebCore::Element*>(element) withKeyboard:withKeyboard];
#else
    return NO;
#endif
}

void WebChromeClient::enterFullScreenForElement(Element* element)
{
    SEL selector = @selector(webView:enterFullScreenForElement:listener:);
    if ([[m_webView UIDelegate] respondsToSelector:selector]) {
        WebKitFullScreenListener* listener = [[WebKitFullScreenListener alloc] initWithElement:element];
        CallUIDelegate(m_webView, selector, kit(element), listener);
        [listener release];
    }
#if !PLATFORM(IOS)
    else
        [m_webView _enterFullScreenForElement:element];
#endif
}

void WebChromeClient::exitFullScreenForElement(Element* element)
{
    SEL selector = @selector(webView:exitFullScreenForElement:listener:);
    if ([[m_webView UIDelegate] respondsToSelector:selector]) {
        WebKitFullScreenListener* listener = [[WebKitFullScreenListener alloc] initWithElement:element];
        CallUIDelegate(m_webView, selector, kit(element), listener);
        [listener release];
    }
#if !PLATFORM(IOS)
    else
        [m_webView _exitFullScreenForElement:element];
#endif
}

#endif // ENABLE(FULLSCREEN_API)

#if ENABLE(SUBTLE_CRYPTO)
bool WebChromeClient::wrapCryptoKey(const Vector<uint8_t>& key, Vector<uint8_t>& wrappedKey) const
{
    Vector<uint8_t> masterKey;
    SEL selector = @selector(webCryptoMasterKeyForWebView:);
    if ([[m_webView UIDelegate] respondsToSelector:selector]) {
        NSData *keyData = CallUIDelegate(m_webView, selector);
        masterKey.append((uint8_t*)[keyData bytes], [keyData length]);
    } else if (!getDefaultWebCryptoMasterKey(masterKey))
        return false;

    return wrapSerializedCryptoKey(masterKey, key, wrappedKey);
}

bool WebChromeClient::unwrapCryptoKey(const Vector<uint8_t>& wrappedKey, Vector<uint8_t>& key) const
{
    Vector<uint8_t> masterKey;
    SEL selector = @selector(webCryptoMasterKeyForWebView:);
    if ([[m_webView UIDelegate] respondsToSelector:selector]) {
        NSData *keyData = CallUIDelegate(m_webView, selector);
        masterKey.append((uint8_t*)[keyData bytes], [keyData length]);
    } else if (!getDefaultWebCryptoMasterKey(masterKey))
        return false;

    return unwrapSerializedCryptoKey(masterKey, wrappedKey, key);
}
#endif
