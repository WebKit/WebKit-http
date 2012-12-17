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
#import "TileCache.h"

#import "IntRect.h"
#import "PlatformCALayer.h"
#import "Region.h"
#import "LayerPool.h"
#import "WebLayer.h"
#import "WebTileCacheLayer.h"
#import "WebTileLayer.h"
#import <QuartzCore/QuartzCore.h>
#import <wtf/MainThread.h>
#import <WebCore/BlockExceptions.h>
#import <utility>

using namespace std;

#if PLATFORM(IOS) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
@interface CALayer (WebCALayerDetails)
- (void)setAcceleratesDrawing:(BOOL)flag;
@end
#endif

@interface WebTiledScrollingIndicatorLayer : CALayer {
    WebCore::TileCache* _tileCache;
    CALayer *_visibleRectFrameLayer; // Owned by being a sublayer.
}
@property (assign) WebCore::TileCache* tileCache;
@property (assign) CALayer* visibleRectFrameLayer;
@end

@implementation WebTiledScrollingIndicatorLayer
@synthesize tileCache = _tileCache;
@synthesize visibleRectFrameLayer = _visibleRectFrameLayer;
- (id)init
{
    if ((self = [super init])) {
        [self setStyle:[NSDictionary dictionaryWithObject:[NSDictionary dictionaryWithObjectsAndKeys:[NSNull null], @"bounds", [NSNull null], @"position", [NSNull null], @"contents", nil] forKey:@"actions"]];

        _visibleRectFrameLayer = [CALayer layer];
        [_visibleRectFrameLayer setStyle:[NSDictionary dictionaryWithObject:[NSDictionary dictionaryWithObjectsAndKeys:[NSNull null], @"bounds", [NSNull null], @"position", nil] forKey:@"actions"]];
        [self addSublayer:_visibleRectFrameLayer];
        [_visibleRectFrameLayer setBorderColor:WebCore::cachedCGColor(WebCore::Color(255, 0, 0), WebCore::ColorSpaceDeviceRGB)];
        [_visibleRectFrameLayer setBorderWidth:2];
        return self;
    }
    return nil;
}

- (void)drawInContext:(CGContextRef)context
{
    if (_tileCache)
        _tileCache->drawTileMapContents(context, [self bounds]);
}
@end

namespace WebCore {

static const int defaultTileCacheWidth = 512;
static const int defaultTileCacheHeight = 512;

PassOwnPtr<TileCache> TileCache::create(WebTileCacheLayer* tileCacheLayer)
{
    return adoptPtr(new TileCache(tileCacheLayer));
}

TileCache::TileCache(WebTileCacheLayer* tileCacheLayer)
    : m_tileCacheLayer(tileCacheLayer)
    , m_tileContainerLayer(adoptCF([[CALayer alloc] init]))
    , m_tileSize(defaultTileCacheWidth, defaultTileCacheHeight)
    , m_tileRevalidationTimer(this, &TileCache::tileRevalidationTimerFired)
    , m_cohortRemovalTimer(this, &TileCache::cohortRemovalTimerFired)
    , m_scale(1)
    , m_deviceScaleFactor(1)
    , m_tileCoverage(CoverageForVisibleArea)
    , m_isInWindow(false)
    , m_scrollingPerformanceLoggingEnabled(false)
    , m_acceleratesDrawing(false)
    , m_tilesAreOpaque(false)
    , m_tileDebugBorderWidth(0)
    , m_indicatorMode(ThreadedScrollingIndication)
{
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    [m_tileCacheLayer addSublayer:m_tileContainerLayer.get()];
#ifndef NDEBUG
    [m_tileContainerLayer.get() setName:@"TileCache Container Layer"];
#endif
    [CATransaction commit];
}

TileCache::~TileCache()
{
    ASSERT(isMainThread());

    for (TileMap::iterator it = m_tiles.begin(), end = m_tiles.end(); it != end; ++it) {
        const TileInfo& tileInfo = it->value;
        [tileInfo.layer.get() setTileCache:0];
    }
    
    if (m_tiledScrollingIndicatorLayer)
        [m_tiledScrollingIndicatorLayer.get() setTileCache:nil];
}

void TileCache::tileCacheLayerBoundsChanged()
{
    if (m_tiles.isEmpty()) {
        // We must revalidate immediately instead of using a timer when there are
        // no tiles to avoid a flash when transitioning from one page to another.
        revalidateTiles();
        return;
    }

    scheduleTileRevalidation(0);
}

void TileCache::setNeedsDisplay()
{
    for (TileMap::const_iterator it = m_tiles.begin(), end = m_tiles.end(); it != end; ++it) {
        const TileInfo& tileInfo = it->value;
        [tileInfo.layer.get() setNeedsDisplay];
    }
}

void TileCache::setNeedsDisplayInRect(const IntRect& rect)
{
    if (m_tiles.isEmpty())
        return;

    FloatRect scaledRect(rect);
    scaledRect.scale(m_scale);
    IntRect repaintRectInTileCoords(enclosingIntRect(scaledRect));

    // For small invalidations, lookup the covered tiles.
    if (repaintRectInTileCoords.height() < 2 * m_tileSize.height() && repaintRectInTileCoords.width() < 2 * m_tileSize.width()) {
        TileIndex topLeft;
        TileIndex bottomRight;
        getTileIndexRangeForRect(repaintRectInTileCoords, topLeft, bottomRight);

        for (int y = topLeft.y(); y <= bottomRight.y(); ++y) {
            for (int x = topLeft.x(); x <= bottomRight.x(); ++x) {
                TileIndex tileIndex(x, y);
                
                TileMap::iterator it = m_tiles.find(tileIndex);
                if (it != m_tiles.end())
                    setTileNeedsDisplayInRect(tileIndex, it->value, repaintRectInTileCoords, m_primaryTileCoverageRect);
            }
        }
        return;
    }

    for (TileMap::iterator it = m_tiles.begin(), end = m_tiles.end(); it != end; ++it)
        setTileNeedsDisplayInRect(it->key, it->value, repaintRectInTileCoords, m_primaryTileCoverageRect);
}

void TileCache::setTileNeedsDisplayInRect(const TileIndex& tileIndex, TileInfo& tileInfo, const IntRect& repaintRectInTileCoords, const IntRect& coverageRectInTileCoords)
{
    WebTileLayer* tileLayer = tileInfo.layer.get();

    IntRect tileRect = rectForTileIndex(tileIndex);
    IntRect tileRepaintRect = tileRect;
    tileRepaintRect.intersect(repaintRectInTileCoords);
    if (tileRepaintRect.isEmpty())
        return;

    tileRepaintRect.moveBy(-tileRect.location());
    
    // We could test for intersection with the visible rect. This would reduce painting yet more,
    // but may make scrolling stale tiles into view more frequent.
    if (tileRect.intersects(coverageRectInTileCoords)) {
        [tileLayer setNeedsDisplayInRect:tileRepaintRect];

        if (shouldShowRepaintCounters()) {
            CGRect bounds = [tileLayer bounds];
            CGRect indicatorRect = CGRectMake(bounds.origin.x, bounds.origin.y, 52, 27);
            [tileLayer setNeedsDisplayInRect:indicatorRect];
        }
    } else
        tileInfo.hasStaleContent = true;
}


void TileCache::drawLayer(WebTileLayer *layer, CGContextRef context)
{
    PlatformCALayer* platformLayer = PlatformCALayer::platformCALayer(m_tileCacheLayer);
    if (!platformLayer)
        return;

    CGContextSaveGState(context);

    CGPoint layerOrigin = [layer frame].origin;
    CGContextTranslateCTM(context, -layerOrigin.x, -layerOrigin.y);
    CGContextScaleCTM(context, m_scale, m_scale);
    drawLayerContents(context, layer, platformLayer);

    CGContextRestoreGState(context);

    drawRepaintCounter(layer, context);
}

void TileCache::setScale(CGFloat scale)
{
    PlatformCALayer* platformLayer = PlatformCALayer::platformCALayer(m_tileCacheLayer);
    float deviceScaleFactor = platformLayer->owner()->platformCALayerDeviceScaleFactor();

    // The scale we get is the produce of the page scale factor and device scale factor.
    // Divide by the device scale factor so we'll get the page scale factor.
    scale /= deviceScaleFactor;

    if (m_scale == scale && m_deviceScaleFactor == deviceScaleFactor)
        return;

#if PLATFORM(IOS) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
    Vector<FloatRect> dirtyRects;

    m_deviceScaleFactor = deviceScaleFactor;
    m_scale = scale;
    [m_tileContainerLayer.get() setTransform:CATransform3DMakeScale(1 / m_scale, 1 / m_scale, 1)];

    revalidateTiles(PruneSecondaryTiles);

    for (TileMap::const_iterator it = m_tiles.begin(), end = m_tiles.end(); it != end; ++it) {
        const TileInfo& tileInfo = it->value;
        [tileInfo.layer.get() setContentsScale:deviceScaleFactor];

        IntRect tileRect = rectForTileIndex(it->key);
        FloatRect scaledTileRect = tileRect;

        scaledTileRect.scale(1 / m_scale);
        dirtyRects.append(scaledTileRect);
    }

    platformLayer->owner()->platformCALayerDidCreateTiles(dirtyRects);
#endif
}

void TileCache::setAcceleratesDrawing(bool acceleratesDrawing)
{
#if PLATFORM(IOS) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
    if (m_acceleratesDrawing == acceleratesDrawing)
        return;

    m_acceleratesDrawing = acceleratesDrawing;

    for (TileMap::const_iterator it = m_tiles.begin(), end = m_tiles.end(); it != end; ++it) {
        const TileInfo& tileInfo = it->value;
        [tileInfo.layer.get() setAcceleratesDrawing:m_acceleratesDrawing];
    }
#else
    UNUSED_PARAM(acceleratesDrawing);
#endif
}

void TileCache::setTilesOpaque(bool opaque)
{
    if (opaque == m_tilesAreOpaque)
        return;

    m_tilesAreOpaque = opaque;

    for (TileMap::iterator it = m_tiles.begin(), end = m_tiles.end(); it != end; ++it) {
        const TileInfo& tileInfo = it->value;
        [tileInfo.layer.get() setOpaque:opaque];
    }
}

void TileCache::setVisibleRect(const IntRect& visibleRect)
{
    if (m_visibleRect == visibleRect)
        return;

    m_visibleRect = visibleRect;
    revalidateTiles();
}

void TileCache::prepopulateRect(const IntRect& rect)
{
    ensureTilesForRect(rect);
}

void TileCache::setIsInWindow(bool isInWindow)
{
    if (m_isInWindow == isInWindow)
        return;

    m_isInWindow = isInWindow;

    if (!m_isInWindow) {
        const double tileRevalidationTimeout = 4;
        scheduleTileRevalidation(tileRevalidationTimeout);
    }
}

void TileCache::setTileCoverage(TileCoverage coverage)
{
    if (coverage == m_tileCoverage)
        return;

    m_tileCoverage = coverage;
    scheduleTileRevalidation(0);
}

void TileCache::forceRepaint()
{
    setNeedsDisplay();
}

void TileCache::setTileDebugBorderWidth(float borderWidth)
{
    if (m_tileDebugBorderWidth == borderWidth)
        return;

    m_tileDebugBorderWidth = borderWidth;
    for (TileMap::const_iterator it = m_tiles.begin(), end = m_tiles.end(); it != end; ++it) {
        const TileInfo& tileInfo = it->value;
        [tileInfo.layer.get() setBorderWidth:m_tileDebugBorderWidth];
    }
}

void TileCache::setTileDebugBorderColor(CGColorRef borderColor)
{
    if (m_tileDebugBorderColor == borderColor)
        return;

    m_tileDebugBorderColor = borderColor;
    for (TileMap::const_iterator it = m_tiles.begin(), end = m_tiles.end(); it != end; ++it) {
        const TileInfo& tileInfo = it->value;
        [tileInfo.layer.get() setBorderColor:m_tileDebugBorderColor.get()];
    }
}

IntRect TileCache::bounds() const
{
    return IntRect(IntPoint(), IntSize([m_tileCacheLayer bounds].size));
}

IntRect TileCache::rectForTileIndex(const TileIndex& tileIndex) const
{
    IntRect rect(tileIndex.x() * m_tileSize.width(), tileIndex.y() * m_tileSize.height(), m_tileSize.width(), m_tileSize.height());
    IntRect scaledBounds(bounds());
    scaledBounds.scale(m_scale);

    rect.intersect(scaledBounds);

    return rect;
}

void TileCache::getTileIndexRangeForRect(const IntRect& rect, TileIndex& topLeft, TileIndex& bottomRight) const
{
    IntRect clampedRect = bounds();
    clampedRect.scale(m_scale);
    clampedRect.intersect(rect);

    topLeft.setX(max(clampedRect.x() / m_tileSize.width(), 0));
    topLeft.setY(max(clampedRect.y() / m_tileSize.height(), 0));

    int bottomXRatio = ceil((float)clampedRect.maxX() / m_tileSize.width());
    bottomRight.setX(max(bottomXRatio - 1, 0));

    int bottomYRatio = ceil((float)clampedRect.maxY() / m_tileSize.height());
    bottomRight.setY(max(bottomYRatio - 1, 0));
}

IntRect TileCache::computeTileCoverageRect(const IntRect& previousVisibleRect) const
{
    // If the page is not in a window (for example if it's in a background tab), we limit the tile coverage rect to the visible rect.
    // Furthermore, if the page can't have scrollbars (for example if its body element has overflow:hidden) it's very unlikely that the
    // page will ever be scrolled so we limit the tile coverage rect as well.
    if (!m_isInWindow || m_tileCoverage & CoverageForSlowScrolling)
        return m_visibleRect;

    bool largeVisibleRectChange = !previousVisibleRect.isEmpty() && !m_visibleRect.intersects(previousVisibleRect);
    
    // FIXME: look at how far the document can scroll in each dimension.
    int coverageHorizontalSize = m_visibleRect.width();
    int coverageVerticalSize = m_visibleRect.height();
    
    // Inflate the coverage rect so that it covers 2x of the visible width and 3x of the visible height.
    // These values were chosen because it's more common to have tall pages and to scroll vertically,
    // so we keep more tiles above and below the current area.
    if (m_tileCoverage & CoverageForHorizontalScrolling && !largeVisibleRectChange)
        coverageHorizontalSize *= 2;

    if (m_tileCoverage & CoverageForVerticalScrolling && !largeVisibleRectChange)
        coverageVerticalSize *= 3;

    // Don't extend coverage before 0 or after the end.
    IntRect coverageBounds = bounds();
    int coverageLeft = m_visibleRect.x() - (coverageHorizontalSize - m_visibleRect.width()) / 2;
    coverageLeft = min(coverageLeft, coverageBounds.maxX() - coverageHorizontalSize);
    coverageLeft = max(coverageLeft, coverageBounds.x());

    int coverageTop = m_visibleRect.y() - (coverageVerticalSize - m_visibleRect.height()) / 2;
    coverageTop = min(coverageTop, coverageBounds.maxY() - coverageVerticalSize);
    coverageTop = max(coverageTop, coverageBounds.y());

    return IntRect(coverageLeft, coverageTop, coverageHorizontalSize, coverageVerticalSize);
}

IntSize TileCache::tileSizeForCoverageRect(const IntRect& coverageRect) const
{
    if (m_tileCoverage & CoverageForSlowScrolling)
        return coverageRect.size();

    return IntSize(defaultTileCacheWidth, defaultTileCacheHeight);
}

void TileCache::scheduleTileRevalidation(double interval)
{
    if (m_tileRevalidationTimer.isActive() && m_tileRevalidationTimer.nextFireInterval() < interval)
        return;

    m_tileRevalidationTimer.startOneShot(interval);
}

void TileCache::tileRevalidationTimerFired(Timer<TileCache>*)
{
    revalidateTiles(PruneSecondaryTiles);
}

unsigned TileCache::blankPixelCount() const
{
    WebTileLayerList tiles(m_tiles.size());

    for (TileMap::const_iterator it = m_tiles.begin(), end = m_tiles.end(); it != end; ++it)
        tiles.append(it->value.layer);

    return blankPixelCountForTiles(tiles, m_visibleRect, IntPoint(0,0));
}

unsigned TileCache::blankPixelCountForTiles(const WebTileLayerList& tiles, IntRect visibleRect, IntPoint tileTranslation)
{
    Region paintedVisibleTiles;

    for (WebTileLayerList::const_iterator it = tiles.begin(), end = tiles.end(); it != end; ++it) {
        const WebTileLayer* tileLayer = it->get();

        IntRect visiblePart(CGRectOffset([tileLayer frame], tileTranslation.x(), tileTranslation.y()));
        visiblePart.intersect(visibleRect);

        if (!visiblePart.isEmpty() && [tileLayer paintCount])
            paintedVisibleTiles.unite(visiblePart);
    }

    Region uncoveredRegion(visibleRect);
    uncoveredRegion.subtract(paintedVisibleTiles);

    return uncoveredRegion.totalArea();
}

static inline void queueTileForRemoval(const TileCache::TileIndex& tileIndex, const TileCache::TileInfo& tileInfo, Vector<TileCache::TileIndex>& tilesToRemove)
{
    WebTileLayer* tileLayer = tileInfo.layer.get();
    [tileLayer removeFromSuperlayer];
    [tileLayer setTileCache:0];
    tilesToRemove.append(tileIndex);
}

void TileCache::removeAllTiles()
{
    Vector<TileIndex> tilesToRemove;

    for (TileMap::iterator it = m_tiles.begin(), end = m_tiles.end(); it != end; ++it)
        queueTileForRemoval(it->key, it->value, tilesToRemove);

    for (size_t i = 0; i < tilesToRemove.size(); ++i) {
        TileInfo tileInfo = m_tiles.take(tilesToRemove[i]);
        LayerPool::sharedPool()->addLayer(tileInfo.layer);
    }
}

void TileCache::removeAllSecondaryTiles()
{
    Vector<TileIndex> tilesToRemove;

    for (TileMap::iterator it = m_tiles.begin(), end = m_tiles.end(); it != end; ++it) {
        const TileInfo& tileInfo = it->value;
        if (tileInfo.cohort == VisibleTileCohort)
            continue;

        queueTileForRemoval(it->key, it->value, tilesToRemove);
    }

    for (size_t i = 0; i < tilesToRemove.size(); ++i) {
        TileInfo tileInfo = m_tiles.take(tilesToRemove[i]);
        LayerPool::sharedPool()->addLayer(tileInfo.layer);
    }
}

void TileCache::removeTilesInCohort(TileCohort cohort)
{
    ASSERT(cohort != VisibleTileCohort);
    Vector<TileIndex> tilesToRemove;

    for (TileMap::iterator it = m_tiles.begin(), end = m_tiles.end(); it != end; ++it) {
        const TileInfo& tileInfo = it->value;
        if (tileInfo.cohort != cohort)
            continue;

        queueTileForRemoval(it->key, it->value, tilesToRemove);
    }

    for (size_t i = 0; i < tilesToRemove.size(); ++i) {
        TileInfo tileInfo = m_tiles.take(tilesToRemove[i]);
        LayerPool::sharedPool()->addLayer(tileInfo.layer);
    }
}

void TileCache::revalidateTiles(TileValidationPolicy validationPolicy)
{
    // If the underlying PlatformLayer has been destroyed, but the WebTileCacheLayer hasn't
    // platformLayer will be null here.
    PlatformCALayer* platformLayer = PlatformCALayer::platformCALayer(m_tileCacheLayer);
    if (!platformLayer)
        return;

    if (m_visibleRect.isEmpty() || bounds().isEmpty())
        return;

    // If the visible rect size changed, drop secondary tiles to avoid lots of repainting.
    // FIXME: when scaled, this changes due to rounding. We should use FloatRects or something.
    // Also changes on rubber banding.
    if (m_visibleRectAtLastRevalidate.size() != m_visibleRect.size())
        validationPolicy = PruneSecondaryTiles;
    
    IntRect tileCoverageRect = computeTileCoverageRect(m_visibleRectAtLastRevalidate);
    FloatRect scaledRect(tileCoverageRect);
    scaledRect.scale(m_scale);
    IntRect coverageRectInTileCoords(enclosingIntRect(scaledRect));

    IntSize oldTileSize = m_tileSize;
    m_tileSize = tileSizeForCoverageRect(tileCoverageRect);
    bool tileSizeChanged = m_tileSize != oldTileSize;

    if (tileSizeChanged) {
        removeAllTiles();
        m_cohortList.clear();
    } else {
        TileCohort currCohort = nextTileCohort();
        unsigned tilesInCohort = 0;
        
        // Move tiles newly outside the coverage rect into the cohort map.
        for (TileMap::iterator it = m_tiles.begin(), end = m_tiles.end(); it != end; ++it) {
            TileInfo& tileInfo = it->value;
            TileIndex tileIndex = it->key;

            WebTileLayer* tileLayer = tileInfo.layer.get();
            IntRect tileRect = rectForTileIndex(tileIndex);
            if (tileRect.intersects(coverageRectInTileCoords)) {
                tileInfo.cohort = VisibleTileCohort;
                if (tileInfo.hasStaleContent) {
                    // FIXME: store a dirty region per layer?
                    [tileLayer setNeedsDisplay];
                    tileInfo.hasStaleContent = false;
                }
            } else {
                // Add to the currentCohort if not already in one.
                if (tileInfo.cohort == VisibleTileCohort) {
                    tileInfo.cohort = currCohort;
                    ++tilesInCohort;
                }
            }
        }
        
        if (tilesInCohort)
            startedNewCohort(currCohort);

        scheduleCohortRemoval();
    }

    TileIndex topLeft;
    TileIndex bottomRight;
    getTileIndexRangeForRect(coverageRectInTileCoords, topLeft, bottomRight);

    Vector<FloatRect> dirtyRects;
    
    // Ensure primary tile coverage tiles.
    m_primaryTileCoverageRect = IntRect();
    int primaryLayerCount = 0;
    
    for (int y = topLeft.y(); y <= bottomRight.y(); ++y) {
        for (int x = topLeft.x(); x <= bottomRight.x(); ++x) {
            TileIndex tileIndex(x, y);

            IntRect tileRect = rectForTileIndex(tileIndex);
            m_primaryTileCoverageRect.unite(tileRect);

            TileInfo& tileInfo = m_tiles.add(tileIndex, TileInfo()).iterator->value;
            if (!tileInfo.layer) {
                tileInfo.layer = createTileLayer(tileRect);
                [m_tileContainerLayer.get() addSublayer:tileInfo.layer.get()];
            } else {
                // We already have a layer for this tile. Ensure that its size is correct.
                FloatSize tileLayerSize([tileInfo.layer.get() frame].size);
                if (tileLayerSize != FloatSize(tileRect.size()))
                    [tileInfo.layer.get() setFrame:tileRect];
            }
            
            ++primaryLayerCount;

            FloatRect scaledTileRect = tileRect;
            scaledTileRect.scale(1 / m_scale);
            dirtyRects.append(scaledTileRect);
        }
    }

    if (validationPolicy == PruneSecondaryTiles) {
        removeAllSecondaryTiles();
        m_cohortList.clear();
    }
    
    if (m_tiledScrollingIndicatorLayer)
        updateTileCoverageMap();

    m_visibleRectAtLastRevalidate = m_visibleRect;

    if (dirtyRects.isEmpty())
        return;
    platformLayer->owner()->platformCALayerDidCreateTiles(dirtyRects);
}

TileCache::TileCohort TileCache::nextTileCohort() const
{
    if (!m_cohortList.isEmpty())
        return m_cohortList.last().cohort + 1;

    return 1;
}

void TileCache::startedNewCohort(TileCohort cohort)
{
    m_cohortList.append(TileCohortInfo(cohort, monotonicallyIncreasingTime()));
}

TileCache::TileCohort TileCache::newestTileCohort() const
{
    return m_cohortList.isEmpty() ? 0 : m_cohortList.last().cohort;
}

TileCache::TileCohort TileCache::oldestTileCohort() const
{
    return m_cohortList.isEmpty() ? 0 : m_cohortList.first().cohort;
}

void TileCache::scheduleCohortRemoval()
{
    const double cohortRemovalTimerSeconds = 1;
    
    // Start the timer, or reschedule the timer from now if it's already active.
    if (!m_cohortRemovalTimer.isActive())
        m_cohortRemovalTimer.startRepeating(cohortRemovalTimerSeconds);
}

void TileCache::cohortRemovalTimerFired(Timer<TileCache>*)
{
    if (m_cohortList.isEmpty()) {
        m_cohortRemovalTimer.stop();
        return;
    }

    double cohortLifeTimeSeconds = 2;
    double timeThreshold = monotonicallyIncreasingTime() - cohortLifeTimeSeconds;
    
    while (!m_cohortList.isEmpty() && m_cohortList.first().creationTime < timeThreshold) {
        TileCohortInfo firstCohort = m_cohortList.takeFirst();
        removeTilesInCohort(firstCohort.cohort);
    }

    if (m_tiledScrollingIndicatorLayer)
        updateTileCoverageMap();
}

void TileCache::ensureTilesForRect(const IntRect& rect)
{
    PlatformCALayer* platformLayer = PlatformCALayer::platformCALayer(m_tileCacheLayer);
    if (!platformLayer)
        return;

    FloatRect scaledRect(rect);
    scaledRect.scale(m_scale);
    IntRect rectInTileCoords(enclosingIntRect(scaledRect));

    if (m_primaryTileCoverageRect.contains(rectInTileCoords))
        return;

    TileIndex topLeft;
    TileIndex bottomRight;
    getTileIndexRangeForRect(rectInTileCoords, topLeft, bottomRight);

    Vector<FloatRect> dirtyRects;
    TileCohort currCohort = nextTileCohort();
    unsigned tilesInCohort = 0;

    for (int y = topLeft.y(); y <= bottomRight.y(); ++y) {
        for (int x = topLeft.x(); x <= bottomRight.x(); ++x) {
            TileIndex tileIndex(x, y);

            IntRect tileRect = rectForTileIndex(tileIndex);
            TileInfo& tileInfo = m_tiles.add(tileIndex, TileInfo()).iterator->value;
            if (!tileInfo.layer) {
                tileInfo.layer = createTileLayer(tileRect);
                [m_tileContainerLayer.get() addSublayer:tileInfo.layer.get()];
            } else {
                // We already have a layer for this tile. Ensure that its size is correct.
                CGSize tileLayerSize = [tileInfo.layer.get() frame].size;
                if (tileLayerSize.width >= tileRect.width() && tileLayerSize.height >= tileRect.height())
                    continue;
                [tileInfo.layer.get() setFrame:tileRect];
            }

            if (!tileRect.intersects(m_primaryTileCoverageRect)) {
                tileInfo.cohort = currCohort;
                ++tilesInCohort;
            }

            FloatRect scaledTileRect = tileRect;
            scaledTileRect.scale(1 / m_scale);
            dirtyRects.append(scaledTileRect);
        }
    }
    
    if (tilesInCohort)
        startedNewCohort(currCohort);

    if (m_tiledScrollingIndicatorLayer)
        updateTileCoverageMap();

    if (!dirtyRects.isEmpty())
        platformLayer->owner()->platformCALayerDidCreateTiles(dirtyRects);
}

void TileCache::updateTileCoverageMap()
{
    FloatRect containerBounds = bounds();
    FloatRect visibleRect = this->visibleRect();
    visibleRect.contract(4, 4); // Layer is positioned 2px from top and left edges.

    float widthScale = 1;
    float scale = 1;
    if (!containerBounds.isEmpty()) {
        widthScale = std::min<float>(visibleRect.width() / containerBounds.width(), 0.1);
        scale = std::min(widthScale, visibleRect.height() / containerBounds.height());
    }
    
    float indicatorScale = scale * m_scale;
    FloatRect mapBounds = containerBounds;
    mapBounds.scale(indicatorScale, indicatorScale);
    
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    
    [m_tiledScrollingIndicatorLayer.get() setBounds:mapBounds];
    [m_tiledScrollingIndicatorLayer.get() setNeedsDisplay];

    visibleRect.scale(indicatorScale, indicatorScale);
    visibleRect.expand(2, 2);
    [[m_tiledScrollingIndicatorLayer.get() visibleRectFrameLayer] setFrame:visibleRect];

    Color backgroundColor;
    switch (m_indicatorMode) {
    case MainThreadScrollingBecauseOfStyleIndictaion:
        backgroundColor = Color(255, 0, 0);
        break;
    case MainThreadScrollingBecauseOfEventHandlersIndication:
        backgroundColor = Color(255, 255, 0);
        break;
    case ThreadedScrollingIndication:
        backgroundColor = Color(0, 200, 0);
        break;
    }

    [[m_tiledScrollingIndicatorLayer.get() visibleRectFrameLayer] setBorderColor:cachedCGColor(backgroundColor, ColorSpaceDeviceRGB)];

    END_BLOCK_OBJC_EXCEPTIONS
}

IntRect TileCache::tileGridExtent() const
{
    TileIndex topLeft;
    TileIndex bottomRight;
    getTileIndexRangeForRect(m_primaryTileCoverageRect, topLeft, bottomRight);

    // Return index of top, left tile and the number of tiles across and down.
    return IntRect(topLeft.x(), topLeft.y(), bottomRight.x() - topLeft.x() + 1, bottomRight.y() - topLeft.y() + 1);
}

// Return the rect in layer coords, not tile coords.
IntRect TileCache::tileCoverageRect() const
{
    IntRect coverageRectInLayerCoords(m_primaryTileCoverageRect);
    coverageRectInLayerCoords.scale(1 / m_scale);
    return coverageRectInLayerCoords;
}

CALayer *TileCache::tiledScrollingIndicatorLayer()
{
    if (!m_tiledScrollingIndicatorLayer) {
        m_tiledScrollingIndicatorLayer = [WebTiledScrollingIndicatorLayer layer];
        [m_tiledScrollingIndicatorLayer.get() setTileCache:this];
        [m_tiledScrollingIndicatorLayer.get() setOpacity:0.75];
        [m_tiledScrollingIndicatorLayer.get() setAnchorPoint:CGPointZero];
        [m_tiledScrollingIndicatorLayer.get() setBorderColor:cachedCGColor(Color::black, ColorSpaceDeviceRGB)];
        [m_tiledScrollingIndicatorLayer.get() setBorderWidth:1];
        [m_tiledScrollingIndicatorLayer.get() setPosition:CGPointMake(2, 2)];
        updateTileCoverageMap();
    }

    return m_tiledScrollingIndicatorLayer.get();
}

void TileCache::setScrollingModeIndication(ScrollingModeIndication scrollingMode)
{
    if (scrollingMode == m_indicatorMode)
        return;

    m_indicatorMode = scrollingMode;
    updateTileCoverageMap();
}

WebTileLayer* TileCache::tileLayerAtIndex(const TileIndex& index) const
{
    return m_tiles.get(index).layer.get();
}

RetainPtr<WebTileLayer> TileCache::createTileLayer(const IntRect& tileRect)
{
    RetainPtr<WebTileLayer> layer = LayerPool::sharedPool()->takeLayerWithSize(tileRect.size());
    if (layer) {
        // If we were able to restore a layer from the LayerPool, we should call setNeedsDisplay to
        // ensure we avoid stale content.
        [layer resetPaintCount];
        [layer setNeedsDisplay];
    } else
        layer = adoptNS([[WebTileLayer alloc] init]);
    [layer.get() setAnchorPoint:CGPointZero];
    [layer.get() setFrame:tileRect];
    [layer.get() setTileCache:this];
    [layer.get() setBorderColor:m_tileDebugBorderColor.get()];
    [layer.get() setBorderWidth:m_tileDebugBorderWidth];
    [layer.get() setEdgeAntialiasingMask:0];
    [layer.get() setOpaque:m_tilesAreOpaque];
#ifndef NDEBUG
    [layer.get() setName:@"Tile"];
#endif

#if PLATFORM(IOS) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
    [layer.get() setContentsScale:m_deviceScaleFactor];
    [layer.get() setAcceleratesDrawing:m_acceleratesDrawing];
#endif

    return layer;
}

bool TileCache::shouldShowRepaintCounters() const
{
    PlatformCALayer* platformLayer = PlatformCALayer::platformCALayer(m_tileCacheLayer);
    if (!platformLayer)
        return false;

    WebCore::PlatformCALayerClient* layerContents = platformLayer->owner();
    ASSERT(layerContents);
    if (!layerContents)
        return false;

    return layerContents->platformCALayerShowRepaintCounter(0);
}

void TileCache::drawRepaintCounter(WebTileLayer *layer, CGContextRef context)
{
    unsigned paintCount = [layer incrementPaintCount];
    if (!shouldShowRepaintCounters())
        return;

    // FIXME: Some of this code could be shared with WebLayer.
    char text[16]; // that's a lot of repaints
    snprintf(text, sizeof(text), "%d", paintCount);

    CGRect indicatorBox = [layer bounds];
    indicatorBox.size.width = 12 + 10 * strlen(text);
    indicatorBox.size.height = 27;
    CGContextSaveGState(context);

    CGContextSetAlpha(context, 0.5f);
    CGContextBeginTransparencyLayerWithRect(context, indicatorBox, 0);

    CGContextSetFillColorWithColor(context, m_tileDebugBorderColor.get());
    CGContextFillRect(context, indicatorBox);

    PlatformCALayer* platformLayer = PlatformCALayer::platformCALayer(m_tileCacheLayer);

    if (platformLayer->acceleratesDrawing())
        CGContextSetRGBFillColor(context, 1, 0, 0, 1);
    else
        CGContextSetRGBFillColor(context, 1, 1, 1, 1);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    CGContextSetTextMatrix(context, CGAffineTransformMakeScale(1, -1));
    CGContextSelectFont(context, "Helvetica", 22, kCGEncodingMacRoman);
    CGContextShowTextAtPoint(context, indicatorBox.origin.x + 5, indicatorBox.origin.y + 22, text, strlen(text));
#pragma clang diagnostic pop

    CGContextEndTransparencyLayer(context);
    CGContextRestoreGState(context);
}

void TileCache::drawTileMapContents(CGContextRef context, CGRect layerBounds)
{
    CGContextSetRGBFillColor(context, 0.3, 0.3, 0.3, 1);
    CGContextFillRect(context, layerBounds);

    CGFloat scaleFactor = layerBounds.size.width / bounds().width();
    
    CGContextSetRGBStrokeColor(context, 0, 0, 0, 1);

    CGFloat contextScale = scaleFactor / scale();
    CGContextScaleCTM(context, contextScale, contextScale);

    CGContextSetLineWidth(context, 0.5 / contextScale);
    
    for (TileMap::const_iterator it = m_tiles.begin(), end = m_tiles.end(); it != end; ++it) {
        const TileInfo& tileInfo = it->value;
        WebTileLayer* tileLayer = tileInfo.layer.get();
        
        CGFloat red = 1;
        CGFloat green = 1;
        CGFloat blue = 1;
        if (tileInfo.hasStaleContent) {
            red = 0.25;
            green = 0.125;
            blue = 0;
        }
        
        TileCohort newestCohort = newestTileCohort();
        TileCohort oldestCohort = oldestTileCohort();
        
        if (tileInfo.cohort != VisibleTileCohort && newestCohort > oldestCohort) {
            float cohortProportion = static_cast<float>((newestCohort - tileInfo.cohort)) / (newestCohort - oldestCohort);
            CGContextSetRGBFillColor(context, red, green, blue, 1 - cohortProportion);
        } else
            CGContextSetRGBFillColor(context, red, green, blue, 1);

        CGRect frame = [tileLayer frame];
        CGContextFillRect(context, frame);
        CGContextStrokeRect(context, frame);
    }
}
    

} // namespace WebCore
