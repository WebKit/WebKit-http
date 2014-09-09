/*
 * Copyright (c) 2012, Google Inc. All rights reserved.
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

#ifndef LayoutRect_h
#define LayoutRect_h

#include "FloatRect.h"
#include "IntRect.h"
#include "LayoutBoxExtent.h"
#include "LayoutPoint.h"
#include <wtf/Vector.h>

namespace WebCore {

class LayoutRect {
public:
    LayoutRect() { }
    LayoutRect(const LayoutPoint& location, const LayoutSize& size)
        : m_location(location), m_size(size) { }
    LayoutRect(LayoutUnit x, LayoutUnit y, LayoutUnit width, LayoutUnit height)
        : m_location(LayoutPoint(x, y)), m_size(LayoutSize(width, height)) { }
    LayoutRect(const FloatPoint& location, const FloatSize& size)
        : m_location(location), m_size(size) { }
    LayoutRect(const IntRect& rect) : m_location(rect.location()), m_size(rect.size()) { }
    
    WEBCORE_EXPORT explicit LayoutRect(const FloatRect&); // don't do this implicitly since it's lossy
        
    LayoutPoint location() const { return m_location; }
    LayoutSize size() const { return m_size; }

    IntSize pixelSnappedSize() const { return snappedIntSize(m_size, m_location); }

    void setLocation(const LayoutPoint& location) { m_location = location; }
    void setSize(const LayoutSize& size) { m_size = size; }

    LayoutUnit x() const { return m_location.x(); }
    LayoutUnit y() const { return m_location.y(); }
    LayoutUnit maxX() const { return x() + width(); }
    LayoutUnit maxY() const { return y() + height(); }
    LayoutUnit width() const { return m_size.width(); }
    LayoutUnit height() const { return m_size.height(); }

    void setX(LayoutUnit x) { m_location.setX(x); }
    void setY(LayoutUnit y) { m_location.setY(y); }
    void setWidth(LayoutUnit width) { m_size.setWidth(width); }
    void setHeight(LayoutUnit height) { m_size.setHeight(height); }

    bool isEmpty() const { return m_size.isEmpty(); }

    // NOTE: The result is rounded to integer values, and thus may be not the exact
    // center point.
    LayoutPoint center() const { return LayoutPoint(x() + width() / 2, y() + height() / 2); }

    void move(const LayoutSize& size) { m_location += size; } 
    void moveBy(const LayoutPoint& offset) { m_location.move(offset.x(), offset.y()); }
    void move(LayoutUnit dx, LayoutUnit dy) { m_location.move(dx, dy); } 

    void expand(const LayoutSize& size) { m_size += size; }
    void expand(const LayoutBoxExtent& box)
    {
        m_location.move(-box.left(), -box.top());
        m_size.expand(box.left() + box.right(), box.top() + box.bottom());
    }
    void expand(LayoutUnit dw, LayoutUnit dh) { m_size.expand(dw, dh); }
    void contract(const LayoutSize& size) { m_size -= size; }
    void contract(const LayoutBoxExtent& box)
    {
        m_location.move(box.left(), box.top());
        m_size.shrink(box.left() + box.right(), box.top() + box.bottom());
    }
    void contract(LayoutUnit dw, LayoutUnit dh) { m_size.expand(-dw, -dh); }

    void shiftXEdgeTo(LayoutUnit edge)
    {
        LayoutUnit delta = edge - x();
        setX(edge);
        setWidth(std::max<LayoutUnit>(0, width() - delta));
    }
    void shiftMaxXEdgeTo(LayoutUnit edge)
    {
        LayoutUnit delta = edge - maxX();
        setWidth(std::max<LayoutUnit>(0, width() + delta));
    }
    void shiftYEdgeTo(LayoutUnit edge)
    {
        LayoutUnit delta = edge - y();
        setY(edge);
        setHeight(std::max<LayoutUnit>(0, height() - delta));
    }
    void shiftMaxYEdgeTo(LayoutUnit edge)
    {
        LayoutUnit delta = edge - maxY();
        setHeight(std::max<LayoutUnit>(0, height() + delta));
    }

    LayoutPoint minXMinYCorner() const { return m_location; } // typically topLeft
    LayoutPoint maxXMinYCorner() const { return LayoutPoint(m_location.x() + m_size.width(), m_location.y()); } // typically topRight
    LayoutPoint minXMaxYCorner() const { return LayoutPoint(m_location.x(), m_location.y() + m_size.height()); } // typically bottomLeft
    LayoutPoint maxXMaxYCorner() const { return LayoutPoint(m_location.x() + m_size.width(), m_location.y() + m_size.height()); } // typically bottomRight
    
    bool intersects(const LayoutRect&) const;
    WEBCORE_EXPORT bool contains(const LayoutRect&) const;

    // This checks to see if the rect contains x,y in the traditional sense.
    // Equivalent to checking if the rect contains a 1x1 rect below and to the right of (px,py).
    bool contains(LayoutUnit px, LayoutUnit py) const
        { return px >= x() && px < maxX() && py >= y() && py < maxY(); }
    bool contains(const LayoutPoint& point) const { return contains(point.x(), point.y()); }

    void intersect(const LayoutRect&);
    WEBCORE_EXPORT void unite(const LayoutRect&);
    void uniteIfNonZero(const LayoutRect&);

    void inflateX(LayoutUnit dx)
    {
        m_location.setX(m_location.x() - dx);
        m_size.setWidth(m_size.width() + dx + dx);
    }
    void inflateY(LayoutUnit dy)
    {
        m_location.setY(m_location.y() - dy);
        m_size.setHeight(m_size.height() + dy + dy);
    }
    void inflate(LayoutUnit d) { inflateX(d); inflateY(d); }
    WEBCORE_EXPORT void scale(float s);
    void scale(float xScale, float yScale);

    LayoutRect transposedRect() const { return LayoutRect(m_location.transposedPoint(), m_size.transposedSize()); }

    static LayoutRect infiniteRect()
    {
        // Return a rect that is slightly smaller than the true max rect to allow pixelSnapping to round up to the nearest IntRect without overflowing.
        return LayoutRect(LayoutUnit::nearlyMin() / 2, LayoutUnit::nearlyMin() / 2, LayoutUnit::nearlyMax(), LayoutUnit::nearlyMax());
    }
    
    operator FloatRect() const { return FloatRect(m_location, m_size); }

private:
    LayoutPoint m_location;
    LayoutSize m_size;
};

inline LayoutRect intersection(const LayoutRect& a, const LayoutRect& b)
{
    LayoutRect c = a;
    c.intersect(b);
    return c;
}

inline LayoutRect unionRect(const LayoutRect& a, const LayoutRect& b)
{
    LayoutRect c = a;
    c.unite(b);
    return c;
}

LayoutRect unionRect(const Vector<LayoutRect>&);

inline bool operator==(const LayoutRect& a, const LayoutRect& b)
{
    return a.location() == b.location() && a.size() == b.size();
}

inline bool operator!=(const LayoutRect& a, const LayoutRect& b)
{
    return a.location() != b.location() || a.size() != b.size();
}

// Integral snapping functions.
inline IntRect snappedIntRect(const LayoutRect& rect)
{
    return IntRect(roundedIntPoint(rect.location()), snappedIntSize(rect.size(), rect.location()));
}

inline IntRect snappedIntRect(LayoutUnit left, LayoutUnit top, LayoutUnit width, LayoutUnit height)
{
    return IntRect(IntPoint(left.round(), top.round()), snappedIntSize(LayoutSize(width, height), LayoutPoint(left, top)));
}

inline IntRect snappedIntRect(LayoutPoint location, LayoutSize size)
{
    return IntRect(roundedIntPoint(location), snappedIntSize(size, location));
}

WEBCORE_EXPORT IntRect enclosingIntRect(const LayoutRect&);
WEBCORE_EXPORT LayoutRect enclosingLayoutRect(const FloatRect&);

// Device pixel snapping functions.
inline FloatRect snapRectToDevicePixels(const LayoutRect& rect, float pixelSnappingFactor)
{
    return FloatRect(FloatPoint(roundToDevicePixel(rect.x(), pixelSnappingFactor), roundToDevicePixel(rect.y(), pixelSnappingFactor)), snapSizeToDevicePixel(rect.size(), rect.location(), pixelSnappingFactor));
}

inline FloatRect snapRectToDevicePixels(LayoutUnit x, LayoutUnit y, LayoutUnit width, LayoutUnit height, float pixelSnappingFactor)
{
    return snapRectToDevicePixels(LayoutRect(x, y, width, height), pixelSnappingFactor);
}

// FIXME: This needs to take vertical centering into account too.
inline FloatRect snapRectToDevicePixelsWithWritingDirection(const LayoutRect& rect, float deviceScaleFactor, bool ltr)
{
    if (!ltr) {
        FloatPoint snappedTopRight = roundPointToDevicePixels(rect.maxXMinYCorner(), deviceScaleFactor, ltr);
        FloatSize snappedSize = snapSizeToDevicePixel(rect.size(), rect.maxXMinYCorner(), deviceScaleFactor);
        return FloatRect(snappedTopRight.x() - snappedSize.width(), snappedTopRight.y(), snappedSize.width(), snappedSize.height());
    }
    return snapRectToDevicePixels(rect, deviceScaleFactor);
}

FloatRect encloseRectToDevicePixels(const LayoutRect&, float pixelSnappingFactor);

} // namespace WebCore

#endif // LayoutRect_h
