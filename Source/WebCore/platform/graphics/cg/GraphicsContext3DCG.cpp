/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#if USE(3D_GRAPHICS)

#include "GraphicsContext3D.h"

#include "BitmapImage.h"
#include "GraphicsContextCG.h"
#include "Image.h"

#include <CoreGraphics/CGBitmapContext.h>
#include <CoreGraphics/CGContext.h>
#include <CoreGraphics/CGDataProvider.h>
#include <CoreGraphics/CGImage.h>

#include <wtf/RetainPtr.h>

namespace WebCore {

enum SourceDataFormatBase {
    SourceFormatBaseR = 0,
    SourceFormatBaseA,
    SourceFormatBaseRA,
    SourceFormatBaseAR,
    SourceFormatBaseRGB,
    SourceFormatBaseRGBA,
    SourceFormatBaseARGB,
    SourceFormatBaseNumFormats
};

enum AlphaFormat {
    AlphaFormatNone = 0,
    AlphaFormatFirst,
    AlphaFormatLast,
    AlphaFormatNumFormats
};

// This returns SourceFormatNumFormats if the combination of input parameters is unsupported.
static GraphicsContext3D::SourceDataFormat getSourceDataFormat(unsigned int componentsPerPixel, AlphaFormat alphaFormat, bool is16BitFormat, bool bigEndian)
{
    const static SourceDataFormatBase formatTableBase[4][AlphaFormatNumFormats] = { // componentsPerPixel x AlphaFormat
        // AlphaFormatNone            AlphaFormatFirst            AlphaFormatLast
        { SourceFormatBaseR,          SourceFormatBaseA,          SourceFormatBaseA          }, // 1 componentsPerPixel
        { SourceFormatBaseNumFormats, SourceFormatBaseAR,         SourceFormatBaseRA         }, // 2 componentsPerPixel
        { SourceFormatBaseRGB,        SourceFormatBaseNumFormats, SourceFormatBaseNumFormats }, // 3 componentsPerPixel
        { SourceFormatBaseNumFormats, SourceFormatBaseARGB,       SourceFormatBaseRGBA        } // 4 componentsPerPixel
    };
    const static GraphicsContext3D::SourceDataFormat formatTable[SourceFormatBaseNumFormats][4] = { // SourceDataFormatBase x bitsPerComponent x endian
        // 8bits, little endian                 8bits, big endian                     16bits, little endian                        16bits, big endian
        { GraphicsContext3D::SourceFormatR8,    GraphicsContext3D::SourceFormatR8,    GraphicsContext3D::SourceFormatR16Little,    GraphicsContext3D::SourceFormatR16Big },
        { GraphicsContext3D::SourceFormatA8,    GraphicsContext3D::SourceFormatA8,    GraphicsContext3D::SourceFormatA16Little,    GraphicsContext3D::SourceFormatA16Big },
        { GraphicsContext3D::SourceFormatAR8,   GraphicsContext3D::SourceFormatRA8,   GraphicsContext3D::SourceFormatRA16Little,   GraphicsContext3D::SourceFormatRA16Big },
        { GraphicsContext3D::SourceFormatRA8,   GraphicsContext3D::SourceFormatAR8,   GraphicsContext3D::SourceFormatAR16Little,   GraphicsContext3D::SourceFormatAR16Big },
        { GraphicsContext3D::SourceFormatBGR8,  GraphicsContext3D::SourceFormatRGB8,  GraphicsContext3D::SourceFormatRGB16Little,  GraphicsContext3D::SourceFormatRGB16Big },
        { GraphicsContext3D::SourceFormatABGR8, GraphicsContext3D::SourceFormatRGBA8, GraphicsContext3D::SourceFormatRGBA16Little, GraphicsContext3D::SourceFormatRGBA16Big },
        { GraphicsContext3D::SourceFormatBGRA8, GraphicsContext3D::SourceFormatARGB8, GraphicsContext3D::SourceFormatARGB16Little, GraphicsContext3D::SourceFormatARGB16Big }
    };

    ASSERT(componentsPerPixel <= 4 && componentsPerPixel > 0);
    SourceDataFormatBase formatBase = formatTableBase[componentsPerPixel - 1][alphaFormat];
    if (formatBase == SourceFormatBaseNumFormats)
        return GraphicsContext3D::SourceFormatNumFormats;
    return formatTable[formatBase][(is16BitFormat ? 2 : 0) + (bigEndian ? 1 : 0)];
}

GraphicsContext3D::ImageExtractor::~ImageExtractor()
{
}

bool GraphicsContext3D::ImageExtractor::extractImage(bool premultiplyAlpha, bool ignoreGammaAndColorProfile)
{
    if (!m_image)
        return false;
    bool hasAlpha = m_image->isBitmapImage() ? m_image->currentFrameHasAlpha() : true;
    if ((ignoreGammaAndColorProfile || (hasAlpha && !premultiplyAlpha)) && m_image->data()) {
        ImageSource decoder(ImageSource::AlphaNotPremultiplied,
                            ignoreGammaAndColorProfile ? ImageSource::GammaAndColorProfileIgnored : ImageSource::GammaAndColorProfileApplied);
        decoder.setData(m_image->data(), true);
        if (!decoder.frameCount())
            return false;
        m_decodedImage.adoptCF(decoder.createFrameAtIndex(0));
        m_cgImage = m_decodedImage.get();
    } else
        m_cgImage = m_image->nativeImageForCurrentFrame();
    if (!m_cgImage)
        return false;

    m_imageWidth = CGImageGetWidth(m_cgImage);
    m_imageHeight = CGImageGetHeight(m_cgImage);
    if (!m_imageWidth || !m_imageHeight)
        return false;

    // See whether the image is using an indexed color space, and if
    // so, re-render it into an RGB color space. The image re-packing
    // code requires color data, not color table indices, for the
    // image data.
    CGColorSpaceRef colorSpace = CGImageGetColorSpace(m_cgImage);
    CGColorSpaceModel model = CGColorSpaceGetModel(colorSpace);
    if (model == kCGColorSpaceModelIndexed) {
        RetainPtr<CGContextRef> bitmapContext;
        // FIXME: we should probably manually convert the image by indexing into
        // the color table, which would allow us to avoid premultiplying the
        // alpha channel. Creation of a bitmap context with an alpha channel
        // doesn't seem to work unless it's premultiplied.
        bitmapContext.adoptCF(CGBitmapContextCreate(0, m_imageWidth, m_imageHeight, 8, m_imageWidth * 4,
                                                    deviceRGBColorSpaceRef(),
                                                    kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host));
        if (!bitmapContext)
            return false;

        CGContextSetBlendMode(bitmapContext.get(), kCGBlendModeCopy);
        CGContextSetInterpolationQuality(bitmapContext.get(), kCGInterpolationNone);
        CGContextDrawImage(bitmapContext.get(), CGRectMake(0, 0, m_imageWidth, m_imageHeight), m_cgImage);

        // Now discard the original CG image and replace it with a copy from the bitmap context.
        m_decodedImage.adoptCF(CGBitmapContextCreateImage(bitmapContext.get()));
        m_cgImage = m_decodedImage.get();
    }

    size_t bitsPerComponent = CGImageGetBitsPerComponent(m_cgImage);
    size_t bitsPerPixel = CGImageGetBitsPerPixel(m_cgImage);
    if (bitsPerComponent != 8 && bitsPerComponent != 16)
        return false;
    if (bitsPerPixel % bitsPerComponent)
        return false;
    size_t componentsPerPixel = bitsPerPixel / bitsPerComponent;

    CGBitmapInfo bitInfo = CGImageGetBitmapInfo(m_cgImage);
    bool bigEndianSource = false;
    // These could technically be combined into one large switch
    // statement, but we prefer not to so that we fail fast if we
    // encounter an unexpected image configuration.
    if (bitsPerComponent == 16) {
        switch (bitInfo & kCGBitmapByteOrderMask) {
        case kCGBitmapByteOrder16Big:
            bigEndianSource = true;
            break;
        case kCGBitmapByteOrder16Little:
            bigEndianSource = false;
            break;
        case kCGBitmapByteOrderDefault:
            // This is a bug in earlier version of cg where the default endian
            // is little whereas the decoded 16-bit png image data is actually
            // Big. Later version (10.6.4) no longer returns ByteOrderDefault.
            bigEndianSource = true;
            break;
        default:
            return false;
        }
    } else {
        switch (bitInfo & kCGBitmapByteOrderMask) {
        case kCGBitmapByteOrder32Big:
            bigEndianSource = true;
            break;
        case kCGBitmapByteOrder32Little:
            bigEndianSource = false;
            break;
        case kCGBitmapByteOrderDefault:
            // It appears that the default byte order is actually big
            // endian even on little endian architectures.
            bigEndianSource = true;
            break;
        default:
            return false;
        }
    }

    m_alphaOp = AlphaDoNothing;
    AlphaFormat alphaFormat = AlphaFormatNone;
    switch (CGImageGetAlphaInfo(m_cgImage)) {
    case kCGImageAlphaPremultipliedFirst:
        if (!premultiplyAlpha)
            m_alphaOp = AlphaDoUnmultiply;
        alphaFormat = AlphaFormatFirst;
        break;
    case kCGImageAlphaFirst:
        // This path is only accessible for MacOS earlier than 10.6.4.
        if (premultiplyAlpha)
            m_alphaOp = AlphaDoPremultiply;
        alphaFormat = AlphaFormatFirst;
        break;
    case kCGImageAlphaNoneSkipFirst:
        // This path is only accessible for MacOS earlier than 10.6.4.
        alphaFormat = AlphaFormatFirst;
        break;
    case kCGImageAlphaPremultipliedLast:
        if (!premultiplyAlpha)
            m_alphaOp = AlphaDoUnmultiply;
        alphaFormat = AlphaFormatLast;
        break;
    case kCGImageAlphaLast:
        if (premultiplyAlpha)
            m_alphaOp = AlphaDoPremultiply;
        alphaFormat = AlphaFormatLast;
        break;
    case kCGImageAlphaNoneSkipLast:
        alphaFormat = AlphaFormatLast;
        break;
    case kCGImageAlphaNone:
        alphaFormat = AlphaFormatNone;
        break;
    default:
        return false;
    }

    m_imageSourceFormat = getSourceDataFormat(componentsPerPixel, alphaFormat, bitsPerComponent == 16, bigEndianSource);
    if (m_imageSourceFormat == SourceFormatNumFormats)
        return false;

    m_pixelData.adoptCF(CGDataProviderCopyData(CGImageGetDataProvider(m_cgImage)));
    if (!m_pixelData)
        return false;

    m_imagePixelData = reinterpret_cast<const void*>(CFDataGetBytePtr(m_pixelData.get()));

    unsigned int srcUnpackAlignment = 0;
    size_t bytesPerRow = CGImageGetBytesPerRow(m_cgImage);
    unsigned padding = bytesPerRow - bitsPerPixel / 8 * m_imageWidth;
    if (padding) {
        srcUnpackAlignment = padding + 1;
        while (bytesPerRow % srcUnpackAlignment)
            ++srcUnpackAlignment;
    }

    m_imageSourceUnpackAlignment = srcUnpackAlignment;

    return true;
}

void GraphicsContext3D::paintToCanvas(const unsigned char* imagePixels, int imageWidth, int imageHeight, int canvasWidth, int canvasHeight, CGContextRef context)
{
    if (!imagePixels || imageWidth <= 0 || imageHeight <= 0 || canvasWidth <= 0 || canvasHeight <= 0 || !context)
        return;
    int rowBytes = imageWidth * 4;
    RetainPtr<CGDataProviderRef> dataProvider(AdoptCF, CGDataProviderCreateWithData(0, imagePixels, rowBytes * imageHeight, 0));
    RetainPtr<CGImageRef> cgImage(AdoptCF, CGImageCreate(imageWidth, imageHeight, 8, 32, rowBytes, deviceRGBColorSpaceRef(), kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host,
        dataProvider.get(), 0, false, kCGRenderingIntentDefault));
    // CSS styling may cause the canvas's content to be resized on
    // the page. Go back to the Canvas to figure out the correct
    // width and height to draw.
    CGRect rect = CGRectMake(0, 0, canvasWidth, canvasHeight);
    // We want to completely overwrite the previous frame's
    // rendering results.
    CGContextSaveGState(context);
    CGContextSetBlendMode(context, kCGBlendModeCopy);
    CGContextSetInterpolationQuality(context, kCGInterpolationNone);
    CGContextDrawImage(context, rect, cgImage.get());
    CGContextRestoreGState(context);
}

} // namespace WebCore

#endif // USE(3D_GRAPHICS)
