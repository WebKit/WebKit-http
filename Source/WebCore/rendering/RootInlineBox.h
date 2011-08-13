/*
 * Copyright (C) 2003, 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef RootInlineBox_h
#define RootInlineBox_h

#include "BidiContext.h"
#include "InlineFlowBox.h"

namespace WebCore {

class EllipsisBox;
class HitTestResult;

struct BidiStatus;
struct GapRects;

class RootInlineBox : public InlineFlowBox {
public:
    RootInlineBox(RenderBlock* block);

    virtual void destroy(RenderArena*);

    virtual bool isRootInlineBox() const { return true; }

    void detachEllipsisBox(RenderArena*);

    RootInlineBox* nextRootBox() const { return static_cast<RootInlineBox*>(m_nextLineBox); }
    RootInlineBox* prevRootBox() const { return static_cast<RootInlineBox*>(m_prevLineBox); }

    virtual void adjustPosition(float dx, float dy);

    LayoutUnit lineTop() const { return m_lineTop; }
    LayoutUnit lineBottom() const { return m_lineBottom; }

    int paginationStrut() const { return m_paginationStrut; }
    void setPaginationStrut(int s) { m_paginationStrut = s; }

    LayoutUnit selectionTop() const;
    LayoutUnit selectionBottom() const;
    LayoutUnit selectionHeight() const { return max<LayoutUnit>(0, selectionBottom() - selectionTop()); }

    int blockDirectionPointInLine() const { return max(lineTop(), selectionTop()); }

    LayoutUnit alignBoxesInBlockDirection(LayoutUnit heightOfBlock, GlyphOverflowAndFallbackFontsMap&, VerticalPositionCache&);
    void setLineTopBottomPositions(LayoutUnit top, LayoutUnit bottom)
    { 
        m_lineTop = top; 
        m_lineBottom = bottom; 
    }

    virtual RenderLineBoxList* rendererLineBoxes() const;

    RenderObject* lineBreakObj() const { return m_lineBreakObj; }
    BidiStatus lineBreakBidiStatus() const;
    void setLineBreakInfo(RenderObject*, unsigned breakPos, const BidiStatus&);

    unsigned lineBreakPos() const { return m_lineBreakPos; }
    void setLineBreakPos(unsigned p) { m_lineBreakPos = p; }

    int blockLogicalHeight() const { return m_blockLogicalHeight; }
    void setBlockLogicalHeight(int h) { m_blockLogicalHeight = h; }

    bool endsWithBreak() const { return m_endsWithBreak; }
    void setEndsWithBreak(bool b) { m_endsWithBreak = b; }

    void childRemoved(InlineBox* box);

    bool lineCanAccommodateEllipsis(bool ltr, int blockEdge, int lineBoxEdge, int ellipsisWidth);
    void placeEllipsis(const AtomicString& ellipsisStr, bool ltr, float blockLeftEdge, float blockRightEdge, float ellipsisWidth, InlineBox* markupBox = 0);
    virtual float placeEllipsisBox(bool ltr, float blockLeftEdge, float blockRightEdge, float ellipsisWidth, bool& foundBox);

    bool hasEllipsisBox() const { return m_hasEllipsisBoxOrHyphen; }
    EllipsisBox* ellipsisBox() const;

    void paintEllipsisBox(PaintInfo&, const LayoutPoint&, LayoutUnit lineTop, LayoutUnit lineBottom) const;

    virtual void clearTruncation();

    virtual int baselinePosition(FontBaseline baselineType) const { return boxModelObject()->baselinePosition(baselineType, m_firstLine, isHorizontal() ? HorizontalLine : VerticalLine, PositionOfInteriorLineBoxes); }
    virtual int lineHeight() const { return boxModelObject()->lineHeight(m_firstLine, isHorizontal() ? HorizontalLine : VerticalLine, PositionOfInteriorLineBoxes); }

#if PLATFORM(MAC)
    void addHighlightOverflow();
    void paintCustomHighlight(PaintInfo&, const LayoutPoint&, const AtomicString& highlightType);
#endif

    virtual void paint(PaintInfo&, const LayoutPoint&, LayoutUnit lineTop, LayoutUnit lineBottom);
    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, const LayoutPoint& pointInContainer, const LayoutPoint& accumulatedOffset, LayoutUnit lineTop, LayoutUnit lineBottom);

    bool hasSelectedChildren() const { return m_hasSelectedChildrenOrCanHaveLeadingExpansion; }
    void setHasSelectedChildren(bool hasSelectedChildren) { m_hasSelectedChildrenOrCanHaveLeadingExpansion = hasSelectedChildren; }

    virtual RenderObject::SelectionState selectionState();
    InlineBox* firstSelectedBox();
    InlineBox* lastSelectedBox();

    GapRects lineSelectionGap(RenderBlock* rootBlock, const LayoutPoint& rootBlockPhysicalPosition, const LayoutSize& offsetFromRootBlock, LayoutUnit selTop, LayoutUnit selHeight, const PaintInfo*);

    RenderBlock* block() const;

    InlineBox* closestLeafChildForPoint(const IntPoint&, bool onlyEditableLeaves);
    InlineBox* closestLeafChildForLogicalLeftPosition(int, bool onlyEditableLeaves = false);

    void appendFloat(RenderBox* floatingBox)
    {
        ASSERT(!isDirty());
        if (m_floats)
            m_floats->append(floatingBox);
        else
            m_floats= adoptPtr(new Vector<RenderBox*>(1, floatingBox));
    }

    Vector<RenderBox*>* floatsPtr() { ASSERT(!isDirty()); return m_floats.get(); }

    virtual void extractLineBoxFromRenderObject();
    virtual void attachLineBoxToRenderObject();
    virtual void removeLineBoxFromRenderObject();
    
    FontBaseline baselineType() const { return static_cast<FontBaseline>(m_baselineType); }

    bool hasAnnotationsBefore() const { return m_hasAnnotationsBefore; }
    bool hasAnnotationsAfter() const { return m_hasAnnotationsAfter; }

    LayoutRect paddedLayoutOverflowRect(LayoutUnit endPadding) const;

    void ascentAndDescentForBox(InlineBox*, GlyphOverflowAndFallbackFontsMap&, LayoutUnit& ascent, LayoutUnit& descent, bool& affectsAscent, bool& affectsDescent) const;
    LayoutUnit verticalPositionForBox(InlineBox*, VerticalPositionCache&);
    bool includeLeadingForBox(InlineBox*) const;
    bool includeFontForBox(InlineBox*) const;
    bool includeGlyphsForBox(InlineBox*) const;
    bool includeMarginForBox(InlineBox*) const;
    bool fitsToGlyphs() const;
    bool includesRootLineBoxFontOrLeading() const;
    
    LayoutUnit logicalTopVisualOverflow() const
    {
        return InlineFlowBox::logicalTopVisualOverflow(lineTop());
    }
    LayoutUnit logicalBottomVisualOverflow() const
    {
        return InlineFlowBox::logicalBottomVisualOverflow(lineBottom());
    }
    LayoutUnit logicalTopLayoutOverflow() const
    {
        return InlineFlowBox::logicalTopLayoutOverflow(lineTop());
    }
    LayoutUnit logicalBottomLayoutOverflow() const
    {
        return InlineFlowBox::logicalBottomLayoutOverflow(lineBottom());
    }

    Node* getLogicalStartBoxWithNode(InlineBox*&) const;
    Node* getLogicalEndBoxWithNode(InlineBox*&) const;
#ifndef NDEBUG
    virtual const char* boxName() const;
#endif
private:
    void setHasEllipsisBox(bool hasEllipsisBox) { m_hasEllipsisBoxOrHyphen = hasEllipsisBox; }

    int beforeAnnotationsAdjustment() const;

    // Where this line ended.  The exact object and the position within that object are stored so that
    // we can create an InlineIterator beginning just after the end of this line.
    RenderObject* m_lineBreakObj;
    unsigned m_lineBreakPos;
    RefPtr<BidiContext> m_lineBreakContext;

    LayoutUnit m_lineTop;
    LayoutUnit m_lineBottom;

    int m_paginationStrut;

    // Floats hanging off the line are pushed into this vector during layout. It is only
    // good for as long as the line has not been marked dirty.
    OwnPtr<Vector<RenderBox*> > m_floats;

    // The logical height of the block at the end of this line.  This is where the next line starts.
    int m_blockLogicalHeight;

    // Whether or not this line uses alphabetic or ideographic baselines by default.
    unsigned m_baselineType : 1; // FontBaseline
    
    // If the line contains any ruby runs, then this will be true.
    bool m_hasAnnotationsBefore : 1;
    bool m_hasAnnotationsAfter : 1;

    WTF::Unicode::Direction m_lineBreakBidiStatusEor : 5;
    WTF::Unicode::Direction m_lineBreakBidiStatusLastStrong : 5;
    WTF::Unicode::Direction m_lineBreakBidiStatusLast : 5;
};

} // namespace WebCore

#endif // RootInlineBox_h
