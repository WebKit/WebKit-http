/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef LayoutTypesInlineMethods_h
#define LayoutTypesInlineMethods_h

#include "FloatRect.h"
#include "FractionalLayoutBoxExtent.h"
#include "FractionalLayoutRect.h"
#include "FractionalLayoutUnit.h"
#include "IntRect.h"
#include "LayoutTypes.h"

#include <wtf/MathExtras.h>

namespace WebCore {

inline FractionalLayoutRect enclosingLayoutRect(const FloatRect& rect)
{
    return enclosingIntRect(rect);
}

inline LayoutSize roundedLayoutSize(const FloatSize& s)
{
#if ENABLE(SUBPIXEL_LAYOUT)
    return FractionalLayoutSize(s);
#else
    return roundedIntSize(s);
#endif
}

inline IntRect pixelSnappedIntRect(LayoutUnit left, LayoutUnit top, LayoutUnit width, LayoutUnit height)
{
    return IntRect(left.round(), top.round(), snapSizeToPixel(width, left), snapSizeToPixel(height, top));
}

inline IntRect pixelSnappedIntRectFromEdges(LayoutUnit left, LayoutUnit top, LayoutUnit right, LayoutUnit bottom)
{
    return IntRect(left.round(), top.round(), snapSizeToPixel(right - left, left), snapSizeToPixel(bottom - top, top));
}

inline LayoutPoint roundedLayoutPoint(const FloatPoint& p)
{
#if ENABLE(SUBPIXEL_LAYOUT)
    return FractionalLayoutPoint(p);
#else
    return roundedIntPoint(p);
#endif
}

inline LayoutPoint flooredLayoutPoint(const FloatPoint& p)
{
    return flooredFractionalLayoutPoint(p);
}

inline LayoutPoint flooredLayoutPoint(const FloatSize& s)
{
    return flooredLayoutPoint(FloatPoint(s));
}

inline int roundToInt(LayoutUnit value)
{
    return value.round();
}

inline int floorToInt(LayoutUnit value)
{
    return value.floor();
}

inline LayoutUnit roundedLayoutUnit(float value)
{
#if ENABLE(SUBPIXEL_LAYOUT)
    return FractionalLayoutUnit::fromFloatRound(value);
#else
    return static_cast<int>(lroundf(value));
#endif
}

inline LayoutUnit ceiledLayoutUnit(float value)
{
#if ENABLE(SUBPIXEL_LAYOUT)
    return FractionalLayoutUnit::fromFloatCeil(value);
#else
    return ceilf(value);
#endif
}

inline LayoutUnit absoluteValue(const LayoutUnit& value)
{
    return value.abs();
}

inline LayoutSize toLayoutSize(const LayoutPoint& p)
{
    return LayoutSize(p.x(), p.y());
}

inline LayoutPoint toLayoutPoint(const LayoutSize& p)
{
    return LayoutPoint(p.width(), p.height());
}

inline LayoutUnit layoutMod(const LayoutUnit& numerator, const LayoutUnit& denominator)
{
    return numerator % denominator;
}

inline IntSize pixelSnappedIntSize(const FractionalLayoutSize& s, const FractionalLayoutPoint& p)
{
    return IntSize(snapSizeToPixel(s.width(), p.x()), snapSizeToPixel(s.height(), p.y()));
}

inline IntRect pixelSnappedIntRect(LayoutPoint location, LayoutSize size)
{
    return IntRect(roundedIntPoint(location), pixelSnappedIntSize(size, location));
}

inline bool isIntegerValue(const LayoutUnit value)
{
    return value.toInt() == value;
}

} // namespace WebCore

#endif // LayoutTypesInlineMethods_h
