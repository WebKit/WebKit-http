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

#include "config.h"
#include "LayerBackingStore.h"

#if USE(UI_SIDE_COMPOSITING)
#include "GraphicsLayer.h"
#include "TextureMapper.h"

#include "stdio.h"

using namespace WebCore;

namespace WebKit {

void LayerBackingStoreTile::swapBuffers(WebCore::TextureMapper* textureMapper)
{
    if (!m_backBuffer)
        return;

    FloatRect targetRect(m_targetRect);
    targetRect.scale(1. / m_scale);
    bool shouldReset = false;
    if (targetRect != rect()) {
        setRect(targetRect);
        shouldReset = true;
    }
    RefPtr<BitmapTexture> texture = this->texture();
    if (!texture) {
        texture = textureMapper->createTexture();
        setTexture(texture.get());
        shouldReset = true;
    }

    QImage qImage = m_backBuffer->createQImage();

    if (shouldReset)
        texture->reset(m_sourceRect.size(), qImage.hasAlphaChannel() ? BitmapTexture::SupportsAlpha : 0);

    texture->updateContents(qImage.constBits(), m_sourceRect);
    m_backBuffer.clear();
}

void LayerBackingStoreTile::setBackBuffer(const WebCore::IntRect& targetRect, const WebCore::IntRect& sourceRect, ShareableBitmap* buffer)
{
    m_sourceRect = sourceRect;
    m_targetRect = targetRect;
    m_backBuffer = buffer;
}

void LayerBackingStore::createTile(int id, float scale)
{
    m_tiles.add(id, LayerBackingStoreTile(scale));
    m_scale = scale;
}

void LayerBackingStore::removeTile(int id)
{
    m_tiles.remove(id);
}

void LayerBackingStore::updateTile(int id, const IntRect& sourceRect, const IntRect& targetRect, ShareableBitmap* backBuffer)
{
    HashMap<int, LayerBackingStoreTile>::iterator it = m_tiles.find(id);
    ASSERT(it != m_tiles.end());
    it->second.setBackBuffer(targetRect, sourceRect, backBuffer);
}

PassRefPtr<BitmapTexture> LayerBackingStore::texture() const
{
    HashMap<int, LayerBackingStoreTile>::const_iterator end = m_tiles.end();
    for (HashMap<int, LayerBackingStoreTile>::const_iterator it = m_tiles.begin(); it != end; ++it) {
        RefPtr<BitmapTexture> texture = it->second.texture();
        if (texture)
            return texture;
    }

    return PassRefPtr<BitmapTexture>();
}

void LayerBackingStore::paintToTextureMapper(TextureMapper* textureMapper, const FloatRect& targetRect, const TransformationMatrix& transform, float opacity, BitmapTexture* mask)
{
    Vector<TextureMapperTile*> tilesToPaint;

    // We have to do this every time we paint, in case the opacity has changed.
    HashMap<int, LayerBackingStoreTile>::iterator end = m_tiles.end();
    FloatRect coveredRect;
    for (HashMap<int, LayerBackingStoreTile>::iterator it = m_tiles.begin(); it != end; ++it) {
        LayerBackingStoreTile& tile = it->second;
        if (!tile.texture())
            continue;

        if (tile.scale() == m_scale) {
            tilesToPaint.append(&tile);
            coveredRect.unite(tile.rect());
            continue;
        }

        // Only show the previous tile if the opacity is high, otherwise effect looks like a bug.
        // We show the previous-scale tile anyway if it doesn't intersect with any current-scale tile.
        if (opacity < 0.95 && coveredRect.intersects(tile.rect()))
            continue;

        tilesToPaint.prepend(&tile);
        coveredRect.unite(tile.rect());
    }

    bool shouldClip = !targetRect.contains(coveredRect);

    if (shouldClip)
        textureMapper->beginClip(transform, targetRect);

    for (size_t i = 0; i < tilesToPaint.size(); ++i)
        tilesToPaint[i]->paint(textureMapper, transform, opacity, mask);

    if (shouldClip)
        textureMapper->endClip();
}

void LayerBackingStore::swapBuffers(TextureMapper* textureMapper)
{
    HashMap<int, LayerBackingStoreTile>::iterator end = m_tiles.end();
    for (HashMap<int, LayerBackingStoreTile>::iterator it = m_tiles.begin(); it != end; ++it)
        it->second.swapBuffers(textureMapper);
}

}
#endif
