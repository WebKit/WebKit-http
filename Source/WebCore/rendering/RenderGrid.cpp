/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
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
#include "RenderGrid.h"

#include "LayoutRepainter.h"
#include "NotImplemented.h"
#include "RenderLayer.h"
#include "RenderView.h"

namespace WebCore {

class RenderGrid::GridTrack {
public:
    GridTrack()
        : m_usedBreadth(0)
    {
    }

    LayoutUnit m_usedBreadth;
};

RenderGrid::RenderGrid(Node* node)
    : RenderBlock(node)
{
    // All of our children must be block level.
    setChildrenInline(false);
}

RenderGrid::~RenderGrid()
{
}

void RenderGrid::layoutBlock(bool relayoutChildren, LayoutUnit)
{
    ASSERT(needsLayout());

    if (!relayoutChildren && simplifiedLayout())
        return;

    // FIXME: Much of this method is boiler plate that matches RenderBox::layoutBlock and Render*FlexibleBox::layoutBlock.
    // It would be nice to refactor some of the duplicate code.
    LayoutRepainter repainter(*this, checkForRepaintDuringLayout());
    LayoutStateMaintainer statePusher(view(), this, locationOffset(), hasTransform() || hasReflection() || style()->isFlippedBlocksWritingMode());

    if (inRenderFlowThread()) {
        // Regions changing widths can force us to relayout our children.
        if (logicalWidthChangedInRegions())
            relayoutChildren = true;
    }
    updateRegionsAndExclusionsLogicalSize();

    LayoutSize previousSize = size();

    setLogicalHeight(0);
    updateLogicalWidth();

    m_overflow.clear();

    layoutGridItems();

    LayoutUnit oldClientAfterEdge = clientLogicalBottom();
    updateLogicalHeight();

    if (size() != previousSize)
        relayoutChildren = true;

    layoutPositionedObjects(relayoutChildren || isRoot());

    computeRegionRangeForBlock();

    computeOverflow(oldClientAfterEdge);
    statePusher.pop();

    updateLayerTransform();

    // Update our scroll information if we're overflow:auto/scroll/hidden now that we know if
    // we overflow or not.
    if (hasOverflowClip())
        layer()->updateScrollInfoAfterLayout();

    repainter.repaintAfterLayout();

    setNeedsLayout(false);
}

void RenderGrid::computePreferredLogicalWidths()
{
    ASSERT(preferredLogicalWidthsDirty());

    m_minPreferredLogicalWidth = 0;
    m_maxPreferredLogicalWidth = 0;

    // FIXME: We don't take our own logical width into account.

    const Vector<GridTrackSize>& trackStyles = style()->gridColumns();

    for (size_t i = 0; i < trackStyles.size(); ++i) {
        Length trackLength = trackStyles[i].length();
        if (!trackLength.isFixed()) {
            notImplemented();
            continue;
        }

        m_minPreferredLogicalWidth += trackLength.intValue();
        m_maxPreferredLogicalWidth += trackLength.intValue();
    }

    // FIXME: We should account for min / max logical width.

    LayoutUnit borderAndPaddingInInlineDirection = borderAndPaddingLogicalWidth();
    m_minPreferredLogicalWidth += borderAndPaddingInInlineDirection;
    m_maxPreferredLogicalWidth += borderAndPaddingInInlineDirection;

    setPreferredLogicalWidthsDirty(false);
}

void RenderGrid::computedUsedBreadthOfGridTracks(TrackSizingDirection direction, Vector<GridTrack>& tracks)
{
    const Vector<GridTrackSize>& trackStyles = (direction == ForColumns) ? style()->gridColumns() : style()->gridRows();
    for (size_t i = 0; i < trackStyles.size(); ++i) {
        GridTrack track;
        switch (trackStyles[i].type()) {
        case LengthTrackSizing: {
            Length trackLength = trackStyles[i].length();
            // FIXME: We stil need to support calc() here (bug 103761).
            if (trackLength.isFixed() || trackLength.isPercent() || trackLength.isViewportPercentage())
                track.m_usedBreadth = valueForLength(trackLength, direction == ForColumns ? logicalWidth() : computeContentLogicalHeight(MainOrPreferredSize, style()->logicalHeight()), view());
            else
                notImplemented();

            break;
        }
        case MinMaxTrackSizing:
            // FIXME: Implement support for minmax track sizing (bug 103311).
            notImplemented();
        }
        tracks.append(track);
    }
}

void RenderGrid::layoutGridItems()
{
    Vector<GridTrack> columnTracks, rowTracks;
    computedUsedBreadthOfGridTracks(ForColumns, columnTracks);
    // FIXME: The logical width of Grid Columns from the prior step is used in
    // the formatting of Grid items in content-sized Grid Rows to determine
    // their required height. We will probably need to pass columns through.
    computedUsedBreadthOfGridTracks(ForRows, rowTracks);

    for (RenderBox* child = firstChildBox(); child; child = child->nextSiblingBox()) {
        LayoutPoint childPosition = findChildLogicalPosition(child, columnTracks, rowTracks);

        size_t columnTrack = resolveGridPosition(child->style()->gridItemColumn());
        size_t rowTrack = resolveGridPosition(child->style()->gridItemRow());

        // FIXME: Properly support implicit rows and columns (bug 103573).
        if (columnTrack < columnTracks.size() && rowTrack < rowTracks.size()) {
            // Because the grid area cannot be styled, we don't need to adjust
            // the grid breadth to account for 'box-sizing'.
            child->setOverrideContainingBlockContentLogicalWidth(columnTracks[columnTrack].m_usedBreadth);
            child->setOverrideContainingBlockContentLogicalHeight(rowTracks[rowTrack].m_usedBreadth);
        }

        // FIXME: Grid items should stretch to fill their cells. Once we
        // implement grid-{column,row}-align, we can also shrink to fit. For
        // now, just size as if we were a regular child.
        child->layoutIfNeeded();

        // FIXME: Handle border & padding on the grid element.
        child->setLogicalLocation(childPosition);
    }

    for (size_t i = 0; i < rowTracks.size(); ++i)
        setLogicalHeight(logicalHeight() + rowTracks[i].m_usedBreadth);

    // FIXME: We should handle min / max logical height.

    setLogicalHeight(logicalHeight() + borderAndPaddingLogicalHeight());
}

size_t RenderGrid::resolveGridPosition(const GridPosition& position) const
{
    // FIXME: Handle other values for grid-{row,column} like ranges or line names.
    switch (position.type()) {
    case IntegerPosition:
        // FIXME: What does a non-positive integer mean for a column/row?
        if (!position.isPositive())
            return 0;

        return position.integerPosition() - 1;
    case AutoPosition:
        // FIXME: We should follow 'grid-auto-flow' for resolution.
        // Until then, we use the 'grid-auto-flow: none' behavior (which is the default)
        // and resolve 'auto' as the first row / column.
        return 0;
    }
    ASSERT_NOT_REACHED();
    return 0;
}

LayoutPoint RenderGrid::findChildLogicalPosition(RenderBox* child, const Vector<GridTrack>& columnTracks, const Vector<GridTrack>& rowTracks)
{
    size_t columnTrack = resolveGridPosition(child->style()->gridItemColumn());
    size_t rowTrack = resolveGridPosition(child->style()->gridItemRow());

    LayoutPoint offset;
    // FIXME: |columnTrack| and |rowTrack| should be smaller than our column / row count.
    for (size_t i = 0; i < columnTrack && i < columnTracks.size(); ++i)
        offset.setX(offset.x() + columnTracks[i].m_usedBreadth);
    for (size_t i = 0; i < rowTrack && i < rowTracks.size(); ++i)
        offset.setY(offset.y() + rowTracks[i].m_usedBreadth);

    // FIXME: Handle margins on the grid item.
    return offset;
}

const char* RenderGrid::renderName() const
{
    if (isFloating())
        return "RenderGrid (floating)";
    if (isOutOfFlowPositioned())
        return "RenderGrid (positioned)";
    if (isAnonymous())
        return "RenderGrid (generated)";
    if (isRelPositioned())
        return "RenderGrid (relative positioned)";
    return "RenderGrid";
}

} // namespace WebCore
