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
#include "TextureMapperBackingStore.h"

#include "GraphicsLayer.h"
#include "ImageBuffer.h"
#include "TextureMapper.h"

namespace WebCore {

void TextureMapperTile::updateContents(TextureMapper* textureMapper, Image* image, const IntRect& dirtyRect, BitmapTexture::PixelFormat format)
{
    IntRect targetRect = enclosingIntRect(m_rect);
    targetRect.intersect(dirtyRect);
    if (targetRect.isEmpty())
        return;
    IntRect sourceRect = targetRect;

    // Normalize sourceRect to the buffer's coordinates.
    sourceRect.move(-dirtyRect.x(), -dirtyRect.y());

    // Normalize targetRect to the texture's coordinates.
    targetRect.move(-m_rect.x(), -m_rect.y());
    if (!m_texture) {
        m_texture = textureMapper->createTexture();
        m_texture->reset(targetRect.size(), image->currentFrameHasAlpha() ? BitmapTexture::SupportsAlpha : 0);
    }

    m_texture->updateContents(image, targetRect, sourceRect, format);
}

void TextureMapperTile::paint(TextureMapper* textureMapper, const TransformationMatrix& transform, float opacity, BitmapTexture* mask)
{
    textureMapper->drawTexture(*texture().get(), rect(), transform, opacity, mask);
}

void TextureMapperTiledBackingStore::updateContentsFromImageIfNeeded(TextureMapper* textureMapper)
{
    if (!m_image)
        return;

    updateContents(textureMapper, m_image.get(), m_image->currentFrameHasAlpha() ? BitmapTexture::BGRAFormat : BitmapTexture::BGRFormat);
    m_image.clear();
}

void TextureMapperTiledBackingStore::paintToTextureMapper(TextureMapper* textureMapper, const FloatRect& targetRect, const TransformationMatrix& transform, float opacity, BitmapTexture* mask)
{
    updateContentsFromImageIfNeeded(textureMapper);
    TransformationMatrix adjustedTransform = transform;
    adjustedTransform.multiply(TransformationMatrix::rectToRect(rect(), targetRect));
    for (size_t i = 0; i < m_tiles.size(); ++i)
        m_tiles[i].paint(textureMapper, adjustedTransform, opacity, mask);
}

void TextureMapperTiledBackingStore::createOrDestroyTilesIfNeeded(const FloatSize& size, const IntSize& tileSize, bool hasAlpha)
{
    if (size == m_size)
        return;

    m_size = size;

    Vector<FloatRect> tileRectsToAdd;
    Vector<int> tileIndicesToRemove;
    static const size_t TileEraseThreshold = 6;

    // This method recycles tiles. We check which tiles we need to add, which to remove, and use as many
    // removable tiles as replacement for new tiles when possible.
    for (float y = 0; y < m_size.height(); y += tileSize.height()) {
        for (float x = 0; x < m_size.width(); x += tileSize.width()) {
            FloatRect tileRect(x, y, tileSize.width(), tileSize.height());
            tileRect.intersect(rect());
            tileRectsToAdd.append(tileRect);
        }
    }

    // Check which tiles need to be removed, and which already exist.
    for (int i = m_tiles.size() - 1; i >= 0; --i) {
        FloatRect oldTile = m_tiles[i].rect();
        bool existsAlready = false;

        for (int j = tileRectsToAdd.size() - 1; j >= 0; --j) {
            FloatRect newTile = tileRectsToAdd[j];
            if (oldTile != newTile)
                continue;

            // A tile that we want to add already exists, no need to add or remove it.
            existsAlready = true;
            tileRectsToAdd.remove(j);
            break;
        }

        // This tile is not needed.
        if (!existsAlready)
            tileIndicesToRemove.append(i);
    }

    // Recycle removable tiles to be used for newly requested tiles.
    for (size_t i = 0; i < tileRectsToAdd.size(); ++i) {
        if (!tileIndicesToRemove.isEmpty()) {
            // We recycle an existing tile for usage with a new tile rect.
            TextureMapperTile& tile = m_tiles[tileIndicesToRemove.last()];
            tileIndicesToRemove.removeLast();
            tile.setRect(tileRectsToAdd[i]);

            if (tile.texture())
                tile.texture()->reset(enclosingIntRect(tile.rect()).size(), hasAlpha ? BitmapTexture::SupportsAlpha : 0);
            continue;
        }

        m_tiles.append(TextureMapperTile(tileRectsToAdd[i]));
    }

    // Remove unnecessary tiles, if they weren't recycled.
    // We use a threshold to make sure we don't create/destroy tiles too eagerly.
    for (size_t i = 0; i < tileIndicesToRemove.size() && m_tiles.size() > TileEraseThreshold; ++i)
        m_tiles.remove(tileIndicesToRemove[i]);
}

void TextureMapperTiledBackingStore::updateContents(TextureMapper* textureMapper, Image* image, const FloatSize& totalSize, const IntRect& dirtyRect, BitmapTexture::PixelFormat format)
{
    createOrDestroyTilesIfNeeded(totalSize, textureMapper->maxTextureSize(), image->currentFrameHasAlpha());
    for (size_t i = 0; i < m_tiles.size(); ++i)
        m_tiles[i].updateContents(textureMapper, image, dirtyRect, format);
}

PassRefPtr<BitmapTexture> TextureMapperTiledBackingStore::texture() const
{
    for (size_t i = 0; i < m_tiles.size(); ++i) {
        RefPtr<BitmapTexture> texture = m_tiles[i].texture();
        if (texture)
            return texture;
    }

    return PassRefPtr<BitmapTexture>();
}

}
