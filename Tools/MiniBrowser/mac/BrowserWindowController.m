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

#import "BrowserWindowController.h"

#import <WebKit2/WKPagePrivate.h>
#import <WebKit2/WKStringCF.h>
#import <WebKit2/WKURLCF.h>

@interface BrowserWindowController ()
- (void)didStartProgress;
- (void)didChangeProgress:(double)value;
- (void)didFinishProgress;
- (void)didStartProvisionalLoadForFrame:(WKFrameRef)frame;
- (void)didCommitLoadForFrame:(WKFrameRef)frame;
- (void)didReceiveServerRedirectForProvisionalLoadForFrame:(WKFrameRef)frame;
- (void)didFailProvisionalLoadWithErrorForFrame:(WKFrameRef)frame;
- (void)didFailLoadWithErrorForFrame:(WKFrameRef)frame;
- (void)didSameDocumentNavigationForFrame:(WKFrameRef)frame;
@end

@implementation BrowserWindowController

- (id)initWithContext:(WKContextRef)context
{
    if ((self = [super initWithWindowNibName:@"BrowserWindow"])) {
        _context = WKRetain(context);
        _zoomTextOnly = NO;
    }
    
    return self;
}

- (void)dealloc
{
    assert(!_context);
    [super dealloc];
}

- (IBAction)fetch:(id)sender
{
    CFURLRef cfURL = CFURLCreateWithString(0, (CFStringRef)[urlText stringValue], 0);
    if (!cfURL)
        return;

    WKURLRef url = WKURLCreateWithCFURL(cfURL);
    CFRelease(cfURL);

    WKPageLoadURL(_webView.pageRef, url);
    WKRelease(url);
}

- (IBAction)showHideWebView:(id)sender
{
    BOOL hidden = ![_webView isHidden];
    
    [_webView setHidden:hidden];
}

- (IBAction)removeReinsertWebView:(id)sender
{
    if ([_webView window]) {
        [_webView retain];
        [_webView removeFromSuperview]; 
    } else {
        [containerView addSubview:_webView];
        [_webView release];
    }
}

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
    SEL action = [menuItem action];

    if (action == @selector(zoomIn:))
        return [self canZoomIn];
    if (action == @selector(zoomOut:))
        return [self canZoomOut];
    if (action == @selector(resetZoom:))
        return [self canResetZoom];

    if (action == @selector(showHideWebView:))
        [menuItem setTitle:[_webView isHidden] ? @"Show Web View" : @"Hide Web View"];
    else if (action == @selector(removeReinsertWebView:))
        [menuItem setTitle:[_webView window] ? @"Remove Web View" : @"Insert Web View"];
    else if (action == @selector(toggleZoomMode:))
        [menuItem setState:_zoomTextOnly ? NSOnState : NSOffState];
    return YES;
}

- (IBAction)reload:(id)sender
{
    WKPageReload(_webView.pageRef);
}

- (IBAction)forceRepaint:(id)sender
{
    [_webView setNeedsDisplay:YES];
}

- (IBAction)goBack:(id)sender
{
    WKPageGoBack(_webView.pageRef);
}

- (IBAction)goForward:(id)sender
{
    WKPageGoForward(_webView.pageRef);
}

- (BOOL)validateUserInterfaceItem:(id <NSValidatedUserInterfaceItem>)item
{
    SEL action = [item action];

    if (action == @selector(goBack:))
        return _webView && WKPageCanGoBack(_webView.pageRef);
    
    if (action == @selector(goForward:))
        return _webView && WKPageCanGoForward(_webView.pageRef);
    
    return YES;
}

- (void)validateToolbar
{
    [toolbar validateVisibleItems];
}

- (BOOL)windowShouldClose:(id)sender
{
    LOG(@"windowShouldClose");
    BOOL canCloseImmediately = WKPageTryClose(_webView.pageRef);
    return canCloseImmediately;
}

- (void)windowWillClose:(NSNotification *)notification
{
    WKRelease(_context);
    _context = 0;
}

- (void)applicationTerminating
{
    WKPageClose(_webView.pageRef);
    WKRelease(_webView.pageRef);
}

#define DefaultMinimumZoomFactor (.5)
#define DefaultMaximumZoomFactor (3.0)
#define DefaultZoomFactorRatio (1.2)

- (double)currentZoomFactor
{
    return _zoomTextOnly ? WKPageGetTextZoomFactor(_webView.pageRef) : WKPageGetPageZoomFactor(_webView.pageRef);
}

- (void)setCurrentZoomFactor:(double)factor
{
    _zoomTextOnly ? WKPageSetTextZoomFactor(_webView.pageRef, factor) : WKPageSetPageZoomFactor(_webView.pageRef, factor);
}

- (BOOL)canZoomIn
{
    return [self currentZoomFactor] * DefaultZoomFactorRatio < DefaultMaximumZoomFactor;
}

- (void)zoomIn:(id)sender
{
    if (![self canZoomIn])
        return;

    double factor = [self currentZoomFactor] * DefaultZoomFactorRatio;
    [self setCurrentZoomFactor:factor];
}

- (BOOL)canZoomOut
{
    return [self currentZoomFactor] / DefaultZoomFactorRatio > DefaultMinimumZoomFactor;
}

- (void)zoomOut:(id)sender
{
    if (![self canZoomIn])
        return;

    double factor = [self currentZoomFactor] / DefaultZoomFactorRatio;
    [self setCurrentZoomFactor:factor];
}

- (BOOL)canResetZoom
{
    return _zoomTextOnly ? (WKPageGetTextZoomFactor(_webView.pageRef) != 1) : (WKPageGetPageZoomFactor(_webView.pageRef) != 1);
}

- (void)resetZoom:(id)sender
{
    if (![self canResetZoom])
        return;

    if (_zoomTextOnly)
        WKPageSetTextZoomFactor(_webView.pageRef, 1);
    else
        WKPageSetPageZoomFactor(_webView.pageRef, 1);
}

- (IBAction)toggleZoomMode:(id)sender
{
    if (_zoomTextOnly) {
        _zoomTextOnly = NO;
        double currentTextZoom = WKPageGetTextZoomFactor(_webView.pageRef);
        WKPageSetPageAndTextZoomFactors(_webView.pageRef, currentTextZoom, 1);
    } else {
        _zoomTextOnly = YES;
        double currentPageZoom = WKPageGetPageZoomFactor(_webView.pageRef);
        WKPageSetPageAndTextZoomFactors(_webView.pageRef, 1, currentPageZoom);
    }
}

- (IBAction)dumpSourceToConsole:(id)sender
{
    WKPageGetSourceForFrame_b(_webView.pageRef, WKPageGetMainFrame(_webView.pageRef), ^(WKStringRef result, WKErrorRef error) {
        CFStringRef cfResult = WKStringCopyCFString(0, result);
        LOG(@"Main frame source\n \"%@\"", (NSString *)cfResult);
        CFRelease(cfResult);
    });
}

// MARK: Loader Client Callbacks

static void didStartProvisionalLoadForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void *clientInfo)
{
    [(BrowserWindowController *)clientInfo didStartProvisionalLoadForFrame:frame];
}

static void didReceiveServerRedirectForProvisionalLoadForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void *clientInfo)
{
    [(BrowserWindowController *)clientInfo didReceiveServerRedirectForProvisionalLoadForFrame:frame];
}

static void didFailProvisionalLoadWithErrorForFrame(WKPageRef page, WKFrameRef frame, WKErrorRef error, WKTypeRef userData, const void *clientInfo)
{
    [(BrowserWindowController *)clientInfo didFailProvisionalLoadWithErrorForFrame:frame];
}

static void didCommitLoadForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void *clientInfo)
{
    [(BrowserWindowController *)clientInfo didCommitLoadForFrame:frame];
}

static void didFinishDocumentLoadForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void *clientInfo)
{
    LOG(@"didFinishDocumentLoadForFrame");
}

static void didFinishLoadForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void *clientInfo)
{
    LOG(@"didFinishLoadForFrame");
}

static void didFailLoadWithErrorForFrame(WKPageRef page, WKFrameRef frame, WKErrorRef error, WKTypeRef userData, const void *clientInfo)
{
    [(BrowserWindowController *)clientInfo didFailLoadWithErrorForFrame:frame];
}

static void didSameDocumentNavigationForFrame(WKPageRef page, WKFrameRef frame, WKSameDocumentNavigationType type, WKTypeRef userData, const void *clientInfo)
{
    [(BrowserWindowController *)clientInfo didSameDocumentNavigationForFrame:frame];
}

static void didReceiveTitleForFrame(WKPageRef page, WKStringRef title, WKFrameRef frame, WKTypeRef userData, const void *clientInfo)
{
    CFStringRef cfTitle = WKStringCopyCFString(0, title);
    LOG(@"didReceiveTitleForFrame \"%@\"", (NSString *)cfTitle);
    CFRelease(cfTitle);
}

static void didFirstLayoutForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void *clientInfo)
{
    LOG(@"didFirstLayoutForFrame");
}

static void didFirstVisuallyNonEmptyLayoutForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void *clientInfo)
{
    LOG(@"didFirstVisuallyNonEmptyLayoutForFrame");
}

static void didRemoveFrameFromHierarchy(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void *clientInfo)
{
    LOG(@"didRemoveFrameFromHierarchy");
}

static void didDisplayInsecureContentForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void *clientInfo)
{
    LOG(@"didDisplayInsecureContentForFrame");
}

static void didRunInsecureContentForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void *clientInfo)
{
    LOG(@"didRunInsecureContentForFrame");
}

static void didStartProgress(WKPageRef page, const void *clientInfo)
{
    [(BrowserWindowController *)clientInfo didStartProgress];
}

static void didChangeProgress(WKPageRef page, const void *clientInfo)
{
    [(BrowserWindowController *)clientInfo didChangeProgress:WKPageGetEstimatedProgress(page)];
}

static void didFinishProgress(WKPageRef page, const void *clientInfo)
{
    [(BrowserWindowController *)clientInfo didFinishProgress];
}

static void didBecomeUnresponsive(WKPageRef page, const void *clientInfo)
{
    LOG(@"didBecomeUnresponsive");
}

static void didBecomeResponsive(WKPageRef page, const void *clientInfo)
{
    LOG(@"didBecomeResponsive");
}

static void processDidExit(WKPageRef page, const void *clientInfo)
{
    LOG(@"processDidExit");
}

static void didChangeBackForwardList(WKPageRef page, WKBackForwardListItemRef addedItem, WKArrayRef removedItems, const void *clientInfo)
{
    [(BrowserWindowController *)clientInfo validateToolbar];
}

// MARK: Policy Client Callbacks

static void decidePolicyForNavigationAction(WKPageRef page, WKFrameRef frame, WKFrameNavigationType navigationType, WKEventModifiers modifiers, WKEventMouseButton mouseButton, WKURLRequestRef request, WKFramePolicyListenerRef listener, WKTypeRef userData, const void* clientInfo)
{
    LOG(@"decidePolicyForNavigationAction");
    WKFramePolicyListenerUse(listener);
}

static void decidePolicyForNewWindowAction(WKPageRef page, WKFrameRef frame, WKFrameNavigationType navigationType, WKEventModifiers modifiers, WKEventMouseButton mouseButton, WKURLRequestRef request, WKStringRef frameName, WKFramePolicyListenerRef listener, WKTypeRef userData, const void* clientInfo)
{
    LOG(@"decidePolicyForNewWindowAction");
    WKFramePolicyListenerUse(listener);
}

static void decidePolicyForResponse(WKPageRef page, WKFrameRef frame, WKURLResponseRef response, WKURLRequestRef request, WKFramePolicyListenerRef listener, WKTypeRef userData, const void* clientInfo)
{
    WKFramePolicyListenerUse(listener);
}

// MARK: UI Client Callbacks

static WKPageRef createNewPage(WKPageRef page, WKDictionaryRef features, WKEventModifiers modifiers, WKEventMouseButton button, const void* clientInfo)
{
    LOG(@"createNewPage");
    BrowserWindowController *controller = [[BrowserWindowController alloc] initWithContext:WKPageGetContext(page)];
    [controller loadWindow];

    return controller->_webView.pageRef;
}

static void showPage(WKPageRef page, const void *clientInfo)
{
    LOG(@"showPage");
    [[(BrowserWindowController *)clientInfo window] orderFront:nil];
}

static void closePage(WKPageRef page, const void *clientInfo)
{
    LOG(@"closePage");
    WKPageClose(page);
    [[(BrowserWindowController *)clientInfo window] close];
    WKRelease(page);
}

static void runJavaScriptAlert(WKPageRef page, WKStringRef message, WKFrameRef frame, const void* clientInfo)
{
    NSAlert* alert = [[NSAlert alloc] init];

    WKURLRef wkURL = WKFrameCopyURL(frame);
    CFURLRef cfURL = WKURLCopyCFURL(0, wkURL);
    WKRelease(wkURL);

    [alert setMessageText:[NSString stringWithFormat:@"JavaScript alert dialog from %@.", [(NSURL *)cfURL absoluteString]]];
    CFRelease(cfURL);

    CFStringRef cfMessage = WKStringCopyCFString(0, message);
    [alert setInformativeText:(NSString *)cfMessage];
    CFRelease(cfMessage);

    [alert addButtonWithTitle:@"OK"];

    [alert runModal];
    [alert release];
}

static bool runJavaScriptConfirm(WKPageRef page, WKStringRef message, WKFrameRef frame, const void* clientInfo)
{
    NSAlert* alert = [[NSAlert alloc] init];

    WKURLRef wkURL = WKFrameCopyURL(frame);
    CFURLRef cfURL = WKURLCopyCFURL(0, wkURL);
    WKRelease(wkURL);

    [alert setMessageText:[NSString stringWithFormat:@"JavaScript confirm dialog from %@.", [(NSURL *)cfURL absoluteString]]];
    CFRelease(cfURL);

    CFStringRef cfMessage = WKStringCopyCFString(0, message);
    [alert setInformativeText:(NSString *)cfMessage];
    CFRelease(cfMessage);

    [alert addButtonWithTitle:@"OK"];
    [alert addButtonWithTitle:@"Cancel"];

    NSInteger button = [alert runModal];
    [alert release];

    return button == NSAlertFirstButtonReturn;
}

static WKStringRef runJavaScriptPrompt(WKPageRef page, WKStringRef message, WKStringRef defaultValue, WKFrameRef frame, const void* clientInfo)
{
    NSAlert* alert = [[NSAlert alloc] init];

    WKURLRef wkURL = WKFrameCopyURL(frame);
    CFURLRef cfURL = WKURLCopyCFURL(0, wkURL);
    WKRelease(wkURL);

    [alert setMessageText:[NSString stringWithFormat:@"JavaScript prompt dialog from %@.", [(NSURL *)cfURL absoluteString]]];
    CFRelease(cfURL);

    CFStringRef cfMessage = WKStringCopyCFString(0, message);
    [alert setInformativeText:(NSString *)cfMessage];
    CFRelease(cfMessage);

    [alert addButtonWithTitle:@"OK"];
    [alert addButtonWithTitle:@"Cancel"];

    NSTextField* input = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 200, 24)];
    CFStringRef cfDefaultValue = WKStringCopyCFString(0, defaultValue);
    [input setStringValue:(NSString *)cfDefaultValue];
    CFRelease(cfDefaultValue);

    [alert setAccessoryView:input];

    NSInteger button = [alert runModal];

    NSString* result = nil;
    if (button == NSAlertFirstButtonReturn) {
        [input validateEditing];
        result = [input stringValue];
    }

    [alert release];

    if (!result)
        return 0;
    return WKStringCreateWithCFString((CFStringRef)result);
}

static void setStatusText(WKPageRef page, WKStringRef text, const void* clientInfo)
{
    LOG(@"setStatusText");
}

static void mouseDidMoveOverElement(WKPageRef page, WKEventModifiers modifiers, WKTypeRef userData, const void *clientInfo)
{
    LOG(@"mouseDidMoveOverElement");
}

static WKRect getWindowFrame(WKPageRef page, const void* clientInfo)
{
    NSRect rect = [[(BrowserWindowController *)clientInfo window] frame];
    WKRect wkRect;
    wkRect.origin.x = rect.origin.x;
    wkRect.origin.y = rect.origin.y;
    wkRect.size.width = rect.size.width;
    wkRect.size.height = rect.size.height;
    return wkRect;
}

static void setWindowFrame(WKPageRef page, WKRect rect, const void* clientInfo)
{
    [[(BrowserWindowController *)clientInfo window] setFrame:NSMakeRect(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height) display:YES];
}

static bool runBeforeUnloadConfirmPanel(WKPageRef page, WKStringRef message, WKFrameRef frame, const void* clientInfo)
{
    NSAlert *alert = [[NSAlert alloc] init];

    WKURLRef wkURL = WKFrameCopyURL(frame);
    CFURLRef cfURL = WKURLCopyCFURL(0, wkURL);
    WKRelease(wkURL);

    [alert setMessageText:[NSString stringWithFormat:@"BeforeUnload confirm dialog from %@.", [(NSURL *)cfURL absoluteString]]];
    CFRelease(cfURL);

    CFStringRef cfMessage = WKStringCopyCFString(0, message);
    [alert setInformativeText:(NSString *)cfMessage];
    CFRelease(cfMessage);

    [alert addButtonWithTitle:@"OK"];
    [alert addButtonWithTitle:@"Cancel"];

    NSInteger button = [alert runModal];
    [alert release];

    return button == NSAlertFirstButtonReturn;
}

static void runOpenPanel(WKPageRef page, WKFrameRef frame, WKOpenPanelParametersRef parameters, WKOpenPanelResultListenerRef listener, const void* clientInfo)
{
    NSOpenPanel *openPanel = [NSOpenPanel openPanel];
    [openPanel setAllowsMultipleSelection:WKOpenPanelParametersGetAllowsMultipleFiles(parameters)];

    WKRetain(listener);

    [openPanel beginSheetModalForWindow:[(BrowserWindowController *)clientInfo window] completionHandler:^(NSInteger result) {
        if (result == NSFileHandlingPanelOKButton) {
            WKMutableArrayRef fileURLs = WKMutableArrayCreate();

            NSURL *nsURL;
            for (nsURL in [openPanel URLs]) {
                WKURLRef wkURL = WKURLCreateWithCFURL((CFURLRef)nsURL);
                WKArrayAppendItem(fileURLs, wkURL);
                WKRelease(wkURL);
            }

            WKOpenPanelResultListenerChooseFiles(listener, fileURLs);

            WKRelease(fileURLs);
        } else
            WKOpenPanelResultListenerCancel(listener);
        
        WKRelease(listener);
    }];
}

- (void)awakeFromNib
{
    _webView = [[WKView alloc] initWithFrame:[containerView frame] contextRef:_context];

    [containerView addSubview:_webView];
    [_webView setFrame:[containerView frame]];
    
    [_webView setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
    
    WKPageLoaderClient loadClient = {
        kWKPageLoaderClientCurrentVersion,
        self,   /* clientInfo */
        didStartProvisionalLoadForFrame,
        didReceiveServerRedirectForProvisionalLoadForFrame,
        didFailProvisionalLoadWithErrorForFrame,
        didCommitLoadForFrame,
        didFinishDocumentLoadForFrame,
        didFinishLoadForFrame,
        didFailLoadWithErrorForFrame,
        didSameDocumentNavigationForFrame,
        didReceiveTitleForFrame,
        didFirstLayoutForFrame,
        didFirstVisuallyNonEmptyLayoutForFrame,
        didRemoveFrameFromHierarchy,
        didDisplayInsecureContentForFrame,
        didRunInsecureContentForFrame,
        0, // canAuthenticateAgainstProtectionSpaceInFrame
        0, // didReceiveAuthenticationChallengeInFrame
        didStartProgress,
        didChangeProgress,
        didFinishProgress,
        didBecomeUnresponsive,
        didBecomeResponsive,
        processDidExit,
        didChangeBackForwardList,
        0, // shouldGoToBackForwardItem
        0  // didFailToInitializePlugin
    };
    WKPageSetPageLoaderClient(_webView.pageRef, &loadClient);
    
    WKPagePolicyClient policyClient = {
        kWKPagePolicyClientCurrentVersion,
        self,       /* clientInfo */
        decidePolicyForNavigationAction,
        decidePolicyForNewWindowAction,
        decidePolicyForResponse,
        0           /* unableToImplementPolicy */
    };
    WKPageSetPagePolicyClient(_webView.pageRef, &policyClient);

    WKPageUIClient uiClient = {
        kWKPageUIClientCurrentVersion,
        self,       /* clientInfo */
        createNewPage,
        showPage,
        closePage,
        0,          /* takeFocus */
        0,          /* focus */
        0,          /* unfocus */
        runJavaScriptAlert,
        runJavaScriptConfirm,
        runJavaScriptPrompt,
        setStatusText,
        mouseDidMoveOverElement,
        0,          /* missingPluginButtonClicked */
        0,          /* didNotHandleKeyEvent */
        0,          /* didNotHandleWheelEvent */
        0,          /* toolbarsAreVisible */
        0,          /* setToolbarsAreVisible */
        0,          /* menuBarIsVisible */
        0,          /* setMenuBarIsVisible */
        0,          /* statusBarIsVisible */
        0,          /* setStatusBarIsVisible */
        0,          /* isResizable */
        0,          /* setIsResizable */
        getWindowFrame,
        setWindowFrame,
        runBeforeUnloadConfirmPanel,
        0,          /* didDraw */
        0,          /* pageDidScroll */
        0,          /* exceededDatabaseQuota */
        runOpenPanel,
        0,          /* decidePolicyForGeolocationPermissionRequest */
        0, // headerHeight
        0, // footerHeight
        0, // drawHeader
        0, // drawFooter
        0, // printFrame
        0, // showModal
        0, // didCompleteRubberBandForMainFrame
        0, // saveDataToFileInDownloadsFolder
        0, // shouldInterruptJavaScript
    };
    WKPageSetPageUIClient(_webView.pageRef, &uiClient);
}

- (void)didStartProgress
{
    [progressIndicator setDoubleValue:0.0];
    [progressIndicator setHidden:NO];
}

- (void)didChangeProgress:(double)value
{
    [progressIndicator setDoubleValue:value];
}

- (void)didFinishProgress
{
    [progressIndicator setHidden:YES];
    [progressIndicator setDoubleValue:1.0];
}

- (void)updateProvisionalURLForFrame:(WKFrameRef)frame
{
    static WKURLRef emptyURL = 0;
    if (!emptyURL)
        emptyURL = WKURLCreateWithUTF8CString("");

    WKURLRef url = WKFrameCopyProvisionalURL(frame);
    if (WKURLIsEqual(url, emptyURL)) {
        WKRelease(url);
        return;
    }

    CFURLRef cfSourceURL = WKURLCopyCFURL(0, url);
    WKRelease(url);

    [urlText setStringValue:(NSString*)CFURLGetString(cfSourceURL)];
    CFRelease(cfSourceURL);
}

- (void)didStartProvisionalLoadForFrame:(WKFrameRef)frame
{
    if (!WKFrameIsMainFrame(frame))
        return;

    [self updateProvisionalURLForFrame:frame];
}

- (void)didReceiveServerRedirectForProvisionalLoadForFrame:(WKFrameRef)frame
{
    if (!WKFrameIsMainFrame(frame))
        return;

    [self updateProvisionalURLForFrame:frame];
}

- (void)didFailProvisionalLoadWithErrorForFrame:(WKFrameRef)frame
{
    if (!WKFrameIsMainFrame(frame))
        return;

    [self updateProvisionalURLForFrame:frame];
}

- (void)didFailLoadWithErrorForFrame:(WKFrameRef)frame
{
    if (!WKFrameIsMainFrame(frame))
        return;

    [self updateProvisionalURLForFrame:frame];
}

- (void)didSameDocumentNavigationForFrame:(WKFrameRef)frame
{
}

- (void)didCommitLoadForFrame:(WKFrameRef)frame
{
}

- (void)loadURLString:(NSString *)urlString
{
    // FIXME: We shouldn't have to set the url text here.
    [urlText setStringValue:urlString];
    [self fetch:nil];
}

- (IBAction)performFindPanelAction:(id)sender
{
    [findPanelWindow makeKeyAndOrderFront:sender];
}

- (IBAction)find:(id)sender
{
    WKStringRef string = WKStringCreateWithCFString((CFStringRef)[sender stringValue]);

    WKPageFindString(_webView.pageRef, string, kWKFindOptionsCaseInsensitive | kWKFindOptionsWrapAround | kWKFindOptionsShowFindIndicator | kWKFindOptionsShowOverlay, 100);
}

@end
