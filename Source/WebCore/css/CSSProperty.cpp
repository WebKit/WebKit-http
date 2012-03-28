/**
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.
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
 */

#include "config.h"
#include "CSSProperty.h"

#include "CSSPropertyLonghand.h"
#include "CSSPropertyNames.h"
#include "PlatformString.h"
#include "RenderStyleConstants.h"

namespace WebCore {

String CSSProperty::cssText() const
{
    return String(getPropertyName(static_cast<CSSPropertyID>(id()))) + ": " + m_value->cssText() + (isImportant() ? " !important" : "") + "; ";
}

enum LogicalBoxSide { BeforeSide, EndSide, AfterSide, StartSide };
enum PhysicalBoxSide { TopSide, RightSide, BottomSide, LeftSide };

static int resolveToPhysicalProperty(TextDirection direction, WritingMode writingMode, LogicalBoxSide logicalSide, const CSSPropertyLonghand& longhand)
{
    if (direction == LTR) {
        if (writingMode == TopToBottomWritingMode) {
            // The common case. The logical and physical box sides match.
            // Left = Start, Right = End, Before = Top, After = Bottom
            return longhand.properties()[logicalSide];
        }

        if (writingMode == BottomToTopWritingMode) {
            // Start = Left, End = Right, Before = Bottom, After = Top.
            switch (logicalSide) {
            case StartSide:
                return longhand.properties()[LeftSide];
            case EndSide:
                return longhand.properties()[RightSide];
            case BeforeSide:
                return longhand.properties()[BottomSide];
            default:
                return longhand.properties()[TopSide];
            }
        }

        if (writingMode == LeftToRightWritingMode) {
            // Start = Top, End = Bottom, Before = Left, After = Right.
            switch (logicalSide) {
            case StartSide:
                return longhand.properties()[TopSide];
            case EndSide:
                return longhand.properties()[BottomSide];
            case BeforeSide:
                return longhand.properties()[LeftSide];
            default:
                return longhand.properties()[RightSide];
            }
        }

        // Start = Top, End = Bottom, Before = Right, After = Left
        switch (logicalSide) {
        case StartSide:
            return longhand.properties()[TopSide];
        case EndSide:
            return longhand.properties()[BottomSide];
        case BeforeSide:
            return longhand.properties()[RightSide];
        default:
            return longhand.properties()[LeftSide];
        }
    }

    if (writingMode == TopToBottomWritingMode) {
        // Start = Right, End = Left, Before = Top, After = Bottom
        switch (logicalSide) {
        case StartSide:
            return longhand.properties()[RightSide];
        case EndSide:
            return longhand.properties()[LeftSide];
        case BeforeSide:
            return longhand.properties()[TopSide];
        default:
            return longhand.properties()[BottomSide];
        }
    }

    if (writingMode == BottomToTopWritingMode) {
        // Start = Right, End = Left, Before = Bottom, After = Top
        switch (logicalSide) {
        case StartSide:
            return longhand.properties()[RightSide];
        case EndSide:
            return longhand.properties()[LeftSide];
        case BeforeSide:
            return longhand.properties()[BottomSide];
        default:
            return longhand.properties()[TopSide];
        }
    }

    if (writingMode == LeftToRightWritingMode) {
        // Start = Bottom, End = Top, Before = Left, After = Right
        switch (logicalSide) {
        case StartSide:
            return longhand.properties()[BottomSide];
        case EndSide:
            return longhand.properties()[TopSide];
        case BeforeSide:
            return longhand.properties()[LeftSide];
        default:
            return longhand.properties()[RightSide];
        }
    }

    // Start = Bottom, End = Top, Before = Right, After = Left
    switch (logicalSide) {
    case StartSide:
        return longhand.properties()[BottomSide];
    case EndSide:
        return longhand.properties()[TopSide];
    case BeforeSide:
        return longhand.properties()[RightSide];
    default:
        return longhand.properties()[LeftSide];
    }
}

enum LogicalExtent { LogicalWidth, LogicalHeight };

static int resolveToPhysicalProperty(WritingMode writingMode, LogicalExtent logicalSide, const int* properties)
{
    if (writingMode == TopToBottomWritingMode || writingMode == BottomToTopWritingMode)
        return properties[logicalSide];
    return logicalSide == LogicalWidth ? properties[1] : properties[0];
}

static const CSSPropertyLonghand& borderDirections()
{
    static const int properties[4] = { CSSPropertyBorderTop, CSSPropertyBorderRight, CSSPropertyBorderBottom, CSSPropertyBorderLeft };
    DEFINE_STATIC_LOCAL(CSSPropertyLonghand, borderDirections, (properties, WTF_ARRAY_LENGTH(properties)));
    return borderDirections;
}

int CSSProperty::resolveDirectionAwareProperty(int propertyID, TextDirection direction, WritingMode writingMode)
{
    switch (static_cast<CSSPropertyID>(propertyID)) {
    case CSSPropertyWebkitMarginEnd:
        return resolveToPhysicalProperty(direction, writingMode, EndSide, marginLonghand());
    case CSSPropertyWebkitMarginStart:
        return resolveToPhysicalProperty(direction, writingMode, StartSide, marginLonghand());
    case CSSPropertyWebkitMarginBefore:
        return resolveToPhysicalProperty(direction, writingMode, BeforeSide, marginLonghand());
    case CSSPropertyWebkitMarginAfter:
        return resolveToPhysicalProperty(direction, writingMode, AfterSide, marginLonghand());
    case CSSPropertyWebkitPaddingEnd:
        return resolveToPhysicalProperty(direction, writingMode, EndSide, paddingLonghand());
    case CSSPropertyWebkitPaddingStart:
        return resolveToPhysicalProperty(direction, writingMode, StartSide, paddingLonghand());
    case CSSPropertyWebkitPaddingBefore:
        return resolveToPhysicalProperty(direction, writingMode, BeforeSide, paddingLonghand());
    case CSSPropertyWebkitPaddingAfter:
        return resolveToPhysicalProperty(direction, writingMode, AfterSide, paddingLonghand());
    case CSSPropertyWebkitBorderEnd:
        return resolveToPhysicalProperty(direction, writingMode, EndSide, borderDirections());
    case CSSPropertyWebkitBorderStart:
        return resolveToPhysicalProperty(direction, writingMode, StartSide, borderDirections());
    case CSSPropertyWebkitBorderBefore:
        return resolveToPhysicalProperty(direction, writingMode, BeforeSide, borderDirections());
    case CSSPropertyWebkitBorderAfter:
        return resolveToPhysicalProperty(direction, writingMode, AfterSide, borderDirections());
    case CSSPropertyWebkitBorderEndColor:
        return resolveToPhysicalProperty(direction, writingMode, EndSide, borderColorLonghand());
    case CSSPropertyWebkitBorderStartColor:
        return resolveToPhysicalProperty(direction, writingMode, StartSide, borderColorLonghand());
    case CSSPropertyWebkitBorderBeforeColor:
        return resolveToPhysicalProperty(direction, writingMode, BeforeSide, borderColorLonghand());
    case CSSPropertyWebkitBorderAfterColor:
        return resolveToPhysicalProperty(direction, writingMode, AfterSide, borderColorLonghand());
    case CSSPropertyWebkitBorderEndStyle:
        return resolveToPhysicalProperty(direction, writingMode, EndSide, borderStyleLonghand());
    case CSSPropertyWebkitBorderStartStyle:
        return resolveToPhysicalProperty(direction, writingMode, StartSide, borderStyleLonghand());
    case CSSPropertyWebkitBorderBeforeStyle:
        return resolveToPhysicalProperty(direction, writingMode, BeforeSide, borderStyleLonghand());
    case CSSPropertyWebkitBorderAfterStyle:
        return resolveToPhysicalProperty(direction, writingMode, AfterSide, borderStyleLonghand());
    case CSSPropertyWebkitBorderEndWidth:
        return resolveToPhysicalProperty(direction, writingMode, EndSide, borderWidthLonghand());
    case CSSPropertyWebkitBorderStartWidth:
        return resolveToPhysicalProperty(direction, writingMode, StartSide, borderWidthLonghand());
    case CSSPropertyWebkitBorderBeforeWidth:
        return resolveToPhysicalProperty(direction, writingMode, BeforeSide, borderWidthLonghand());
    case CSSPropertyWebkitBorderAfterWidth:
        return resolveToPhysicalProperty(direction, writingMode, AfterSide, borderWidthLonghand());
    case CSSPropertyWebkitLogicalWidth: {
        const int properties[2] = { CSSPropertyWidth, CSSPropertyHeight };
        return resolveToPhysicalProperty(writingMode, LogicalWidth, properties);
    }
    case CSSPropertyWebkitLogicalHeight: {
        const int properties[2] = { CSSPropertyWidth, CSSPropertyHeight };
        return resolveToPhysicalProperty(writingMode, LogicalHeight, properties);
    }
    case CSSPropertyWebkitMinLogicalWidth: {
        const int properties[2] = { CSSPropertyMinWidth, CSSPropertyMinHeight };
        return resolveToPhysicalProperty(writingMode, LogicalWidth, properties);
    }
    case CSSPropertyWebkitMinLogicalHeight: {
        const int properties[2] = { CSSPropertyMinWidth, CSSPropertyMinHeight };
        return resolveToPhysicalProperty(writingMode, LogicalHeight, properties);
    }
    case CSSPropertyWebkitMaxLogicalWidth: {
        const int properties[2] = { CSSPropertyMaxWidth, CSSPropertyMaxHeight };
        return resolveToPhysicalProperty(writingMode, LogicalWidth, properties);
    }
    case CSSPropertyWebkitMaxLogicalHeight: {
        const int properties[2] = { CSSPropertyMaxWidth, CSSPropertyMaxHeight };
        return resolveToPhysicalProperty(writingMode, LogicalHeight, properties);
    }
    default:
        return propertyID;
    }
}

bool CSSProperty::isInheritedProperty(unsigned propertyID)
{
    switch (static_cast<CSSPropertyID>(propertyID)) {
    case CSSPropertyBorderCollapse:
    case CSSPropertyBorderSpacing:
    case CSSPropertyCaptionSide:
    case CSSPropertyColor:
    case CSSPropertyCursor:
    case CSSPropertyDirection:
    case CSSPropertyEmptyCells:
    case CSSPropertyFont:
    case CSSPropertyFontFamily:
    case CSSPropertyFontSize:
    case CSSPropertyFontStyle:
    case CSSPropertyFontVariant:
    case CSSPropertyFontWeight:
    case CSSPropertyImageRendering:
    case CSSPropertyLetterSpacing:
    case CSSPropertyLineHeight:
    case CSSPropertyListStyle:
    case CSSPropertyListStyleImage:
    case CSSPropertyListStyleType:
    case CSSPropertyListStylePosition:
    case CSSPropertyOrphans:
    case CSSPropertyPointerEvents:
    case CSSPropertyQuotes:
    case CSSPropertyResize:
    case CSSPropertySpeak:
    case CSSPropertyTextAlign:
    case CSSPropertyTextDecoration:
    case CSSPropertyTextIndent:
    case CSSPropertyTextRendering:
    case CSSPropertyTextShadow:
    case CSSPropertyTextTransform:
    case CSSPropertyVisibility:
    case CSSPropertyWebkitAspectRatio:
    case CSSPropertyWebkitBorderHorizontalSpacing:
    case CSSPropertyWebkitBorderVerticalSpacing:
    case CSSPropertyWebkitBoxDirection:
    case CSSPropertyWebkitColorCorrection:
    case CSSPropertyWebkitFontFeatureSettings:
    case CSSPropertyWebkitFontKerning:
    case CSSPropertyWebkitFontSmoothing:
    case CSSPropertyWebkitFontVariantLigatures:
    case CSSPropertyWebkitLocale:
    case CSSPropertyWebkitHighlight:
    case CSSPropertyWebkitHyphenateCharacter:
    case CSSPropertyWebkitHyphenateLimitAfter:
    case CSSPropertyWebkitHyphenateLimitBefore:
    case CSSPropertyWebkitHyphenateLimitLines:
    case CSSPropertyWebkitHyphens:
    case CSSPropertyWebkitLineAlign:
    case CSSPropertyWebkitLineBoxContain:
    case CSSPropertyWebkitLineBreak:
    case CSSPropertyWebkitLineGrid:
    case CSSPropertyWebkitLineSnap:
    case CSSPropertyWebkitNbspMode:
#if ENABLE(OVERFLOW_SCROLLING)
    case CSSPropertyWebkitOverflowScrolling:
#endif
    case CSSPropertyWebkitPrintColorAdjust:
    case CSSPropertyWebkitRtlOrdering:
    case CSSPropertyWebkitTextCombine:
    case CSSPropertyWebkitTextDecorationsInEffect:
    case CSSPropertyWebkitTextEmphasis:
    case CSSPropertyWebkitTextEmphasisColor:
    case CSSPropertyWebkitTextEmphasisPosition:
    case CSSPropertyWebkitTextEmphasisStyle:
    case CSSPropertyWebkitTextFillColor:
    case CSSPropertyWebkitTextOrientation:
    case CSSPropertyWebkitTextSecurity:
    case CSSPropertyWebkitTextSizeAdjust:
    case CSSPropertyWebkitTextStroke:
    case CSSPropertyWebkitTextStrokeColor:
    case CSSPropertyWebkitTextStrokeWidth:
    case CSSPropertyWebkitUserModify:
    case CSSPropertyWebkitUserSelect:
    case CSSPropertyWebkitWritingMode:
    case CSSPropertyWhiteSpace:
    case CSSPropertyWidows:
    case CSSPropertyWordBreak:
    case CSSPropertyWordSpacing:
    case CSSPropertyWordWrap:
#if ENABLE(SVG)
    case CSSPropertyClipRule:
    case CSSPropertyColorInterpolation:
    case CSSPropertyColorInterpolationFilters:
    case CSSPropertyColorRendering:
    case CSSPropertyFill:
    case CSSPropertyFillOpacity:
    case CSSPropertyFillRule:
    case CSSPropertyGlyphOrientationHorizontal:
    case CSSPropertyGlyphOrientationVertical:
    case CSSPropertyKerning:
    case CSSPropertyMarker:
    case CSSPropertyMarkerEnd:
    case CSSPropertyMarkerMid:
    case CSSPropertyMarkerStart:
    case CSSPropertyStroke:
    case CSSPropertyStrokeDasharray:
    case CSSPropertyStrokeDashoffset:
    case CSSPropertyStrokeLinecap:
    case CSSPropertyStrokeLinejoin:
    case CSSPropertyStrokeMiterlimit:
    case CSSPropertyStrokeOpacity:
    case CSSPropertyStrokeWidth:
    case CSSPropertyShapeRendering:
    case CSSPropertyTextAnchor:
    case CSSPropertyWritingMode:
#endif
#if ENABLE(TOUCH_EVENTS)
    case CSSPropertyWebkitTapHighlightColor:
#endif
        return true;
    case CSSPropertyDisplay:
    case CSSPropertyZoom:
    case CSSPropertyBackground:
    case CSSPropertyBackgroundAttachment:
    case CSSPropertyBackgroundClip:
    case CSSPropertyBackgroundColor:
    case CSSPropertyBackgroundImage:
    case CSSPropertyBackgroundOrigin:
    case CSSPropertyBackgroundPosition:
    case CSSPropertyBackgroundPositionX:
    case CSSPropertyBackgroundPositionY:
    case CSSPropertyBackgroundRepeat:
    case CSSPropertyBackgroundRepeatX:
    case CSSPropertyBackgroundRepeatY:
    case CSSPropertyBackgroundSize:
    case CSSPropertyBorder:
    case CSSPropertyBorderBottom:
    case CSSPropertyBorderBottomColor:
    case CSSPropertyBorderBottomLeftRadius:
    case CSSPropertyBorderBottomRightRadius:
    case CSSPropertyBorderBottomStyle:
    case CSSPropertyBorderBottomWidth:
    case CSSPropertyBorderColor:
    case CSSPropertyBorderImage:
    case CSSPropertyBorderImageOutset:
    case CSSPropertyBorderImageRepeat:
    case CSSPropertyBorderImageSlice:
    case CSSPropertyBorderImageSource:
    case CSSPropertyBorderImageWidth:
    case CSSPropertyBorderLeft:
    case CSSPropertyBorderLeftColor:
    case CSSPropertyBorderLeftStyle:
    case CSSPropertyBorderLeftWidth:
    case CSSPropertyBorderRadius:
    case CSSPropertyBorderRight:
    case CSSPropertyBorderRightColor:
    case CSSPropertyBorderRightStyle:
    case CSSPropertyBorderRightWidth:
    case CSSPropertyBorderStyle:
    case CSSPropertyBorderTop:
    case CSSPropertyBorderTopColor:
    case CSSPropertyBorderTopLeftRadius:
    case CSSPropertyBorderTopRightRadius:
    case CSSPropertyBorderTopStyle:
    case CSSPropertyBorderTopWidth:
    case CSSPropertyBorderWidth:
    case CSSPropertyBottom:
    case CSSPropertyBoxShadow:
    case CSSPropertyBoxSizing:
    case CSSPropertyClear:
    case CSSPropertyClip:
    case CSSPropertyContent:
    case CSSPropertyCounterIncrement:
    case CSSPropertyCounterReset:
    case CSSPropertyFloat:
    case CSSPropertyFontStretch:
    case CSSPropertyHeight:
    case CSSPropertyLeft:
    case CSSPropertyMargin:
    case CSSPropertyMarginBottom:
    case CSSPropertyMarginLeft:
    case CSSPropertyMarginRight:
    case CSSPropertyMarginTop:
    case CSSPropertyMaxHeight:
    case CSSPropertyMaxWidth:
    case CSSPropertyMinHeight:
    case CSSPropertyMinWidth:
    case CSSPropertyOpacity:
    case CSSPropertyOutline:
    case CSSPropertyOutlineColor:
    case CSSPropertyOutlineOffset:
    case CSSPropertyOutlineStyle:
    case CSSPropertyOutlineWidth:
    case CSSPropertyOverflow:
    case CSSPropertyOverflowX:
    case CSSPropertyOverflowY:
    case CSSPropertyPadding:
    case CSSPropertyPaddingBottom:
    case CSSPropertyPaddingLeft:
    case CSSPropertyPaddingRight:
    case CSSPropertyPaddingTop:
    case CSSPropertyPage:
    case CSSPropertyPageBreakAfter:
    case CSSPropertyPageBreakBefore:
    case CSSPropertyPageBreakInside:
    case CSSPropertyPosition:
    case CSSPropertyRight:
    case CSSPropertySize:
    case CSSPropertySrc:
    case CSSPropertyTableLayout:
    case CSSPropertyTextLineThrough:
    case CSSPropertyTextLineThroughColor:
    case CSSPropertyTextLineThroughMode:
    case CSSPropertyTextLineThroughStyle:
    case CSSPropertyTextLineThroughWidth:
    case CSSPropertyTextOverflow:
    case CSSPropertyTextOverline:
    case CSSPropertyTextOverlineColor:
    case CSSPropertyTextOverlineMode:
    case CSSPropertyTextOverlineStyle:
    case CSSPropertyTextOverlineWidth:
    case CSSPropertyTextUnderline:
    case CSSPropertyTextUnderlineColor:
    case CSSPropertyTextUnderlineMode:
    case CSSPropertyTextUnderlineStyle:
    case CSSPropertyTextUnderlineWidth:
    case CSSPropertyTop:
    case CSSPropertyUnicodeBidi:
    case CSSPropertyUnicodeRange:
    case CSSPropertyVerticalAlign:
    case CSSPropertyWidth:
    case CSSPropertyZIndex:
    case CSSPropertyWebkitAnimation:
    case CSSPropertyWebkitAnimationDelay:
    case CSSPropertyWebkitAnimationDirection:
    case CSSPropertyWebkitAnimationDuration:
    case CSSPropertyWebkitAnimationFillMode:
    case CSSPropertyWebkitAnimationIterationCount:
    case CSSPropertyWebkitAnimationName:
    case CSSPropertyWebkitAnimationPlayState:
    case CSSPropertyWebkitAnimationTimingFunction:
    case CSSPropertyWebkitAppearance:
    case CSSPropertyWebkitBackfaceVisibility:
    case CSSPropertyWebkitBackgroundClip:
    case CSSPropertyWebkitBackgroundComposite:
    case CSSPropertyWebkitBackgroundOrigin:
    case CSSPropertyWebkitBackgroundSize:
    case CSSPropertyWebkitBorderAfter:
    case CSSPropertyWebkitBorderAfterColor:
    case CSSPropertyWebkitBorderAfterStyle:
    case CSSPropertyWebkitBorderAfterWidth:
    case CSSPropertyWebkitBorderBefore:
    case CSSPropertyWebkitBorderBeforeColor:
    case CSSPropertyWebkitBorderBeforeStyle:
    case CSSPropertyWebkitBorderBeforeWidth:
    case CSSPropertyWebkitBorderEnd:
    case CSSPropertyWebkitBorderEndColor:
    case CSSPropertyWebkitBorderEndStyle:
    case CSSPropertyWebkitBorderEndWidth:
    case CSSPropertyWebkitBorderFit:
    case CSSPropertyWebkitBorderImage:
    case CSSPropertyWebkitBorderRadius:
    case CSSPropertyWebkitBorderStart:
    case CSSPropertyWebkitBorderStartColor:
    case CSSPropertyWebkitBorderStartStyle:
    case CSSPropertyWebkitBorderStartWidth:
    case CSSPropertyWebkitBoxAlign:
    case CSSPropertyWebkitBoxFlex:
    case CSSPropertyWebkitBoxFlexGroup:
    case CSSPropertyWebkitBoxLines:
    case CSSPropertyWebkitBoxOrdinalGroup:
    case CSSPropertyWebkitBoxOrient:
    case CSSPropertyWebkitBoxPack:
    case CSSPropertyWebkitBoxReflect:
    case CSSPropertyWebkitBoxShadow:
    case CSSPropertyWebkitColumnAxis:
    case CSSPropertyWebkitColumnBreakAfter:
    case CSSPropertyWebkitColumnBreakBefore:
    case CSSPropertyWebkitColumnBreakInside:
    case CSSPropertyWebkitColumnCount:
    case CSSPropertyWebkitColumnGap:
    case CSSPropertyWebkitColumnRule:
    case CSSPropertyWebkitColumnRuleColor:
    case CSSPropertyWebkitColumnRuleStyle:
    case CSSPropertyWebkitColumnRuleWidth:
    case CSSPropertyWebkitColumnSpan:
    case CSSPropertyWebkitColumnWidth:
    case CSSPropertyWebkitColumns:
#if ENABLE(CSS_FILTERS)
    case CSSPropertyWebkitFilter:
#endif
    case CSSPropertyWebkitFlexOrder:
    case CSSPropertyWebkitFlexPack:
    case CSSPropertyWebkitFlexAlign:
    case CSSPropertyWebkitFlexItemAlign:
    case CSSPropertyWebkitFlexDirection:
    case CSSPropertyWebkitFlexFlow:
    case CSSPropertyWebkitFlexLinePack:
    case CSSPropertyWebkitFlexWrap:
    case CSSPropertyWebkitFontSizeDelta:
#if ENABLE(CSS_GRID_LAYOUT)
    case CSSPropertyWebkitGridColumns:
    case CSSPropertyWebkitGridRows:

    case CSSPropertyWebkitGridColumn:
    case CSSPropertyWebkitGridRow:
#endif
    case CSSPropertyWebkitLineClamp:
    case CSSPropertyWebkitLogicalWidth:
    case CSSPropertyWebkitLogicalHeight:
    case CSSPropertyWebkitMarginAfterCollapse:
    case CSSPropertyWebkitMarginBeforeCollapse:
    case CSSPropertyWebkitMarginBottomCollapse:
    case CSSPropertyWebkitMarginTopCollapse:
    case CSSPropertyWebkitMarginCollapse:
    case CSSPropertyWebkitMarginAfter:
    case CSSPropertyWebkitMarginBefore:
    case CSSPropertyWebkitMarginEnd:
    case CSSPropertyWebkitMarginStart:
    case CSSPropertyWebkitMarquee:
    case CSSPropertyWebkitMarqueeDirection:
    case CSSPropertyWebkitMarqueeIncrement:
    case CSSPropertyWebkitMarqueeRepetition:
    case CSSPropertyWebkitMarqueeSpeed:
    case CSSPropertyWebkitMarqueeStyle:
    case CSSPropertyWebkitMask:
    case CSSPropertyWebkitMaskAttachment:
    case CSSPropertyWebkitMaskBoxImage:
    case CSSPropertyWebkitMaskBoxImageOutset:
    case CSSPropertyWebkitMaskBoxImageRepeat:
    case CSSPropertyWebkitMaskBoxImageSlice:
    case CSSPropertyWebkitMaskBoxImageSource:
    case CSSPropertyWebkitMaskBoxImageWidth:
    case CSSPropertyWebkitMaskClip:
    case CSSPropertyWebkitMaskComposite:
    case CSSPropertyWebkitMaskImage:
    case CSSPropertyWebkitMaskOrigin:
    case CSSPropertyWebkitMaskPosition:
    case CSSPropertyWebkitMaskPositionX:
    case CSSPropertyWebkitMaskPositionY:
    case CSSPropertyWebkitMaskRepeat:
    case CSSPropertyWebkitMaskRepeatX:
    case CSSPropertyWebkitMaskRepeatY:
    case CSSPropertyWebkitMaskSize:
    case CSSPropertyWebkitMatchNearestMailBlockquoteColor:
    case CSSPropertyWebkitMaxLogicalWidth:
    case CSSPropertyWebkitMaxLogicalHeight:
    case CSSPropertyWebkitMinLogicalWidth:
    case CSSPropertyWebkitMinLogicalHeight:
    case CSSPropertyWebkitPaddingAfter:
    case CSSPropertyWebkitPaddingBefore:
    case CSSPropertyWebkitPaddingEnd:
    case CSSPropertyWebkitPaddingStart:
    case CSSPropertyWebkitPerspective:
    case CSSPropertyWebkitPerspectiveOrigin:
    case CSSPropertyWebkitPerspectiveOriginX:
    case CSSPropertyWebkitPerspectiveOriginY:
    case CSSPropertyWebkitTransform:
    case CSSPropertyWebkitTransformOrigin:
    case CSSPropertyWebkitTransformOriginX:
    case CSSPropertyWebkitTransformOriginY:
    case CSSPropertyWebkitTransformOriginZ:
    case CSSPropertyWebkitTransformStyle:
    case CSSPropertyWebkitTransition:
    case CSSPropertyWebkitTransitionDelay:
    case CSSPropertyWebkitTransitionDuration:
    case CSSPropertyWebkitTransitionProperty:
    case CSSPropertyWebkitTransitionTimingFunction:
    case CSSPropertyWebkitUserDrag:
    case CSSPropertyWebkitFlowInto:
    case CSSPropertyWebkitFlowFrom:
    case CSSPropertyWebkitRegionOverflow:
    case CSSPropertyWebkitRegionBreakAfter:
    case CSSPropertyWebkitRegionBreakBefore:
    case CSSPropertyWebkitRegionBreakInside:
    case CSSPropertyWebkitWrap:
    case CSSPropertyWebkitWrapFlow:
    case CSSPropertyWebkitWrapMargin:
    case CSSPropertyWebkitWrapPadding:
    case CSSPropertyWebkitWrapShapeInside:
    case CSSPropertyWebkitWrapShapeOutside:
    case CSSPropertyWebkitWrapThrough:
#if ENABLE(SVG)
    case CSSPropertyClipPath:
    case CSSPropertyMask:
    case CSSPropertyEnableBackground:
    case CSSPropertyFilter:
    case CSSPropertyFloodColor:
    case CSSPropertyFloodOpacity:
    case CSSPropertyLightingColor:
    case CSSPropertyStopColor:
    case CSSPropertyStopOpacity:
    case CSSPropertyColorProfile:
    case CSSPropertyAlignmentBaseline:
    case CSSPropertyBaselineShift:
    case CSSPropertyDominantBaseline:
    case CSSPropertyVectorEffect:
    case CSSPropertyWebkitSvgShadow:
#endif
#if ENABLE(DASHBOARD_SUPPORT)
    case CSSPropertyWebkitDashboardRegion:
#endif
        return false;
    case CSSPropertyInvalid:
        ASSERT_NOT_REACHED();
        return false;
    }
    ASSERT_NOT_REACHED();
    return false;
}

} // namespace WebCore
