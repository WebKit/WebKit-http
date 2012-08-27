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
#include "GraphicsSurface.h"

#include "NotImplemented.h"

#if USE(GRAPHICS_SURFACE)
namespace WebCore {

PassRefPtr<GraphicsSurface> GraphicsSurface::create(const IntSize& size, Flags flags, uint64_t token)
{
    return platformImport(size, flags, token);
}

PassRefPtr<GraphicsSurface> GraphicsSurface::create(const IntSize& size, GraphicsSurface::Flags flags)
{
    return platformCreate(size, flags);
}

uint64_t GraphicsSurface::exportToken()
{
    return platformExport();
}

uint32_t GraphicsSurface::getTextureID()
{
    return platformGetTextureID();
}

PassOwnPtr<GraphicsContext> GraphicsSurface::beginPaint(const IntRect& rect, LockOptions lockOptions)
{
    int stride = 0;
    char* bits = platformLock(rect, &stride, lockOptions);
    OwnPtr<GraphicsContext> graphicsContext = platformBeginPaint(rect.size(), bits, stride);
    return graphicsContext.release();
}

void GraphicsSurface::copyToGLTexture(uint32_t target, uint32_t texture, const IntRect& targetRect, const IntPoint& offset)
{
    platformCopyToGLTexture(target, texture, targetRect, offset);
}

void GraphicsSurface::copyFromFramebuffer(uint32_t fbo, const IntRect& sourceRect)
{
    platformCopyFromFramebuffer(fbo, sourceRect);
}

void GraphicsSurface::paintToTextureMapper(TextureMapper* textureMapper, const FloatRect& targetRect, const TransformationMatrix& transform, float opacity, BitmapTexture* mask)
{
    platformPaintToTextureMapper(textureMapper, targetRect, transform, opacity, mask);
}

uint32_t GraphicsSurface::frontBuffer()
{
    return platformFrontBuffer();
}

uint32_t GraphicsSurface::swapBuffers()
{
    return platformSwapBuffers();
}

GraphicsSurface::GraphicsSurface(const IntSize& size, Flags flags)
    : m_flags(flags)
    , m_size(size)
    , m_platformSurface(0)
    , m_texture(0)
    , m_fbo(0)
    , m_private(0)
{
}

GraphicsSurface::~GraphicsSurface()
{
    platformDestroy();
}

}
#endif
