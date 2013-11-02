/*
 * Copyright (C) 2011, 2012, 2013 Apple Inc. All rights reserved.
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
#import "TileController.h"

#import "IntRect.h"
#import "PlatformCALayer.h"
#import "Region.h"
#import "LayerPool.h"
#import "WebLayer.h"
#import <wtf/MainThread.h>
#import <utility>

using namespace std;

namespace WebCore {
    
enum TileValidationPolicyFlag {
    PruneSecondaryTiles = 1 << 0,
    UnparentAllTiles = 1 << 1
};

static const int defaultTileWidth = 512;
static const int defaultTileHeight = 512;

PassOwnPtr<TileController> TileController::create(PlatformCALayer* rootPlatformLayer)
{
    return adoptPtr(new TileController(rootPlatformLayer));
}

TileController::TileController(PlatformCALayer* rootPlatformLayer)
    : m_tileCacheLayer(rootPlatformLayer)
    , m_tileSize(defaultTileWidth, defaultTileHeight)
    , m_tileRevalidationTimer(this, &TileController::tileRevalidationTimerFired)
    , m_cohortRemovalTimer(this, &TileController::cohortRemovalTimerFired)
    , m_scale(1)
    , m_deviceScaleFactor(1)
    , m_tileCoverage(CoverageForVisibleArea)
    , m_isInWindow(false)
    , m_scrollingPerformanceLoggingEnabled(false)
    , m_aggressivelyRetainsTiles(false)
    , m_unparentsOffscreenTiles(false)
    , m_acceleratesDrawing(false)
    , m_tilesAreOpaque(false)
    , m_clipsToExposedRect(false)
    , m_tileDebugBorderWidth(0)
    , m_indicatorMode(ThreadedScrollingIndication)
{
    m_tileContainerLayer = m_tileCacheLayer->createCompatibleLayer(PlatformCALayer::LayerTypeLayer, nullptr);
#ifndef NDEBUG
    m_tileContainerLayer->setName("TileController Container Layer");
#endif
    rootPlatformLayer->appendSublayer(m_tileContainerLayer.get());
}

TileController::~TileController()
{
    ASSERT(isMainThread());

    for (TileMap::iterator it = m_tiles.begin(), end = m_tiles.end(); it != end; ++it)
        it->value.layer->setOwner(nullptr);

    if (m_tiledScrollingIndicatorLayer)
        m_tiledScrollingIndicatorLayer->setOwner(nullptr);
}

void TileController::tileCacheLayerBoundsChanged()
{
    ASSERT(owningGraphicsLayer()->isCommittingChanges());
    setNeedsRevalidateTiles();
}

void TileController::setNeedsDisplay()
{
    for (TileMap::const_iterator it = m_tiles.begin(), end = m_tiles.end(); it != end; ++it) {
        const TileInfo& tileInfo = it->value;
        tileInfo.layer->setNeedsDisplay();
    }
}

void TileController::setNeedsDisplayInRect(const IntRect& rect)
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

void TileController::setTileNeedsDisplayInRect(const TileIndex& tileIndex, TileInfo& tileInfo, const IntRect& repaintRectInTileCoords, const IntRect& coverageRectInTileCoords)
{
    PlatformCALayer* tileLayer = tileInfo.layer.get();

    IntRect tileRect = rectForTileIndex(tileIndex);
    FloatRect tileRepaintRect = tileRect;
    tileRepaintRect.intersect(repaintRectInTileCoords);
    if (tileRepaintRect.isEmpty())
        return;

    tileRepaintRect.moveBy(-tileRect.location());
    
    // We could test for intersection with the visible rect. This would reduce painting yet more,
    // but may make scrolling stale tiles into view more frequent.
    if (tileRect.intersects(coverageRectInTileCoords)) {
        tileLayer->setNeedsDisplay(&tileRepaintRect);

        if (owningGraphicsLayer()->platformCALayerShowRepaintCounter(0)) {
            FloatRect indicatorRect(0, 0, 52, 27);
            tileLayer->setNeedsDisplay(&indicatorRect);
        }
    } else
        tileInfo.hasStaleContent = true;
}

void TileController::platformCALayerPaintContents(PlatformCALayer* platformCALayer, GraphicsContext& context, const IntRect&)
{
    if (platformCALayer == m_tiledScrollingIndicatorLayer.get()) {
        drawTileMapContents(context.platformContext(), m_tiledScrollingIndicatorLayer->bounds());
        return;
    }

    {
        GraphicsContextStateSaver stateSaver(context);

        FloatPoint3D layerOrigin = platformCALayer->position();
        context.translate(-layerOrigin.x(), -layerOrigin.y());
        context.scale(FloatSize(m_scale, m_scale));

        drawLayerContents(context.platformContext(), m_tileCacheLayer);
    }

    int repaintCount = platformCALayerIncrementRepaintCount(platformCALayer);
    if (owningGraphicsLayer()->platformCALayerShowRepaintCounter(0))
        drawRepaintIndicator(context.platformContext(), platformCALayer, repaintCount, cachedCGColor(m_tileDebugBorderColor, ColorSpaceDeviceRGB));

    if (scrollingPerformanceLoggingEnabled()) {
        FloatRect visiblePart(platformCALayer->position().x(), platformCALayer->position().y(), platformCALayer->bounds().size().width(), platformCALayer->bounds().size().height());
        visiblePart.intersect(visibleRect());

        if (repaintCount == 1 && !visiblePart.isEmpty())
            WTFLogAlways("SCROLLING: Filled visible fresh tile. Time: %f Unfilled Pixels: %u\n", WTF::monotonicallyIncreasingTime(), blankPixelCount());
    }
}

float TileController::platformCALayerDeviceScaleFactor()
{
    return owningGraphicsLayer()->platformCALayerDeviceScaleFactor();
}

bool TileController::platformCALayerShowDebugBorders() const
{
    return owningGraphicsLayer()->platformCALayerShowDebugBorders();
}

bool TileController::platformCALayerShowRepaintCounter(PlatformCALayer*) const
{
    return owningGraphicsLayer()->platformCALayerShowRepaintCounter(0);
}

void TileController::setScale(float scale)
{
    ASSERT(owningGraphicsLayer()->isCommittingChanges());

    float deviceScaleFactor = owningGraphicsLayer()->platformCALayerDeviceScaleFactor();

    // The scale we get is the product of the page scale factor and device scale factor.
    // Divide by the device scale factor so we'll get the page scale factor.
    scale /= deviceScaleFactor;

    if (m_scale == scale && m_deviceScaleFactor == deviceScaleFactor)
        return;

    m_deviceScaleFactor = deviceScaleFactor;
    m_scale = scale;

    TransformationMatrix transform;
    transform.scale(1 / m_scale);
    m_tileContainerLayer->setTransform(transform);

    // FIXME: we may revalidateTiles twice in this commit.
    revalidateTiles(PruneSecondaryTiles, PruneSecondaryTiles);

    for (TileMap::const_iterator it = m_tiles.begin(), end = m_tiles.end(); it != end; ++it)
        it->value.layer->setContentsScale(deviceScaleFactor);
}

void TileController::setAcceleratesDrawing(bool acceleratesDrawing)
{
    if (m_acceleratesDrawing == acceleratesDrawing)
        return;

    m_acceleratesDrawing = acceleratesDrawing;

    for (TileMap::const_iterator it = m_tiles.begin(), end = m_tiles.end(); it != end; ++it) {
        const TileInfo& tileInfo = it->value;
        tileInfo.layer->setAcceleratesDrawing(m_acceleratesDrawing);
    }
}

void TileController::setTilesOpaque(bool opaque)
{
    if (opaque == m_tilesAreOpaque)
        return;

    m_tilesAreOpaque = opaque;

    for (TileMap::iterator it = m_tiles.begin(), end = m_tiles.end(); it != end; ++it) {
        const TileInfo& tileInfo = it->value;
        tileInfo.layer->setOpaque(opaque);
    }
}

void TileController::setVisibleRect(const FloatRect& visibleRect)
{
    ASSERT(owningGraphicsLayer()->isCommittingChanges());
    if (m_visibleRect == visibleRect)
        return;

    m_visibleRect = visibleRect;
    setNeedsRevalidateTiles();
}

bool TileController::tilesWouldChangeForVisibleRect(const FloatRect& newVisibleRect) const
{
    FloatRect visibleRect = newVisibleRect;

    if (m_clipsToExposedRect)
        visibleRect.intersect(m_exposedRect);

    if (visibleRect.isEmpty() || bounds().isEmpty())
        return false;
        
    FloatRect currentTileCoverageRect = computeTileCoverageRect(m_visibleRect, newVisibleRect);
    FloatRect scaledRect(currentTileCoverageRect);
    scaledRect.scale(m_scale);
    IntRect currentCoverageRectInTileCoords(enclosingIntRect(scaledRect));

    IntSize newTileSize = tileSizeForCoverageRect(currentTileCoverageRect);
    bool tileSizeChanged = newTileSize != m_tileSize;
    if (tileSizeChanged)
        return true;

    TileIndex topLeft;
    TileIndex bottomRight;
    getTileIndexRangeForRect(currentCoverageRectInTileCoords, topLeft, bottomRight);

    IntRect coverageRect = rectForTileIndex(topLeft);
    coverageRect.unite(rectForTileIndex(bottomRight));
    return coverageRect != m_primaryTileCoverageRect;
}

void TileController::setExposedRect(const FloatRect& exposedRect)
{
    if (m_exposedRect == exposedRect)
        return;

    m_exposedRect = exposedRect;
    setNeedsRevalidateTiles();
}

void TileController::setClipsToExposedRect(bool clipsToExposedRect)
{
    if (m_clipsToExposedRect == clipsToExposedRect)
        return;

    m_clipsToExposedRect = clipsToExposedRect;

    // Going from not clipping to clipping, we don't need to revalidate right away.
    if (clipsToExposedRect)
        setNeedsRevalidateTiles();
}

void TileController::prepopulateRect(const FloatRect& rect)
{
    FloatRect scaledRect(rect);
    scaledRect.scale(m_scale);
    IntRect rectInTileCoords(enclosingIntRect(scaledRect));

    if (m_primaryTileCoverageRect.contains(rectInTileCoords))
        return;
    
    m_secondaryTileCoverageRects.append(rect);
    setNeedsRevalidateTiles();
}

void TileController::setIsInWindow(bool isInWindow)
{
    if (m_isInWindow == isInWindow)
        return;

    m_isInWindow = isInWindow;

    if (m_isInWindow)
        setNeedsRevalidateTiles();
    else {
        const double tileRevalidationTimeout = 4;
        scheduleTileRevalidation(tileRevalidationTimeout);
    }
}

void TileController::setTileCoverage(TileCoverage coverage)
{
    if (coverage == m_tileCoverage)
        return;

    m_tileCoverage = coverage;
    setNeedsRevalidateTiles();
}

void TileController::revalidateTiles()
{
    ASSERT(owningGraphicsLayer()->isCommittingChanges());
    revalidateTiles(0, 0);
}

void TileController::forceRepaint()
{
    setNeedsDisplay();
}

void TileController::setTileDebugBorderWidth(float borderWidth)
{
    if (m_tileDebugBorderWidth == borderWidth)
        return;

    m_tileDebugBorderWidth = borderWidth;
    for (TileMap::const_iterator it = m_tiles.begin(), end = m_tiles.end(); it != end; ++it) {
        const TileInfo& tileInfo = it->value;
        tileInfo.layer->setBorderWidth(m_tileDebugBorderWidth);
    }
}

void TileController::setTileDebugBorderColor(Color borderColor)
{
    if (m_tileDebugBorderColor == borderColor)
        return;

    m_tileDebugBorderColor = borderColor;
    for (TileMap::const_iterator it = m_tiles.begin(), end = m_tiles.end(); it != end; ++it)
        it->value.layer->setBorderColor(borderColor);
}

IntRect TileController::bounds() const
{
    return IntRect(IntPoint(), expandedIntSize(m_tileCacheLayer->bounds().size()));
}

IntRect TileController::rectForTileIndex(const TileIndex& tileIndex) const
{
    IntRect rect(tileIndex.x() * m_tileSize.width(), tileIndex.y() * m_tileSize.height(), m_tileSize.width(), m_tileSize.height());
    IntRect scaledBounds(bounds());
    scaledBounds.scale(m_scale);

    rect.intersect(scaledBounds);

    return rect;
}

void TileController::getTileIndexRangeForRect(const IntRect& rect, TileIndex& topLeft, TileIndex& bottomRight) const
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

FloatRect TileController::computeTileCoverageRect(const FloatRect& previousVisibleRect, const FloatRect& currentVisibleRect) const
{
    FloatRect visibleRect = currentVisibleRect;

    if (m_clipsToExposedRect)
        visibleRect.intersect(m_exposedRect);

    // If the page is not in a window (for example if it's in a background tab), we limit the tile coverage rect to the visible rect.
    // Furthermore, if the page can't have scrollbars (for example if its body element has overflow:hidden) it's very unlikely that the
    // page will ever be scrolled so we limit the tile coverage rect as well.
    if (!m_isInWindow || m_tileCoverage & CoverageForSlowScrolling)
        return visibleRect;

    bool largeVisibleRectChange = !previousVisibleRect.isEmpty() && !visibleRect.intersects(previousVisibleRect);
    
    // FIXME: look at how far the document can scroll in each dimension.
    float coverageHorizontalSize = visibleRect.width();
    float coverageVerticalSize = visibleRect.height();
    
    // Inflate the coverage rect so that it covers 2x of the visible width and 3x of the visible height.
    // These values were chosen because it's more common to have tall pages and to scroll vertically,
    // so we keep more tiles above and below the current area.
    if (m_tileCoverage & CoverageForHorizontalScrolling && !largeVisibleRectChange)
        coverageHorizontalSize *= 2;

    if (m_tileCoverage & CoverageForVerticalScrolling && !largeVisibleRectChange)
        coverageVerticalSize *= 3;

    // Don't extend coverage before 0 or after the end.
    FloatRect coverageBounds = bounds();
    float coverageLeft = visibleRect.x() - (coverageHorizontalSize - visibleRect.width()) / 2;
    coverageLeft = min(coverageLeft, coverageBounds.maxX() - coverageHorizontalSize);
    coverageLeft = max(coverageLeft, coverageBounds.x());

    float coverageTop = visibleRect.y() - (coverageVerticalSize - visibleRect.height()) / 2;
    coverageTop = min(coverageTop, coverageBounds.maxY() - coverageVerticalSize);
    coverageTop = max(coverageTop, coverageBounds.y());

    return FloatRect(coverageLeft, coverageTop, coverageHorizontalSize, coverageVerticalSize);
}

IntSize TileController::tileSizeForCoverageRect(const FloatRect& coverageRect) const
{
    if (m_tileCoverage & CoverageForSlowScrolling) {
        FloatSize tileSize = coverageRect.size();
        tileSize.scale(m_scale);
        return expandedIntSize(tileSize);
    }

    return IntSize(defaultTileWidth, defaultTileHeight);
}

void TileController::scheduleTileRevalidation(double interval)
{
    if (m_tileRevalidationTimer.isActive() && m_tileRevalidationTimer.nextFireInterval() < interval)
        return;

    m_tileRevalidationTimer.startOneShot(interval);
}

void TileController::tileRevalidationTimerFired(Timer<TileController>*)
{
    if (m_isInWindow) {
        setNeedsRevalidateTiles();
        return;
    }

    TileValidationPolicyFlags foregroundValidationPolicy = m_aggressivelyRetainsTiles ? 0 : PruneSecondaryTiles;
    TileValidationPolicyFlags backgroundValidationPolicy = foregroundValidationPolicy | UnparentAllTiles;

    revalidateTiles(foregroundValidationPolicy, backgroundValidationPolicy);
}

unsigned TileController::blankPixelCount() const
{
    PlatformLayerList tiles(m_tiles.size());

    for (TileMap::const_iterator it = m_tiles.begin(), end = m_tiles.end(); it != end; ++it) {
        if (PlatformLayer *layer = it->value.layer->platformLayer())
            tiles.append(layer);
    }

    return blankPixelCountForTiles(tiles, m_visibleRect, IntPoint(0,0));
}

unsigned TileController::blankPixelCountForTiles(const PlatformLayerList& tiles, const FloatRect& visibleRect, const IntPoint& tileTranslation)
{
    Region paintedVisibleTiles;

    for (PlatformLayerList::const_iterator it = tiles.begin(), end = tiles.end(); it != end; ++it) {
        const PlatformLayer* tileLayer = it->get();

        FloatRect visiblePart(CGRectOffset([tileLayer frame], tileTranslation.x(), tileTranslation.y()));
        visiblePart.intersect(visibleRect);

        if (!visiblePart.isEmpty())
            paintedVisibleTiles.unite(enclosingIntRect(visiblePart));
    }

    Region uncoveredRegion(enclosingIntRect(visibleRect));
    uncoveredRegion.subtract(paintedVisibleTiles);

    return uncoveredRegion.totalArea();
}

static inline void queueTileForRemoval(const TileController::TileIndex& tileIndex, const TileController::TileInfo& tileInfo, Vector<TileController::TileIndex>& tilesToRemove, TileController::RepaintCountMap& repaintCounts)
{
    tileInfo.layer->removeFromSuperlayer();
    repaintCounts.remove(tileInfo.layer.get());
    tilesToRemove.append(tileIndex);
}

void TileController::removeAllTiles()
{
    Vector<TileIndex> tilesToRemove;

    for (TileMap::iterator it = m_tiles.begin(), end = m_tiles.end(); it != end; ++it)
        queueTileForRemoval(it->key, it->value, tilesToRemove, m_tileRepaintCounts);

    for (size_t i = 0; i < tilesToRemove.size(); ++i) {
        TileInfo tileInfo = m_tiles.take(tilesToRemove[i]);
        LayerPool::sharedPool()->addLayer(tileInfo.layer);
    }
}

void TileController::removeAllSecondaryTiles()
{
    Vector<TileIndex> tilesToRemove;

    for (TileMap::iterator it = m_tiles.begin(), end = m_tiles.end(); it != end; ++it) {
        const TileInfo& tileInfo = it->value;
        if (tileInfo.cohort == VisibleTileCohort)
            continue;

        queueTileForRemoval(it->key, it->value, tilesToRemove, m_tileRepaintCounts);
    }

    for (size_t i = 0; i < tilesToRemove.size(); ++i) {
        TileInfo tileInfo = m_tiles.take(tilesToRemove[i]);
        LayerPool::sharedPool()->addLayer(tileInfo.layer);
    }
}

void TileController::removeTilesInCohort(TileCohort cohort)
{
    ASSERT(cohort != VisibleTileCohort);
    Vector<TileIndex> tilesToRemove;

    for (TileMap::iterator it = m_tiles.begin(), end = m_tiles.end(); it != end; ++it) {
        const TileInfo& tileInfo = it->value;
        if (tileInfo.cohort != cohort)
            continue;

        queueTileForRemoval(it->key, it->value, tilesToRemove, m_tileRepaintCounts);
    }

    for (size_t i = 0; i < tilesToRemove.size(); ++i) {
        TileInfo tileInfo = m_tiles.take(tilesToRemove[i]);
        LayerPool::sharedPool()->addLayer(tileInfo.layer);
    }
}

void TileController::setNeedsRevalidateTiles()
{
    owningGraphicsLayer()->platformCALayerSetNeedsToRevalidateTiles();
}

void TileController::revalidateTiles(TileValidationPolicyFlags foregroundValidationPolicy, TileValidationPolicyFlags backgroundValidationPolicy)
{
    FloatRect visibleRect = m_visibleRect;
    IntRect bounds = this->bounds();

    if (m_clipsToExposedRect)
        visibleRect.intersect(m_exposedRect);

    if (visibleRect.isEmpty() || bounds.isEmpty())
        return;
    
    TileValidationPolicyFlags validationPolicy = m_isInWindow ? foregroundValidationPolicy : backgroundValidationPolicy;
    
    FloatRect tileCoverageRect = computeTileCoverageRect(m_visibleRectAtLastRevalidate, m_visibleRect);
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

            PlatformCALayer* tileLayer = tileInfo.layer.get();
            IntRect tileRect = rectForTileIndex(tileIndex);
            if (tileRect.intersects(coverageRectInTileCoords)) {
                tileInfo.cohort = VisibleTileCohort;
                if (tileInfo.hasStaleContent) {
                    // FIXME: store a dirty region per layer?
                    tileLayer->setNeedsDisplay();
                    tileInfo.hasStaleContent = false;
                }
            } else {
                // Add to the currentCohort if not already in one.
                if (tileInfo.cohort == VisibleTileCohort) {
                    tileInfo.cohort = currCohort;
                    ++tilesInCohort;
                    
                    if (m_unparentsOffscreenTiles)
                        tileLayer->removeFromSuperlayer();
                }
            }
        }
        
        if (tilesInCohort)
            startedNewCohort(currCohort);

        if (!m_aggressivelyRetainsTiles)
            scheduleCohortRemoval();
    }

    // Ensure primary tile coverage tiles.
    m_primaryTileCoverageRect = ensureTilesForRect(tileCoverageRect, CoverageType::PrimaryTiles);

    if (validationPolicy & PruneSecondaryTiles) {
        removeAllSecondaryTiles();
        m_cohortList.clear();
    } else {
        for (size_t i = 0; i < m_secondaryTileCoverageRects.size(); ++i)
            ensureTilesForRect(m_secondaryTileCoverageRects[i], CoverageType::SecondaryTiles);
        m_secondaryTileCoverageRects.clear();
    }

    if (m_unparentsOffscreenTiles && (validationPolicy & UnparentAllTiles)) {
        for (TileMap::iterator it = m_tiles.begin(), end = m_tiles.end(); it != end; ++it)
            it->value.layer->removeFromSuperlayer();
    }

    if (m_boundsAtLastRevalidate != bounds) {
        FloatRect scaledBounds(bounds);
        scaledBounds.scale(m_scale);
        IntRect boundsInTileCoords(enclosingIntRect(scaledBounds));

        TileIndex topLeftForBounds;
        TileIndex bottomRightForBounds;
        getTileIndexRangeForRect(boundsInTileCoords, topLeftForBounds, bottomRightForBounds);

        Vector<TileIndex> tilesToRemove;
        for (TileMap::iterator it = m_tiles.begin(), end = m_tiles.end(); it != end; ++it) {
            const TileIndex& index = it->key;
            if (index.y() < topLeftForBounds.y()
                || index.y() > bottomRightForBounds.y()
                || index.x() < topLeftForBounds.x()
                || index.x() > bottomRightForBounds.x())
                queueTileForRemoval(index, it->value, tilesToRemove, m_tileRepaintCounts);
        }

        for (size_t i = 0, size = tilesToRemove.size(); i < size; ++i) {
            TileInfo tileInfo = m_tiles.take(tilesToRemove[i]);
            LayerPool::sharedPool()->addLayer(tileInfo.layer);
        }
    }

    if (m_tiledScrollingIndicatorLayer)
        updateTileCoverageMap();

    m_visibleRectAtLastRevalidate = visibleRect;
    m_boundsAtLastRevalidate = bounds;
}

TileController::TileCohort TileController::nextTileCohort() const
{
    if (!m_cohortList.isEmpty())
        return m_cohortList.last().cohort + 1;

    return 1;
}

void TileController::startedNewCohort(TileCohort cohort)
{
    m_cohortList.append(TileCohortInfo(cohort, monotonicallyIncreasingTime()));
}

TileController::TileCohort TileController::newestTileCohort() const
{
    return m_cohortList.isEmpty() ? 0 : m_cohortList.last().cohort;
}

TileController::TileCohort TileController::oldestTileCohort() const
{
    return m_cohortList.isEmpty() ? 0 : m_cohortList.first().cohort;
}

void TileController::scheduleCohortRemoval()
{
    const double cohortRemovalTimerSeconds = 1;

    // Start the timer, or reschedule the timer from now if it's already active.
    if (!m_cohortRemovalTimer.isActive())
        m_cohortRemovalTimer.startRepeating(cohortRemovalTimerSeconds);
}

void TileController::cohortRemovalTimerFired(Timer<TileController>*)
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

IntRect TileController::ensureTilesForRect(const FloatRect& rect, CoverageType newTileType)
{
    if (m_unparentsOffscreenTiles && !m_isInWindow)
        return IntRect();

    FloatRect scaledRect(rect);
    scaledRect.scale(m_scale);
    IntRect rectInTileCoords(enclosingIntRect(scaledRect));

    TileIndex topLeft;
    TileIndex bottomRight;
    getTileIndexRangeForRect(rectInTileCoords, topLeft, bottomRight);

    TileCohort currCohort = nextTileCohort();
    unsigned tilesInCohort = 0;

    IntRect coverageRect;

    for (int y = topLeft.y(); y <= bottomRight.y(); ++y) {
        for (int x = topLeft.x(); x <= bottomRight.x(); ++x) {
            TileIndex tileIndex(x, y);

            IntRect tileRect = rectForTileIndex(tileIndex);
            TileInfo& tileInfo = m_tiles.add(tileIndex, TileInfo()).iterator->value;

            coverageRect.unite(tileRect);

            bool shouldChangeTileLayerFrame = false;

            if (!tileInfo.layer)
                tileInfo.layer = createTileLayer(tileRect);
            else {
                // We already have a layer for this tile. Ensure that its size is correct.
                FloatSize tileLayerSize(tileInfo.layer->bounds().size());
                shouldChangeTileLayerFrame = tileLayerSize != FloatSize(tileRect.size());

                if (shouldChangeTileLayerFrame) {
                    tileInfo.layer->setBounds(FloatRect(FloatPoint(), tileRect.size()));
                    tileInfo.layer->setPosition(tileRect.location());
                }
            }

            if (newTileType == CoverageType::SecondaryTiles && !tileRect.intersects(m_primaryTileCoverageRect)) {
                tileInfo.cohort = currCohort;
                ++tilesInCohort;
            }

            bool shouldParentTileLayer = (!m_unparentsOffscreenTiles || m_isInWindow) && !tileInfo.layer->superlayer();

            if (shouldParentTileLayer)
                m_tileContainerLayer->appendSublayer(tileInfo.layer.get());
        }
    }
    
    if (tilesInCohort)
        startedNewCohort(currCohort);

    return coverageRect;
}

void TileController::updateTileCoverageMap()
{
    FloatRect containerBounds = bounds();
    FloatRect visibleRect = this->visibleRect();

    if (m_clipsToExposedRect)
        visibleRect.intersect(m_exposedRect);

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

    if (m_clipsToExposedRect)
        m_tiledScrollingIndicatorLayer->setPosition(m_exposedRect.location() + FloatPoint(2, 2));
    else
        m_tiledScrollingIndicatorLayer->setPosition(FloatPoint(2, 2));

    m_tiledScrollingIndicatorLayer->setBounds(mapBounds);
    m_tiledScrollingIndicatorLayer->setNeedsDisplay();

    visibleRect.scale(indicatorScale, indicatorScale);
    visibleRect.expand(2, 2);
    m_visibleRectIndicatorLayer->setPosition(visibleRect.location());
    m_visibleRectIndicatorLayer->setBounds(FloatRect(FloatPoint(), visibleRect.size()));

    Color visibleRectIndicatorColor;
    switch (m_indicatorMode) {
    case MainThreadScrollingBecauseOfStyleIndication:
        visibleRectIndicatorColor = Color(255, 0, 0);
        break;
    case MainThreadScrollingBecauseOfEventHandlersIndication:
        visibleRectIndicatorColor = Color(255, 255, 0);
        break;
    case ThreadedScrollingIndication:
        visibleRectIndicatorColor = Color(0, 200, 0);
        break;
    }

    m_visibleRectIndicatorLayer->setBorderColor(visibleRectIndicatorColor);
}

IntRect TileController::tileGridExtent() const
{
    TileIndex topLeft;
    TileIndex bottomRight;
    getTileIndexRangeForRect(m_primaryTileCoverageRect, topLeft, bottomRight);

    // Return index of top, left tile and the number of tiles across and down.
    return IntRect(topLeft.x(), topLeft.y(), bottomRight.x() - topLeft.x() + 1, bottomRight.y() - topLeft.y() + 1);
}

double TileController::retainedTileBackingStoreMemory() const
{
    double totalBytes = 0;
    
    for (TileMap::const_iterator it = m_tiles.begin(), end = m_tiles.end(); it != end; ++it) {
        const TileInfo& tileInfo = it->value;
        if (tileInfo.layer->superlayer()) {
            FloatRect bounds = tileInfo.layer->bounds();
            double contentsScale = tileInfo.layer->contentsScale();
            totalBytes += 4 * bounds.width() * contentsScale * bounds.height() * contentsScale;
        }
    }

    return totalBytes;
}

// Return the rect in layer coords, not tile coords.
IntRect TileController::tileCoverageRect() const
{
    IntRect coverageRectInLayerCoords(m_primaryTileCoverageRect);
    coverageRectInLayerCoords.scale(1 / m_scale);
    return coverageRectInLayerCoords;
}

PlatformCALayer* TileController::tiledScrollingIndicatorLayer()
{
    if (!m_tiledScrollingIndicatorLayer) {
        m_tiledScrollingIndicatorLayer = m_tileCacheLayer->createCompatibleLayer(PlatformCALayer::LayerTypeSimpleLayer, this);
        m_tiledScrollingIndicatorLayer->setOpacity(0.75);
        m_tiledScrollingIndicatorLayer->setAnchorPoint(FloatPoint3D());
        m_tiledScrollingIndicatorLayer->setBorderColor(Color::black);
        m_tiledScrollingIndicatorLayer->setBorderWidth(1);
        m_tiledScrollingIndicatorLayer->setPosition(FloatPoint(2, 2));

        m_visibleRectIndicatorLayer = m_tileCacheLayer->createCompatibleLayer(PlatformCALayer::LayerTypeLayer, nullptr);
        m_visibleRectIndicatorLayer->setBorderWidth(2);
        m_visibleRectIndicatorLayer->setAnchorPoint(FloatPoint3D());
        m_visibleRectIndicatorLayer->setBorderColor(Color(255, 0, 0));

        m_tiledScrollingIndicatorLayer->appendSublayer(m_visibleRectIndicatorLayer.get());

        updateTileCoverageMap();
    }

    return m_tiledScrollingIndicatorLayer.get();
}

void TileController::setScrollingModeIndication(ScrollingModeIndication scrollingMode)
{
    if (scrollingMode == m_indicatorMode)
        return;

    m_indicatorMode = scrollingMode;

    if (m_tiledScrollingIndicatorLayer)
        updateTileCoverageMap();
}

RefPtr<PlatformCALayer> TileController::createTileLayer(const IntRect& tileRect)
{
    RefPtr<PlatformCALayer> layer = LayerPool::sharedPool()->takeLayerWithSize(tileRect.size());

    if (layer) {
        m_tileRepaintCounts.remove(layer.get());
        layer->setOwner(this);
    } else
        layer = m_tileCacheLayer->createCompatibleLayer(PlatformCALayer::LayerTypeTiledBackingTileLayer, this);

    layer->setAnchorPoint(FloatPoint3D());
    layer->setBounds(FloatRect(FloatPoint(), tileRect.size()));
    layer->setPosition(tileRect.location());
    layer->setBorderColor(m_tileDebugBorderColor);
    layer->setBorderWidth(m_tileDebugBorderWidth);
    layer->setEdgeAntialiasingMask(0);
    layer->setOpaque(m_tilesAreOpaque);
#ifndef NDEBUG
    layer->setName("Tile");
#endif

    layer->setContentsScale(m_deviceScaleFactor);
    layer->setAcceleratesDrawing(m_acceleratesDrawing);

    layer->setNeedsDisplay();

    return layer;
}

int TileController::platformCALayerIncrementRepaintCount(PlatformCALayer* platformCALayer)
{
    int repaintCount = 0;

    if (m_tileRepaintCounts.contains(platformCALayer))
        repaintCount = m_tileRepaintCounts.get(platformCALayer);

    m_tileRepaintCounts.set(platformCALayer, ++repaintCount);

    return repaintCount;
}

void TileController::drawTileMapContents(CGContextRef context, CGRect layerBounds)
{
    CGContextSetRGBFillColor(context, 0.3, 0.3, 0.3, 1);
    CGContextFillRect(context, layerBounds);

    CGFloat scaleFactor = layerBounds.size.width / bounds().width();

    CGFloat contextScale = scaleFactor / scale();
    CGContextScaleCTM(context, contextScale, contextScale);
    
    for (TileMap::const_iterator it = m_tiles.begin(), end = m_tiles.end(); it != end; ++it) {
        const TileInfo& tileInfo = it->value;
        PlatformCALayer* tileLayer = tileInfo.layer.get();

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

        if (!m_aggressivelyRetainsTiles && tileInfo.cohort != VisibleTileCohort && newestCohort > oldestCohort) {
            float cohortProportion = static_cast<float>((newestCohort - tileInfo.cohort)) / (newestCohort - oldestCohort);
            CGContextSetRGBFillColor(context, red, green, blue, 1 - cohortProportion);
        } else
            CGContextSetRGBFillColor(context, red, green, blue, 1);

        if (tileLayer->superlayer()) {
            CGContextSetLineWidth(context, 0.5 / contextScale);
            CGContextSetRGBStrokeColor(context, 0, 0, 0, 1);
        } else {
            CGContextSetLineWidth(context, 1 / contextScale);
            CGContextSetRGBStrokeColor(context, 0.2, 0.1, 0.9, 1);
        }

        CGRect frame = CGRectMake(tileLayer->position().x(), tileLayer->position().y(), tileLayer->bounds().size().width(), tileLayer->bounds().size().height());
        CGContextFillRect(context, frame);
        CGContextStrokeRect(context, frame);
    }
}
    

} // namespace WebCore
