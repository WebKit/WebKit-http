/*
 * Copyright (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2014 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Adobe Systems Incorporated. All rights reserved.
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
#include "RenderStyle.h"

#include "ContentData.h"
#include "CursorList.h"
#include "CSSPropertyNames.h"
#include "FloatRoundedRect.h"
#include "Font.h"
#include "FontSelector.h"
#include "InlineTextBoxStyle.h"
#include "Pagination.h"
#include "QuotesData.h"
#include "RenderObject.h"
#include "ScaleTransformOperation.h"
#include "ShadowData.h"
#include "StyleImage.h"
#include "StyleInheritedData.h"
#include "StyleResolver.h"
#if ENABLE(TOUCH_EVENTS)
#include "RenderTheme.h"
#endif
#include <wtf/MathExtras.h>
#include <wtf/StdLibExtras.h>
#include <algorithm>

#if ENABLE(IOS_TEXT_AUTOSIZING)
#include <wtf/text/StringHash.h>
#endif

#if ENABLE(TEXT_AUTOSIZING)
#include "TextAutosizer.h"
#endif

namespace WebCore {

struct SameSizeAsBorderValue {
    float m_width;
    RGBA32 m_color;
    int m_restBits;
};

COMPILE_ASSERT(sizeof(BorderValue) == sizeof(SameSizeAsBorderValue), BorderValue_should_not_grow);

struct SameSizeAsRenderStyle : public RefCounted<SameSizeAsRenderStyle> {
    void* dataRefs[7];
    void* ownPtrs[1];
    void* dataRefSvgStyle;
    struct InheritedFlags {
        unsigned m_bitfields[2];
    } inherited_flags;

    struct NonInheritedFlags {
        uint64_t m_flags;
    } noninherited_flags;
};

COMPILE_ASSERT(sizeof(RenderStyle) == sizeof(SameSizeAsRenderStyle), RenderStyle_should_stay_small);

inline RenderStyle& defaultStyle()
{
    static RenderStyle& style = RenderStyle::createDefaultStyle().leakRef();
    return style;
}

PassRef<RenderStyle> RenderStyle::create()
{
    return adoptRef(*new RenderStyle(defaultStyle()));
}

PassRef<RenderStyle> RenderStyle::createDefaultStyle()
{
    return adoptRef(*new RenderStyle(true));
}

PassRef<RenderStyle> RenderStyle::createAnonymousStyleWithDisplay(const RenderStyle* parentStyle, EDisplay display)
{
    auto newStyle = RenderStyle::create();
    newStyle.get().inheritFrom(parentStyle);
    newStyle.get().inheritUnicodeBidiFrom(parentStyle);
    newStyle.get().setDisplay(display);
    return newStyle;
}

PassRef<RenderStyle> RenderStyle::clone(const RenderStyle* other)
{
    return adoptRef(*new RenderStyle(*other));
}

PassRef<RenderStyle> RenderStyle::createStyleInheritingFromPseudoStyle(const RenderStyle& pseudoStyle)
{
    ASSERT(pseudoStyle.styleType() == BEFORE || pseudoStyle.styleType() == AFTER);

    auto style = RenderStyle::create();
    style.get().inheritFrom(&pseudoStyle);
    return style;
}

ALWAYS_INLINE RenderStyle::RenderStyle(bool)
    : m_box(StyleBoxData::create())
    , visual(StyleVisualData::create())
    , m_background(StyleBackgroundData::create())
    , surround(StyleSurroundData::create())
    , rareNonInheritedData(StyleRareNonInheritedData::create())
    , rareInheritedData(StyleRareInheritedData::create())
    , inherited(StyleInheritedData::create())
    , m_svgStyle(SVGRenderStyle::create())
{
    inherited_flags._empty_cells = initialEmptyCells();
    inherited_flags._caption_side = initialCaptionSide();
    inherited_flags._list_style_type = initialListStyleType();
    inherited_flags._list_style_position = initialListStylePosition();
    inherited_flags._visibility = initialVisibility();
    inherited_flags._text_align = initialTextAlign();
    inherited_flags._text_transform = initialTextTransform();
    inherited_flags._text_decorations = initialTextDecoration();
    inherited_flags._cursor_style = initialCursor();
#if ENABLE(CURSOR_VISIBILITY)
    inherited_flags.m_cursorVisibility = initialCursorVisibility();
#endif
    inherited_flags._direction = initialDirection();
    inherited_flags._white_space = initialWhiteSpace();
    inherited_flags._border_collapse = initialBorderCollapse();
    inherited_flags.m_rtlOrdering = initialRTLOrdering();
    inherited_flags._box_direction = initialBoxDirection();
    inherited_flags.m_printColorAdjust = initialPrintColorAdjust();
    inherited_flags._pointerEvents = initialPointerEvents();
    inherited_flags._insideLink = NotInsideLink;
    inherited_flags.m_writingMode = initialWritingMode();

    static_assert((sizeof(InheritedFlags) <= 8), "InheritedFlags does not grow");
    static_assert((sizeof(NonInheritedFlags) <= 8), "NonInheritedFlags does not grow");
}

ALWAYS_INLINE RenderStyle::RenderStyle(const RenderStyle& o)
    : RefCounted<RenderStyle>()
    , m_box(o.m_box)
    , visual(o.visual)
    , m_background(o.m_background)
    , surround(o.surround)
    , rareNonInheritedData(o.rareNonInheritedData)
    , rareInheritedData(o.rareInheritedData)
    , inherited(o.inherited)
    , m_svgStyle(o.m_svgStyle)
    , inherited_flags(o.inherited_flags)
    , noninherited_flags(o.noninherited_flags)
{
}

void RenderStyle::inheritFrom(const RenderStyle* inheritParent, IsAtShadowBoundary isAtShadowBoundary)
{
    if (isAtShadowBoundary == AtShadowBoundary) {
        // Even if surrounding content is user-editable, shadow DOM should act as a single unit, and not necessarily be editable
        EUserModify currentUserModify = userModify();
        rareInheritedData = inheritParent->rareInheritedData;
        setUserModify(currentUserModify);
    } else
        rareInheritedData = inheritParent->rareInheritedData;
    inherited = inheritParent->inherited;
    inherited_flags = inheritParent->inherited_flags;

    if (m_svgStyle != inheritParent->m_svgStyle)
        m_svgStyle.access()->inheritFrom(inheritParent->m_svgStyle.get());
}

void RenderStyle::copyNonInheritedFrom(const RenderStyle* other)
{
    m_box = other->m_box;
    visual = other->visual;
    m_background = other->m_background;
    surround = other->surround;
    rareNonInheritedData = other->rareNonInheritedData;
    noninherited_flags.copyNonInheritedFrom(other->noninherited_flags);

    if (m_svgStyle != other->m_svgStyle)
        m_svgStyle.access()->copyNonInheritedFrom(other->m_svgStyle.get());

    ASSERT(zoom() == initialZoom());
}

bool RenderStyle::operator==(const RenderStyle& o) const
{
    // compare everything except the pseudoStyle pointer
    return inherited_flags == o.inherited_flags
        && noninherited_flags == o.noninherited_flags
        && m_box == o.m_box
        && visual == o.visual
        && m_background == o.m_background
        && surround == o.surround
        && rareNonInheritedData == o.rareNonInheritedData
        && rareInheritedData == o.rareInheritedData
        && inherited == o.inherited
        && m_svgStyle == o.m_svgStyle
            ;
}

bool RenderStyle::isStyleAvailable() const
{
    return this != StyleResolver::styleNotYetAvailable();
}

bool RenderStyle::hasUniquePseudoStyle() const
{
    if (!m_cachedPseudoStyles || styleType() != NOPSEUDO)
        return false;

    for (size_t i = 0; i < m_cachedPseudoStyles->size(); ++i) {
        RenderStyle* pseudoStyle = m_cachedPseudoStyles->at(i).get();
        if (pseudoStyle->unique())
            return true;
    }

    return false;
}

RenderStyle* RenderStyle::getCachedPseudoStyle(PseudoId pid) const
{
    if (!m_cachedPseudoStyles || !m_cachedPseudoStyles->size())
        return 0;

    if (styleType() != NOPSEUDO) 
        return 0;

    for (size_t i = 0; i < m_cachedPseudoStyles->size(); ++i) {
        RenderStyle* pseudoStyle = m_cachedPseudoStyles->at(i).get();
        if (pseudoStyle->styleType() == pid)
            return pseudoStyle;
    }

    return 0;
}

RenderStyle* RenderStyle::addCachedPseudoStyle(PassRefPtr<RenderStyle> pseudo)
{
    if (!pseudo)
        return 0;

    ASSERT(pseudo->styleType() > NOPSEUDO);

    RenderStyle* result = pseudo.get();

    if (!m_cachedPseudoStyles)
        m_cachedPseudoStyles = std::make_unique<PseudoStyleCache>();

    m_cachedPseudoStyles->append(pseudo);

    return result;
}

void RenderStyle::removeCachedPseudoStyle(PseudoId pid)
{
    if (!m_cachedPseudoStyles)
        return;
    for (size_t i = 0; i < m_cachedPseudoStyles->size(); ++i) {
        RenderStyle* pseudoStyle = m_cachedPseudoStyles->at(i).get();
        if (pseudoStyle->styleType() == pid) {
            m_cachedPseudoStyles->remove(i);
            return;
        }
    }
}

bool RenderStyle::inheritedNotEqual(const RenderStyle* other) const
{
    return inherited_flags != other->inherited_flags
           || inherited != other->inherited
           || m_svgStyle->inheritedNotEqual(other->m_svgStyle.get())
           || rareInheritedData != other->rareInheritedData;
}

#if ENABLE(IOS_TEXT_AUTOSIZING)
inline unsigned computeFontHash(const Font& font)
{
    unsigned hashCodes[2] = {
        CaseFoldingHash::hash(font.fontDescription().firstFamily().impl()),
        static_cast<unsigned>(font.fontDescription().specifiedSize())
    };
    return StringHasher::computeHash(reinterpret_cast<UChar*>(hashCodes), 2 * sizeof(unsigned) / sizeof(UChar));
}

uint32_t RenderStyle::hashForTextAutosizing() const
{
    // FIXME: Not a very smart hash. Could be improved upon. See <https://bugs.webkit.org/show_bug.cgi?id=121131>.
    uint32_t hash = 0;
    
    hash ^= rareNonInheritedData->m_appearance;
    hash ^= rareNonInheritedData->marginBeforeCollapse;
    hash ^= rareNonInheritedData->marginAfterCollapse;
    hash ^= rareNonInheritedData->lineClamp.value();
    hash ^= rareInheritedData->overflowWrap;
    hash ^= rareInheritedData->nbspMode;
    hash ^= rareInheritedData->lineBreak;
    hash ^= WTF::FloatHash<float>::hash(inherited->specifiedLineHeight.value());
    hash ^= computeFontHash(inherited->font);
    hash ^= inherited->horizontal_border_spacing;
    hash ^= inherited->vertical_border_spacing;
    hash ^= inherited_flags._box_direction;
    hash ^= inherited_flags.m_rtlOrdering;
    hash ^= noninherited_flags.position();
    hash ^= noninherited_flags.floating();
    hash ^= rareNonInheritedData->textOverflow;
    hash ^= rareInheritedData->textSecurity;
    return hash;
}

bool RenderStyle::equalForTextAutosizing(const RenderStyle* other) const
{
    return rareNonInheritedData->m_appearance == other->rareNonInheritedData->m_appearance
        && rareNonInheritedData->marginBeforeCollapse == other->rareNonInheritedData->marginBeforeCollapse
        && rareNonInheritedData->marginAfterCollapse == other->rareNonInheritedData->marginAfterCollapse
        && rareNonInheritedData->lineClamp == other->rareNonInheritedData->lineClamp
        && rareInheritedData->textSizeAdjust == other->rareInheritedData->textSizeAdjust
        && rareInheritedData->overflowWrap == other->rareInheritedData->overflowWrap
        && rareInheritedData->nbspMode == other->rareInheritedData->nbspMode
        && rareInheritedData->lineBreak == other->rareInheritedData->lineBreak
        && rareInheritedData->textSecurity == other->rareInheritedData->textSecurity
        && inherited->specifiedLineHeight == other->inherited->specifiedLineHeight
        && inherited->font.equalForTextAutoSizing(other->inherited->font)
        && inherited->horizontal_border_spacing == other->inherited->horizontal_border_spacing
        && inherited->vertical_border_spacing == other->inherited->vertical_border_spacing
        && inherited_flags._box_direction == other->inherited_flags._box_direction
        && inherited_flags.m_rtlOrdering == other->inherited_flags.m_rtlOrdering
        && noninherited_flags.position() == other->noninherited_flags.position()
        && noninherited_flags.floating() == other->noninherited_flags.floating()
        && rareNonInheritedData->textOverflow == other->rareNonInheritedData->textOverflow;
}
#endif // ENABLE(IOS_TEXT_AUTOSIZING)

bool RenderStyle::inheritedDataShared(const RenderStyle* other) const
{
    // This is a fast check that only looks if the data structures are shared.
    return inherited_flags == other->inherited_flags
        && inherited.get() == other->inherited.get()
        && m_svgStyle.get() == other->m_svgStyle.get()
        && rareInheritedData.get() == other->rareInheritedData.get();
}

static bool positionChangeIsMovementOnly(const LengthBox& a, const LengthBox& b, const Length& width)
{
    // If any unit types are different, then we can't guarantee
    // that this was just a movement.
    if (a.left().type() != b.left().type()
        || a.right().type() != b.right().type()
        || a.top().type() != b.top().type()
        || a.bottom().type() != b.bottom().type())
        return false;

    // Only one unit can be non-auto in the horizontal direction and
    // in the vertical direction.  Otherwise the adjustment of values
    // is changing the size of the box.
    if (!a.left().isIntrinsicOrAuto() && !a.right().isIntrinsicOrAuto())
        return false;
    if (!a.top().isIntrinsicOrAuto() && !a.bottom().isIntrinsicOrAuto())
        return false;
    // If our width is auto and left or right is specified then this 
    // is not just a movement - we need to resize to our container.
    if ((!a.left().isIntrinsicOrAuto() || !a.right().isIntrinsicOrAuto()) && width.isIntrinsicOrAuto())
        return false;

    // One of the units is fixed or percent in both directions and stayed
    // that way in the new style.  Therefore all we are doing is moving.
    return true;
}

inline bool RenderStyle::changeAffectsVisualOverflow(const RenderStyle& other) const
{
    if (rareNonInheritedData.get() != other.rareNonInheritedData.get()
        && !rareNonInheritedData->shadowDataEquivalent(*other.rareNonInheritedData.get()))
        return true;

    if (inherited_flags._text_decorations != other.inherited_flags._text_decorations
        || visual->textDecoration != other.visual->textDecoration
        || rareNonInheritedData->m_textDecorationStyle != other.rareNonInheritedData->m_textDecorationStyle) {
        // Underlines are always drawn outside of their textbox bounds when text-underline-position: under;
        // is specified. We can take an early out here.
        if (textUnderlinePosition() == TextUnderlinePositionUnder
            || other.textUnderlinePosition() == TextUnderlinePositionUnder)
            return true;
        return visualOverflowForDecorations(*this, nullptr) != visualOverflowForDecorations(other, nullptr);
    }

    return false;
}

bool RenderStyle::changeRequiresLayout(const RenderStyle* other, unsigned& changedContextSensitiveProperties) const
{
    if (m_box->width() != other->m_box->width()
        || m_box->minWidth() != other->m_box->minWidth()
        || m_box->maxWidth() != other->m_box->maxWidth()
        || m_box->height() != other->m_box->height()
        || m_box->minHeight() != other->m_box->minHeight()
        || m_box->maxHeight() != other->m_box->maxHeight())
        return true;

    if (m_box->verticalAlign() != other->m_box->verticalAlign() || noninherited_flags.verticalAlign() != other->noninherited_flags.verticalAlign())
        return true;

    if (m_box->boxSizing() != other->m_box->boxSizing())
        return true;

    if (surround->margin != other->surround->margin)
        return true;

    if (surround->padding != other->surround->padding)
        return true;

    // FIXME: We should add an optimized form of layout that just recomputes visual overflow.
    if (changeAffectsVisualOverflow(*other))
        return true;

    if (rareNonInheritedData.get() != other->rareNonInheritedData.get()) {
        if (rareNonInheritedData->m_appearance != other->rareNonInheritedData->m_appearance 
            || rareNonInheritedData->marginBeforeCollapse != other->rareNonInheritedData->marginBeforeCollapse
            || rareNonInheritedData->marginAfterCollapse != other->rareNonInheritedData->marginAfterCollapse
            || rareNonInheritedData->lineClamp != other->rareNonInheritedData->lineClamp
            || rareNonInheritedData->textOverflow != other->rareNonInheritedData->textOverflow)
            return true;

        if (rareNonInheritedData->m_regionFragment != other->rareNonInheritedData->m_regionFragment)
            return true;

#if ENABLE(CSS_SHAPES)
        if (rareNonInheritedData->m_shapeMargin != other->rareNonInheritedData->m_shapeMargin)
            return true;
#endif

        if (rareNonInheritedData->m_deprecatedFlexibleBox.get() != other->rareNonInheritedData->m_deprecatedFlexibleBox.get()
            && *rareNonInheritedData->m_deprecatedFlexibleBox.get() != *other->rareNonInheritedData->m_deprecatedFlexibleBox.get())
            return true;

        if (rareNonInheritedData->m_flexibleBox.get() != other->rareNonInheritedData->m_flexibleBox.get()
            && *rareNonInheritedData->m_flexibleBox.get() != *other->rareNonInheritedData->m_flexibleBox.get())
            return true;
        if (rareNonInheritedData->m_order != other->rareNonInheritedData->m_order
            || rareNonInheritedData->m_alignContent != other->rareNonInheritedData->m_alignContent
            || rareNonInheritedData->m_alignItems != other->rareNonInheritedData->m_alignItems
            || rareNonInheritedData->m_alignSelf != other->rareNonInheritedData->m_alignSelf
            || rareNonInheritedData->m_justifyContent != other->rareNonInheritedData->m_justifyContent)
            return true;

        if (!rareNonInheritedData->reflectionDataEquivalent(*other->rareNonInheritedData.get()))
            return true;

        if (rareNonInheritedData->m_multiCol.get() != other->rareNonInheritedData->m_multiCol.get()
            && *rareNonInheritedData->m_multiCol.get() != *other->rareNonInheritedData->m_multiCol.get())
            return true;

        if (rareNonInheritedData->m_transform.get() != other->rareNonInheritedData->m_transform.get()
            && *rareNonInheritedData->m_transform.get() != *other->rareNonInheritedData->m_transform.get()) {
            changedContextSensitiveProperties |= ContextSensitivePropertyTransform;
            // Don't return; keep looking for another change
        }

#if ENABLE(CSS_GRID_LAYOUT)
        if (rareNonInheritedData->m_grid.get() != other->rareNonInheritedData->m_grid.get()
            || rareNonInheritedData->m_gridItem.get() != other->rareNonInheritedData->m_gridItem.get())
            return true;
#endif

#if ENABLE(DASHBOARD_SUPPORT)
        // If regions change, trigger a relayout to re-calc regions.
        if (rareNonInheritedData->m_dashboardRegions != other->rareNonInheritedData->m_dashboardRegions)
            return true;
#endif
    }

    if (rareInheritedData.get() != other->rareInheritedData.get()) {
        if (rareInheritedData->indent != other->rareInheritedData->indent
#if ENABLE(CSS3_TEXT)
            || rareInheritedData->m_textAlignLast != other->rareInheritedData->m_textAlignLast
            || rareInheritedData->m_textJustify != other->rareInheritedData->m_textJustify
            || rareInheritedData->m_textIndentLine != other->rareInheritedData->m_textIndentLine
#endif
            || rareInheritedData->m_effectiveZoom != other->rareInheritedData->m_effectiveZoom
#if ENABLE(IOS_TEXT_AUTOSIZING)
            || rareInheritedData->textSizeAdjust != other->rareInheritedData->textSizeAdjust
#endif
            || rareInheritedData->wordBreak != other->rareInheritedData->wordBreak
            || rareInheritedData->overflowWrap != other->rareInheritedData->overflowWrap
            || rareInheritedData->nbspMode != other->rareInheritedData->nbspMode
            || rareInheritedData->lineBreak != other->rareInheritedData->lineBreak
            || rareInheritedData->textSecurity != other->rareInheritedData->textSecurity
            || rareInheritedData->hyphens != other->rareInheritedData->hyphens
            || rareInheritedData->hyphenationLimitBefore != other->rareInheritedData->hyphenationLimitBefore
            || rareInheritedData->hyphenationLimitAfter != other->rareInheritedData->hyphenationLimitAfter
            || rareInheritedData->hyphenationString != other->rareInheritedData->hyphenationString
            || rareInheritedData->locale != other->rareInheritedData->locale
            || rareInheritedData->m_rubyPosition != other->rareInheritedData->m_rubyPosition
            || rareInheritedData->textEmphasisMark != other->rareInheritedData->textEmphasisMark
            || rareInheritedData->textEmphasisPosition != other->rareInheritedData->textEmphasisPosition
            || rareInheritedData->textEmphasisCustomMark != other->rareInheritedData->textEmphasisCustomMark
            || rareInheritedData->m_textOrientation != other->rareInheritedData->m_textOrientation
            || rareInheritedData->m_tabSize != other->rareInheritedData->m_tabSize
            || rareInheritedData->m_lineBoxContain != other->rareInheritedData->m_lineBoxContain
            || rareInheritedData->m_lineGrid != other->rareInheritedData->m_lineGrid
#if ENABLE(CSS_IMAGE_ORIENTATION)
            || rareInheritedData->m_imageOrientation != other->rareInheritedData->m_imageOrientation
#endif
#if ENABLE(CSS_IMAGE_RESOLUTION)
            || rareInheritedData->m_imageResolutionSource != other->rareInheritedData->m_imageResolutionSource
            || rareInheritedData->m_imageResolutionSnap != other->rareInheritedData->m_imageResolutionSnap
            || rareInheritedData->m_imageResolution != other->rareInheritedData->m_imageResolution
#endif
            || rareInheritedData->m_lineSnap != other->rareInheritedData->m_lineSnap
            || rareInheritedData->m_lineAlign != other->rareInheritedData->m_lineAlign
#if ENABLE(ACCELERATED_OVERFLOW_SCROLLING)
            || rareInheritedData->useTouchOverflowScrolling != other->rareInheritedData->useTouchOverflowScrolling
#endif
            || rareInheritedData->listStyleImage != other->rareInheritedData->listStyleImage)
            return true;

        if (!rareInheritedData->shadowDataEquivalent(*other->rareInheritedData.get()))
            return true;

        if (textStrokeWidth() != other->textStrokeWidth())
            return true;
    }

#if ENABLE(TEXT_AUTOSIZING)
    if (visual->m_textAutosizingMultiplier != other->visual->m_textAutosizingMultiplier)
        return true;
#endif

    if (inherited->line_height != other->inherited->line_height
#if ENABLE(IOS_TEXT_AUTOSIZING)
        || inherited->specifiedLineHeight != other->inherited->specifiedLineHeight
#endif
        || inherited->font != other->inherited->font
        || inherited->horizontal_border_spacing != other->inherited->horizontal_border_spacing
        || inherited->vertical_border_spacing != other->inherited->vertical_border_spacing
        || inherited_flags._box_direction != other->inherited_flags._box_direction
        || inherited_flags.m_rtlOrdering != other->inherited_flags.m_rtlOrdering
        || noninherited_flags.position() != other->noninherited_flags.position()
        || noninherited_flags.floating() != other->noninherited_flags.floating()
        || noninherited_flags.originalDisplay() != other->noninherited_flags.originalDisplay())
        return true;


    if ((noninherited_flags.effectiveDisplay()) >= TABLE) {
        if (inherited_flags._border_collapse != other->inherited_flags._border_collapse
            || inherited_flags._empty_cells != other->inherited_flags._empty_cells
            || inherited_flags._caption_side != other->inherited_flags._caption_side
            || noninherited_flags.tableLayout() != other->noninherited_flags.tableLayout())
            return true;

        // In the collapsing border model, 'hidden' suppresses other borders, while 'none'
        // does not, so these style differences can be width differences.
        if (inherited_flags._border_collapse
            && ((borderTopStyle() == BHIDDEN && other->borderTopStyle() == BNONE)
                || (borderTopStyle() == BNONE && other->borderTopStyle() == BHIDDEN)
                || (borderBottomStyle() == BHIDDEN && other->borderBottomStyle() == BNONE)
                || (borderBottomStyle() == BNONE && other->borderBottomStyle() == BHIDDEN)
                || (borderLeftStyle() == BHIDDEN && other->borderLeftStyle() == BNONE)
                || (borderLeftStyle() == BNONE && other->borderLeftStyle() == BHIDDEN)
                || (borderRightStyle() == BHIDDEN && other->borderRightStyle() == BNONE)
                || (borderRightStyle() == BNONE && other->borderRightStyle() == BHIDDEN)))
            return true;
    }

    if (noninherited_flags.effectiveDisplay() == LIST_ITEM) {
        if (inherited_flags._list_style_type != other->inherited_flags._list_style_type
            || inherited_flags._list_style_position != other->inherited_flags._list_style_position)
            return true;
    }

    if (inherited_flags._text_align != other->inherited_flags._text_align
        || inherited_flags._text_transform != other->inherited_flags._text_transform
        || inherited_flags._direction != other->inherited_flags._direction
        || inherited_flags._white_space != other->inherited_flags._white_space
        || noninherited_flags.clear() != other->noninherited_flags.clear()
        || noninherited_flags.unicodeBidi() != other->noninherited_flags.unicodeBidi())
        return true;

    // Check block flow direction.
    if (inherited_flags.m_writingMode != other->inherited_flags.m_writingMode)
        return true;

    // Check text combine mode.
    if (rareNonInheritedData->m_textCombine != other->rareNonInheritedData->m_textCombine)
        return true;

    // Overflow returns a layout hint.
    if (noninherited_flags.overflowX() != other->noninherited_flags.overflowX()
        || noninherited_flags.overflowY() != other->noninherited_flags.overflowY())
        return true;

    // If our border widths change, then we need to layout.  Other changes to borders
    // only necessitate a repaint.
    if (borderLeftWidth() != other->borderLeftWidth()
        || borderTopWidth() != other->borderTopWidth()
        || borderBottomWidth() != other->borderBottomWidth()
        || borderRightWidth() != other->borderRightWidth())
        return true;

    // If the counter directives change, trigger a relayout to re-calculate counter values and rebuild the counter node tree.
    const CounterDirectiveMap* mapA = rareNonInheritedData->m_counterDirectives.get();
    const CounterDirectiveMap* mapB = other->rareNonInheritedData->m_counterDirectives.get();
    if (!(mapA == mapB || (mapA && mapB && *mapA == *mapB)))
        return true;

    if ((visibility() == COLLAPSE) != (other->visibility() == COLLAPSE))
        return true;

    if (rareNonInheritedData->hasOpacity() != other->rareNonInheritedData->hasOpacity()) {
        // FIXME: We would like to use SimplifiedLayout here, but we can't quite do that yet.
        // We need to make sure SimplifiedLayout can operate correctly on RenderInlines (we will need
        // to add a selfNeedsSimplifiedLayout bit in order to not get confused and taint every line).
        // In addition we need to solve the floating object issue when layers come and go. Right now
        // a full layout is necessary to keep floating object lists sane.
        return true;
    }

#if ENABLE(CSS_FILTERS)
    if (rareNonInheritedData->hasFilters() != other->rareNonInheritedData->hasFilters())
        return true;
#endif

    const QuotesData* quotesDataA = rareInheritedData->quotes.get();
    const QuotesData* quotesDataB = other->rareInheritedData->quotes.get();
    if (!(quotesDataA == quotesDataB || (quotesDataA && quotesDataB && *quotesDataA == *quotesDataB)))
        return true;

    if (position() != StaticPosition) {
        if (surround->offset != other->surround->offset) {
            // FIXME: We would like to use SimplifiedLayout for relative positioning, but we can't quite do that yet.
            // We need to make sure SimplifiedLayout can operate correctly on RenderInlines (we will need
            // to add a selfNeedsSimplifiedLayout bit in order to not get confused and taint every line).
            if (position() != AbsolutePosition)
                return true;

            // Optimize for the case where a positioned layer is moving but not changing size.
            if (!positionChangeIsMovementOnly(surround->offset, other->surround->offset, m_box->width()))
                return true;
        }
    }
    
    return false;
}

bool RenderStyle::changeRequiresPositionedLayoutOnly(const RenderStyle* other, unsigned&) const
{
    if (position() == StaticPosition)
        return false;

    if (surround->offset != other->surround->offset) {
        // Optimize for the case where a positioned layer is moving but not changing size.
        if (position() == AbsolutePosition && positionChangeIsMovementOnly(surround->offset, other->surround->offset, m_box->width()))
            return true;
    }
    
    return false;
}

bool RenderStyle::changeRequiresLayerRepaint(const RenderStyle* other, unsigned& changedContextSensitiveProperties) const
{
    // StyleResolver has ensured that zIndex is non-auto only if it's applicable.
    if (m_box->zIndex() != other->m_box->zIndex() || m_box->hasAutoZIndex() != other->m_box->hasAutoZIndex())
        return true;

    if (position() != StaticPosition) {
        if (visual->clip != other->visual->clip
            || visual->hasClip != other->visual->hasClip)
            return true;
    }

#if ENABLE(CSS_COMPOSITING)
    if (rareNonInheritedData->m_effectiveBlendMode != other->rareNonInheritedData->m_effectiveBlendMode)
        return true;
#endif

    if (rareNonInheritedData->opacity != other->rareNonInheritedData->opacity) {
        changedContextSensitiveProperties |= ContextSensitivePropertyOpacity;
        // Don't return; keep looking for another change.
    }

#if ENABLE(CSS_FILTERS)
    if (rareNonInheritedData->m_filter.get() != other->rareNonInheritedData->m_filter.get()
        && *rareNonInheritedData->m_filter.get() != *other->rareNonInheritedData->m_filter.get()) {
        changedContextSensitiveProperties |= ContextSensitivePropertyFilter;
        // Don't return; keep looking for another change.
    }
#endif

    if (rareNonInheritedData->m_mask != other->rareNonInheritedData->m_mask
        || rareNonInheritedData->m_maskBoxImage != other->rareNonInheritedData->m_maskBoxImage)
        return true;

    return false;
}

bool RenderStyle::changeRequiresRepaint(const RenderStyle* other, unsigned&) const
{
    if (inherited_flags._visibility != other->inherited_flags._visibility
        || inherited_flags.m_printColorAdjust != other->inherited_flags.m_printColorAdjust
        || inherited_flags._insideLink != other->inherited_flags._insideLink
        || surround->border != other->surround->border
        || !m_background->isEquivalentForPainting(*other->m_background)
        || rareInheritedData->userModify != other->rareInheritedData->userModify
        || rareInheritedData->userSelect != other->rareInheritedData->userSelect
        || rareNonInheritedData->userDrag != other->rareNonInheritedData->userDrag
        || rareNonInheritedData->m_borderFit != other->rareNonInheritedData->m_borderFit
        || rareNonInheritedData->m_objectFit != other->rareNonInheritedData->m_objectFit
        || rareInheritedData->m_imageRendering != other->rareInheritedData->m_imageRendering)
        return true;

#if ENABLE(CSS_SHAPES)
    if (rareNonInheritedData->m_shapeOutside != other->rareNonInheritedData->m_shapeOutside)
        return true;
#endif

    if (rareNonInheritedData->m_clipPath != other->rareNonInheritedData->m_clipPath)
        return true;

    return false;
}

bool RenderStyle::changeRequiresRepaintIfTextOrBorderOrOutline(const RenderStyle* other, unsigned&) const
{
    if (inherited->color != other->inherited->color
        || inherited_flags._text_decorations != other->inherited_flags._text_decorations
        || visual->textDecoration != other->visual->textDecoration
        || rareNonInheritedData->m_textDecorationStyle != other->rareNonInheritedData->m_textDecorationStyle
        || rareNonInheritedData->m_textDecorationColor != other->rareNonInheritedData->m_textDecorationColor
        || rareInheritedData->m_textDecorationSkip != other->rareInheritedData->m_textDecorationSkip
        || rareInheritedData->textFillColor != other->rareInheritedData->textFillColor
        || rareInheritedData->textStrokeColor != other->rareInheritedData->textStrokeColor
        || rareInheritedData->textEmphasisColor != other->rareInheritedData->textEmphasisColor
        || rareInheritedData->textEmphasisFill != other->rareInheritedData->textEmphasisFill)
        return true;

    return false;
}

bool RenderStyle::changeRequiresRecompositeLayer(const RenderStyle* other, unsigned&) const
{
    if (rareNonInheritedData.get() != other->rareNonInheritedData.get()) {
        if (rareNonInheritedData->m_transformStyle3D != other->rareNonInheritedData->m_transformStyle3D
            || rareNonInheritedData->m_backfaceVisibility != other->rareNonInheritedData->m_backfaceVisibility
            || rareNonInheritedData->m_perspective != other->rareNonInheritedData->m_perspective
            || rareNonInheritedData->m_perspectiveOriginX != other->rareNonInheritedData->m_perspectiveOriginX
            || rareNonInheritedData->m_perspectiveOriginY != other->rareNonInheritedData->m_perspectiveOriginY)
            return true;
    }

    return false;
}

StyleDifference RenderStyle::diff(const RenderStyle* other, unsigned& changedContextSensitiveProperties) const
{
    changedContextSensitiveProperties = ContextSensitivePropertyNone;

    StyleDifference svgChange = StyleDifferenceEqual;
    if (m_svgStyle != other->m_svgStyle) {
        svgChange = m_svgStyle->diff(other->m_svgStyle.get());
        if (svgChange == StyleDifferenceLayout)
            return svgChange;
    }

    if (changeRequiresLayout(other, changedContextSensitiveProperties))
        return StyleDifferenceLayout;

    // SVGRenderStyle::diff() might have returned StyleDifferenceRepaint, eg. if fill changes.
    // If eg. the font-size changed at the same time, we're not allowed to return StyleDifferenceRepaint,
    // but have to return StyleDifferenceLayout, that's why  this if branch comes after all branches
    // that are relevant for SVG and might return StyleDifferenceLayout.
    if (svgChange != StyleDifferenceEqual)
        return svgChange;

    if (changeRequiresPositionedLayoutOnly(other, changedContextSensitiveProperties))
        return StyleDifferenceLayoutPositionedMovementOnly;

    if (changeRequiresLayerRepaint(other, changedContextSensitiveProperties))
        return StyleDifferenceRepaintLayer;

    if (changeRequiresRepaint(other, changedContextSensitiveProperties))
        return StyleDifferenceRepaint;

    if (changeRequiresRecompositeLayer(other, changedContextSensitiveProperties))
        return StyleDifferenceRecompositeLayer;

    if (changeRequiresRepaintIfTextOrBorderOrOutline(other, changedContextSensitiveProperties))
        return StyleDifferenceRepaintIfTextOrBorderOrOutline;

    // Cursors are not checked, since they will be set appropriately in response to mouse events,
    // so they don't need to cause any repaint or layout.

    // Animations don't need to be checked either.  We always set the new style on the RenderObject, so we will get a chance to fire off
    // the resulting transition properly.
    return StyleDifferenceEqual;
}

bool RenderStyle::diffRequiresRepaint(const RenderStyle* style) const
{
    unsigned changedContextSensitiveProperties = 0;
    return changeRequiresRepaint(style, changedContextSensitiveProperties);
}

void RenderStyle::setClip(Length top, Length right, Length bottom, Length left)
{
    StyleVisualData* data = visual.access();
    data->clip.m_top = top;
    data->clip.m_right = right;
    data->clip.m_bottom = bottom;
    data->clip.m_left = left;
}

void RenderStyle::addCursor(PassRefPtr<StyleImage> image, const IntPoint& hotSpot)
{
    if (!rareInheritedData.access()->cursorData)
        rareInheritedData.access()->cursorData = CursorList::create();
    rareInheritedData.access()->cursorData->append(CursorData(image, hotSpot));
}

void RenderStyle::setCursorList(PassRefPtr<CursorList> other)
{
    rareInheritedData.access()->cursorData = other;
}

void RenderStyle::setQuotes(PassRefPtr<QuotesData> q)
{
    if (rareInheritedData->quotes == q || (rareInheritedData->quotes && q && *rareInheritedData->quotes == *q))
        return;

    rareInheritedData.access()->quotes = q;
}

void RenderStyle::clearCursorList()
{
    if (rareInheritedData->cursorData)
        rareInheritedData.access()->cursorData = 0;
}

void RenderStyle::clearContent()
{
    if (rareNonInheritedData->m_content)
        rareNonInheritedData.access()->m_content = nullptr;
}

void RenderStyle::appendContent(std::unique_ptr<ContentData> contentData)
{
    auto& content = rareNonInheritedData.access()->m_content;
    ContentData* lastContent = content.get();
    while (lastContent && lastContent->next())
        lastContent = lastContent->next();

    if (lastContent)
        lastContent->setNext(WTF::move(contentData));
    else
        content = WTF::move(contentData);
}

void RenderStyle::setContent(PassRefPtr<StyleImage> image, bool add)
{
    if (!image)
        return;
        
    if (add) {
        appendContent(std::make_unique<ImageContentData>(image));
        return;
    }

    rareNonInheritedData.access()->m_content = std::make_unique<ImageContentData>(image);
    if (!rareNonInheritedData.access()->m_altText.isNull())
        rareNonInheritedData.access()->m_content->setAltText(rareNonInheritedData.access()->m_altText);
}

void RenderStyle::setContent(const String& string, bool add)
{
    auto& content = rareNonInheritedData.access()->m_content;
    if (add) {
        ContentData* lastContent = content.get();
        while (lastContent && lastContent->next())
            lastContent = lastContent->next();

        if (lastContent) {
            // We attempt to merge with the last ContentData if possible.
            if (lastContent->isText()) {
                TextContentData* textContent = toTextContentData(lastContent);
                textContent->setText(textContent->text() + string);
            } else
                lastContent->setNext(std::make_unique<TextContentData>(string));

            if (!rareNonInheritedData.access()->m_altText.isNull())
                lastContent->setAltText(rareNonInheritedData.access()->m_altText);
            return;
        }
    }

    content = std::make_unique<TextContentData>(string);
    if (!rareNonInheritedData.access()->m_altText.isNull())
        content->setAltText(rareNonInheritedData.access()->m_altText);
}

void RenderStyle::setContent(std::unique_ptr<CounterContent> counter, bool add)
{
    if (!counter)
        return;

    if (add) {
        appendContent(std::make_unique<CounterContentData>(WTF::move(counter)));
        return;
    }

    rareNonInheritedData.access()->m_content = std::make_unique<CounterContentData>(WTF::move(counter));
}

void RenderStyle::setContent(QuoteType quote, bool add)
{
    if (add) {
        appendContent(std::make_unique<QuoteContentData>(quote));
        return;
    }

    rareNonInheritedData.access()->m_content = std::make_unique<QuoteContentData>(quote);
}

void RenderStyle::setContentAltText(const String& string)
{
    rareNonInheritedData.access()->m_altText = string;
    
    if (rareNonInheritedData.access()->m_content)
        rareNonInheritedData.access()->m_content->setAltText(string);
}

const String& RenderStyle::contentAltText() const
{
    return rareNonInheritedData->m_altText;
}

static inline bool requireTransformOrigin(const Vector<RefPtr<TransformOperation>>& transformOperations, RenderStyle::ApplyTransformOrigin applyOrigin)
{
    // transform-origin brackets the transform with translate operations.
    // Optimize for the case where the only transform is a translation, since the transform-origin is irrelevant
    // in that case.
    if (applyOrigin != RenderStyle::IncludeTransformOrigin)
        return false;

    for (auto& operation : transformOperations) {
        TransformOperation::OperationType type = operation->type();
        if (type != TransformOperation::TRANSLATE_X
            && type != TransformOperation::TRANSLATE_Y
            && type != TransformOperation::TRANSLATE 
            && type != TransformOperation::TRANSLATE_Z
            && type != TransformOperation::TRANSLATE_3D)
            return true;
    }

    return false;
}

void RenderStyle::applyTransform(TransformationMatrix& transform, const FloatRect& boundingBox, ApplyTransformOrigin applyOrigin) const
{
    auto& operations = rareNonInheritedData->m_transform->m_operations.operations();
    bool applyTransformOrigin = requireTransformOrigin(operations, applyOrigin);

    float offsetX = transformOriginX().isPercentNotCalculated() ? boundingBox.x() : 0;
    float offsetY = transformOriginY().isPercentNotCalculated() ? boundingBox.y() : 0;

    if (applyTransformOrigin) {
        transform.translate3d(floatValueForLength(transformOriginX(), boundingBox.width()) + offsetX,
                              floatValueForLength(transformOriginY(), boundingBox.height()) + offsetY,
                              transformOriginZ());
    }

    for (auto& operation : operations)
        operation->apply(transform, boundingBox.size());

    if (applyTransformOrigin) {
        transform.translate3d(-floatValueForLength(transformOriginX(), boundingBox.width()) - offsetX,
                              -floatValueForLength(transformOriginY(), boundingBox.height()) - offsetY,
                              -transformOriginZ());
    }
}

void RenderStyle::setPageScaleTransform(float scale)
{
    if (scale == 1)
        return;
    TransformOperations transform;
    transform.operations().append(ScaleTransformOperation::create(scale, scale, ScaleTransformOperation::SCALE));
    setTransform(transform);
    setTransformOriginX(Length(0, Fixed));
    setTransformOriginY(Length(0, Fixed));
}

void RenderStyle::setTextShadow(std::unique_ptr<ShadowData> shadowData, bool add)
{
    ASSERT(!shadowData || (!shadowData->spread() && shadowData->style() == Normal));

    StyleRareInheritedData* rareData = rareInheritedData.access();
    if (!add) {
        rareData->textShadow = WTF::move(shadowData);
        return;
    }

    shadowData->setNext(WTF::move(rareData->textShadow));
    rareData->textShadow = WTF::move(shadowData);
}

void RenderStyle::setBoxShadow(std::unique_ptr<ShadowData> shadowData, bool add)
{
    StyleRareNonInheritedData* rareData = rareNonInheritedData.access();
    if (!add) {
        rareData->m_boxShadow = WTF::move(shadowData);
        return;
    }

    shadowData->setNext(WTF::move(rareData->m_boxShadow));
    rareData->m_boxShadow = WTF::move(shadowData);
}

static RoundedRect::Radii calcRadiiFor(const BorderData& border, const LayoutSize& size)
{
    return RoundedRect::Radii(
        LayoutSize(valueForLength(border.topLeft().width(), size.width()),
            valueForLength(border.topLeft().height(), size.height())),
        LayoutSize(valueForLength(border.topRight().width(), size.width()),
            valueForLength(border.topRight().height(), size.height())),
        LayoutSize(valueForLength(border.bottomLeft().width(), size.width()),
            valueForLength(border.bottomLeft().height(), size.height())),
        LayoutSize(valueForLength(border.bottomRight().width(), size.width()),
            valueForLength(border.bottomRight().height(), size.height())));
}

StyleImage* RenderStyle::listStyleImage() const { return rareInheritedData->listStyleImage.get(); }
void RenderStyle::setListStyleImage(PassRefPtr<StyleImage> v)
{
    if (rareInheritedData->listStyleImage != v)
        rareInheritedData.access()->listStyleImage = v;
}

Color RenderStyle::color() const { return inherited->color; }
Color RenderStyle::visitedLinkColor() const { return inherited->visitedLinkColor; }
void RenderStyle::setColor(const Color& v) { SET_VAR(inherited, color, v); }
void RenderStyle::setVisitedLinkColor(const Color& v) { SET_VAR(inherited, visitedLinkColor, v); }

short RenderStyle::horizontalBorderSpacing() const { return inherited->horizontal_border_spacing; }
short RenderStyle::verticalBorderSpacing() const { return inherited->vertical_border_spacing; }
void RenderStyle::setHorizontalBorderSpacing(short v) { SET_VAR(inherited, horizontal_border_spacing, v); }
void RenderStyle::setVerticalBorderSpacing(short v) { SET_VAR(inherited, vertical_border_spacing, v); }

RoundedRect RenderStyle::getRoundedBorderFor(const LayoutRect& borderRect, bool includeLogicalLeftEdge, bool includeLogicalRightEdge) const
{
    RoundedRect roundedRect(borderRect);
    if (hasBorderRadius()) {
        RoundedRect::Radii radii = calcRadiiFor(surround->border, borderRect.size());
        radii.scale(calcBorderRadiiConstraintScaleFor(borderRect, radii));
        roundedRect.includeLogicalEdges(radii, isHorizontalWritingMode(), includeLogicalLeftEdge, includeLogicalRightEdge);
    }
    return roundedRect;
}

RoundedRect RenderStyle::getRoundedInnerBorderFor(const LayoutRect& borderRect, bool includeLogicalLeftEdge, bool includeLogicalRightEdge) const
{
    bool horizontal = isHorizontalWritingMode();

    LayoutUnit leftWidth = (!horizontal || includeLogicalLeftEdge) ? borderLeftWidth() : 0;
    LayoutUnit rightWidth = (!horizontal || includeLogicalRightEdge) ? borderRightWidth() : 0;
    LayoutUnit topWidth = (horizontal || includeLogicalLeftEdge) ? borderTopWidth() : 0;
    LayoutUnit bottomWidth = (horizontal || includeLogicalRightEdge) ? borderBottomWidth() : 0;

    return getRoundedInnerBorderFor(borderRect, topWidth, bottomWidth, leftWidth, rightWidth, includeLogicalLeftEdge, includeLogicalRightEdge);
}

RoundedRect RenderStyle::getRoundedInnerBorderFor(const LayoutRect& borderRect, LayoutUnit topWidth, LayoutUnit bottomWidth,
    LayoutUnit leftWidth, LayoutUnit rightWidth, bool includeLogicalLeftEdge, bool includeLogicalRightEdge) const
{
    LayoutRect innerRect(borderRect.x() + leftWidth, 
               borderRect.y() + topWidth, 
               borderRect.width() - leftWidth - rightWidth, 
               borderRect.height() - topWidth - bottomWidth);

    RoundedRect roundedRect(innerRect);

    if (hasBorderRadius()) {
        RoundedRect::Radii radii = getRoundedBorderFor(borderRect).radii();
        radii.shrink(topWidth, bottomWidth, leftWidth, rightWidth);
        roundedRect.includeLogicalEdges(radii, isHorizontalWritingMode(), includeLogicalLeftEdge, includeLogicalRightEdge);
    }
    return roundedRect;
}

static bool allLayersAreFixed(const FillLayer* layer)
{
    bool allFixed = true;
    
    for (const FillLayer* currLayer = layer; currLayer; currLayer = currLayer->next())
        allFixed &= (currLayer->image() && currLayer->attachment() == FixedBackgroundAttachment);

    return layer && allFixed;
}

bool RenderStyle::hasEntirelyFixedBackground() const
{
    return allLayersAreFixed(backgroundLayers());
}

const CounterDirectiveMap* RenderStyle::counterDirectives() const
{
    return rareNonInheritedData->m_counterDirectives.get();
}

CounterDirectiveMap& RenderStyle::accessCounterDirectives()
{
    auto& map = rareNonInheritedData.access()->m_counterDirectives;
    if (!map)
        map = std::make_unique<CounterDirectiveMap>();
    return *map;
}

const CounterDirectives RenderStyle::getCounterDirectives(const AtomicString& identifier) const
{
    if (const CounterDirectiveMap* directives = counterDirectives())
        return directives->get(identifier);
    return CounterDirectives();
}

const AtomicString& RenderStyle::hyphenString() const
{
    ASSERT(hyphens() != HyphensNone);

    const AtomicString& hyphenationString = rareInheritedData.get()->hyphenationString;
    if (!hyphenationString.isNull())
        return hyphenationString;

    // FIXME: This should depend on locale.
    DEPRECATED_DEFINE_STATIC_LOCAL(AtomicString, hyphenMinusString, (&hyphenMinus, 1));
    DEPRECATED_DEFINE_STATIC_LOCAL(AtomicString, hyphenString, (&hyphen, 1));
    return font().primaryFontHasGlyphForCharacter(hyphen) ? hyphenString : hyphenMinusString;
}

const AtomicString& RenderStyle::textEmphasisMarkString() const
{
    switch (textEmphasisMark()) {
    case TextEmphasisMarkNone:
        return nullAtom;
    case TextEmphasisMarkCustom:
        return textEmphasisCustomMark();
    case TextEmphasisMarkDot: {
        DEPRECATED_DEFINE_STATIC_LOCAL(AtomicString, filledDotString, (&bullet, 1));
        DEPRECATED_DEFINE_STATIC_LOCAL(AtomicString, openDotString, (&whiteBullet, 1));
        return textEmphasisFill() == TextEmphasisFillFilled ? filledDotString : openDotString;
    }
    case TextEmphasisMarkCircle: {
        DEPRECATED_DEFINE_STATIC_LOCAL(AtomicString, filledCircleString, (&blackCircle, 1));
        DEPRECATED_DEFINE_STATIC_LOCAL(AtomicString, openCircleString, (&whiteCircle, 1));
        return textEmphasisFill() == TextEmphasisFillFilled ? filledCircleString : openCircleString;
    }
    case TextEmphasisMarkDoubleCircle: {
        DEPRECATED_DEFINE_STATIC_LOCAL(AtomicString, filledDoubleCircleString, (&fisheye, 1));
        DEPRECATED_DEFINE_STATIC_LOCAL(AtomicString, openDoubleCircleString, (&bullseye, 1));
        return textEmphasisFill() == TextEmphasisFillFilled ? filledDoubleCircleString : openDoubleCircleString;
    }
    case TextEmphasisMarkTriangle: {
        DEPRECATED_DEFINE_STATIC_LOCAL(AtomicString, filledTriangleString, (&blackUpPointingTriangle, 1));
        DEPRECATED_DEFINE_STATIC_LOCAL(AtomicString, openTriangleString, (&whiteUpPointingTriangle, 1));
        return textEmphasisFill() == TextEmphasisFillFilled ? filledTriangleString : openTriangleString;
    }
    case TextEmphasisMarkSesame: {
        DEPRECATED_DEFINE_STATIC_LOCAL(AtomicString, filledSesameString, (&sesameDot, 1));
        DEPRECATED_DEFINE_STATIC_LOCAL(AtomicString, openSesameString, (&whiteSesameDot, 1));
        return textEmphasisFill() == TextEmphasisFillFilled ? filledSesameString : openSesameString;
    }
    case TextEmphasisMarkAuto:
        ASSERT_NOT_REACHED();
        return nullAtom;
    }

    ASSERT_NOT_REACHED();
    return nullAtom;
}

#if ENABLE(DASHBOARD_SUPPORT)
const Vector<StyleDashboardRegion>& RenderStyle::initialDashboardRegions()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Vector<StyleDashboardRegion>, emptyList, ());
    return emptyList;
}

const Vector<StyleDashboardRegion>& RenderStyle::noneDashboardRegions()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Vector<StyleDashboardRegion>, noneList, ());
    static bool noneListInitialized = false;

    if (!noneListInitialized) {
        StyleDashboardRegion region;
        region.label = "";
        region.offset.m_top  = Length();
        region.offset.m_right = Length();
        region.offset.m_bottom = Length();
        region.offset.m_left = Length();
        region.type = StyleDashboardRegion::None;
        noneList.append(region);
        noneListInitialized = true;
    }
    return noneList;
}
#endif

void RenderStyle::adjustAnimations()
{
    AnimationList* animationList = rareNonInheritedData->m_animations.get();
    if (!animationList)
        return;

    // Get rid of empty animations and anything beyond them
    for (size_t i = 0; i < animationList->size(); ++i) {
        if (animationList->animation(i).isEmpty()) {
            animationList->resize(i);
            break;
        }
    }

    if (animationList->isEmpty()) {
        clearAnimations();
        return;
    }

    // Repeat patterns into layers that don't have some properties set.
    animationList->fillUnsetProperties();
}

void RenderStyle::adjustTransitions()
{
    AnimationList* transitionList = rareNonInheritedData->m_transitions.get();
    if (!transitionList)
        return;

    // Get rid of empty transitions and anything beyond them
    for (size_t i = 0; i < transitionList->size(); ++i) {
        if (transitionList->animation(i).isEmpty()) {
            transitionList->resize(i);
            break;
        }
    }

    if (transitionList->isEmpty()) {
        clearTransitions();
        return;
    }

    // Repeat patterns into layers that don't have some properties set.
    transitionList->fillUnsetProperties();

    // Make sure there are no duplicate properties. This is an O(n^2) algorithm
    // but the lists tend to be very short, so it is probably ok
    for (size_t i = 0; i < transitionList->size(); ++i) {
        for (size_t j = i+1; j < transitionList->size(); ++j) {
            if (transitionList->animation(i).property() == transitionList->animation(j).property()) {
                // toss i
                transitionList->remove(i);
                j = i;
            }
        }
    }
}

AnimationList* RenderStyle::accessAnimations()
{
    if (!rareNonInheritedData.access()->m_animations)
        rareNonInheritedData.access()->m_animations = std::make_unique<AnimationList>();
    return rareNonInheritedData->m_animations.get();
}

AnimationList* RenderStyle::accessTransitions()
{
    if (!rareNonInheritedData.access()->m_transitions)
        rareNonInheritedData.access()->m_transitions = std::make_unique<AnimationList>();
    return rareNonInheritedData->m_transitions.get();
}

const Animation* RenderStyle::transitionForProperty(CSSPropertyID property) const
{
    if (transitions()) {
        for (size_t i = 0; i < transitions()->size(); ++i) {
            const Animation& p = transitions()->animation(i);
            if (p.animationMode() == Animation::AnimateAll || p.property() == property) {
                return &p;
            }
        }
    }
    return 0;
}

const Font& RenderStyle::font() const { return inherited->font; }
const FontMetrics& RenderStyle::fontMetrics() const { return inherited->font.fontMetrics(); }
const FontDescription& RenderStyle::fontDescription() const { return inherited->font.fontDescription(); }
float RenderStyle::specifiedFontSize() const { return fontDescription().specifiedSize(); }
float RenderStyle::computedFontSize() const { return fontDescription().computedSize(); }
int RenderStyle::fontSize() const { return inherited->font.pixelSize(); }

const Length& RenderStyle::wordSpacing() const { return rareInheritedData->wordSpacing; }
float RenderStyle::letterSpacing() const { return inherited->font.letterSpacing(); }

bool RenderStyle::setFontDescription(const FontDescription& v)
{
    if (inherited->font.fontDescription() != v) {
        inherited.access()->font = Font(v, inherited->font.letterSpacing(), inherited->font.wordSpacing());
        return true;
    }
    return false;
}

#if ENABLE(IOS_TEXT_AUTOSIZING)
const Length& RenderStyle::specifiedLineHeight() const { return inherited->specifiedLineHeight; }
void RenderStyle::setSpecifiedLineHeight(Length v) { SET_VAR(inherited, specifiedLineHeight, v); }
#else
const Length& RenderStyle::specifiedLineHeight() const { return inherited->line_height; }
#endif

Length RenderStyle::lineHeight() const
{
    const Length& lh = inherited->line_height;
#if ENABLE(TEXT_AUTOSIZING)
    // Unlike fontDescription().computedSize() and hence fontSize(), this is
    // recalculated on demand as we only store the specified line height.
    // FIXME: Should consider scaling the fixed part of any calc expressions
    // too, though this involves messily poking into CalcExpressionLength.
    float multiplier = textAutosizingMultiplier();
    if (multiplier > 1 && lh.isFixed())
        return Length(TextAutosizer::computeAutosizedFontSize(lh.value(), multiplier), Fixed);
#endif
    return lh;
}
void RenderStyle::setLineHeight(Length specifiedLineHeight) { SET_VAR(inherited, line_height, specifiedLineHeight); }

int RenderStyle::computedLineHeight() const
{
    const Length& lh = lineHeight();

    // Negative value means the line height is not set. Use the font's built-in spacing.
    if (lh.isNegative())
        return fontMetrics().lineSpacing();

    if (lh.isPercent())
        return minimumValueForLength(lh, fontSize());

    return lh.value();
}

void RenderStyle::setWordSpacing(Length value)
{
    float fontWordSpacing;
    switch (value.type()) {
    case Auto:
        fontWordSpacing = 0;
        break;
    case Percent:
        fontWordSpacing = value.percent() * font().spaceWidth() / 100;
        break;
    case Fixed:
        fontWordSpacing = value.value();
        break;
    default:
        ASSERT_NOT_REACHED();
        fontWordSpacing = 0;
        break;
    }
    inherited.access()->font.setWordSpacing(fontWordSpacing);
    rareInheritedData.access()->wordSpacing = WTF::move(value);
}

void RenderStyle::setLetterSpacing(float v) { inherited.access()->font.setLetterSpacing(v); }

void RenderStyle::setFontSize(float size)
{
    // size must be specifiedSize if Text Autosizing is enabled, but computedSize if text
    // zoom is enabled (if neither is enabled it's irrelevant as they're probably the same).

    ASSERT(std::isfinite(size));
    if (!std::isfinite(size) || size < 0)
        size = 0;
    else
        size = std::min(maximumAllowedFontSize, size);

    FontSelector* currentFontSelector = font().fontSelector();
    FontDescription desc(fontDescription());
    desc.setSpecifiedSize(size);
    desc.setComputedSize(size);

#if ENABLE(TEXT_AUTOSIZING)
    float multiplier = textAutosizingMultiplier();
    if (multiplier > 1) {
        float autosizedFontSize = TextAutosizer::computeAutosizedFontSize(size, multiplier);
        desc.setComputedSize(min(maximumAllowedFontSize, autosizedFontSize));
    }
#endif

    setFontDescription(desc);
    font().update(currentFontSelector);
}

void RenderStyle::getShadowExtent(const ShadowData* shadow, LayoutUnit &top, LayoutUnit &right, LayoutUnit &bottom, LayoutUnit &left) const
{
    top = 0;
    right = 0;
    bottom = 0;
    left = 0;

    for ( ; shadow; shadow = shadow->next()) {
        if (shadow->style() == Inset)
            continue;

        int extentAndSpread = shadow->paintingExtent() + shadow->spread();
        top = std::min<LayoutUnit>(top, shadow->y() - extentAndSpread);
        right = std::max<LayoutUnit>(right, shadow->x() + extentAndSpread);
        bottom = std::max<LayoutUnit>(bottom, shadow->y() + extentAndSpread);
        left = std::min<LayoutUnit>(left, shadow->x() - extentAndSpread);
    }
}

LayoutBoxExtent RenderStyle::getShadowInsetExtent(const ShadowData* shadow) const
{
    LayoutUnit top = 0;
    LayoutUnit right = 0;
    LayoutUnit bottom = 0;
    LayoutUnit left = 0;

    for ( ; shadow; shadow = shadow->next()) {
        if (shadow->style() == Normal)
            continue;

        int extentAndSpread = shadow->paintingExtent() + shadow->spread();
        top = std::max<LayoutUnit>(top, shadow->y() + extentAndSpread);
        right = std::min<LayoutUnit>(right, shadow->x() - extentAndSpread);
        bottom = std::min<LayoutUnit>(bottom, shadow->y() - extentAndSpread);
        left = std::max<LayoutUnit>(left, shadow->x() + extentAndSpread);
    }

    return LayoutBoxExtent(top, right, bottom, left);
}

void RenderStyle::getShadowHorizontalExtent(const ShadowData* shadow, LayoutUnit &left, LayoutUnit &right) const
{
    left = 0;
    right = 0;

    for ( ; shadow; shadow = shadow->next()) {
        if (shadow->style() == Inset)
            continue;

        int extentAndSpread = shadow->paintingExtent() + shadow->spread();
        left = std::min<LayoutUnit>(left, shadow->x() - extentAndSpread);
        right = std::max<LayoutUnit>(right, shadow->x() + extentAndSpread);
    }
}

void RenderStyle::getShadowVerticalExtent(const ShadowData* shadow, LayoutUnit &top, LayoutUnit &bottom) const
{
    top = 0;
    bottom = 0;

    for ( ; shadow; shadow = shadow->next()) {
        if (shadow->style() == Inset)
            continue;

        int extentAndSpread = shadow->paintingExtent() + shadow->spread();
        top = std::min<LayoutUnit>(top, shadow->y() - extentAndSpread);
        bottom = std::max<LayoutUnit>(bottom, shadow->y() + extentAndSpread);
    }
}

Color RenderStyle::colorIncludingFallback(int colorProperty, bool visitedLink) const
{
    Color result;
    EBorderStyle borderStyle = BNONE;
    switch (colorProperty) {
    case CSSPropertyBackgroundColor:
        return visitedLink ? visitedLinkBackgroundColor() : backgroundColor(); // Background color doesn't fall back.
    case CSSPropertyBorderLeftColor:
        result = visitedLink ? visitedLinkBorderLeftColor() : borderLeftColor();
        borderStyle = borderLeftStyle();
        break;
    case CSSPropertyBorderRightColor:
        result = visitedLink ? visitedLinkBorderRightColor() : borderRightColor();
        borderStyle = borderRightStyle();
        break;
    case CSSPropertyBorderTopColor:
        result = visitedLink ? visitedLinkBorderTopColor() : borderTopColor();
        borderStyle = borderTopStyle();
        break;
    case CSSPropertyBorderBottomColor:
        result = visitedLink ? visitedLinkBorderBottomColor() : borderBottomColor();
        borderStyle = borderBottomStyle();
        break;
    case CSSPropertyColor:
        result = visitedLink ? visitedLinkColor() : color();
        break;
    case CSSPropertyOutlineColor:
        result = visitedLink ? visitedLinkOutlineColor() : outlineColor();
        break;
    case CSSPropertyWebkitColumnRuleColor:
        result = visitedLink ? visitedLinkColumnRuleColor() : columnRuleColor();
        break;
    case CSSPropertyWebkitTextDecorationColor:
        // Text decoration color fallback is handled in RenderObject::decorationColor.
        return visitedLink ? visitedLinkTextDecorationColor() : textDecorationColor();
    case CSSPropertyWebkitTextEmphasisColor:
        result = visitedLink ? visitedLinkTextEmphasisColor() : textEmphasisColor();
        break;
    case CSSPropertyWebkitTextFillColor:
        result = visitedLink ? visitedLinkTextFillColor() : textFillColor();
        break;
    case CSSPropertyWebkitTextStrokeColor:
        result = visitedLink ? visitedLinkTextStrokeColor() : textStrokeColor();
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    if (!result.isValid()) {
        if (!visitedLink && (borderStyle == INSET || borderStyle == OUTSET || borderStyle == RIDGE || borderStyle == GROOVE))
            result.setRGB(238, 238, 238);
        else
            result = visitedLink ? visitedLinkColor() : color();
    }
    return result;
}

Color RenderStyle::visitedDependentColor(int colorProperty) const
{
    Color unvisitedColor = colorIncludingFallback(colorProperty, false);
    if (insideLink() != InsideVisitedLink)
        return unvisitedColor;

    Color visitedColor = colorIncludingFallback(colorProperty, true);

    // Text decoration color validity is preserved (checked in RenderObject::decorationColor).
    if (colorProperty == CSSPropertyWebkitTextDecorationColor)
        return visitedColor;

    // FIXME: Technically someone could explicitly specify the color transparent, but for now we'll just
    // assume that if the background color is transparent that it wasn't set. Note that it's weird that
    // we're returning unvisited info for a visited link, but given our restriction that the alpha values
    // have to match, it makes more sense to return the unvisited background color if specified than it
    // does to return black. This behavior matches what Firefox 4 does as well.
    if (colorProperty == CSSPropertyBackgroundColor && visitedColor == Color::transparent)
        return unvisitedColor;

    // Take the alpha from the unvisited color, but get the RGB values from the visited color.
    return Color(visitedColor.red(), visitedColor.green(), visitedColor.blue(), unvisitedColor.alpha());
}

const BorderValue& RenderStyle::borderBefore() const
{
    switch (writingMode()) {
    case TopToBottomWritingMode:
        return borderTop();
    case BottomToTopWritingMode:
        return borderBottom();
    case LeftToRightWritingMode:
        return borderLeft();
    case RightToLeftWritingMode:
        return borderRight();
    }
    ASSERT_NOT_REACHED();
    return borderTop();
}

const BorderValue& RenderStyle::borderAfter() const
{
    switch (writingMode()) {
    case TopToBottomWritingMode:
        return borderBottom();
    case BottomToTopWritingMode:
        return borderTop();
    case LeftToRightWritingMode:
        return borderRight();
    case RightToLeftWritingMode:
        return borderLeft();
    }
    ASSERT_NOT_REACHED();
    return borderBottom();
}

const BorderValue& RenderStyle::borderStart() const
{
    if (isHorizontalWritingMode())
        return isLeftToRightDirection() ? borderLeft() : borderRight();
    return isLeftToRightDirection() ? borderTop() : borderBottom();
}

const BorderValue& RenderStyle::borderEnd() const
{
    if (isHorizontalWritingMode())
        return isLeftToRightDirection() ? borderRight() : borderLeft();
    return isLeftToRightDirection() ? borderBottom() : borderTop();
}

float RenderStyle::borderBeforeWidth() const
{
    switch (writingMode()) {
    case TopToBottomWritingMode:
        return borderTopWidth();
    case BottomToTopWritingMode:
        return borderBottomWidth();
    case LeftToRightWritingMode:
        return borderLeftWidth();
    case RightToLeftWritingMode:
        return borderRightWidth();
    }
    ASSERT_NOT_REACHED();
    return borderTopWidth();
}

float RenderStyle::borderAfterWidth() const
{
    switch (writingMode()) {
    case TopToBottomWritingMode:
        return borderBottomWidth();
    case BottomToTopWritingMode:
        return borderTopWidth();
    case LeftToRightWritingMode:
        return borderRightWidth();
    case RightToLeftWritingMode:
        return borderLeftWidth();
    }
    ASSERT_NOT_REACHED();
    return borderBottomWidth();
}

float RenderStyle::borderStartWidth() const
{
    if (isHorizontalWritingMode())
        return isLeftToRightDirection() ? borderLeftWidth() : borderRightWidth();
    return isLeftToRightDirection() ? borderTopWidth() : borderBottomWidth();
}

float RenderStyle::borderEndWidth() const
{
    if (isHorizontalWritingMode())
        return isLeftToRightDirection() ? borderRightWidth() : borderLeftWidth();
    return isLeftToRightDirection() ? borderBottomWidth() : borderTopWidth();
}

void RenderStyle::setMarginStart(Length margin)
{
    if (isHorizontalWritingMode()) {
        if (isLeftToRightDirection())
            setMarginLeft(margin);
        else
            setMarginRight(margin);
    } else {
        if (isLeftToRightDirection())
            setMarginTop(margin);
        else
            setMarginBottom(margin);
    }
}

void RenderStyle::setMarginEnd(Length margin)
{
    if (isHorizontalWritingMode()) {
        if (isLeftToRightDirection())
            setMarginRight(margin);
        else
            setMarginLeft(margin);
    } else {
        if (isLeftToRightDirection())
            setMarginBottom(margin);
        else
            setMarginTop(margin);
    }
}

TextEmphasisMark RenderStyle::textEmphasisMark() const
{
    TextEmphasisMark mark = static_cast<TextEmphasisMark>(rareInheritedData->textEmphasisMark);
    if (mark != TextEmphasisMarkAuto)
        return mark;

    if (isHorizontalWritingMode())
        return TextEmphasisMarkDot;

    return TextEmphasisMarkSesame;
}

#if ENABLE(TOUCH_EVENTS)
Color RenderStyle::initialTapHighlightColor()
{
    return RenderTheme::tapHighlightColor();
}
#endif

LayoutBoxExtent RenderStyle::imageOutsets(const NinePieceImage& image) const
{
    return LayoutBoxExtent(NinePieceImage::computeOutset(image.outset().top(), borderTopWidth()),
                           NinePieceImage::computeOutset(image.outset().right(), borderRightWidth()),
                           NinePieceImage::computeOutset(image.outset().bottom(), borderBottomWidth()),
                           NinePieceImage::computeOutset(image.outset().left(), borderLeftWidth()));
}

void RenderStyle::getFontAndGlyphOrientation(FontOrientation& fontOrientation, NonCJKGlyphOrientation& glyphOrientation)
{
    if (isHorizontalWritingMode()) {
        fontOrientation = Horizontal;
        glyphOrientation = NonCJKGlyphOrientationVerticalRight;
        return;
    }

    switch (textOrientation()) {
    case TextOrientationVerticalRight:
        fontOrientation = Vertical;
        glyphOrientation = NonCJKGlyphOrientationVerticalRight;
        return;
    case TextOrientationUpright:
        fontOrientation = Vertical;
        glyphOrientation = NonCJKGlyphOrientationUpright;
        return;
    case TextOrientationSideways:
        if (writingMode() == LeftToRightWritingMode) {
            // FIXME: This should map to sideways-left, which is not supported yet.
            fontOrientation = Vertical;
            glyphOrientation = NonCJKGlyphOrientationVerticalRight;
            return;
        }
        fontOrientation = Horizontal;
        glyphOrientation = NonCJKGlyphOrientationVerticalRight;
        return;
    case TextOrientationSidewaysRight:
        fontOrientation = Horizontal;
        glyphOrientation = NonCJKGlyphOrientationVerticalRight;
        return;
    default:
        ASSERT_NOT_REACHED();
        fontOrientation = Horizontal;
        glyphOrientation = NonCJKGlyphOrientationVerticalRight;
        return;
    }
}

void RenderStyle::setBorderImageSource(PassRefPtr<StyleImage> image)
{
    if (surround->border.m_image.image() == image.get())
        return;
    surround.access()->border.m_image.setImage(image);
}

void RenderStyle::setBorderImageSlices(LengthBox slices)
{
    if (surround->border.m_image.imageSlices() == slices)
        return;
    surround.access()->border.m_image.setImageSlices(slices);
}

void RenderStyle::setBorderImageWidth(LengthBox slices)
{
    if (surround->border.m_image.borderSlices() == slices)
        return;
    surround.access()->border.m_image.setBorderSlices(slices);
}

void RenderStyle::setBorderImageOutset(LengthBox outset)
{
    if (surround->border.m_image.outset() == outset)
        return;
    surround.access()->border.m_image.setOutset(outset);
}

void RenderStyle::setColumnStylesFromPaginationMode(const Pagination::Mode& paginationMode)
{
    if (paginationMode == Pagination::Unpaginated)
        return;
    
    setColumnFill(ColumnFillAuto);
    
    switch (paginationMode) {
    case Pagination::LeftToRightPaginated:
        setColumnAxis(HorizontalColumnAxis);
        if (isHorizontalWritingMode())
            setColumnProgression(isLeftToRightDirection() ? NormalColumnProgression : ReverseColumnProgression);
        else
            setColumnProgression(isFlippedBlocksWritingMode() ? ReverseColumnProgression : NormalColumnProgression);
        break;
    case Pagination::RightToLeftPaginated:
        setColumnAxis(HorizontalColumnAxis);
        if (isHorizontalWritingMode())
            setColumnProgression(isLeftToRightDirection() ? ReverseColumnProgression : NormalColumnProgression);
        else
            setColumnProgression(isFlippedBlocksWritingMode() ? NormalColumnProgression : ReverseColumnProgression);
        break;
    case Pagination::TopToBottomPaginated:
        setColumnAxis(VerticalColumnAxis);
        if (isHorizontalWritingMode())
            setColumnProgression(isFlippedBlocksWritingMode() ? ReverseColumnProgression : NormalColumnProgression);
        else
            setColumnProgression(isLeftToRightDirection() ? NormalColumnProgression : ReverseColumnProgression);
        break;
    case Pagination::BottomToTopPaginated:
        setColumnAxis(VerticalColumnAxis);
        if (isHorizontalWritingMode())
            setColumnProgression(isFlippedBlocksWritingMode() ? NormalColumnProgression : ReverseColumnProgression);
        else
            setColumnProgression(isLeftToRightDirection() ? ReverseColumnProgression : NormalColumnProgression);
        break;
    case Pagination::Unpaginated:
        ASSERT_NOT_REACHED();
        break;
    }
}

} // namespace WebCore
