/*
 Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
 */

#ifndef LayerBackingStore_h
#define LayerBackingStore_h

#if USE(UI_SIDE_COMPOSITING)
#include "HashMap.h"
#include "ShareableBitmap.h"
#include "TextureMapper.h"
#include "TextureMapperBackingStore.h"

namespace WebKit {

class LayerBackingStoreTile : public WebCore::TextureMapperTile {
public:
    LayerBackingStoreTile(float scale = 1)
        : TextureMapperTile(WebCore::FloatRect())
        , m_scale(scale)
    {
    }

    inline float scale() const { return m_scale; }
    void swapBuffers(WebCore::TextureMapper*);
    void setBackBuffer(const WebCore::IntRect&, const WebCore::IntRect&, ShareableBitmap* buffer);

private:
    RefPtr<ShareableBitmap> m_backBuffer;
    WebCore::IntRect m_sourceRect;
    WebCore::IntRect m_targetRect;
    float m_scale;
};

class LayerBackingStore : public WebCore::TextureMapperBackingStore {
public:
    void createTile(int, float);
    void removeTile(int);
    void updateTile(int, const WebCore::IntRect&, const WebCore::IntRect&, ShareableBitmap*);
    static PassRefPtr<LayerBackingStore> create() { return adoptRef(new LayerBackingStore); }
    void swapBuffers(WebCore::TextureMapper*);
    PassRefPtr<WebCore::BitmapTexture> texture() const;
    virtual void paintToTextureMapper(WebCore::TextureMapper*, const WebCore::FloatRect&, const WebCore::TransformationMatrix&, float, WebCore::BitmapTexture*);

private:
    LayerBackingStore()
        : m_scale(1.)
    { }
    HashMap<int, LayerBackingStoreTile> m_tiles;
    float m_scale;
};

}
#endif

#endif // LayerBackingStore_h
