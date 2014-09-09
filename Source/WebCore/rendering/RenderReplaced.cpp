/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2006, 2007 Apple Inc. All rights reserved.
 * Copyright (C) Research In Motion Limited 2011-2012. All rights reserved.
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
#include "RenderReplaced.h"

#include "FloatRoundedRect.h"
#include "Frame.h"
#include "GraphicsContext.h"
#include "HTMLElement.h"
#include "InlineElementBox.h"
#include "LayoutRepainter.h"
#include "Page.h"
#include "RenderBlock.h"
#include "RenderFlowThread.h"
#include "RenderImage.h"
#include "RenderLayer.h"
#include "RenderNamedFlowFragment.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "VisiblePosition.h"
#include <wtf/StackStats.h>

namespace WebCore {

const int cDefaultWidth = 300;
const int cDefaultHeight = 150;

RenderReplaced::RenderReplaced(Element& element, PassRef<RenderStyle> style)
    : RenderBox(element, WTF::move(style), RenderReplacedFlag)
    , m_intrinsicSize(cDefaultWidth, cDefaultHeight)
{
    setReplaced(true);
}

RenderReplaced::RenderReplaced(Element& element, PassRef<RenderStyle> style, const LayoutSize& intrinsicSize)
    : RenderBox(element, WTF::move(style), RenderReplacedFlag)
    , m_intrinsicSize(intrinsicSize)
{
    setReplaced(true);
}

RenderReplaced::RenderReplaced(Document& document, PassRef<RenderStyle> style, const LayoutSize& intrinsicSize)
    : RenderBox(document, WTF::move(style), RenderReplacedFlag)
    , m_intrinsicSize(intrinsicSize)
{
    setReplaced(true);
}

RenderReplaced::~RenderReplaced()
{
}

void RenderReplaced::willBeDestroyed()
{
    if (!documentBeingDestroyed() && parent())
        parent()->dirtyLinesFromChangedChild(this);

    RenderBox::willBeDestroyed();
}

void RenderReplaced::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBox::styleDidChange(diff, oldStyle);

    bool hadStyle = (oldStyle != 0);
    float oldZoom = hadStyle ? oldStyle->effectiveZoom() : RenderStyle::initialZoom();
    if (style().effectiveZoom() != oldZoom)
        intrinsicSizeChanged();
}

void RenderReplaced::layout()
{
    StackStats::LayoutCheckPoint layoutCheckPoint;
    ASSERT(needsLayout());
    
    LayoutRepainter repainter(*this, checkForRepaintDuringLayout());
    
    setHeight(minimumReplacedHeight());

    updateLogicalWidth();
    updateLogicalHeight();

    // Now that we've calculated our preferred layout, we check to see
    // if we should further constrain sizing to the intrinsic aspect ratio.
    if (style().aspectRatioType() == AspectRatioFromIntrinsic && !m_intrinsicSize.isEmpty()) {
        float aspectRatio = m_intrinsicSize.aspectRatio();
        LayoutSize frameSize = size();
        float frameAspectRatio = frameSize.aspectRatio();
        if (frameAspectRatio < aspectRatio)
            setHeight(computeReplacedLogicalHeightRespectingMinMaxHeight(frameSize.height() * frameAspectRatio / aspectRatio));
        else if (frameAspectRatio > aspectRatio)
            setWidth(computeReplacedLogicalWidthRespectingMinMaxWidth(frameSize.width() * aspectRatio / frameAspectRatio, ComputePreferred));
    }

    clearOverflow();
    addVisualEffectOverflow();
    updateLayerTransform();
    invalidateBackgroundObscurationStatus();

    repainter.repaintAfterLayout();
    clearNeedsLayout();
}

void RenderReplaced::intrinsicSizeChanged()
{
    int scaledWidth = static_cast<int>(cDefaultWidth * style().effectiveZoom());
    int scaledHeight = static_cast<int>(cDefaultHeight * style().effectiveZoom());
    m_intrinsicSize = IntSize(scaledWidth, scaledHeight);
    setNeedsLayoutAndPrefWidthsRecalc();
}

void RenderReplaced::paint(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    if (!shouldPaint(paintInfo, paintOffset))
        return;
    
    LayoutPoint adjustedPaintOffset = paintOffset + location();
    
    if (hasBoxDecorations() && (paintInfo.phase == PaintPhaseForeground || paintInfo.phase == PaintPhaseSelection)) 
        paintBoxDecorations(paintInfo, adjustedPaintOffset);
    
    if (paintInfo.phase == PaintPhaseMask) {
        paintMask(paintInfo, adjustedPaintOffset);
        return;
    }

    LayoutRect paintRect = LayoutRect(adjustedPaintOffset, size());
    if ((paintInfo.phase == PaintPhaseOutline || paintInfo.phase == PaintPhaseSelfOutline) && style().outlineWidth())
        paintOutline(paintInfo, paintRect);
    
    if (paintInfo.phase != PaintPhaseForeground && paintInfo.phase != PaintPhaseSelection && !canHaveChildren())
        return;
    
    if (!paintInfo.shouldPaintWithinRoot(*this))
        return;
    
    bool drawSelectionTint = selectionState() != SelectionNone && !document().printing();
    if (paintInfo.phase == PaintPhaseSelection) {
        if (selectionState() == SelectionNone)
            return;
        drawSelectionTint = false;
    }

    bool completelyClippedOut = false;
    if (style().hasBorderRadius()) {
        LayoutRect borderRect = LayoutRect(adjustedPaintOffset, size());

        if (borderRect.isEmpty())
            completelyClippedOut = true;
        else {
            // Push a clip if we have a border radius, since we want to round the foreground content that gets painted.
            paintInfo.context->save();
            FloatRoundedRect roundedInnerRect = FloatRoundedRect(style().getRoundedInnerBorderFor(paintRect,
                paddingTop() + borderTop(), paddingBottom() + borderBottom(), paddingLeft() + borderLeft(), paddingRight() + borderRight(), true, true));
            clipRoundedInnerRect(paintInfo.context, paintRect, roundedInnerRect);
        }
    }

    if (!completelyClippedOut) {
        paintReplaced(paintInfo, adjustedPaintOffset);

        if (style().hasBorderRadius())
            paintInfo.context->restore();
    }
        
    // The selection tint never gets clipped by border-radius rounding, since we want it to run right up to the edges of
    // surrounding content.
    if (drawSelectionTint) {
        LayoutRect selectionPaintingRect = localSelectionRect();
        selectionPaintingRect.moveBy(adjustedPaintOffset);
        paintInfo.context->fillRect(pixelSnappedIntRect(selectionPaintingRect), selectionBackgroundColor(), style().colorSpace());
    }
}

bool RenderReplaced::shouldPaint(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    if (paintInfo.phase != PaintPhaseForeground && paintInfo.phase != PaintPhaseOutline && paintInfo.phase != PaintPhaseSelfOutline 
            && paintInfo.phase != PaintPhaseSelection && paintInfo.phase != PaintPhaseMask)
        return false;

    if (!paintInfo.shouldPaintWithinRoot(*this))
        return false;
        
    // if we're invisible or haven't received a layout yet, then just bail.
    if (style().visibility() != VISIBLE)
        return false;
    
    RenderNamedFlowFragment* namedFlowFragment = currentRenderNamedFlowFragment();
    // Check our region range to make sure we need to be painting in this region.
    if (namedFlowFragment && !namedFlowFragment->flowThread()->objectShouldFragmentInFlowRegion(this, namedFlowFragment))
        return false;

    LayoutPoint adjustedPaintOffset = paintOffset + location();

    // Early exit if the element touches the edges.
    LayoutUnit top = adjustedPaintOffset.y() + visualOverflowRect().y();
    LayoutUnit bottom = adjustedPaintOffset.y() + visualOverflowRect().maxY();
    if (isSelected() && m_inlineBoxWrapper) {
        const RootInlineBox& rootBox = m_inlineBoxWrapper->root();
        LayoutUnit selTop = paintOffset.y() + rootBox.selectionTop();
        LayoutUnit selBottom = paintOffset.y() + selTop + rootBox.selectionHeight();
        top = std::min(selTop, top);
        bottom = std::max(selBottom, bottom);
    }
    
    LayoutRect localRepaintRect = paintInfo.rect;
    localRepaintRect.inflate(maximalOutlineSize(paintInfo.phase));
    if (adjustedPaintOffset.x() + visualOverflowRect().x() >= localRepaintRect.maxX() || adjustedPaintOffset.x() + visualOverflowRect().maxX() <= localRepaintRect.x())
        return false;

    if (top >= localRepaintRect.maxY() || bottom <= localRepaintRect.y())
        return false;

    return true;
}

static inline RenderBlock* firstContainingBlockWithLogicalWidth(const RenderReplaced* replaced)
{
    // We have to lookup the containing block, which has an explicit width, which must not be equal to our direct containing block.
    // If the embedded document appears _after_ we performed the initial layout, our intrinsic size is 300x150. If our containing
    // block doesn't provide an explicit width, it's set to the 300 default, coming from the initial layout run.
    RenderBlock* containingBlock = replaced->containingBlock();
    if (!containingBlock)
        return 0;

    for (; !containingBlock->isRenderView() && !containingBlock->isBody(); containingBlock = containingBlock->containingBlock()) {
        if (containingBlock->style().logicalWidth().isSpecified())
            return containingBlock;
    }

    return 0;
}

bool RenderReplaced::hasReplacedLogicalWidth() const
{
    if (style().logicalWidth().isSpecified())
        return true;

    if (style().logicalWidth().isAuto())
        return false;

    return firstContainingBlockWithLogicalWidth(this);
}

bool RenderReplaced::hasReplacedLogicalHeight() const
{
    if (style().logicalHeight().isAuto())
        return false;

    if (style().logicalHeight().isSpecified()) {
        if (hasAutoHeightOrContainingBlockWithAutoHeight())
            return false;
        return true;
    }

    return false;
}

void RenderReplaced::computeAspectRatioInformationForRenderBox(RenderBox* contentRenderer, FloatSize& constrainedSize, double& intrinsicRatio) const
{
    FloatSize intrinsicSize;
    if (contentRenderer) {
        contentRenderer->computeIntrinsicRatioInformation(intrinsicSize, intrinsicRatio);

        // Handle zoom & vertical writing modes here, as the embedded document doesn't know about them.
        intrinsicSize.scale(style().effectiveZoom());

        if (isRenderImage())
            intrinsicSize.scale(toRenderImage(this)->imageDevicePixelRatio());

        // Update our intrinsic size to match what the content renderer has computed, so that when we
        // constrain the size below, the correct intrinsic size will be obtained for comparison against
        // min and max widths.
        if (intrinsicRatio && !intrinsicSize.isEmpty())
            m_intrinsicSize = LayoutSize(intrinsicSize);

        if (!isHorizontalWritingMode()) {
            if (intrinsicRatio)
                intrinsicRatio = 1 / intrinsicRatio;
            intrinsicSize = intrinsicSize.transposedSize();
        }
    } else {
        computeIntrinsicRatioInformation(intrinsicSize, intrinsicRatio);
        if (intrinsicRatio && !intrinsicSize.isEmpty())
            m_intrinsicSize = LayoutSize(isHorizontalWritingMode() ? intrinsicSize : intrinsicSize.transposedSize());
    }

    // Now constrain the intrinsic size along each axis according to minimum and maximum width/heights along the
    // opposite axis. So for example a maximum width that shrinks our width will result in the height we compute here
    // having to shrink in order to preserve the aspect ratio. Because we compute these values independently along
    // each axis, the final returned size may in fact not preserve the aspect ratio.
    // FIXME: In the long term, it might be better to just return this code more to the way it used to be before this
    // function was added, since all it has done is make the code more unclear.
    constrainedSize = intrinsicSize;
    if (intrinsicRatio && !intrinsicSize.isEmpty() && style().logicalWidth().isAuto() && style().logicalHeight().isAuto()) {
        // We can't multiply or divide by 'intrinsicRatio' here, it breaks tests, like fast/images/zoomed-img-size.html, which
        // can only be fixed once subpixel precision is available for things like intrinsicWidth/Height - which include zoom!
        constrainedSize.setWidth(RenderBox::computeReplacedLogicalHeight() * intrinsicSize.width() / intrinsicSize.height());
        constrainedSize.setHeight(RenderBox::computeReplacedLogicalWidth() * intrinsicSize.height() / intrinsicSize.width());
    }
}

LayoutRect RenderReplaced::replacedContentRect(const LayoutSize& intrinsicSize) const
{
    LayoutRect contentRect = contentBoxRect();
    ObjectFit objectFit = style().objectFit();
    if (objectFit == ObjectFitFill)
        return contentRect;

    if (!intrinsicSize.width() || !intrinsicSize.height())
        return contentRect;

    LayoutRect finalRect = contentRect;
    switch (objectFit) {
    case ObjectFitContain:
    case ObjectFitScaleDown:
    case ObjectFitCover:
        finalRect.setSize(finalRect.size().fitToAspectRatio(intrinsicSize, objectFit == ObjectFitCover ? AspectRatioFitGrow : AspectRatioFitShrink));
        if (objectFit != ObjectFitScaleDown || finalRect.width() <= intrinsicSize.width())
            break;
        FALLTHROUGH;
    case ObjectFitNone:
        finalRect.setSize(intrinsicSize);
        break;
    case ObjectFitFill:
        ASSERT_NOT_REACHED();
    }

    // FIXME: This is where object-position should be taken into account, but since it's not
    // implemented yet, assume the initial value of "50% 50%".
    LayoutUnit xOffset = (contentRect.width() - finalRect.width()) / 2;
    LayoutUnit yOffset = (contentRect.height() - finalRect.height()) / 2;
    finalRect.move(xOffset, yOffset);

    return finalRect;
}

void RenderReplaced::computeIntrinsicRatioInformation(FloatSize& intrinsicSize, double& intrinsicRatio) const
{
    // If there's an embeddedContentBox() of a remote, referenced document available, this code-path should never be used.
    ASSERT(!embeddedContentBox());
    intrinsicSize = FloatSize(intrinsicLogicalWidth(), intrinsicLogicalHeight());

    // Figure out if we need to compute an intrinsic ratio.
    if (intrinsicSize.isEmpty() || !hasAspectRatio())
        return;

    intrinsicRatio = intrinsicSize.width() / intrinsicSize.height();
}

LayoutUnit RenderReplaced::computeReplacedLogicalWidth(ShouldComputePreferred shouldComputePreferred) const
{
    if (style().logicalWidth().isSpecified() || style().logicalWidth().isIntrinsic())
        return computeReplacedLogicalWidthRespectingMinMaxWidth(computeReplacedLogicalWidthUsing(style().logicalWidth()), shouldComputePreferred);

    RenderBox* contentRenderer = embeddedContentBox();

    // 10.3.2 Inline, replaced elements: http://www.w3.org/TR/CSS21/visudet.html#inline-replaced-width
    double intrinsicRatio = 0;
    FloatSize constrainedSize;
    computeAspectRatioInformationForRenderBox(contentRenderer, constrainedSize, intrinsicRatio);

    if (style().logicalWidth().isAuto()) {
        bool computedHeightIsAuto = hasAutoHeightOrContainingBlockWithAutoHeight();
        bool hasIntrinsicWidth = constrainedSize.width() > 0;

        // If 'height' and 'width' both have computed values of 'auto' and the element also has an intrinsic width, then that intrinsic width is the used value of 'width'.
        if (computedHeightIsAuto && hasIntrinsicWidth)
            return computeReplacedLogicalWidthRespectingMinMaxWidth(constrainedSize.width(), shouldComputePreferred);

        bool hasIntrinsicHeight = constrainedSize.height() > 0;
        if (intrinsicRatio) {
            // If 'height' and 'width' both have computed values of 'auto' and the element has no intrinsic width, but does have an intrinsic height and intrinsic ratio;
            // or if 'width' has a computed value of 'auto', 'height' has some other computed value, and the element does have an intrinsic ratio; then the used value
            // of 'width' is: (used height) * (intrinsic ratio)
            if (intrinsicRatio && ((computedHeightIsAuto && !hasIntrinsicWidth && hasIntrinsicHeight) || !computedHeightIsAuto)) {
                LayoutUnit logicalHeight = computeReplacedLogicalHeight();
                return computeReplacedLogicalWidthRespectingMinMaxWidth(roundToInt(round(logicalHeight * intrinsicRatio)), shouldComputePreferred);
            }

            // If 'height' and 'width' both have computed values of 'auto' and the element has an intrinsic ratio but no intrinsic height or width, then the used value of
            // 'width' is undefined in CSS 2.1. However, it is suggested that, if the containing block's width does not itself depend on the replaced element's width, then
            // the used value of 'width' is calculated from the constraint equation used for block-level, non-replaced elements in normal flow.
            if (computedHeightIsAuto && !hasIntrinsicWidth && !hasIntrinsicHeight) {
                // The aforementioned 'constraint equation' used for block-level, non-replaced elements in normal flow:
                // 'margin-left' + 'border-left-width' + 'padding-left' + 'width' + 'padding-right' + 'border-right-width' + 'margin-right' = width of containing block
                LayoutUnit logicalWidth;
                if (RenderBlock* blockWithWidth = firstContainingBlockWithLogicalWidth(this))
                    logicalWidth = blockWithWidth->computeReplacedLogicalWidthRespectingMinMaxWidth(blockWithWidth->computeReplacedLogicalWidthUsing(blockWithWidth->style().logicalWidth()), shouldComputePreferred);
                else
                    logicalWidth = containingBlock()->availableLogicalWidth();

                // This solves above equation for 'width' (== logicalWidth).
                LayoutUnit marginStart = minimumValueForLength(style().marginStart(), logicalWidth);
                LayoutUnit marginEnd = minimumValueForLength(style().marginEnd(), logicalWidth);
                logicalWidth = std::max<LayoutUnit>(0, logicalWidth - (marginStart + marginEnd + (width() - clientWidth())));
                return computeReplacedLogicalWidthRespectingMinMaxWidth(logicalWidth, shouldComputePreferred);
            }
        }

        // Otherwise, if 'width' has a computed value of 'auto', and the element has an intrinsic width, then that intrinsic width is the used value of 'width'.
        if (hasIntrinsicWidth)
            return computeReplacedLogicalWidthRespectingMinMaxWidth(constrainedSize.width(), shouldComputePreferred);

        // Otherwise, if 'width' has a computed value of 'auto', but none of the conditions above are met, then the used value of 'width' becomes 300px. If 300px is too
        // wide to fit the device, UAs should use the width of the largest rectangle that has a 2:1 ratio and fits the device instead.
        // Note: We fall through and instead return intrinsicLogicalWidth() here - to preserve existing WebKit behavior, which might or might not be correct, or desired.
        // Changing this to return cDefaultWidth, will affect lots of test results. Eg. some tests assume that a blank <img> tag (which implies width/height=auto)
        // has no intrinsic size, which is wrong per CSS 2.1, but matches our behavior since a long time.
    }

    return computeReplacedLogicalWidthRespectingMinMaxWidth(intrinsicLogicalWidth(), shouldComputePreferred);
}

LayoutUnit RenderReplaced::computeReplacedLogicalHeight() const
{
    // 10.5 Content height: the 'height' property: http://www.w3.org/TR/CSS21/visudet.html#propdef-height
    if (hasReplacedLogicalHeight())
        return computeReplacedLogicalHeightRespectingMinMaxHeight(computeReplacedLogicalHeightUsing(style().logicalHeight()));

    RenderBox* contentRenderer = embeddedContentBox();

    // 10.6.2 Inline, replaced elements: http://www.w3.org/TR/CSS21/visudet.html#inline-replaced-height
    double intrinsicRatio = 0;
    FloatSize constrainedSize;
    computeAspectRatioInformationForRenderBox(contentRenderer, constrainedSize, intrinsicRatio);

    bool widthIsAuto = style().logicalWidth().isAuto();
    bool hasIntrinsicHeight = constrainedSize.height() > 0;

    // If 'height' and 'width' both have computed values of 'auto' and the element also has an intrinsic height, then that intrinsic height is the used value of 'height'.
    if (widthIsAuto && hasIntrinsicHeight)
        return computeReplacedLogicalHeightRespectingMinMaxHeight(constrainedSize.height());

    // Otherwise, if 'height' has a computed value of 'auto', and the element has an intrinsic ratio then the used value of 'height' is:
    // (used width) / (intrinsic ratio)
    if (intrinsicRatio)
        return computeReplacedLogicalHeightRespectingMinMaxHeight(roundToInt(round(availableLogicalWidth() / intrinsicRatio)));

    // Otherwise, if 'height' has a computed value of 'auto', and the element has an intrinsic height, then that intrinsic height is the used value of 'height'.
    if (hasIntrinsicHeight)
        return computeReplacedLogicalHeightRespectingMinMaxHeight(constrainedSize.height());

    // Otherwise, if 'height' has a computed value of 'auto', but none of the conditions above are met, then the used value of 'height' must be set to the height
    // of the largest rectangle that has a 2:1 ratio, has a height not greater than 150px, and has a width not greater than the device width.
    return computeReplacedLogicalHeightRespectingMinMaxHeight(intrinsicLogicalHeight());
}

void RenderReplaced::computeIntrinsicLogicalWidths(LayoutUnit& minLogicalWidth, LayoutUnit& maxLogicalWidth) const
{
    minLogicalWidth = maxLogicalWidth = intrinsicLogicalWidth();
}

void RenderReplaced::computePreferredLogicalWidths()
{
    ASSERT(preferredLogicalWidthsDirty());

    // We cannot resolve any percent logical width here as the available logical
    // width may not be set on our containing block.
    if (style().logicalWidth().isPercent())
        computeIntrinsicLogicalWidths(m_minPreferredLogicalWidth, m_maxPreferredLogicalWidth);
    else
        m_minPreferredLogicalWidth = m_maxPreferredLogicalWidth = computeReplacedLogicalWidth(ComputePreferred);

    const RenderStyle& styleToUse = style();
    if (styleToUse.logicalWidth().isPercent() || styleToUse.logicalMaxWidth().isPercent())
        m_minPreferredLogicalWidth = 0;

    if (styleToUse.logicalMinWidth().isFixed() && styleToUse.logicalMinWidth().value() > 0) {
        m_maxPreferredLogicalWidth = std::max(m_maxPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(styleToUse.logicalMinWidth().value()));
        m_minPreferredLogicalWidth = std::max(m_minPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(styleToUse.logicalMinWidth().value()));
    }
    
    if (styleToUse.logicalMaxWidth().isFixed()) {
        m_maxPreferredLogicalWidth = std::min(m_maxPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(styleToUse.logicalMaxWidth().value()));
        m_minPreferredLogicalWidth = std::min(m_minPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(styleToUse.logicalMaxWidth().value()));
    }

    LayoutUnit borderAndPadding = borderAndPaddingLogicalWidth();
    m_minPreferredLogicalWidth += borderAndPadding;
    m_maxPreferredLogicalWidth += borderAndPadding;

    setPreferredLogicalWidthsDirty(false);
}

VisiblePosition RenderReplaced::positionForPoint(const LayoutPoint& point, const RenderRegion* region)
{
    // FIXME: This code is buggy if the replaced element is relative positioned.
    InlineBox* box = inlineBoxWrapper();
    const RootInlineBox* rootBox = box ? &box->root() : 0;
    
    LayoutUnit top = rootBox ? rootBox->selectionTop() : logicalTop();
    LayoutUnit bottom = rootBox ? rootBox->selectionBottom() : logicalBottom();
    
    LayoutUnit blockDirectionPosition = isHorizontalWritingMode() ? point.y() + y() : point.x() + x();
    LayoutUnit lineDirectionPosition = isHorizontalWritingMode() ? point.x() + x() : point.y() + y();
    
    if (blockDirectionPosition < top)
        return createVisiblePosition(caretMinOffset(), DOWNSTREAM); // coordinates are above
    
    if (blockDirectionPosition >= bottom)
        return createVisiblePosition(caretMaxOffset(), DOWNSTREAM); // coordinates are below
    
    if (element()) {
        if (lineDirectionPosition <= logicalLeft() + (logicalWidth() / 2))
            return createVisiblePosition(0, DOWNSTREAM);
        return createVisiblePosition(1, DOWNSTREAM);
    }

    return RenderBox::positionForPoint(point, region);
}

LayoutRect RenderReplaced::selectionRectForRepaint(const RenderLayerModelObject* repaintContainer, bool clipToVisibleContent)
{
    ASSERT(!needsLayout());

    if (!isSelected())
        return LayoutRect();
    
    LayoutRect rect = localSelectionRect();
    if (clipToVisibleContent)
        computeRectForRepaint(repaintContainer, rect);
    else
        rect = localToContainerQuad(FloatRect(rect), repaintContainer).enclosingBoundingBox();
    
    return rect;
}

LayoutRect RenderReplaced::localSelectionRect(bool checkWhetherSelected) const
{
    if (checkWhetherSelected && !isSelected())
        return LayoutRect();

    if (!m_inlineBoxWrapper)
        // We're a block-level replaced element.  Just return our own dimensions.
        return LayoutRect(LayoutPoint(), size());
    
    const RootInlineBox& rootBox = m_inlineBoxWrapper->root();
    LayoutUnit newLogicalTop = rootBox.blockFlow().style().isFlippedBlocksWritingMode() ? m_inlineBoxWrapper->logicalBottom() - rootBox.selectionBottom() : rootBox.selectionTop() - m_inlineBoxWrapper->logicalTop();
    if (rootBox.blockFlow().style().isHorizontalWritingMode())
        return LayoutRect(0, newLogicalTop, width(), rootBox.selectionHeight());
    return LayoutRect(newLogicalTop, 0, rootBox.selectionHeight(), height());
}

void RenderReplaced::setSelectionState(SelectionState state)
{
    // The selection state for our containing block hierarchy is updated by the base class call.
    RenderBox::setSelectionState(state);

    if (m_inlineBoxWrapper && canUpdateSelectionOnRootLineBoxes())
        m_inlineBoxWrapper->root().setHasSelectedChildren(isSelected());
}

bool RenderReplaced::isSelected() const
{
    SelectionState s = selectionState();
    if (s == SelectionNone)
        return false;
    if (s == SelectionInside)
        return true;

    int selectionStart, selectionEnd;
    selectionStartEnd(selectionStart, selectionEnd);
    if (s == SelectionStart)
        return selectionStart == 0;
        
    int end = element()->hasChildNodes() ? element()->childNodeCount() : 1;
    if (s == SelectionEnd)
        return selectionEnd == end;
    if (s == SelectionBoth)
        return selectionStart == 0 && selectionEnd == end;
        
    ASSERT(0);
    return false;
}

LayoutRect RenderReplaced::clippedOverflowRectForRepaint(const RenderLayerModelObject* repaintContainer) const
{
    if (style().visibility() != VISIBLE && !enclosingLayer()->hasVisibleContent())
        return LayoutRect();

    // The selectionRect can project outside of the overflowRect, so take their union
    // for repainting to avoid selection painting glitches.
    LayoutRect r = unionRect(localSelectionRect(false), visualOverflowRect());

    // FIXME: layoutDelta needs to be applied in parts before/after transforms and
    // repaint containers. https://bugs.webkit.org/show_bug.cgi?id=23308
    r.move(view().layoutDelta());

    r.inflate(style().outlineSize());

    computeRectForRepaint(repaintContainer, r);
    return r;
}

}
