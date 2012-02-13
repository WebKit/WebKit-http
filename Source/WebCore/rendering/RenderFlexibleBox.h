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

#ifndef RenderFlexibleBox_h
#define RenderFlexibleBox_h

#include "RenderBlock.h"

namespace WebCore {

class RenderFlexibleBox : public RenderBlock {
public:
    RenderFlexibleBox(Node*);
    virtual ~RenderFlexibleBox();

    virtual const char* renderName() const;

    virtual bool isFlexibleBox() const { return true; }
    virtual void computePreferredLogicalWidths();
    virtual void layoutBlock(bool relayoutChildren, int pageLogicalHeight = 0, BlockLayoutPass = NormalLayoutPass);

    bool isHorizontalFlow() const;

private:
    struct FlexOrderHashTraits;
    typedef HashSet<int, DefaultHash<int>::Hash, FlexOrderHashTraits> FlexOrderHashSet;

    class FlexOrderIterator;
    typedef WTF::HashMap<const RenderBox*, LayoutUnit> InflexibleFlexItemSize;
    typedef WTF::Vector<RenderBox*> OrderedFlexItemList;

    bool hasOrthogonalFlow(RenderBox* child) const;
    bool isColumnFlow() const;
    bool isLeftToRightFlow() const;
    Length crossAxisLength() const;
    Length mainAxisLengthForChild(RenderBox* child) const;
    void setCrossAxisExtent(LayoutUnit);
    LayoutUnit crossAxisExtentForChild(RenderBox* child);
    LayoutUnit mainAxisExtentForChild(RenderBox* child);
    LayoutUnit crossAxisExtent() const;
    LayoutUnit mainAxisExtent() const;
    LayoutUnit crossAxisContentExtent() const;
    LayoutUnit mainAxisContentExtent() const;
    WritingMode transformedWritingMode() const;
    LayoutUnit flowAwareBorderStart() const;
    LayoutUnit flowAwareBorderBefore() const;
    LayoutUnit flowAwareBorderEnd() const;
    LayoutUnit crossAxisBorderAndPaddingExtent() const;
    LayoutUnit flowAwarePaddingStart() const;
    LayoutUnit flowAwarePaddingBefore() const;
    LayoutUnit flowAwarePaddingEnd() const;
    LayoutUnit flowAwareMarginStartForChild(RenderBox* child) const;
    LayoutUnit flowAwareMarginEndForChild(RenderBox* child) const;
    LayoutUnit flowAwareMarginBeforeForChild(RenderBox* child) const;
    LayoutUnit flowAwareMarginAfterForChild(RenderBox* child) const;
    LayoutUnit crossAxisMarginExtentForChild(RenderBox* child) const;
    LayoutUnit crossAxisScrollbarExtent() const;
    LayoutPoint flowAwareLocationForChild(RenderBox* child) const;
    // FIXME: Supporting layout deltas.
    void setFlowAwareLocationForChild(RenderBox* child, const LayoutPoint&);
    void adjustAlignmentForChild(RenderBox* child, LayoutUnit);
    LayoutUnit mainAxisBorderAndPaddingExtentForChild(RenderBox* child) const;
    LayoutUnit mainAxisScrollbarExtentForChild(RenderBox* child) const;
    LayoutUnit preferredMainAxisContentExtentForChild(RenderBox* child) const;

    void layoutFlexItems(bool relayoutChildren);

    float positiveFlexForChild(RenderBox* child) const;
    float negativeFlexForChild(RenderBox* child) const;

    LayoutUnit availableAlignmentSpaceForChild(RenderBox*);
    LayoutUnit marginBoxAscent(RenderBox*);

    void computeMainAxisPreferredSizes(bool relayoutChildren, FlexOrderHashSet&);
    void computeFlexOrder(FlexOrderIterator&, OrderedFlexItemList& orderedChildren, LayoutUnit& preferredMainAxisExtent, float& totalPositiveFlexibility, float& totalNegativeFlexibility);
    bool runFreeSpaceAllocationAlgorithm(const OrderedFlexItemList&, LayoutUnit& availableFreeSpace, float& totalPositiveFlexibility, float& totalNegativeFlexibility, InflexibleFlexItemSize&, WTF::Vector<LayoutUnit>& childSizes);
    void setLogicalOverrideSize(RenderBox* child, LayoutUnit childPreferredSize);
    void prepareChildForPositionedLayout(RenderBox* child, LayoutUnit mainAxisOffset, LayoutUnit crossAxisOffset);
    void layoutAndPlaceChildren(const OrderedFlexItemList&, const WTF::Vector<LayoutUnit>& childSizes, LayoutUnit availableFreeSpace);
    void layoutColumnReverse(const OrderedFlexItemList&, const WTF::Vector<LayoutUnit>& childSizes, LayoutUnit availableFreeSpace);
    void alignChildren(const OrderedFlexItemList&, LayoutUnit maxAscent);
};

} // namespace WebCore

#endif // RenderFlexibleBox_h
