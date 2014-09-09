/*
    Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
                  2004, 2005, 2007 Rob Buis <buis@kde.org>
    Copyright (C) Research In Motion Limited 2010. All rights reserved.
    Copyright (C) 2014 Adobe Systems Incorporated. All rights reserved.

    Based on khtml code by:
    Copyright (C) 1999 Antti Koivisto (koivisto@kde.org)
    Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
    Copyright (C) 2002-2003 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2002 Apple Inc.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "SVGRenderStyleDefs.h"

#include "RenderStyle.h"
#include "SVGRenderStyle.h"

namespace WebCore {

StyleFillData::StyleFillData()
    : opacity(SVGRenderStyle::initialFillOpacity())
    , paintType(SVGRenderStyle::initialFillPaintType())
    , paintColor(SVGRenderStyle::initialFillPaintColor())
    , paintUri(SVGRenderStyle::initialFillPaintUri())
    , visitedLinkPaintType(SVGRenderStyle::initialStrokePaintType())
    , visitedLinkPaintColor(SVGRenderStyle::initialFillPaintColor())
    , visitedLinkPaintUri(SVGRenderStyle::initialFillPaintUri())
{
}

inline StyleFillData::StyleFillData(const StyleFillData& other)
    : RefCounted<StyleFillData>()
    , opacity(other.opacity)
    , paintType(other.paintType)
    , paintColor(other.paintColor)
    , paintUri(other.paintUri)
    , visitedLinkPaintType(other.visitedLinkPaintType)
    , visitedLinkPaintColor(other.visitedLinkPaintColor)
    , visitedLinkPaintUri(other.visitedLinkPaintUri)
{
}

PassRef<StyleFillData> StyleFillData::copy() const
{
    return adoptRef(*new StyleFillData(*this));
}

bool StyleFillData::operator==(const StyleFillData& other) const
{
    return opacity == other.opacity 
        && paintType == other.paintType
        && paintColor == other.paintColor
        && paintUri == other.paintUri
        && visitedLinkPaintType == other.visitedLinkPaintType
        && visitedLinkPaintColor == other.visitedLinkPaintColor
        && visitedLinkPaintUri == other.visitedLinkPaintUri;
}

StyleStrokeData::StyleStrokeData()
    : opacity(SVGRenderStyle::initialStrokeOpacity())
    , miterLimit(SVGRenderStyle::initialStrokeMiterLimit())
    , width(RenderStyle::initialOneLength())
    , dashOffset(RenderStyle::initialZeroLength())
    , dashArray(SVGRenderStyle::initialStrokeDashArray())
    , paintType(SVGRenderStyle::initialStrokePaintType())
    , paintColor(SVGRenderStyle::initialStrokePaintColor())
    , paintUri(SVGRenderStyle::initialStrokePaintUri())
    , visitedLinkPaintType(SVGRenderStyle::initialStrokePaintType())
    , visitedLinkPaintColor(SVGRenderStyle::initialStrokePaintColor())
    , visitedLinkPaintUri(SVGRenderStyle::initialStrokePaintUri())
{
}

inline StyleStrokeData::StyleStrokeData(const StyleStrokeData& other)
    : RefCounted<StyleStrokeData>()
    , opacity(other.opacity)
    , miterLimit(other.miterLimit)
    , width(other.width)
    , dashOffset(other.dashOffset)
    , dashArray(other.dashArray)
    , paintType(other.paintType)
    , paintColor(other.paintColor)
    , paintUri(other.paintUri)
    , visitedLinkPaintType(other.visitedLinkPaintType)
    , visitedLinkPaintColor(other.visitedLinkPaintColor)
    , visitedLinkPaintUri(other.visitedLinkPaintUri)
{
}

PassRef<StyleStrokeData> StyleStrokeData::copy() const
{
    return adoptRef(*new StyleStrokeData(*this));
}

bool StyleStrokeData::operator==(const StyleStrokeData& other) const
{
    return width == other.width
        && opacity == other.opacity
        && miterLimit == other.miterLimit
        && dashOffset == other.dashOffset
        && dashArray == other.dashArray
        && paintType == other.paintType
        && paintColor == other.paintColor
        && paintUri == other.paintUri
        && visitedLinkPaintType == other.visitedLinkPaintType
        && visitedLinkPaintColor == other.visitedLinkPaintColor
        && visitedLinkPaintUri == other.visitedLinkPaintUri;
}

StyleStopData::StyleStopData()
    : opacity(SVGRenderStyle::initialStopOpacity())
    , color(SVGRenderStyle::initialStopColor())
{
}

inline StyleStopData::StyleStopData(const StyleStopData& other)
    : RefCounted<StyleStopData>()
    , opacity(other.opacity)
    , color(other.color)
{
}

PassRef<StyleStopData> StyleStopData::copy() const
{
    return adoptRef(*new StyleStopData(*this));
}

bool StyleStopData::operator==(const StyleStopData& other) const
{
    return color == other.color
        && opacity == other.opacity;
}

StyleTextData::StyleTextData()
    : kerning(SVGRenderStyle::initialKerning())
{
}

inline StyleTextData::StyleTextData(const StyleTextData& other)
    : RefCounted<StyleTextData>()
    , kerning(other.kerning)
{
}

PassRef<StyleTextData> StyleTextData::copy() const
{
    return adoptRef(*new StyleTextData(*this));
}

bool StyleTextData::operator==(const StyleTextData& other) const
{
    return kerning == other.kerning;
}

StyleMiscData::StyleMiscData()
    : floodColor(SVGRenderStyle::initialFloodColor())
    , floodOpacity(SVGRenderStyle::initialFloodOpacity())
    , lightingColor(SVGRenderStyle::initialLightingColor())
    , baselineShiftValue(SVGRenderStyle::initialBaselineShiftValue())
{
}

inline StyleMiscData::StyleMiscData(const StyleMiscData& other)
    : RefCounted<StyleMiscData>()
    , floodColor(other.floodColor)
    , floodOpacity(other.floodOpacity)
    , lightingColor(other.lightingColor)
    , baselineShiftValue(other.baselineShiftValue)
{
}

PassRef<StyleMiscData> StyleMiscData::copy() const
{
    return adoptRef(*new StyleMiscData(*this));
}

bool StyleMiscData::operator==(const StyleMiscData& other) const
{
    return floodOpacity == other.floodOpacity
        && floodColor == other.floodColor
        && lightingColor == other.lightingColor
        && baselineShiftValue == other.baselineShiftValue;
}

StyleShadowSVGData::StyleShadowSVGData()
{
}

inline StyleShadowSVGData::StyleShadowSVGData(const StyleShadowSVGData& other)
    : RefCounted<StyleShadowSVGData>()
    , shadow(other.shadow ? std::make_unique<ShadowData>(*other.shadow) : nullptr)
{
}

PassRef<StyleShadowSVGData> StyleShadowSVGData::copy() const
{
    return adoptRef(*new StyleShadowSVGData(*this));
}

bool StyleShadowSVGData::operator==(const StyleShadowSVGData& other) const
{
    if ((!shadow && other.shadow) || (shadow && !other.shadow))
        return false;
    if (shadow && other.shadow && (*shadow != *other.shadow))
        return false;
    return true;
}

StyleResourceData::StyleResourceData()
    : clipper(SVGRenderStyle::initialClipperResource())
    , filter(SVGRenderStyle::initialFilterResource())
    , masker(SVGRenderStyle::initialMaskerResource())
{
}

inline StyleResourceData::StyleResourceData(const StyleResourceData& other)
    : RefCounted<StyleResourceData>()
    , clipper(other.clipper)
    , filter(other.filter)
    , masker(other.masker)
{
}

PassRef<StyleResourceData> StyleResourceData::copy() const
{
    return adoptRef(*new StyleResourceData(*this));
}

bool StyleResourceData::operator==(const StyleResourceData& other) const
{
    return clipper == other.clipper
        && filter == other.filter
        && masker == other.masker;
}

StyleInheritedResourceData::StyleInheritedResourceData()
    : markerStart(SVGRenderStyle::initialMarkerStartResource())
    , markerMid(SVGRenderStyle::initialMarkerMidResource())
    , markerEnd(SVGRenderStyle::initialMarkerEndResource())
{
}

inline StyleInheritedResourceData::StyleInheritedResourceData(const StyleInheritedResourceData& other)
    : RefCounted<StyleInheritedResourceData>()
    , markerStart(other.markerStart)
    , markerMid(other.markerMid)
    , markerEnd(other.markerEnd)
{
}

PassRef<StyleInheritedResourceData> StyleInheritedResourceData::copy() const
{
    return adoptRef(*new StyleInheritedResourceData(*this));
}

bool StyleInheritedResourceData::operator==(const StyleInheritedResourceData& other) const
{
    return markerStart == other.markerStart
        && markerMid == other.markerMid
        && markerEnd == other.markerEnd;
}

StyleLayoutData::StyleLayoutData()
    : cx(RenderStyle::initialZeroLength())
    , cy(RenderStyle::initialZeroLength())
    , r(RenderStyle::initialZeroLength())
    , rx(RenderStyle::initialZeroLength())
    , ry(RenderStyle::initialZeroLength())
    , x(RenderStyle::initialZeroLength())
    , y(RenderStyle::initialZeroLength())
{
}

inline StyleLayoutData::StyleLayoutData(const StyleLayoutData& other)
    : RefCounted<StyleLayoutData>()
    , cx(other.cx)
    , cy(other.cy)
    , r(other.r)
    , rx(other.rx)
    , ry(other.ry)
    , x(other.x)
    , y(other.y)
{
}

PassRef<StyleLayoutData> StyleLayoutData::copy() const
{
    return adoptRef(*new StyleLayoutData(*this));
}

bool StyleLayoutData::operator==(const StyleLayoutData& other) const
{
    return cx == other.cx
        && cy == other.cy
        && r == other.r
        && rx == other.rx
        && ry == other.ry
        && x == other.x
        && y == other.y;
}

}
