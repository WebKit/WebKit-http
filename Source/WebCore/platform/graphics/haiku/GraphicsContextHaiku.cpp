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
#include "Color.h"
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

    struct Layer {
    public:
        Layer(BView* _view)
            : view(_view)
            , bitmap(nullptr)
            , clipping()
            , clippingSet(false)
            , opacity(255)
            , globalAlpha(1.f)
            , currentShape(nullptr)
            , locationInParent(B_ORIGIN)
            , accumulatedOrigin(B_ORIGIN)
            , previous(nullptr)
        {
        }
        Layer(Layer* previous)
            : view(nullptr)
            , bitmap(nullptr)
            , clipping()
            , clippingSet(false)
            , opacity(255)
            , globalAlpha(1.f)
            , currentShape(nullptr)
            , locationInParent(B_ORIGIN)
            , accumulatedOrigin(B_ORIGIN)
            , previous(previous)
        {
            BRegion parentClipping;
            previous->view->GetClippingRegion(&parentClipping);
            BRect frameInParent = parentClipping.Frame();
            if (!frameInParent.IsValid())
                frameInParent = previous->view->Bounds();

            // TODO: locationInParent and accumulatedOrigin can
            // probably somehow be merged. But for now it works.
            accumulatedOrigin.x = -frameInParent.left;
            accumulatedOrigin.y = -frameInParent.top;
            locationInParent += frameInParent.LeftTop();

            // FIXME not so good design here. Creating a BBitmap that accept
            // views is not cheap (spawns a window thread in app_server, etc),
            // and layers are created and deleted a lot in WebKit. We really
            // need a cheaper way to handle this.
            frameInParent.OffsetTo(B_ORIGIN);
            view = new BView(frameInParent, "WebCore transparency layer", 0, 0);
            bitmap = new BBitmap(frameInParent, B_RGBA32, true);
            bitmap->Lock();
            bitmap->AddChild(view);

            view->SetHighColor(255, 255, 255, 0);
            view->FillRect(view->Bounds());

            view->SetHighColor(previous->view->HighColor());
            view->SetLowColor(previous->view->LowColor());
            view->SetViewColor(previous->view->ViewColor());
            view->SetDrawingMode(previous->view->DrawingMode());
            view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
            view->SetOrigin(accumulatedOrigin);
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
        }

        BView* view;
        BBitmap* bitmap;
        BRegion clipping;
        bool clippingSet;
        uint8 opacity;
        float globalAlpha;
        BShape* currentShape;
        BPoint locationInParent;
        BPoint accumulatedOrigin;

        Layer* previous;
    };

    struct CustomGraphicsState {
        CustomGraphicsState()
            : previous(0)
            , imageInterpolationQuality(InterpolationDefault)
        {
        }

        CustomGraphicsState(CustomGraphicsState* previous)
            : previous(previous)
            , imageInterpolationQuality(previous->imageInterpolationQuality)
        {
        }

        CustomGraphicsState* previous;
        InterpolationQuality imageInterpolationQuality;

        bool clippingSet;
        BRegion clippingRegion;
        BPicture clipOutPicture;
    };

    void pushState()
    {
    	m_graphicsState = new CustomGraphicsState(m_graphicsState);
        view()->PushState();

        m_graphicsState->clippingSet = m_currentLayer->clippingSet;
        m_graphicsState->clippingRegion = m_currentLayer->clipping;
        resetClipping();
    }

    void popState()
    {
    	ASSERT(m_graphicsState->previous);
    	if (!m_graphicsState->previous)
    	    return;

    	CustomGraphicsState* oldTop = m_graphicsState;
    	m_graphicsState = oldTop->previous;
    	delete oldTop;

        m_currentLayer->accumulatedOrigin -= view()->Origin();
        view()->PopState();

        m_currentLayer->clippingSet = m_graphicsState->clippingSet;
        m_currentLayer->clipping = m_graphicsState->clippingRegion;
    }

    BView* view() const
    {
        return m_currentLayer->view;
    }

    // Unlike in Haiku, all clipping operations are cumulative. It's possible
    // to clip several times, and the intersection of all the clipping path is
    // used. In Haiku, calling ConstrainClippingRegion or ClipToPicture removes
    // existing clippings at the same level. So, we have to implement the
    // combination of clipping levels here.
    void constrainClipping(const BRegion& region)
    {
        // Region-clipping in Haiku applies the clipping after the view
        // transformation, whereas WebKit would want it applied before.
        // However, the fast region clipping that reduces the viewport to
        // small dimensions when drawing is an essential element in getting
        // acceptable performances when drawing (performing alpha-blended
        // picture clipping on the whole view is extremely slow).
        // However, there is no way to transform a region with an arbitrary
        // transformation. The result would not be represented as a set of
        // grid-aligned (vertical/horizontal) rectangles.
        // So, the idea is to use region-clipping when the transformation is
        // simple enough (translation + scaling only), and picture-based
        // clipping for more complex cases.
        BAffineTransform t = m_currentLayer->view->Transform();
        BRegion regionCopy(region);
        // region.ScaleBy(t.sx, t.sy);
        regionCopy.OffsetBy(t.tx, t.ty);

        // Matrix is made only of translation and scaling. We can transform
        // the region safely.
        if (m_currentLayer->clippingSet)
            m_currentLayer->clipping.IntersectWith(&regionCopy);
        else
            m_currentLayer->clipping = regionCopy;

        m_currentLayer->clippingSet = true;
        m_currentLayer->view->ConstrainClippingRegion(
            &m_currentLayer->clipping);
    }

    void excludeClipping(const FloatRect& rect)
    {
        if (!m_currentLayer->clippingSet) {
            BRegion region(m_currentLayer->view->Bounds());
            m_currentLayer->clipping = region;
            m_currentLayer->clippingSet = true;
        }

        m_currentLayer->clipping.Exclude(rect);
        m_currentLayer->view->ConstrainClippingRegion(&m_currentLayer->clipping);
    }

    BRegion& clipping()
    {
        return m_currentLayer->clipping;
    }

    void resetClipping()
    {
        m_currentLayer->clippingSet = false;
    	m_currentLayer->clipping = BRegion();
        m_currentLayer->view->ConstrainClippingRegion(NULL);
    }


    void clipToShape(BShape* shape, bool inverse, WindRule windRule)
    {
        // FIXME calling clipToShape several times without interleaved
        // Push/PopState should still intersect the clipping. In Haiku, it is
        // replaced instead. So, we must keep the clipping picture ourselves
        // and handle the intersection.

        BPicture picture;
        BView* view = m_currentLayer->view;
        view->LockLooper();

        if (inverse)
            view->AppendToPicture(&m_graphicsState->clipOutPicture);
        else
            view->BeginPicture(&picture);
        view->PushState();

        view->SetLowColor(make_color(255, 255, 255, 0));
        view->SetViewColor(make_color(255, 255, 255, 0));
        view->SetHighColor(make_color(0, 0, 0, 255));
        view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
        view->SetFillRule(windRule == RULE_EVENODD ? B_EVEN_ODD : B_NONZERO);

        view->FillShape(shape);

        view->PopState();
        BPicture* result = view->EndPicture();

        if (inverse)
            view->ClipToInversePicture(result);
        else
            view->ClipToPicture(result);

        view->UnlockLooper();
    }

    void pushLayer(float opacity)
    {
        m_currentLayer = new Layer(m_currentLayer);
        m_currentLayer->opacity = (uint8)(opacity * 255.0);
        m_currentLayer->view->SetOrigin(m_currentLayer->accumulatedOrigin + m_scrollPos);
    }

    void popLayer()
    {
        if (!m_currentLayer->previous)
            return;
        Layer* layer = m_currentLayer;
        m_currentLayer = layer->previous;
        if (layer->opacity > 0) {
            layer->view->Sync();

            BView* target = m_currentLayer->view;
            target->PushState();

            // We use a clipping picture which covers the current target bounds
            // and is filled with white at the right opacity. The effect is
            // compositing this opacity with the layer bitmap one as it is
            // blended onto the target, giving us the correct result even if
            // the bitmap itself has some partially transparent areas.
            // TODO this relies on the compositing operation to do the right
            // thing with the alpha channel, but right now it doesn't really
            // work.
            BPoint bitmapLocation(layer->locationInParent);

            BPicture picture;
            target->BeginPicture(&picture);

            target->SetDrawingMode(B_OP_COPY);
            target->SetHighColor(make_color(255, 255, 255, layer->opacity));

            target->FillRect(target->Bounds());

            target->EndPicture();
            target->ClipToPicture(&picture);

            target->SetDrawingMode(B_OP_ALPHA);

            target->DrawBitmap(layer->bitmap, bitmapLocation - m_scrollPos);
            target->PopState();
        }
        delete layer;
    }

    ShadowBlur& shadowBlur()
    {
        return blur;
    }

    Layer* m_currentLayer;
    CustomGraphicsState* m_graphicsState;
    ShadowBlur blur;
    pattern m_strokeStyle;
    BPoint m_scrollPos;
};

GraphicsContextPlatformPrivate::GraphicsContextPlatformPrivate(BView* view)
    : m_currentLayer(new Layer(view))
    , m_graphicsState(new CustomGraphicsState)
    , m_strokeStyle(B_SOLID_HIGH)
{
}

GraphicsContextPlatformPrivate::~GraphicsContextPlatformPrivate()
{
    while (Layer* previous = m_currentLayer->previous) {
        delete m_currentLayer;
        m_currentLayer = previous;
    }
    delete m_currentLayer;

    while (CustomGraphicsState* previous = m_graphicsState->previous) {
        delete m_graphicsState;
        m_graphicsState = previous;
    }
    delete m_graphicsState;
}

void GraphicsContext::platformInit(PlatformGraphicsContext* context)
{
    m_data = new GraphicsContextPlatformPrivate(context);
    setPaintingDisabled(!context);
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

void GraphicsContext::drawConvexPolygon(size_t pointsLength, const FloatPoint* points, bool /*shouldAntialias*/)
{
    if (paintingDisabled())
        return;

    BPoint bPoints[pointsLength];
    for (size_t i = 0; i < pointsLength; i++) {
        bPoints[i] = points[i];
    }

    if (fillColor().alpha())
        m_data->view()->FillPolygon(bPoints, pointsLength);

    if (strokeStyle() != NoStroke) {
        // Stroke with low color
        m_data->view()->StrokePolygon(bPoints, pointsLength, true, m_data->m_strokeStyle);
    }
}

void GraphicsContext::clipConvexPolygon(size_t numPoints, const FloatPoint* points, bool /*antialiased*/)
{
    if (paintingDisabled())
        return;

    BShape* shape = new BShape();
    shape->MoveTo(points[0]);
    for(unsigned int i = 1; i < numPoints; i ++)
    {
        shape->LineTo(points[i]);
    }
    shape->Close();
    m_data->clipToShape(shape, false, fillRule());
}

void GraphicsContext::fillRect(const FloatRect& rect, const Color& color, ColorSpace /*colorSpace*/)
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

void GraphicsContext::platformFillRoundedRect(const FloatRoundedRect& roundRect, const Color& color, ColorSpace /*colorSpace*/)
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
    setPlatformFillColor(color, ColorSpaceDeviceRGB);
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

void GraphicsContext::clipPath(const Path& path, WindRule windRule)
{
    clip(path, windRule);
}

void GraphicsContext::clip(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    if(rect.isEmpty())
        m_data->constrainClipping(BRegion()); // Clip everything
    else if(!rect.isInfinite()) {
        BRegion newRegion(rect);
        m_data->constrainClipping(newRegion);
    }
}

IntRect GraphicsContext::clipBounds() const
{
    BRect r(m_data->clipping().Frame());
    if(!r.IsValid()) {
        // No clipping, return view bounds
        return IntRect(m_data->view()->Bounds());
    }

    return IntRect(r);
}

void GraphicsContext::clip(const Path& path, WindRule windRule)
{
    if (paintingDisabled())
        return;

    m_data->clipToShape(path.platformPath(), false, windRule);
}

void GraphicsContext::canvasClip(const Path& path, WindRule windRule)
{
    clip(path, windRule);
}

void GraphicsContext::clipOut(const Path& path)
{
    if (paintingDisabled())
        return;

    if (path.isEmpty()) {
        return;
    }
    m_data->clipToShape(path.platformPath(), true, fillRule());
}

void GraphicsContext::clipOut(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    if(rect.isInfinite())
        m_data->constrainClipping(BRegion()); // Clip everything
    else if(!rect.isEmpty()) {
        m_data->excludeClipping(rect);
    }
}

void GraphicsContext::drawFocusRing(const Path& path, int width, int /*offset*/, const Color& color)
{
    if (paintingDisabled() || width <= 0 || color.alpha() == 0)
        return;

    // GTK forces this to 2, we use 1. A focus ring several pixels thick doesn't
    // look good.
    width = 1;

    m_data->view()->PushState();
    setPlatformFillColor(color, ColorSpaceDeviceRGB);
    m_data->view()->SetPenSize(width);
    m_data->view()->StrokeShape(path.platformPath(), B_SOLID_HIGH);
    m_data->view()->PopState();
}

void GraphicsContext::drawFocusRing(const Vector<IntRect>& rects, int width, int /* offset */, const Color& color)
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

    setPlatformFillColor(color, ColorSpaceDeviceRGB);
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

    m_data->pushLayer(opacity);
}

void GraphicsContext::endPlatformTransparencyLayer()
{
    if (paintingDisabled())
        return;

    m_data->popLayer();
}

bool GraphicsContext::supportsTransparencyLayers()
{
    return false;
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
    m_data->m_currentLayer->globalAlpha = opacity;
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
}

AffineTransform GraphicsContext::getCTM(IncludeDeviceScale) const
{
    // TODO: Maybe this needs to use the accumulated transform?
    BAffineTransform t = m_data->view()->Transform();
    AffineTransform matrix(t.sx, t.shy, t.shx, t.sy, t.tx, t.ty);

    // TODO the translation would better be handled directly in the matrix?
    BPoint origin = m_data->view()->Origin();
    matrix.translate(origin.x, origin.y);

    return matrix;
}

void GraphicsContext::translate(float x, float y)
{
    if (paintingDisabled() || (x == 0.f && y == 0.f))
        return;

#if 0
    // FIXME it would be simpler to use the BAffineTransform here , but it has
    // clipping problems. For now we avoid this by using SetOrigin instead.
    BAffineTransform current = m_data->view()->Transform();
    current.TranslateBy(x, y);
    m_data->view()->SetTransform(current);
#endif
    m_data->m_scrollPos = BPoint(x, y);
    BPoint org = m_data->view()->Origin() + m_data->m_scrollPos;
    m_data->view()->SetOrigin(org);
}

void GraphicsContext::rotate(float radians)
{
    if (paintingDisabled() || radians == 0.f)
        return;

    BAffineTransform current = m_data->view()->Transform();
    current.RotateBy(radians);
    m_data->view()->SetTransform(current);
}

void GraphicsContext::scale(const FloatSize& size)
{
    if (paintingDisabled())
        return;

    BAffineTransform current = m_data->view()->Transform();
    current.ScaleBy(size.width(), size.height());
    m_data->view()->SetTransform(current);
}

void GraphicsContext::concatCTM(const AffineTransform& transform)
{
    if (paintingDisabled())
        return;

    BAffineTransform current = m_data->view()->Transform();
    current.Multiply(transform);
        // Should we use PreMultiply?
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

void GraphicsContext::setImageInterpolationQuality(InterpolationQuality quality)
{
    m_data->m_graphicsState->imageInterpolationQuality = quality;
}

InterpolationQuality GraphicsContext::imageInterpolationQuality() const
{
    return m_data->m_graphicsState->imageInterpolationQuality;
}

void GraphicsContext::setURLForRect(const URL& /*link*/, const IntRect& /*destRect*/)
{
    notImplemented();
}

void GraphicsContext::setPlatformStrokeColor(const Color& color, ColorSpace /*colorSpace*/)
{
    if (paintingDisabled())
        return;

    // NOTE: In theory, we should be able to use the low color and
    // return B_SOLID_LOW for the SolidStroke case in setPlatformStrokeStyle()
    // below. More stuff needs to be fixed, though, it will for example
    // prevent the text caret from rendering.
//    m_data->view()->SetLowColor(color);
    setPlatformFillColor(color, ColorSpaceDeviceRGB);
}

void GraphicsContext::setPlatformStrokeStyle(StrokeStyle strokeStyle)
{
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

void GraphicsContext::setPlatformFillColor(const Color& color, ColorSpace /*colorSpace*/)
{
    if (paintingDisabled())
        return;

    rgb_color fixed = color;
    fixed.alpha *= m_data->m_currentLayer->globalAlpha;
    m_data->view()->SetHighColor(fixed);
}

void GraphicsContext::clearPlatformShadow()
{
    if (paintingDisabled())
        return;

    m_data->shadowBlur().clear();
}

void GraphicsContext::setPlatformShadow(FloatSize const& size, float /*blur*/,
    Color const& /*color*/, ColorSpace)
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
                                                    m_state.shadowColorSpace,
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

