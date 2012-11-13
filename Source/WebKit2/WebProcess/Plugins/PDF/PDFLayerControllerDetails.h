/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#import <PDFKit/PDFKit.h>

@protocol PDFLayerControllerDelegate <NSObject>

- (void)updateScrollPosition:(CGPoint)newPosition;
- (void)writeItemsToPasteboard:(NSArray *)items withTypes:(NSArray *)types;
- (void)showDefinitionForAttributedString:(NSAttributedString *)string atPoint:(CGPoint)point;
- (void)performWebSearch:(NSString *)string;
- (void)openWithPreview;
- (void)saveToPDF;

- (void)pdfLayerController:(PDFLayerController *)pdfLayerController didChangeActiveAnnotation:(PDFAnnotation *)annotation;
- (void)pdfLayerController:(PDFLayerController *)pdfLayerController clickedLinkWithURL:(NSURL *)url;
- (void)pdfLayerController:(PDFLayerController *)pdfLayerController didChangeContentScaleFactor:(CGFloat)scaleFactor;

@end

@interface PDFLayerController : NSObject
@end

@interface PDFLayerController (Details)

@property(retain) CALayer *parentLayer;
@property(retain) PDFDocument *document;
@property(retain) id<PDFLayerControllerDelegate> delegate;

- (void)setFrameSize:(CGSize)size;

- (void)setDisplayMode:(int)mode;
- (void)setDisplaysPageBreaks:(BOOL)pageBreaks;

- (CGFloat)contentScaleFactor;
- (void)setContentScaleFactor:(CGFloat)scaleFactor;

- (CGFloat)deviceScaleFactor;
- (void)setDeviceScaleFactor:(CGFloat)scaleFactor;

- (CGSize)contentSize;
- (CGSize)contentSizeRespectingZoom;

- (void)snapshotInContext:(CGContextRef)context;

- (void)magnifyWithMagnification:(CGFloat)magnification atPoint:(CGPoint)point immediately:(BOOL)immediately;

- (CGPoint)scrollPosition;
- (void)setScrollPosition:(CGPoint)newPosition;
- (void)scrollWithDelta:(CGSize)delta;

- (void)mouseDown:(NSEvent *)event;
- (void)rightMouseDown:(NSEvent *)event;
- (void)mouseMoved:(NSEvent *)event;
- (void)mouseUp:(NSEvent *)event;
- (void)mouseDragged:(NSEvent *)event;
- (void)mouseEntered:(NSEvent *)event;
- (void)mouseExited:(NSEvent *)event;

- (NSMenu *)menuForEvent:(NSEvent *)event;

- (NSArray *)findString:(NSString *)string caseSensitive:(BOOL)isCaseSensitive highlightMatches:(BOOL)shouldHighlightMatches;

- (id)currentSelection;
- (void)copySelection;
- (void)selectAll;

- (bool)keyDown:(NSEvent *)event;

- (void)setHUDEnabled:(BOOL)enabled;
- (BOOL)hudEnabled;

- (CGRect)boundsForAnnotation:(PDFAnnotation *)annotation;

@end
