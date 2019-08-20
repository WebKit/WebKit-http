/*
 * Copyright (C) 2008 Holger Hans Peter Freyther
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
#include "StillImageQt.h"

#include "GraphicsContext.h"
#include "ShadowBlur.h"

#include <QPainter>

namespace WebCore {

StillImage::StillImage(const QImage& image)
    : m_image(new QImage(image))
    , m_ownsImage(true)
{}

StillImage::StillImage(const QImage* image)
    : m_image(image)
    , m_ownsImage(false)
{}

StillImage::StillImage(QImage&& image)
    : m_image(new QImage())
    , m_ownsImage(true)
{
    const_cast<QImage*>(m_image)->swap(image);
}

StillImage::~StillImage()
{
    if (m_ownsImage)
        delete m_image;
}

bool StillImage::currentFrameKnownToBeOpaque() const
{
    return !m_image->hasAlphaChannel();
}

FloatSize StillImage::size() const
{
    return FloatSize(m_image->width(), m_image->height());
}

NativeImagePtr StillImage::nativeImageForCurrentFrame(const GraphicsContext*)
{
    return const_cast<NativeImagePtr>(m_image);
}

ImageDrawResult StillImage::draw(GraphicsContext& ctxt, const FloatRect& dst,
    const FloatRect& src, CompositeOperator op, BlendMode blendMode, DecodingMode, ImageOrientationDescription)
{
    if (m_image->isNull())
        return ImageDrawResult::DidNothing;

    FloatRect normalizedSrc = src.normalized();
    FloatRect normalizedDst = dst.normalized();

    // source rect needs scaling from the device coords to image coords
    normalizedSrc.scale(m_image->devicePixelRatio());

    CompositeOperator previousOperator = ctxt.compositeOperation();
    BlendMode previousBlendMode = ctxt.blendModeOperation();
    ctxt.setCompositeOperation(op, blendMode);

    if (ctxt.hasShadow()) {
        ShadowBlur shadow(ctxt.state());
        const auto& pixmap = *m_image;
        shadow.drawShadowLayer(ctxt.getCTM(), ctxt.clipBounds(), normalizedDst,
            [normalizedSrc, normalizedDst, &pixmap](GraphicsContext& shadowContext)
            {
                QPainter* shadowPainter = shadowContext.platformContext();
                shadowPainter->drawImage(normalizedDst, pixmap, normalizedSrc);
            },
            [&ctxt](ImageBuffer& layerImage, const FloatPoint& layerOrigin, const FloatSize& layerSize)
            {
                ctxt.drawImageBuffer(layerImage, FloatRect(roundedIntPoint(layerOrigin), layerSize), FloatRect(FloatPoint(), layerSize), ctxt.compositeOperation());
            });
    }

    ctxt.platformContext()->drawImage(normalizedDst, *m_image, normalizedSrc);
    ctxt.setCompositeOperation(previousOperator, previousBlendMode);
    return ImageDrawResult::DidDraw;
}

}
