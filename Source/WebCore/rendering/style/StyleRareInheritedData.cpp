/*
 * Copyright (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "StyleRareInheritedData.h"

#include "CursorList.h"
#include "QuotesData.h"
#include "RenderStyle.h"
#include "RenderStyleConstants.h"
#include "ShadowData.h"
#include "WebCoreMemoryInstrumentation.h"

namespace WebCore {

struct SameSizeAsStyleRareInheritedData : public RefCounted<SameSizeAsStyleRareInheritedData> {
    Color firstColor;
    float firstFloat;
    Color colors[5];
    void* ownPtrs[1];
    AtomicString atomicStrings[5];
    void* refPtrs[2];
    Length lengths[1];
    float secondFloat;
    unsigned m_bitfields[2];
    short pagedMediaShorts[2];
    unsigned unsigneds[1];
    short hyphenationShorts[3];

#if ENABLE(CSS_IMAGE_RESOLUTION)
    float imageResolutionFloats;
#endif

#if ENABLE(TOUCH_EVENTS)
    Color touchColors;
#endif

#if ENABLE(CSS_VARIABLES)
    void* variableDataRefs[1];
#endif
};

COMPILE_ASSERT(sizeof(StyleRareInheritedData) == sizeof(SameSizeAsStyleRareInheritedData), StyleRareInheritedData_should_bit_pack);

StyleRareInheritedData::StyleRareInheritedData()
    : textStrokeWidth(RenderStyle::initialTextStrokeWidth())
    , indent(RenderStyle::initialTextIndent())
    , m_effectiveZoom(RenderStyle::initialZoom())
    , widows(RenderStyle::initialWidows())
    , orphans(RenderStyle::initialOrphans())
    , textSecurity(RenderStyle::initialTextSecurity())
    , userModify(READ_ONLY)
    , wordBreak(RenderStyle::initialWordBreak())
    , overflowWrap(RenderStyle::initialOverflowWrap())
    , nbspMode(NBNORMAL)
    , khtmlLineBreak(LBNORMAL)
    , textSizeAdjust(RenderStyle::initialTextSizeAdjust())
    , resize(RenderStyle::initialResize())
    , userSelect(RenderStyle::initialUserSelect())
    , colorSpace(ColorSpaceDeviceRGB)
    , speak(SpeakNormal)
    , hyphens(HyphensManual)
    , textEmphasisFill(TextEmphasisFillFilled)
    , textEmphasisMark(TextEmphasisMarkNone)
    , textEmphasisPosition(TextEmphasisPositionOver)
    , m_lineBoxContain(RenderStyle::initialLineBoxContain())
#if ENABLE(CSS_IMAGE_ORIENTATION)
    , m_imageOrientation(RenderStyle::initialImageOrientation())
#endif
    , m_imageRendering(RenderStyle::initialImageRendering())
    , m_lineSnap(RenderStyle::initialLineSnap())
    , m_lineAlign(RenderStyle::initialLineAlign())
#if ENABLE(ACCELERATED_OVERFLOW_SCROLLING)
    , useTouchOverflowScrolling(RenderStyle::initialUseTouchOverflowScrolling())
#endif
#if ENABLE(CSS_IMAGE_RESOLUTION)
    , m_imageResolutionSource(RenderStyle::initialImageResolutionSource())
    , m_imageResolutionSnap(RenderStyle::initialImageResolutionSnap())
#endif
    , hyphenationLimitBefore(-1)
    , hyphenationLimitAfter(-1)
    , hyphenationLimitLines(-1)
    , m_lineGrid(RenderStyle::initialLineGrid())
    , m_tabSize(RenderStyle::initialTabSize())
#if ENABLE(CSS_IMAGE_RESOLUTION)
    , m_imageResolution(RenderStyle::initialImageResolution())
#endif
#if ENABLE(TOUCH_EVENTS)
    , tapHighlightColor(RenderStyle::initialTapHighlightColor())
#endif    
{
#if ENABLE(CSS_VARIABLES)
    m_variables.init();
#endif
}

StyleRareInheritedData::StyleRareInheritedData(const StyleRareInheritedData& o)
    : RefCounted<StyleRareInheritedData>()
    , textStrokeColor(o.textStrokeColor)
    , textStrokeWidth(o.textStrokeWidth)
    , textFillColor(o.textFillColor)
    , textEmphasisColor(o.textEmphasisColor)
    , visitedLinkTextStrokeColor(o.visitedLinkTextStrokeColor)
    , visitedLinkTextFillColor(o.visitedLinkTextFillColor)
    , visitedLinkTextEmphasisColor(o.visitedLinkTextEmphasisColor)
    , textShadow(o.textShadow ? adoptPtr(new ShadowData(*o.textShadow)) : nullptr)
    , highlight(o.highlight)
    , cursorData(o.cursorData)
    , indent(o.indent)
    , m_effectiveZoom(o.m_effectiveZoom)
    , widows(o.widows)
    , orphans(o.orphans)
    , textSecurity(o.textSecurity)
    , userModify(o.userModify)
    , wordBreak(o.wordBreak)
    , overflowWrap(o.overflowWrap)
    , nbspMode(o.nbspMode)
    , khtmlLineBreak(o.khtmlLineBreak)
    , textSizeAdjust(o.textSizeAdjust)
    , resize(o.resize)
    , userSelect(o.userSelect)
    , colorSpace(o.colorSpace)
    , speak(o.speak)
    , hyphens(o.hyphens)
    , textEmphasisFill(o.textEmphasisFill)
    , textEmphasisMark(o.textEmphasisMark)
    , textEmphasisPosition(o.textEmphasisPosition)
    , m_lineBoxContain(o.m_lineBoxContain)
#if ENABLE(CSS_IMAGE_ORIENTATION)
    , m_imageOrientation(o.m_imageOrientation)
#endif
    , m_imageRendering(o.m_imageRendering)
    , m_lineSnap(o.m_lineSnap)
    , m_lineAlign(o.m_lineAlign)
#if ENABLE(ACCELERATED_OVERFLOW_SCROLLING)
    , useTouchOverflowScrolling(o.useTouchOverflowScrolling)
#endif
#if ENABLE(CSS_IMAGE_RESOLUTION)
    , m_imageResolutionSource(o.m_imageResolutionSource)
    , m_imageResolutionSnap(o.m_imageResolutionSnap)
#endif
    , hyphenationString(o.hyphenationString)
    , hyphenationLimitBefore(o.hyphenationLimitBefore)
    , hyphenationLimitAfter(o.hyphenationLimitAfter)
    , hyphenationLimitLines(o.hyphenationLimitLines)
    , locale(o.locale)
    , textEmphasisCustomMark(o.textEmphasisCustomMark)
    , m_lineGrid(o.m_lineGrid)
    , m_tabSize(o.m_tabSize)
#if ENABLE(CSS_IMAGE_RESOLUTION)
    , m_imageResolution(o.m_imageResolution)
#endif
#if ENABLE(TOUCH_EVENTS)
    , tapHighlightColor(o.tapHighlightColor)
#endif
#if ENABLE(CSS_VARIABLES)
    , m_variables(o.m_variables)
#endif
{
}

StyleRareInheritedData::~StyleRareInheritedData()
{
}

static bool cursorDataEquivalent(const CursorList* c1, const CursorList* c2)
{
    if (c1 == c2)
        return true;
    if ((!c1 && c2) || (c1 && !c2))
        return false;
    return (*c1 == *c2);
}

bool StyleRareInheritedData::operator==(const StyleRareInheritedData& o) const
{
    return textStrokeColor == o.textStrokeColor
        && textStrokeWidth == o.textStrokeWidth
        && textFillColor == o.textFillColor
        && textEmphasisColor == o.textEmphasisColor
        && visitedLinkTextStrokeColor == o.visitedLinkTextStrokeColor
        && visitedLinkTextFillColor == o.visitedLinkTextFillColor
        && visitedLinkTextEmphasisColor == o.visitedLinkTextEmphasisColor
#if ENABLE(TOUCH_EVENTS)
        && tapHighlightColor == o.tapHighlightColor
#endif
        && shadowDataEquivalent(o)
        && highlight == o.highlight
        && cursorDataEquivalent(cursorData.get(), o.cursorData.get())
        && indent == o.indent
        && m_effectiveZoom == o.m_effectiveZoom
        && widows == o.widows
        && orphans == o.orphans
        && textSecurity == o.textSecurity
        && userModify == o.userModify
        && wordBreak == o.wordBreak
        && overflowWrap == o.overflowWrap
        && nbspMode == o.nbspMode
        && khtmlLineBreak == o.khtmlLineBreak
#if ENABLE(ACCELERATED_OVERFLOW_SCROLLING)
        && useTouchOverflowScrolling == o.useTouchOverflowScrolling
#endif
        && textSizeAdjust == o.textSizeAdjust
        && resize == o.resize
        && userSelect == o.userSelect
        && colorSpace == o.colorSpace
        && speak == o.speak
        && hyphens == o.hyphens
        && hyphenationLimitBefore == o.hyphenationLimitBefore
        && hyphenationLimitAfter == o.hyphenationLimitAfter
        && hyphenationLimitLines == o.hyphenationLimitLines
        && textEmphasisFill == o.textEmphasisFill
        && textEmphasisMark == o.textEmphasisMark
        && textEmphasisPosition == o.textEmphasisPosition
        && m_lineBoxContain == o.m_lineBoxContain
        && hyphenationString == o.hyphenationString
        && locale == o.locale
        && textEmphasisCustomMark == o.textEmphasisCustomMark
        && QuotesData::equals(quotes.get(), o.quotes.get())
        && m_tabSize == o.m_tabSize
        && m_lineGrid == o.m_lineGrid
#if ENABLE(CSS_IMAGE_ORIENTATION)
        && m_imageOrientation == o.m_imageOrientation
#endif
        && m_imageRendering == o.m_imageRendering
#if ENABLE(CSS_IMAGE_RESOLUTION)
        && m_imageResolutionSource == o.m_imageResolutionSource
        && m_imageResolutionSnap == o.m_imageResolutionSnap
        && m_imageResolution == o.m_imageResolution
#endif
        && m_lineSnap == o.m_lineSnap
#if ENABLE(CSS_VARIABLES)
        && m_variables == o.m_variables
#endif
        && m_lineAlign == o.m_lineAlign;
}

bool StyleRareInheritedData::shadowDataEquivalent(const StyleRareInheritedData& o) const
{
    if ((!textShadow && o.textShadow) || (textShadow && !o.textShadow))
        return false;
    if (textShadow && o.textShadow && (*textShadow != *o.textShadow))
        return false;
    return true;
}

void StyleRareInheritedData::reportMemoryUsage(MemoryObjectInfo* memoryObjectInfo) const
{
    MemoryClassInfo info(memoryObjectInfo, this, WebCoreMemoryTypes::CSS);
    info.addMember(textShadow);
    info.addMember(highlight);
    info.addMember(cursorData);
    info.addMember(hyphenationString);
    info.addMember(locale);
    info.addMember(textEmphasisCustomMark);
    info.addMember(quotes);
    info.addMember(m_lineGrid);
#if ENABLE(CSS_VARIABLES)
    info.addMember(m_variables);
#endif
}

} // namespace WebCore
