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

#ifndef TileCache_h
#define TileCache_h

#include "IntSize.h"
#include <wtf/Noncopyable.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RetainPtr.h>

OBJC_CLASS CALayer;
OBJC_CLASS WebTileCacheLayer;
OBJC_CLASS WebTileLayer;

namespace WebCore {

class IntPoint;
class IntRect;

class TileCache {
    WTF_MAKE_NONCOPYABLE(TileCache);

public:
    static PassOwnPtr<TileCache> create(WebTileCacheLayer*, const IntSize& tileSize);

    void tileCacheLayerBoundsChanged();

    void setNeedsDisplay();
    void setNeedsDisplayInRect(const IntRect&);
    void drawLayer(WebTileLayer*, CGContextRef);

    bool acceleratesDrawing() const { return m_acceleratesDrawing; }
    void setAcceleratesDrawing(bool);

    CALayer *tileContainerLayer() const { return m_tileContainerLayer.get(); }

    float tileDebugBorderWidth() const { return m_tileDebugBorderWidth; }
    void setTileDebugBorderWidth(float);

    CGColorRef tileDebugBorderColor() const { return m_tileDebugBorderColor.get(); }
    void setTileDebugBorderColor(CGColorRef);

private:
    TileCache(WebTileCacheLayer*, const IntSize& tileSize);

    IntRect bounds() const;
    void getTileRangeForRect(const IntRect&, IntPoint& topLeft, IntPoint& bottomRight);

    IntSize numTilesForGridSize(const IntSize&) const;
    void resizeTileGrid(const IntSize& numTiles);

    WebTileLayer* tileLayerAtPosition(const IntPoint&) const;
    RetainPtr<WebTileLayer> createTileLayer();

    bool shouldShowRepaintCounters() const;

    WebTileCacheLayer* m_tileCacheLayer;
    const IntSize m_tileSize;

    RetainPtr<CALayer> m_tileContainerLayer;

    RetainPtr<CGColorRef> m_tileDebugBorderColor;
    float m_tileDebugBorderWidth;

    // Number of tiles in each dimension.
    IntSize m_numTilesInGrid;

    bool m_acceleratesDrawing;
};

} // namespace WebCore

#endif // TileCache_h
