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

#ifndef TiledLayerChromium_h
#define TiledLayerChromium_h

#if USE(ACCELERATED_COMPOSITING)

#include "LayerChromium.h"
#include "cc/CCLayerTilingData.h"
#include "cc/CCTiledLayerImpl.h"

namespace WebCore {

class LayerTextureUpdater;
class UpdatableTile;

class TiledLayerChromium : public LayerChromium {
public:
    enum TilingOption { AlwaysTile, NeverTile, AutoTile };

    virtual ~TiledLayerChromium();

    virtual void updateCompositorResources(GraphicsContext3D*, TextureAllocator*);
    virtual void setIsMask(bool);

    virtual void pushPropertiesTo(CCLayerImpl*);

    virtual bool drawsContent() const;

    // Reserves all existing and valid tile textures to protect them from being
    // recycled by the texture manager.
    void protectTileTextures(const IntRect& contentRect);

protected:
    explicit TiledLayerChromium(CCLayerDelegate*);

    virtual void cleanupResources();
    void updateTileSizeAndTilingOption();

    virtual void createTextureUpdater(const CCLayerTreeHost*) = 0;
    virtual LayerTextureUpdater* textureUpdater() const = 0;

    // Set invalidations to be potentially repainted during update().
    void invalidateRect(const IntRect& contentRect);
    // Prepare data needed to update textures that intersect with contentRect.
    void prepareToUpdate(const IntRect& contentRect);
    // Update invalid textures that intersect with contentRect provided in prepareToUpdate().
    void updateRect(GraphicsContext3D*, LayerTextureUpdater*);
    virtual void protectVisibleTileTextures();

private:
    virtual PassRefPtr<CCLayerImpl> createCCLayerImpl();

    virtual void setLayerTreeHost(CCLayerTreeHost*);

    void createTilerIfNeeded();
    void setTilingOption(TilingOption);

    UpdatableTile* tileAt(int, int) const;
    UpdatableTile* createTile(int, int);
    void invalidateTiles(const IntRect& contentRect);

    TextureManager* textureManager() const;

    // State held between update and upload.
    IntRect m_paintRect;
    IntRect m_updateRect;

    // Tightly packed set of unused tiles.
    Vector<RefPtr<UpdatableTile> > m_unusedTiles;

    TilingOption m_tilingOption;
    GC3Denum m_textureFormat;
    bool m_skipsDraw;
    LayerTextureUpdater::Orientation m_textureOrientation;
    LayerTextureUpdater::SampledTexelFormat m_sampledTexelFormat;

    OwnPtr<CCLayerTilingData> m_tiler;
};

}
#endif // USE(ACCELERATED_COMPOSITING)

#endif
