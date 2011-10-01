/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#include "config.h"

#if USE(ACCELERATED_COMPOSITING)

#include "TiledLayerChromium.h"

#include "GraphicsContext3D.h"
#include "LayerRendererChromium.h"
#include "ManagedTexture.h"
#include "TextStream.h"
#include "cc/CCLayerImpl.h"
#include "cc/CCTiledLayerImpl.h"
#include <wtf/CurrentTime.h>

// Start tiling when the width and height of a layer are larger than this size.
static int maxUntiledSize = 512;

// When tiling is enabled, use tiles of this dimension squared.
static int defaultTileSize = 256;

using namespace std;

namespace WebCore {

class UpdatableTile : public CCLayerTilingData::Tile {
    WTF_MAKE_NONCOPYABLE(UpdatableTile);
public:
    explicit UpdatableTile(PassOwnPtr<ManagedTexture> tex) : m_tex(tex) { }

    ManagedTexture* texture() { return m_tex.get(); }

    bool dirty() const { return !m_dirtyLayerRect.isEmpty(); }
    void clearDirty() { m_dirtyLayerRect = IntRect(); }

    // Layer-space dirty rectangle that needs to be repainted.
    IntRect m_dirtyLayerRect;
private:
    OwnPtr<ManagedTexture> m_tex;
};

TiledLayerChromium::TiledLayerChromium(CCLayerDelegate* delegate)
    : LayerChromium(delegate)
    , m_tilingOption(AutoTile)
    , m_textureFormat(GraphicsContext3D::INVALID_ENUM)
    , m_skipsDraw(false)
    , m_textureOrientation(LayerTextureUpdater::InvalidOrientation)
    , m_sampledTexelFormat(LayerTextureUpdater::SampledTexelFormatInvalid)
{
}

TiledLayerChromium::~TiledLayerChromium()
{
}

PassRefPtr<CCLayerImpl> TiledLayerChromium::createCCLayerImpl()
{
    return CCTiledLayerImpl::create(id());
}

void TiledLayerChromium::cleanupResources()
{
    LayerChromium::cleanupResources();

    m_tiler.clear();
    m_unusedTiles.clear();
    m_paintRect = IntRect();
    m_updateRect = IntRect();
}

void TiledLayerChromium::updateTileSizeAndTilingOption()
{
    if (!m_tiler)
        return;

    const IntSize tileSize(min(defaultTileSize, contentBounds().width()), min(defaultTileSize, contentBounds().height()));

    // Tile if both dimensions large, or any one dimension large and the other
    // extends into a second tile. This heuristic allows for long skinny layers
    // (e.g. scrollbars) that are Nx1 tiles to minimize wasted texture space.
    const bool anyDimensionLarge = contentBounds().width() > maxUntiledSize || contentBounds().height() > maxUntiledSize;
    const bool anyDimensionOneTile = contentBounds().width() <= defaultTileSize || contentBounds().height() <= defaultTileSize;
    const bool autoTiled = anyDimensionLarge && !anyDimensionOneTile;

    bool isTiled;
    if (m_tilingOption == AlwaysTile)
        isTiled = true;
    else if (m_tilingOption == NeverTile)
        isTiled = false;
    else
        isTiled = autoTiled;

    IntSize requestedSize = isTiled ? tileSize : contentBounds();
    const int maxSize = layerTreeHost()->layerRendererCapabilities().maxTextureSize;
    IntSize clampedSize = requestedSize.shrunkTo(IntSize(maxSize, maxSize));
    m_tiler->setTileSize(clampedSize);
}

bool TiledLayerChromium::drawsContent() const
{
    if (!m_delegate)
        return false;

    if (!m_tiler)
        return true;

    if (m_tilingOption == NeverTile && m_tiler->numTiles() > 1)
        return false;

    return !m_skipsDraw;
}

void TiledLayerChromium::setLayerTreeHost(CCLayerTreeHost* host)
{
    LayerChromium::setLayerTreeHost(host);

    if (m_tiler || !host)
        return;

    createTextureUpdater(host);

    m_textureFormat = host->layerRendererCapabilities().bestTextureFormat;
    m_textureOrientation = textureUpdater()->orientation();
    m_sampledTexelFormat = textureUpdater()->sampledTexelFormat(m_textureFormat);
    m_tiler = CCLayerTilingData::create(
        IntSize(defaultTileSize, defaultTileSize),
        isNonCompositedContent() ? CCLayerTilingData::NoBorderTexels : CCLayerTilingData::HasBorderTexels);
}

void TiledLayerChromium::updateCompositorResources(GraphicsContext3D* context, TextureAllocator* allocator)
{
    // Painting could cause compositing to get turned off, which may cause the tiler to become invalidated mid-update.
    if (m_skipsDraw || m_updateRect.isEmpty() || !m_tiler->numTiles())
        return;

    int left, top, right, bottom;
    m_tiler->contentRectToTileIndices(m_updateRect, left, top, right, bottom);
    for (int j = top; j <= bottom; ++j) {
        for (int i = left; i <= right; ++i) {
            UpdatableTile* tile = tileAt(i, j);
            if (!tile)
                tile = createTile(i, j);
            else if (!tile->dirty())
                continue;

            // Calculate page-space rectangle to copy from.
            IntRect sourceRect = m_tiler->tileContentRect(tile);
            const IntPoint anchor = sourceRect.location();
            sourceRect.intersect(m_tiler->layerRectToContentRect(tile->m_dirtyLayerRect));
            // Paint rect not guaranteed to line up on tile boundaries, so
            // make sure that sourceRect doesn't extend outside of it.
            sourceRect.intersect(m_paintRect);
            if (sourceRect.isEmpty())
                continue;

            ASSERT(tile->texture()->isReserved());

            // Calculate tile-space rectangle to upload into.
            IntRect destRect(IntPoint(sourceRect.x() - anchor.x(), sourceRect.y() - anchor.y()), sourceRect.size());
            if (destRect.x() < 0)
                CRASH();
            if (destRect.y() < 0)
                CRASH();

            // Offset from paint rectangle to this tile's dirty rectangle.
            IntPoint paintOffset(sourceRect.x() - m_paintRect.x(), sourceRect.y() - m_paintRect.y());
            if (paintOffset.x() < 0)
                CRASH();
            if (paintOffset.y() < 0)
                CRASH();
            if (paintOffset.x() + destRect.width() > m_paintRect.width())
                CRASH();
            if (paintOffset.y() + destRect.height() > m_paintRect.height())
                CRASH();

            tile->texture()->bindTexture(context, allocator);
            const GC3Dint filter = m_tiler->hasBorderTexels() ? GraphicsContext3D::LINEAR : GraphicsContext3D::NEAREST;
            GLC(context, context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MIN_FILTER, filter));
            GLC(context, context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MAG_FILTER, filter));
            GLC(context, context->bindTexture(GraphicsContext3D::TEXTURE_2D, 0));

            textureUpdater()->updateTextureRect(context, allocator, tile->texture(), sourceRect, destRect);
            tile->clearDirty();
        }
    }
}

void TiledLayerChromium::setTilingOption(TilingOption tilingOption)
{
    m_tilingOption = tilingOption;
    updateTileSizeAndTilingOption();
}

void TiledLayerChromium::setIsMask(bool isMask)
{
    setTilingOption(isMask ? NeverTile : AutoTile);
}

void TiledLayerChromium::pushPropertiesTo(CCLayerImpl* layer)
{
    LayerChromium::pushPropertiesTo(layer);

    CCTiledLayerImpl* tiledLayer = static_cast<CCTiledLayerImpl*>(layer);
    if (!m_tiler) {
        tiledLayer->setSkipsDraw(true);
        return;
    }

    tiledLayer->setSkipsDraw(m_skipsDraw);
    tiledLayer->setTextureOrientation(m_textureOrientation);
    tiledLayer->setSampledTexelFormat(m_sampledTexelFormat);
    tiledLayer->setTilingData(*m_tiler);

    for (CCLayerTilingData::TileMap::const_iterator iter = m_tiler->tiles().begin(); iter != m_tiler->tiles().end(); ++iter) {
        int i = iter->first.first;
        int j = iter->first.second;
        UpdatableTile* tile = static_cast<UpdatableTile*>(iter->second.get());
        if (!tile->texture()->isValid(m_tiler->tileSize(), m_textureFormat))
            continue;

        tiledLayer->syncTextureId(i, j, tile->texture()->textureId());
    }
}

TextureManager* TiledLayerChromium::textureManager() const
{
    if (!layerTreeHost())
        return 0;
    return layerTreeHost()->contentsTextureManager();
}

UpdatableTile* TiledLayerChromium::tileAt(int i, int j) const
{
    return static_cast<UpdatableTile*>(m_tiler->tileAt(i, j));
}

UpdatableTile* TiledLayerChromium::createTile(int i, int j)
{
    RefPtr<UpdatableTile> tile;
    if (m_unusedTiles.size() > 0) {
        tile = m_unusedTiles.last().release();
        m_unusedTiles.removeLast();
        ASSERT(tile->refCount() == 1);
    } else {
        TextureManager* manager = textureManager();
        tile = adoptRef(new UpdatableTile(ManagedTexture::create(manager)));
    }
    m_tiler->addTile(tile, i, j);
    tile->m_dirtyLayerRect = m_tiler->tileLayerRect(tile.get());

    return tile.get();
}

void TiledLayerChromium::invalidateTiles(const IntRect& contentRect)
{
    if (!m_tiler->numTiles())
        return;

    Vector<CCLayerTilingData::TileMapKey> removeKeys;
    for (CCLayerTilingData::TileMap::const_iterator iter = m_tiler->tiles().begin(); iter != m_tiler->tiles().end(); ++iter) {
        CCLayerTilingData::Tile* tile = iter->second.get();
        IntRect tileRect = m_tiler->tileContentRect(tile);
        if (tileRect.intersects(contentRect))
            continue;
        removeKeys.append(iter->first);
    }

    for (size_t i = 0; i < removeKeys.size(); ++i)
        m_unusedTiles.append(static_pointer_cast<UpdatableTile>(m_tiler->takeTile(removeKeys[i].first, removeKeys[i].second)));
}

void TiledLayerChromium::invalidateRect(const IntRect& contentRect)
{
    if (!m_tiler || contentRect.isEmpty() || m_skipsDraw)
        return;

    m_tiler->growLayerToContain(contentRect);

    // Dirty rects are always in layer space, as the layer could be repositioned
    // after invalidation.
    const IntRect layerRect = m_tiler->contentRectToLayerRect(contentRect);

    int left, top, right, bottom;
    m_tiler->contentRectToTileIndices(contentRect, left, top, right, bottom);
    for (int j = top; j <= bottom; ++j) {
        for (int i = left; i <= right; ++i) {
            UpdatableTile* tile = tileAt(i, j);
            if (!tile)
                continue;
            IntRect bound = m_tiler->tileLayerRect(tile);
            bound.intersect(layerRect);
            tile->m_dirtyLayerRect.unite(bound);
        }
    }
}

void TiledLayerChromium::protectVisibleTileTextures()
{
    protectTileTextures(IntRect(IntPoint::zero(), contentBounds()));
}

void TiledLayerChromium::protectTileTextures(const IntRect& contentRect)
{
    if (!m_tiler || contentRect.isEmpty())
        return;

    int left, top, right, bottom;
    m_tiler->contentRectToTileIndices(contentRect, left, top, right, bottom);

    for (int j = top; j <= bottom; ++j) {
        for (int i = left; i <= right; ++i) {
            UpdatableTile* tile = tileAt(i, j);
            if (!tile || !tile->texture()->isValid(m_tiler->tileSize(), m_textureFormat))
                continue;

            tile->texture()->reserve(m_tiler->tileSize(), m_textureFormat);
        }
    }
}

void TiledLayerChromium::prepareToUpdate(const IntRect& contentRect)
{
    ASSERT(m_tiler);

    m_skipsDraw = false;

    if (contentRect.isEmpty()) {
        m_updateRect = IntRect();
        return;
    }

    // Invalidate old tiles that were previously used but aren't in use this
    // frame so that they can get reused for new tiles.
    invalidateTiles(contentRect);
    m_tiler->growLayerToContain(contentRect);

    if (!m_tiler->numTiles()) {
        m_updateRect = IntRect();
        return;
    }

    // Create tiles as needed, expanding a dirty rect to contain all
    // the dirty regions currently being drawn.
    IntRect dirtyLayerRect;
    int left, top, right, bottom;
    m_tiler->contentRectToTileIndices(contentRect, left, top, right, bottom);
    for (int j = top; j <= bottom; ++j) {
        for (int i = left; i <= right; ++i) {
            UpdatableTile* tile = tileAt(i, j);
            if (!tile)
                tile = createTile(i, j);

            if (!tile->texture()->isValid(m_tiler->tileSize(), m_textureFormat))
                tile->m_dirtyLayerRect = m_tiler->tileLayerRect(tile);

            if (!tile->texture()->reserve(m_tiler->tileSize(), m_textureFormat)) {
                m_skipsDraw = true;
                cleanupResources();
                return;
            }

            dirtyLayerRect.unite(tile->m_dirtyLayerRect);
        }
    }

    // Due to borders, when the paint rect is extended to tile boundaries, it
    // may end up overlapping more tiles than the original content rect. Record
    // that original rect so we don't upload more tiles than necessary.
    m_updateRect = contentRect;

    m_paintRect = m_tiler->layerRectToContentRect(dirtyLayerRect);
    if (dirtyLayerRect.isEmpty())
        return;

    // Calling prepareToUpdate() calls into WebKit to paint, which may have the side
    // effect of disabling compositing, which causes our reference to the texture updater to be deleted.
    // However, we can't free the memory backing the GraphicsContext until the paint finishes,
    // so we grab a local reference here to hold the updater alive until the paint completes.
    RefPtr<LayerTextureUpdater> protector(textureUpdater());
    textureUpdater()->prepareToUpdate(m_paintRect, m_tiler->tileSize(), m_tiler->hasBorderTexels());
}

}
#endif // USE(ACCELERATED_COMPOSITING)
