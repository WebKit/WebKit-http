/*
 * Copyright (C) 2006 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Simon Hausmann <hausmann@kde.org>
 * Copyright (C) 2009 Torch Mobile Inc. http://www.torchmobile.com/
 * Copyright (C) 2010 Sencha, Inc.
 *
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "NativeImage.h"

#include "AffineTransform.h"
#include "BitmapImage.h"
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "ImageObserver.h"
#include "ShadowBlur.h"
#include "StillImageQt.h"
#include "Timer.h"

#include <QPainter>
#include <QPixmap>

namespace WebCore {

IntSize nativeImageSize(const NativeImagePtr& image)
{
    return image ? IntSize(image->size()) : IntSize();
}

bool nativeImageHasAlpha(const NativeImagePtr& image)
{
    return !image || image->hasAlpha();
}

Color nativeImageSinglePixelSolidColor(const NativeImagePtr& image)
{
    if (!image || image->width() != 1 || image->height() != 1)
        return Color();

    return QColor::fromRgba(image->toImage().pixel(0, 0));
}

void drawNativeImage(const NativeImagePtr& image, GraphicsContext& ctxt, const FloatRect& destRect, const FloatRect& srcRect, const IntSize& srcSize, CompositeOperator op, BlendMode blendMode, const ImageOrientation&)
{
    // QTFIXME: Handle imageSize? See e.g. NativeImageDirect2D

    QRectF normalizedDst = destRect.normalized();
    QRectF normalizedSrc = srcRect.normalized();

    CompositeOperator previousOperator = ctxt.compositeOperation();
    BlendMode previousBlendMode = ctxt.blendModeOperation();
    ctxt.setCompositeOperation(!image->hasAlpha() && op == CompositeSourceOver && blendMode == BlendMode::Normal ? CompositeCopy : op, blendMode);

    if (ctxt.hasShadow()) {
        ShadowBlur shadow(ctxt.state());
        const auto& pixmap = *image;
        shadow.drawShadowLayer(ctxt.getCTM(), ctxt.clipBounds(), normalizedDst,
            [normalizedSrc, normalizedDst, &pixmap](GraphicsContext& shadowContext)
            {
                QPainter* shadowPainter = shadowContext.platformContext();
                shadowPainter->drawPixmap(normalizedDst, pixmap, normalizedSrc);
            },
            [&ctxt](ImageBuffer& layerImage, const FloatPoint& layerOrigin, const FloatSize& layerSize)
            {
                ctxt.drawImageBuffer(layerImage, FloatRect(roundedIntPoint(layerOrigin), layerSize), FloatRect(FloatPoint(), layerSize), ctxt.compositeOperation());
            });
    }

    ctxt.platformContext()->drawPixmap(normalizedDst, *image, normalizedSrc);

    ctxt.setCompositeOperation(previousOperator, previousBlendMode);
}

void clearNativeImageSubimages(const NativeImagePtr&)
{
}

} // namespace WebCore
