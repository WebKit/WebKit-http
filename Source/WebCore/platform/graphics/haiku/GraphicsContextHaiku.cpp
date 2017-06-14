/*
 * Copyright (C) 2007 Ryan Leavengood <leavengood@gmail.com>
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 * Copyright (C) 2015 Julian Harnath <julian.harnath@rwth-aachen.de>
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
#include "GraphicsContext.h"

#include "AffineTransform.h"
#include "Color.h"
#include "DisplayListRecorder.h"
#include "ImageBuffer.h"
#include "NotImplemented.h"
#include "Path.h"
#include "ShadowBlur.h"
#include "TransformationMatrix.h"

#include <wtf/text/CString.h>
#include <Bitmap.h>
#include <GraphicsDefs.h>
#include <Picture.h>
#include <Region.h>
#include <Shape.h>
#include <View.h>
#include <Window.h>
#include <stdio.h>

namespace WebCore {

class GraphicsContextPlatformPrivate {
public:
    GraphicsContextPlatformPrivate(BView* view);
    ~GraphicsContextPlatformPrivate();

    struct CustomGraphicsState {
        CustomGraphicsState()
            : previous(0)
            , imageInterpolationQuality(InterpolationDefault)
            , globalAlpha(1.0f)
        {
        }

        CustomGraphicsState(CustomGraphicsState* previous)
            : previous(previous)
            , imageInterpolationQuality(previous->imageInterpolationQuality)
            , globalAlpha(previous->globalAlpha)
        {
        }

        CustomGraphicsState* previous;

        InterpolationQuality imageInterpolationQuality;
        float globalAlpha;
    };

    void pushState()
    {
    	m_graphicsState = new CustomGraphicsState(m_graphicsState);
        view()->PushState();
    }

    void popState()
    {
    	ASSERT(m_graphicsState->previous);
    	if (!m_graphicsState->previous)
    	    return;

    	CustomGraphicsState* oldTop = m_graphicsState;
    	m_graphicsState = oldTop->previous;
    	delete oldTop;

        view()->PopState();
    }

    BView* view() const
    {
        return m_view;
    }

    CustomGraphicsState* state() const
    {
        return m_graphicsState;
    }

    ShadowBlur& shadowBlur()
    {
        return blur;
    }

    CustomGraphicsState* m_graphicsState;
    BView* m_view;
    ShadowBlur blur;
    pattern m_strokeStyle;
};

GraphicsContextPlatformPrivate::GraphicsContextPlatformPrivate(BView* view)
    : m_graphicsState(new CustomGraphicsState)
    , m_view(view)
    , m_strokeStyle(B_SOLID_HIGH)
{
}

GraphicsContextPlatformPrivate::~GraphicsContextPlatformPrivate()
{
    while (CustomGraphicsState* previous = m_graphicsState->previous) {
        delete m_graphicsState;
        m_graphicsState = previous;
    }
    delete m_graphicsState;
}

void GraphicsContext::platformInit(PlatformGraphicsContext* context)
{
    m_data = new GraphicsContextPlatformPrivate(context);
}

void GraphicsContext::platformDestroy()
{
    delete m_data;
}

PlatformGraphicsContext* GraphicsContext::platformContext() const
{
    return m_data->view();
}

void GraphicsContext::savePlatformState()
{
	m_data->pushState();
}

void GraphicsContext::restorePlatformState()
{
	m_data->popState();
}

// Draws a filled rectangle with a stroked border.
void GraphicsContext::drawRect(const FloatRect& rect, float borderThickness)
{
    if (paintingDisabled())
        return;

    if (m_state.fillPattern)
        notImplemented();
    else if (m_state.fillGradient) {
        BGradient* gradient = m_state.fillGradient->platformGradient();
        m_data->view()->FillRect(rect, *gradient);
    } else if (fillColor().alpha())
        m_data->view()->FillRect(rect);

    // TODO: Support gradients
    if (strokeStyle() != NoStroke && borderThickness > 0.0f && strokeColor().alpha())
    {
        m_data->view()->SetPenSize(borderThickness);
        m_data->view()->StrokeRect(rect, m_data->m_strokeStyle);
    }
}

// This is only used to draw borders.
void GraphicsContext::drawLine(const FloatPoint& point1, const FloatPoint& point2)
{
    if (paintingDisabled() || strokeStyle() == NoStroke || strokeThickness() <= 0.0f || !strokeColor().alpha())
        return;
    m_data->view()->StrokeLine(point1, point2, m_data->m_strokeStyle);
}

// This method is only used to draw the little circles used in lists.
void GraphicsContext::drawEllipse(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    if (m_state.fillPattern || m_state.fillGradient || fillColor().alpha()) {
//        TODO: What's this shadow business?
        if (m_state.fillPattern)
            notImplemented();
        else if (m_state.fillGradient) {
            BGradient* gradient = m_state.fillGradient->platformGradient();
            m_data->view()->FillEllipse(rect, *gradient);
        } else
            m_data->view()->FillEllipse(rect);
    }

    // TODO: Support gradients
    if (strokeStyle() != NoStroke && strokeThickness() > 0.0f && strokeColor().alpha())
        m_data->view()->StrokeEllipse(rect, m_data->m_strokeStyle);
}

void GraphicsContext::strokeRect(const FloatRect& rect, float width)
{
    if (paintingDisabled() || strokeStyle() == NoStroke || width <= 0.0f || !strokeColor().alpha())
        return;

    float oldSize = m_data->view()->PenSize();
    m_data->view()->SetPenSize(width);
    // TODO stroke the shadow
    m_data->view()->StrokeRect(rect, m_data->m_strokeStyle);
    m_data->view()->SetPenSize(oldSize);
}

void GraphicsContext::strokePath(const Path& path)
{
    if (paintingDisabled())
        return;

    m_data->view()->MovePenTo(B_ORIGIN);

    // TODO: stroke the shadow (cf shadowAndStrokeCurrentCairoPath)

    if (m_state.strokePattern)
        notImplemented();
    else if (m_state.strokeGradient) {
        notImplemented();
//      BGradient* gradient = m_state.strokeGradient->platformGradient();
//      m_data->view()->StrokeShape(m_data->shape(), *gradient);
    } else if(strokeColor().alpha()) {
        drawing_mode mode = m_data->view()->DrawingMode();
        if (m_data->view()->HighColor().alpha < 255)
            m_data->view()->SetDrawingMode(B_OP_ALPHA);

        m_data->view()->StrokeShape(path.platformPath(), m_data->m_strokeStyle);
        m_data->view()->SetDrawingMode(mode);
    }
}

void GraphicsContext::fillRect(const FloatRect& rect, const Color& color)
{
    if (paintingDisabled())
        return;

    BView* view = m_data->view();
    rgb_color previousColor = view->HighColor();

#if 0
    // FIXME needs support for Composite SourceIn.
    if (hasShadow()) {
        m_data->shadowBlur().drawRectShadow(this, rect, RoundedRect::Radii());
    }
#endif

    //setPlatformFillColor(color, ColorSpaceDeviceRGB);
    view->SetHighColor(color);
    view->FillRect(rect);

    view->SetHighColor(previousColor);
}

void GraphicsContext::fillRect(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    // TODO fill the shadow
    m_data->view()->FillRect(rect);
}

void GraphicsContext::platformFillRoundedRect(const FloatRoundedRect& roundRect, const Color& color)
{
    if (paintingDisabled() || !color.alpha())
        return;

    const FloatRect& rect = roundRect.rect();
    const FloatSize& topLeft = roundRect.radii().topLeft();
    const FloatSize& topRight = roundRect.radii().topRight();
    const FloatSize& bottomLeft = roundRect.radii().bottomLeft();
    const FloatSize& bottomRight = roundRect.radii().bottomRight();

#if 0
    // FIXME needs support for Composite SourceIn.
    if (hasShadow())
        m_data->shadowBlur().drawRectShadow(this, rect, FloatRoundedRect::Radii(topLeft, topRight, bottomLeft, bottomRight));
#endif

    BPoint points[3];
    const float kRadiusBezierScale = 1.0f - 0.5522847498f; //  1 - (sqrt(2) - 1) * 4 / 3

    BShape shape;
    shape.MoveTo(BPoint(rect.x() + topLeft.width(), rect.y()));
    shape.LineTo(BPoint(rect.maxX() - topRight.width(), rect.y()));
    points[0].x = rect.maxX() - kRadiusBezierScale * topRight.width();
    points[0].y = rect.y();
    points[1].x = rect.maxX();
    points[1].y = rect.y() + kRadiusBezierScale * topRight.height();
    points[2].x = rect.maxX();
    points[2].y = rect.y() + topRight.height();
    shape.BezierTo(points);
    shape.LineTo(BPoint(rect.maxX(), rect.maxY() - bottomRight.height()));
    points[0].x = rect.maxX();
    points[0].y = rect.maxY() - kRadiusBezierScale * bottomRight.height();
    points[1].x = rect.maxX() - kRadiusBezierScale * bottomRight.width();
    points[1].y = rect.maxY();
    points[2].x = rect.maxX() - bottomRight.width();
    points[2].y = rect.maxY();
    shape.BezierTo(points);
    shape.LineTo(BPoint(rect.x() + bottomLeft.width(), rect.maxY()));
    points[0].x = rect.x() + kRadiusBezierScale * bottomLeft.width();
    points[0].y = rect.maxY();
    points[1].x = rect.x();
    points[1].y = rect.maxY() - kRadiusBezierScale * bottomRight.height();
    points[2].x = rect.x();
    points[2].y = rect.maxY() - bottomRight.height();
    shape.BezierTo(points);
    shape.LineTo(BPoint(rect.x(), rect.y() + topLeft.height()));
    points[0].x = rect.x();
    points[0].y = rect.y() + kRadiusBezierScale * topLeft.height();
    points[1].x = rect.x() + kRadiusBezierScale * topLeft.width();
    points[1].y = rect.y();
    points[2].x = rect.x() + topLeft.width();
    points[2].y = rect.y();
    shape.BezierTo(points);
    shape.Close();

    rgb_color oldColor = m_data->view()->HighColor();
    setPlatformFillColor(color);
    m_data->view()->MovePenTo(B_ORIGIN);
    m_data->view()->FillShape(&shape);

    m_data->view()->SetHighColor(oldColor);
}

void GraphicsContext::fillPath(const Path& path)
{
    if (paintingDisabled())
        return;

    BView* view = m_data->view();

    view->SetFillRule(fillRule() == RULE_NONZERO ? B_NONZERO : B_EVEN_ODD);
    view->MovePenTo(B_ORIGIN);

    // TODO: Render the shadow (cf shadowAndFillCurrentCairoPath)
    if (m_state.fillPattern)
        notImplemented();
    else if (m_state.fillGradient) {
        view->SetDrawingMode(B_OP_ALPHA);
        BGradient* gradient = m_state.fillGradient->platformGradient();
        view->FillShape(path.platformPath(), *gradient);
    } else if (fillColor().alpha()) {
        drawing_mode mode = view->DrawingMode();
        if (view->HighColor().alpha < 255)
            view->SetDrawingMode(B_OP_ALPHA);

        view->FillShape(path.platformPath());
        view->SetDrawingMode(mode);
    }
}

void GraphicsContext::clip(const FloatRect& rect)
{
    if (paintingDisabled())
        return;
    m_data->view()->ClipToRect(rect);
}

IntRect GraphicsContext::clipBounds() const
{
    BRegion region;
    // TODO does this also take clipping shape into account?
    m_data->view()->GetClippingRegion(&region);
    return IntRect(region.Frame());
}

void GraphicsContext::clipPath(const Path& path, WindRule windRule)
{
    if (paintingDisabled())
        return;

    m_data->view()->SetFillRule(windRule == RULE_EVENODD ? B_EVEN_ODD : B_NONZERO);
    m_data->view()->ClipToShape(path.platformPath());
    // TODO: reset wind rule
}

void GraphicsContext::clipToImageBuffer(ImageBuffer& buffer, const FloatRect& destRect)
{
    if (paintingDisabled())
        return;

    BBitmap* surface = &buffer.m_data.m_bitmap;
    BPicture picture;
    BView* view = platformContext();

    if (!view)
        return;

    view->LockLooper();
    view->BeginPicture(&picture);
    view->PushState();

    view->SetLowColor(make_color(255, 255, 255, 0));
    view->SetViewColor(make_color(255, 255, 255, 0));
    view->SetHighColor(make_color(0, 0, 0, 255));
    view->SetDrawingMode(B_OP_ALPHA);
    view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);

    view->DrawBitmap(surface, destRect);

    view->PopState();
    view->EndPicture();
    view->ClipToPicture(&picture);
    view->UnlockLooper();
}


void GraphicsContext::drawPattern(Image& image, const FloatRect& tileRect,
    const AffineTransform& patternTransform, const FloatPoint& phase,
    const FloatSize& spacing, CompositeOperator op, const FloatRect& destRect,
    BlendMode blendMode)
{
    if (paintingDisabled())
        return;

    if (isRecording()) {
        m_displayListRecorder->drawPattern(image, tileRect, patternTransform, phase, spacing, op, destRect, blendMode);
        return;
    }

    BBitmap* pixels = image.nativeImageForCurrentFrame();
    if (!pixels || !pixels->IsValid()) // If the image hasn't fully loaded.
        return;

    // Figure out if the image has any alpha transparency, we can use faster drawing if not
    bool hasAlpha = false;

    uint8* bits = reinterpret_cast<uint8*>(pixels->Bits());
    uint32 width = pixels->Bounds().IntegerWidth() + 1;
    uint32 height = pixels->Bounds().IntegerHeight() + 1;

    uint32 bytesPerRow = pixels->BytesPerRow();
    for (uint32 y = 0; y < height && !hasAlpha; y++) {
        uint8* p = bits;
        for (uint32 x = 0; x < width && !hasAlpha; x++) {
            hasAlpha = p[3] < 255;
            p += 4;
        }
        bits += bytesPerRow;
    }

    save();
    if (hasAlpha)
        platformContext()->SetDrawingMode(B_OP_ALPHA);
    else
        platformContext()->SetDrawingMode(B_OP_COPY);

    clip(enclosingIntRect(destRect));
    float currentW = phase.x();
    BRect bTileRect(tileRect);
    // FIXME app_server doesn't support B_TILE_BITMAP in DrawBitmap calls. This
    // would be much faster (#11196)
    while (currentW < destRect.x() + destRect.width()) {
        float currentH = phase.y();
        while (currentH < destRect.y() + destRect.height()) {
            BRect bDstRect(currentW, currentH, currentW + width - 1, currentH + height - 1);
            platformContext()->DrawBitmapAsync(pixels, bTileRect, bDstRect);
            currentH += height;
        }
        currentW += width;
    }
    restore();
}


void GraphicsContext::canvasClip(const Path& path, WindRule windRule)
{
    clipPath(path, windRule);
}

void GraphicsContext::clipOut(const Path& path)
{
    if (paintingDisabled() || path.isEmpty())
        return;

    m_data->view()->ClipToInverseShape(path.platformPath());
}

void GraphicsContext::clipOut(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    m_data->view()->ClipToInverseRect(rect);
}

void GraphicsContext::drawFocusRing(const Path& path, float width, float /*offset*/, const Color& color)
{
    if (paintingDisabled() || width <= 0 || color.alpha() == 0)
        return;

    // GTK forces this to 2, we use 1. A focus ring several pixels thick doesn't
    // look good.
    width = 1;

    m_data->view()->PushState();
    setPlatformFillColor(color);
    m_data->view()->SetPenSize(width);
    m_data->view()->StrokeShape(path.platformPath(), B_SOLID_HIGH);
    m_data->view()->PopState();
}

void GraphicsContext::drawFocusRing(const Vector<IntRect>& rects, float width, float /* offset */, const Color& color)
{
    if (paintingDisabled() || width <= 0 || color.alpha() == 0)
        return;

    unsigned rectCount = rects.size();
    if (rectCount <= 0)
        return;

    m_data->view()->PushState();

    // GTK forces this to 2, we use 1. A focus ring several pixels thick doesn't
    // look good.
    // FIXME this still draws a focus ring that looks not so good on "details"
    // elements. Maybe we should disable that somewhere.
    width = 1;

    setPlatformFillColor(color);
    m_data->view()->SetPenSize(width);
    // FIXME: maybe we should implement this with BShape?
    for (unsigned i = 0; i < rectCount; ++i)
        m_data->view()->StrokeRect(rects[i], B_SOLID_HIGH);

    m_data->view()->PopState();
}

void GraphicsContext::drawLineForText(const FloatPoint& origin, float width, bool /*printing*/, bool /* doubleLines */)
{
    if (paintingDisabled())
        return;

    FloatPoint endPoint = origin + FloatSize(width, 0);
    drawLine(IntPoint(origin), IntPoint(endPoint));
}

void GraphicsContext::drawLinesForText(const FloatPoint& point, const DashArray& widths, bool printing, bool doubleUnderlines)
{
    if (paintingDisabled())
        return;

    if (widths.size() <= 0)
        return;

    if (isRecording()) {
        m_displayListRecorder->drawLinesForText(point, widths, printing, doubleUnderlines, strokeThickness());
        return;
    }

    // TODO would be faster to use BeginLineArray/EndLineArray here
    // TODO in Cairo, these are not lines, but filled rectangle? Whats the thickness?
    for (size_t i = 0; i < widths.size(); i += 2)
    {
        drawLineForText(FloatPoint(point.x() + widths[i], point.y()),
            widths[i+1] - widths[i], printing, doubleUnderlines);
    }
}


void GraphicsContext::updateDocumentMarkerResources()
{
}

void GraphicsContext::drawLineForDocumentMarker(const FloatPoint&, float /* width */, DocumentMarkerLineStyle /* style */)
{
    if (paintingDisabled())
        return;

    notImplemented();
}

FloatRect GraphicsContext::roundToDevicePixels(const FloatRect& rect, RoundingMode /* mode */)
{
    FloatRect rounded(rect);
    rounded.setX(roundf(rect.x()));
    rounded.setY(roundf(rect.y()));
    rounded.setWidth(roundf(rect.width()));
    rounded.setHeight(roundf(rect.height()));
    return rounded;
}

void GraphicsContext::beginPlatformTransparencyLayer(float opacity)
{
    if (paintingDisabled())
        return;

    m_data->view()->BeginLayer(static_cast<uint8>(opacity * 255.0));
}

void GraphicsContext::endPlatformTransparencyLayer()
{
    if (paintingDisabled())
        return;

    m_data->view()->EndLayer();
}

bool GraphicsContext::supportsTransparencyLayers()
{
    return true;
}

/* Used by canvas.clearRect. Must clear the given rectangle with transparent black. */
void GraphicsContext::clearRect(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

	m_data->view()->PushState();
    m_data->view()->SetHighColor(0, 0, 0, 0);
    m_data->view()->SetDrawingMode(B_OP_COPY);
    m_data->view()->FillRect(rect);
    m_data->view()->PopState();
}

void GraphicsContext::setLineCap(LineCap lineCap)
{
    if (paintingDisabled())
        return;

    cap_mode mode = B_BUTT_CAP;
    switch (lineCap) {
    case RoundCap:
        mode = B_ROUND_CAP;
        break;
    case SquareCap:
        mode = B_SQUARE_CAP;
        break;
    case ButtCap:
    default:
        break;
    }

    m_data->view()->SetLineMode(mode, m_data->view()->LineJoinMode(), m_data->view()->LineMiterLimit());
}

void GraphicsContext::setLineDash(const DashArray& /*dashes*/, float /*dashOffset*/)
{
    // TODO this is used to draw dashed strokes in SVG, but we need app_server support
    notImplemented();
}

void GraphicsContext::setLineJoin(LineJoin lineJoin)
{
    if (paintingDisabled())
        return;

    join_mode mode = B_MITER_JOIN;
    switch (lineJoin) {
    case RoundJoin:
        mode = B_ROUND_JOIN;
        break;
    case BevelJoin:
        mode = B_BEVEL_JOIN;
        break;
    case MiterJoin:
    default:
        break;
    }

    m_data->view()->SetLineMode(m_data->view()->LineCapMode(), mode, m_data->view()->LineMiterLimit());
}

void GraphicsContext::setMiterLimit(float limit)
{
    if (paintingDisabled())
        return;

    m_data->view()->SetLineMode(m_data->view()->LineCapMode(), m_data->view()->LineJoinMode(), limit);
}

void GraphicsContext::setPlatformAlpha(float opacity)
{
    if (paintingDisabled())
        return;

    // FIXME the alpha is only applied to plain colors, not bitmaps, gradients,
    // or anything else. Support should be moved to app_server using the trick
    // mentionned here: http://permalink.gmane.org/gmane.comp.graphics.agg/2241
    m_data->state()->globalAlpha = opacity;
}

void GraphicsContext::setPlatformCompositeOperation(CompositeOperator op, BlendMode blend)
{
    if (paintingDisabled())
        return;

    drawing_mode mode = B_OP_COPY;
    switch (op) {
    case CompositeClear:
    case CompositeCopy:
        // Use the default above
        break;
    case CompositeSourceOver:
        mode = B_OP_ALPHA;
        break;
    case CompositePlusLighter:
        mode = B_OP_ADD;
        break;
    case CompositeDifference:
    case CompositePlusDarker:
        mode = B_OP_SUBTRACT;
        break;
    case CompositeDestinationOut:
        mode = B_OP_ERASE;
        break;
    case CompositeSourceAtop:
        // Draw source only where destination isn't transparent
    case CompositeSourceIn:
        // Like B_OP_ALPHA, but don't touch destination alpha channel
    case CompositeSourceOut:
        // Erase everything, draw source only where destination was transparent
    case CompositeDestinationOver:
        // Draw source only where destination is transparent
    case CompositeDestinationAtop:
        // Draw source only where destination is transparent, erase where source is transparent
    case CompositeDestinationIn:
        // Mask destination with source alpha channel. Don't use source colors.
    case CompositeXOR:
        // Draw source where destination is transparent, erase intersection of source and dest.
    default:
        fprintf(stderr, "GraphicsContext::setCompositeOperation: Unsupported composite operation %s\n",
                compositeOperatorName(op, blend).utf8().data());
    }
    m_data->view()->SetDrawingMode(mode);

    if (mode == B_OP_ALPHA)
        m_data->view()->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
}

AffineTransform GraphicsContext::getCTM(IncludeDeviceScale) const
{
    if (paintingDisabled())
        return AffineTransform();

    if (isRecording()) {
        WTFLogAlways("GraphicsContext::getCTM() is not yet compatible with recording contexts.");
        return AffineTransform();
    }

    BAffineTransform t = m_data->view()->Transform();
    	// TODO: we actually need to ues the combined transform here
    AffineTransform matrix(t.sx, t.shy, t.shx, t.sy, t.tx, t.ty);
    return matrix;
}

void GraphicsContext::translate(float x, float y)
{
    if (paintingDisabled() || (x == 0.f && y == 0.f))
        return;

	m_data->view()->TranslateBy(x, y);
}

void GraphicsContext::rotate(float radians)
{
    if (paintingDisabled() || radians == 0.f)
        return;

	m_data->view()->RotateBy(radians);
}

void GraphicsContext::scale(const FloatSize& size)
{
    if (paintingDisabled())
        return;

    m_data->view()->ScaleBy(size.width(), size.height());
}

void GraphicsContext::concatCTM(const AffineTransform& transform)
{
    if (paintingDisabled())
        return;

    BAffineTransform current = m_data->view()->Transform();
    current.PreMultiply(transform);
    m_data->view()->SetTransform(current);
}

void GraphicsContext::setCTM(const AffineTransform& transform)
{
    if (paintingDisabled())
        return;

    m_data->view()->SetTransform(transform);
}

void GraphicsContext::setPlatformShouldAntialias(bool /*enable*/)
{
    if (paintingDisabled())
        return;

    notImplemented();
}

void GraphicsContext::setPlatformImageInterpolationQuality(InterpolationQuality quality)
{
    m_data->state()->imageInterpolationQuality = quality;
}

void GraphicsContext::setURLForRect(const URL& /*link*/, const IntRect& /*destRect*/)
{
    notImplemented();
}

void GraphicsContext::setPlatformStrokeColor(const Color& color)
{
    if (paintingDisabled())
        return;

    // NOTE: In theory, we should be able to use the low color and
    // return B_SOLID_LOW for the SolidStroke case in setPlatformStrokeStyle()
    // below. More stuff needs to be fixed, though, it will for example
    // prevent the text caret from rendering.
//    m_data->view()->SetLowColor(color);
    setPlatformFillColor(color);
}

void GraphicsContext::setPlatformStrokeStyle(StrokeStyle strokeStyle)
{
    if (paintingDisabled())
        return;

    switch (strokeStyle) {
    case SolidStroke:
    case DoubleStroke:
    case WavyStroke:
        m_data->m_strokeStyle = B_SOLID_HIGH;
        break;
    case DottedStroke:
        m_data->m_strokeStyle = B_MIXED_COLORS;
        break;
    case DashedStroke:
        // FIXME: use a better dashed stroke!
        notImplemented();
        m_data->m_strokeStyle = B_MIXED_COLORS;
        break;
    case NoStroke:
        m_data->m_strokeStyle = B_SOLID_LOW;
        break;
    }
}

void GraphicsContext::setPlatformStrokeThickness(float thickness)
{
    if (paintingDisabled())
        return;

    m_data->view()->SetPenSize(thickness);
}

void GraphicsContext::setPlatformFillColor(const Color& color)
{
    if (paintingDisabled())
        return;

    rgb_color fixed = color;
    fixed.alpha *= m_data->state()->globalAlpha;
    m_data->view()->SetHighColor(fixed);
}

void GraphicsContext::clearPlatformShadow()
{
    if (paintingDisabled())
        return;

    m_data->shadowBlur().clear();
}

void GraphicsContext::setPlatformShadow(FloatSize const& size, float /*blur*/,
    Color const& /*color*/)
{
    if (paintingDisabled())
        return;

    if (m_state.shadowsIgnoreTransforms) {
        // Meaning that this graphics context is associated with a CanvasRenderingContext
        // We flip the height since CG and HTML5 Canvas have opposite Y axis
        m_state.shadowOffset = FloatSize(size.width(), -size.height());
    }

    // Cairo doesn't support shadows natively, they are drawn manually in the draw* functions using ShadowBlur.
    m_data->shadowBlur().setShadowValues(FloatSize(m_state.shadowBlur, m_state.shadowBlur),
                                                    m_state.shadowOffset,
                                                    m_state.shadowColor,
                                                    m_state.shadowsIgnoreTransforms);
}

#if ENABLE(3D_RENDERING) && USE(TEXTURE_MAPPER)
TransformationMatrix GraphicsContext::get3DTransform() const
{
    // FIXME: Can we approximate the transformation better than this?
    return getCTM().toTransformationMatrix();
}

void GraphicsContext::concat3DTransform(const TransformationMatrix& transform)
{
    concatCTM(transform.toAffineTransform());
}

void GraphicsContext::set3DTransform(const TransformationMatrix& transform)
{
    setCTM(transform.toAffineTransform());
}
#endif

} // namespace WebCore

