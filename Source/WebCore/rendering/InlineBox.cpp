/*
 * Copyright (C) 2003, 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
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
#include "InlineBox.h"

#include "FontMetrics.h"
#include "Frame.h"
#include "HitTestResult.h"
#include "InlineFlowBox.h"
#include "Page.h"
#include "PaintInfo.h"
#include "RenderBlockFlow.h"
#include "RenderLineBreak.h"
#include "RootInlineBox.h"

#ifndef NDEBUG
#include <stdio.h>
#endif

using namespace std;

namespace WebCore {

struct SameSizeAsInlineBox {
    virtual ~SameSizeAsInlineBox() { }
    void* a[4];
    FloatPoint b;
    float c;
    uint32_t d : 32;
#if !ASSERT_DISABLED
    bool f;
#endif
};

COMPILE_ASSERT(sizeof(InlineBox) == sizeof(SameSizeAsInlineBox), InlineBox_size_guard);

#if !ASSERT_DISABLED
InlineBox::~InlineBox()
{
    if (!m_hasBadParent && m_parent)
        m_parent->setHasBadChildList();
}
#endif

void InlineBox::removeFromParent()
{ 
    if (parent())
        parent()->removeChild(this);
}

#ifndef NDEBUG
const char* InlineBox::boxName() const
{
    return "InlineBox";
}

void InlineBox::showTreeForThis() const
{
    m_renderer.showTreeForThis();
}

void InlineBox::showLineTreeForThis() const
{
    m_renderer.containingBlock()->showLineTreeAndMark(this, "*");
}

void InlineBox::showLineTreeAndMark(const InlineBox* markedBox1, const char* markedLabel1, const InlineBox* markedBox2, const char* markedLabel2, const RenderObject* obj, int depth) const
{
    int printedCharacters = 0;
    if (this == markedBox1)
        printedCharacters += fprintf(stderr, "%s", markedLabel1);
    if (this == markedBox2)
        printedCharacters += fprintf(stderr, "%s", markedLabel2);
    if (&m_renderer == obj)
        printedCharacters += fprintf(stderr, "*");
    for (; printedCharacters < depth * 2; printedCharacters++)
        fputc(' ', stderr);

    showBox(printedCharacters);
}

void InlineBox::showBox(int printedCharacters) const
{
    printedCharacters += fprintf(stderr, "%s\t%p", boxName(), this);
    for (; printedCharacters < showTreeCharacterOffset; printedCharacters++)
        fputc(' ', stderr);
    fprintf(stderr, "\t%s %p\n", renderer().renderName(), &renderer());
}
#endif

float InlineBox::logicalHeight() const
{
    if (hasVirtualLogicalHeight())
        return virtualLogicalHeight();

    const RenderStyle& lineStyle = this->lineStyle();
    if (renderer().isTextOrLineBreak())
        return behavesLikeText() ? lineStyle.fontMetrics().height() : 0;
    if (renderer().isBox() && parent())
        return isHorizontal() ? toRenderBox(renderer()).height() : toRenderBox(renderer()).width();

    ASSERT(isInlineFlowBox());
    RenderBoxModelObject* flowObject = boxModelObject();
    const FontMetrics& fontMetrics = lineStyle.fontMetrics();
    float result = fontMetrics.height();
    if (parent())
        result += flowObject->borderAndPaddingLogicalHeight();
    return result;
}

int InlineBox::baselinePosition(FontBaseline baselineType) const
{
    if (renderer().isLineBreak() && !behavesLikeText())
        return 0;
    return boxModelObject()->baselinePosition(baselineType, m_bitfields.firstLine(), isHorizontal() ? HorizontalLine : VerticalLine, PositionOnContainingLine);
}

LayoutUnit InlineBox::lineHeight() const
{
    if (renderer().isLineBreak() && !behavesLikeText())
        return 0;
    return boxModelObject()->lineHeight(m_bitfields.firstLine(), isHorizontal() ? HorizontalLine : VerticalLine, PositionOnContainingLine);
}

int InlineBox::caretMinOffset() const 
{ 
    return m_renderer.caretMinOffset();
}

int InlineBox::caretMaxOffset() const 
{ 
    return m_renderer.caretMaxOffset();
}

void InlineBox::dirtyLineBoxes()
{
    markDirty();
    for (InlineFlowBox* curr = parent(); curr && !curr->isDirty(); curr = curr->parent())
        curr->markDirty();
}

void InlineBox::deleteLine()
{
    if (!m_bitfields.extracted()) {
        if (m_renderer.isBox())
            toRenderBox(renderer()).setInlineBoxWrapper(0);
        else if (renderer().isLineBreak())
            toRenderLineBreak(renderer()).setInlineBoxWrapper(0);
    }
    delete this;
}

void InlineBox::extractLine()
{
    m_bitfields.setExtracted(true);
    if (m_renderer.isBox())
        toRenderBox(renderer()).setInlineBoxWrapper(0);
    else if (renderer().isLineBreak())
        toRenderLineBreak(renderer()).setInlineBoxWrapper(0);
}

void InlineBox::attachLine()
{
    m_bitfields.setExtracted(false);
    if (m_renderer.isBox())
        toRenderBox(renderer()).setInlineBoxWrapper(this);
    else if (renderer().isLineBreak())
        toRenderLineBreak(renderer()).setInlineBoxWrapper(this);
}

void InlineBox::adjustPosition(float dx, float dy)
{
    m_topLeft.move(dx, dy);

    if (m_renderer.isReplaced())
        toRenderBox(renderer()).move(dx, dy);
}

void InlineBox::paint(PaintInfo& paintInfo, const LayoutPoint& paintOffset, LayoutUnit /* lineTop */, LayoutUnit /*lineBottom*/)
{
    RenderElement& renderer = toRenderElement(this->renderer());
    if (!paintInfo.shouldPaintWithinRoot(renderer) || (paintInfo.phase != PaintPhaseForeground && paintInfo.phase != PaintPhaseSelection))
        return;

    LayoutPoint childPoint = paintOffset;
    if (parent()->renderer().style().isFlippedBlocksWritingMode() && renderer.isBox()) // Faster than calling containingBlock().
        childPoint = m_renderer.containingBlock()->flipForWritingModeForChild(&toRenderBox(renderer), childPoint);
    
    // Paint all phases of replaced elements atomically, as though the replaced element established its
    // own stacking context.  (See Appendix E.2, section 6.4 on inline block/table elements in the CSS2.1
    // specification.)
    bool preservePhase = paintInfo.phase == PaintPhaseSelection || paintInfo.phase == PaintPhaseTextClip;
    PaintInfo info(paintInfo);
    info.phase = preservePhase ? paintInfo.phase : PaintPhaseBlockBackground;
    renderer.paint(info, childPoint);
    if (!preservePhase) {
        info.phase = PaintPhaseChildBlockBackgrounds;
        renderer.paint(info, childPoint);
        info.phase = PaintPhaseFloat;
        renderer.paint(info, childPoint);
        info.phase = PaintPhaseForeground;
        renderer.paint(info, childPoint);
        info.phase = PaintPhaseOutline;
        renderer.paint(info, childPoint);
    }
}

bool InlineBox::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, LayoutUnit /* lineTop */, LayoutUnit /*lineBottom*/)
{
    // Hit test all phases of replaced elements atomically, as though the replaced element established its
    // own stacking context.  (See Appendix E.2, section 6.4 on inline block/table elements in the CSS2.1
    // specification.)
    LayoutPoint childPoint = accumulatedOffset;
    if (parent()->renderer().style().isFlippedBlocksWritingMode() && !renderer().isLineBreak()) // Faster than calling containingBlock().
        childPoint = m_renderer.containingBlock()->flipForWritingModeForChild(&toRenderBox(renderer()), childPoint);
    
    return m_renderer.hitTest(request, result, locationInContainer, childPoint);
}

const RootInlineBox& InlineBox::root() const
{ 
    if (m_parent)
        return m_parent->root(); 
    ASSERT_WITH_SECURITY_IMPLICATION(isRootInlineBox());
    return static_cast<const RootInlineBox&>(*this);
}

RootInlineBox& InlineBox::root()
{ 
    if (m_parent)
        return m_parent->root(); 
    ASSERT_WITH_SECURITY_IMPLICATION(isRootInlineBox());
    return static_cast<RootInlineBox&>(*this);
}

bool InlineBox::nextOnLineExists() const
{
    if (!m_bitfields.determinedIfNextOnLineExists()) {
        m_bitfields.setDeterminedIfNextOnLineExists(true);

        if (!parent())
            m_bitfields.setNextOnLineExists(false);
        else if (nextOnLine())
            m_bitfields.setNextOnLineExists(true);
        else
            m_bitfields.setNextOnLineExists(parent()->nextOnLineExists());
    }
    return m_bitfields.nextOnLineExists();
}

InlineBox* InlineBox::nextLeafChild() const
{
    InlineBox* leaf = 0;
    for (InlineBox* box = nextOnLine(); box && !leaf; box = box->nextOnLine())
        leaf = box->isLeaf() ? box : toInlineFlowBox(box)->firstLeafChild();
    if (!leaf && parent())
        leaf = parent()->nextLeafChild();
    return leaf;
}
    
InlineBox* InlineBox::prevLeafChild() const
{
    InlineBox* leaf = 0;
    for (InlineBox* box = prevOnLine(); box && !leaf; box = box->prevOnLine())
        leaf = box->isLeaf() ? box : toInlineFlowBox(box)->lastLeafChild();
    if (!leaf && parent())
        leaf = parent()->prevLeafChild();
    return leaf;
}

InlineBox* InlineBox::nextLeafChildIgnoringLineBreak() const
{
    InlineBox* leaf = nextLeafChild();
    if (leaf && leaf->isLineBreak())
        return 0;
    return leaf;
}

InlineBox* InlineBox::prevLeafChildIgnoringLineBreak() const
{
    InlineBox* leaf = prevLeafChild();
    if (leaf && leaf->isLineBreak())
        return 0;
    return leaf;
}

RenderObject::SelectionState InlineBox::selectionState()
{
    return m_renderer.selectionState();
}

bool InlineBox::canAccommodateEllipsis(bool ltr, int blockEdge, int ellipsisWidth) const
{
    // Non-replaced elements can always accommodate an ellipsis.
    if (!m_renderer.isReplaced())
        return true;
    
    IntRect boxRect(left(), 0, m_logicalWidth, 10);
    IntRect ellipsisRect(ltr ? blockEdge - ellipsisWidth : blockEdge, 0, ellipsisWidth, 10);
    return !(boxRect.intersects(ellipsisRect));
}

float InlineBox::placeEllipsisBox(bool, float, float, float, float& truncatedWidth, bool&)
{
    // Use -1 to mean "we didn't set the position."
    truncatedWidth += logicalWidth();
    return -1;
}

void InlineBox::clearKnownToHaveNoOverflow()
{ 
    m_bitfields.setKnownToHaveNoOverflow(false);
    if (parent() && parent()->knownToHaveNoOverflow())
        parent()->clearKnownToHaveNoOverflow();
}

FloatPoint InlineBox::locationIncludingFlipping()
{
    if (!m_renderer.style().isFlippedBlocksWritingMode())
        return FloatPoint(x(), y());
    RenderBlockFlow& block = root().blockFlow();
    if (block.style().isHorizontalWritingMode())
        return FloatPoint(x(), block.height() - height() - y());
    else
        return FloatPoint(block.width() - width() - x(), y());
}

void InlineBox::flipForWritingMode(FloatRect& rect)
{
    if (!m_renderer.style().isFlippedBlocksWritingMode())
        return;
    root().blockFlow().flipForWritingMode(rect);
}

FloatPoint InlineBox::flipForWritingMode(const FloatPoint& point)
{
    if (!m_renderer.style().isFlippedBlocksWritingMode())
        return point;
    return root().blockFlow().flipForWritingMode(point);
}

void InlineBox::flipForWritingMode(LayoutRect& rect)
{
    if (!m_renderer.style().isFlippedBlocksWritingMode())
        return;
    root().blockFlow().flipForWritingMode(rect);
}

LayoutPoint InlineBox::flipForWritingMode(const LayoutPoint& point)
{
    if (!m_renderer.style().isFlippedBlocksWritingMode())
        return point;
    return root().blockFlow().flipForWritingMode(point);
}

} // namespace WebCore

#ifndef NDEBUG

void showTree(const WebCore::InlineBox* b)
{
    if (b)
        b->showTreeForThis();
}

void showLineTree(const WebCore::InlineBox* b)
{
    if (b)
        b->showLineTreeForThis();
}

#endif
