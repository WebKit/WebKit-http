/*
 * Copyright (c) 2006, Google Inc. All rights reserved.
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
#include "GraphicsContext.h"

#include "AffineTransform.h"
#include "Color.h"
#include "FloatRect.h"
#include "Gradient.h"
#include "ImageBuffer.h"
#include "IntRect.h"
#include "NativeImageSkia.h"
#include "NotImplemented.h"
#include "PlatformContextSkia.h"

#include "SkBitmap.h"
#include "SkBlurMaskFilter.h"
#include "SkColorFilter.h"
#include "SkCornerPathEffect.h"
#include "SkLayerDrawLooper.h"
#include "SkShader.h"
#include "SkiaUtils.h"
#include "skia/ext/platform_canvas.h"

#include <math.h>
#include <wtf/Assertions.h>
#include <wtf/MathExtras.h>
#include <wtf/UnusedParam.h>

#if PLATFORM(CHROMIUM) && OS(DARWIN)
#include <CoreGraphics/CGColorSpace.h>
#endif

using namespace std;

namespace WebCore {

namespace {

inline int fastMod(int value, int max)
{
    int sign = SkExtractSign(value);

    value = SkApplySign(value, sign);
    if (value >= max)
        value %= max;
    return SkApplySign(value, sign);
}

inline float square(float n)
{
    return n * n;
}

}  // namespace

// "Seatbelt" functions ------------------------------------------------------
//
// These functions check certain graphics primitives for being "safe".
// Skia has historically crashed when sent crazy data. These functions do
// additional checking to prevent crashes.
//
// Ideally, all of these would be fixed in the graphics layer and we would not
// have to do any checking. You can uncomment the ENSURE_VALUE_SAFETY_FOR_SKIA
// flag to check the graphics layer.

// Disabling these checks (20/01/2010), since we think we've fixed all the Skia
// bugs.  Leaving the code in for now, so we can revert easily if necessary.
// #define ENSURE_VALUE_SAFETY_FOR_SKIA

#ifdef ENSURE_VALUE_SAFETY_FOR_SKIA
static bool isCoordinateSkiaSafe(float coord)
{
    // First check for valid floats.
#if defined(_MSC_VER)
    if (!_finite(coord))
#else
    if (!finite(coord))
#endif
        return false;

    // Skia uses 16.16 fixed point and 26.6 fixed point in various places. If
    // the transformed point exceeds 15 bits, we just declare that it's
    // unreasonable to catch both of these cases.
    static const int maxPointMagnitude = 32767;
    if (coord > maxPointMagnitude || coord < -maxPointMagnitude)
        return false;

    return true;
}
#endif

static bool isPointSkiaSafe(const SkMatrix& transform, const SkPoint& pt)
{
#ifdef ENSURE_VALUE_SAFETY_FOR_SKIA
    // Now check for points that will overflow. We check the *transformed*
    // points since this is what will be rasterized.
    SkPoint xPt;
    transform.mapPoints(&xPt, &pt, 1);
    return isCoordinateSkiaSafe(xPt.fX) && isCoordinateSkiaSafe(xPt.fY);
#else
    return true;
#endif
}

static bool isRectSkiaSafe(const SkMatrix& transform, const SkRect& rc)
{
#ifdef ENSURE_VALUE_SAFETY_FOR_SKIA
    SkPoint topleft = {rc.fLeft, rc.fTop};
    SkPoint bottomright = {rc.fRight, rc.fBottom};
    return isPointSkiaSafe(transform, topleft) && isPointSkiaSafe(transform, bottomright);
#else
    return true;
#endif
}

bool isPathSkiaSafe(const SkMatrix& transform, const SkPath& path)
{
#ifdef ENSURE_VALUE_SAFETY_FOR_SKIA
    SkPoint current_points[4];
    SkPath::Iter iter(path, false);
    for (SkPath::Verb verb = iter.next(current_points);
         verb != SkPath::kDone_Verb;
         verb = iter.next(current_points)) {
        switch (verb) {
        case SkPath::kMove_Verb:
            // This move will be duplicated in the next verb, so we can ignore.
            break;
        case SkPath::kLine_Verb:
            // iter.next returns 2 points.
            if (!isPointSkiaSafe(transform, current_points[0])
                || !isPointSkiaSafe(transform, current_points[1]))
                return false;
            break;
        case SkPath::kQuad_Verb:
            // iter.next returns 3 points.
            if (!isPointSkiaSafe(transform, current_points[0])
                || !isPointSkiaSafe(transform, current_points[1])
                || !isPointSkiaSafe(transform, current_points[2]))
                return false;
            break;
        case SkPath::kCubic_Verb:
            // iter.next returns 4 points.
            if (!isPointSkiaSafe(transform, current_points[0])
                || !isPointSkiaSafe(transform, current_points[1])
                || !isPointSkiaSafe(transform, current_points[2])
                || !isPointSkiaSafe(transform, current_points[3]))
                return false;
            break;
        case SkPath::kClose_Verb:
        case SkPath::kDone_Verb:
        default:
            break;
        }
    }
    return true;
#else
    return true;
#endif
}

// Local helper functions ------------------------------------------------------

void addCornerArc(SkPath* path, const SkRect& rect, const IntSize& size, int startAngle)
{
    SkIRect ir;
    int rx = SkMin32(SkScalarRound(rect.width()), size.width());
    int ry = SkMin32(SkScalarRound(rect.height()), size.height());

    ir.set(-rx, -ry, rx, ry);
    switch (startAngle) {
    case 0:
        ir.offset(rect.fRight - ir.fRight, rect.fBottom - ir.fBottom);
        break;
    case 90:
        ir.offset(rect.fLeft - ir.fLeft, rect.fBottom - ir.fBottom);
        break;
    case 180:
        ir.offset(rect.fLeft - ir.fLeft, rect.fTop - ir.fTop);
        break;
    case 270:
        ir.offset(rect.fRight - ir.fRight, rect.fTop - ir.fTop);
        break;
    default:
        ASSERT(0);
    }

    SkRect r;
    r.set(ir);
    path->arcTo(r, SkIntToScalar(startAngle), SkIntToScalar(90), false);
}

// -----------------------------------------------------------------------------

// This may be called with a NULL pointer to create a graphics context that has
// no painting.
void GraphicsContext::platformInit(PlatformGraphicsContext* gc)
{
    if (gc)
        gc->setGraphicsContext(this);

    // the caller owns the gc
    m_data = gc;
    setPaintingDisabled(!gc || !gc->canvas());
}

void GraphicsContext::platformDestroy()
{
}

PlatformGraphicsContext* GraphicsContext::platformContext() const
{
    ASSERT(!paintingDisabled());
    return m_data;
}

// State saving ----------------------------------------------------------------

void GraphicsContext::savePlatformState()
{
    if (paintingDisabled())
        return;

    // Save our private State.
    platformContext()->save();
}

void GraphicsContext::restorePlatformState()
{
    if (paintingDisabled())
        return;

    // Restore our private State.
    platformContext()->restore();
}

void GraphicsContext::beginPlatformTransparencyLayer(float opacity)
{
    if (paintingDisabled())
        return;

    // We need the "alpha" layer flag here because the base layer is opaque
    // (the surface of the page) but layers on top may have transparent parts.
    // Without explicitly setting the alpha flag, the layer will inherit the
    // opaque setting of the base and some things won't work properly.

    SkPaint layerPaint;
    layerPaint.setAlpha(static_cast<unsigned char>(opacity * 255));
    layerPaint.setXfermodeMode(platformContext()->getXfermodeMode());

    platformContext()->canvas()->saveLayer(
        0,
        &layerPaint,
        static_cast<SkCanvas::SaveFlags>(SkCanvas::kHasAlphaLayer_SaveFlag |
                                         SkCanvas::kFullColorLayer_SaveFlag));
}

void GraphicsContext::endPlatformTransparencyLayer()
{
    if (paintingDisabled())
        return;
    platformContext()->canvas()->restore();
}

bool GraphicsContext::supportsTransparencyLayers()
{
    return true;
}

// Graphics primitives ---------------------------------------------------------

void GraphicsContext::addInnerRoundedRectClip(const IntRect& rect, int thickness)
{
    if (paintingDisabled())
        return;

    SkRect r(rect);
    if (!isRectSkiaSafe(getCTM(), r))
        return;

    SkPath path;
    path.addOval(r, SkPath::kCW_Direction);
    // only perform the inset if we won't invert r
    if (2 * thickness < rect.width() && 2 * thickness < rect.height()) {
        // Adding one to the thickness doesn't make the border too thick as
        // it's painted over afterwards. But without this adjustment the
        // border appears a little anemic after anti-aliasing.
        r.inset(SkIntToScalar(thickness + 1), SkIntToScalar(thickness + 1));
        path.addOval(r, SkPath::kCCW_Direction);
    }
    platformContext()->clipPathAntiAliased(path);
}

void GraphicsContext::clearPlatformShadow()
{
    if (paintingDisabled())
        return;
    platformContext()->setDrawLooper(0);
}

void GraphicsContext::clearRect(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    SkRect r = rect;
    if (!isRectSkiaSafe(getCTM(), r))
        ClipRectToCanvas(*platformContext()->canvas(), r, &r);

    SkPaint paint;
    platformContext()->setupPaintForFilling(&paint);
    paint.setXfermodeMode(SkXfermode::kClear_Mode);
    platformContext()->canvas()->drawRect(r, paint);
}

void GraphicsContext::clip(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    SkRect r(rect);
    if (!isRectSkiaSafe(getCTM(), r))
        return;

    platformContext()->canvas()->clipRect(r);
}

void GraphicsContext::clip(const Path& path)
{
    if (paintingDisabled())
        return;

    const SkPath& p = *path.platformPath();
    if (!isPathSkiaSafe(getCTM(), p))
        return;

    platformContext()->clipPathAntiAliased(p);
}

void GraphicsContext::canvasClip(const Path& path)
{
    if (paintingDisabled())
        return;

    const SkPath& p = *path.platformPath();
    if (!isPathSkiaSafe(getCTM(), p))
        return;

    platformContext()->canvasClipPath(p);
}

void GraphicsContext::clipOut(const IntRect& rect)
{
    if (paintingDisabled())
        return;

    SkRect r(rect);
    if (!isRectSkiaSafe(getCTM(), r))
        return;

    platformContext()->canvas()->clipRect(r, SkRegion::kDifference_Op);
}

void GraphicsContext::clipOut(const Path& p)
{
    if (paintingDisabled())
        return;

    SkPath path = *p.platformPath();
    if (!isPathSkiaSafe(getCTM(), path))
        return;

    path.toggleInverseFillType();
    platformContext()->clipPathAntiAliased(path);
}

void GraphicsContext::clipPath(const Path& pathToClip, WindRule clipRule)
{
    if (paintingDisabled())
        return;

    SkPath path = *pathToClip.platformPath();
    if (!isPathSkiaSafe(getCTM(), path))
        return;

    path.setFillType(clipRule == RULE_EVENODD ? SkPath::kEvenOdd_FillType : SkPath::kWinding_FillType);
    platformContext()->clipPathAntiAliased(path);
}

void GraphicsContext::concatCTM(const AffineTransform& affine)
{
    if (paintingDisabled())
        return;

    platformContext()->canvas()->concat(affine);
}

void GraphicsContext::setCTM(const AffineTransform& affine)
{
    if (paintingDisabled())
        return;

    platformContext()->canvas()->setMatrix(affine);
}

static void setPathFromConvexPoints(SkPath* path, size_t numPoints, const FloatPoint* points)
{
    path->incReserve(numPoints);
    path->moveTo(WebCoreFloatToSkScalar(points[0].x()),
                 WebCoreFloatToSkScalar(points[0].y()));
    for (size_t i = 1; i < numPoints; ++i) {
        path->lineTo(WebCoreFloatToSkScalar(points[i].x()),
                     WebCoreFloatToSkScalar(points[i].y()));
    }
    path->setIsConvex(true);
}

void GraphicsContext::drawConvexPolygon(size_t numPoints,
                                        const FloatPoint* points,
                                        bool shouldAntialias)
{
    if (paintingDisabled())
        return;

    if (numPoints <= 1)
        return;

    SkPath path;
    setPathFromConvexPoints(&path, numPoints, points);

    if (!isPathSkiaSafe(getCTM(), path))
        return;

    SkPaint paint;
    platformContext()->setupPaintForFilling(&paint);
    paint.setAntiAlias(shouldAntialias);
    platformContext()->canvas()->drawPath(path, paint);

    if (strokeStyle() != NoStroke) {
        paint.reset();
        platformContext()->setupPaintForStroking(&paint, 0, 0);
        platformContext()->canvas()->drawPath(path, paint);
    }
}

void GraphicsContext::clipConvexPolygon(size_t numPoints, const FloatPoint* points, bool antialiased)
{
    if (paintingDisabled())
        return;

    if (numPoints <= 1)
        return;

    SkPath path;
    if (!isPathSkiaSafe(getCTM(), path))
        return;

    setPathFromConvexPoints(&path, numPoints, points);
    if (antialiased)
        platformContext()->clipPathAntiAliased(path);
    else
        platformContext()->canvas()->clipPath(path);
}

// This method is only used to draw the little circles used in lists.
void GraphicsContext::drawEllipse(const IntRect& elipseRect)
{
    if (paintingDisabled())
        return;

    SkRect rect = elipseRect;
    if (!isRectSkiaSafe(getCTM(), rect))
        return;

    SkPaint paint;
    platformContext()->setupPaintForFilling(&paint);
    platformContext()->canvas()->drawOval(rect, paint);

    if (strokeStyle() != NoStroke) {
        paint.reset();
        platformContext()->setupPaintForStroking(&paint, &rect, 0);
        platformContext()->canvas()->drawOval(rect, paint);
    }
}

void GraphicsContext::drawFocusRing(const Path& path, int width, int offset, const Color& color)
{
    // FIXME: implement
}

static inline void drawOuterPath(SkCanvas* canvas, const SkPath& path, SkPaint& paint, int width)
{
#if PLATFORM(CHROMIUM) && OS(DARWIN)
    paint.setAlpha(64);
    paint.setStrokeWidth(width);
    paint.setPathEffect(new SkCornerPathEffect((width - 1) * 0.5f))->unref();
#else
    paint.setStrokeWidth(1);
    paint.setPathEffect(new SkCornerPathEffect(1))->unref();
#endif
    canvas->drawPath(path, paint);
}

static inline void drawInnerPath(SkCanvas* canvas, const SkPath& path, SkPaint& paint, int width)
{
#if PLATFORM(CHROMIUM) && OS(DARWIN)
    paint.setAlpha(128);
    paint.setStrokeWidth(width * 0.5f);
    canvas->drawPath(path, paint);
#endif
}

static inline SkScalar getFocusRingOutset()
{
#if PLATFORM(CHROMIUM) && OS(DARWIN)
    return 0.75f;
#else
    return 0.5f;
#endif
}

void GraphicsContext::drawFocusRing(const Vector<IntRect>& rects, int width, int /* offset */, const Color& color)
{
    if (paintingDisabled())
        return;

    unsigned rectCount = rects.size();
    if (!rectCount)
        return;

    SkRegion focusRingRegion;
    const SkScalar focusRingOutset = getFocusRingOutset();
    for (unsigned i = 0; i < rectCount; i++) {
        SkIRect r = rects[i];
        r.inset(-focusRingOutset, -focusRingOutset);
        focusRingRegion.op(r, SkRegion::kUnion_Op);
    }

    SkPath path;
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);

    paint.setColor(color.rgb());
    focusRingRegion.getBoundaryPath(&path);
    SkCanvas* canvas = platformContext()->canvas();
    drawOuterPath(canvas, path, paint, width);
    drawInnerPath(canvas, path, paint, width);
}

// This is only used to draw borders.
void GraphicsContext::drawLine(const IntPoint& point1, const IntPoint& point2)
{
    if (paintingDisabled())
        return;

    StrokeStyle penStyle = strokeStyle();
    if (penStyle == NoStroke)
        return;

    SkPaint paint;
    if (!isPointSkiaSafe(getCTM(), point1) || !isPointSkiaSafe(getCTM(), point2))
        return;

    FloatPoint p1 = point1;
    FloatPoint p2 = point2;
    bool isVerticalLine = (p1.x() == p2.x());
    int width = roundf(strokeThickness());

    // We know these are vertical or horizontal lines, so the length will just
    // be the sum of the displacement component vectors give or take 1 -
    // probably worth the speed up of no square root, which also won't be exact.
    FloatSize disp = p2 - p1;
    int length = SkScalarRound(disp.width() + disp.height());
    platformContext()->setupPaintForStroking(&paint, 0, length);

    if (strokeStyle() == DottedStroke || strokeStyle() == DashedStroke) {
        // Do a rect fill of our endpoints.  This ensures we always have the
        // appearance of being a border.  We then draw the actual dotted/dashed line.

        SkRect r1, r2;
        r1.set(p1.x(), p1.y(), p1.x() + width, p1.y() + width);
        r2.set(p2.x(), p2.y(), p2.x() + width, p2.y() + width);

        if (isVerticalLine) {
            r1.offset(-width / 2, 0);
            r2.offset(-width / 2, -width);
        } else {
            r1.offset(0, -width / 2);
            r2.offset(-width, -width / 2);
        }
        SkPaint fillPaint;
        fillPaint.setColor(paint.getColor());
        platformContext()->canvas()->drawRect(r1, fillPaint);
        platformContext()->canvas()->drawRect(r2, fillPaint);
    }

    adjustLineToPixelBoundaries(p1, p2, width, penStyle);
    SkPoint pts[2] = { (SkPoint)p1, (SkPoint)p2 };

    platformContext()->canvas()->drawPoints(SkCanvas::kLines_PointMode, 2, pts, paint);
}

void GraphicsContext::drawLineForTextChecking(const FloatPoint& pt, float width, TextCheckingLineStyle style)
{
    if (paintingDisabled())
        return;

    // Create the pattern we'll use to draw the underline.
    static SkBitmap* misspellBitmap = 0;
    if (!misspellBitmap) {
        // We use a 2-pixel-high misspelling indicator because that seems to be
        // what WebKit is designed for, and how much room there is in a typical
        // page for it.
        const int rowPixels = 32;  // Must be multiple of 4 for pattern below.
        const int colPixels = 2;
        misspellBitmap = new SkBitmap;
        misspellBitmap->setConfig(SkBitmap::kARGB_8888_Config,
                                   rowPixels, colPixels);
        misspellBitmap->allocPixels();

        misspellBitmap->eraseARGB(0, 0, 0, 0);
        const uint32_t lineColor = 0xFFFF0000;  // Opaque red.
        const uint32_t antiColor = 0x60600000;  // Semitransparent red.

        // Pattern:  X o   o X o   o X
        //             o X o   o X o
        uint32_t* row1 = misspellBitmap->getAddr32(0, 0);
        uint32_t* row2 = misspellBitmap->getAddr32(0, 1);
        for (int x = 0; x < rowPixels; x++) {
            switch (x % 4) {
            case 0:
                row1[x] = lineColor;
                break;
            case 1:
                row1[x] = antiColor;
                row2[x] = antiColor;
                break;
            case 2:
                row2[x] = lineColor;
                break;
            case 3:
                row1[x] = antiColor;
                row2[x] = antiColor;
                break;
            }
        }
    }

    // Offset it vertically by 1 so that there's some space under the text.
    SkScalar originX = WebCoreFloatToSkScalar(pt.x());
    SkScalar originY = WebCoreFloatToSkScalar(pt.y()) + 1;

    // Make a shader for the bitmap with an origin of the box we'll draw. This
    // shader is refcounted and will have an initial refcount of 1.
    SkShader* shader = SkShader::CreateBitmapShader(
        *misspellBitmap, SkShader::kRepeat_TileMode,
        SkShader::kRepeat_TileMode);
    SkMatrix matrix;
    matrix.reset();
    matrix.postTranslate(originX, originY);
    shader->setLocalMatrix(matrix);

    // Assign the shader to the paint & release our reference. The paint will
    // now own the shader and the shader will be destroyed when the paint goes
    // out of scope.
    SkPaint paint;
    paint.setShader(shader);
    shader->unref();

    SkRect rect;
    rect.set(originX,
             originY,
             originX + WebCoreFloatToSkScalar(width),
             originY + SkIntToScalar(misspellBitmap->height()));
    platformContext()->canvas()->drawRect(rect, paint);
}

void GraphicsContext::drawLineForText(const FloatPoint& pt,
                                      float width,
                                      bool printing)
{
    if (paintingDisabled())
        return;

    if (width <= 0)
        return;

    int thickness = SkMax32(static_cast<int>(strokeThickness()), 1);
    SkRect r;
    r.fLeft = WebCoreFloatToSkScalar(pt.x());
    r.fTop = WebCoreFloatToSkScalar(pt.y());
    r.fRight = r.fLeft + WebCoreFloatToSkScalar(width);
    r.fBottom = r.fTop + SkIntToScalar(thickness);

    SkPaint paint;
    platformContext()->setupPaintForFilling(&paint);
    // Text lines are drawn using the stroke color.
    paint.setColor(platformContext()->effectiveStrokeColor());
    platformContext()->canvas()->drawRect(r, paint);
}

// Draws a filled rectangle with a stroked border.
void GraphicsContext::drawRect(const IntRect& rect)
{
    if (paintingDisabled())
        return;

    SkRect r = rect;
    if (!isRectSkiaSafe(getCTM(), r)) {
        // See the fillRect below.
        ClipRectToCanvas(*platformContext()->canvas(), r, &r);
    }

    platformContext()->drawRect(r);
}

void GraphicsContext::fillPath(const Path& pathToFill)
{
    if (paintingDisabled())
        return;

    SkPath path = *pathToFill.platformPath();
    if (!isPathSkiaSafe(getCTM(), path))
      return;

    const GraphicsContextState& state = m_state;
    path.setFillType(state.fillRule == RULE_EVENODD ?
        SkPath::kEvenOdd_FillType : SkPath::kWinding_FillType);

    SkPaint paint;
    platformContext()->setupPaintForFilling(&paint);

    platformContext()->canvas()->drawPath(path, paint);
}

void GraphicsContext::fillRect(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    SkRect r = rect;
    if (!isRectSkiaSafe(getCTM(), r)) {
        // See the other version of fillRect below.
        ClipRectToCanvas(*platformContext()->canvas(), r, &r);
    }

    platformContext()->save();

    SkPaint paint;
    platformContext()->setupPaintForFilling(&paint);
    platformContext()->canvas()->drawRect(r, paint);

    platformContext()->restore();
}

void GraphicsContext::fillRect(const FloatRect& rect, const Color& color, ColorSpace colorSpace)
{
    if (paintingDisabled())
        return;

    SkRect r = rect;
    if (!isRectSkiaSafe(getCTM(), r)) {
        // Special case when the rectangle overflows fixed point. This is a
        // workaround to fix bug 1212844. When the input rectangle is very
        // large, it can overflow Skia's internal fixed point rect. This
        // should be fixable in Skia (since the output bitmap isn't that
        // large), but until that is fixed, we try to handle it ourselves.
        //
        // We manually clip the rectangle to the current clip rect. This
        // will prevent overflow. The rectangle will be transformed to the
        // canvas' coordinate space before it is converted to fixed point
        // so we are guaranteed not to overflow after doing this.
        ClipRectToCanvas(*platformContext()->canvas(), r, &r);
    }

    SkPaint paint;
    platformContext()->setupPaintCommon(&paint);
    paint.setColor(color.rgb());
    platformContext()->canvas()->drawRect(r, paint);
}

void GraphicsContext::fillRoundedRect(const IntRect& rect,
                                      const IntSize& topLeft,
                                      const IntSize& topRight,
                                      const IntSize& bottomLeft,
                                      const IntSize& bottomRight,
                                      const Color& color,
                                      ColorSpace colorSpace)
{
    if (paintingDisabled())
        return;

    SkRect r = rect;
    if (!isRectSkiaSafe(getCTM(), r))
        // See fillRect().
        ClipRectToCanvas(*platformContext()->canvas(), r, &r);

    if (topLeft.width() + topRight.width() > rect.width()
            || bottomLeft.width() + bottomRight.width() > rect.width()
            || topLeft.height() + bottomLeft.height() > rect.height()
            || topRight.height() + bottomRight.height() > rect.height()) {
        // Not all the radii fit, return a rect. This matches the behavior of
        // Path::createRoundedRectangle. Without this we attempt to draw a round
        // shadow for a square box.
        fillRect(rect, color, colorSpace);
        return;
    }

    SkPath path;
    addCornerArc(&path, r, topRight, 270);
    addCornerArc(&path, r, bottomRight, 0);
    addCornerArc(&path, r, bottomLeft, 90);
    addCornerArc(&path, r, topLeft, 180);

    SkPaint paint;
    platformContext()->setupPaintForFilling(&paint);
    paint.setColor(color.rgb());
    platformContext()->canvas()->drawPath(path, paint);
}

AffineTransform GraphicsContext::getCTM() const
{
    if (paintingDisabled())
        return AffineTransform();

    const SkMatrix& m = platformContext()->canvas()->getTotalMatrix();
    return AffineTransform(SkScalarToDouble(m.getScaleX()),
                           SkScalarToDouble(m.getSkewY()),
                           SkScalarToDouble(m.getSkewX()),
                           SkScalarToDouble(m.getScaleY()),
                           SkScalarToDouble(m.getTranslateX()),
                           SkScalarToDouble(m.getTranslateY()));
}

FloatRect GraphicsContext::roundToDevicePixels(const FloatRect& rect, RoundingMode)
{
    return rect;
}

void GraphicsContext::scale(const FloatSize& size)
{
    if (paintingDisabled())
        return;

    platformContext()->canvas()->scale(WebCoreFloatToSkScalar(size.width()),
        WebCoreFloatToSkScalar(size.height()));
}

void GraphicsContext::setAlpha(float alpha)
{
    if (paintingDisabled())
        return;

    platformContext()->setAlpha(alpha);
}

void GraphicsContext::setPlatformCompositeOperation(CompositeOperator op)
{
    if (paintingDisabled())
        return;

    platformContext()->setXfermodeMode(WebCoreCompositeToSkiaComposite(op));
}

InterpolationQuality GraphicsContext::imageInterpolationQuality() const
{
    return platformContext()->interpolationQuality();
}

void GraphicsContext::setImageInterpolationQuality(InterpolationQuality q)
{
    platformContext()->setInterpolationQuality(q);
}

void GraphicsContext::setLineCap(LineCap cap)
{
    if (paintingDisabled())
        return;
    switch (cap) {
    case ButtCap:
        platformContext()->setLineCap(SkPaint::kButt_Cap);
        break;
    case RoundCap:
        platformContext()->setLineCap(SkPaint::kRound_Cap);
        break;
    case SquareCap:
        platformContext()->setLineCap(SkPaint::kSquare_Cap);
        break;
    default:
        ASSERT(0);
        break;
    }
}

void GraphicsContext::setLineDash(const DashArray& dashes, float dashOffset)
{
    if (paintingDisabled())
        return;

    // FIXME: This is lifted directly off SkiaSupport, lines 49-74
    // so it is not guaranteed to work correctly.
    size_t dashLength = dashes.size();
    if (!dashLength) {
        // If no dash is set, revert to solid stroke
        // FIXME: do we need to set NoStroke in some cases?
        platformContext()->setStrokeStyle(SolidStroke);
        platformContext()->setDashPathEffect(0);
        return;
    }

    size_t count = !(dashLength % 2) ? dashLength : dashLength * 2;
    SkScalar* intervals = new SkScalar[count];

    for (unsigned int i = 0; i < count; i++)
        intervals[i] = dashes[i % dashLength];

    platformContext()->setDashPathEffect(new SkDashPathEffect(intervals, count, dashOffset));

    delete[] intervals;
}

void GraphicsContext::setLineJoin(LineJoin join)
{
    if (paintingDisabled())
        return;
    switch (join) {
    case MiterJoin:
        platformContext()->setLineJoin(SkPaint::kMiter_Join);
        break;
    case RoundJoin:
        platformContext()->setLineJoin(SkPaint::kRound_Join);
        break;
    case BevelJoin:
        platformContext()->setLineJoin(SkPaint::kBevel_Join);
        break;
    default:
        ASSERT(0);
        break;
    }
}

void GraphicsContext::setMiterLimit(float limit)
{
    if (paintingDisabled())
        return;
    platformContext()->setMiterLimit(limit);
}

void GraphicsContext::setPlatformFillColor(const Color& color, ColorSpace colorSpace)
{
    if (paintingDisabled())
        return;

    platformContext()->setFillColor(color.rgb());
}

void GraphicsContext::setPlatformShadow(const FloatSize& size,
                                        float blurFloat,
                                        const Color& color,
                                        ColorSpace colorSpace)
{
    if (paintingDisabled())
        return;

    // Detect when there's no effective shadow and clear the looper.
    if (!size.width() && !size.height() && !blurFloat) {
        platformContext()->setDrawLooper(0);
        return;
    }

    double width = size.width();
    double height = size.height();
    double blur = blurFloat;

    uint32_t mfFlags = SkBlurMaskFilter::kHighQuality_BlurFlag;
    SkXfermode::Mode colorMode = SkXfermode::kSrc_Mode;

    if (m_state.shadowsIgnoreTransforms)  {
        // Currently only the GraphicsContext associated with the
        // CanvasRenderingContext for HTMLCanvasElement have shadows ignore
        // Transforms. So with this flag set, we know this state is associated
        // with a CanvasRenderingContext.
        mfFlags |= SkBlurMaskFilter::kIgnoreTransform_BlurFlag;

        // CSS wants us to ignore the original's alpha, but Canvas wants us to
        // modulate with it. Using shadowsIgnoreTransforms to tell us that we're
        // in a Canvas, we change the colormode to kDst_Mode, so we don't overwrite
        // it with our layer's (default opaque-black) color.
        colorMode = SkXfermode::kDst_Mode;

        // CG uses natural orientation for Y axis, but the HTML5 canvas spec
        // does not.
        // So we now flip the height since it was flipped in
        // CanvasRenderingContext in order to work with CG.
        height = -height;
    }

    SkColor c;
    if (color.isValid())
        c = color.rgb();
    else
        c = SkColorSetARGB(0xFF/3, 0, 0, 0);    // "std" apple shadow color.

    // TODO(tc): Should we have a max value for the blur?  CG clamps at 1000.0
    // for perf reasons.

    SkLayerDrawLooper* dl = new SkLayerDrawLooper;
    SkAutoUnref aur(dl);

    // top layer, we just draw unchanged
    dl->addLayer();

    // lower layer contains our offset, blur, and colorfilter
    SkLayerDrawLooper::LayerInfo info;

    info.fPaintBits |= SkLayerDrawLooper::kMaskFilter_Bit; // our blur
    info.fPaintBits |= SkLayerDrawLooper::kColorFilter_Bit;
    info.fColorMode = colorMode;
    info.fOffset.set(width, height);
    info.fPostTranslate = m_state.shadowsIgnoreTransforms;

    SkMaskFilter* mf = SkBlurMaskFilter::Create(blur / 2, SkBlurMaskFilter::kNormal_BlurStyle, mfFlags);

    SkColorFilter* cf = SkColorFilter::CreateModeFilter(c, SkXfermode::kSrcIn_Mode);

    SkPaint* paint = dl->addLayer(info);
    SkSafeUnref(paint->setMaskFilter(mf));
    SkSafeUnref(paint->setColorFilter(cf));

    // dl is now built, just install it
    platformContext()->setDrawLooper(dl);
}

void GraphicsContext::setPlatformStrokeColor(const Color& strokecolor, ColorSpace colorSpace)
{
    if (paintingDisabled())
        return;

    platformContext()->setStrokeColor(strokecolor.rgb());
}

void GraphicsContext::setPlatformStrokeStyle(StrokeStyle stroke)
{
    if (paintingDisabled())
        return;

    platformContext()->setStrokeStyle(stroke);
}

void GraphicsContext::setPlatformStrokeThickness(float thickness)
{
    if (paintingDisabled())
        return;

    platformContext()->setStrokeThickness(thickness);
}

void GraphicsContext::setPlatformTextDrawingMode(TextDrawingModeFlags mode)
{
    if (paintingDisabled())
        return;

    platformContext()->setTextDrawingMode(mode);
}

void GraphicsContext::setURLForRect(const KURL& link, const IntRect& destRect)
{
}

void GraphicsContext::setPlatformShouldAntialias(bool enable)
{
    if (paintingDisabled())
        return;

    platformContext()->setUseAntialiasing(enable);
}

void GraphicsContext::strokeArc(const IntRect& r, int startAngle, int angleSpan)
{
    if (paintingDisabled())
        return;

    SkPaint paint;
    SkRect oval = r;
    if (strokeStyle() == NoStroke) {
        // Stroke using the fill color.
        // TODO(brettw) is this really correct? It seems unreasonable.
        platformContext()->setupPaintForFilling(&paint);
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(WebCoreFloatToSkScalar(strokeThickness()));
    } else
        platformContext()->setupPaintForStroking(&paint, 0, 0);

    // We do this before converting to scalar, so we don't overflow SkFixed.
    startAngle = fastMod(startAngle, 360);
    angleSpan = fastMod(angleSpan, 360);

    SkPath path;
    path.addArc(oval, SkIntToScalar(-startAngle), SkIntToScalar(-angleSpan));
    if (!isPathSkiaSafe(getCTM(), path))
        return;
    platformContext()->canvas()->drawPath(path, paint);
}

void GraphicsContext::strokePath(const Path& pathToStroke)
{
    if (paintingDisabled())
        return;

    SkPath path = *pathToStroke.platformPath();
    if (!isPathSkiaSafe(getCTM(), path))
        return;

    SkPaint paint;
    platformContext()->setupPaintForStroking(&paint, 0, 0);
    platformContext()->canvas()->drawPath(path, paint);
}

void GraphicsContext::strokeRect(const FloatRect& rect, float lineWidth)
{
    if (paintingDisabled())
        return;

    if (!isRectSkiaSafe(getCTM(), rect))
        return;

    SkPaint paint;
    platformContext()->setupPaintForStroking(&paint, 0, 0);
    paint.setStrokeWidth(WebCoreFloatToSkScalar(lineWidth));
    // strokerect has special rules for CSS when the rect is degenerate:
    // if width==0 && height==0, do nothing
    // if width==0 || height==0, then just draw line for the other dimension
    SkRect r(rect);
    bool validW = r.width() > 0;
    bool validH = r.height() > 0;
    SkCanvas* canvas = platformContext()->canvas();
    if (validW && validH)
        canvas->drawRect(r, paint);
    else if (validW || validH) {
        // we are expected to respect the lineJoin, so we can't just call
        // drawLine -- we have to create a path that doubles back on itself.
        SkPath path;
        path.moveTo(r.fLeft, r.fTop);
        path.lineTo(r.fRight, r.fBottom);
        path.close();
        canvas->drawPath(path, paint);
    }
}

void GraphicsContext::rotate(float angleInRadians)
{
    if (paintingDisabled())
        return;

    platformContext()->canvas()->rotate(WebCoreFloatToSkScalar(
        angleInRadians * (180.0f / 3.14159265f)));
}

void GraphicsContext::translate(float w, float h)
{
    if (paintingDisabled())
        return;

    platformContext()->canvas()->translate(WebCoreFloatToSkScalar(w),
                                           WebCoreFloatToSkScalar(h));
}

bool GraphicsContext::isAcceleratedContext() const
{
    return platformContext()->isAccelerated();
}

#if PLATFORM(CHROMIUM) && OS(DARWIN)
CGColorSpaceRef deviceRGBColorSpaceRef()
{
    static CGColorSpaceRef deviceSpace = CGColorSpaceCreateDeviceRGB();
    return deviceSpace;
}
#endif

}  // namespace WebCore
