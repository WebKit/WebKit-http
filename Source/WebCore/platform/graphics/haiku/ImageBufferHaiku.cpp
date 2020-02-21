/*
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
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

#include "ColorUtilities.h"
#include "GraphicsContext.h"
#include "ImageData.h"
#include "IntRect.h"
#include "MIMETypeRegistry.h"
#include "NotImplemented.h"
#include "StillImageHaiku.h"
#include "JavaScriptCore/JSCInlines.h"
#include "JavaScriptCore/TypedArrayInlines.h"

#include <wtf/text/Base64.h>
#include <wtf/text/CString.h>
#include <BitmapStream.h>
#include <Picture.h>
#include <String.h>
#include <TranslatorRoster.h>
#include <stdio.h>


namespace WebCore {


ImageBufferData::ImageBufferData(const FloatSize& size)
    : m_bitmap(adoptRef(new BitmapRef(BRect(0, 0, size.width() - 1., size.height() - 1.), B_RGBA32, true)))
    , m_view(NULL)
    , m_context(NULL)
{
    // Always keep the bitmap locked, we are the only client.
    m_bitmap->Lock();
    if(size.isEmpty())
        return;

    if (!m_bitmap->IsLocked() || !m_bitmap->IsValid())
        return;

    m_view = new BView(m_bitmap->Bounds(), "WebKit ImageBufferData", 0, 0);
    m_bitmap->AddChild(m_view);

    // Fill with completely transparent color.
    memset(m_bitmap->Bits(), 0, m_bitmap->BitsLength());

    // Since ImageBuffer is used mainly for Canvas, explicitly initialize
    // its view's graphics state with the corresponding canvas defaults
    // NOTE: keep in sync with CanvasRenderingContext2D::State
    m_view->SetLineMode(B_BUTT_CAP, B_MITER_JOIN, 10);
    m_view->SetDrawingMode(B_OP_ALPHA);
    m_view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
    m_context = new GraphicsContext(m_view);

    m_image = StillImage::createForRendering(m_bitmap);
}

ImageBufferData::~ImageBufferData()
{
    m_view = nullptr;
        // m_bitmap owns m_view and deletes it when going out of this destructor.
    m_image = nullptr;
    delete m_context;

    m_bitmap->Unlock();
}


NativeImagePtr ImageBuffer::nativeImage() const
{
	return m_data.m_bitmap;
}


ImageBuffer::ImageBuffer(const FloatSize& size, float resolutionScale, ColorSpace, RenderingMode, const HostWindow*, bool& success)
    : m_data(size)
    , m_size(size)
    , m_logicalSize(size)
    , m_resolutionScale(resolutionScale)
{
    success = false;  // Make early return mean error.

    float scaledWidth = ceilf(m_resolutionScale * size.width());
    float scaledHeight = ceilf(m_resolutionScale * size.height());

    // FIXME: Should we automatically use a lower resolution?
    if (!FloatSize(scaledWidth, scaledHeight).isExpressibleAsIntSize())
        return;

    if (m_size.isEmpty())
        return;

    success = m_data.m_view != nullptr;
}

ImageBuffer::~ImageBuffer()
{
}

GraphicsContext& ImageBuffer::context() const
{
    return *m_data.m_context;
}

RefPtr<Image> ImageBuffer::sinkIntoImage(std::unique_ptr<ImageBuffer> imageBuffer, PreserveResolution preserveResolution)
{
    return imageBuffer->copyImage(DontCopyBackingStore, preserveResolution);
}

RefPtr<Image> ImageBuffer::copyImage(BackingStoreCopy copyBehavior, PreserveResolution) const
{
    if (m_data.m_view)
        m_data.m_view->Sync();

    if (copyBehavior == CopyBackingStore)
        return StillImage::create(NativeImagePtr(new BitmapRef(*dynamic_cast<BBitmap*>(m_data.m_bitmap.get()))));

    return StillImage::createForRendering(m_data.m_bitmap);
}

void ImageBuffer::drawConsuming(std::unique_ptr<ImageBuffer> imageBuffer, GraphicsContext& destContext, const FloatRect& destRect, const FloatRect& srcRect, const ImagePaintingOptions& options)
{
    imageBuffer->draw(destContext, destRect, srcRect, options);
}

void ImageBuffer::draw(GraphicsContext& destContext, const FloatRect& destRect, const FloatRect& srcRect,
                       const ImagePaintingOptions& options)
{
    if (!m_data.m_view)
        return;

    m_data.m_view->Sync();
    if (&destContext == &context() && destRect.intersects(srcRect)) {
        // We're drawing into our own buffer.  In order for this to work, we need to copy the source buffer first.
        RefPtr<Image> copy = copyImage(CopyBackingStore);
        destContext.drawImage(*copy.get(), destRect, srcRect, options);
    } else
        destContext.drawImage(*m_data.m_image.get(), destRect, srcRect, options);
}

void ImageBuffer::drawPattern(GraphicsContext& destContext,
    const FloatRect& destRect, const FloatRect& srcRect,
    const AffineTransform& patternTransform, const FloatPoint& phase,
    const FloatSize& size, const ImagePaintingOptions& options)
{
    if (!m_data.m_view)
        return;

    m_data.m_view->Sync();
    if (&destContext == &context() && srcRect.intersects(destRect)) {
        // We're drawing into our own buffer.  In order for this to work, we need to copy the source buffer first.
        RefPtr<Image> copy = copyImage(CopyBackingStore);
        copy->drawPattern(destContext, destRect, srcRect, patternTransform, phase, size, options.compositeOperator());
    } else
        m_data.m_image->drawPattern(destContext, destRect, srcRect, patternTransform, phase, size, options.compositeOperator());
}

void ImageBuffer::platformTransformColorSpace(const std::array<uint8_t, 256>& lookUpTable)
{
    uint8* rowData = reinterpret_cast<uint8*>(m_data.m_bitmap->Bits());
    unsigned bytesPerRow = m_data.m_bitmap->BytesPerRow();
    unsigned rows = m_size.height();
    unsigned columns = m_size.width();
    for (unsigned y = 0; y < rows; y++) {
        uint8* pixel = rowData;
        for (unsigned x = 0; x < columns; x++) {
            // lookUpTable doesn't seem to support a LUT for each color channel
            // separately (judging from the other ports). We don't need to
            // convert from/to pre-multiplied color space since BBitmap storage
            // is not pre-multiplied.
            pixel[0] = lookUpTable[pixel[0]];
            pixel[1] = lookUpTable[pixel[1]];
            pixel[2] = lookUpTable[pixel[2]];
            // alpha stays unmodified.
            pixel += 4;
        }
        rowData += bytesPerRow;
    }
}

void ImageBuffer::transformColorSpace(ColorSpace srcColorSpace, ColorSpace dstColorSpace)
{
    if (srcColorSpace == dstColorSpace)
        return;

    // only sRGB <-> linearRGB are supported at the moment
    if ((srcColorSpace != ColorSpace::LinearRGB && srcColorSpace != ColorSpace::SRGB)
        || (dstColorSpace != ColorSpace::LinearRGB && dstColorSpace != ColorSpace::SRGB))
        return;

    if (dstColorSpace == ColorSpace::LinearRGB) {
        static const std::array<uint8_t, 256> linearRgbLUT = [] {
            std::array<uint8_t, 256> array;
            for (unsigned i = 0; i < 256; i++) {
                float color = i / 255.0f;
                color = sRGBToLinearColorComponent(color);
                array[i] = static_cast<uint8_t>(round(color * 255));
            }
            return array;
        }();
        platformTransformColorSpace(linearRgbLUT);
    } else if (dstColorSpace == ColorSpace::SRGB) {
        static const std::array<uint8_t, 256> deviceRgbLUT= [] {
            std::array<uint8_t, 256> array;
            for (unsigned i = 0; i < 256; i++) {
                float color = i / 255.0f;
                color = linearToSRGBColorComponent(color);
                array[i] = static_cast<uint8_t>(round(color * 255));
            }
            return array;
        }();
        platformTransformColorSpace(deviceRgbLUT);
    }
}

template <AlphaPremultiplication premultiplied>
RefPtr<ImageData> getData(const IntRect& rect, const IntRect& logicalRect, const ImageBufferData& data, const IntSize& size, const IntSize& logicalSize, float resolutionScale)
{
    auto result = ImageData::create(rect.size());
    auto* pixelArray = result ? result->data() : nullptr;
    if (!result)
        return nullptr;

    // Can overflow, as we are adding 2 ints.
    int endx = 0;
    if (!WTF::safeAdd(rect.x(), rect.width(), endx))
        return nullptr;

    // Can overflow, as we are adding 2 ints.
    int endy = 0;
    if (!WTF::safeAdd(rect.y(), rect.height(), endy))
        return nullptr;

    if (rect.x() < 0 || rect.y() < 0 || endx > size.width() || endy > size.height())
        pixelArray->zeroFill();

    int originx = rect.x();
    int destx = 0;
    if (originx < 0) {
        destx = -originx;
        originx = 0;
    }

    if (endx > size.width())
        endx = size.width();
    int numColumns = endx - originx;

    int originy = rect.y();
    int desty = 0;
    if (originy < 0) {
        desty = -originy;
        originy = 0;
    }

    if (endy > size.height())
        endy = size.height();
    int numRows = endy - originy;

    // Nothing will be copied, so just return the result.
    if (numColumns <= 0 || numRows <= 0)
        return result;

    // The size of the derived surface is in BackingStoreCoordinateSystem.
    // We need to set the device scale for the derived surface from this ImageBuffer.
    IntRect imageRect(originx, originy, numColumns, numRows);
    NativeImagePtr imageSurface = data.m_bitmap;
    //cairoSurfaceSetDeviceScale(imageSurface.get(), resolutionScale, resolutionScale);
    originx = imageRect.x();
    originy = imageRect.y();

    unsigned char* dataSrc = (unsigned char*)imageSurface->Bits();
    unsigned char* dataDst = pixelArray->data();
    int stride = imageSurface->BytesPerRow();
    unsigned destBytesPerRow = 4 * rect.width();

    unsigned char* destRows = dataDst + desty * destBytesPerRow + destx * 4;
    for (int y = 0; y < numRows; ++y) {
        unsigned* row = reinterpret_cast_ptr<unsigned*>(dataSrc + stride * (y + originy));
        for (int x = 0; x < numColumns; x++) {
            int basex = x * 4;
            unsigned* pixel = row + x + originx;

            // Avoid calling Color::colorFromPremultipliedARGB() because one
            // function call per pixel is too expensive.
            unsigned alpha = (*pixel & 0xFF000000) >> 24;
            unsigned red = (*pixel & 0x00FF0000) >> 16;
            unsigned green = (*pixel & 0x0000FF00) >> 8;
            unsigned blue = (*pixel & 0x000000FF);

            if (premultiplied == AlphaPremultiplication::Unpremultiplied) {
                if (alpha && alpha != 255) {
                    red = red * 255 / alpha;
                    green = green * 255 / alpha;
                    blue = blue * 255 / alpha;
                }
            }

            destRows[basex]     = red;
            destRows[basex + 1] = green;
            destRows[basex + 2] = blue;
            destRows[basex + 3] = alpha;
        }
        destRows += destBytesPerRow;
    }

    return result;
}

RefPtr<ImageData> ImageBuffer::getImageData(AlphaPremultiplication outputFormat, const IntRect& srcRect) const
{
    IntRect logicalRect = srcRect;
    IntRect backingStoreRect = srcRect;
    backingStoreRect.scale(m_resolutionScale);
    
    if (outputFormat == AlphaPremultiplication::Unpremultiplied)
        return getData<AlphaPremultiplication::Unpremultiplied>(backingStoreRect, logicalRect, m_data, m_size, m_logicalSize, m_resolutionScale);
    return getData<AlphaPremultiplication::Premultiplied>(backingStoreRect, logicalRect, m_data, m_size, m_logicalSize, m_resolutionScale);
}

void ImageBuffer::putImageData(AlphaPremultiplication sourceFormat, const ImageData& imageData, const IntRect& sourceRect, const IntPoint& destPoint)
{
    IntRect logicalSourceRect = sourceRect;
    IntPoint logicalDestPoint = destPoint;

    IntRect scaledSourceRect = sourceRect;
    IntSize scaledSourceSize = imageData.size();
    IntPoint scaledDestPoint = destPoint;

    scaledSourceRect.scale(m_resolutionScale);
    scaledDestPoint.scale(m_resolutionScale);

    ASSERT(scaledSourceRect.width() > 0);
    ASSERT(scaledSourceRect.height() > 0);

    int originx = scaledSourceRect.x();
    int destx = scaledDestPoint.x() + scaledSourceRect.x();
    int logicalDestx = logicalDestPoint.x() + logicalSourceRect.x();
    ASSERT(destx >= 0);
    ASSERT(destx < m_size.width());
    ASSERT(originx >= 0);
    ASSERT(originx <= scaledSourceRect.maxX());

    int endx = scaledDestPoint.x() + scaledSourceRect.maxX();
    int logicalEndx = logicalDestPoint.x() + logicalSourceRect.maxX();
    ASSERT(endx <= m_size.width());

    int numColumns = endx - destx;
    int logicalNumColumns = logicalEndx - logicalDestx;

    int originy = scaledSourceRect.y();
    int desty = scaledDestPoint.y() + scaledSourceRect.y();
    int logicalDesty = logicalDestPoint.y() + logicalSourceRect.y();
    ASSERT(desty >= 0);
    ASSERT(desty < m_size.height());
    ASSERT(originy >= 0);
    ASSERT(originy <= scaledSourceRect.maxY());

    int endy = scaledDestPoint.y() + scaledSourceRect.maxY();
    int logicalEndy = logicalDestPoint.y() + logicalSourceRect.maxY();
    ASSERT(endy <= m_size.height());
    int numRows = endy - desty;
    int logicalNumRows = logicalEndy - logicalDesty;

    // The size of the derived surface is in BackingStoreCoordinateSystem.
    // We need to set the device scale for the derived surface from this ImageBuffer.
    IntRect imageRect(destx, desty, numColumns, numRows);
    NativeImagePtr imageSurface = m_data.m_bitmap;
    //cairoSurfaceSetDeviceScale(imageSurface.get(), m_resolutionScale, m_resolutionScale);
    destx = imageRect.x();
    desty = imageRect.y();

    uint8_t* pixelData = (uint8_t*)imageSurface->Bits();

    unsigned srcBytesPerRow = 4 * scaledSourceSize.width();
    int stride = imageSurface->BytesPerRow();

    const uint8_t* srcRows = imageData.data()->data() + originy * srcBytesPerRow + originx * 4;
    for (int y = 0; y < numRows; ++y) {
        unsigned* row = reinterpret_cast_ptr<unsigned*>(pixelData + stride * (y + desty));
        for (int x = 0; x < numColumns; x++) {
            int basex = x * 4;
            unsigned* pixel = row + x + destx;

            // Avoid calling Color::premultipliedARGBFromColor() because one
            // function call per pixel is too expensive.
            unsigned red = srcRows[basex];
            unsigned green = srcRows[basex + 1];
            unsigned blue = srcRows[basex + 2];
            unsigned alpha = srcRows[basex + 3];

            if (sourceFormat == AlphaPremultiplication::Unpremultiplied) {
                if (alpha != 255) {
                    red = (red * alpha + 254) / 255;
                    green = (green * alpha + 254) / 255;
                    blue = (blue * alpha + 254) / 255;
                }
            }

            *pixel = (alpha << 24) | red  << 16 | green  << 8 | blue;
        }
        srcRows += srcBytesPerRow;
    }

    // This cairo surface operation is done in LogicalCoordinateSystem.
    //cairo_surface_mark_dirty_rectangle(imageSurface.get(), logicalDestx, logicalDesty, logicalNumColumns, logicalNumRows);
}

static inline void convertFromData(const uint8* sourceRows, unsigned sourceBytesPerRow,
                                   uint8* destRows, unsigned destBytesPerRow,
                                   unsigned rows, unsigned columns)
{
    for (unsigned y = 0; y < rows; y++) {
        const uint8* s = sourceRows;
        uint8* d = destRows;
        for (unsigned x = 0; x < columns; x++) {
            // RGBA -> BGRA or BGRA -> RGBA
            d[0] = s[2];
            d[1] = s[1];
            d[2] = s[0];
            d[3] = s[3];
            d += 4;
            s += 4;
        }
        sourceRows += sourceBytesPerRow;
        destRows += destBytesPerRow;
    }
}

static inline void convertFromInternalData(const uint8* sourceRows, unsigned sourceBytesPerRow,
                                           uint8* destRows, unsigned destBytesPerRow,
                                           unsigned rows, unsigned columns,
                                           bool premultiplied)
{
    if (premultiplied) {
        // Internal storage is not pre-multiplied, pre-multiply on the fly.
        for (unsigned y = 0; y < rows; y++) {
            const uint8* s = sourceRows;
            uint8* d = destRows;
            for (unsigned x = 0; x < columns; x++) {
                // RGBA -> BGRA or BGRA -> RGBA
                d[0] = static_cast<uint16>(s[2]) * s[3] / 255;
                d[1] = static_cast<uint16>(s[1]) * s[3] / 255;
                d[2] = static_cast<uint16>(s[0]) * s[3] / 255;
                d[3] = s[3];
                d += 4;
                s += 4;
            }
            sourceRows += sourceBytesPerRow;
            destRows += destBytesPerRow;
        }
    } else {
        convertFromData(sourceRows, sourceBytesPerRow,
                        destRows, destBytesPerRow,
                        rows, columns);
    }
}

static inline void convertToInternalData(const uint8* sourceRows, unsigned sourceBytesPerRow,
                                         uint8* destRows, unsigned destBytesPerRow,
                                         unsigned rows, unsigned columns,
                                         bool premultiplied)
{
    if (premultiplied) {
        // Internal storage is not pre-multiplied, de-multiply source data.
        for (unsigned y = 0; y < rows; y++) {
            const uint8* s = sourceRows;
            uint8* d = destRows;
            for (unsigned x = 0; x < columns; x++) {
                // RGBA -> BGRA or BGRA -> RGBA
                if (s[3]) {
                    d[0] = static_cast<uint16>(s[2]) * 255 / s[3];
                    d[1] = static_cast<uint16>(s[1]) * 255 / s[3];
                    d[2] = static_cast<uint16>(s[0]) * 255 / s[3];
                    d[3] = s[3];
                } else {
                    d[0] = 0;
                    d[1] = 0;
                    d[2] = 0;
                    d[3] = 0;
                }
                d += 4;
                s += 4;
            }
            sourceRows += sourceBytesPerRow;
            destRows += destBytesPerRow;
        }
    } else {
        convertFromData(sourceRows, sourceBytesPerRow,
                        destRows, destBytesPerRow,
                        rows, columns);
    }
}

// TODO: PreserveResolution
String ImageBuffer::toDataURL(const String& mimeType, Optional<double> quality, PreserveResolution) const
{
    if (!MIMETypeRegistry::isSupportedImageMIMETypeForEncoding(mimeType))
        return "data:,";

    Vector<uint8_t> binaryBuffer = toData(mimeType, quality);

    if (binaryBuffer.size() == 0)
        return "data:,";

    Vector<char> encodedBuffer;
    base64Encode(binaryBuffer, encodedBuffer);

	return "data:" + mimeType + ";base64;" + encodedBuffer;
}


// TODO: quality
Vector<uint8_t> ImageBuffer::toData(const String& mimeType, Optional<double> /*quality*/) const
{
    BString mimeTypeString(mimeType);

    uint32 translatorType = 0;

    BTranslatorRoster* roster = BTranslatorRoster::Default();
    translator_id* translators;
    int32 translatorCount;
    roster->GetAllTranslators(&translators, &translatorCount);
    for (int32 i = 0; i < translatorCount; i++) {
        // Skip translators that don't support archived BBitmaps as input data.
        const translation_format* inputFormats;
        int32 formatCount;
        roster->GetInputFormats(translators[i], &inputFormats, &formatCount);
        bool supportsBitmaps = false;
        for (int32 j = 0; j < formatCount; j++) {
            if (inputFormats[j].type == B_TRANSLATOR_BITMAP) {
                supportsBitmaps = true;
                break;
            }
        }
        if (!supportsBitmaps)
            continue;

        const translation_format* outputFormats;
        roster->GetOutputFormats(translators[i], &outputFormats, &formatCount);
        for (int32 j = 0; j < formatCount; j++) {
            if (outputFormats[j].group == B_TRANSLATOR_BITMAP
                && mimeTypeString == outputFormats[j].MIME) {
                translatorType = outputFormats[j].type;
            }
        }
        if (translatorType)
            break;
    }


    BMallocIO translatedStream;
        // BBitmapStream doesn't take "const Bitmap*"...
    BBitmapStream bitmapStream(m_data.m_bitmap.get());
	BBitmap* tmp = NULL;
    if (roster->Translate(&bitmapStream, 0, 0, &translatedStream, translatorType,
                          B_TRANSLATOR_BITMAP, mimeType.utf8().data()) != B_OK) {
        bitmapStream.DetachBitmap(&tmp);
        return Vector<uint8_t>();
    }

    bitmapStream.DetachBitmap(&tmp);

    // FIXME we could use a BVectorIO to avoid an extra copy here
    Vector<uint8_t> result;
    off_t size;
    translatedStream.GetSize(&size);
    result.append((uint8_t*)translatedStream.Buffer(), size);

    return result;
}

} // namespace WebCore

