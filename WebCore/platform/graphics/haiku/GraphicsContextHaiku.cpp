/*
 * Copyright (C) 2007 Ryan Leavengood <leavengood@gmail.com>
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
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
#include "CString.h"
#include "Color.h"
#include "Font.h"
#include "FontData.h"
#include "GraphicsContextPrivate.h"
#include "NotImplemented.h"
#include "Path.h"
#include "Pen.h"
#include <Bitmap.h>
#include <GraphicsDefs.h>
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

    struct Layer {
    public:
        Layer(BView* _view)
            : view(_view)
            , bitmap(0)
            , globalAlpha(255)
            , currentShape(0)
            , clipShape(0)
            , locationInParent(B_ORIGIN)
            , accumulatedOrigin(B_ORIGIN)
            , previous(0)
        {
            strokeColor.red = 0;
            strokeColor.green = 0;
            strokeColor.blue = 0;
            strokeColor.alpha = 0;

            fillColor.red = 0;
            fillColor.green = 0;
            fillColor.blue = 0;
            fillColor.alpha = 255;
        }
        Layer(Layer* previous)
            : view(0)
            , bitmap(0)
            , globalAlpha(255)
            , currentShape(0)
            , clipShape(previous->clipShape ? new BShape(*previous->clipShape) : 0)
            , locationInParent(B_ORIGIN)
            , accumulatedOrigin(B_ORIGIN)
            , previous(previous)
        {
            strokeColor.red = 0;
            strokeColor.green = 0;
            strokeColor.blue = 0;
            strokeColor.alpha = 0;

            fillColor.red = 0;
            fillColor.green = 0;
            fillColor.blue = 0;
            fillColor.alpha = 255;

            BRegion parentClipping;
            previous->view->GetClippingRegion(&parentClipping);
            BRect frameInParent = parentClipping.Frame();
            if (!frameInParent.IsValid())
                frameInParent = previous->view->Bounds();
            BRect bounds = frameInParent.OffsetToCopy(B_ORIGIN);
            locationInParent += frameInParent.LeftTop();
            view = new BView(bounds, "WebCore transparency layer", 0, 0);
            bitmap = new BBitmap(bounds, B_RGBA32, true);
            bitmap->Lock();
            bitmap->AddChild(view);
            view->SetHighColor(0, 0, 0, 0);
            view->FillRect(view->Bounds());
            view->SetHighColor(previous->view->HighColor());
            view->SetDrawingMode(previous->view->DrawingMode());
            view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
            // TODO: locationInParent and accumulatedOrigin can
            // probably somehow be merged. But for now it works.
            accumulatedOrigin.x = -frameInParent.left;
            accumulatedOrigin.y = -frameInParent.top;
            view->SetOrigin(previous->accumulatedOrigin + accumulatedOrigin);
            view->SetScale(previous->view->Scale());
            view->SetPenSize(previous->view->PenSize());
        }
        ~Layer()
        {
            if (bitmap) {
                bitmap->Unlock();
                // Deleting the bitmap will also take care of the view,
                // if there is no bitmap, the view does not belong to us (initial layer).
                delete bitmap;
            }
            delete currentShape;
            delete clipShape;
        }

        BView* view;
        BBitmap* bitmap;
        rgb_color strokeColor;
        rgb_color fillColor;
        uint8 globalAlpha;
        BShape* currentShape;
        BShape* clipShape;
        BPoint locationInParent;
        BPoint accumulatedOrigin;

        Layer* previous;
    };

    BView* view() const
    {
        return m_currentLayer->view;
    }

    void setShape(BShape* shape)
    {
        delete m_currentLayer->currentShape;
        m_currentLayer->currentShape = shape;
    }

    BShape* shape() const
    {
        if (!m_currentLayer->currentShape)
            m_currentLayer->currentShape = new BShape();
        return m_currentLayer->currentShape;
    }

    void setClipShape(BShape* shape)
    {
    	// NOTE: For proper clipping, the paths would have to
    	// be combined with the previous layers. But this for now,
    	// this is just supposed to support a small hack to get
    	// box elements with round corners to work properly. BView shall
    	// get proper clipping path support in the future.
        delete m_currentLayer->clipShape;
        m_currentLayer->clipShape = shape;
    }

    BShape* clipShape() const
    {
        if (!m_currentLayer->clipShape)
            m_currentLayer->clipShape = new BShape();
        return m_currentLayer->clipShape;
    }

    void pushLayer(float opacity)
    {
        m_currentLayer = new Layer(m_currentLayer);
        m_currentLayer->globalAlpha = (uint8)(opacity * 255.0);
    }

    void popLayer()
    {
        if (!m_currentLayer->previous)
            return;
        Layer* layer = m_currentLayer;
        m_currentLayer = layer->previous;
        if (layer->globalAlpha > 0) {
            // Post process the bitmap in order to apply global alpha...
            layer->view->Sync();
            if (layer->globalAlpha < 255) {
                uint8* bits = reinterpret_cast<uint8*>(layer->bitmap->Bits());
                uint32 width = layer->bitmap->Bounds().IntegerWidth() + 1;
                uint32 height = layer->bitmap->Bounds().IntegerHeight() + 1;
                uint32 bpr = layer->bitmap->BytesPerRow();
                uint8 alpha = layer->globalAlpha;
                for (uint32 y = 0; y < height; y++) {
                    uint8* p = bits + 3;
                    for (uint32 x = 0; x < width; x++) {
                        *p = (uint8)((uint16)*p * alpha / 255);
                        p += 4;
                    }
                    bits += bpr;
                }
            }
            BPoint bitmapLocation(layer->locationInParent);
            bitmapLocation -= m_currentLayer->accumulatedOrigin;
            drawing_mode drawingMode = m_currentLayer->view->DrawingMode();
            m_currentLayer->view->SetDrawingMode(B_OP_ALPHA);
            m_currentLayer->view->DrawBitmap(layer->bitmap, bitmapLocation);
            m_currentLayer->view->SetDrawingMode(drawingMode);
        }
        delete layer;
    }

    Layer* m_currentLayer;
};

GraphicsContextPlatformPrivate::GraphicsContextPlatformPrivate(BView* view)
    : m_currentLayer(new Layer(view))
{
}

GraphicsContextPlatformPrivate::~GraphicsContextPlatformPrivate()
{
    while (Layer* previous = m_currentLayer->previous) {
        delete m_currentLayer;
        m_currentLayer = previous;
    }
    delete m_currentLayer;
}

GraphicsContext::GraphicsContext(PlatformGraphicsContext* context)
    : m_common(createGraphicsContextPrivate())
    , m_data(new GraphicsContextPlatformPrivate(context))
{
    setPaintingDisabled(!context);
}

GraphicsContext::~GraphicsContext()
{
    destroyGraphicsContextPrivate(m_common);
    delete m_data;
}

PlatformGraphicsContext* GraphicsContext::platformContext() const
{
    return m_data->view();
}

void GraphicsContext::savePlatformState()
{
    m_data->view()->PushState();
}

void GraphicsContext::restorePlatformState()
{
    m_data->m_currentLayer->accumulatedOrigin -= m_data->view()->Origin();
    m_data->view()->PopState();
}

// Draws a filled rectangle with a stroked border.
void GraphicsContext::drawRect(const IntRect& rect)
{
    if (paintingDisabled())
        return;

    m_data->view()->FillRect(rect);
    if (strokeStyle() != NoStroke)
        m_data->view()->StrokeRect(rect, getHaikuStrokeStyle());
}

// This is only used to draw borders.
void GraphicsContext::drawLine(const IntPoint& point1, const IntPoint& point2)
{
    if (paintingDisabled() || strokeStyle() == NoStroke || strokeThickness() <= 0.0f || !strokeColor().alpha())
        return;

    m_data->view()->StrokeLine(point1, point2, getHaikuStrokeStyle());
}

// This method is only used to draw the little circles used in lists.
void GraphicsContext::drawEllipse(const IntRect& rect)
{
    if (paintingDisabled())
        return;

    m_data->view()->FillEllipse(rect);
    if (strokeStyle() != NoStroke)
        m_data->view()->StrokeEllipse(rect, getHaikuStrokeStyle());
}

void GraphicsContext::strokeArc(const IntRect& rect, int startAngle, int angleSpan)
{
    if (paintingDisabled() || strokeStyle() == NoStroke || strokeThickness() <= 0.0f || !strokeColor().alpha())
        return;

    // TODO: The code below will only make round-corner boxen look nice. For an utterly shocking
    // implementation of round corner drawing, see RenderBoxModelObject::paintBorder(). It tries
    // to use one (or two) alpha mask(s) per box corner to cut off a thicker stroke and doubles
    // the stroke with. All this to align the arc with the box sides...

    m_data->view()->PushState();
    float penSize = strokeThickness() / 2.0f;
    m_data->view()->SetPenSize(penSize);
    BRect bRect(rect.x(), rect.y(), rect.right(), rect.bottom());
    if (startAngle >= 0 && startAngle < 90) {
        bRect.right -= penSize;
        bRect.top += penSize / 2.0f;
        bRect.bottom -= penSize / 2.0f;
    } else if (startAngle >= 90 && startAngle < 180) {
        bRect.left += penSize / 2.0f;
        bRect.top += penSize / 2.0f;
        bRect.right -= penSize / 2.0f;
        bRect.bottom -= penSize / 2.0f;
    } else if (startAngle >= 180 && startAngle < 270) {
        bRect.left += penSize / 2.0f;
        bRect.right -= penSize / 2.0f;
        bRect.bottom -= penSize;
    } else if (startAngle >= 270 && startAngle < 360) {
        bRect.right -= penSize;
        bRect.bottom -= penSize;
    }
    bRect.OffsetTo(floorf(bRect.left), floorf(bRect.top));
    uint32 flags = m_data->view()->Flags();
    m_data->view()->SetFlags(flags | B_SUBPIXEL_PRECISE);
    m_data->view()->StrokeArc(bRect, startAngle, angleSpan, getHaikuStrokeStyle());
    m_data->view()->SetFlags(flags);

    m_data->view()->PopState();
}

void GraphicsContext::strokePath()
{
    if (paintingDisabled())
        return;

    if (!m_data->shape())
        return;

//    m_data->view()->SetFillRule(toHaikuFillRule(fillRule()));
    m_data->view()->MovePenTo(B_ORIGIN);

    if (m_common->state.strokePattern || m_common->state.strokeGradient || strokeColor().alpha()) {
        if (m_common->state.strokePattern)
            notImplemented();
        else if (m_common->state.strokeGradient) {
            notImplemented();
//            BGradient* gradient = m_common->state.strokeGradient->platformGradient();
//            gradient->SetTransform(m_common->state.fillGradient->gradientSpaceTransform());
//            m_data->view()->StrokeShape(m_data->shape(), *gradient);
        } else
            m_data->view()->StrokeShape(m_data->shape());
    }
    m_data->setShape(0);
}

void GraphicsContext::drawConvexPolygon(size_t pointsLength, const FloatPoint* points, bool shouldAntialias)
{
    if (paintingDisabled())
        return;

    BPoint bPoints[pointsLength];
    for (size_t i = 0; i < pointsLength; i++)
        bPoints[i] = points[i];

    m_data->view()->FillPolygon(bPoints, pointsLength);
    if (strokeStyle() != NoStroke)
        // Stroke with low color
        m_data->view()->StrokePolygon(bPoints, pointsLength, true, getHaikuStrokeStyle());
}

void GraphicsContext::fillRect(const FloatRect& rect, const Color& color, ColorSpace colorSpace)
{
    if (paintingDisabled())
        return;

    m_data->view()->PushState();
    m_data->view()->SetHighColor(color);
    if (color.hasAlpha())
        m_data->view()->SetDrawingMode(B_OP_ALPHA);
    fillRect(rect);
    m_data->view()->PopState();
}

void GraphicsContext::fillRect(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    // NOTE: Trick to implement filling rects with clipping path
    // (needed for box elements with round corners):
    // When the rect extends outside the current clipping path on
    // all sides, then we can fill the path instead of the rect.
    if (m_data->clipShape()) {
    	BRect bRect(rect);
    	BRect clipPathBounds(m_data->clipShape()->Bounds());
    	// NOTE: BShapes do not suffer the weird coordinate mixup
    	// of other drawing primitives, since the conversion would be
    	// too expensive in the app_server. Thus the right/bottom edge
    	// can be considered one pixel smaller. On screen, it will be
    	// the same again.
    	clipPathBounds.right--;
    	clipPathBounds.bottom--;
    	if (bRect.Contains(clipPathBounds)) {
    		m_data->view()->MovePenTo(B_ORIGIN);
    		m_data->view()->FillShape(m_data->clipShape());
    		return;
    	}
    }
    m_data->view()->FillRect(rect);
}

void GraphicsContext::fillRoundedRect(const IntRect& rect, const IntSize& topLeft, const IntSize& topRight, const IntSize& bottomLeft, const IntSize& bottomRight, const Color& color, ColorSpace colorSpace)
{
    if (paintingDisabled() || !color.alpha())
        return;

    BPoint points[3];
    const float kRadiusBezierScale = 0.56f;

    BShape shape;
    shape.MoveTo(BPoint(rect.x() + topLeft.width(), rect.y()));
    shape.LineTo(BPoint(rect.right() - topRight.width(), rect.y()));
    points[0].x = rect.right() - kRadiusBezierScale * topRight.width();
    points[0].y = rect.y();
    points[1].x = rect.right();
    points[1].y = rect.y() + kRadiusBezierScale * topRight.height();
    points[2].x = rect.right();
    points[2].y = rect.y() + topRight.height();
    shape.BezierTo(points);
    shape.LineTo(BPoint(rect.right(), rect.bottom() - bottomRight.height()));
    points[0].x = rect.right();
    points[0].y = rect.bottom() - kRadiusBezierScale * bottomRight.height();
    points[1].x = rect.right() - kRadiusBezierScale * bottomRight.width();
    points[1].y = rect.bottom();
    points[2].x = rect.right() - bottomRight.width();
    points[2].y = rect.bottom();
    shape.BezierTo(points);
    shape.LineTo(BPoint(rect.x() + bottomLeft.width(), rect.bottom()));
    points[0].x = rect.x() + kRadiusBezierScale * bottomLeft.width();
    points[0].y = rect.bottom();
    points[1].x = rect.x();
    points[1].y = rect.bottom() - kRadiusBezierScale * bottomRight.height();
    points[2].x = rect.x();
    points[2].y = rect.bottom() - bottomRight.height();
    shape.BezierTo(points);
    shape.LineTo(BPoint(rect.x(), rect.y() + topLeft.height()));
    points[0].x = rect.x();
    points[0].y = rect.y() - kRadiusBezierScale * topLeft.height();
    points[1].x = rect.x() + kRadiusBezierScale * topLeft.width();
    points[1].y = rect.y();
    points[2].x = rect.x() + topLeft.width();
    points[2].y = rect.y();
    shape.BezierTo(points);
    shape.Close();

    rgb_color oldColor = m_data->view()->HighColor();
    m_data->view()->SetHighColor(color);
    m_data->view()->MovePenTo(B_ORIGIN);
    m_data->view()->FillShape(&shape);
    m_data->view()->SetHighColor(oldColor);
}

void GraphicsContext::fillPath()
{
    if (paintingDisabled())
        return;

    if (!m_data->shape())
        return;

//    m_data->view()->SetFillRule(toHaikuFillRule(fillRule()));
    m_data->view()->MovePenTo(B_ORIGIN);

    if (m_common->state.fillPattern || m_common->state.fillGradient || fillColor().alpha()) {
//        drawFilledShadowPath(this, p, path); TODO: What's this shadow business?
        if (m_common->state.fillPattern)
            notImplemented();
        else if (m_common->state.fillGradient) {
            BGradient* gradient = m_common->state.fillGradient->platformGradient();
//            gradient->SetTransform(m_common->state.fillGradient->gradientSpaceTransform());
            m_data->view()->FillShape(m_data->shape(), *gradient);
        } else
            m_data->view()->FillShape(m_data->shape());
    }
    m_data->setShape(0);
}

void GraphicsContext::beginPath()
{
    m_data->setShape(new BShape());
}

void GraphicsContext::addPath(const Path& path)
{
    if (!m_data->shape())
        m_data->setShape(new BShape(*path.platformPath()));
    else
        m_data->shape()->AddShape(path.platformPath());
}

void GraphicsContext::clipPath(WindRule clipRule)
{
    if (paintingDisabled())
        return;

    notImplemented();
}

void GraphicsContext::clip(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    BRegion region(rect);
    m_data->view()->ConstrainClippingRegion(&region);
}

void GraphicsContext::clip(const Path& path)
{
    if (paintingDisabled())
        return;

    if (path.platformPath()->Bounds().IsValid())
        m_data->setClipShape(new BShape(*path.platformPath()));
    else
        m_data->setClipShape(0);
    // Clipping still not actually implemented...
    notImplemented();
}

void GraphicsContext::canvasClip(const Path& path)
{
    clip(path);
}

void GraphicsContext::clipOut(const Path& path)
{
    if (paintingDisabled())
        return;

    notImplemented();
}

void GraphicsContext::clipToImageBuffer(const FloatRect&, const ImageBuffer*)
{
    notImplemented();
}

void GraphicsContext::drawFocusRing(const Vector<Path>& paths, int width, int offset, const Color& color)
{
    if (paintingDisabled())
        return;

    notImplemented();
}

void GraphicsContext::drawFocusRing(const Vector<IntRect>& rects, int /* width */, int /* offset */, const Color& color)
{
    if (paintingDisabled())
        return;

    unsigned rectCount = rects.size();

    // FIXME: maybe we should implement this with BShape?

    if (rects.size() > 1) {
        BRegion    region;
        for (unsigned i = 0; i < rectCount; ++i)
            region.Include(BRect(rects[i]));

        m_data->view()->SetHighColor(color);
        m_data->view()->StrokeRect(region.Frame(), B_MIXED_COLORS);
    }
}

void GraphicsContext::drawLineForText(const IntPoint& origin, int width, bool printing)
{
    if (paintingDisabled())
        return;

    IntPoint endPoint = origin + IntSize(width, 0);
    drawLine(origin, endPoint);
}

void GraphicsContext::drawLineForMisspellingOrBadGrammar(const IntPoint&, int width, bool grammar)
{
    if (paintingDisabled())
        return;

    notImplemented();
}

FloatRect GraphicsContext::roundToDevicePixels(const FloatRect& rect)
{
    FloatRect rounded(rect);
    rounded.setX(roundf(rect.x()));
    rounded.setY(roundf(rect.y()));
    rounded.setWidth(roundf(rect.width()));
    rounded.setHeight(roundf(rect.height()));
    return rounded;
}

void GraphicsContext::beginTransparencyLayer(float opacity)
{
    if (paintingDisabled())
        return;

    m_data->pushLayer(opacity);
}

void GraphicsContext::endTransparencyLayer()
{
    if (paintingDisabled())
        return;

    m_data->popLayer();
}

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

void GraphicsContext::strokeRect(const FloatRect& rect, float width)
{
    if (paintingDisabled())
        return;

    float oldSize = m_data->view()->PenSize();
    m_data->view()->SetPenSize(width);
    m_data->view()->StrokeRect(rect, getHaikuStrokeStyle());
    m_data->view()->SetPenSize(oldSize);
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

void GraphicsContext::setLineDash(const DashArray& dashes, float dashOffset)
{
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

void GraphicsContext::setAlpha(float opacity)
{
    if (paintingDisabled())
        return;

    m_data->m_currentLayer->globalAlpha = (uint8)(opacity * 255.0f);
}

void GraphicsContext::setCompositeOperation(CompositeOperator op)
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
        mode = B_OP_OVER;
        break;
    default:
        printf("GraphicsContext::setCompositeOperation: Unsupported composite operation %s\n",
                compositeOperatorName(op).utf8().data());
    }
    m_data->view()->SetDrawingMode(mode);
}

AffineTransform GraphicsContext::getCTM() const
{
    // TODO: Maybe this needs to use the accumulated transform?
    AffineTransform matrix;
    BPoint origin = m_data->view()->Origin();
    matrix.translate(origin.x, origin.y);
    matrix.scale(m_data->view()->Scale());
    return matrix;
}

void GraphicsContext::translate(float x, float y)
{
    if (paintingDisabled())
        return;

    m_data->m_currentLayer->accumulatedOrigin.x += x;
    m_data->m_currentLayer->accumulatedOrigin.y += y;
    BPoint origin(m_data->view()->Origin());
    m_data->view()->SetOrigin(origin.x + x, origin.y + y);

    // TODO: currentPath needs to be translated along, according to Qt implementation
}

IntPoint GraphicsContext::origin()
{
    BPoint origin = m_data->view()->Origin();
    return IntPoint(origin.x, origin.y);
}

void GraphicsContext::rotate(float radians)
{
    if (paintingDisabled())
        return;

    notImplemented();
}

void GraphicsContext::scale(const FloatSize& size)
{
    if (paintingDisabled())
        return;

    // NOTE: Non-uniform scaling not supported on Haiku, yet.
    m_data->view()->SetScale((size.width() + size.height()) / 2);
}

void GraphicsContext::clipOut(const IntRect& rect)
{
    if (paintingDisabled())
        return;

    BRegion region(m_data->view()->Bounds());
    region.Exclude(rect);
    m_data->view()->ConstrainClippingRegion(&region);
}

void GraphicsContext::clipOutEllipseInRect(const IntRect& rect)
{
    if (paintingDisabled())
        return;

    notImplemented();
}

void GraphicsContext::addInnerRoundedRectClip(const IntRect& rect, int thickness)
{
    if (paintingDisabled())
        return;

    // NOTE: Used by RenderBoxModelObject to clip out the inner part of an arc when rending box corners...
    // TODO: Use this method to detect if we are rendering a round-corner-box...

    notImplemented();
}

void GraphicsContext::concatCTM(const AffineTransform& transform)
{
    if (paintingDisabled())
        return;

    notImplemented();
}

void GraphicsContext::setPlatformShouldAntialias(bool enable)
{
    if (paintingDisabled())
        return;

    notImplemented();
}

void GraphicsContext::setImageInterpolationQuality(InterpolationQuality)
{
}

void GraphicsContext::setURLForRect(const KURL& link, const IntRect& destRect)
{
    notImplemented();
}

void GraphicsContext::setPlatformFont(const Font& font)
{
    m_data->view()->SetFont(font.primaryFont()->platformData().font());
}

void GraphicsContext::setPlatformStrokeColor(const Color& color, ColorSpace colorSpace)
{
    if (paintingDisabled())
        return;

    m_data->view()->SetHighColor(color);
}

pattern GraphicsContext::getHaikuStrokeStyle()
{
    switch (strokeStyle()) {
    case SolidStroke:
        return B_SOLID_HIGH;
        break;
    case DottedStroke:
        return B_MIXED_COLORS;
        break;
    case DashedStroke:
        // FIXME: use a better dashed stroke!
        notImplemented();
        return B_MIXED_COLORS;
        break;
    default:
        return B_SOLID_LOW;
        break;
    }
}

void GraphicsContext::setPlatformStrokeStyle(const StrokeStyle& strokeStyle)
{
    // FIXME: see getHaikuStrokeStyle.
    notImplemented();
}

void GraphicsContext::setPlatformStrokeThickness(float thickness)
{
    if (paintingDisabled())
        return;

    m_data->view()->SetPenSize(thickness);
}

void GraphicsContext::setPlatformFillColor(const Color& color, ColorSpace colorSpace)
{
    if (paintingDisabled())
        return;

    m_data->view()->SetHighColor(color);
}

void GraphicsContext::clearPlatformShadow()
{
    notImplemented();
}

void GraphicsContext::setPlatformShadow(IntSize const&, int, Color const&, ColorSpace)
{
    notImplemented();
}

} // namespace WebCore

