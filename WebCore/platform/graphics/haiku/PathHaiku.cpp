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

Path::Path()
    : m_path(new BShape())
{
}

Path::~Path()
{
    delete m_path;
}

Path::Path(const Path& other)
    : m_path(new BShape(*other.platformPath()))
{
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
    // TODO: This is probably too expensive. Use one instance of a scratch
    // image.
    BBitmap bitmap(BRect(0, 0, 0, 0),
        B_BITMAP_CLEAR_TO_WHITE | B_BITMAP_ACCEPTS_VIEWS, B_RGB32);
    BView view(BRect(0, 0, 0, 0), "", 0, 0);
    bitmap.Lock();
    bitmap.AddChild(&view);
    // Current pen location is used as origin for the shape.
    view.MovePenTo(-point.x(), -point.y());
    // TODO: Handle WindRule... (needs support in BView, the backend already
    // support it.)
    view.FillShape(m_path);
    view.Sync();

    uint8* bits = reinterpret_cast<uint8*>(bitmap.Bits());
    bool result = bits[0] < 128;

    view.RemoveSelf();
    bitmap.Unlock();
    return result;
}

bool Path::strokeContains(StrokeStyleApplier* applier, const FloatPoint& point) const
{
    ASSERT(applier);

    // TODO: This is probably too expensive. Use one instance of a scratch
    // image, see above.
    // TODO: Code duplication above.
    BBitmap bitmap(BRect(0, 0, 0, 0),
        B_BITMAP_CLEAR_TO_WHITE | B_BITMAP_ACCEPTS_VIEWS, B_RGB32);
    BView view(BRect(0, 0, 0, 0), "", 0, 0);
    bitmap.Lock();
    bitmap.AddChild(&view);
    // Current pen location is used as origin for the shape.
    view.MovePenTo(-point.x(), -point.y());
    // TODO: Handle WindRule... (needs support in BView, the backend already
    // support it.)
    GraphicsContext context(&view);
    applier->strokeStyle(&context);
    
    view.StrokeShape(m_path);
    view.Sync();

    uint8* bits = reinterpret_cast<uint8*>(bitmap.Bits());
    bool result = bits[0] < 128;

    view.RemoveSelf();
    bitmap.Unlock();
    return result;
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

