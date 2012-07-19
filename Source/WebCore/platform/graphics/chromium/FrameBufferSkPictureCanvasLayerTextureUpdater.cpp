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

#include "FrameBufferSkPictureCanvasLayerTextureUpdater.h"

#include "LayerPainterChromium.h"
#include "SharedGraphicsContext3D.h"
#include "SkCanvas.h"
#include "SkGpuDevice.h"
#include "cc/CCProxy.h"

namespace WebCore {

static PassOwnPtr<SkCanvas> createAcceleratedCanvas(GraphicsContext3D* context,
                                                    IntSize canvasSize,
                                                    unsigned textureId)
{
    GrContext* grContext = context->grContext();
    GrPlatformTextureDesc textureDesc;
    textureDesc.fFlags = kRenderTarget_GrPlatformTextureFlag;
    textureDesc.fWidth = canvasSize.width();
    textureDesc.fHeight = canvasSize.height();
    textureDesc.fConfig = kSkia8888_PM_GrPixelConfig;
    textureDesc.fTextureHandle = textureId;
    SkAutoTUnref<GrTexture> target(grContext->createPlatformTexture(textureDesc));
    SkAutoTUnref<SkDevice> device(new SkGpuDevice(grContext, target.get()));
    return adoptPtr(new SkCanvas(device.get()));
}

FrameBufferSkPictureCanvasLayerTextureUpdater::Texture::Texture(FrameBufferSkPictureCanvasLayerTextureUpdater* textureUpdater, PassOwnPtr<CCPrioritizedTexture> texture)
    : LayerTextureUpdater::Texture(texture)
    , m_textureUpdater(textureUpdater)
{
}

FrameBufferSkPictureCanvasLayerTextureUpdater::Texture::~Texture()
{
}

void FrameBufferSkPictureCanvasLayerTextureUpdater::Texture::updateRect(CCResourceProvider* resourceProvider, const IntRect& sourceRect, const IntRect& destRect)
{
    RefPtr<GraphicsContext3D> sharedContext = CCProxy::hasImplThread() ? SharedGraphicsContext3D::getForImplThread() : SharedGraphicsContext3D::get();
    if (!sharedContext)
        return;
    textureUpdater()->updateTextureRect(sharedContext.release(), resourceProvider, texture(), sourceRect, destRect);
}

PassRefPtr<FrameBufferSkPictureCanvasLayerTextureUpdater> FrameBufferSkPictureCanvasLayerTextureUpdater::create(PassOwnPtr<LayerPainterChromium> painter)
{
    return adoptRef(new FrameBufferSkPictureCanvasLayerTextureUpdater(painter));
}

FrameBufferSkPictureCanvasLayerTextureUpdater::FrameBufferSkPictureCanvasLayerTextureUpdater(PassOwnPtr<LayerPainterChromium> painter)
    : SkPictureCanvasLayerTextureUpdater(painter)
{
}

FrameBufferSkPictureCanvasLayerTextureUpdater::~FrameBufferSkPictureCanvasLayerTextureUpdater()
{
}

PassOwnPtr<LayerTextureUpdater::Texture> FrameBufferSkPictureCanvasLayerTextureUpdater::createTexture(CCPrioritizedTextureManager* manager)
{
    return adoptPtr(new Texture(this, CCPrioritizedTexture::create(manager)));
}

LayerTextureUpdater::SampledTexelFormat FrameBufferSkPictureCanvasLayerTextureUpdater::sampledTexelFormat(GC3Denum textureFormat)
{
    // Here we directly render to the texture, so the component order is always correct.
    return LayerTextureUpdater::SampledTexelFormatRGBA;
}

void FrameBufferSkPictureCanvasLayerTextureUpdater::updateTextureRect(PassRefPtr<GraphicsContext3D> prpContext, CCResourceProvider* resourceProvider, CCPrioritizedTexture* texture, const IntRect& sourceRect, const IntRect& destRect)
{
    RefPtr<GraphicsContext3D> context(prpContext);

    // Make sure ganesh uses the correct GL context.
    context->makeContextCurrent();

    texture->acquireBackingTexture(resourceProvider);
    CCScopedLockResourceForWrite lock(resourceProvider, texture->resourceId());
    // Create an accelerated canvas to draw on.
    OwnPtr<SkCanvas> canvas = createAcceleratedCanvas(context.get(), texture->size(), lock.textureId());

    // The compositor expects the textures to be upside-down so it can flip
    // the final composited image. Ganesh renders the image upright so we
    // need to do a y-flip.
    canvas->translate(0.0, texture->size().height());
    canvas->scale(1.0, -1.0);
    // Only the region corresponding to destRect on the texture must be updated.
    canvas->clipRect(SkRect(destRect));
    // Translate the origin of contentRect to that of destRect.
    // Note that destRect is defined relative to sourceRect.
    canvas->translate(contentRect().x() - sourceRect.x() + destRect.x(),
                      contentRect().y() - sourceRect.y() + destRect.y());
    drawPicture(canvas.get());

    // Flush ganesh context so that all the rendered stuff appears on the texture.
    context->grContext()->flush();

    // Flush the GL context so rendering results from this context are visible in the compositor's context.
    context->flush();
}

} // namespace WebCore
#endif // USE(ACCELERATED_COMPOSITING)
