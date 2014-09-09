/*
 * Copyright (C) 2005, 2006 Apple Inc.  All rights reserved.
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

#import <WebKitLegacy/WebNSViewExtras.h>

#import <WebKitLegacy/DOMExtensions.h>
#import <WebKitLegacy/WebDataSource.h>
#import <WebKitLegacy/WebFramePrivate.h>
#import <WebKitLegacy/WebFrameViewInternal.h>
#import <WebKitLegacy/WebNSImageExtras.h>
#import <WebKitLegacy/WebNSURLExtras.h>
#import <WebKitLegacy/WebView.h>

#if !PLATFORM(IOS)
#import <WebKitLegacy/WebNSPasteboardExtras.h>
#endif

#if PLATFORM(IOS)
#import <WebCore/WAKViewPrivate.h>
#import <WebCore/WAKWindow.h>
#endif

#define WebDragStartHysteresisX                 5.0f
#define WebDragStartHysteresisY                 5.0f
#define WebMaxDragImageSize                     NSMakeSize(400.0f, 400.0f)
#define WebMaxOriginalImageArea                 (1500.0f * 1500.0f)
#define WebDragIconRightInset                   7.0f
#define WebDragIconBottomInset                  3.0f

@implementation NSView (WebExtras)

- (NSView *)_web_superviewOfClass:(Class)class
{
    NSView *view = [self superview];
    while (view  && ![view isKindOfClass:class])
        view = [view superview];
    return view;
}

- (WebFrameView *)_web_parentWebFrameView
{
    return (WebFrameView *)[self _web_superviewOfClass:[WebFrameView class]];
}

#if !PLATFORM(IOS)
// FIXME: Mail is the only client of _webView, remove this method once no versions of Mail need it.
- (WebView *)_webView
{
    return (WebView *)[self _web_superviewOfClass:[WebView class]];
}

/* Determine whether a mouse down should turn into a drag; started as copy of NSTableView code */
- (BOOL)_web_dragShouldBeginFromMouseDown:(NSEvent *)mouseDownEvent
                           withExpiration:(NSDate *)expiration
                              xHysteresis:(float)xHysteresis
                              yHysteresis:(float)yHysteresis
{
    NSEvent *nextEvent, *firstEvent, *dragEvent, *mouseUp;
    BOOL dragIt;

    if ([mouseDownEvent type] != NSLeftMouseDown) {
        return NO;
    }

    nextEvent = nil;
    firstEvent = nil;
    dragEvent = nil;
    mouseUp = nil;
    dragIt = NO;

    while ((nextEvent = [[self window] nextEventMatchingMask:(NSLeftMouseUpMask | NSLeftMouseDraggedMask)
                                                   untilDate:expiration
                                                      inMode:NSEventTrackingRunLoopMode
                                                     dequeue:YES]) != nil) {
        if (firstEvent == nil) {
            firstEvent = nextEvent;
        }

        if ([nextEvent type] == NSLeftMouseDragged) {
            float deltax = ABS([nextEvent locationInWindow].x - [mouseDownEvent locationInWindow].x);
            float deltay = ABS([nextEvent locationInWindow].y - [mouseDownEvent locationInWindow].y);
            dragEvent = nextEvent;

            if (deltax >= xHysteresis) {
                dragIt = YES;
                break;
            }

            if (deltay >= yHysteresis) {
                dragIt = YES;
                break;
            }
        } else if ([nextEvent type] == NSLeftMouseUp) {
            mouseUp = nextEvent;
            break;
        }
    }

    // Since we've been dequeuing the events (If we don't, we'll never see the mouse up...),
    // we need to push some of the events back on.  It makes sense to put the first and last
    // drag events and the mouse up if there was one.
    if (mouseUp != nil) {
        [NSApp postEvent:mouseUp atStart:YES];
    }
    if (dragEvent != nil) {
        [NSApp postEvent:dragEvent atStart:YES];
    }
    if (firstEvent != mouseUp && firstEvent != dragEvent) {
        [NSApp postEvent:firstEvent atStart:YES];
    }

    return dragIt;
}

- (BOOL)_web_dragShouldBeginFromMouseDown:(NSEvent *)mouseDownEvent
                           withExpiration:(NSDate *)expiration
{
    return [self _web_dragShouldBeginFromMouseDown:mouseDownEvent
                                    withExpiration:expiration
                                       xHysteresis:WebDragStartHysteresisX
                                       yHysteresis:WebDragStartHysteresisY];
}


- (NSDragOperation)_web_dragOperationForDraggingInfo:(id <NSDraggingInfo>)sender
{
    if (![NSApp modalWindow] && 
        ![[self window] attachedSheet] &&
        [sender draggingSource] != self &&
        [[sender draggingPasteboard] _web_bestURL]) {

        return NSDragOperationCopy;
    }
    
    return NSDragOperationNone;
}

- (void)_web_DragImageForElement:(DOMElement *)element
                         rect:(NSRect)rect
                        event:(NSEvent *)event
                   pasteboard:(NSPasteboard *)pasteboard 
                       source:(id)source
                       offset:(NSPoint *)dragImageOffset
{
    NSPoint mouseDownPoint = [self convertPoint:[event locationInWindow] fromView:nil];
    NSImage *dragImage;
    NSPoint origin;

    NSImage *image = [element image];
    if (image != nil && [image size].height * [image size].width <= WebMaxOriginalImageArea) {
        NSSize originalSize = rect.size;
        origin = rect.origin;
        
        dragImage = [[image copy] autorelease];
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        [dragImage setScalesWhenResized:YES];
#pragma clang diagnostic pop
        [dragImage setSize:originalSize];
        
        [dragImage _web_scaleToMaxSize:WebMaxDragImageSize];
        NSSize newSize = [dragImage size];
        
        [dragImage _web_dissolveToFraction:WebDragImageAlpha];
        
        // Properly orient the drag image and orient it differently if it's smaller than the original
        origin.x = mouseDownPoint.x - (((mouseDownPoint.x - origin.x) / originalSize.width) * newSize.width);
        origin.y = origin.y + originalSize.height;
        origin.y = mouseDownPoint.y - (((mouseDownPoint.y - origin.y) / originalSize.height) * newSize.height);
    } else {
        // FIXME: This has been broken for a while.
        // There's no way to get the MIME type for the image from a DOM element.
        // The old code used WKGetPreferredExtensionForMIMEType([image MIMEType]);
        NSString *extension = @"";
        dragImage = [[NSWorkspace sharedWorkspace] iconForFileType:extension];
        NSSize offset = NSMakeSize([dragImage size].width - WebDragIconRightInset, -WebDragIconBottomInset);
        origin = NSMakePoint(mouseDownPoint.x - offset.width, mouseDownPoint.y - offset.height);
    }

    // This is the offset from the lower left corner of the image to the mouse location.  Because we
    // are a flipped view the calculation of Y is inverted.
    if (dragImageOffset) {
        dragImageOffset->x = mouseDownPoint.x - origin.x;
        dragImageOffset->y = origin.y - mouseDownPoint.y;
    }
    
    // Per kwebster, offset arg is ignored
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    [self dragImage:dragImage at:origin offset:NSZeroSize event:event pasteboard:pasteboard source:source slideBack:YES];
#pragma clang diagnostic pop
}
#endif // !PLATFORM(IOS)

- (BOOL)_web_firstResponderIsSelfOrDescendantView
{
    NSResponder *responder = [[self window] firstResponder];
    return (responder && 
           (responder == self || 
           ([responder isKindOfClass:[NSView class]] && [(NSView *)responder isDescendantOf:self])));
}

- (NSRect)_web_convertRect:(NSRect)aRect toView:(NSView *)aView
{
    // Converting to this view's window; let -convertRect:toView: handle it
    if (aView == nil)
        return [self convertRect:aRect toView:nil];
        
    // This view must be in a window.  Do whatever weird thing -convertRect:toView: does in this situation.
    NSWindow *thisWindow = [self window];
    if (!thisWindow)
        return [self convertRect:aRect toView:aView];
    
    // The other view must be in a window, too.
    NSWindow *otherWindow = [aView window];
    if (!otherWindow)
        return [self convertRect:aRect toView:aView];

    // Convert to this window's coordinates
    NSRect convertedRect = [self convertRect:aRect toView:nil];
    
    // Convert to screen coordinates
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    convertedRect.origin = [thisWindow convertBaseToScreen:convertedRect.origin];

    // Convert to other window's coordinates
    convertedRect.origin = [otherWindow convertScreenToBase:convertedRect.origin];
#pragma clang diagnostic pop
    
    // Convert to other view's coordinates
    convertedRect = [aView convertRect:convertedRect fromView:nil];
    
    return convertedRect;
}

@end

#if PLATFORM(IOS)
@implementation NSView (WebDocumentViewExtras)

- (WebFrame *)_frame
{
    WebFrameView *webFrameView = [self _web_parentWebFrameView];
    return [webFrameView webFrame];
}

- (WebView *)_webView
{
    // We used to use the view hierarchy exclusively here, but that won't work
    // right when the first viewDidMoveToSuperview call is done, and this will.
    return [[self _frame] webView];
}

@end
#endif
