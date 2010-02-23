/*
 * Copyright (C) 2007 Ryan Leavengood <leavengood@gmail.com>
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 * Copyright (C) 2010 Michael Lotz <mmlr@mlotz.ch>
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
#include "Path.h"

#include "AffineTransform.h"
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "NotImplemented.h"
#include "PlatformString.h"
#include "StrokeStyleApplier.h"
#include "TransformationMatrix.h"
#include <Bitmap.h>
#include <Shape.h>
#include <View.h>

namespace WebCore {

// A one pixel sized BBitmap for drawing into. Default high-color of BViews
// is black. So testing of any color channel for < 128 means the pixel was hit.
class HitTestBitmap {
public:
    HitTestBitmap()
        : m_bitmap(0)
        , m_view(0)
    {
        // Do not create the bitmap here, since this object is initialized
        // statically, and BBitmaps need a running BApplication to work.
    }

    ~HitTestBitmap()
    {
        // m_bitmap being 0 simply means WebCore never performed any hit-tests
        // on Paths.
        if (!m_bitmap)
            return;

        m_bitmap->Unlock();
        // Will also delete the BView attached to the bitmap:
        delete m_bitmap;
    }

    void init()
    {
        if (m_bitmap)
            return;

        m_bitmap = new BBitmap(BRect(0, 0, 0, 0), B_RGB32, true);
        // Keep the dummmy window locked at all times, so we don't need
        // to worry about it anymore.
        m_bitmap->Lock();
        // Add a BView which does any rendering.
        m_view = new BView(m_bitmap->Bounds(), "WebCore hi-test view", 0, 0);
        m_bitmap->AddChild(m_view);

        clearToWhite();
    }

    void clearToWhite()
    {
        memset(m_bitmap->Bits(), 255, m_bitmap->BitsLength());
    }

    void prepareHitTest(float x, float y)
    {
        clearToWhite();
        // The current pen location is used as the reference point for
        // where the shape is rendered in the view. Obviously Be thought this
        // was a neat idea for using BShapes as symbols, although it is
        // inconsistent with drawing other primitives. SetOrigin() should
        // be used instead, but this is cheaper:
        m_view->MovePenTo(-x, -y);
    }

    bool hitTest(BShape* shape, float x, float y, WindRule rule)
    {
        prepareHitTest(x, y);

        m_view->FillShape(shape);

        return hitTestPixel();
    }

    bool hitTest(BShape* shape, float x, float y, StrokeStyleApplier* applier)
    {
        prepareHitTest(x, y);

        GraphicsContext context(m_view);
        applier->strokeStyle(&context);
        m_view->StrokeShape(shape);

        return hitTestPixel();
    }

    bool hitTestPixel() const
    {
        // Make sure the app_server has rendered everything already.
        m_view->Sync();
        // Bitmap is white before drawing, anti-aliasing threshold is mid-grey,
        // then the pixel is considered within the black shape. Theoretically,
        // it would be enough to test one color channel, but since the Haiku
        // app_server renders all shapes with LCD sub-pixel anti-aliasing, the
        // color channels can in fact differ at edge pixels.
        const uint8* bits = reinterpret_cast<const uint8*>(m_bitmap->Bits());
        return (static_cast<uint16>(bits[0]) + bits[1] + bits[2]) / 3 < 128;
    }

private:
    BBitmap* m_bitmap;
    BView* m_view;
};

// Reuse the same hit test bitmap for all paths. Initialization is lazy, and
// needs to be, since BBitmaps need a running BApplication. If WebCore ever
// runs each Document in it's own thread, we shall re-use the internal bitmap's
// lock to synchronize access. Since the pointer is likely only sitting above
// one document at a time, it seems unlikely to be a problem anyway.
static HitTestBitmap gHitTestBitmap;

// #pragma mark - Path

Path::Path()
    : m_path(new BShape())
{
}

Path::Path(const Path& other)
    : m_path(new BShape(*other.platformPath()))
{
}

Path::~Path()
{
    delete m_path;
}

Path& Path::operator=(const Path& other)
{
    if (&other != this) {
        m_path->Clear();
        m_path->AddShape(other.platformPath());
    }

    return *this;
}

bool Path::hasCurrentPoint() const
{
    return !isEmpty();
}

bool Path::contains(const FloatPoint& point, WindRule rule) const
{
    gHitTestBitmap.init();
    return gHitTestBitmap.hitTest(m_path, point.x(), point.y(), rule);
}

bool Path::strokeContains(StrokeStyleApplier* applier, const FloatPoint& point) const
{
    ASSERT(applier);

    gHitTestBitmap.init();
    return gHitTestBitmap.hitTest(m_path, point.x(), point.y(), applier);
}

void Path::translate(const FloatSize& size)
{
    // BShapeIterator allows us to modify the path data "in place"
    class TranslateIterator : public BShapeIterator {
    public:
        TranslateIterator(BShape* shape, const FloatSize& size)
            : m_shape(shape)
            , m_size(size)
        {
        }
        virtual status_t IterateMoveTo(BPoint* point)
        {
            point->x += m_size.width();
            point->y += m_size.height();
            return B_OK;
        }

        virtual status_t IterateLineTo(int32 lineCount, BPoint* linePts)
        {
            while (lineCount--) {
                linePts->x += m_size.width();
                linePts->y += m_size.height();
                linePts++;
            }
            return B_OK;
        }

        virtual status_t IterateBezierTo(int32 bezierCount, BPoint* bezierPts)
        {
            while (bezierCount--) {
                bezierPts[0].x += m_size.width();
                bezierPts[0].y += m_size.height();
                bezierPts[1].x += m_size.width();
                bezierPts[1].y += m_size.height();
                bezierPts[2].x += m_size.width();
                bezierPts[2].y += m_size.height();
                bezierPts += 3;
            }
            return B_OK;
        }

        virtual status_t IterateClose()
        {
            return B_OK;
        }

    private:
        BShape* m_shape;
        const FloatSize& m_size;
    } translateIterator(m_path, size);

    translateIterator.Iterate(m_path);
}

FloatRect Path::boundingRect() const
{
    return m_path->Bounds();
}

void Path::moveTo(const FloatPoint& p)
{
    m_path->MoveTo(p);
}

void Path::addLineTo(const FloatPoint& p)
{
    m_path->LineTo(p);
}

void Path::addQuadCurveTo(const FloatPoint& cp, const FloatPoint& p)
{
    BPoint control = cp;

    BPoint points[3];
    points[0] = control;
    points[0].x += (control.x - points[0].x) * (2.0 / 3.0);
    points[0].y += (control.y - points[0].y) * (2.0 / 3.0);

    points[1] = p;
    points[1].x += (control.x - points[1].x) * (2.0 / 3.0);
    points[1].y += (control.y - points[1].y) * (2.0 / 3.0);

    points[2] = p;
    m_path->BezierTo(points);
}

void Path::addBezierCurveTo(const FloatPoint& cp1, const FloatPoint& cp2, const FloatPoint& p)
{
    BPoint points[3];
    points[0] = cp1;
    points[1] = cp2;
    points[2] = p;
    m_path->BezierTo(points);
}

void Path::addArcTo(const FloatPoint& p1, const FloatPoint& p2, float radius)
{
    notImplemented();
}

void Path::closeSubpath()
{
    m_path->Close();
}

void Path::addArc(const FloatPoint& p, float r, float sar, float ear, bool anticlockwise)
{
    notImplemented();
}

void Path::addRect(const FloatRect& r)
{
    m_path->Close();
    m_path->MoveTo(BPoint(r.x(), r.y()));
    m_path->LineTo(BPoint(r.x() + r.width(), r.y()));
    m_path->LineTo(BPoint(r.x() + r.width(), r.y() + r.height()));
    m_path->LineTo(BPoint(r.x(), r.y() + r.height()));
    m_path->Close();
}

void Path::addEllipse(const FloatRect& r)
{
    notImplemented();
}

void Path::clear()
{
    m_path->Clear();
}

bool Path::isEmpty() const
{
    return !m_path->Bounds().IsValid();
}

String Path::debugString() const
{
    notImplemented();
    return String();
}

void Path::apply(void* info, PathApplierFunction function) const
{
    notImplemented();
}

void Path::transform(const AffineTransform& transform)
{
    // BShapeIterator allows us to modify the path data "in place"
    class TransformIterator : public BShapeIterator {
    public:
        TransformIterator(BShape* shape, const AffineTransform& transform)
            : m_shape(shape)
            , m_transform(transform)
        {
        }
        virtual status_t IterateMoveTo(BPoint* point)
        {
            *point = m_transform.mapPoint(*point);
            return B_OK;
        }

        virtual status_t IterateLineTo(int32 lineCount, BPoint* linePts)
        {
            while (lineCount--) {
                *linePts = m_transform.mapPoint(*linePts);
                linePts++;
            }
            return B_OK;
        }

        virtual status_t IterateBezierTo(int32 bezierCount, BPoint* bezierPts)
        {
            while (bezierCount--) {
                bezierPts[0] = m_transform.mapPoint(bezierPts[0]);
                bezierPts[1] = m_transform.mapPoint(bezierPts[1]);
                bezierPts[2] = m_transform.mapPoint(bezierPts[2]);
                bezierPts += 3;
            }
            return B_OK;
        }

        virtual status_t IterateClose()
        {
            return B_OK;
        }

    private:
        BShape* m_shape;
        const AffineTransform& m_transform;
    } transformIterator(m_path, transform);

    transformIterator.Iterate(m_path);
}

FloatRect Path::strokeBoundingRect(StrokeStyleApplier* applier)
{
    notImplemented();
    return m_path->Bounds();
}

} // namespace WebCore

