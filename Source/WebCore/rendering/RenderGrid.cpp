/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2013 Igalia S.L. All rights reserved.
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

#include "GridCoordinate.h"
#include "LayoutRepainter.h"
#include "NotImplemented.h"
#include "RenderLayer.h"
#include "RenderView.h"

namespace WebCore {

static const int infinity = -1;

class GridTrack {
public:
    GridTrack()
        : m_usedBreadth(0)
        , m_maxBreadth(0)
    {
    }

    void growUsedBreadth(LayoutUnit growth)
    {
        ASSERT(growth >= 0);
        m_usedBreadth += growth;
    }
    LayoutUnit usedBreadth() const { return m_usedBreadth; }

    void growMaxBreadth(LayoutUnit growth)
    {
        if (m_maxBreadth == infinity)
            m_maxBreadth = m_usedBreadth + growth;
        else
            m_maxBreadth += growth;
    }
    LayoutUnit maxBreadthIfNotInfinite() const
    {
        return (m_maxBreadth == infinity) ? m_usedBreadth : m_maxBreadth;
    }

    LayoutUnit m_usedBreadth;
    LayoutUnit m_maxBreadth;
};

struct GridTrackForNormalization {
    GridTrackForNormalization(const GridTrack& track, double flex)
        : m_track(&track)
        , m_flex(flex)
        , m_normalizedFlexValue(track.m_usedBreadth / flex)
    {
    }

    const GridTrack* m_track;
    double m_flex;
    LayoutUnit m_normalizedFlexValue;
};

class RenderGrid::GridIterator {
    WTF_MAKE_NONCOPYABLE(GridIterator);
public:
    // |direction| is the direction that is fixed to |fixedTrackIndex| so e.g
    // GridIterator(m_grid, ForColumns, 1) will walk over the rows of the 2nd column.
    GridIterator(const Vector<Vector<Vector<RenderBox*, 1>>>& grid, TrackSizingDirection direction, size_t fixedTrackIndex)
        : m_grid(grid)
        , m_direction(direction)
        , m_rowIndex((direction == ForColumns) ? 0 : fixedTrackIndex)
        , m_columnIndex((direction == ForColumns) ? fixedTrackIndex : 0)
        , m_childIndex(0)
    {
        ASSERT(m_rowIndex < m_grid.size());
        ASSERT(m_columnIndex < m_grid[0].size());
    }

    RenderBox* nextGridItem()
    {
        if (!m_grid.size())
            return 0;

        size_t& varyingTrackIndex = (m_direction == ForColumns) ? m_rowIndex : m_columnIndex;
        const size_t endOfVaryingTrackIndex = (m_direction == ForColumns) ? m_grid.size() : m_grid[0].size();
        for (; varyingTrackIndex < endOfVaryingTrackIndex; ++varyingTrackIndex) {
            const Vector<RenderBox*>& children = m_grid[m_rowIndex][m_columnIndex];
            if (m_childIndex < children.size())
                return children[m_childIndex++];

            m_childIndex = 0;
        }
        return 0;
    }

    PassOwnPtr<GridCoordinate> nextEmptyGridArea()
    {
        if (m_grid.isEmpty())
            return nullptr;

        size_t& varyingTrackIndex = (m_direction == ForColumns) ? m_rowIndex : m_columnIndex;
        const size_t endOfVaryingTrackIndex = (m_direction == ForColumns) ? m_grid.size() : m_grid[0].size();
        for (; varyingTrackIndex < endOfVaryingTrackIndex; ++varyingTrackIndex) {
            const Vector<RenderBox*>& children = m_grid[m_rowIndex][m_columnIndex];
            if (children.isEmpty()) {
                OwnPtr<GridCoordinate> result = adoptPtr(new GridCoordinate(GridSpan(m_rowIndex, m_rowIndex), GridSpan(m_columnIndex, m_columnIndex)));
                // Advance the iterator to avoid an infinite loop where we would return the same grid area over and over.
                ++varyingTrackIndex;
                return result.release();
            }
        }
        return nullptr;
    }

private:
    const Vector<Vector<Vector<RenderBox*, 1>>>& m_grid;
    TrackSizingDirection m_direction;
    size_t m_rowIndex;
    size_t m_columnIndex;
    size_t m_childIndex;
};

RenderGrid::RenderGrid(Element& element, PassRef<RenderStyle> style)
    : RenderBlock(element, std::move(style), 0)
    , m_orderIterator(*this)
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
    LayoutStateMaintainer statePusher(&view(), this, locationOffset(), hasTransform() || hasReflection() || style().isFlippedBlocksWritingMode());

    prepareShapesAndPaginationBeforeBlockLayout(relayoutChildren);

    LayoutSize previousSize = size();

    setLogicalHeight(0);
    updateLogicalWidth();

    layoutGridItems();

    LayoutUnit oldClientAfterEdge = clientLogicalBottom();
    updateLogicalHeight();

    if (size() != previousSize)
        relayoutChildren = true;

    layoutPositionedObjects(relayoutChildren || isRoot());

    updateShapesAfterBlockLayout();

    computeOverflow(oldClientAfterEdge);
    statePusher.pop();

    updateLayerTransform();

    // Update our scroll information if we're overflow:auto/scroll/hidden now that we know if
    // we overflow or not.
    updateScrollInfoAfterLayout();

    repainter.repaintAfterLayout();

    clearNeedsLayout();
}

void RenderGrid::computeIntrinsicLogicalWidths(LayoutUnit& minLogicalWidth, LayoutUnit& maxLogicalWidth) const
{
    const_cast<RenderGrid*>(this)->placeItemsOnGrid();

    // FIXME: This is an inefficient way to fill our sizes as it will try every grid areas, when we would
    // only want to account for fixed grid tracks and grid items. Also this will be incorrect if we have spanning
    // grid items.
    for (size_t i = 0; i < gridColumnCount(); ++i) {
        const GridTrackSize& trackSize = gridTrackSize(ForColumns, i);
        LayoutUnit minTrackBreadth = computePreferredTrackWidth(trackSize.minTrackBreadth(), i);
        LayoutUnit maxTrackBreadth = computePreferredTrackWidth(trackSize.maxTrackBreadth(), i);
        maxTrackBreadth = std::max(maxTrackBreadth, minTrackBreadth);

        minLogicalWidth += minTrackBreadth;
        maxLogicalWidth += maxTrackBreadth;

        // FIXME: This should add in the scrollbarWidth (e.g. see RenderFlexibleBox).
    }

    const_cast<RenderGrid*>(this)->clearGrid();
}

void RenderGrid::computePreferredLogicalWidths()
{
    ASSERT(preferredLogicalWidthsDirty());

    m_minPreferredLogicalWidth = 0;
    m_maxPreferredLogicalWidth = 0;

    // FIXME: We don't take our own logical width into account. Once we do, we need to make sure
    // we apply (and test the interaction with) min-width / max-width.

    computeIntrinsicLogicalWidths(m_minPreferredLogicalWidth, m_maxPreferredLogicalWidth);

    LayoutUnit borderAndPaddingInInlineDirection = borderAndPaddingLogicalWidth();
    m_minPreferredLogicalWidth += borderAndPaddingInInlineDirection;
    m_maxPreferredLogicalWidth += borderAndPaddingInInlineDirection;

    setPreferredLogicalWidthsDirty(false);
}

LayoutUnit RenderGrid::computePreferredTrackWidth(const GridLength& gridLength, size_t trackIndex) const
{
    if (gridLength.isFlex())
        return 0;

    const Length& length = gridLength.length();

    if (length.isFixed()) {
        // Grid areas don't have borders, margins or paddings so we don't need to account for them.
        return length.intValue();
    }

    if (length.isMinContent()) {
        LayoutUnit minContentSize = 0;
        GridIterator iterator(m_grid, ForColumns, trackIndex);
        while (RenderBox* gridItem = iterator.nextGridItem()) {
            // FIXME: We should include the child's fixed margins like RenderFlexibleBox.
            minContentSize = std::max(minContentSize, gridItem->minPreferredLogicalWidth());
        }
        return minContentSize;
    }

    if (length.isMaxContent()) {
        LayoutUnit maxContentSize = 0;
        GridIterator iterator(m_grid, ForColumns, trackIndex);
        while (RenderBox* gridItem = iterator.nextGridItem()) {
            // FIXME: We should include the child's fixed margins like RenderFlexibleBox.
            maxContentSize = std::max(maxContentSize, gridItem->maxPreferredLogicalWidth());
        }
        return maxContentSize;
    }

    // FIXME: css3-sizing mentions that we should resolve "definite sizes"
    // (including <percentage> and calc()) but we don't do it elsewhere.
    return 0;
}

void RenderGrid::computedUsedBreadthOfGridTracks(TrackSizingDirection direction, Vector<GridTrack>& columnTracks, Vector<GridTrack>& rowTracks)
{
    LayoutUnit availableLogicalSpace = (direction == ForColumns) ? availableLogicalWidth() : availableLogicalHeight(IncludeMarginBorderPadding);
    Vector<GridTrack>& tracks = (direction == ForColumns) ? columnTracks : rowTracks;
    for (size_t i = 0; i < tracks.size(); ++i) {
        GridTrack& track = tracks[i];
        const GridTrackSize& trackSize = gridTrackSize(direction, i);
        const GridLength& minTrackBreadth = trackSize.minTrackBreadth();
        const GridLength& maxTrackBreadth = trackSize.maxTrackBreadth();

        track.m_usedBreadth = computeUsedBreadthOfMinLength(direction, minTrackBreadth);
        track.m_maxBreadth = computeUsedBreadthOfMaxLength(direction, maxTrackBreadth, track.m_usedBreadth);

        track.m_maxBreadth = std::max(track.m_maxBreadth, track.m_usedBreadth);
    }

    // FIXME: We shouldn't call resolveContentBasedTrackSizingFunctions if we have no min-content / max-content tracks.
    resolveContentBasedTrackSizingFunctions(direction, columnTracks, rowTracks, availableLogicalSpace);

    if (availableLogicalSpace <= 0)
        return;

    const size_t tracksSize = tracks.size();
    Vector<GridTrack*> tracksForDistribution(tracksSize);
    for (size_t i = 0; i < tracksSize; ++i)
        tracksForDistribution[i] = tracks.data() + i;

    distributeSpaceToTracks(tracksForDistribution, 0, &GridTrack::usedBreadth, &GridTrack::growUsedBreadth, availableLogicalSpace);

    // 4. Grow all Grid tracks having a fraction as the MaxTrackSizingFunction.

    // FIXME: Handle the case where RemainingSpace is not defined.
    double normalizedFractionBreadth = computeNormalizedFractionBreadth(tracks, direction, availableLogicalSpace);
    for (size_t i = 0; i < tracksSize; ++i) {
        const GridTrackSize& trackSize = gridTrackSize(direction, i);
        if (!trackSize.maxTrackBreadth().isFlex())
            continue;

        tracks[i].m_usedBreadth = std::max<LayoutUnit>(tracks[i].m_usedBreadth, normalizedFractionBreadth * trackSize.maxTrackBreadth().flex());
    }
}

LayoutUnit RenderGrid::computeUsedBreadthOfMinLength(TrackSizingDirection direction, const GridLength& gridLength) const
{
    if (gridLength.isFlex())
        return 0;

    const Length& trackLength = gridLength.length();
    ASSERT(!trackLength.isAuto());
    if (trackLength.isFixed() || trackLength.isPercent() || trackLength.isViewportPercentage())
        return computeUsedBreadthOfSpecifiedLength(direction, trackLength);

    ASSERT(trackLength.isMinContent() || trackLength.isMaxContent());
    return 0;
}

LayoutUnit RenderGrid::computeUsedBreadthOfMaxLength(TrackSizingDirection direction, const GridLength& gridLength, LayoutUnit usedBreadth) const
{
    if (gridLength.isFlex())
        return usedBreadth;

    const Length& trackLength = gridLength.length();
    ASSERT(!trackLength.isAuto());
    if (trackLength.isFixed() || trackLength.isPercent() || trackLength.isViewportPercentage()) {
        LayoutUnit computedBreadth = computeUsedBreadthOfSpecifiedLength(direction, trackLength);
        ASSERT(computedBreadth != infinity);
        return computedBreadth;
    }

    ASSERT(trackLength.isMinContent() || trackLength.isMaxContent());
    return infinity;
}

LayoutUnit RenderGrid::computeUsedBreadthOfSpecifiedLength(TrackSizingDirection direction, const Length& trackLength) const
{
    // FIXME: We still need to support calc() here (https://webkit.org/b/103761).
    ASSERT(trackLength.isFixed() || trackLength.isPercent() || trackLength.isViewportPercentage());
    return valueForLength(trackLength, direction == ForColumns ? logicalWidth() : computeContentLogicalHeight(style().logicalHeight()));
}

double RenderGrid::computeNormalizedFractionBreadth(Vector<GridTrack>& tracks, TrackSizingDirection direction, LayoutUnit availableLogicalSpace) const
{
    // |availableLogicalSpace| already accounts for the used breadths so no need to remove it here.

    Vector<GridTrackForNormalization> tracksForNormalization;
    for (size_t i = 0; i < tracks.size(); ++i) {
        const GridTrackSize& trackSize = gridTrackSize(direction, i);
        if (!trackSize.maxTrackBreadth().isFlex())
            continue;

        tracksForNormalization.append(GridTrackForNormalization(tracks[i], trackSize.maxTrackBreadth().flex()));
    }

    // FIXME: Ideally we shouldn't come here without any <flex> grid track.
    if (tracksForNormalization.isEmpty())
        return LayoutUnit();

    std::sort(tracksForNormalization.begin(), tracksForNormalization.end(),
              [](const GridTrackForNormalization& track1, const GridTrackForNormalization& track2) {
                  return track1.m_normalizedFlexValue < track2.m_normalizedFlexValue;
              });

    // These values work together: as we walk over our grid tracks, we increase fractionValueBasedOnGridItemsRatio
    // to match a grid track's usedBreadth to <flex> ratio until the total fractions sized grid tracks wouldn't
    // fit into availableLogicalSpaceIgnoringFractionTracks.
    double accumulatedFractions = 0;
    LayoutUnit fractionValueBasedOnGridItemsRatio = 0;
    LayoutUnit availableLogicalSpaceIgnoringFractionTracks = availableLogicalSpace;

    for (size_t i = 0; i < tracksForNormalization.size(); ++i) {
        const GridTrackForNormalization& track = tracksForNormalization[i];
        if (track.m_normalizedFlexValue > fractionValueBasedOnGridItemsRatio) {
            // If the normalized flex value (we ordered |tracksForNormalization| by increasing normalized flex value)
            // will make us overflow our container, then stop. We have the previous step's ratio is the best fit.
            if (track.m_normalizedFlexValue * accumulatedFractions > availableLogicalSpaceIgnoringFractionTracks)
                break;

            fractionValueBasedOnGridItemsRatio = track.m_normalizedFlexValue;
        }

        accumulatedFractions += track.m_flex;
        // This item was processed so we re-add its used breadth to the available space to accurately count the remaining space.
        availableLogicalSpaceIgnoringFractionTracks += track.m_track->m_usedBreadth;
    }

    return availableLogicalSpaceIgnoringFractionTracks / accumulatedFractions;
}

const GridTrackSize& RenderGrid::gridTrackSize(TrackSizingDirection direction, size_t i) const
{
    const Vector<GridTrackSize>& trackStyles = (direction == ForColumns) ? style().gridColumns() : style().gridRows();
    if (i >= trackStyles.size())
        return (direction == ForColumns) ? style().gridAutoColumns() : style().gridAutoRows();

    return trackStyles[i];
}

size_t RenderGrid::explicitGridColumnCount() const
{
    return style().gridColumns().size();
}

size_t RenderGrid::explicitGridRowCount() const
{
    return style().gridRows().size();
}

size_t RenderGrid::explicitGridSizeForSide(GridPositionSide side) const
{
    return (side == ColumnStartSide || side == ColumnEndSide) ? explicitGridColumnCount() : explicitGridRowCount();
}

LayoutUnit RenderGrid::logicalContentHeightForChild(RenderBox* child, Vector<GridTrack>& columnTracks)
{
    // FIXME: We shouldn't force a layout every time this function is called but
    // 1) Return computeLogicalHeight's value if it's available. Unfortunately computeLogicalHeight
    // doesn't return if the logical height is available so would need to be changed.
    // 2) Relayout if the column track's used breadth changed OR the logical height is unavailable.
    if (!child->needsLayout())
        child->setNeedsLayout(MarkOnlyThis);

    child->setOverrideContainingBlockContentLogicalWidth(gridAreaBreadthForChild(child, ForColumns, columnTracks));
    // If |child| has a percentage logical height, we shouldn't let it override its intrinsic height, which is
    // what we are interested in here. Thus we need to set the override logical height to -1 (no possible resolution).
    child->setOverrideContainingBlockContentLogicalHeight(-1);
    child->layout();
    return child->logicalHeight();
}

LayoutUnit RenderGrid::minContentForChild(RenderBox* child, TrackSizingDirection direction, Vector<GridTrack>& columnTracks)
{
    bool hasOrthogonalWritingMode = child->isHorizontalWritingMode() != isHorizontalWritingMode();
    // FIXME: Properly support orthogonal writing mode.
    if (hasOrthogonalWritingMode)
        return 0;

    if (direction == ForColumns) {
        // FIXME: It's unclear if we should return the intrinsic width or the preferred width.
        // See http://lists.w3.org/Archives/Public/www-style/2013Jan/0245.html
        return child->minPreferredLogicalWidth();
    }

    return logicalContentHeightForChild(child, columnTracks);
}

LayoutUnit RenderGrid::maxContentForChild(RenderBox* child, TrackSizingDirection direction, Vector<GridTrack>& columnTracks)
{
    bool hasOrthogonalWritingMode = child->isHorizontalWritingMode() != isHorizontalWritingMode();
    // FIXME: Properly support orthogonal writing mode.
    if (hasOrthogonalWritingMode)
        return LayoutUnit();

    if (direction == ForColumns) {
        // FIXME: It's unclear if we should return the intrinsic width or the preferred width.
        // See http://lists.w3.org/Archives/Public/www-style/2013Jan/0245.html
        return child->maxPreferredLogicalWidth();
    }

    return logicalContentHeightForChild(child, columnTracks);
}

void RenderGrid::resolveContentBasedTrackSizingFunctions(TrackSizingDirection direction, Vector<GridTrack>& columnTracks, Vector<GridTrack>& rowTracks, LayoutUnit& availableLogicalSpace)
{
    // FIXME: Split the grid tracks into groups that doesn't overlap a <flex> grid track.

    Vector<GridTrack>& tracks = (direction == ForColumns) ? columnTracks : rowTracks;

    // FIXME: Per step 2 of the specification, we should order the grid items by increasing span.
    for (RenderBox* child = firstChildBox(); child; child = child->nextSiblingBox()) {
        resolveContentBasedTrackSizingFunctionsForItems(direction, columnTracks, rowTracks, child, &GridTrackSize::hasMinOrMaxContentMinTrackBreadth, &RenderGrid::minContentForChild, &GridTrack::usedBreadth, &GridTrack::growUsedBreadth);
        resolveContentBasedTrackSizingFunctionsForItems(direction, columnTracks, rowTracks, child, &GridTrackSize::hasMaxContentMinTrackBreadth, &RenderGrid::maxContentForChild, &GridTrack::usedBreadth, &GridTrack::growUsedBreadth);
        resolveContentBasedTrackSizingFunctionsForItems(direction, columnTracks, rowTracks, child, &GridTrackSize::hasMinOrMaxContentMaxTrackBreadth, &RenderGrid::minContentForChild, &GridTrack::maxBreadthIfNotInfinite, &GridTrack::growMaxBreadth);
        resolveContentBasedTrackSizingFunctionsForItems(direction, columnTracks, rowTracks, child, &GridTrackSize::hasMaxContentMaxTrackBreadth, &RenderGrid::maxContentForChild, &GridTrack::maxBreadthIfNotInfinite, &GridTrack::growMaxBreadth);
    }

    for (size_t i = 0; i < tracks.size(); ++i) {
        GridTrack& track = tracks[i];
        if (track.m_maxBreadth == infinity)
            track.m_maxBreadth = track.m_usedBreadth;

        availableLogicalSpace -= track.m_usedBreadth;
    }
}

void RenderGrid::resolveContentBasedTrackSizingFunctionsForItems(TrackSizingDirection direction, Vector<GridTrack>& columnTracks, Vector<GridTrack>& rowTracks, RenderBox* gridItem, FilterFunction filterFunction, SizingFunction sizingFunction, AccumulatorGetter trackGetter, AccumulatorGrowFunction trackGrowthFunction)
{
    const GridCoordinate coordinate = cachedGridCoordinate(gridItem);
    const size_t initialTrackIndex = (direction == ForColumns) ? coordinate.columns.initialPositionIndex : coordinate.rows.initialPositionIndex;
    const size_t finalTrackIndex = (direction == ForColumns) ? coordinate.columns.finalPositionIndex : coordinate.rows.finalPositionIndex;

    Vector<GridTrack*> tracks;
    for (size_t trackIndex = initialTrackIndex; trackIndex <= finalTrackIndex; ++trackIndex) {
        const GridTrackSize& trackSize = gridTrackSize(direction, trackIndex);
        if (!(trackSize.*filterFunction)())
            continue;

        GridTrack& track = (direction == ForColumns) ? columnTracks[trackIndex] : rowTracks[trackIndex];
        tracks.append(&track);
    }

    LayoutUnit additionalBreadthSpace = (this->*sizingFunction)(gridItem, direction, columnTracks);
    for (size_t trackIndexForSpace = initialTrackIndex; trackIndexForSpace <= finalTrackIndex; ++trackIndexForSpace) {
        GridTrack& track = (direction == ForColumns) ? columnTracks[trackIndexForSpace] : rowTracks[trackIndexForSpace];
        additionalBreadthSpace -= (track.*trackGetter)();
    }

    // FIXME: We should pass different values for |tracksForGrowthAboveMaxBreadth|.
    distributeSpaceToTracks(tracks, &tracks, trackGetter, trackGrowthFunction, additionalBreadthSpace);
}

static bool sortByGridTrackGrowthPotential(const GridTrack* track1, const GridTrack* track2)
{
    return (track1->m_maxBreadth - track1->m_usedBreadth) < (track2->m_maxBreadth - track2->m_usedBreadth);
}

void RenderGrid::distributeSpaceToTracks(Vector<GridTrack*>& tracks, Vector<GridTrack*>* tracksForGrowthAboveMaxBreadth, AccumulatorGetter trackGetter, AccumulatorGrowFunction trackGrowthFunction, LayoutUnit& availableLogicalSpace)
{
    std::sort(tracks.begin(), tracks.end(), sortByGridTrackGrowthPotential);

    size_t tracksSize = tracks.size();
    Vector<LayoutUnit> updatedTrackBreadths(tracksSize);

    for (size_t i = 0; i < tracksSize; ++i) {
        GridTrack& track = *tracks[i];
        LayoutUnit availableLogicalSpaceShare = availableLogicalSpace / (tracksSize - i);
        LayoutUnit trackBreadth = (tracks[i]->*trackGetter)();
        LayoutUnit growthShare = std::max(LayoutUnit(), std::min(availableLogicalSpaceShare, track.m_maxBreadth - trackBreadth));
        // We should never shrink any grid track or else we can't guarantee we abide by our min-sizing function.
        updatedTrackBreadths[i] = trackBreadth + growthShare;
        availableLogicalSpace -= growthShare;
    }

    if (availableLogicalSpace > 0 && tracksForGrowthAboveMaxBreadth) {
        tracksSize = tracksForGrowthAboveMaxBreadth->size();
        for (size_t i = 0; i < tracksSize; ++i) {
            LayoutUnit growthShare = availableLogicalSpace / (tracksSize - i);
            updatedTrackBreadths[i] += growthShare;
            availableLogicalSpace -= growthShare;
        }
    }

    for (size_t i = 0; i < tracksSize; ++i) {
        LayoutUnit growth = updatedTrackBreadths[i] - (tracks[i]->*trackGetter)();
        if (growth >= 0)
            (tracks[i]->*trackGrowthFunction)(growth);
    }
}

#ifndef NDEBUG
bool RenderGrid::tracksAreWiderThanMinTrackBreadth(TrackSizingDirection direction, const Vector<GridTrack>& tracks)
{
    for (size_t i = 0; i < tracks.size(); ++i) {
        const GridTrackSize& trackSize = gridTrackSize(direction, i);
        const GridLength& minTrackBreadth = trackSize.minTrackBreadth();
        if (computeUsedBreadthOfMinLength(direction, minTrackBreadth) > tracks[i].m_usedBreadth)
            return false;
    }
    return true;
}
#endif

void RenderGrid::growGrid(TrackSizingDirection direction)
{
    if (direction == ForColumns) {
        const size_t oldColumnSize = m_grid[0].size();
        for (size_t row = 0; row < m_grid.size(); ++row)
            m_grid[row].grow(oldColumnSize + 1);
    } else {
        const size_t oldRowSize = m_grid.size();
        m_grid.grow(oldRowSize + 1);
        m_grid[oldRowSize].grow(m_grid[0].size());
    }
}

void RenderGrid::insertItemIntoGrid(RenderBox* child, const GridCoordinate& coordinate)
{
    m_grid[coordinate.rows.initialPositionIndex][coordinate.columns.initialPositionIndex].append(child);
    m_gridItemCoordinate.set(child, coordinate);
}

void RenderGrid::insertItemIntoGrid(RenderBox* child, size_t rowTrack, size_t columnTrack)
{
    const GridSpan& rowSpan = resolveGridPositionsFromAutoPlacementPosition(child, ForRows, rowTrack);
    const GridSpan& columnSpan = resolveGridPositionsFromAutoPlacementPosition(child, ForColumns, columnTrack);
    insertItemIntoGrid(child, GridCoordinate(rowSpan, columnSpan));
}

void RenderGrid::placeItemsOnGrid()
{
    ASSERT(!gridWasPopulated());
    ASSERT(m_gridItemCoordinate.isEmpty());

    populateExplicitGridAndOrderIterator();

    Vector<RenderBox*> autoMajorAxisAutoGridItems;
    Vector<RenderBox*> specifiedMajorAxisAutoGridItems;
    GridAutoFlow autoFlow = style().gridAutoFlow();
    for (RenderBox* child = m_orderIterator.first(); child; child = m_orderIterator.next()) {
        // FIXME: We never re-resolve positions if the grid is grown during auto-placement which may lead auto / <integer>
        // positions to not match the author's intent. The specification is unclear on what should be done in this case.
        OwnPtr<GridSpan> rowPositions = resolveGridPositionsFromStyle(child, ForRows);
        OwnPtr<GridSpan> columnPositions = resolveGridPositionsFromStyle(child, ForColumns);
        if (!rowPositions || !columnPositions) {
            GridSpan* majorAxisPositions = (autoPlacementMajorAxisDirection() == ForColumns) ? columnPositions.get() : rowPositions.get();
            if (!majorAxisPositions)
                autoMajorAxisAutoGridItems.append(child);
            else
                specifiedMajorAxisAutoGridItems.append(child);
            continue;
        }
        insertItemIntoGrid(child, GridCoordinate(*rowPositions, *columnPositions));
    }

    ASSERT(gridRowCount() >= style().gridRows().size());
    ASSERT(gridColumnCount() >= style().gridColumns().size());

    if (autoFlow == AutoFlowNone) {
        // If we did collect some grid items, they won't be placed thus never laid out.
        ASSERT(!autoMajorAxisAutoGridItems.size());
        ASSERT(!specifiedMajorAxisAutoGridItems.size());
        return;
    }

    placeSpecifiedMajorAxisItemsOnGrid(specifiedMajorAxisAutoGridItems);
    placeAutoMajorAxisItemsOnGrid(autoMajorAxisAutoGridItems);
}

void RenderGrid::populateExplicitGridAndOrderIterator()
{
    // FIXME: We should find a way to share OrderValues's initialization code with RenderFlexibleBox.
    OrderIterator::OrderValues orderValues;
    size_t maximumRowIndex = std::max<size_t>(1, explicitGridRowCount());
    size_t maximumColumnIndex = std::max<size_t>(1, explicitGridColumnCount());

    for (RenderBox* child = firstChildBox(); child; child = child->nextSiblingBox()) {
        // Avoid growing the vector for the common-case default value of 0. This optimizes the most common case which is
        // one or a few values with the default order 0
        int order = child->style().order();
        if (orderValues.isEmpty() || orderValues.last() != order)
            orderValues.append(order);

        // This function bypasses the cache (cachedGridCoordinate()) as it is used to build it.
        OwnPtr<GridSpan> rowPositions = resolveGridPositionsFromStyle(child, ForRows);
        OwnPtr<GridSpan> columnPositions = resolveGridPositionsFromStyle(child, ForColumns);

        // |positions| is 0 if we need to run the auto-placement algorithm. Our estimation ignores
        // this case as the auto-placement algorithm will grow the grid as needed.
        if (rowPositions)
            maximumRowIndex = std::max(maximumRowIndex, rowPositions->finalPositionIndex + 1);
        if (columnPositions)
            maximumColumnIndex = std::max(maximumColumnIndex, columnPositions->finalPositionIndex + 1);
    }

    m_grid.grow(maximumRowIndex);
    for (size_t i = 0; i < m_grid.size(); ++i)
        m_grid[i].grow(maximumColumnIndex);

    m_orderIterator.setOrderValues(std::move(orderValues));
}

void RenderGrid::placeSpecifiedMajorAxisItemsOnGrid(Vector<RenderBox*> autoGridItems)
{
    for (size_t i = 0; i < autoGridItems.size(); ++i) {
        OwnPtr<GridSpan> majorAxisPositions = resolveGridPositionsFromStyle(autoGridItems[i], autoPlacementMajorAxisDirection());
        GridIterator iterator(m_grid, autoPlacementMajorAxisDirection(), majorAxisPositions->initialPositionIndex);
        if (OwnPtr<GridCoordinate> emptyGridArea = iterator.nextEmptyGridArea()) {
            insertItemIntoGrid(autoGridItems[i], emptyGridArea->rows.initialPositionIndex, emptyGridArea->columns.initialPositionIndex);
            continue;
        }

        growGrid(autoPlacementMinorAxisDirection());
        OwnPtr<GridCoordinate> emptyGridArea = iterator.nextEmptyGridArea();
        ASSERT(emptyGridArea);
        insertItemIntoGrid(autoGridItems[i], emptyGridArea->rows.initialPositionIndex, emptyGridArea->columns.initialPositionIndex);
    }
}

void RenderGrid::placeAutoMajorAxisItemsOnGrid(Vector<RenderBox*> autoGridItems)
{
    for (size_t i = 0; i < autoGridItems.size(); ++i)
        placeAutoMajorAxisItemOnGrid(autoGridItems[i]);
}

void RenderGrid::placeAutoMajorAxisItemOnGrid(RenderBox* gridItem)
{
    OwnPtr<GridSpan> minorAxisPositions = resolveGridPositionsFromStyle(gridItem, autoPlacementMinorAxisDirection());
    ASSERT(!resolveGridPositionsFromStyle(gridItem, autoPlacementMajorAxisDirection()));
    size_t minorAxisIndex = 0;
    if (minorAxisPositions) {
        minorAxisIndex = minorAxisPositions->initialPositionIndex;
        GridIterator iterator(m_grid, autoPlacementMinorAxisDirection(), minorAxisIndex);
        if (OwnPtr<GridCoordinate> emptyGridArea = iterator.nextEmptyGridArea()) {
            insertItemIntoGrid(gridItem, emptyGridArea->rows.initialPositionIndex, emptyGridArea->columns.initialPositionIndex);
            return;
        }
    } else {
        const size_t endOfMajorAxis = (autoPlacementMajorAxisDirection() == ForColumns) ? gridColumnCount() : gridRowCount();
        for (size_t majorAxisIndex = 0; majorAxisIndex < endOfMajorAxis; ++majorAxisIndex) {
            GridIterator iterator(m_grid, autoPlacementMajorAxisDirection(), majorAxisIndex);
            if (OwnPtr<GridCoordinate> emptyGridArea = iterator.nextEmptyGridArea()) {
                insertItemIntoGrid(gridItem, emptyGridArea->rows.initialPositionIndex, emptyGridArea->columns.initialPositionIndex);
                return;
            }
        }
    }

    // We didn't find an empty grid area so we need to create an extra major axis line and insert our gridItem in it.
    const size_t columnIndex = (autoPlacementMajorAxisDirection() == ForColumns) ? m_grid[0].size() : minorAxisIndex;
    const size_t rowIndex = (autoPlacementMajorAxisDirection() == ForColumns) ? minorAxisIndex : m_grid.size();
    growGrid(autoPlacementMajorAxisDirection());
    insertItemIntoGrid(gridItem, rowIndex, columnIndex);
}

RenderGrid::TrackSizingDirection RenderGrid::autoPlacementMajorAxisDirection() const
{
    GridAutoFlow flow = style().gridAutoFlow();
    ASSERT(flow != AutoFlowNone);
    return (flow == AutoFlowColumn) ? ForColumns : ForRows;
}

RenderGrid::TrackSizingDirection RenderGrid::autoPlacementMinorAxisDirection() const
{
    GridAutoFlow flow = style().gridAutoFlow();
    ASSERT(flow != AutoFlowNone);
    return (flow == AutoFlowColumn) ? ForRows : ForColumns;
}

void RenderGrid::clearGrid()
{
    m_grid.clear();
    m_gridItemCoordinate.clear();
}

void RenderGrid::layoutGridItems()
{
    placeItemsOnGrid();

    Vector<GridTrack> columnTracks(gridColumnCount());
    Vector<GridTrack> rowTracks(gridRowCount());
    computedUsedBreadthOfGridTracks(ForColumns, columnTracks, rowTracks);
    ASSERT(tracksAreWiderThanMinTrackBreadth(ForColumns, columnTracks));
    computedUsedBreadthOfGridTracks(ForRows, columnTracks, rowTracks);
    ASSERT(tracksAreWiderThanMinTrackBreadth(ForRows, rowTracks));

    for (RenderBox* child = firstChildBox(); child; child = child->nextSiblingBox()) {
        LayoutPoint childPosition = findChildLogicalPosition(child, columnTracks, rowTracks);

        // Because the grid area cannot be styled, we don't need to adjust
        // the grid breadth to account for 'box-sizing'.
        LayoutUnit oldOverrideContainingBlockContentLogicalWidth = child->hasOverrideContainingBlockLogicalWidth() ? child->overrideContainingBlockContentLogicalWidth() : LayoutUnit();
        LayoutUnit oldOverrideContainingBlockContentLogicalHeight = child->hasOverrideContainingBlockLogicalHeight() ? child->overrideContainingBlockContentLogicalHeight() : LayoutUnit();

        // FIXME: For children in a content sized track, we clear the overrideContainingBlockContentLogicalHeight
        // in minContentForChild / maxContentForChild which means that we will always relayout the child.
        LayoutUnit overrideContainingBlockContentLogicalWidth = gridAreaBreadthForChild(child, ForColumns, columnTracks);
        LayoutUnit overrideContainingBlockContentLogicalHeight = gridAreaBreadthForChild(child, ForRows, rowTracks);
        if (oldOverrideContainingBlockContentLogicalWidth != overrideContainingBlockContentLogicalWidth || oldOverrideContainingBlockContentLogicalHeight != overrideContainingBlockContentLogicalHeight)
            child->setNeedsLayout(MarkOnlyThis);

        child->setOverrideContainingBlockContentLogicalWidth(overrideContainingBlockContentLogicalWidth);
        child->setOverrideContainingBlockContentLogicalHeight(overrideContainingBlockContentLogicalHeight);

        LayoutRect oldChildRect = child->frameRect();

        // FIXME: Grid items should stretch to fill their cells. Once we
        // implement grid-{column,row}-align, we can also shrink to fit. For
        // now, just size as if we were a regular child.
        child->layoutIfNeeded();

        // FIXME: Handle border & padding on the grid element.
        child->setLogicalLocation(childPosition);

        // If the child moved, we have to repaint it as well as any floating/positioned
        // descendants. An exception is if we need a layout. In this case, we know we're going to
        // repaint ourselves (and the child) anyway.
        if (!selfNeedsLayout() && child->checkForRepaintDuringLayout())
            child->repaintDuringLayoutIfMoved(oldChildRect);
    }

    for (size_t i = 0; i < rowTracks.size(); ++i)
        setLogicalHeight(logicalHeight() + rowTracks[i].m_usedBreadth);

    // FIXME: We should handle min / max logical height.

    setLogicalHeight(logicalHeight() + borderAndPaddingLogicalHeight());
    clearGrid();
}

GridCoordinate RenderGrid::cachedGridCoordinate(const RenderBox* gridItem) const
{
    ASSERT(m_gridItemCoordinate.contains(gridItem));
    return m_gridItemCoordinate.get(gridItem);
}

GridSpan RenderGrid::resolveGridPositionsFromAutoPlacementPosition(const RenderBox*, TrackSizingDirection, size_t initialPosition) const
{
    // FIXME: We don't support spanning with auto positions yet. Once we do, this is wrong. Also we should make
    // sure the grid can accomodate the new item as we only grow 1 position in a given direction.
    return GridSpan(initialPosition, initialPosition);
}

PassOwnPtr<GridSpan> RenderGrid::resolveGridPositionsFromStyle(const RenderBox* gridItem, TrackSizingDirection direction) const
{
    const GridPosition& initialPosition = (direction == ForColumns) ? gridItem->style().gridItemColumnStart() : gridItem->style().gridItemRowStart();
    const GridPositionSide initialPositionSide = (direction == ForColumns) ? ColumnStartSide : RowStartSide;
    const GridPosition& finalPosition = (direction == ForColumns) ? gridItem->style().gridItemColumnEnd() : gridItem->style().gridItemRowEnd();
    const GridPositionSide finalPositionSide = (direction == ForColumns) ? ColumnEndSide : RowEndSide;

    // We should NEVER see both spans as they should have been handled during style resolve.
    ASSERT(!initialPosition.isSpan() || !finalPosition.isSpan());

    if (initialPosition.shouldBeResolvedAgainstOppositePosition() && finalPosition.shouldBeResolvedAgainstOppositePosition()) {
        if (style().gridAutoFlow() == AutoFlowNone)
            return adoptPtr(new GridSpan(0, 0));

        // We can't get our grid positions without running the auto placement algorithm.
        return nullptr;
    }

    if (initialPosition.shouldBeResolvedAgainstOppositePosition()) {
        // Infer the position from the final position ('auto / 1' or 'span 2 / 3' case).
        const size_t finalResolvedPosition = resolveGridPositionFromStyle(finalPosition, finalPositionSide);
        return resolveGridPositionAgainstOppositePosition(finalResolvedPosition, initialPosition, initialPositionSide);
    }

    if (finalPosition.shouldBeResolvedAgainstOppositePosition()) {
        // Infer our position from the initial position ('1 / auto' or '3 / span 2' case).
        const size_t initialResolvedPosition = resolveGridPositionFromStyle(initialPosition, initialPositionSide);
        return resolveGridPositionAgainstOppositePosition(initialResolvedPosition, finalPosition, finalPositionSide);
    }

    size_t resolvedInitialPosition = resolveGridPositionFromStyle(initialPosition, initialPositionSide);
    size_t resolvedFinalPosition = resolveGridPositionFromStyle(finalPosition, finalPositionSide);

    // If 'grid-row-end' specifies a line at or before that specified by 'grid-row-start', it computes to 'span 1'.
    if (resolvedFinalPosition < resolvedInitialPosition)
        resolvedFinalPosition = resolvedInitialPosition;

    return adoptPtr(new GridSpan(resolvedInitialPosition, resolvedFinalPosition));
}

inline static size_t adjustGridPositionForRowEndColumnEndSide(size_t resolvedPosition)
{
    return resolvedPosition ? resolvedPosition - 1 : 0;
}

static size_t adjustGridPositionForSide(size_t resolvedPosition, RenderGrid::GridPositionSide side)
{
    // An item finishing on the N-th line belongs to the N-1-th cell.
    if (side == RenderGrid::ColumnEndSide || side == RenderGrid::RowEndSide)
        return adjustGridPositionForRowEndColumnEndSide(resolvedPosition);

    return resolvedPosition;
}

size_t RenderGrid::resolveNamedGridLinePositionFromStyle(const GridPosition& position, GridPositionSide side) const
{
    ASSERT(!position.namedGridLine().isNull());

    const NamedGridLinesMap& gridLinesNames = (side == ColumnStartSide || side == ColumnEndSide) ? style().namedGridColumnLines() : style().namedGridRowLines();
    NamedGridLinesMap::const_iterator it = gridLinesNames.find(position.namedGridLine());
    if (it == gridLinesNames.end()) {
        if (position.isPositive())
            return 0;
        const size_t lastLine = explicitGridSizeForSide(side);
        return adjustGridPositionForSide(lastLine, side);
    }

    size_t namedGridLineIndex;
    if (position.isPositive())
        namedGridLineIndex = std::min<size_t>(position.integerPosition(), it->value.size()) - 1;
    else
        namedGridLineIndex = std::max<int>(it->value.size() - abs(position.integerPosition()), 0);
    return adjustGridPositionForSide(it->value[namedGridLineIndex], side);
}

size_t RenderGrid::resolveGridPositionFromStyle(const GridPosition& position, GridPositionSide side) const
{
    // FIXME: Handle other values for grid-{row,column} like ranges or line names.
    switch (position.type()) {
    case ExplicitPosition: {
        ASSERT(position.integerPosition());

        if (!position.namedGridLine().isNull())
            return resolveNamedGridLinePositionFromStyle(position, side);

        // Handle <integer> explicit position.
        if (position.isPositive())
            return adjustGridPositionForSide(position.integerPosition() - 1, side);

        size_t resolvedPosition = abs(position.integerPosition()) - 1;
        const size_t endOfTrack = explicitGridSizeForSide(side);

        // Per http://lists.w3.org/Archives/Public/www-style/2013Mar/0589.html, we clamp negative value to the first line.
        if (endOfTrack < resolvedPosition)
            return 0;

        return adjustGridPositionForSide(endOfTrack - resolvedPosition, side);
    }
    case AutoPosition:
        // 'auto' depends on the opposite position for resolution (e.g. grid-row: auto / 1).
        ASSERT_NOT_REACHED();
        return 0;
    case SpanPosition:
        // FIXME: Handle span positions.
        ASSERT_NOT_REACHED();
        return 0;
    }
    ASSERT_NOT_REACHED();
    return 0;
}

PassOwnPtr<GridSpan> RenderGrid::resolveGridPositionAgainstOppositePosition(size_t resolvedOppositePosition, const GridPosition& position, GridPositionSide side) const
{
    if (position.isAuto())
        return GridSpan::create(resolvedOppositePosition, resolvedOppositePosition);

    ASSERT(position.isSpan());
    ASSERT(position.spanPosition() > 0);

    if (!position.namedGridLine().isNull()) {
        // span 2 'c' -> we need to find the appropriate grid line before / after our opposite position.
        return resolveNamedGridLinePositionAgainstOppositePosition(resolvedOppositePosition, position, side);
    }

    // 'span 1' is contained inside a single grid track regardless of the direction.
    // That's why the CSS span value is one more than the offset we apply.
    size_t positionOffset = position.spanPosition() - 1;
    if (side == ColumnStartSide || side == RowStartSide) {
        size_t initialResolvedPosition = std::max<int>(0, resolvedOppositePosition - positionOffset);
        return GridSpan::create(initialResolvedPosition, resolvedOppositePosition);
    }

    return GridSpan::create(resolvedOppositePosition, resolvedOppositePosition + positionOffset);
}

PassOwnPtr<GridSpan> RenderGrid::resolveNamedGridLinePositionAgainstOppositePosition(size_t resolvedOppositePosition, const GridPosition& position, GridPositionSide side) const
{
    ASSERT(position.isSpan());
    ASSERT(!position.namedGridLine().isNull());
    // Negative positions are not allowed per the specification and should have been handled during parsing.
    ASSERT(position.spanPosition() > 0);

    const NamedGridLinesMap& gridLinesNames = (side == ColumnStartSide || side == ColumnEndSide) ? style().namedGridColumnLines() : style().namedGridRowLines();
    NamedGridLinesMap::const_iterator it = gridLinesNames.find(position.namedGridLine());

    // If there is no named grid line of that name, we resolve the position to 'auto' (which is equivalent to 'span 1' in this case).
    // See http://lists.w3.org/Archives/Public/www-style/2013Jun/0394.html.
    if (it == gridLinesNames.end())
        return GridSpan::create(resolvedOppositePosition, resolvedOppositePosition);

    if (side == RowStartSide || side == ColumnStartSide)
        return resolveRowStartColumnStartNamedGridLinePositionAgainstOppositePosition(resolvedOppositePosition, position, it->value);

    return resolveRowEndColumnEndNamedGridLinePositionAgainstOppositePosition(resolvedOppositePosition, position, it->value);
}

PassOwnPtr<GridSpan> RenderGrid::resolveRowStartColumnStartNamedGridLinePositionAgainstOppositePosition(size_t resolvedOppositePosition, const GridPosition& position, const Vector<size_t>& gridLines) const
{
    // The grid line inequality needs to be strict (which doesn't match the after / end case) because |resolvedOppositePosition|
    // is already converted to an index in our grid representation (ie one was removed from the grid line to account for the side).
    // FIXME: This could be a binary search as |gridLines| is ordered.
    int firstLineBeforeOppositePositionIndex = gridLines.size() - 1;
    for (; firstLineBeforeOppositePositionIndex >= 0 && gridLines[firstLineBeforeOppositePositionIndex] > resolvedOppositePosition; --firstLineBeforeOppositePositionIndex) { }

    size_t gridLineIndex = std::max<int>(0, firstLineBeforeOppositePositionIndex - position.spanPosition() + 1);
    size_t resolvedGridLinePosition = gridLines[gridLineIndex];
    if (resolvedGridLinePosition > resolvedOppositePosition)
        resolvedGridLinePosition = resolvedOppositePosition;
    return GridSpan::create(resolvedGridLinePosition, resolvedOppositePosition);
}

PassOwnPtr<GridSpan> RenderGrid::resolveRowEndColumnEndNamedGridLinePositionAgainstOppositePosition(size_t resolvedOppositePosition, const GridPosition& position, const Vector<size_t>& gridLines) const
{
    // FIXME: This could be a binary search as |gridLines| is ordered.
    size_t firstLineAfterOppositePositionIndex = 0;
    for (; firstLineAfterOppositePositionIndex < gridLines.size() && gridLines[firstLineAfterOppositePositionIndex] <= resolvedOppositePosition; ++firstLineAfterOppositePositionIndex) { }

    size_t gridLineIndex = std::min(gridLines.size() - 1, firstLineAfterOppositePositionIndex + position.spanPosition() - 1);
    size_t resolvedGridLinePosition = adjustGridPositionForRowEndColumnEndSide(gridLines[gridLineIndex]);
    if (resolvedGridLinePosition < resolvedOppositePosition)
        resolvedGridLinePosition = resolvedOppositePosition;
    return GridSpan::create(resolvedOppositePosition, resolvedGridLinePosition);
}

LayoutUnit RenderGrid::gridAreaBreadthForChild(const RenderBox* child, TrackSizingDirection direction, const Vector<GridTrack>& tracks) const
{
    const GridCoordinate& coordinate = cachedGridCoordinate(child);
    const GridSpan& span = (direction == ForColumns) ? coordinate.columns : coordinate.rows;
    LayoutUnit gridAreaBreadth = 0;
    for (size_t trackIndex = span.initialPositionIndex; trackIndex <= span.finalPositionIndex; ++trackIndex)
        gridAreaBreadth += tracks[trackIndex].m_usedBreadth;
    return gridAreaBreadth;
}

LayoutPoint RenderGrid::findChildLogicalPosition(RenderBox* child, const Vector<GridTrack>& columnTracks, const Vector<GridTrack>& rowTracks)
{
    const GridCoordinate& coordinate = cachedGridCoordinate(child);

    // The grid items should be inside the grid container's border box, that's why they need to be shifted.
    LayoutPoint offset(borderAndPaddingStart(), borderAndPaddingBefore());
    // FIXME: |columnTrack| and |rowTrack| should be smaller than our column / row count.
    for (size_t i = 0; i < coordinate.columns.initialPositionIndex && i < columnTracks.size(); ++i)
        offset.setX(offset.x() + columnTracks[i].m_usedBreadth);
    for (size_t i = 0; i < coordinate.rows.initialPositionIndex && i < rowTracks.size(); ++i)
        offset.setY(offset.y() + rowTracks[i].m_usedBreadth);

    // FIXME: Handle margins on the grid item.
    return offset;
}

void RenderGrid::paintChildren(PaintInfo& paintInfo, const LayoutPoint& paintOffset, PaintInfo& forChild, bool usePrintRect)
{
    for (RenderBox* child = m_orderIterator.first(); child; child = m_orderIterator.next())
        paintChild(*child, paintInfo, paintOffset, forChild, usePrintRect);
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
