/*
 * Copyright (C) 2003, 2006, 2009 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "IntRect.h"

#include "FractionalLayoutRect.h"
#include "FloatRect.h"
#include <algorithm>

using std::max;
using std::min;

namespace WebCore {

IntRect::IntRect(const FloatRect& r)
    : m_location(clampToInteger(r.x()), clampToInteger(r.y()))
    , m_size(clampToInteger(r.width()), clampToInteger(r.height()))
{
}

IntRect::IntRect(const FractionalLayoutRect& r)
    : m_location(r.x(), r.y())
    , m_size(r.width(), r.height())
{
}

bool IntRect::intersects(const IntRect& other) const
{
    // Checking emptiness handles negative widths as well as zero.
    return !isEmpty() && !other.isEmpty()
        && x() < other.maxX() && other.x() < maxX()
        && y() < other.maxY() && other.y() < maxY();
}

bool IntRect::contains(const IntRect& other) const
{
    return x() <= other.x() && maxX() >= other.maxX()
        && y() <= other.y() && maxY() >= other.maxY();
}

void IntRect::intersect(const IntRect& other)
{
    int l = max(x(), other.x());
    int t = max(y(), other.y());
    int r = min(maxX(), other.maxX());
    int b = min(maxY(), other.maxY());

    // Return a clean empty rectangle for non-intersecting cases.
    if (l >= r || t >= b) {
        l = 0;
        t = 0;
        r = 0;
        b = 0;
    }

    m_location.setX(l);
    m_location.setY(t);
    m_size.setWidth(r - l);
    m_size.setHeight(b - t);
}

void IntRect::unite(const IntRect& other)
{
    // Handle empty special cases first.
    if (other.isEmpty())
        return;
    if (isEmpty()) {
        *this = other;
        return;
    }

    int l = min(x(), other.x());
    int t = min(y(), other.y());
    int r = max(maxX(), other.maxX());
    int b = max(maxY(), other.maxY());

    m_location.setX(l);
    m_location.setY(t);
    m_size.setWidth(r - l);
    m_size.setHeight(b - t);
}

void IntRect::uniteIfNonZero(const IntRect& other)
{
    // Handle empty special cases first.
    if (!other.width() && !other.height())
        return;
    if (!width() && !height()) {
        *this = other;
        return;
    }

    int left = min(x(), other.x());
    int top = min(y(), other.y());
    int right = max(maxX(), other.maxX());
    int bottom = max(maxY(), other.maxY());

    m_location.setX(left);
    m_location.setY(top);
    m_size.setWidth(right - left);
    m_size.setHeight(bottom - top);
}

void IntRect::scale(float s)
{
    m_location.setX((int)(x() * s));
    m_location.setY((int)(y() * s));
    m_size.setWidth((int)(width() * s));
    m_size.setHeight((int)(height() * s));
}

static inline int distanceToInterval(int pos, int start, int end)
{
    if (pos < start)
        return start - pos;
    if (pos > end)
        return end - pos;
    return 0;
}

IntSize IntRect::differenceToPoint(const IntPoint& point) const
{
    int xdistance = distanceToInterval(point.x(), x(), maxX());
    int ydistance = distanceToInterval(point.y(), y(), maxY());
    return IntSize(xdistance, ydistance);
}

IntSize IntRect::differenceFromCenterLineToPoint(const IntPoint& point) const
{
    // The center-line is the natural center of a rectangle. It has an equal distance to all sides of the rectangle.
    IntPoint centerPoint = center();
    int xdistance = centerPoint.x() - point.x();
    int ydistance = centerPoint.y() - point.y();
    if (width() > height())
        xdistance = distanceToInterval(point.x(), x() + (height() / 2), maxX() - (height() / 2));
    else
        ydistance = distanceToInterval(point.y(), y() + (width() / 2), maxY() - (width() / 2));
    return IntSize(xdistance, ydistance);
}

IntRect unionRect(const Vector<IntRect>& rects)
{
    IntRect result;

    size_t count = rects.size();
    for (size_t i = 0; i < count; ++i)
        result.unite(rects[i]);

    return result;
}

} // namespace WebCore
