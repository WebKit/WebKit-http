/*
 * Copyright (C) 2013, 2014 Apple Inc. All rights reserved.
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

#import "WKBase.h"
#import "WKBrowsingContextController.h"
#import "WKBrowsingContextGroup.h"
#import "WKProcessGroup.h"
#import <UIKit/UIKit.h>
#import <wtf/RetainPtr.h>

@class WKContentView;
@class WKWebView;

namespace WebCore {
struct Highlight;
}

namespace WebKit {
class DrawingAreaProxy;
class FindIndicator;
class GeolocationPermissionRequestProxy;
class RemoteLayerTreeTransaction;
class WebContext;
class WebFrameProxy;
class WebPageProxy;
class WebSecurityOrigin;
struct WebPageConfiguration;
}

@interface WKContentView : UIView {
@package
    RefPtr<WebKit::WebPageProxy> _page;
    WKWebView *_webView;
    BOOL _isBackground;
}

@property (nonatomic, readonly) WKBrowsingContextController *browsingContextController;

@property (nonatomic, readonly) WebKit::WebPageProxy* page;
@property (nonatomic, readonly) BOOL isAssistingNode;
@property (nonatomic, getter=isShowingInspectorIndication) BOOL showingInspectorIndication;
@property (nonatomic, readonly) BOOL isBackground;

- (instancetype)initWithFrame:(CGRect)frame context:(WebKit::WebContext&)context configuration:(WebKit::WebPageConfiguration)webPageConfiguration webView:(WKWebView *)webView;

- (void)didUpdateVisibleRect:(CGRect)visibleRect unobscuredRect:(CGRect)unobscuredRect
    unobscuredRectInScrollViewCoordinates:(CGRect)unobscuredRectInScrollViewCoordinates
    scale:(CGFloat)scale minimumScale:(CGFloat)minimumScale
    inStableState:(BOOL)isStableState isChangingObscuredInsetsInteractively:(BOOL)isChangingObscuredInsetsInteractively;

- (void)didFinishScrolling;
- (void)didZoomToScale:(CGFloat)scale;
- (void)willStartZoomOrScroll;

- (std::unique_ptr<WebKit::DrawingAreaProxy>)_createDrawingAreaProxy;
- (void)_processDidExit;
- (void)_didRelaunchProcess;
- (void)_setAcceleratedCompositingRootView:(UIView *)rootView;

- (void)_showInspectorHighlight:(const WebCore::Highlight&)highlight;
- (void)_hideInspectorHighlight;

- (void)_didCommitLoadForMainFrame;
- (void)_didCommitLayerTree:(const WebKit::RemoteLayerTreeTransaction&)layerTreeTransaction;

- (void)_decidePolicyForGeolocationRequestFromOrigin:(WebKit::WebSecurityOrigin&)origin frame:(WebKit::WebFrameProxy&)frame request:(WebKit::GeolocationPermissionRequestProxy&)permissionRequest;

- (void)_setAccessibilityWebProcessToken:(NSData *)data;

- (BOOL)_scrollToRect:(CGRect)targetRect withOrigin:(CGPoint)origin minimumScrollDistance:(CGFloat)minimumScrollDistance;
- (void)_zoomToFocusRect:(CGRect)rectToFocus selectionRect:(CGRect)selectionRect fontSize:(float)fontSize minimumScale:(double)minimumScale maximumScale:(double)maximumScale allowScaling:(BOOL)allowScaling forceScroll:(BOOL)forceScroll;
- (BOOL)_zoomToRect:(CGRect)targetRect withOrigin:(CGPoint)origin fitEntireRect:(BOOL)fitEntireRect minimumScale:(double)minimumScale maximumScale:(double)maximumScale minimumScrollDistance:(CGFloat)minimumScrollDistance;
- (void)_zoomOutWithOrigin:(CGPoint)origin;

@end
