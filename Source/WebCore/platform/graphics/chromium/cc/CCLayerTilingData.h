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


#ifndef CCLayerTilingData_h
#define CCLayerTilingData_h

#if USE(ACCELERATED_COMPOSITING)

#include "IntRect.h"
#include "Region.h"
#include "TilingData.h"
#include <wtf/HashMap.h>
#include <wtf/HashTraits.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class CCLayerTilingData {
public:
    enum BorderTexelOption { HasBorderTexels, NoBorderTexels };

    static PassOwnPtr<CCLayerTilingData> create(const IntSize& tileSize, BorderTexelOption);

    bool hasEmptyBounds() const { return m_tilingData.hasEmptyBounds(); }
    int numTilesX() const { return m_tilingData.numTilesX(); }
    int numTilesY() const { return m_tilingData.numTilesY(); }
    IntRect tileBounds(int i, int j) const { return m_tilingData.tileBounds(i, j); }
    IntPoint textureOffset(int xIndex, int yIndex) const { return m_tilingData.textureOffset(xIndex, yIndex); }

    // Change the tile size. This may invalidate all the existing tiles.
    void setTileSize(const IntSize&);
    const IntSize& tileSize() const;
    // Change the border texel setting. This may invalidate all existing tiles.
    void setBorderTexelOption(BorderTexelOption);
    bool hasBorderTexels() const { return m_tilingData.borderTexels(); }

    bool isEmpty() const { return hasEmptyBounds() || !tiles().size(); }

    const CCLayerTilingData& operator=(const CCLayerTilingData&);

    class Tile {
        WTF_MAKE_NONCOPYABLE(Tile);
    public:
        Tile() : m_i(-1), m_j(-1) { }
        virtual ~Tile() { }

        int i() const { return m_i; }
        int j() const { return m_j; }
        void moveTo(int i, int j) { m_i = i; m_j = j; }

        const IntRect& opaqueRect() const { return m_opaqueRect; }
        void setOpaqueRect(const IntRect& opaqueRect) { m_opaqueRect = opaqueRect; }
    private:
        int m_i;
        int m_j;
        IntRect m_opaqueRect;
    };
    // Default hash key traits for integers disallow 0 and -1 as a key, so
    // use a custom hash trait which disallows -1 and -2 instead.
    typedef std::pair<int, int> TileMapKey;
    struct TileMapKeyTraits : HashTraits<TileMapKey> {
        static const bool emptyValueIsZero = false;
        static const bool needsDestruction = false;
        static TileMapKey emptyValue() { return std::make_pair(-1, -1); }
        static void constructDeletedValue(TileMapKey& slot) { slot = std::make_pair(-2, -2); }
        static bool isDeletedValue(TileMapKey value) { return value.first == -2 && value.second == -2; }
    };
    typedef HashMap<TileMapKey, OwnPtr<Tile>, DefaultHash<TileMapKey>::Hash, TileMapKeyTraits> TileMap;

    void addTile(PassOwnPtr<Tile>, int, int);
    PassOwnPtr<Tile> takeTile(int, int);
    Tile* tileAt(int, int) const;
    const TileMap& tiles() const { return m_tiles; }

    void setBounds(const IntSize&);
    IntSize bounds() const;

    void contentRectToTileIndices(const IntRect&, int &left, int &top, int &right, int &bottom) const;
    IntRect tileRect(const Tile*) const;

    Region opaqueRegionInContentRect(const IntRect&) const;

    void reset();

protected:
    CCLayerTilingData(const IntSize& tileSize, BorderTexelOption);

    TileMap m_tiles;
    TilingData m_tilingData;
};

}

#endif // USE(ACCELERATED_COMPOSITING)

#endif
