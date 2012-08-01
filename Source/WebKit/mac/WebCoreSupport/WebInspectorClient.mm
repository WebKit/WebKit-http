/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc.  All rights reserved.
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

#import "WebInspectorClient.h"

#import "DOMNodeInternal.h"
#import "WebDelegateImplementationCaching.h"
#import "WebFrameInternal.h"
#import "WebFrameView.h"
#import "WebInspector.h"
#import "WebInspectorPrivate.h"
#import "WebInspectorFrontend.h"
#import "WebLocalizableStringsInternal.h"
#import "WebNodeHighlighter.h"
#import "WebUIDelegate.h"
#import "WebPolicyDelegate.h"
#import "WebViewInternal.h"
#import <WebCore/InspectorController.h>
#import <WebCore/Page.h>
#import <WebCore/SoftLinking.h>
#import <WebKit/DOMExtensions.h>
#import <WebKitSystemInterface.h>
#import <wtf/PassOwnPtr.h>

SOFT_LINK_STAGED_FRAMEWORK_OPTIONAL(WebInspector, PrivateFrameworks, A)

using namespace WebCore;

@interface WebInspectorWindowController : NSWindowController <NSWindowDelegate> {
@private
    RetainPtr<WebView> _inspectedWebView;
    WebView *_webView;
    WebInspectorFrontendClient* _frontendClient;
    WebInspectorClient* _inspectorClient;
    BOOL _attachedToInspectedWebView;
    BOOL _shouldAttach;
    BOOL _visible;
    BOOL _destroyingInspectorView;
}
- (id)initWithInspectedWebView:(WebView *)webView;
- (NSString *)inspectorPagePath;
- (WebView *)webView;
- (void)attach;
- (void)detach;
- (BOOL)attached;
- (void)setFrontendClient:(WebInspectorFrontendClient*)frontendClient;
- (void)setInspectorClient:(WebInspectorClient*)inspectorClient;
- (WebInspectorClient*)inspectorClient;
- (void)setAttachedWindowHeight:(unsigned)height;
- (void)destroyInspectorView:(bool)notifyInspectorController;
@end


// MARK: -

WebInspectorClient::WebInspectorClient(WebView *webView)
    : m_webView(webView)
    , m_highlighter(AdoptNS, [[WebNodeHighlighter alloc] initWithInspectedWebView:webView])
    , m_frontendPage(0)
    , m_frontendClient(0)
{
}

void WebInspectorClient::inspectorDestroyed()
{
    closeInspectorFrontend();
    delete this;
}

InspectorFrontendChannel* WebInspectorClient::openInspectorFrontend(InspectorController* inspectorController)
{
    RetainPtr<WebInspectorWindowController> windowController(AdoptNS, [[WebInspectorWindowController alloc] initWithInspectedWebView:m_webView]);
    [windowController.get() setInspectorClient:this];

    m_frontendPage = core([windowController.get() webView]);
    OwnPtr<WebInspectorFrontendClient> frontendClient = adoptPtr(new WebInspectorFrontendClient(m_webView, windowController.get(), inspectorController, m_frontendPage, createFrontendSettings()));
    m_frontendClient = frontendClient.get();
    RetainPtr<WebInspectorFrontend> webInspectorFrontend(AdoptNS, [[WebInspectorFrontend alloc] initWithFrontendClient:frontendClient.get()]);
    [[m_webView inspector] setFrontend:webInspectorFrontend.get()];
    m_frontendPage->inspectorController()->setInspectorFrontendClient(frontendClient.release());
    return this;
}

void WebInspectorClient::closeInspectorFrontend()
{
    if (m_frontendClient)
        m_frontendClient->disconnectFromBackend();
}

void WebInspectorClient::bringFrontendToFront()
{
    m_frontendClient->bringToFront();
}

void WebInspectorClient::didResizeMainFrame(Frame*)
{
    if (m_frontendClient)
        m_frontendClient->setDockingUnavailable(!m_frontendClient->canAttachWindow());
}

void WebInspectorClient::highlight()
{
    [m_highlighter.get() highlight];
}

void WebInspectorClient::hideHighlight()
{
    [m_highlighter.get() hideHighlight];
}

void WebInspectorClient::releaseFrontend()
{
    m_frontendClient = 0;
    m_frontendPage = 0;
}

WebInspectorFrontendClient::WebInspectorFrontendClient(WebView* inspectedWebView, WebInspectorWindowController* windowController, InspectorController* inspectorController, Page* frontendPage, WTF::PassOwnPtr<Settings> settings)
    : InspectorFrontendClientLocal(inspectorController,  frontendPage, settings)
    , m_inspectedWebView(inspectedWebView)
    , m_windowController(windowController)
{
    [windowController setFrontendClient:this];
}

void WebInspectorFrontendClient::frontendLoaded()
{
    [m_windowController.get() showWindow:nil];
    if ([m_windowController.get() attached])
        restoreAttachedWindowHeight();

    InspectorFrontendClientLocal::frontendLoaded();

    WebFrame *frame = [m_inspectedWebView mainFrame];
    
    WebFrameLoadDelegateImplementationCache* implementations = WebViewGetFrameLoadDelegateImplementations(m_inspectedWebView);
    if (implementations->didClearInspectorWindowObjectForFrameFunc)
        CallFrameLoadDelegate(implementations->didClearInspectorWindowObjectForFrameFunc, m_inspectedWebView,
                              @selector(webView:didClearInspectorWindowObject:forFrame:), [frame windowObject], frame);

    bool attached = [m_windowController.get() attached];
    setAttachedWindow(attached);
}

static bool useWebKitWebInspector()
{
    // Call the soft link framework function to dlopen it, then [NSBundle bundleWithIdentifier:] will work.
    WebInspectorLibrary();

    if (![[NSBundle bundleWithIdentifier:@"com.apple.WebInspector"] pathForResource:@"Main" ofType:@"html"])
        return true;

    if (![[NSBundle bundleWithIdentifier:@"com.apple.WebCore"] pathForResource:@"inspector" ofType:@"html" inDirectory:@"inspector"])
        return false;

    return [[NSUserDefaults standardUserDefaults] boolForKey:@"UseWebKitWebInspector"];
}

String WebInspectorFrontendClient::localizedStringsURL()
{
    NSBundle *bundle = useWebKitWebInspector() ? [NSBundle bundleWithIdentifier:@"com.apple.WebCore"] : [NSBundle bundleWithIdentifier:@"com.apple.WebInspector"]; 
    NSString *path = [bundle pathForResource:@"localizedStrings" ofType:@"js"];
    if ([path length])
        return [[NSURL fileURLWithPath:path] absoluteString];
    return String();
}

String WebInspectorFrontendClient::hiddenPanels()
{
    NSString *hiddenPanels = [[NSUserDefaults standardUserDefaults] stringForKey:@"WebKitInspectorHiddenPanels"];
    if (hiddenPanels)
        return hiddenPanels;
    return String();
}

void WebInspectorFrontendClient::bringToFront()
{
    updateWindowTitle();
    [m_windowController.get() showWindow:nil];
}

void WebInspectorFrontendClient::closeWindow()
{
    [m_windowController.get() destroyInspectorView:true];
}

void WebInspectorFrontendClient::disconnectFromBackend()
{
    [m_windowController.get() destroyInspectorView:false];
}

void WebInspectorFrontendClient::attachWindow()
{
    if ([m_windowController.get() attached])
        return;
    [m_windowController.get() attach];
    restoreAttachedWindowHeight();
}

void WebInspectorFrontendClient::detachWindow()
{
    [m_windowController.get() detach];
}

void WebInspectorFrontendClient::setAttachedWindowHeight(unsigned height)
{
    [m_windowController.get() setAttachedWindowHeight:height];
}

void WebInspectorFrontendClient::inspectedURLChanged(const String& newURL)
{
    m_inspectedURL = newURL;
    updateWindowTitle();
}

void WebInspectorFrontendClient::updateWindowTitle() const
{
    NSString *title = [NSString stringWithFormat:UI_STRING_INTERNAL("Web Inspector — %@", "Web Inspector window title"), (NSString *)m_inspectedURL];
    [[m_windowController.get() window] setTitle:title];
}


// MARK: -

@implementation WebInspectorWindowController
- (id)init
{
    if (!(self = [super initWithWindow:nil]))
        return nil;

    // Keep preferences separate from the rest of the client, making sure we are using expected preference values.

    WebPreferences *preferences = [[WebPreferences alloc] init];
    [preferences setAllowsAnimatedImages:YES];
    [preferences setApplicationChromeModeEnabled:YES];
    [preferences setAuthorAndUserStylesEnabled:YES];
    [preferences setAutosaves:NO];
    [preferences setDefaultFixedFontSize:11];
    [preferences setFixedFontFamily:@"Menlo"];
    [preferences setJavaEnabled:NO];
    [preferences setJavaScriptEnabled:YES];
    [preferences setLoadsImagesAutomatically:YES];
    [preferences setMinimumFontSize:0];
    [preferences setMinimumLogicalFontSize:9];
    [preferences setPlugInsEnabled:NO];
    [preferences setSuppressesIncrementalRendering:YES];
    [preferences setTabsToLinks:NO];
    [preferences setUserStyleSheetEnabled:NO];

    _webView = [[WebView alloc] init];
    [_webView setPreferences:preferences];
    [_webView setDrawsBackground:NO];
    [_webView setProhibitsMainFrameScrolling:YES];
    [_webView setUIDelegate:self];
    [_webView setPolicyDelegate:self];

    [preferences release];

    NSURLRequest *request = [[NSURLRequest alloc] initWithURL:[NSURL fileURLWithPath:[self inspectorPagePath]]];
    [[_webView mainFrame] loadRequest:request];
    [request release];

    [self setWindowFrameAutosaveName:@"Web Inspector 2"];
    return self;
}

- (id)initWithInspectedWebView:(WebView *)webView
{
    if (!(self = [self init]))
        return nil;

    _inspectedWebView = webView;
    return self;
}

- (void)dealloc
{
    [_webView release];
    [super dealloc];
}

// MARK: -

- (NSString *)inspectorPagePath
{
    NSString *path;
    if (useWebKitWebInspector())
        path = [[NSBundle bundleWithIdentifier:@"com.apple.WebCore"] pathForResource:@"inspector" ofType:@"html" inDirectory:@"inspector"];
    else
        path = [[NSBundle bundleWithIdentifier:@"com.apple.WebInspector"] pathForResource:@"Main" ofType:@"html"];

    ASSERT([path length]);
    return path;
}

// MARK: -

- (WebView *)webView
{
    return _webView;
}

- (NSWindow *)window
{
    NSWindow *window = [super window];
    if (window)
        return window;

    bool useTexturedWindow = useWebKitWebInspector();

    NSUInteger styleMask = (NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask);

    if (useTexturedWindow)
        styleMask |= NSTexturedBackgroundWindowMask;

    window = [[NSWindow alloc] initWithContentRect:NSMakeRect(60.0, 200.0, 750.0, 650.0) styleMask:styleMask backing:NSBackingStoreBuffered defer:NO];
    [window setDelegate:self];
    [window setMinSize:NSMakeSize(400.0, 400.0)];

    if (useTexturedWindow) {
        [window setAutorecalculatesContentBorderThickness:NO forEdge:NSMaxYEdge];
        [window setContentBorderThickness:55. forEdge:NSMaxYEdge];
        WKNSWindowMakeBottomCornersSquare(window);
    }

    [self setWindow:window];
    [window release];

    return window;
}

// MARK: -

- (BOOL)windowShouldClose:(id)sender
{
    [self destroyInspectorView:true];

    return YES;
}

- (void)close
{
    if (!_visible)
        return;

    _visible = NO;

    if (_attachedToInspectedWebView) {
        if ([_inspectedWebView.get() _isClosed])
            return;

        [_webView removeFromSuperview];

        WebFrameView *frameView = [[_inspectedWebView.get() mainFrame] frameView];
        NSRect frameViewRect = [frameView frame];

        // Setting the height based on the previous height is done to work with
        // Safari's find banner. This assumes the previous height is the Y origin.
        frameViewRect.size.height += NSMinY(frameViewRect);
        frameViewRect.origin.y = 0.0;

        [frameView setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
        [frameView setFrame:frameViewRect];

        [_inspectedWebView.get() displayIfNeeded];
    } else
        [super close];
}

- (IBAction)showWindow:(id)sender
{
    if (_visible) {
        if (!_attachedToInspectedWebView)
            [super showWindow:sender]; // call super so the window will be ordered front if needed
        return;
    }

    _visible = YES;
    
    _shouldAttach = _inspectorClient->inspectorStartsAttached();
    
    if (_shouldAttach && !_frontendClient->canAttachWindow())
        _shouldAttach = NO;

    if (_shouldAttach) {
        WebFrameView *frameView = [[_inspectedWebView.get() mainFrame] frameView];

        [_webView removeFromSuperview];
        [_inspectedWebView.get() addSubview:_webView positioned:NSWindowBelow relativeTo:(NSView *)frameView];
        [[_inspectedWebView.get() window] makeFirstResponder:_webView];

        [_webView setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable | NSViewMaxYMargin)];
        [frameView setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable | NSViewMinYMargin)];

        _attachedToInspectedWebView = YES;
    } else {
        _attachedToInspectedWebView = NO;

        NSView *contentView = [[self window] contentView];
        [_webView setFrame:[contentView frame]];
        [_webView setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
        [_webView removeFromSuperview];
        [contentView addSubview:_webView];

        [super showWindow:nil];
    }
}

// MARK: -

- (void)attach
{
    if (_attachedToInspectedWebView)
        return;

    _inspectorClient->setInspectorStartsAttached(true);

    [self close];
    [self showWindow:nil];
}

- (void)detach
{
    if (!_attachedToInspectedWebView)
        return;

    _inspectorClient->setInspectorStartsAttached(false);

    [self close];
    [self showWindow:nil];
}

- (BOOL)attached
{
    return _attachedToInspectedWebView;
}

- (void)setFrontendClient:(WebInspectorFrontendClient*)frontendClient
{
    _frontendClient = frontendClient;
}

- (void)setInspectorClient:(WebInspectorClient*)inspectorClient
{
    _inspectorClient = inspectorClient;
}

- (WebInspectorClient*)inspectorClient
{
    return _inspectorClient;
}

- (void)setAttachedWindowHeight:(unsigned)height
{
    if (!_attachedToInspectedWebView)
        return;

    WebFrameView *frameView = [[_inspectedWebView.get() mainFrame] frameView];
    NSRect frameViewRect = [frameView frame];

    // Setting the height based on the difference is done to work with
    // Safari's find banner. This assumes the previous height is the Y origin.
    CGFloat heightDifference = (NSMinY(frameViewRect) - height);
    frameViewRect.size.height += heightDifference;
    frameViewRect.origin.y = height;

    [_webView setFrame:NSMakeRect(0.0, 0.0, NSWidth(frameViewRect), height)];
    [frameView setFrame:frameViewRect];
}

- (void)destroyInspectorView:(bool)notifyInspectorController
{
    [[_inspectedWebView.get() inspector] releaseFrontend];
    _inspectorClient->releaseFrontend();

    if (_destroyingInspectorView)
        return;
    _destroyingInspectorView = YES;

    if (_attachedToInspectedWebView)
        [self close];

    _visible = NO;

    if (notifyInspectorController) {
        if (Page* inspectedPage = [_inspectedWebView.get() page])
            inspectedPage->inspectorController()->disconnectFrontend();
    }

    RetainPtr<WebInspectorWindowController> protect(self);
    [_webView close];
}

// MARK: -
// MARK: UI delegate

- (NSUInteger)webView:(WebView *)sender dragDestinationActionMaskForDraggingInfo:(id <NSDraggingInfo>)draggingInfo
{
    return WebDragDestinationActionNone;
}

// MARK: -
// MARK: Policy delegate

- (void)webView:(WebView *)webView decidePolicyForNavigationAction:(NSDictionary *)actionInformation request:(NSURLRequest *)request frame:(WebFrame *)frame decisionListener:(id<WebPolicyDecisionListener>)listener
{
    // Allow non-main frames to navigate anywhere.
    if (frame != [webView mainFrame]) {
        [listener use];
        return;
    }

    // Allow loading of the main inspector file.
    if ([[request URL] isFileURL] && [[[request URL] path] isEqualToString:[self inspectorPagePath]]) {
        [listener use];
        return;
    }

    // Prevent everything else from loading in the inspector's page.
    [listener ignore];

    // And instead load it in the inspected page.
    [[_inspectedWebView.get() mainFrame] loadRequest:request];
}

// MARK: -
// These methods can be used by UI elements such as menu items and toolbar buttons when the inspector is the key window.

// This method is really only implemented to keep any UI elements enabled.
- (void)showWebInspector:(id)sender
{
    [[_inspectedWebView.get() inspector] show:sender];
}

- (void)showErrorConsole:(id)sender
{
    [[_inspectedWebView.get() inspector] showConsole:sender];
}

- (void)toggleDebuggingJavaScript:(id)sender
{
    [[_inspectedWebView.get() inspector] toggleDebuggingJavaScript:sender];
}

- (void)toggleProfilingJavaScript:(id)sender
{
    [[_inspectedWebView.get() inspector] toggleProfilingJavaScript:sender];
}

- (BOOL)validateUserInterfaceItem:(id <NSValidatedUserInterfaceItem>)item
{
    BOOL isMenuItem = [(id)item isKindOfClass:[NSMenuItem class]];
    if ([item action] == @selector(toggleDebuggingJavaScript:) && isMenuItem) {
        NSMenuItem *menuItem = (NSMenuItem *)item;
        if ([[_inspectedWebView.get() inspector] isDebuggingJavaScript])
            [menuItem setTitle:UI_STRING_INTERNAL("Stop Debugging JavaScript", "title for Stop Debugging JavaScript menu item")];
        else
            [menuItem setTitle:UI_STRING_INTERNAL("Start Debugging JavaScript", "title for Start Debugging JavaScript menu item")];
    } else if ([item action] == @selector(toggleProfilingJavaScript:) && isMenuItem) {
        NSMenuItem *menuItem = (NSMenuItem *)item;
        if ([[_inspectedWebView.get() inspector] isProfilingJavaScript])
            [menuItem setTitle:UI_STRING_INTERNAL("Stop Profiling JavaScript", "title for Stop Profiling JavaScript menu item")];
        else
            [menuItem setTitle:UI_STRING_INTERNAL("Start Profiling JavaScript", "title for Start Profiling JavaScript menu item")];
    }

    return YES;
}


@end
