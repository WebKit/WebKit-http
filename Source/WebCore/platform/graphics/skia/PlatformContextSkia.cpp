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

#include "PlatformContextSkia.h"

#include "AffineTransform.h"
#include "DrawingBuffer.h"
#include "Extensions3D.h"
#include "GraphicsContext.h"
#include "GraphicsContext3D.h"
#include "ImageBuffer.h"
#include "NativeImageSkia.h"
#include "SkiaUtils.h"
#include "Texture.h"
#include "TilingData.h"

#include "skia/ext/image_operations.h"
#include "skia/ext/platform_canvas.h"

#include "SkBitmap.h"
#include "SkColorPriv.h"
#include "SkDashPathEffect.h"
#include "SkShader.h"

#include "GrContext.h"
#include "SkGpuDevice.h"

#include <wtf/MathExtras.h>
#include <wtf/Vector.h>

namespace WebCore {

extern bool isPathSkiaSafe(const SkMatrix& transform, const SkPath& path);

// State -----------------------------------------------------------------------

// Encapsulates the additional painting state information we store for each
// pushed graphics state.
struct PlatformContextSkia::State {
    State();
    State(const State&);
    ~State();

    // Common shader state.
    float m_alpha;
    SkXfermode::Mode m_xferMode;
    bool m_useAntialiasing;
    SkDrawLooper* m_looper;

    // Fill.
    SkColor m_fillColor;

    // Stroke.
    StrokeStyle m_strokeStyle;
    SkColor m_strokeColor;
    float m_strokeThickness;
    int m_dashRatio;  // Ratio of the length of a dash to its width.
    float m_miterLimit;
    SkPaint::Cap m_lineCap;
    SkPaint::Join m_lineJoin;
    SkDashPathEffect* m_dash;

    // Text. (See TextModeFill & friends in GraphicsContext.h.)
    TextDrawingModeFlags m_textDrawingMode;

    // Helper function for applying the state's alpha value to the given input
    // color to produce a new output color.
    SkColor applyAlpha(SkColor) const;

    // If non-empty, the current State is clipped to this image.
    SkBitmap m_imageBufferClip;
    // If m_imageBufferClip is non-empty, this is the region the image is clipped to.
    FloatRect m_clip;

    // This is a list of clipping paths which are currently active, in the
    // order in which they were pushed.
    WTF::Vector<SkPath> m_antiAliasClipPaths;
    InterpolationQuality m_interpolationQuality;

    PlatformContextSkia::State cloneInheritedProperties();
private:
    // Not supported.
    void operator=(const State&);
};

// Note: Keep theses default values in sync with GraphicsContextState.
PlatformContextSkia::State::State()
    : m_alpha(1)
    , m_xferMode(SkXfermode::kSrcOver_Mode)
    , m_useAntialiasing(true)
    , m_looper(0)
    , m_fillColor(0xFF000000)
    , m_strokeStyle(SolidStroke)
    , m_strokeColor(Color::black)
    , m_strokeThickness(0)
    , m_dashRatio(3)
    , m_miterLimit(4)
    , m_lineCap(SkPaint::kDefault_Cap)
    , m_lineJoin(SkPaint::kDefault_Join)
    , m_dash(0)
    , m_textDrawingMode(TextModeFill)
    , m_interpolationQuality(InterpolationHigh)
{
}

PlatformContextSkia::State::State(const State& other)
    : m_alpha(other.m_alpha)
    , m_xferMode(other.m_xferMode)
    , m_useAntialiasing(other.m_useAntialiasing)
    , m_looper(other.m_looper)
    , m_fillColor(other.m_fillColor)
    , m_strokeStyle(other.m_strokeStyle)
    , m_strokeColor(other.m_strokeColor)
    , m_strokeThickness(other.m_strokeThickness)
    , m_dashRatio(other.m_dashRatio)
    , m_miterLimit(other.m_miterLimit)
    , m_lineCap(other.m_lineCap)
    , m_lineJoin(other.m_lineJoin)
    , m_dash(other.m_dash)
    , m_textDrawingMode(other.m_textDrawingMode)
    , m_imageBufferClip(other.m_imageBufferClip)
    , m_clip(other.m_clip)
    , m_antiAliasClipPaths(other.m_antiAliasClipPaths)
    , m_interpolationQuality(other.m_interpolationQuality)
{
    // Up the ref count of these. SkSafeRef does nothing if its argument is 0.
    SkSafeRef(m_looper);
    SkSafeRef(m_dash);
}

PlatformContextSkia::State::~State()
{
    SkSafeUnref(m_looper);
    SkSafeUnref(m_dash);
}

// Returns a new State with all of this object's inherited properties copied.
PlatformContextSkia::State PlatformContextSkia::State::cloneInheritedProperties()
{
    PlatformContextSkia::State state(*this);

    // Everything is inherited except for the clip paths.
    state.m_antiAliasClipPaths.clear();

    return state;
}

SkColor PlatformContextSkia::State::applyAlpha(SkColor c) const
{
    int s = roundf(m_alpha * 256);
    if (s >= 256)
        return c;
    if (s < 0)
        return 0;

    int a = SkAlphaMul(SkColorGetA(c), s);
    return (c & 0x00FFFFFF) | (a << 24);
}

// PlatformContextSkia ---------------------------------------------------------

// Danger: canvas can be NULL.
PlatformContextSkia::PlatformContextSkia(SkCanvas* canvas)
    : m_canvas(canvas)
    , m_printing(false)
    , m_drawingToImageBuffer(false)
    , m_gpuContext(0)
{
    m_stateStack.append(State());
    m_state = &m_stateStack.last();

    // will be assigned in setGraphicsContext()
    m_gc = 0;
}

PlatformContextSkia::~PlatformContextSkia()
{
}

void PlatformContextSkia::setCanvas(SkCanvas* canvas)
{
    m_canvas = canvas;
}

void PlatformContextSkia::setDrawingToImageBuffer(bool value)
{
    m_drawingToImageBuffer = value;
}

bool PlatformContextSkia::isDrawingToImageBuffer() const
{
    return m_drawingToImageBuffer;
}

void PlatformContextSkia::save()
{
    ASSERT(!hasImageResamplingHint());

    m_stateStack.append(m_state->cloneInheritedProperties());
    m_state = &m_stateStack.last();

    // The clip image only needs to be applied once. Reset the image so that we
    // don't attempt to clip multiple times.
    m_state->m_imageBufferClip.reset();

    // Save our native canvas.
    canvas()->save();
}

void PlatformContextSkia::beginLayerClippedToImage(const FloatRect& rect,
                                                   const ImageBuffer* imageBuffer)
{
    // Skia doesn't support clipping to an image, so we create a layer. The next
    // time restore is invoked the layer and |imageBuffer| are combined to
    // create the resulting image.
    m_state->m_clip = rect;
    SkRect bounds = { SkFloatToScalar(rect.x()), SkFloatToScalar(rect.y()),
                      SkFloatToScalar(rect.maxX()), SkFloatToScalar(rect.maxY()) };

    canvas()->clipRect(bounds);
    canvas()->saveLayerAlpha(&bounds, 255,
                             static_cast<SkCanvas::SaveFlags>(SkCanvas::kHasAlphaLayer_SaveFlag | SkCanvas::kFullColorLayer_SaveFlag));
    // Copy off the image as |imageBuffer| may be deleted before restore is invoked.
    const SkBitmap* bitmap = imageBuffer->context()->platformContext()->bitmap();
    if (!bitmap->pixelRef()) {
        // The bitmap owns it's pixels. This happens when we've allocated the
        // pixels in some way and assigned them directly to the bitmap (as
        // happens when we allocate a DIB). In this case the assignment operator
        // does not copy the pixels, rather the copied bitmap ends up
        // referencing the same pixels. As the pixels may not live as long as we
        // need it to, we copy the image.
        bitmap->copyTo(&m_state->m_imageBufferClip, SkBitmap::kARGB_8888_Config);
    } else {
        // If there is a pixel ref, we can safely use the assignment operator.
        m_state->m_imageBufferClip = *bitmap;
    }
}

void PlatformContextSkia::clipPathAntiAliased(const SkPath& clipPath)
{
    makeGrContextCurrent();
    if (m_canvas->getTopDevice()->getDeviceCapabilities() & SkDevice::kVector_Capability) {
        // When the output is a vector device, like PDF, we don't need antialiased clips.
        // It's up to the PDF rendering engine to do that. We can simply disable the
        // antialiased clip code if the output is a vector device.
        canvas()->clipPath(clipPath);
        return;
    }

    // If we are currently tracking any anti-alias clip paths, then we already
    // have a layer in place and don't need to add another.
    bool haveLayerOutstanding = m_state->m_antiAliasClipPaths.size();

    // See comments in applyAntiAliasedClipPaths about how this works.
    m_state->m_antiAliasClipPaths.append(clipPath);

    if (!haveLayerOutstanding) {
        SkRect bounds = clipPath.getBounds();
        canvas()->saveLayerAlpha(&bounds, 255, static_cast<SkCanvas::SaveFlags>(SkCanvas::kHasAlphaLayer_SaveFlag | SkCanvas::kFullColorLayer_SaveFlag | SkCanvas::kClipToLayer_SaveFlag));
        // Guards state modification during clipped operations.
        // The state is popped in applyAntiAliasedClipPaths().
        canvas()->save();
    }
}

void PlatformContextSkia::restore()
{
    if (!m_state->m_imageBufferClip.empty()) {
        applyClipFromImage(m_state->m_clip, m_state->m_imageBufferClip);
        canvas()->restore();
    }

    if (!m_state->m_antiAliasClipPaths.isEmpty())
        applyAntiAliasedClipPaths(m_state->m_antiAliasClipPaths);

    m_stateStack.removeLast();
    m_state = &m_stateStack.last();

    // Restore our native canvas.
    canvas()->restore();
}

void PlatformContextSkia::drawRect(SkRect rect)
{
    SkPaint paint;
    int fillcolorNotTransparent = m_state->m_fillColor & 0xFF000000;
    if (fillcolorNotTransparent) {
        setupPaintForFilling(&paint);
        canvas()->drawRect(rect, paint);
    }

    if (m_state->m_strokeStyle != NoStroke
        && (m_state->m_strokeColor & 0xFF000000)) {
        // We do a fill of four rects to simulate the stroke of a border.
        paint.reset();
        setupPaintForFilling(&paint);
        // need to jam in the strokeColor
        paint.setColor(this->effectiveStrokeColor());

        SkRect topBorder = { rect.fLeft, rect.fTop, rect.fRight, rect.fTop + 1 };
        canvas()->drawRect(topBorder, paint);
        SkRect bottomBorder = { rect.fLeft, rect.fBottom - 1, rect.fRight, rect.fBottom };
        canvas()->drawRect(bottomBorder, paint);
        SkRect leftBorder = { rect.fLeft, rect.fTop + 1, rect.fLeft + 1, rect.fBottom - 1 };
        canvas()->drawRect(leftBorder, paint);
        SkRect rightBorder = { rect.fRight - 1, rect.fTop + 1, rect.fRight, rect.fBottom - 1 };
        canvas()->drawRect(rightBorder, paint);
    }
}

void PlatformContextSkia::setupPaintCommon(SkPaint* paint) const
{
#if defined(SK_DEBUG)
    {
        SkPaint defaultPaint;
        SkASSERT(*paint == defaultPaint);
    }
#endif

    paint->setAntiAlias(m_state->m_useAntialiasing);
    paint->setXfermodeMode(m_state->m_xferMode);
    paint->setLooper(m_state->m_looper);
}

void PlatformContextSkia::setupShader(SkPaint* paint, Gradient* grad, Pattern* pat, SkColor color) const
{
    SkShader* shader = 0;

    if (grad) {
        shader = grad->platformGradient();
        color = SK_ColorBLACK;
    } else if (pat) {
        shader = pat->platformPattern(m_gc->getCTM());
        color = SK_ColorBLACK;
    }

    paint->setColor(m_state->applyAlpha(color));
    paint->setShader(shader);
}

void PlatformContextSkia::setupPaintForFilling(SkPaint* paint) const
{
    setupPaintCommon(paint);

    const GraphicsContextState& state = m_gc->state();
    setupShader(paint, state.fillGradient.get(), state.fillPattern.get(), m_state->m_fillColor);
}

float PlatformContextSkia::setupPaintForStroking(SkPaint* paint, SkRect* rect, int length) const
{
    setupPaintCommon(paint);

    const GraphicsContextState& state = m_gc->state();
    setupShader(paint, state.strokeGradient.get(), state.strokePattern.get(), m_state->m_strokeColor);

    float width = m_state->m_strokeThickness;

    paint->setStyle(SkPaint::kStroke_Style);
    paint->setStrokeWidth(SkFloatToScalar(width));
    paint->setStrokeCap(m_state->m_lineCap);
    paint->setStrokeJoin(m_state->m_lineJoin);
    paint->setStrokeMiter(SkFloatToScalar(m_state->m_miterLimit));

    if (m_state->m_dash)
        paint->setPathEffect(m_state->m_dash);
    else {
        switch (m_state->m_strokeStyle) {
        case NoStroke:
        case SolidStroke:
            break;
        case DashedStroke:
            width = m_state->m_dashRatio * width;
            // Fall through.
        case DottedStroke:
            // Truncate the width, since we don't want fuzzy dots or dashes.
            int dashLength = static_cast<int>(width);
            // Subtract off the endcaps, since they're rendered separately.
            int distance = length - 2 * static_cast<int>(m_state->m_strokeThickness);
            int phase = 1;
            if (dashLength > 1) {
                // Determine how many dashes or dots we should have.
                int numDashes = distance / dashLength;
                int remainder = distance % dashLength;
                // Adjust the phase to center the dashes within the line.
                if (numDashes % 2 == 0) {
                    // Even:  shift right half a dash, minus half the remainder
                    phase = (dashLength - remainder) / 2;
                } else {
                    // Odd:  shift right a full dash, minus half the remainder
                    phase = dashLength - remainder / 2;
                }
            }
            SkScalar dashLengthSk = SkIntToScalar(dashLength);
            SkScalar intervals[2] = { dashLengthSk, dashLengthSk };
            paint->setPathEffect(new SkDashPathEffect(intervals, 2, SkIntToScalar(phase)))->unref();
        }
    }

    return width;
}

void PlatformContextSkia::setDrawLooper(SkDrawLooper* dl)
{
    SkRefCnt_SafeAssign(m_state->m_looper, dl);
}

void PlatformContextSkia::setMiterLimit(float ml)
{
    m_state->m_miterLimit = ml;
}

void PlatformContextSkia::setAlpha(float alpha)
{
    m_state->m_alpha = alpha;
}

void PlatformContextSkia::setLineCap(SkPaint::Cap lc)
{
    m_state->m_lineCap = lc;
}

void PlatformContextSkia::setLineJoin(SkPaint::Join lj)
{
    m_state->m_lineJoin = lj;
}

void PlatformContextSkia::setXfermodeMode(SkXfermode::Mode pdm)
{
    m_state->m_xferMode = pdm;
}

void PlatformContextSkia::setFillColor(SkColor color)
{
    m_state->m_fillColor = color;
}

SkDrawLooper* PlatformContextSkia::getDrawLooper() const
{
    return m_state->m_looper;
}

StrokeStyle PlatformContextSkia::getStrokeStyle() const
{
    return m_state->m_strokeStyle;
}

void PlatformContextSkia::setStrokeStyle(StrokeStyle strokeStyle)
{
    m_state->m_strokeStyle = strokeStyle;
}

void PlatformContextSkia::setStrokeColor(SkColor strokeColor)
{
    m_state->m_strokeColor = strokeColor;
}

float PlatformContextSkia::getStrokeThickness() const
{
    return m_state->m_strokeThickness;
}

void PlatformContextSkia::setStrokeThickness(float thickness)
{
    m_state->m_strokeThickness = thickness;
}

TextDrawingModeFlags PlatformContextSkia::getTextDrawingMode() const
{
    return m_state->m_textDrawingMode;
}

float PlatformContextSkia::getAlpha() const
{
    return m_state->m_alpha;
}

int PlatformContextSkia::getNormalizedAlpha() const
{
    int alpha = roundf(m_state->m_alpha * 256);
    if (alpha > 255)
        alpha = 255;
    else if (alpha < 0)
        alpha = 0;
    return alpha;
}

void PlatformContextSkia::setTextDrawingMode(TextDrawingModeFlags mode)
{
    // TextModeClip is never used, so we assert that it isn't set:
    // https://bugs.webkit.org/show_bug.cgi?id=21898
    ASSERT(!(mode & TextModeClip));
    m_state->m_textDrawingMode = mode;
}

void PlatformContextSkia::setUseAntialiasing(bool enable)
{
    m_state->m_useAntialiasing = enable;
}

SkColor PlatformContextSkia::effectiveFillColor() const
{
    return m_state->applyAlpha(m_state->m_fillColor);
}

SkColor PlatformContextSkia::effectiveStrokeColor() const
{
    return m_state->applyAlpha(m_state->m_strokeColor);
}

void PlatformContextSkia::canvasClipPath(const SkPath& path)
{
    m_canvas->clipPath(path);
}

InterpolationQuality PlatformContextSkia::interpolationQuality() const
{
    return m_state->m_interpolationQuality;
}

void PlatformContextSkia::setInterpolationQuality(InterpolationQuality interpolationQuality)
{
    m_state->m_interpolationQuality = interpolationQuality;
}

void PlatformContextSkia::setDashPathEffect(SkDashPathEffect* dash)
{
    if (dash != m_state->m_dash) {
        SkSafeUnref(m_state->m_dash);
        m_state->m_dash = dash;
    }
}

void PlatformContextSkia::paintSkPaint(const SkRect& rect,
                                       const SkPaint& paint)
{
    m_canvas->drawRect(rect, paint);
}

const SkBitmap* PlatformContextSkia::bitmap() const
{
    return &m_canvas->getDevice()->accessBitmap(false);
}

bool PlatformContextSkia::isNativeFontRenderingAllowed()
{
#if ENABLE(SKIA_TEXT)
    return false;
#else
    if (isAccelerated())
        return false;
    return skia::SupportsPlatformPaint(m_canvas);
#endif
}

void PlatformContextSkia::getImageResamplingHint(IntSize* srcSize, FloatSize* dstSize) const
{
    *srcSize = m_imageResamplingHintSrcSize;
    *dstSize = m_imageResamplingHintDstSize;
}

void PlatformContextSkia::setImageResamplingHint(const IntSize& srcSize, const FloatSize& dstSize)
{
    m_imageResamplingHintSrcSize = srcSize;
    m_imageResamplingHintDstSize = dstSize;
}

void PlatformContextSkia::clearImageResamplingHint()
{
    m_imageResamplingHintSrcSize = IntSize();
    m_imageResamplingHintDstSize = FloatSize();
}

bool PlatformContextSkia::hasImageResamplingHint() const
{
    return !m_imageResamplingHintSrcSize.isEmpty() && !m_imageResamplingHintDstSize.isEmpty();
}

void PlatformContextSkia::applyClipFromImage(const FloatRect& rect, const SkBitmap& imageBuffer)
{
    // NOTE: this assumes the image mask contains opaque black for the portions that are to be shown, as such we
    // only look at the alpha when compositing. I'm not 100% sure this is what WebKit expects for image clipping.
    SkPaint paint;
    paint.setXfermodeMode(SkXfermode::kDstIn_Mode);
    m_canvas->drawBitmap(imageBuffer, SkFloatToScalar(rect.x()), SkFloatToScalar(rect.y()), &paint);
}

void PlatformContextSkia::applyAntiAliasedClipPaths(WTF::Vector<SkPath>& paths)
{
    // Anti-aliased clipping:
    //
    // Skia's clipping is 1-bit only. Consider what would happen if it were 8-bit:
    // We have a square canvas, filled with white and we declare a circular
    // clipping path. Then we fill twice with a black rectangle. The fractional
    // pixels would first get the correct color (white * alpha + black * (1 -
    // alpha)), but the second fill would apply the alpha to the already
    // modified color and the result would be too dark.
    //
    // This, anti-aliased clipping needs to be performed after the drawing has
    // been done. In order to do this, we create a new layer of the canvas in
    // clipPathAntiAliased and store the clipping path. All drawing is done to
    // the layer's bitmap while it's in effect. When WebKit calls restore() to
    // undo the clipping, this function is called.
    //
    // Here, we walk the list of clipping paths backwards and, for each, we
    // clear outside of the clipping path. We only need a single extra layer
    // for any number of clipping paths.
    //
    // When we call restore on the SkCanvas, the layer's bitmap is composed
    // into the layer below and we end up with correct, anti-aliased clipping.

    m_canvas->restore();

    SkPaint paint;
    paint.setXfermodeMode(SkXfermode::kClear_Mode);
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kFill_Style);

    for (size_t i = paths.size() - 1; i < paths.size(); --i) {
        paths[i].toggleInverseFillType();
        m_canvas->drawPath(paths[i], paint);
    }

    m_canvas->restore();
}

void PlatformContextSkia::setGraphicsContext3D(GraphicsContext3D* context, DrawingBuffer* drawingBuffer, const WebCore::IntSize& size)
{
    m_gpuContext = context;
#if ENABLE(ACCELERATED_2D_CANVAS)
    if (context && drawingBuffer) {
        // use skia gpu rendering if available
        GrContext* gr = context->grContext();
        if (gr) {
            context->makeContextCurrent();
            drawingBuffer->bind();

            gr->resetContext();
            drawingBuffer->setGrContext(gr);

            GrPlatformSurfaceDesc drawBufDesc;
            drawingBuffer->getGrPlatformSurfaceDesc(&drawBufDesc);
            SkAutoTUnref<GrTexture> drawBufTex(static_cast<GrTexture*>(gr->createPlatformSurface(drawBufDesc)));
            m_canvas->setDevice(new SkGpuDevice(gr, drawBufTex.get()))->unref();
        } else
            m_gpuContext = 0;
    }
#endif
}

void PlatformContextSkia::makeGrContextCurrent()
{
    if (m_gpuContext)
        m_gpuContext->makeContextCurrent();
}

bool PlatformContextSkia::paintsIntoImageBuffer() const
{
    return m_gpuContext ? m_gpuContext->paintsIntoCanvasBuffer() : true;
}

} // namespace WebCore
