/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2008 Holger Hans Peter Freyther
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2010 Torch Mobile (Beijing) Co. Ltd. All rights reserved.
 * Copyright (C) 2014 Digia Plc. and/or its subsidiary(-ies)
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
#include "ImageBuffer.h"

#include "GraphicsContext.h"
#include "ImageData.h"
#include "IntRect.h"
#include "MIMETypeRegistry.h"
#include "StillImageQt.h"
#include "TransparencyLayer.h"
#include <runtime/JSCInlines.h>
#include <runtime/TypedArrayInlines.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

#include <QBuffer>
#include <QImage>
#include <QImageWriter>
#include <QPainter>
#include <QPixmap>
#include <math.h>

namespace WebCore {

#if ENABLE(ACCELERATED_2D_CANVAS)
ImageBuffer::ImageBuffer(const IntSize& size, ColorSpace, QOpenGLContext* compatibleContext, bool& success)
    : m_data(size, compatibleContext)
    , m_size(size)
    , m_logicalSize(size)
    , m_resolutionScale(1.0)
{
    success = m_data.m_painter && m_data.m_painter->isActive();
    if (!success)
        return;

    m_data.m_context = std::make_unique<GraphicsContext>(m_data.m_painter);
}
#endif

ImageBuffer::ImageBuffer(const FloatSize& size, float resolutionScale, ColorSpace, RenderingMode /*renderingMode*/, bool& success)
    : m_data(size, resolutionScale)
    , m_size(size * resolutionScale)
    , m_logicalSize(size)
    , m_resolutionScale(resolutionScale)
{
    success = m_data.m_painter && m_data.m_painter->isActive();
    if (!success)
        return;

    m_data.m_context = std::make_unique<GraphicsContext>(m_data.m_painter);
}

ImageBuffer::~ImageBuffer()
{
}

#if ENABLE(ACCELERATED_2D_CANVAS)
std::unique_ptr<ImageBuffer> ImageBuffer::createCompatibleBuffer(const IntSize& size, ColorSpace colorSpace, QOpenGLContext* context)
{
    bool success = false;
    std::unique_ptr<ImageBuffer> buf(new ImageBuffer(size, colorSpace, context, success));
    if (!success)
        return nullptr;
    return buf;
}
#endif

GraphicsContext& ImageBuffer::context() const
{
    ASSERT(m_data.m_painter->isActive());

    return *m_data.m_context;
}

RefPtr<Image> ImageBuffer::copyImage(BackingStoreCopy copyBehavior, ScaleBehavior) const
{
    if (copyBehavior == CopyBackingStore)
        return m_data.m_impl->copyImage();

    return m_data.m_impl->image();
}

RefPtr<Image> ImageBuffer::sinkIntoImage(std::unique_ptr<ImageBuffer> imageBuffer, ScaleBehavior)
{
    return imageBuffer->m_data.m_impl->takeImage();
}

BackingStoreCopy ImageBuffer::fastCopyImageMode()
{
    return DontCopyBackingStore;
}

void ImageBuffer::drawConsuming(std::unique_ptr<ImageBuffer> imageBuffer, GraphicsContext& destContext, const FloatRect& destRect, const FloatRect& srcRect, CompositeOperator op, BlendMode blendMode)
{
    imageBuffer->draw(destContext, destRect, srcRect, op, blendMode);
}

void ImageBuffer::draw(GraphicsContext& destContext, const FloatRect& destRect, const FloatRect& srcRect,
    CompositeOperator op, BlendMode blendMode)
{
    m_data.m_impl->draw(destContext, destRect, srcRect, op, blendMode, &destContext == &context());
}

void ImageBuffer::drawPattern(GraphicsContext& destContext, const FloatRect& srcRect, const AffineTransform& patternTransform,
                              const FloatPoint& phase, const FloatSize& spacing, CompositeOperator op, const FloatRect& destRect, BlendMode blendMode)
{
    m_data.m_impl->drawPattern(destContext, srcRect, patternTransform, phase, spacing, op, destRect, blendMode, &destContext == &context());
}

void ImageBuffer::platformTransformColorSpace(const Vector<int>& lookUpTable)
{
    m_data.m_impl->platformTransformColorSpace(lookUpTable);
}

template <Multiply multiplied>
PassRefPtr<Uint8ClampedArray> getImageData(const IntRect& unscaledRect, float scale, const ImageBufferData& imageData, const IntSize& size,
    ImageBuffer::CoordinateSystem coordinateSystem)
{
    IntRect rect(unscaledRect);

    if (coordinateSystem == ImageBuffer::LogicalCoordinateSystem)
        rect.scale(scale);

    float area = 4.0f * rect.width() * rect.height();
    if (area > static_cast<float>(std::numeric_limits<int>::max()))
        return 0;

    RefPtr<Uint8ClampedArray> result = Uint8ClampedArray::createUninitialized(rect.width() * rect.height() * 4);

    QImage::Format format = (multiplied == Unmultiplied) ? QImage::Format_RGBA8888 : QImage::Format_RGBA8888_Premultiplied;
    QImage image(result->data(), rect.width(), rect.height(), format);
    if (coordinateSystem == ImageBuffer::LogicalCoordinateSystem)
        image.setDevicePixelRatio(scale);
    if (rect.x() < 0 || rect.y() < 0 || rect.maxX() > size.width() || rect.maxY() > size.height())
        image.fill(0);

    // Let drawImage deal with the conversion.
    // FIXME: This is inefficient for accelerated ImageBuffers when only part of the imageData is read.
    QPainter painter(&image);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.drawImage(QPoint(0, 0), imageData.m_impl->toQImage(), rect);
    painter.end();

    return result.release();
}

PassRefPtr<Uint8ClampedArray> ImageBuffer::getUnmultipliedImageData(const IntRect& rect, CoordinateSystem coordinateSystem) const
{
    return getImageData<Unmultiplied>(rect, m_resolutionScale, m_data, m_size, coordinateSystem);
}

PassRefPtr<Uint8ClampedArray> ImageBuffer::getPremultipliedImageData(const IntRect& rect, CoordinateSystem coordinateSystem) const
{
    return getImageData<Premultiplied>(rect, m_resolutionScale, m_data, m_size, coordinateSystem);
}

void ImageBuffer::putByteArray(Multiply multiplied, Uint8ClampedArray* source, const IntSize& sourceSize, const IntRect& sourceRect, const IntPoint& destPoint, CoordinateSystem coordinateSystem)
{
    ASSERT(sourceRect.width() > 0);
    ASSERT(sourceRect.height() > 0);

    bool isPainting = m_data.m_painter->isActive();
    if (!isPainting)
        m_data.m_painter->begin(m_data.m_impl->paintDevice());
    else {
        m_data.m_painter->save();

        // putImageData() should be unaffected by painter state
        m_data.m_painter->resetTransform();
        m_data.m_painter->setOpacity(1.0);
        m_data.m_painter->setClipping(false);
    }

    // source rect & size need scaling from the device coords to image coords
    IntSize scaledSourceSize(sourceSize);
    IntRect scaledSourceRect(sourceRect);
    if (coordinateSystem == LogicalCoordinateSystem) {
        scaledSourceSize.scale(m_resolutionScale);
        scaledSourceRect.scale(m_resolutionScale);
    }

    // Let drawImage deal with the conversion.
    QImage::Format format = (multiplied == Unmultiplied) ? QImage::Format_RGBA8888 : QImage::Format_RGBA8888_Premultiplied;
    QImage image(source->data(), scaledSourceSize.width(), scaledSourceSize.height(), format);
    image.setDevicePixelRatio(m_resolutionScale);

    m_data.m_painter->setCompositionMode(QPainter::CompositionMode_Source);
    m_data.m_painter->drawImage(destPoint + sourceRect.location(), image, scaledSourceRect);

    if (!isPainting)
        m_data.m_painter->end();
    else
        m_data.m_painter->restore();
}

static bool encodeImage(const QPixmap& pixmap, const String& format, const double* quality, QByteArray& data)
{
    int compressionQuality = -1;
    if (format == "jpeg" || format == "webp") {
        compressionQuality = 100;
        if (quality && *quality >= 0.0 && *quality <= 1.0)
            compressionQuality = static_cast<int>(*quality * 100 + 0.5);
    }

    QBuffer buffer(&data);
    buffer.open(QBuffer::WriteOnly);
    bool success = pixmap.save(&buffer, format.utf8().data(), compressionQuality);
    buffer.close();

    return success;
}

String ImageBuffer::toDataURL(const String& mimeType, const double* quality, CoordinateSystem) const
{
    ASSERT(MIMETypeRegistry::isSupportedImageMIMETypeForEncoding(mimeType));

    // QImageWriter does not support mimetypes. It does support Qt image formats (png,
    // gif, jpeg..., xpm) so skip the image/ to get the Qt image format used to encode
    // the m_pixmap image.

    RefPtr<Image> image = copyImage(DontCopyBackingStore);
    QByteArray data;
    if (!encodeImage(*image->nativeImageForCurrentFrame(), mimeType.substring(sizeof "image"), quality, data))
        return "data:,";

    return "data:" + mimeType + ";base64," + data.toBase64().data();
}

PlatformLayer* ImageBuffer::platformLayer() const
{
    return m_data.m_impl->platformLayer();
}

}
