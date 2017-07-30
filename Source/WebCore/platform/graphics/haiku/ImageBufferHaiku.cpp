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

#include "GraphicsContext.h"
#include "ImageData.h"
#include "IntRect.h"
#include "JSCellInlines.h"
#include "JSCJSValueInlines.h"
#include "MIMETypeRegistry.h"
#include "NotImplemented.h"
#include "StillImageHaiku.h"
#include "TypedArrayInlines.h"
#include "TypedArrays.h"
#include <wtf/text/Base64.h>
#include <wtf/text/CString.h>
#include <BitmapStream.h>
#include <Picture.h>
#include <String.h>
#include <TranslatorRoster.h>
#include <stdio.h>


namespace WebCore {

ImageBufferData::ImageBufferData(const FloatSize& size)
    : m_bitmap(BRect(0, 0, size.width() - 1., size.height() - 1.), B_RGBA32, true)
    , m_view(NULL)
    , m_context(NULL)
{
    // Always keep the bitmap locked, we are the only client.
    m_bitmap.Lock();
    if(size.isEmpty())
        return;

    if (!m_bitmap.IsLocked() || !m_bitmap.IsValid())
        return;

    m_view = new BView(m_bitmap.Bounds(), "WebKit ImageBufferData", 0, 0);
    m_bitmap.AddChild(m_view);

    // Fill with completely transparent color.
    memset(m_bitmap.Bits(), 0, m_bitmap.BitsLength());

    // Since ImageBuffer is used mainly for Canvas, explicitly initialize
    // its view's graphics state with the corresponding canvas defaults
    // NOTE: keep in sync with CanvasRenderingContext2D::State
    m_view->SetLineMode(B_BUTT_CAP, B_MITER_JOIN, 10);
    m_view->SetDrawingMode(B_OP_ALPHA);
    m_view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
    m_context = new GraphicsContext(m_view);

    m_image = StillImage::createForRendering(&m_bitmap);
}

ImageBufferData::~ImageBufferData()
{
    m_view = nullptr;
        // m_bitmap owns m_view and deletes it when going out of this destructor.
    m_image = nullptr;
    delete m_context;

    m_bitmap.Unlock();
}

ImageBuffer::ImageBuffer(const FloatSize& size, float /* resolutionScale */, ColorSpace, RenderingMode, bool& success)
    : m_data(size)
    , m_size(size)
    , m_logicalSize(size)
{
    if (size.isEmpty()) {
        success = false;
        return;
    }

    success = m_data.m_view != nullptr;
}

ImageBuffer::~ImageBuffer()
{
}

GraphicsContext& ImageBuffer::context() const
{
    return *m_data.m_context;
}

RefPtr<Image> ImageBuffer::sinkIntoImage(std::unique_ptr<ImageBuffer> imageBuffer, ScaleBehavior scaleBehavior)
{
    return imageBuffer->copyImage(DontCopyBackingStore, scaleBehavior);
}

RefPtr<Image> ImageBuffer::copyImage(BackingStoreCopy copyBehavior, ScaleBehavior) const
{
    if (m_data.m_view)
        m_data.m_view->Sync();

    if (copyBehavior == CopyBackingStore)
        return StillImage::create(m_data.m_bitmap);

    return StillImage::createForRendering(&m_data.m_bitmap);
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
                       CompositeOperator op, BlendMode)
{
    if (!m_data.m_view)
        return;

    m_data.m_view->Sync();
    ImagePaintingOptions options;
    options.m_compositeOperator = op;
    if (&destContext == &context() && destRect.intersects(srcRect)) {
        // We're drawing into our own buffer.  In order for this to work, we need to copy the source buffer first.
        RefPtr<Image> copy = copyImage(CopyBackingStore);
        destContext.drawImage(*copy.get(), destRect, srcRect, options);
    } else
        destContext.drawImage(*m_data.m_image.get(), destRect, srcRect, options);
}

void ImageBuffer::drawPattern(GraphicsContext& destContext, const FloatRect& srcRect, const AffineTransform& patternTransform,
    const FloatPoint& phase, const FloatSize& size, CompositeOperator op, const FloatRect& destRect, BlendMode)
{
    if (!m_data.m_view)
        return;

    m_data.m_view->Sync();
    if (&destContext == &context() && srcRect.intersects(destRect)) {
        // We're drawing into our own buffer.  In order for this to work, we need to copy the source buffer first.
        RefPtr<Image> copy = copyImage(CopyBackingStore);
        copy->drawPattern(destContext, srcRect, patternTransform, phase, size, op, destRect);
    } else
        m_data.m_image->drawPattern(destContext, srcRect, patternTransform, phase, size, op, destRect);
}

void ImageBuffer::platformTransformColorSpace(const Vector<int>& lookUpTable)
{
    uint8* rowData = reinterpret_cast<uint8*>(m_data.m_bitmap.Bits());
    unsigned bytesPerRow = m_data.m_bitmap.BytesPerRow();
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

static PassRefPtr<Uint8ClampedArray> getImageData(const IntRect& rect, const ImageBufferData& imageData, const IntSize& size, bool premultiplied)
{
    RefPtr<Uint8ClampedArray> result = Uint8ClampedArray::createUninitialized(rect.width() * rect.height() * 4);
    unsigned char* data = result->data();

    // If the destination image is larger than the source image, the outside
    // regions need to be transparent. This way is simply, although with a
    // a slight overhead for the inside region.
    if (rect.x() < 0 || rect.y() < 0 || rect.maxX() > size.width() || rect.maxY() > size.height())
        result->zeroFill();

    // If the requested image is outside the source image, we can return at
    // this point.
    if (rect.x() > size.width() || rect.y() > size.height() || rect.maxX() < 0 || rect.maxY() < 0)
        return result;

    // Normalize the dest rectangle to not write before the allocated space,
    // when there are negative coordinates
    int originx = rect.x();
    int destx = 0;
    if (originx < 0) {
        destx = -originx;
        originx = 0;
    }
    int endx = rect.maxX();
    if (endx > size.width())
        endx = size.width();

    int originy = rect.y();
    int desty = 0;
    if (originy < 0) {
        desty = -originy;
        originy = 0;
    }
    int endy = rect.maxY();
    if (endy > size.height())
        endy = size.height();

    // Now we know there must be an intersection rect which we need to extract.
    BRect sourceRect(0, 0, size.width() - 1, size.height() - 1);
    sourceRect = BRect(rect) & sourceRect;

    unsigned destBytesPerRow = 4 * rect.width();
    unsigned char* destRows = data;
    // Offset the destination pointer to point at the first pixel of the
    // intersection rect.
    destRows += destx * 4 + desty * destBytesPerRow;

    const uint8* sourceRows = reinterpret_cast<const uint8*>(imageData.m_bitmap.Bits());
    uint32 sourceBytesPerRow = imageData.m_bitmap.BytesPerRow();
    // Offset the source pointer to point at the first pixel of the
    // intersection rect.
    sourceRows += (int)sourceRect.left * 4 + (int)sourceRect.top * sourceBytesPerRow;

    unsigned rows = sourceRect.IntegerHeight() + 1;
    unsigned columns = sourceRect.IntegerWidth() + 1;
    convertFromInternalData(sourceRows, sourceBytesPerRow, destRows, destBytesPerRow,
        rows, columns, premultiplied);

    return result.release();
}

PassRefPtr<Uint8ClampedArray> ImageBuffer::getUnmultipliedImageData(const IntRect& rect, CoordinateSystem) const
{
    // Make sure all asynchronous drawing has finished
    m_data.m_view->Sync();
    return getImageData(rect, m_data, m_size, false);
}

PassRefPtr<Uint8ClampedArray> ImageBuffer::getPremultipliedImageData(const IntRect& rect, CoordinateSystem) const
{
    // Make sure all asynchronous drawing has finished
    m_data.m_view->Sync();
    return getImageData(rect, m_data, m_size, true);
}

void ImageBuffer::putByteArray(Multiply multiplied, Uint8ClampedArray* source, const IntSize& sourceSize, const IntRect& sourceRect, const IntPoint& destPoint, CoordinateSystem)
{
    // Make sure all asynchronous drawing has finished
    m_data.m_view->Sync();

    // If the source image is outside the destination image, we can return at
    // this point.
    // TODO: Check if this isn't already done in WebCore.
    if (destPoint.x() > sourceSize.width() || destPoint.y() > sourceSize.height()
        || destPoint.x() + sourceRect.width() < 0
        || destPoint.y() + sourceRect.height() < 0) {
        return;
    }

    const unsigned char* sourceRows = source->data();
    unsigned sourceBytesPerRow = 4 * sourceSize.width();
    // Offset the source pointer to the first pixel of the source rect.
    sourceRows += sourceRect.x() * 4 + sourceRect.y() * sourceBytesPerRow;

    // We know there must be an intersection rect.
    BRect destRect(destPoint.x(), destPoint.y(), sourceRect.width() - 1, sourceRect.height() - 1);
    destRect = destRect & BRect(0, 0, sourceSize.width() - 1, sourceSize.height() - 1);

    unsigned char* destRows = reinterpret_cast<unsigned char*>(m_data.m_bitmap.Bits());
    uint32 destBytesPerRow = m_data.m_bitmap.BytesPerRow();
    // Offset the source pointer to point at the first pixel of the
    // intersection rect.
    destRows += (int)destRect.left * 4 + (int)destRect.top * destBytesPerRow;

    unsigned rows = sourceRect.height();
    unsigned columns = sourceRect.width();
    convertToInternalData(sourceRows, sourceBytesPerRow, destRows, destBytesPerRow,
        rows, columns, multiplied == Premultiplied);
}

// TODO: quality
String ImageBuffer::toDataURL(const String& mimeType, const double* /*quality*/, CoordinateSystem) const
{
    if (!MIMETypeRegistry::isSupportedImageMIMETypeForEncoding(mimeType))
        return "data:,";

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
    BBitmap* bitmap = const_cast<BBitmap*>(&m_data.m_bitmap);
        // BBitmapStream doesn't take "const Bitmap*"...
    BBitmapStream bitmapStream(bitmap);
    if (roster->Translate(&bitmapStream, 0, 0, &translatedStream, translatorType,
                          B_TRANSLATOR_BITMAP, mimeType.utf8().data()) != B_OK) {
        bitmapStream.DetachBitmap(&bitmap);
        return "data:,";
    }

    bitmapStream.DetachBitmap(&bitmap);

    Vector<char> encodedBuffer;
    base64Encode(reinterpret_cast<const char*>(translatedStream.Buffer()),
                 translatedStream.BufferLength(), encodedBuffer);

    return String::format("data:%s;base64,%.*s", mimeType.utf8().data(),
        (int)encodedBuffer.size(), encodedBuffer.data());
}

} // namespace WebCore

