/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#import "config.h"
#import "WebTileLayer.h"

#import "TileCache.h"
#import <wtf/CurrentTime.h>
#import <wtf/UnusedParam.h>

using namespace WebCore;

@interface WebTileLayer (ScrollingPerformanceLoggingInternal)
- (void)logFilledFreshTile;
@end

@implementation WebTileLayer

- (id<CAAction>)actionForKey:(NSString *)key
{
    UNUSED_PARAM(key);
    
    // Disable all animations.
    return nil;
}

- (void)drawInContext:(CGContextRef)context
{
    if (_tileCache) {
        _tileCache->drawLayer(self, context);

        if (_tileCache->scrollingPerformanceLoggingEnabled())
            [self logFilledFreshTile];
    }
}

- (void)setTileCache:(WebCore::TileCache*)tileCache
{
    _tileCache = tileCache;
}

- (unsigned)incrementRepaintCount
{
    return ++_repaintCount;
}

- (unsigned)repaintCount
{
    return _repaintCount;
}

- (void)logFilledFreshTile
{
    IntRect visiblePart(enclosingIntRect([self frame]));
    visiblePart.intersect(_tileCache->visibleRect());

    if ([self repaintCount] == 1 && !visiblePart.isEmpty())
        WTFLogAlways("SCROLLING: Filled visible fresh tile. Time: %f Unfilled Pixels: %u\n", WTF::monotonicallyIncreasingTime(), _tileCache->blankPixelCount());
}

@end

