/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef WebLayer_h
#define WebLayer_h

#if USE(ACCELERATED_COMPOSITING)

#import <QuartzCore/QuartzCore.h>
#import <WebCore/FloatRect.h>
#import <wtf/Vector.h>

const unsigned webLayerMaxRectsToPaint = 5;
const float webLayerWastedSpaceThreshold = 0.75f;

@interface WebSimpleLayer : CALayer
@end

@interface WebLayer : WebSimpleLayer
@end

namespace WebCore {
class GraphicsLayer;
class PlatformCALayer;
class PlatformCALayerClient;

// Functions allows us to share implementation across WebTiledLayer and WebLayer
void drawLayerContents(CGContextRef, WebCore::PlatformCALayer*);
void drawLayerContents(CGContextRef, WebCore::PlatformCALayer*, Vector<WebCore::FloatRect, webLayerMaxRectsToPaint> dirtyRects);
void drawRepaintIndicator(CGContextRef, WebCore::PlatformCALayer*, int repaintCount, CGColorRef customBackgroundColor);
}

#endif // USE(ACCELERATED_COMPOSITING)

#endif // WebLayer_h
