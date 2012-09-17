/*
 * Copyright (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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
#include "StyleRareNonInheritedData.h"

#include "ContentData.h"
#include "RenderCounter.h"
#include "RenderStyle.h"
#include "ShadowData.h"
#include "StyleFilterData.h"
#include "StyleTransformData.h"
#include "StyleImage.h"
#include "StyleResolver.h"
#include "WebCoreMemoryInstrumentation.h"

namespace WebCore {

StyleRareNonInheritedData::StyleRareNonInheritedData()
    : opacity(RenderStyle::initialOpacity())
    , m_aspectRatioDenominator(RenderStyle::initialAspectRatioDenominator())
    , m_aspectRatioNumerator(RenderStyle::initialAspectRatioNumerator())
    , m_perspective(RenderStyle::initialPerspective())
    , m_perspectiveOriginX(RenderStyle::initialPerspectiveOriginX())
    , m_perspectiveOriginY(RenderStyle::initialPerspectiveOriginY())
    , lineClamp(RenderStyle::initialLineClamp())
    , m_mask(FillLayer(MaskFillLayer))
    , m_pageSize()
    , m_wrapShapeInside(RenderStyle::initialWrapShapeInside())
    , m_wrapShapeOutside(RenderStyle::initialWrapShapeOutside())
    , m_wrapMargin(RenderStyle::initialWrapMargin())
    , m_wrapPadding(RenderStyle::initialWrapPadding())
    , m_clipPath(RenderStyle::initialClipPath())
    , m_visitedLinkBackgroundColor(RenderStyle::initialBackgroundColor())
    , m_order(RenderStyle::initialOrder())
    , m_flowThread(RenderStyle::initialFlowThread())
    , m_regionThread(RenderStyle::initialRegionThread())
    , m_regionOverflow(RenderStyle::initialRegionOverflow())
    , m_regionBreakAfter(RenderStyle::initialPageBreak())
    , m_regionBreakBefore(RenderStyle::initialPageBreak())
    , m_regionBreakInside(RenderStyle::initialPageBreak())
    , m_pageSizeType(PAGE_SIZE_AUTO)
    , m_transformStyle3D(RenderStyle::initialTransformStyle3D())
    , m_backfaceVisibility(RenderStyle::initialBackfaceVisibility())
    , m_alignContent(RenderStyle::initialAlignContent())
    , m_alignItems(RenderStyle::initialAlignItems())
    , m_alignSelf(RenderStyle::initialAlignSelf())
    , m_justifyContent(RenderStyle::initialJustifyContent())
    , userDrag(RenderStyle::initialUserDrag())
    , textOverflow(RenderStyle::initialTextOverflow())
    , marginBeforeCollapse(MCOLLAPSE)
    , marginAfterCollapse(MCOLLAPSE)
    , m_appearance(RenderStyle::initialAppearance())
    , m_borderFit(RenderStyle::initialBorderFit())
    , m_textCombine(RenderStyle::initialTextCombine())
#if ENABLE(CSS3_TEXT_DECORATION)
    , m_textDecorationStyle(RenderStyle::initialTextDecorationStyle())
#endif // CSS3_TEXT_DECORATION
    , m_wrapFlow(RenderStyle::initialWrapFlow())
    , m_wrapThrough(RenderStyle::initialWrapThrough())
#if USE(ACCELERATED_COMPOSITING)
    , m_runningAcceleratedAnimation(false)
#endif
    , m_hasAspectRatio(false)
#if ENABLE(CSS_COMPOSITING)
    , m_effectiveBlendMode(RenderStyle::initialBlendMode())
#endif
{
    m_maskBoxImage.setMaskDefaults();
}

StyleRareNonInheritedData::StyleRareNonInheritedData(const StyleRareNonInheritedData& o)
    : RefCounted<StyleRareNonInheritedData>()
    , opacity(o.opacity)
    , m_aspectRatioDenominator(o.m_aspectRatioDenominator)
    , m_aspectRatioNumerator(o.m_aspectRatioNumerator)
    , m_perspective(o.m_perspective)
    , m_perspectiveOriginX(o.m_perspectiveOriginX)
    , m_perspectiveOriginY(o.m_perspectiveOriginY)
    , lineClamp(o.lineClamp)
    , m_deprecatedFlexibleBox(o.m_deprecatedFlexibleBox)
    , m_flexibleBox(o.m_flexibleBox)
    , m_marquee(o.m_marquee)
    , m_multiCol(o.m_multiCol)
    , m_transform(o.m_transform)
#if ENABLE(CSS_FILTERS)
    , m_filter(o.m_filter)
#endif
    , m_grid(o.m_grid)
    , m_gridItem(o.m_gridItem)
    , m_content(o.m_content ? o.m_content->clone() : nullptr)
    , m_counterDirectives(o.m_counterDirectives ? clone(*o.m_counterDirectives) : nullptr)
    , m_boxShadow(o.m_boxShadow ? adoptPtr(new ShadowData(*o.m_boxShadow)) : nullptr)
    , m_boxReflect(o.m_boxReflect)
    , m_animations(o.m_animations ? adoptPtr(new AnimationList(*o.m_animations)) : nullptr)
    , m_transitions(o.m_transitions ? adoptPtr(new AnimationList(*o.m_transitions)) : nullptr)
    , m_mask(o.m_mask)
    , m_maskBoxImage(o.m_maskBoxImage)
    , m_pageSize(o.m_pageSize)
    , m_wrapShapeInside(o.m_wrapShapeInside)
    , m_wrapShapeOutside(o.m_wrapShapeOutside)
    , m_wrapMargin(o.m_wrapMargin)
    , m_wrapPadding(o.m_wrapPadding)
    , m_clipPath(o.m_clipPath)
    , m_visitedLinkBackgroundColor(o.m_visitedLinkBackgroundColor)
    , m_visitedLinkOutlineColor(o.m_visitedLinkOutlineColor)
    , m_visitedLinkBorderLeftColor(o.m_visitedLinkBorderLeftColor)
    , m_visitedLinkBorderRightColor(o.m_visitedLinkBorderRightColor)
    , m_visitedLinkBorderTopColor(o.m_visitedLinkBorderTopColor)
    , m_visitedLinkBorderBottomColor(o.m_visitedLinkBorderBottomColor)
    , m_order(o.m_order)
    , m_flowThread(o.m_flowThread)
    , m_regionThread(o.m_regionThread)
    , m_regionOverflow(o.m_regionOverflow)
    , m_regionBreakAfter(o.m_regionBreakAfter)
    , m_regionBreakBefore(o.m_regionBreakBefore)
    , m_regionBreakInside(o.m_regionBreakInside)
    , m_pageSizeType(o.m_pageSizeType)
    , m_transformStyle3D(o.m_transformStyle3D)
    , m_backfaceVisibility(o.m_backfaceVisibility)
    , m_alignContent(o.m_alignContent)
    , m_alignItems(o.m_alignItems)
    , m_alignSelf(o.m_alignSelf)
    , m_justifyContent(o.m_justifyContent)
    , userDrag(o.userDrag)
    , textOverflow(o.textOverflow)
    , marginBeforeCollapse(o.marginBeforeCollapse)
    , marginAfterCollapse(o.marginAfterCollapse)
    , m_appearance(o.m_appearance)
    , m_borderFit(o.m_borderFit)
    , m_textCombine(o.m_textCombine)
#if ENABLE(CSS3_TEXT_DECORATION)
    , m_textDecorationStyle(o.m_textDecorationStyle)
#endif // CSS3_TEXT_DECORATION
    , m_wrapFlow(o.m_wrapFlow)
    , m_wrapThrough(o.m_wrapThrough)
#if USE(ACCELERATED_COMPOSITING)
    , m_runningAcceleratedAnimation(o.m_runningAcceleratedAnimation)
#endif
    , m_hasAspectRatio(o.m_hasAspectRatio)
#if ENABLE(CSS_COMPOSITING)
    , m_effectiveBlendMode(RenderStyle::initialBlendMode())
#endif
{
}

StyleRareNonInheritedData::~StyleRareNonInheritedData()
{
}

bool StyleRareNonInheritedData::operator==(const StyleRareNonInheritedData& o) const
{
    return opacity == o.opacity
        && m_aspectRatioDenominator == o.m_aspectRatioDenominator
        && m_aspectRatioNumerator == o.m_aspectRatioNumerator
        && m_perspective == o.m_perspective
        && m_perspectiveOriginX == o.m_perspectiveOriginX
        && m_perspectiveOriginY == o.m_perspectiveOriginY
        && lineClamp == o.lineClamp
#if ENABLE(DASHBOARD_SUPPORT) || ENABLE(WIDGET_REGION)
        && m_dashboardRegions == o.m_dashboardRegions
#endif
        && m_deprecatedFlexibleBox == o.m_deprecatedFlexibleBox
        && m_flexibleBox == o.m_flexibleBox
        && m_marquee == o.m_marquee
        && m_multiCol == o.m_multiCol
        && m_transform == o.m_transform
#if ENABLE(CSS_FILTERS)
        && m_filter == o.m_filter
#endif
        && m_grid == o.m_grid
        && m_gridItem == o.m_gridItem
        && contentDataEquivalent(o)
        && counterDataEquivalent(o)
        && shadowDataEquivalent(o)
        && reflectionDataEquivalent(o)
        && animationDataEquivalent(o)
        && transitionDataEquivalent(o)
        && m_mask == o.m_mask
        && m_maskBoxImage == o.m_maskBoxImage
        && m_pageSize == o.m_pageSize
        && m_wrapShapeInside == o.m_wrapShapeInside
        && m_wrapShapeOutside == o.m_wrapShapeOutside
        && m_wrapMargin == o.m_wrapMargin
        && m_wrapPadding == o.m_wrapPadding
        && m_clipPath == o.m_clipPath
        && m_visitedLinkBackgroundColor == o.m_visitedLinkBackgroundColor
        && m_visitedLinkOutlineColor == o.m_visitedLinkOutlineColor
        && m_visitedLinkBorderLeftColor == o.m_visitedLinkBorderLeftColor
        && m_visitedLinkBorderRightColor == o.m_visitedLinkBorderRightColor
        && m_visitedLinkBorderTopColor == o.m_visitedLinkBorderTopColor
        && m_visitedLinkBorderBottomColor == o.m_visitedLinkBorderBottomColor
        && m_order == o.m_order
        && m_flowThread == o.m_flowThread
        && m_regionThread == o.m_regionThread
        && m_regionOverflow == o.m_regionOverflow
        && m_regionBreakAfter == o.m_regionBreakAfter
        && m_regionBreakBefore == o.m_regionBreakBefore
        && m_regionBreakInside == o.m_regionBreakInside
        && m_pageSizeType == o.m_pageSizeType
        && m_transformStyle3D == o.m_transformStyle3D
        && m_backfaceVisibility == o.m_backfaceVisibility
        && m_alignContent == o.m_alignContent
        && m_alignItems == o.m_alignItems
        && m_alignSelf == o.m_alignSelf
        && m_justifyContent == o.m_justifyContent
        && userDrag == o.userDrag
        && textOverflow == o.textOverflow
        && marginBeforeCollapse == o.marginBeforeCollapse
        && marginAfterCollapse == o.marginAfterCollapse
        && m_appearance == o.m_appearance
        && m_borderFit == o.m_borderFit
        && m_textCombine == o.m_textCombine
#if ENABLE(CSS3_TEXT_DECORATION)
        && m_textDecorationStyle == o.m_textDecorationStyle
#endif // CSS3_TEXT_DECORATION
        && m_wrapFlow == o.m_wrapFlow
        && m_wrapThrough == o.m_wrapThrough
#if USE(ACCELERATED_COMPOSITING)
        && !m_runningAcceleratedAnimation && !o.m_runningAcceleratedAnimation
#endif
#if ENABLE(CSS_COMPOSITING)
        && m_effectiveBlendMode == o.m_effectiveBlendMode
#endif
        && m_hasAspectRatio == o.m_hasAspectRatio;
}

bool StyleRareNonInheritedData::contentDataEquivalent(const StyleRareNonInheritedData& o) const
{
    if (m_content.get() == o.m_content.get())
        return true;
        
    if (m_content && o.m_content && *m_content == *o.m_content)
        return true;

    return false;
}

bool StyleRareNonInheritedData::counterDataEquivalent(const StyleRareNonInheritedData& o) const
{
    if (m_counterDirectives.get() == o.m_counterDirectives.get())
        return true;
        
    if (m_counterDirectives && o.m_counterDirectives && *m_counterDirectives == *o.m_counterDirectives)
        return true;

    return false;
}

bool StyleRareNonInheritedData::shadowDataEquivalent(const StyleRareNonInheritedData& o) const
{
    if ((!m_boxShadow && o.m_boxShadow) || (m_boxShadow && !o.m_boxShadow))
        return false;
    if (m_boxShadow && o.m_boxShadow && (*m_boxShadow != *o.m_boxShadow))
        return false;
    return true;
}

bool StyleRareNonInheritedData::reflectionDataEquivalent(const StyleRareNonInheritedData& o) const
{
    if (m_boxReflect != o.m_boxReflect) {
        if (!m_boxReflect || !o.m_boxReflect)
            return false;
        return *m_boxReflect == *o.m_boxReflect;
    }
    return true;
}

bool StyleRareNonInheritedData::animationDataEquivalent(const StyleRareNonInheritedData& o) const
{
    if ((!m_animations && o.m_animations) || (m_animations && !o.m_animations))
        return false;
    if (m_animations && o.m_animations && (*m_animations != *o.m_animations))
        return false;
    return true;
}

bool StyleRareNonInheritedData::transitionDataEquivalent(const StyleRareNonInheritedData& o) const
{
    if ((!m_transitions && o.m_transitions) || (m_transitions && !o.m_transitions))
        return false;
    if (m_transitions && o.m_transitions && (*m_transitions != *o.m_transitions))
        return false;
    return true;
}

void StyleRareNonInheritedData::reportMemoryUsage(MemoryObjectInfo* memoryObjectInfo) const
{
    MemoryClassInfo info(memoryObjectInfo, this, WebCoreMemoryTypes::CSS);
#if ENABLE(DASHBOARD_SUPPORT)
    info.addVector(m_dashboardRegions);
#endif
    info.addMember(m_deprecatedFlexibleBox);
    info.addMember(m_flexibleBox);
    info.addMember(m_marquee);
    info.addMember(m_multiCol);
    info.addMember(m_transform);
#if ENABLE(CSS_FILTERS)
    info.addMember(m_filter);
#endif
    info.addMember(m_grid);
    info.addMember(m_gridItem);
    info.addMember(m_content);
    info.addMember(m_counterDirectives);
    info.addMember(m_boxShadow);
    info.addMember(m_boxReflect);
    info.addMember(m_animations);
    info.addMember(m_transitions);
    info.addMember(m_wrapShapeInside);
    info.addMember(m_wrapShapeOutside);
    info.addMember(m_clipPath);
    info.addMember(m_flowThread);
    info.addMember(m_regionThread);
}

} // namespace WebCore
