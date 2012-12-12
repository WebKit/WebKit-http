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

#include "IntPointHash.h"
#include "IntRect.h"
#include "TiledBacking.h"
#include "Timer.h"
#include <wtf/Deque.h>
#include <wtf/HashMap.h>
#include <wtf/Noncopyable.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RetainPtr.h>

OBJC_CLASS CALayer;
OBJC_CLASS WebTileCacheLayer;
OBJC_CLASS WebTileLayer;
OBJC_CLASS WebTiledScrollingIndicatorLayer;

namespace WebCore {

class FloatRect;
class IntPoint;
class IntRect;

typedef Vector<RetainPtr<WebTileLayer> > WebTileLayerList;

class TileCache : public TiledBacking {
    WTF_MAKE_NONCOPYABLE(TileCache);

public:
    static PassOwnPtr<TileCache> create(WebTileCacheLayer*);
    ~TileCache();

    void tileCacheLayerBoundsChanged();

    void setNeedsDisplay();
    void setNeedsDisplayInRect(const IntRect&);
    void drawLayer(WebTileLayer *, CGContextRef);

    void setScale(CGFloat);
    CGFloat scale() const { return m_scale; }

    bool acceleratesDrawing() const { return m_acceleratesDrawing; }
    void setAcceleratesDrawing(bool);

    void setTilesOpaque(bool);
    bool tilesAreOpaque() const { return m_tilesAreOpaque; }

    CALayer *tileContainerLayer() const { return m_tileContainerLayer.get(); }

    void setTileDebugBorderWidth(float);
    void setTileDebugBorderColor(CGColorRef);

    virtual IntRect visibleRect() const OVERRIDE { return m_visibleRect; }

    unsigned blankPixelCount() const;
    static unsigned blankPixelCountForTiles(const WebTileLayerList&, IntRect, IntPoint);

    // Only public for the WebTileCacheMapLayer.
    void drawTileMapContents(CGContextRef, CGRect);

public:
    // Only public for inline methods in the implementation file.
    typedef IntPoint TileIndex;
    typedef unsigned TileCohort;
    static const TileCohort VisibleTileCohort = UINT_MAX;

    struct TileInfo {
        RetainPtr<WebTileLayer> layer;
        TileCohort cohort; // VisibleTileCohort is visible.
        bool hasStaleContent;
        
        TileInfo()
            : cohort(VisibleTileCohort)
            , hasStaleContent(false)
        { }
    };

private:
    TileCache(WebTileCacheLayer*);

    // TiledBacking member functions.
    virtual void setVisibleRect(const IntRect&) OVERRIDE;
    virtual void prepopulateRect(const IntRect&) OVERRIDE;
    virtual void setIsInWindow(bool) OVERRIDE;
    virtual void setTileCoverage(TileCoverage) OVERRIDE;
    virtual TileCoverage tileCoverage() const OVERRIDE { return m_tileCoverage; }
    virtual void forceRepaint() OVERRIDE;
    virtual IntSize tileSize() const OVERRIDE { return m_tileSize; }
    virtual IntRect tileGridExtent() const OVERRIDE;
    virtual void setScrollingPerformanceLoggingEnabled(bool flag) OVERRIDE { m_scrollingPerformanceLoggingEnabled = flag; }
    virtual bool scrollingPerformanceLoggingEnabled() const OVERRIDE { return m_scrollingPerformanceLoggingEnabled; }
    virtual IntRect tileCoverageRect() const OVERRIDE;
    virtual CALayer *tiledScrollingIndicatorLayer() OVERRIDE;
    virtual void setScrollingModeIndication(ScrollingModeIndication) OVERRIDE;

    IntRect bounds() const;

    IntRect rectForTileIndex(const TileIndex&) const;
    void getTileIndexRangeForRect(const IntRect&, TileIndex& topLeft, TileIndex& bottomRight) const;

    IntRect computeTileCoverageRect(const IntRect& previousVisibleRect) const;
    IntSize tileSizeForCoverageRect(const IntRect&) const;

    void scheduleTileRevalidation(double interval);
    void tileRevalidationTimerFired(Timer<TileCache>*);

    void scheduleCohortRemoval();
    void cohortRemovalTimerFired(Timer<TileCache>*);
    
    enum TileValidationPolicy {
        KeepSecondaryTiles,
        PruneSecondaryTiles
    };
    void revalidateTiles(TileValidationPolicy = KeepSecondaryTiles);
    void ensureTilesForRect(const IntRect&);
    void updateTileCoverageMap();

    void removeAllTiles();
    void removeAllSecondaryTiles();
    void removeTilesInCohort(TileCohort);

    TileCohort nextTileCohort() const;
    void startedNewCohort(TileCohort);
    
    TileCohort newestTileCohort() const;
    TileCohort oldestTileCohort() const;

    void setTileNeedsDisplayInRect(const TileIndex&, TileInfo&, const IntRect& repaintRectInTileCoords, const IntRect& coverageRectInTileCoords);

    WebTileLayer* tileLayerAtIndex(const TileIndex&) const;
    RetainPtr<WebTileLayer> createTileLayer(const IntRect&);

    bool shouldShowRepaintCounters() const;
    void drawRepaintCounter(WebTileLayer *, CGContextRef);

    WebTileCacheLayer* m_tileCacheLayer;
    RetainPtr<CALayer> m_tileContainerLayer;
    RetainPtr<WebTiledScrollingIndicatorLayer> m_tiledScrollingIndicatorLayer; // Used for coverage visualization.

    IntSize m_tileSize;
    IntRect m_visibleRect;
    IntRect m_visibleRectAtLastRevalidate;

    typedef HashMap<TileIndex, TileInfo> TileMap;
    TileMap m_tiles;
    Timer<TileCache> m_tileRevalidationTimer;
    Timer<TileCache> m_cohortRemovalTimer;

    struct TileCohortInfo {
        TileCohort cohort;
        double creationTime; // in monotonicallyIncreasingTime().
        TileCohortInfo(TileCohort inCohort, double inTime)
            : cohort(inCohort)
            , creationTime(inTime)
        { }
    };
    typedef Deque<TileCohortInfo> TileCohortList;
    TileCohortList m_cohortList;
    
    IntRect m_primaryTileCoverageRect; // In tile coords.

    CGFloat m_scale;
    CGFloat m_deviceScaleFactor;

    TileCoverage m_tileCoverage;
    bool m_isInWindow;
    bool m_scrollingPerformanceLoggingEnabled;
    bool m_acceleratesDrawing;
    bool m_tilesAreOpaque;

    RetainPtr<CGColorRef> m_tileDebugBorderColor;
    float m_tileDebugBorderWidth;
    ScrollingModeIndication m_indicatorMode;
};

} // namespace WebCore

#endif // TileCache_h
