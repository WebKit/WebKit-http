/*
 * Copyright (c) 2008, Google Inc. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "AffineTransform.h"
#include "BitmapImage.h"
#include "BitmapImageSingleFrameSkia.h"
#include "FloatConversion.h"
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "ImageObserver.h"
#include "Logging.h"
#include "NativeImageSkia.h"
#include "PlatformContextSkia.h"
#include "PlatformString.h"
#include "SkPixelRef.h"
#include "SkRect.h"
#include "SkShader.h"
#include "SkiaUtils.h"
#include "Texture.h"

#include "skia/ext/image_operations.h"
#include "skia/ext/platform_canvas.h"

namespace WebCore {

// Used by computeResamplingMode to tell how bitmaps should be resampled.
enum ResamplingMode {
    // Nearest neighbor resampling. Used when we detect that the page is
    // trying to make a pattern by stretching a small bitmap very large.
    RESAMPLE_NONE,

    // Default skia resampling. Used for large growing of images where high
    // quality resampling doesn't get us very much except a slowdown.
    RESAMPLE_LINEAR,

    // High quality resampling.
    RESAMPLE_AWESOME,
};

static ResamplingMode computeResamplingMode(PlatformContextSkia* platformContext, const NativeImageSkia& bitmap, int srcWidth, int srcHeight, float destWidth, float destHeight)
{
    if (platformContext->hasImageResamplingHint()) {
        IntSize srcSize;
        FloatSize dstSize;
        platformContext->getImageResamplingHint(&srcSize, &dstSize);
        srcWidth = srcSize.width();
        srcHeight = srcSize.height();
        destWidth = dstSize.width();
        destHeight = dstSize.height();
    }

    int destIWidth = static_cast<int>(destWidth);
    int destIHeight = static_cast<int>(destHeight);

    // The percent change below which we will not resample. This usually means
    // an off-by-one error on the web page, and just doing nearest neighbor
    // sampling is usually good enough.
    const float kFractionalChangeThreshold = 0.025f;

    // Images smaller than this in either direction are considered "small" and
    // are not resampled ever (see below).
    const int kSmallImageSizeThreshold = 8;

    // The amount an image can be stretched in a single direction before we
    // say that it is being stretched so much that it must be a line or
    // background that doesn't need resampling.
    const float kLargeStretch = 3.0f;

    // Figure out if we should resample this image. We try to prune out some
    // common cases where resampling won't give us anything, since it is much
    // slower than drawing stretched.
    if (srcWidth == destIWidth && srcHeight == destIHeight) {
        // We don't need to resample if the source and destination are the same.
        return RESAMPLE_NONE;
    }

    if (srcWidth <= kSmallImageSizeThreshold
        || srcHeight <= kSmallImageSizeThreshold
        || destWidth <= kSmallImageSizeThreshold
        || destHeight <= kSmallImageSizeThreshold) {
        // Never resample small images. These are often used for borders and
        // rules (think 1x1 images used to make lines).
        return RESAMPLE_NONE;
    }

    if (srcHeight * kLargeStretch <= destHeight || srcWidth * kLargeStretch <= destWidth) {
        // Large image detected.

        // Don't resample if it is being stretched a lot in only one direction.
        // This is trying to catch cases where somebody has created a border
        // (which might be large) and then is stretching it to fill some part
        // of the page.
        if (srcWidth == destWidth || srcHeight == destHeight)
            return RESAMPLE_NONE;

        // The image is growing a lot and in more than one direction. Resampling
        // is slow and doesn't give us very much when growing a lot.
        return RESAMPLE_LINEAR;
    }

    if ((fabs(destWidth - srcWidth) / srcWidth < kFractionalChangeThreshold)
        && (fabs(destHeight - srcHeight) / srcHeight < kFractionalChangeThreshold)) {
        // It is disappointingly common on the web for image sizes to be off by
        // one or two pixels. We don't bother resampling if the size difference
        // is a small fraction of the original size.
        return RESAMPLE_NONE;
    }

    // When the image is not yet done loading, use linear. We don't cache the
    // partially resampled images, and as they come in incrementally, it causes
    // us to have to resample the whole thing every time.
    if (!bitmap.isDataComplete())
        return RESAMPLE_LINEAR;

    // Everything else gets resampled.
    // If the platform context permits high quality interpolation, use it.
    // High quality interpolation only enabled for scaling and translation.
    if (platformContext->interpolationQuality() == InterpolationHigh
        && !(platformContext->canvas()->getTotalMatrix().getType() & (SkMatrix::kAffine_Mask | SkMatrix::kPerspective_Mask)))
        return RESAMPLE_AWESOME;
    
    return RESAMPLE_LINEAR;
}

// Draws the given bitmap to the given canvas. The subset of the source bitmap
// identified by src_rect is drawn to the given destination rect. The bitmap
// will be resampled to resample_width * resample_height (this is the size of
// the whole image, not the subset). See shouldResampleBitmap for more.
//
// This does a lot of computation to resample only the portion of the bitmap
// that will only be drawn. This is critical for performance since when we are
// scrolling, for example, we are only drawing a small strip of the image.
// Resampling the whole image every time is very slow, so this speeds up things
// dramatically.
static void drawResampledBitmap(SkCanvas& canvas, SkPaint& paint, const NativeImageSkia& bitmap, const SkIRect& srcIRect, const SkRect& destRect)
{
    // First get the subset we need. This is efficient and does not copy pixels.
    SkBitmap subset;
    bitmap.extractSubset(&subset, srcIRect);
    SkRect srcRect;
    srcRect.set(srcIRect);

    // Whether we're doing a subset or using the full source image.
    bool srcIsFull = srcIRect.fLeft == 0 && srcIRect.fTop == 0
        && srcIRect.width() == bitmap.width()
        && srcIRect.height() == bitmap.height();

    // We will always draw in integer sizes, so round the destination rect.
    SkIRect destRectRounded;
    destRect.round(&destRectRounded);
    SkIRect resizedImageRect =  // Represents the size of the resized image.
        { 0, 0, destRectRounded.width(), destRectRounded.height() };

    // Apply forward transform to destRect to estimate required size of
    // re-sampled bitmap, and use only in calls required to resize, or that
    // check for the required size.
    SkRect destRectTransformed;
    canvas.getTotalMatrix().mapRect(&destRectTransformed, destRect);
    SkIRect destRectTransformedRounded;
    destRectTransformed.round(&destRectTransformedRounded);

    if (srcIsFull && bitmap.hasResizedBitmap(destRectTransformedRounded.width(), destRectTransformedRounded.height())) {
        // Yay, this bitmap frame already has a resized version.
        SkBitmap resampled = bitmap.resizedBitmap(destRectTransformedRounded.width(), destRectTransformedRounded.height());
        canvas.drawBitmapRect(resampled, 0, destRect, &paint);
        return;
    }

    // Compute the visible portion of our rect.
    // We also need to compute the transformed portion of the
    // visible portion for use below.
    SkRect destBitmapSubsetSk;
    ClipRectToCanvas(canvas, destRect, &destBitmapSubsetSk);
    SkRect destBitmapSubsetTransformed;
    canvas.getTotalMatrix().mapRect(&destBitmapSubsetTransformed, destBitmapSubsetSk);
    destBitmapSubsetSk.offset(-destRect.fLeft, -destRect.fTop);
    SkIRect destBitmapSubsetTransformedRounded;
    destBitmapSubsetTransformed.round(&destBitmapSubsetTransformedRounded);
    destBitmapSubsetTransformedRounded.offset(-destRectTransformedRounded.fLeft, -destRectTransformedRounded.fTop);

    // The matrix inverting, etc. could have introduced rounding error which
    // causes the bounds to be outside of the resized bitmap. We round outward
    // so we always lean toward it being larger rather than smaller than we
    // need, and then clamp to the bitmap bounds so we don't get any invalid
    // data.
    SkIRect destBitmapSubsetSkI;
    destBitmapSubsetSk.roundOut(&destBitmapSubsetSkI);
    if (!destBitmapSubsetSkI.intersect(resizedImageRect))
        return;  // Resized image does not intersect.

    if (srcIsFull && bitmap.shouldCacheResampling(
            resizedImageRect.width(),
            resizedImageRect.height(),
            destBitmapSubsetSkI.width(),
            destBitmapSubsetSkI.height())) {
        // We're supposed to resize the entire image and cache it, even though
        // we don't need all of it.
        SkBitmap resampled = bitmap.resizedBitmap(destRectTransformedRounded.width(),
                                                  destRectTransformedRounded.height());
        canvas.drawBitmapRect(resampled, 0, destRect, &paint);
    } else {
        // We should only resize the exposed part of the bitmap to do the
        // minimal possible work.

        // Resample the needed part of the image.
        // Transforms above plus rounding may cause destBitmapSubsetTransformedRounded
        // to go outside the image, so need to clip to avoid problems.
        if (destBitmapSubsetTransformedRounded.intersect(0, 0,
                destRectTransformedRounded.width(), destRectTransformedRounded.height())) {

            SkBitmap resampled = skia::ImageOperations::Resize(subset,
                skia::ImageOperations::RESIZE_LANCZOS3,
                destRectTransformedRounded.width(), destRectTransformedRounded.height(),
                destBitmapSubsetTransformedRounded);

            // Compute where the new bitmap should be drawn. Since our new bitmap
            // may be smaller than the original, we have to shift it over by the
            // same amount that we cut off the top and left.
            destBitmapSubsetSkI.offset(destRect.fLeft, destRect.fTop);
            SkRect offsetDestRect;
            offsetDestRect.set(destBitmapSubsetSkI);

            canvas.drawBitmapRect(resampled, 0, offsetDestRect, &paint);
        }
    }
}

static void paintSkBitmap(PlatformContextSkia* platformContext, const NativeImageSkia& bitmap, const SkIRect& srcRect, const SkRect& destRect, const SkXfermode::Mode& compOp)
{
    SkPaint paint;
    paint.setXfermodeMode(compOp);
    paint.setFilterBitmap(true);
    paint.setAlpha(platformContext->getNormalizedAlpha());
    paint.setLooper(platformContext->getDrawLooper());

    SkCanvas* canvas = platformContext->canvas();

    ResamplingMode resampling;
    if (platformContext->isAccelerated())
        resampling = RESAMPLE_LINEAR;
    else
        resampling = platformContext->printing() ? RESAMPLE_NONE :
            computeResamplingMode(platformContext, bitmap, srcRect.width(), srcRect.height(), SkScalarToFloat(destRect.width()), SkScalarToFloat(destRect.height()));
    if (resampling == RESAMPLE_AWESOME) {
        drawResampledBitmap(*canvas, paint, bitmap, srcRect, destRect);
    } else {
        // No resampling necessary, we can just draw the bitmap. We want to
        // filter it if we decided to do linear interpolation above, or if there
        // is something interesting going on with the matrix (like a rotation).
        // Note: for serialization, we will want to subset the bitmap first so
        // we don't send extra pixels.
        canvas->drawBitmapRect(bitmap, &srcRect, destRect, &paint);
    }
}

// Transforms the given dimensions with the given matrix. Used to see how big
// images will be once transformed.
static void TransformDimensions(const SkMatrix& matrix, float srcWidth, float srcHeight, float* destWidth, float* destHeight) {
    // Transform 3 points to see how long each side of the bitmap will be.
    SkPoint src_points[3];  // (0, 0), (width, 0), (0, height).
    src_points[0].set(0, 0);
    src_points[1].set(SkFloatToScalar(srcWidth), 0);
    src_points[2].set(0, SkFloatToScalar(srcHeight));

    // Now measure the length of the two transformed vectors relative to the
    // transformed origin to see how big the bitmap will be. Note: for skews,
    // this isn't the best thing, but we don't have skews.
    SkPoint dest_points[3];
    matrix.mapPoints(dest_points, src_points, 3);
    *destWidth = SkScalarToFloat((dest_points[1] - dest_points[0]).length());
    *destHeight = SkScalarToFloat((dest_points[2] - dest_points[0]).length());
}

// A helper method for translating negative width and height values.
FloatRect normalizeRect(const FloatRect& rect)
{
    FloatRect norm = rect;
    if (norm.width() < 0) {
        norm.setX(norm.x() + norm.width());
        norm.setWidth(-norm.width());
    }
    if (norm.height() < 0) {
        norm.setY(norm.y() + norm.height());
        norm.setHeight(-norm.height());
    }
    return norm;
}

bool FrameData::clear(bool clearMetadata)
{
    if (clearMetadata)
        m_haveMetadata = false;

    if (m_frame) {
        // ImageSource::createFrameAtIndex() allocated |m_frame| and passed
        // ownership to BitmapImage; we must delete it here.
        delete m_frame;
        m_frame = 0;
        return true;
    }
    return false;
}

void Image::drawPattern(GraphicsContext* context,
                        const FloatRect& floatSrcRect,
                        const AffineTransform& patternTransform,
                        const FloatPoint& phase,
                        ColorSpace styleColorSpace,
                        CompositeOperator compositeOp,
                        const FloatRect& destRect)
{
    FloatRect normSrcRect = normalizeRect(floatSrcRect);
    if (destRect.isEmpty() || normSrcRect.isEmpty())
        return;  // nothing to draw

    NativeImageSkia* bitmap = nativeImageForCurrentFrame();
    if (!bitmap)
        return;

    // This is a very inexpensive operation. It will generate a new bitmap but
    // it will internally reference the old bitmap's pixels, adjusting the row
    // stride so the extra pixels appear as padding to the subsetted bitmap.
    SkBitmap srcSubset;
    SkIRect srcRect = enclosingIntRect(normSrcRect);
    bitmap->extractSubset(&srcSubset, srcRect);

    SkBitmap resampled;
    SkShader* shader;

    // Figure out what size the bitmap will be in the destination. The
    // destination rect is the bounds of the pattern, we need to use the
    // matrix to see how bit it will be.
    float destBitmapWidth, destBitmapHeight;
    TransformDimensions(patternTransform, srcRect.width(), srcRect.height(),
                        &destBitmapWidth, &destBitmapHeight);

    // Compute the resampling mode.
    ResamplingMode resampling;
    if (context->platformContext()->isAccelerated())
        resampling = RESAMPLE_LINEAR;
    else {
        if (context->platformContext()->printing())
            resampling = RESAMPLE_LINEAR;
        else
            resampling = computeResamplingMode(context->platformContext(), *bitmap, srcRect.width(), srcRect.height(), destBitmapWidth, destBitmapHeight);
    }

    // Load the transform WebKit requested.
    SkMatrix matrix(patternTransform);

    if (resampling == RESAMPLE_AWESOME) {
        // Do nice resampling.
        SkBitmap resampled;
        int width = static_cast<int>(destBitmapWidth);
        int height = static_cast<int>(destBitmapHeight);
        if (!srcRect.fLeft && !srcRect.fTop
            && srcRect.fRight == bitmap->width() && srcRect.fBottom == bitmap->height()
            && (bitmap->hasResizedBitmap(width, height)
                || bitmap->shouldCacheResampling(width, height, width, height))) {
            // resizedBitmap() caches resized image.
            resampled = bitmap->resizedBitmap(width, height);
        } else {
            resampled = skia::ImageOperations::Resize(srcSubset,
                skia::ImageOperations::RESIZE_LANCZOS3, width, height);
        }
        shader = SkShader::CreateBitmapShader(resampled, SkShader::kRepeat_TileMode, SkShader::kRepeat_TileMode);

        // Since we just resized the bitmap, we need to undo the scale set in
        // the image transform.
        matrix.setScaleX(SkIntToScalar(1));
        matrix.setScaleY(SkIntToScalar(1));
    } else {
        // No need to do nice resampling.
        shader = SkShader::CreateBitmapShader(srcSubset, SkShader::kRepeat_TileMode, SkShader::kRepeat_TileMode);
    }

    // We also need to translate it such that the origin of the pattern is the
    // origin of the destination rect, which is what WebKit expects. Skia uses
    // the coordinate system origin as the base for the patter. If WebKit wants
    // a shifted image, it will shift it from there using the patternTransform.
    float adjustedX = phase.x() + normSrcRect.x() *
                      narrowPrecisionToFloat(patternTransform.a());
    float adjustedY = phase.y() + normSrcRect.y() *
                      narrowPrecisionToFloat(patternTransform.d());
    matrix.postTranslate(SkFloatToScalar(adjustedX),
                         SkFloatToScalar(adjustedY));
    shader->setLocalMatrix(matrix);

    SkPaint paint;
    paint.setShader(shader)->unref();
    paint.setXfermodeMode(WebCoreCompositeToSkiaComposite(compositeOp));
    paint.setFilterBitmap(resampling == RESAMPLE_LINEAR);

    context->platformContext()->paintSkPaint(destRect, paint);
}

// ================================================
// BitmapImage Class
// ================================================

// FIXME: These should go to BitmapImageSkia.cpp

void BitmapImage::initPlatformData()
{
    // This is not used. On Mac, the "platform" data is a cache of some OS
    // specific versions of the image that are created is some cases. These
    // aren't normally used, it is equivalent to getHBITMAP on Windows, and
    // the platform data is the cache.
}

void BitmapImage::invalidatePlatformData()
{
    // See initPlatformData above.
}

void BitmapImage::checkForSolidColor()
{
    m_isSolidColor = false;
    m_checkedForSolidColor = true;

    if (frameCount() > 1)
        return;

    WebCore::NativeImageSkia* frame = frameAtIndex(0);

    if (frame && size().width() == 1 && size().height() == 1) {
        SkAutoLockPixels lock(*frame);
        if (!frame->getPixels())
            return;

        m_isSolidColor = true;
        m_solidColor = Color(frame->getColor(0, 0));
    }
}

void BitmapImage::draw(GraphicsContext* ctxt, const FloatRect& dstRect,
                       const FloatRect& srcRect, ColorSpace colorSpace, CompositeOperator compositeOp)
{
    if (!m_source.initialized())
        return;

    // Spin the animation to the correct frame before we try to draw it, so we
    // don't draw an old frame and then immediately need to draw a newer one,
    // causing flicker and wasting CPU.
    startAnimation();

    NativeImageSkia* bm = nativeImageForCurrentFrame();
    if (!bm)
        return;  // It's too early and we don't have an image yet.

    FloatRect normDstRect = normalizeRect(dstRect);
    FloatRect normSrcRect = normalizeRect(srcRect);

    if (normSrcRect.isEmpty() || normDstRect.isEmpty())
        return;  // Nothing to draw.

    ctxt->platformContext()->makeGrContextCurrent();

    paintSkBitmap(ctxt->platformContext(),
                  *bm,
                  enclosingIntRect(normSrcRect),
                  normDstRect,
                  WebCoreCompositeToSkiaComposite(compositeOp));

    if (ImageObserver* observer = imageObserver())
        observer->didDraw(this);
}

// FIXME: These should go into BitmapImageSingleFrameSkia.cpp

void BitmapImageSingleFrameSkia::draw(GraphicsContext* ctxt,
                                      const FloatRect& dstRect,
                                      const FloatRect& srcRect,
                                      ColorSpace styleColorSpace,
                                      CompositeOperator compositeOp)
{
    FloatRect normDstRect = normalizeRect(dstRect);
    FloatRect normSrcRect = normalizeRect(srcRect);

    if (normSrcRect.isEmpty() || normDstRect.isEmpty())
        return;  // Nothing to draw.

    ctxt->platformContext()->makeGrContextCurrent();

    paintSkBitmap(ctxt->platformContext(),
                  m_nativeImage,
                  enclosingIntRect(normSrcRect),
                  normDstRect,
                  WebCoreCompositeToSkiaComposite(compositeOp));

    if (ImageObserver* observer = imageObserver())
        observer->didDraw(this);
}

BitmapImageSingleFrameSkia::BitmapImageSingleFrameSkia(const SkBitmap& bitmap)
    : m_nativeImage(bitmap)
{
}

PassRefPtr<BitmapImageSingleFrameSkia> BitmapImageSingleFrameSkia::create(const SkBitmap& bitmap, bool copyPixels)
{
    if (copyPixels) {
        SkBitmap temp;
        bitmap.copyTo(&temp, bitmap.config());
        return adoptRef(new BitmapImageSingleFrameSkia(temp));
    }
    return adoptRef(new BitmapImageSingleFrameSkia(bitmap));
}

}  // namespace WebCore
