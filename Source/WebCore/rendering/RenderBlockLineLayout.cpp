/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2003, 2004, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All right reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#include "BidiResolver.h"
#include "Hyphenation.h"
#include "InlineIterator.h"
#include "InlineTextBox.h"
#include "Logging.h"
#include "RenderArena.h"
#include "RenderCombineText.h"
#include "RenderFlowThread.h"
#include "RenderInline.h"
#include "RenderLayer.h"
#include "RenderListMarker.h"
#include "RenderRubyRun.h"
#include "RenderView.h"
#include "Settings.h"
#include "TrailingFloatsRootInlineBox.h"
#include "VerticalPositionCache.h"
#include "break_lines.h"
#include <wtf/AlwaysInline.h>
#include <wtf/RefCountedLeakCounter.h>
#include <wtf/StdLibExtras.h>
#include <wtf/Vector.h>
#include <wtf/unicode/CharacterNames.h>

#if ENABLE(CSS_EXCLUSIONS)
#include "WrapShapeInfo.h"
#endif

#if ENABLE(SVG)
#include "RenderSVGInlineText.h"
#include "SVGRootInlineBox.h"
#endif

using namespace std;
using namespace WTF;
using namespace Unicode;

namespace WebCore {

// We don't let our line box tree for a single line get any deeper than this.
const unsigned cMaxLineDepth = 200;

class LineWidth {
public:
    LineWidth(RenderBlock* block, bool isFirstLine)
        : m_block(block)
        , m_uncommittedWidth(0)
        , m_committedWidth(0)
        , m_overhangWidth(0)
        , m_left(0)
        , m_right(0)
        , m_availableWidth(0)
#if ENABLE(CSS_EXCLUSIONS)
        , m_segment(0)
#endif
        , m_isFirstLine(isFirstLine)
    {
        ASSERT(block);
#if ENABLE(CSS_EXCLUSIONS)
        WrapShapeInfo* wrapShapeInfo = m_block->wrapShapeInfo();
        // FIXME: Bug 91878: Add support for multiple segments, currently we only support one
        if (wrapShapeInfo && wrapShapeInfo->lineState() == WrapShapeInfo::LINE_INSIDE_SHAPE) {
            // All interior shape positions should have at least one segment
            ASSERT(wrapShapeInfo->hasSegments());
            m_segment = &wrapShapeInfo->segments()[0];
        }
#endif
        updateAvailableWidth();
    }
#if ENABLE(SUBPIXEL_LAYOUT)
    bool fitsOnLine() const { return currentWidth() <= m_availableWidth + LayoutUnit::epsilon(); }
    bool fitsOnLine(float extra) const { return currentWidth() + extra <= m_availableWidth + LayoutUnit::epsilon(); }
#else
    bool fitsOnLine() const { return currentWidth() <= m_availableWidth; }
    bool fitsOnLine(float extra) const { return currentWidth() + extra <= m_availableWidth; }
#endif
    float currentWidth() const { return m_committedWidth + m_uncommittedWidth; }

    // FIXME: We should eventually replace these three functions by ones that work on a higher abstraction.
    float uncommittedWidth() const { return m_uncommittedWidth; }
    float committedWidth() const { return m_committedWidth; }
    float availableWidth() const { return m_availableWidth; }

    void updateAvailableWidth();
    void shrinkAvailableWidthForNewFloatIfNeeded(RenderBlock::FloatingObject*);
    void addUncommittedWidth(float delta) { m_uncommittedWidth += delta; }
    void commit()
    {
        m_committedWidth += m_uncommittedWidth;
        m_uncommittedWidth = 0;
    }
    void applyOverhang(RenderRubyRun*, RenderObject* startRenderer, RenderObject* endRenderer);
    void fitBelowFloats();

private:
    void computeAvailableWidthFromLeftAndRight()
    {
        m_availableWidth = max(0.0f, m_right - m_left) + m_overhangWidth;
    }

private:
    RenderBlock* m_block;
    float m_uncommittedWidth;
    float m_committedWidth;
    float m_overhangWidth; // The amount by which |m_availableWidth| has been inflated to account for possible contraction due to ruby overhang.
    float m_left;
    float m_right;
    float m_availableWidth;
#if ENABLE(CSS_EXCLUSIONS)
    const LineSegment* m_segment;
#endif
    bool m_isFirstLine;
};

static LayoutUnit logicalHeightForLine(RenderBlock* block)
{
    InlineFlowBox* lineBox = block->firstRootBox();
    LayoutUnit logicalHeight = 0;
    if (!lineBox)
        return logicalHeight;

    if (lineBox->firstChild() && lineBox->firstChild()->renderer() && lineBox->firstChild()->renderer()->isRenderBlock())
        logicalHeight = toRenderBlock(lineBox->firstChild()->renderer())->logicalHeight();
    else
        logicalHeight = lineBox->height();
    return logicalHeight;
}

inline void LineWidth::updateAvailableWidth()
{
    LayoutUnit height = m_block->logicalHeight();
    LayoutUnit logicalHeight = logicalHeightForLine(m_block);
    m_left = m_block->logicalLeftOffsetForLine(height, m_isFirstLine, logicalHeight);
    m_right = m_block->logicalRightOffsetForLine(height, m_isFirstLine, logicalHeight);

#if ENABLE(CSS_EXCLUSIONS)
    if (m_segment) {
        m_left = max<float>(m_segment->logicalLeft, m_left);
        m_right = min<float>(m_segment->logicalRight, m_right);
    }
#endif

    computeAvailableWidthFromLeftAndRight();
}

inline void LineWidth::shrinkAvailableWidthForNewFloatIfNeeded(RenderBlock::FloatingObject* newFloat)
{
    LayoutUnit height = m_block->logicalHeight();
    if (height < m_block->logicalTopForFloat(newFloat) || height >= m_block->logicalBottomForFloat(newFloat))
        return;

    if (newFloat->type() == RenderBlock::FloatingObject::FloatLeft) {
        m_left = m_block->pixelSnappedLogicalRightForFloat(newFloat);
        if (m_isFirstLine && m_block->style()->isLeftToRightDirection())
            m_left += floorToInt(m_block->textIndentOffset());
    } else {
        m_right = m_block->pixelSnappedLogicalLeftForFloat(newFloat);
        if (m_isFirstLine && !m_block->style()->isLeftToRightDirection())
            m_right -= floorToInt(m_block->textIndentOffset());
    }

    computeAvailableWidthFromLeftAndRight();
}

void LineWidth::applyOverhang(RenderRubyRun* rubyRun, RenderObject* startRenderer, RenderObject* endRenderer)
{
    int startOverhang;
    int endOverhang;
    rubyRun->getOverhang(m_isFirstLine, startRenderer, endRenderer, startOverhang, endOverhang);

    startOverhang = min<int>(startOverhang, m_committedWidth);
    m_availableWidth += startOverhang;

    endOverhang = max(min<int>(endOverhang, m_availableWidth - currentWidth()), 0);
    m_availableWidth += endOverhang;
    m_overhangWidth += startOverhang + endOverhang;
}

void LineWidth::fitBelowFloats()
{
    ASSERT(!m_committedWidth);
    ASSERT(!fitsOnLine());

    LayoutUnit floatLogicalBottom;
    LayoutUnit lastFloatLogicalBottom = m_block->logicalHeight();
    float newLineWidth = m_availableWidth;
    float newLineLeft = m_left;
    float newLineRight = m_right;
    while (true) {
        floatLogicalBottom = m_block->nextFloatLogicalBottomBelow(lastFloatLogicalBottom);
        if (floatLogicalBottom <= lastFloatLogicalBottom)
            break;

        newLineLeft = m_block->logicalLeftOffsetForLine(floatLogicalBottom, m_isFirstLine);
        newLineRight = m_block->logicalRightOffsetForLine(floatLogicalBottom, m_isFirstLine);
        newLineWidth = max(0.0f, newLineRight - newLineLeft);
        lastFloatLogicalBottom = floatLogicalBottom;
        if (newLineWidth >= m_uncommittedWidth)
            break;
    }

    if (newLineWidth > m_availableWidth) {
        m_block->setLogicalHeight(lastFloatLogicalBottom);
        m_availableWidth = newLineWidth + m_overhangWidth;
        m_left = newLineLeft;
        m_right = newLineRight;
    }
}

class LineInfo {
public:
    LineInfo()
        : m_isFirstLine(true)
        , m_isLastLine(false)
        , m_isEmpty(true)
        , m_previousLineBrokeCleanly(true)
        , m_floatPaginationStrut(0)
        , m_runsFromLeadingWhitespace(0)
    { }

    bool isFirstLine() const { return m_isFirstLine; }
    bool isLastLine() const { return m_isLastLine; }
    bool isEmpty() const { return m_isEmpty; }
    bool previousLineBrokeCleanly() const { return m_previousLineBrokeCleanly; }
    LayoutUnit floatPaginationStrut() const { return m_floatPaginationStrut; }
    unsigned runsFromLeadingWhitespace() const { return m_runsFromLeadingWhitespace; }
    void resetRunsFromLeadingWhitespace() { m_runsFromLeadingWhitespace = 0; }
    void incrementRunsFromLeadingWhitespace() { m_runsFromLeadingWhitespace++; }

    void setFirstLine(bool firstLine) { m_isFirstLine = firstLine; }
    void setLastLine(bool lastLine) { m_isLastLine = lastLine; }
    void setEmpty(bool empty, RenderBlock* block = 0, LineWidth* lineWidth = 0)
    {
        if (m_isEmpty == empty)
            return;
        m_isEmpty = empty;
        if (!empty && block && floatPaginationStrut()) {
            block->setLogicalHeight(block->logicalHeight() + floatPaginationStrut());
            setFloatPaginationStrut(0);
            lineWidth->updateAvailableWidth();
        }
    }

    void setPreviousLineBrokeCleanly(bool previousLineBrokeCleanly) { m_previousLineBrokeCleanly = previousLineBrokeCleanly; }
    void setFloatPaginationStrut(LayoutUnit strut) { m_floatPaginationStrut = strut; }

private:
    bool m_isFirstLine;
    bool m_isLastLine;
    bool m_isEmpty;
    bool m_previousLineBrokeCleanly;
    LayoutUnit m_floatPaginationStrut;
    unsigned m_runsFromLeadingWhitespace;
};

static inline LayoutUnit borderPaddingMarginStart(RenderInline* child)
{
    return child->marginStart() + child->paddingStart() + child->borderStart();
}

static inline LayoutUnit borderPaddingMarginEnd(RenderInline* child)
{
    return child->marginEnd() + child->paddingEnd() + child->borderEnd();
}

static LayoutUnit inlineLogicalWidth(RenderObject* child, bool start = true, bool end = true)
{
    unsigned lineDepth = 1;
    LayoutUnit extraWidth = 0;
    RenderObject* parent = child->parent();
    while (parent->isRenderInline() && lineDepth++ < cMaxLineDepth) {
        RenderInline* parentAsRenderInline = toRenderInline(parent);
        if (start && !child->previousSibling())
            extraWidth += borderPaddingMarginStart(parentAsRenderInline);
        if (end && !child->nextSibling())
            extraWidth += borderPaddingMarginEnd(parentAsRenderInline);
        child = parent;
        parent = child->parent();
    }
    return extraWidth;
}

static void determineDirectionality(TextDirection& dir, InlineIterator iter)
{
    while (!iter.atEnd()) {
        if (iter.atParagraphSeparator())
            return;
        if (UChar current = iter.current()) {
            Direction charDirection = direction(current);
            if (charDirection == LeftToRight) {
                dir = LTR;
                return;
            }
            if (charDirection == RightToLeft || charDirection == RightToLeftArabic) {
                dir = RTL;
                return;
            }
        }
        iter.increment();
    }
}

static void checkMidpoints(LineMidpointState& lineMidpointState, InlineIterator& lBreak)
{
    // Check to see if our last midpoint is a start point beyond the line break.  If so,
    // shave it off the list, and shave off a trailing space if the previous end point doesn't
    // preserve whitespace.
    if (lBreak.m_obj && lineMidpointState.numMidpoints && !(lineMidpointState.numMidpoints % 2)) {
        InlineIterator* midpoints = lineMidpointState.midpoints.data();
        InlineIterator& endpoint = midpoints[lineMidpointState.numMidpoints - 2];
        const InlineIterator& startpoint = midpoints[lineMidpointState.numMidpoints - 1];
        InlineIterator currpoint = endpoint;
        while (!currpoint.atEnd() && currpoint != startpoint && currpoint != lBreak)
            currpoint.increment();
        if (currpoint == lBreak) {
            // We hit the line break before the start point.  Shave off the start point.
            lineMidpointState.numMidpoints--;
            if (endpoint.m_obj->style()->collapseWhiteSpace())
                endpoint.m_pos--;
        }
    }
}

static void addMidpoint(LineMidpointState& lineMidpointState, const InlineIterator& midpoint)
{
    if (lineMidpointState.midpoints.size() <= lineMidpointState.numMidpoints)
        lineMidpointState.midpoints.grow(lineMidpointState.numMidpoints + 10);

    InlineIterator* midpoints = lineMidpointState.midpoints.data();
    midpoints[lineMidpointState.numMidpoints++] = midpoint;
}

static inline BidiRun* createRun(int start, int end, RenderObject* obj, InlineBidiResolver& resolver)
{
    return new (obj->renderArena()) BidiRun(start, end, obj, resolver.context(), resolver.dir());
}

void RenderBlock::appendRunsForObject(BidiRunList<BidiRun>& runs, int start, int end, RenderObject* obj, InlineBidiResolver& resolver)
{
    if (start > end || shouldSkipCreatingRunsForObject(obj))
        return;

    LineMidpointState& lineMidpointState = resolver.midpointState();
    bool haveNextMidpoint = (lineMidpointState.currentMidpoint < lineMidpointState.numMidpoints);
    InlineIterator nextMidpoint;
    if (haveNextMidpoint)
        nextMidpoint = lineMidpointState.midpoints[lineMidpointState.currentMidpoint];
    if (lineMidpointState.betweenMidpoints) {
        if (!(haveNextMidpoint && nextMidpoint.m_obj == obj))
            return;
        // This is a new start point. Stop ignoring objects and
        // adjust our start.
        lineMidpointState.betweenMidpoints = false;
        start = nextMidpoint.m_pos;
        lineMidpointState.currentMidpoint++;
        if (start < end)
            return appendRunsForObject(runs, start, end, obj, resolver);
    } else {
        if (!haveNextMidpoint || (obj != nextMidpoint.m_obj)) {
            runs.addRun(createRun(start, end, obj, resolver));
            return;
        }

        // An end midpoint has been encountered within our object.  We
        // need to go ahead and append a run with our endpoint.
        if (static_cast<int>(nextMidpoint.m_pos + 1) <= end) {
            lineMidpointState.betweenMidpoints = true;
            lineMidpointState.currentMidpoint++;
            if (nextMidpoint.m_pos != UINT_MAX) { // UINT_MAX means stop at the object and don't include any of it.
                if (static_cast<int>(nextMidpoint.m_pos + 1) > start)
                    runs.addRun(createRun(start, nextMidpoint.m_pos + 1, obj, resolver));
                return appendRunsForObject(runs, nextMidpoint.m_pos + 1, end, obj, resolver);
            }
        } else
           runs.addRun(createRun(start, end, obj, resolver));
    }
}

static inline InlineBox* createInlineBoxForRenderer(RenderObject* obj, bool isRootLineBox, bool isOnlyRun = false)
{
    if (isRootLineBox)
        return toRenderBlock(obj)->createAndAppendRootInlineBox();

    if (obj->isText()) {
        InlineTextBox* textBox = toRenderText(obj)->createInlineTextBox();
        // We only treat a box as text for a <br> if we are on a line by ourself or in strict mode
        // (Note the use of strict mode.  In "almost strict" mode, we don't treat the box for <br> as text.)
        if (obj->isBR())
            textBox->setIsText(isOnlyRun || obj->document()->inNoQuirksMode());
        return textBox;
    }

    if (obj->isBox())
        return toRenderBox(obj)->createInlineBox();

    return toRenderInline(obj)->createAndAppendInlineFlowBox();
}

static inline void dirtyLineBoxesForRenderer(RenderObject* o, bool fullLayout)
{
    if (o->isText()) {
        RenderText* renderText = toRenderText(o);
        renderText->updateTextIfNeeded(); // FIXME: Counters depend on this hack. No clue why. Should be investigated and removed.
        renderText->dirtyLineBoxes(fullLayout);
    } else
        toRenderInline(o)->dirtyLineBoxes(fullLayout);
}

static bool parentIsConstructedOrHaveNext(InlineFlowBox* parentBox)
{
    do {
        if (parentBox->isConstructed() || parentBox->nextOnLine())
            return true;
        parentBox = parentBox->parent();
    } while (parentBox);
    return false;
}

InlineFlowBox* RenderBlock::createLineBoxes(RenderObject* obj, const LineInfo& lineInfo, InlineBox* childBox)
{
    // See if we have an unconstructed line box for this object that is also
    // the last item on the line.
    unsigned lineDepth = 1;
    InlineFlowBox* parentBox = 0;
    InlineFlowBox* result = 0;
    bool hasDefaultLineBoxContain = style()->lineBoxContain() == RenderStyle::initialLineBoxContain();
    do {
        ASSERT(obj->isRenderInline() || obj == this);

        RenderInline* inlineFlow = (obj != this) ? toRenderInline(obj) : 0;

        // Get the last box we made for this render object.
        parentBox = inlineFlow ? inlineFlow->lastLineBox() : toRenderBlock(obj)->lastLineBox();

        // If this box or its ancestor is constructed then it is from a previous line, and we need
        // to make a new box for our line.  If this box or its ancestor is unconstructed but it has
        // something following it on the line, then we know we have to make a new box
        // as well.  In this situation our inline has actually been split in two on
        // the same line (this can happen with very fancy language mixtures).
        bool constructedNewBox = false;
        bool allowedToConstructNewBox = !hasDefaultLineBoxContain || !inlineFlow || inlineFlow->alwaysCreateLineBoxes();
        bool canUseExistingParentBox = parentBox && !parentIsConstructedOrHaveNext(parentBox);
        if (allowedToConstructNewBox && !canUseExistingParentBox) {
            // We need to make a new box for this render object.  Once
            // made, we need to place it at the end of the current line.
            InlineBox* newBox = createInlineBoxForRenderer(obj, obj == this);
            ASSERT(newBox->isInlineFlowBox());
            parentBox = toInlineFlowBox(newBox);
            parentBox->setFirstLineStyleBit(lineInfo.isFirstLine());
            parentBox->setIsHorizontal(isHorizontalWritingMode());
            if (!hasDefaultLineBoxContain)
                parentBox->clearDescendantsHaveSameLineHeightAndBaseline();
            constructedNewBox = true;
        }

        if (constructedNewBox || canUseExistingParentBox) {
            if (!result)
                result = parentBox;

            // If we have hit the block itself, then |box| represents the root
            // inline box for the line, and it doesn't have to be appended to any parent
            // inline.
            if (childBox)
                parentBox->addToLine(childBox);

            if (!constructedNewBox || obj == this)
                break;

            childBox = parentBox;
        }

        // If we've exceeded our line depth, then jump straight to the root and skip all the remaining
        // intermediate inline flows.
        obj = (++lineDepth >= cMaxLineDepth) ? this : obj->parent();

    } while (true);

    return result;
}

static bool reachedEndOfTextRenderer(const BidiRunList<BidiRun>& bidiRuns)
{
    BidiRun* run = bidiRuns.logicallyLastRun();
    if (!run)
        return true;
    unsigned int pos = run->stop();
    RenderObject* r = run->m_object;
    if (!r->isText() || r->isBR())
        return false;
    RenderText* renderText = toRenderText(r);
    if (pos >= renderText->textLength())
        return true;

    while (isASCIISpace(renderText->characters()[pos])) {
        pos++;
        if (pos >= renderText->textLength())
            return true;
    }
    return false;
}

RootInlineBox* RenderBlock::constructLine(BidiRunList<BidiRun>& bidiRuns, const LineInfo& lineInfo)
{
    ASSERT(bidiRuns.firstRun());

    bool rootHasSelectedChildren = false;
    InlineFlowBox* parentBox = 0;
    int runCount = bidiRuns.runCount() - lineInfo.runsFromLeadingWhitespace();
    for (BidiRun* r = bidiRuns.firstRun(); r; r = r->next()) {
        // Create a box for our object.
        bool isOnlyRun = (runCount == 1);
        if (runCount == 2 && !r->m_object->isListMarker())
            isOnlyRun = (!style()->isLeftToRightDirection() ? bidiRuns.lastRun() : bidiRuns.firstRun())->m_object->isListMarker();

        if (lineInfo.isEmpty())
            continue;

        InlineBox* box = createInlineBoxForRenderer(r->m_object, false, isOnlyRun);
        r->m_box = box;

        ASSERT(box);
        if (!box)
            continue;

        if (!rootHasSelectedChildren && box->renderer()->selectionState() != RenderObject::SelectionNone)
            rootHasSelectedChildren = true;

        // If we have no parent box yet, or if the run is not simply a sibling,
        // then we need to construct inline boxes as necessary to properly enclose the
        // run's inline box.
        if (!parentBox || parentBox->renderer() != r->m_object->parent())
            // Create new inline boxes all the way back to the appropriate insertion point.
            parentBox = createLineBoxes(r->m_object->parent(), lineInfo, box);
        else {
            // Append the inline box to this line.
            parentBox->addToLine(box);
        }

        bool visuallyOrdered = r->m_object->style()->rtlOrdering() == VisualOrder;
        box->setBidiLevel(r->level());

        if (box->isInlineTextBox()) {
            InlineTextBox* text = toInlineTextBox(box);
            text->setStart(r->m_start);
            text->setLen(r->m_stop - r->m_start);
            text->setDirOverride(r->dirOverride(visuallyOrdered));
            if (r->m_hasHyphen)
                text->setHasHyphen(true);
        }
    }

    // We should have a root inline box.  It should be unconstructed and
    // be the last continuation of our line list.
    ASSERT(lastLineBox() && !lastLineBox()->isConstructed());

    // Set the m_selectedChildren flag on the root inline box if one of the leaf inline box
    // from the bidi runs walk above has a selection state.
    if (rootHasSelectedChildren)
        lastLineBox()->root()->setHasSelectedChildren(true);

    // Set bits on our inline flow boxes that indicate which sides should
    // paint borders/margins/padding.  This knowledge will ultimately be used when
    // we determine the horizontal positions and widths of all the inline boxes on
    // the line.
    bool isLogicallyLastRunWrapped = bidiRuns.logicallyLastRun()->m_object && bidiRuns.logicallyLastRun()->m_object->isText() ? !reachedEndOfTextRenderer(bidiRuns) : true;
    lastLineBox()->determineSpacingForFlowBoxes(lineInfo.isLastLine(), isLogicallyLastRunWrapped, bidiRuns.logicallyLastRun()->m_object);

    // Now mark the line boxes as being constructed.
    lastLineBox()->setConstructed();

    // Return the last line.
    return lastRootBox();
}

ETextAlign RenderBlock::textAlignmentForLine(bool endsWithSoftBreak) const
{
    ETextAlign alignment = style()->textAlign();
    if (!endsWithSoftBreak && alignment == JUSTIFY)
        alignment = TASTART;

    return alignment;
}

static void updateLogicalWidthForLeftAlignedBlock(bool isLeftToRightDirection, BidiRun* trailingSpaceRun, float& logicalLeft, float& totalLogicalWidth, float availableLogicalWidth)
{
    // The direction of the block should determine what happens with wide lines.
    // In particular with RTL blocks, wide lines should still spill out to the left.
    if (isLeftToRightDirection) {
        if (totalLogicalWidth > availableLogicalWidth && trailingSpaceRun)
            trailingSpaceRun->m_box->setLogicalWidth(max<float>(0, trailingSpaceRun->m_box->logicalWidth() - totalLogicalWidth + availableLogicalWidth));
        return;
    }

    if (trailingSpaceRun)
        trailingSpaceRun->m_box->setLogicalWidth(0);
    else if (totalLogicalWidth > availableLogicalWidth)
        logicalLeft -= (totalLogicalWidth - availableLogicalWidth);
}

static void updateLogicalWidthForRightAlignedBlock(bool isLeftToRightDirection, BidiRun* trailingSpaceRun, float& logicalLeft, float& totalLogicalWidth, float availableLogicalWidth)
{
    // Wide lines spill out of the block based off direction.
    // So even if text-align is right, if direction is LTR, wide lines should overflow out of the right
    // side of the block.
    if (isLeftToRightDirection) {
        if (trailingSpaceRun) {
            totalLogicalWidth -= trailingSpaceRun->m_box->logicalWidth();
            trailingSpaceRun->m_box->setLogicalWidth(0);
        }
        if (totalLogicalWidth < availableLogicalWidth)
            logicalLeft += availableLogicalWidth - totalLogicalWidth;
        return;
    }

    if (totalLogicalWidth > availableLogicalWidth && trailingSpaceRun) {
        trailingSpaceRun->m_box->setLogicalWidth(max<float>(0, trailingSpaceRun->m_box->logicalWidth() - totalLogicalWidth + availableLogicalWidth));
        totalLogicalWidth -= trailingSpaceRun->m_box->logicalWidth();
    } else
        logicalLeft += availableLogicalWidth - totalLogicalWidth;
}

static void updateLogicalWidthForCenterAlignedBlock(bool isLeftToRightDirection, BidiRun* trailingSpaceRun, float& logicalLeft, float& totalLogicalWidth, float availableLogicalWidth)
{
    float trailingSpaceWidth = 0;
    if (trailingSpaceRun) {
        totalLogicalWidth -= trailingSpaceRun->m_box->logicalWidth();
        trailingSpaceWidth = min(trailingSpaceRun->m_box->logicalWidth(), (availableLogicalWidth - totalLogicalWidth + 1) / 2);
        trailingSpaceRun->m_box->setLogicalWidth(max<float>(0, trailingSpaceWidth));
    }
    if (isLeftToRightDirection)
        logicalLeft += max<float>((availableLogicalWidth - totalLogicalWidth) / 2, 0);
    else
        logicalLeft += totalLogicalWidth > availableLogicalWidth ? (availableLogicalWidth - totalLogicalWidth) : (availableLogicalWidth - totalLogicalWidth) / 2 - trailingSpaceWidth;
}

void RenderBlock::setMarginsForRubyRun(BidiRun* run, RenderRubyRun* renderer, RenderObject* previousObject, const LineInfo& lineInfo)
{
    int startOverhang;
    int endOverhang;
    RenderObject* nextObject = 0;
    for (BidiRun* runWithNextObject = run->next(); runWithNextObject; runWithNextObject = runWithNextObject->next()) {
        if (!runWithNextObject->m_object->isOutOfFlowPositioned() && !runWithNextObject->m_box->isLineBreak()) {
            nextObject = runWithNextObject->m_object;
            break;
        }
    }
    renderer->getOverhang(lineInfo.isFirstLine(), renderer->style()->isLeftToRightDirection() ? previousObject : nextObject, renderer->style()->isLeftToRightDirection() ? nextObject : previousObject, startOverhang, endOverhang);
    setMarginStartForChild(renderer, -startOverhang);
    setMarginEndForChild(renderer, -endOverhang);
}

static inline float measureHyphenWidth(RenderText* renderer, const Font& font)
{
    RenderStyle* style = renderer->style();
    return font.width(RenderBlock::constructTextRun(renderer, font, style->hyphenString().string(), style));
}

static inline void setLogicalWidthForTextRun(RootInlineBox* lineBox, BidiRun* run, RenderText* renderer, float xPos, const LineInfo& lineInfo,
                                   GlyphOverflowAndFallbackFontsMap& textBoxDataMap, VerticalPositionCache& verticalPositionCache)
{
    HashSet<const SimpleFontData*> fallbackFonts;
    GlyphOverflow glyphOverflow;
    
    // Always compute glyph overflow if the block's line-box-contain value is "glyphs".
    if (lineBox->fitsToGlyphs()) {
        // If we don't stick out of the root line's font box, then don't bother computing our glyph overflow. This optimization
        // will keep us from computing glyph bounds in nearly all cases.
        bool includeRootLine = lineBox->includesRootLineBoxFontOrLeading();
        int baselineShift = lineBox->verticalPositionForBox(run->m_box, verticalPositionCache);
        int rootDescent = includeRootLine ? lineBox->renderer()->style(lineInfo.isFirstLine())->font().fontMetrics().descent() : 0;
        int rootAscent = includeRootLine ? lineBox->renderer()->style(lineInfo.isFirstLine())->font().fontMetrics().ascent() : 0;
        int boxAscent = renderer->style(lineInfo.isFirstLine())->font().fontMetrics().ascent() - baselineShift;
        int boxDescent = renderer->style(lineInfo.isFirstLine())->font().fontMetrics().descent() + baselineShift;
        if (boxAscent > rootDescent ||  boxDescent > rootAscent)
            glyphOverflow.computeBounds = true; 
    }
    
    LayoutUnit hyphenWidth = 0;
    if (toInlineTextBox(run->m_box)->hasHyphen()) {
        const Font& font = renderer->style(lineInfo.isFirstLine())->font();
        hyphenWidth = measureHyphenWidth(renderer, font);
    }
    run->m_box->setLogicalWidth(renderer->width(run->m_start, run->m_stop - run->m_start, xPos, lineInfo.isFirstLine(), &fallbackFonts, &glyphOverflow) + hyphenWidth);
    if (!fallbackFonts.isEmpty()) {
        ASSERT(run->m_box->isText());
        GlyphOverflowAndFallbackFontsMap::iterator it = textBoxDataMap.add(toInlineTextBox(run->m_box), make_pair(Vector<const SimpleFontData*>(), GlyphOverflow())).iterator;
        ASSERT(it->second.first.isEmpty());
        copyToVector(fallbackFonts, it->second.first);
        run->m_box->parent()->clearDescendantsHaveSameLineHeightAndBaseline();
    }
    if ((glyphOverflow.top || glyphOverflow.bottom || glyphOverflow.left || glyphOverflow.right)) {
        ASSERT(run->m_box->isText());
        GlyphOverflowAndFallbackFontsMap::iterator it = textBoxDataMap.add(toInlineTextBox(run->m_box), make_pair(Vector<const SimpleFontData*>(), GlyphOverflow())).iterator;
        it->second.second = glyphOverflow;
        run->m_box->clearKnownToHaveNoOverflow();
    }
}

static inline void computeExpansionForJustifiedText(BidiRun* firstRun, BidiRun* trailingSpaceRun, Vector<unsigned, 16>& expansionOpportunities, unsigned expansionOpportunityCount, float& totalLogicalWidth, float availableLogicalWidth)
{
    if (!expansionOpportunityCount || availableLogicalWidth <= totalLogicalWidth)
        return;

    size_t i = 0;
    for (BidiRun* r = firstRun; r; r = r->next()) {
        if (!r->m_box || r == trailingSpaceRun)
            continue;
        
        if (r->m_object->isText()) {
            unsigned opportunitiesInRun = expansionOpportunities[i++];
            
            ASSERT(opportunitiesInRun <= expansionOpportunityCount);
            
            // Only justify text if whitespace is collapsed.
            if (r->m_object->style()->collapseWhiteSpace()) {
                InlineTextBox* textBox = toInlineTextBox(r->m_box);
                int expansion = (availableLogicalWidth - totalLogicalWidth) * opportunitiesInRun / expansionOpportunityCount;
                textBox->setExpansion(expansion);
                totalLogicalWidth += expansion;
            }
            expansionOpportunityCount -= opportunitiesInRun;
            if (!expansionOpportunityCount)
                break;
        }
    }
}

void RenderBlock::updateLogicalWidthForAlignment(const ETextAlign& textAlign, BidiRun* trailingSpaceRun, float& logicalLeft, float& totalLogicalWidth, float& availableLogicalWidth, int expansionOpportunityCount)
{
    // Armed with the total width of the line (without justification),
    // we now examine our text-align property in order to determine where to position the
    // objects horizontally. The total width of the line can be increased if we end up
    // justifying text.
    switch (textAlign) {
    case LEFT:
    case WEBKIT_LEFT:
        updateLogicalWidthForLeftAlignedBlock(style()->isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
        break;
    case RIGHT:
    case WEBKIT_RIGHT:
        updateLogicalWidthForRightAlignedBlock(style()->isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
        break;
    case CENTER:
    case WEBKIT_CENTER:
        updateLogicalWidthForCenterAlignedBlock(style()->isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
        break;
    case JUSTIFY:
        adjustInlineDirectionLineBounds(expansionOpportunityCount, logicalLeft, availableLogicalWidth);
        if (expansionOpportunityCount) {
            if (trailingSpaceRun) {
                totalLogicalWidth -= trailingSpaceRun->m_box->logicalWidth();
                trailingSpaceRun->m_box->setLogicalWidth(0);
            }
            break;
        }
        // Fall through
    case TASTART:
        if (style()->isLeftToRightDirection())
            updateLogicalWidthForLeftAlignedBlock(style()->isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
        else
            updateLogicalWidthForRightAlignedBlock(style()->isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
        break;
    case TAEND:
        if (style()->isLeftToRightDirection())
            updateLogicalWidthForRightAlignedBlock(style()->isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
        else
            updateLogicalWidthForLeftAlignedBlock(style()->isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
        break;
    }
}

void RenderBlock::computeInlineDirectionPositionsForLine(RootInlineBox* lineBox, const LineInfo& lineInfo, BidiRun* firstRun, BidiRun* trailingSpaceRun, bool reachedEnd,
                                                         GlyphOverflowAndFallbackFontsMap& textBoxDataMap, VerticalPositionCache& verticalPositionCache)
{
    ETextAlign textAlign = textAlignmentForLine(!reachedEnd && !lineBox->endsWithBreak());
    
    LayoutUnit lineLogicalHeight = logicalHeightForLine(this);
    // CSS 2.1: "'Text-indent' only affects a line if it is the first formatted line of an element. For example, the first line of an anonymous block 
    // box is only affected if it is the first child of its parent element."
    bool firstLine = lineInfo.isFirstLine() && !(isAnonymousBlock() && parent()->firstChild() != this);
    float logicalLeft = pixelSnappedLogicalLeftOffsetForLine(logicalHeight(), firstLine, lineLogicalHeight);
    float logicalRight = pixelSnappedLogicalRightOffsetForLine(logicalHeight(), firstLine, lineLogicalHeight);
#if ENABLE(CSS_EXCLUSIONS)
    WrapShapeInfo* wrapShapeInfo = this->wrapShapeInfo();
    if (wrapShapeInfo && wrapShapeInfo->lineState() == WrapShapeInfo::LINE_INSIDE_SHAPE) {
        logicalLeft = max<float>(roundToInt(wrapShapeInfo->segments()[0].logicalLeft), logicalLeft);
        logicalRight = min<float>(floorToInt(wrapShapeInfo->segments()[0].logicalRight), logicalRight);
    }
#endif
    float availableLogicalWidth = logicalRight - logicalLeft;

    bool needsWordSpacing = false;
    float totalLogicalWidth = lineBox->getFlowSpacingLogicalWidth();
    unsigned expansionOpportunityCount = 0;
    bool isAfterExpansion = true;
    Vector<unsigned, 16> expansionOpportunities;
    RenderObject* previousObject = 0;

    for (BidiRun* r = firstRun; r; r = r->next()) {
        if (!r->m_box || r->m_object->isOutOfFlowPositioned() || r->m_box->isLineBreak())
            continue; // Positioned objects are only participating to figure out their
                      // correct static x position.  They have no effect on the width.
                      // Similarly, line break boxes have no effect on the width.
        if (r->m_object->isText()) {
            RenderText* rt = toRenderText(r->m_object);
            if (textAlign == JUSTIFY && r != trailingSpaceRun) {
                if (!isAfterExpansion)
                    toInlineTextBox(r->m_box)->setCanHaveLeadingExpansion(true);
                unsigned opportunitiesInRun = Font::expansionOpportunityCount(rt->characters() + r->m_start, r->m_stop - r->m_start, r->m_box->direction(), isAfterExpansion);
                expansionOpportunities.append(opportunitiesInRun);
                expansionOpportunityCount += opportunitiesInRun;
            }

            if (int length = rt->textLength()) {
                if (!r->m_start && needsWordSpacing && isSpaceOrNewline(rt->characters()[r->m_start]))
                    totalLogicalWidth += rt->style(lineInfo.isFirstLine())->font().wordSpacing();
                needsWordSpacing = !isSpaceOrNewline(rt->characters()[r->m_stop - 1]) && r->m_stop == length;
            }

            setLogicalWidthForTextRun(lineBox, r, rt, totalLogicalWidth, lineInfo, textBoxDataMap, verticalPositionCache);
        } else {
            isAfterExpansion = false;
            if (!r->m_object->isRenderInline()) {
                RenderBox* renderBox = toRenderBox(r->m_object);
                if (renderBox->isRubyRun())
                    setMarginsForRubyRun(r, toRenderRubyRun(renderBox), previousObject, lineInfo);
                r->m_box->setLogicalWidth(logicalWidthForChild(renderBox));
                totalLogicalWidth += marginStartForChild(renderBox) + marginEndForChild(renderBox);
            }
        }

        totalLogicalWidth += r->m_box->logicalWidth();
        previousObject = r->m_object;
    }

    if (isAfterExpansion && !expansionOpportunities.isEmpty()) {
        expansionOpportunities.last()--;
        expansionOpportunityCount--;
    }

    updateLogicalWidthForAlignment(textAlign, trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth, expansionOpportunityCount);

    computeExpansionForJustifiedText(firstRun, trailingSpaceRun, expansionOpportunities, expansionOpportunityCount, totalLogicalWidth, availableLogicalWidth);

    // The widths of all runs are now known.  We can now place every inline box (and
    // compute accurate widths for the inline flow boxes).
    needsWordSpacing = false;
    lineBox->placeBoxesInInlineDirection(logicalLeft, needsWordSpacing, textBoxDataMap);
}

void RenderBlock::computeBlockDirectionPositionsForLine(RootInlineBox* lineBox, BidiRun* firstRun, GlyphOverflowAndFallbackFontsMap& textBoxDataMap,
                                                        VerticalPositionCache& verticalPositionCache)
{
    setLogicalHeight(lineBox->alignBoxesInBlockDirection(logicalHeight(), textBoxDataMap, verticalPositionCache));

    // Now make sure we place replaced render objects correctly.
    for (BidiRun* r = firstRun; r; r = r->next()) {
        ASSERT(r->m_box);
        if (!r->m_box)
            continue; // Skip runs with no line boxes.

        // Align positioned boxes with the top of the line box.  This is
        // a reasonable approximation of an appropriate y position.
        if (r->m_object->isOutOfFlowPositioned())
            r->m_box->setLogicalTop(logicalHeight());

        // Position is used to properly position both replaced elements and
        // to update the static normal flow x/y of positioned elements.
        if (r->m_object->isText())
            toRenderText(r->m_object)->positionLineBox(r->m_box);
        else if (r->m_object->isBox())
            toRenderBox(r->m_object)->positionLineBox(r->m_box);
    }
    // Positioned objects and zero-length text nodes destroy their boxes in
    // position(), which unnecessarily dirties the line.
    lineBox->markDirty(false);
}

static inline bool isCollapsibleSpace(UChar character, RenderText* renderer)
{
    if (character == ' ' || character == '\t' || character == softHyphen)
        return true;
    if (character == '\n')
        return !renderer->style()->preserveNewline();
    if (character == noBreakSpace)
        return renderer->style()->nbspMode() == SPACE;
    return false;
}


static void setStaticPositions(RenderBlock* block, RenderBox* child)
{
    // FIXME: The math here is actually not really right. It's a best-guess approximation that
    // will work for the common cases
    RenderObject* containerBlock = child->container();
    LayoutUnit blockHeight = block->logicalHeight();
    if (containerBlock->isRenderInline()) {
        // A relative positioned inline encloses us. In this case, we also have to determine our
        // position as though we were an inline. Set |staticInlinePosition| and |staticBlockPosition| on the relative positioned
        // inline so that we can obtain the value later.
        toRenderInline(containerBlock)->layer()->setStaticInlinePosition(block->startAlignedOffsetForLine(blockHeight, false));
        toRenderInline(containerBlock)->layer()->setStaticBlockPosition(blockHeight);
    }

    if (child->style()->isOriginalDisplayInlineType())
        block->setStaticInlinePositionForChild(child, blockHeight, block->startAlignedOffsetForLine(blockHeight, false));
    else
        block->setStaticInlinePositionForChild(child, blockHeight, block->startOffsetForContent(blockHeight));
    child->layer()->setStaticBlockPosition(blockHeight);
}

inline BidiRun* RenderBlock::handleTrailingSpaces(BidiRunList<BidiRun>& bidiRuns, BidiContext* currentContext)
{
    if (!bidiRuns.runCount()
        || !bidiRuns.logicallyLastRun()->m_object->style()->breakOnlyAfterWhiteSpace()
        || !bidiRuns.logicallyLastRun()->m_object->style()->autoWrap())
        return 0;

    BidiRun* trailingSpaceRun = bidiRuns.logicallyLastRun();
    RenderObject* lastObject = trailingSpaceRun->m_object;
    if (!lastObject->isText())
        return 0;

    RenderText* lastText = toRenderText(lastObject);
    const UChar* characters = lastText->characters();
    int firstSpace = trailingSpaceRun->stop();
    while (firstSpace > trailingSpaceRun->start()) {
        UChar current = characters[firstSpace - 1];
        if (!isCollapsibleSpace(current, lastText))
            break;
        firstSpace--;
    }
    if (firstSpace == trailingSpaceRun->stop())
        return 0;

    TextDirection direction = style()->direction();
    bool shouldReorder = trailingSpaceRun != (direction == LTR ? bidiRuns.lastRun() : bidiRuns.firstRun());
    if (firstSpace != trailingSpaceRun->start()) {
        BidiContext* baseContext = currentContext;
        while (BidiContext* parent = baseContext->parent())
            baseContext = parent;

        BidiRun* newTrailingRun = new (renderArena()) BidiRun(firstSpace, trailingSpaceRun->m_stop, trailingSpaceRun->m_object, baseContext, OtherNeutral);
        trailingSpaceRun->m_stop = firstSpace;
        if (direction == LTR)
            bidiRuns.addRun(newTrailingRun);
        else
            bidiRuns.prependRun(newTrailingRun);
        trailingSpaceRun = newTrailingRun;
        return trailingSpaceRun;
    }
    if (!shouldReorder)
        return trailingSpaceRun;

    if (direction == LTR) {
        bidiRuns.moveRunToEnd(trailingSpaceRun);
        trailingSpaceRun->m_level = 0;
    } else {
        bidiRuns.moveRunToBeginning(trailingSpaceRun);
        trailingSpaceRun->m_level = 1;
    }
    return trailingSpaceRun;
}

void RenderBlock::appendFloatingObjectToLastLine(FloatingObject* floatingObject)
{
    ASSERT(!floatingObject->m_originatingLine);
    floatingObject->m_originatingLine = lastRootBox();
    lastRootBox()->appendFloat(floatingObject->renderer());
}

// FIXME: This should be a BidiStatus constructor or create method.
static inline BidiStatus statusWithDirection(TextDirection textDirection, bool isOverride)
{
    WTF::Unicode::Direction direction = textDirection == LTR ? LeftToRight : RightToLeft;
    RefPtr<BidiContext> context = BidiContext::create(textDirection == LTR ? 0 : 1, direction, isOverride, FromStyleOrDOM);

    // This copies BidiStatus and may churn the ref on BidiContext. I doubt it matters.
    return BidiStatus(direction, direction, direction, context.release());
}

// FIXME: BidiResolver should have this logic.
static inline void constructBidiRuns(InlineBidiResolver& topResolver, BidiRunList<BidiRun>& bidiRuns, const InlineIterator& endOfLine, VisualDirectionOverride override, bool previousLineBrokeCleanly)
{
    // FIXME: We should pass a BidiRunList into createBidiRunsForLine instead
    // of the resolver owning the runs.
    ASSERT(&topResolver.runs() == &bidiRuns);
    RenderObject* currentRoot = topResolver.position().root();
    topResolver.createBidiRunsForLine(endOfLine, override, previousLineBrokeCleanly);

    while (!topResolver.isolatedRuns().isEmpty()) {
        // It does not matter which order we resolve the runs as long as we resolve them all.
        BidiRun* isolatedRun = topResolver.isolatedRuns().last();
        topResolver.isolatedRuns().removeLast();

        RenderObject* startObj = isolatedRun->object();

        // Only inlines make sense with unicode-bidi: isolate (blocks are already isolated).
        // FIXME: Because enterIsolate is not passed a RenderObject, we have to crawl up the
        // tree to see which parent inline is the isolate. We could change enterIsolate
        // to take a RenderObject and do this logic there, but that would be a layering
        // violation for BidiResolver (which knows nothing about RenderObject).
        RenderInline* isolatedInline = toRenderInline(containingIsolate(startObj, currentRoot));
        InlineBidiResolver isolatedResolver;
        EUnicodeBidi unicodeBidi = isolatedInline->style()->unicodeBidi();
        TextDirection direction;
        if (unicodeBidi == Plaintext)
            determineDirectionality(direction, InlineIterator(isolatedInline, isolatedRun->object(), 0));
        else {
            ASSERT(unicodeBidi == Isolate || unicodeBidi == IsolateOverride);
            direction = isolatedInline->style()->direction();
        }
        isolatedResolver.setStatus(statusWithDirection(direction, isOverride(unicodeBidi)));

        // FIXME: The fact that we have to construct an Iterator here
        // currently prevents this code from moving into BidiResolver.
        if (!bidiFirstSkippingEmptyInlines(isolatedInline, &isolatedResolver))
            continue;

        // The starting position is the beginning of the first run within the isolate that was identified
        // during the earlier call to createBidiRunsForLine. This can be but is not necessarily the
        // first run within the isolate.
        InlineIterator iter = InlineIterator(isolatedInline, startObj, isolatedRun->m_start);
        isolatedResolver.setPositionIgnoringNestedIsolates(iter);

        // We stop at the next end of line; we may re-enter this isolate in the next call to constructBidiRuns().
        // FIXME: What should end and previousLineBrokeCleanly be?
        // rniwa says previousLineBrokeCleanly is just a WinIE hack and could always be false here?
        isolatedResolver.createBidiRunsForLine(endOfLine, NoVisualOverride, previousLineBrokeCleanly);
        // Note that we do not delete the runs from the resolver.
        // We're not guaranteed to get any BidiRuns in the previous step. If we don't, we allow the placeholder
        // itself to be turned into an InlineBox. We can't remove it here without potentially losing track of
        // the logically last run.
        if (isolatedResolver.runs().runCount())
            bidiRuns.replaceRunWithRuns(isolatedRun, isolatedResolver.runs());

        // If we encountered any nested isolate runs, just move them
        // to the top resolver's list for later processing.
        if (!isolatedResolver.isolatedRuns().isEmpty()) {
            topResolver.isolatedRuns().append(isolatedResolver.isolatedRuns());
            isolatedResolver.isolatedRuns().clear();
        }
    }
}

// This function constructs line boxes for all of the text runs in the resolver and computes their position.
RootInlineBox* RenderBlock::createLineBoxesFromBidiRuns(BidiRunList<BidiRun>& bidiRuns, const InlineIterator& end, LineInfo& lineInfo, VerticalPositionCache& verticalPositionCache, BidiRun* trailingSpaceRun)
{
    if (!bidiRuns.runCount())
        return 0;

    // FIXME: Why is this only done when we had runs?
    lineInfo.setLastLine(!end.m_obj);

    RootInlineBox* lineBox = constructLine(bidiRuns, lineInfo);
    if (!lineBox)
        return 0;

    lineBox->setEndsWithBreak(lineInfo.previousLineBrokeCleanly());
    
#if ENABLE(SVG)
    bool isSVGRootInlineBox = lineBox->isSVGRootInlineBox();
#else
    bool isSVGRootInlineBox = false;
#endif
    
    GlyphOverflowAndFallbackFontsMap textBoxDataMap;
    
    // Now we position all of our text runs horizontally.
    if (!isSVGRootInlineBox)
        computeInlineDirectionPositionsForLine(lineBox, lineInfo, bidiRuns.firstRun(), trailingSpaceRun, end.atEnd(), textBoxDataMap, verticalPositionCache);
    
    // Now position our text runs vertically.
    computeBlockDirectionPositionsForLine(lineBox, bidiRuns.firstRun(), textBoxDataMap, verticalPositionCache);
    
#if ENABLE(SVG)
    // SVG text layout code computes vertical & horizontal positions on its own.
    // Note that we still need to execute computeVerticalPositionsForLine() as
    // it calls InlineTextBox::positionLineBox(), which tracks whether the box
    // contains reversed text or not. If we wouldn't do that editing and thus
    // text selection in RTL boxes would not work as expected.
    if (isSVGRootInlineBox) {
        ASSERT(isSVGText());
        static_cast<SVGRootInlineBox*>(lineBox)->computePerCharacterLayoutInformation();
    }
#endif
    
    // Compute our overflow now.
    lineBox->computeOverflow(lineBox->lineTop(), lineBox->lineBottom(), textBoxDataMap);
    
#if PLATFORM(MAC)
    // Highlight acts as an overflow inflation.
    if (style()->highlight() != nullAtom)
        lineBox->addHighlightOverflow();
#endif
    return lineBox;
}

// Like LayoutState for layout(), LineLayoutState keeps track of global information
// during an entire linebox tree layout pass (aka layoutInlineChildren).
class LineLayoutState {
public:
    LineLayoutState(bool fullLayout, LayoutUnit& repaintLogicalTop, LayoutUnit& repaintLogicalBottom)
        : m_lastFloat(0)
        , m_endLine(0)
        , m_floatIndex(0)
        , m_endLineLogicalTop(0)
        , m_endLineMatched(false)
        , m_checkForFloatsFromLastLine(false)
        , m_isFullLayout(fullLayout)
        , m_repaintLogicalTop(repaintLogicalTop)
        , m_repaintLogicalBottom(repaintLogicalBottom)
        , m_usesRepaintBounds(false)
    { }

    void markForFullLayout() { m_isFullLayout = true; }
    bool isFullLayout() const { return m_isFullLayout; }

    bool usesRepaintBounds() const { return m_usesRepaintBounds; }

    void setRepaintRange(LayoutUnit logicalHeight)
    { 
        m_usesRepaintBounds = true;
        m_repaintLogicalTop = m_repaintLogicalBottom = logicalHeight; 
    }
    
    void updateRepaintRangeFromBox(RootInlineBox* box, LayoutUnit paginationDelta = 0)
    {
        m_usesRepaintBounds = true;
        m_repaintLogicalTop = min(m_repaintLogicalTop, box->logicalTopVisualOverflow() + min(paginationDelta, ZERO_LAYOUT_UNIT));
        m_repaintLogicalBottom = max(m_repaintLogicalBottom, box->logicalBottomVisualOverflow() + max(paginationDelta, ZERO_LAYOUT_UNIT));
    }
    
    bool endLineMatched() const { return m_endLineMatched; }
    void setEndLineMatched(bool endLineMatched) { m_endLineMatched = endLineMatched; }

    bool checkForFloatsFromLastLine() const { return m_checkForFloatsFromLastLine; }
    void setCheckForFloatsFromLastLine(bool check) { m_checkForFloatsFromLastLine = check; }

    LineInfo& lineInfo() { return m_lineInfo; }
    const LineInfo& lineInfo() const { return m_lineInfo; }

    LayoutUnit endLineLogicalTop() const { return m_endLineLogicalTop; }
    void setEndLineLogicalTop(LayoutUnit logicalTop) { m_endLineLogicalTop = logicalTop; }

    RootInlineBox* endLine() const { return m_endLine; }
    void setEndLine(RootInlineBox* line) { m_endLine = line; }

    RenderBlock::FloatingObject* lastFloat() const { return m_lastFloat; }
    void setLastFloat(RenderBlock::FloatingObject* lastFloat) { m_lastFloat = lastFloat; }

    Vector<RenderBlock::FloatWithRect>& floats() { return m_floats; }
    
    unsigned floatIndex() const { return m_floatIndex; }
    void setFloatIndex(unsigned floatIndex) { m_floatIndex = floatIndex; }
    
private:
    Vector<RenderBlock::FloatWithRect> m_floats;
    RenderBlock::FloatingObject* m_lastFloat;
    RootInlineBox* m_endLine;
    LineInfo m_lineInfo;
    unsigned m_floatIndex;
    LayoutUnit m_endLineLogicalTop;
    bool m_endLineMatched;
    bool m_checkForFloatsFromLastLine;
    
    bool m_isFullLayout;

    // FIXME: Should this be a range object instead of two ints?
    LayoutUnit& m_repaintLogicalTop;
    LayoutUnit& m_repaintLogicalBottom;

    bool m_usesRepaintBounds;
};

static void deleteLineRange(LineLayoutState& layoutState, RenderArena* arena, RootInlineBox* startLine, RootInlineBox* stopLine = 0)
{
    RootInlineBox* boxToDelete = startLine;
    while (boxToDelete && boxToDelete != stopLine) {
        layoutState.updateRepaintRangeFromBox(boxToDelete);
        // Note: deleteLineRange(renderArena(), firstRootBox()) is not identical to deleteLineBoxTree().
        // deleteLineBoxTree uses nextLineBox() instead of nextRootBox() when traversing.
        RootInlineBox* next = boxToDelete->nextRootBox();
        boxToDelete->deleteLine(arena);
        boxToDelete = next;
    }
}

void RenderBlock::layoutRunsAndFloats(LineLayoutState& layoutState, bool hasInlineChild)
{
    // We want to skip ahead to the first dirty line
    InlineBidiResolver resolver;
    RootInlineBox* startLine = determineStartPosition(layoutState, resolver);

    unsigned consecutiveHyphenatedLines = 0;
    if (startLine) {
        for (RootInlineBox* line = startLine->prevRootBox(); line && line->isHyphenated(); line = line->prevRootBox())
            consecutiveHyphenatedLines++;
    }

    // FIXME: This would make more sense outside of this function, but since
    // determineStartPosition can change the fullLayout flag we have to do this here. Failure to call
    // determineStartPosition first will break fast/repaint/line-flow-with-floats-9.html.
    if (layoutState.isFullLayout() && hasInlineChild && !selfNeedsLayout()) {
        setNeedsLayout(true, MarkOnlyThis); // Mark as needing a full layout to force us to repaint.
        RenderView* v = view();
        if (v && !v->doingFullRepaint() && hasLayer()) {
            // Because we waited until we were already inside layout to discover
            // that the block really needed a full layout, we missed our chance to repaint the layer
            // before layout started.  Luckily the layer has cached the repaint rect for its original
            // position and size, and so we can use that to make a repaint happen now.
            repaintUsingContainer(containerForRepaint(), pixelSnappedIntRect(layer()->repaintRect()));
        }
    }

    if (m_floatingObjects && !m_floatingObjects->set().isEmpty())
        layoutState.setLastFloat(m_floatingObjects->set().last());

    // We also find the first clean line and extract these lines.  We will add them back
    // if we determine that we're able to synchronize after handling all our dirty lines.
    InlineIterator cleanLineStart;
    BidiStatus cleanLineBidiStatus;
    if (!layoutState.isFullLayout() && startLine)
        determineEndPosition(layoutState, startLine, cleanLineStart, cleanLineBidiStatus);

    if (startLine) {
        if (!layoutState.usesRepaintBounds())
            layoutState.setRepaintRange(logicalHeight());
        deleteLineRange(layoutState, renderArena(), startLine);
    }

    if (!layoutState.isFullLayout() && lastRootBox() && lastRootBox()->endsWithBreak()) {
        // If the last line before the start line ends with a line break that clear floats,
        // adjust the height accordingly.
        // A line break can be either the first or the last object on a line, depending on its direction.
        if (InlineBox* lastLeafChild = lastRootBox()->lastLeafChild()) {
            RenderObject* lastObject = lastLeafChild->renderer();
            if (!lastObject->isBR())
                lastObject = lastRootBox()->firstLeafChild()->renderer();
            if (lastObject->isBR()) {
                EClear clear = lastObject->style()->clear();
                if (clear != CNONE)
                    newLine(clear);
            }
        }
    }

    layoutRunsAndFloatsInRange(layoutState, resolver, cleanLineStart, cleanLineBidiStatus, consecutiveHyphenatedLines);
    linkToEndLineIfNeeded(layoutState);
    repaintDirtyFloats(layoutState.floats());
}

RenderBlock::RenderTextInfo::RenderTextInfo()
    : m_text(0)
    , m_font(0)
{
}

RenderBlock::RenderTextInfo::~RenderTextInfo()
{
}

void RenderBlock::layoutRunsAndFloatsInRange(LineLayoutState& layoutState, InlineBidiResolver& resolver, const InlineIterator& cleanLineStart, const BidiStatus& cleanLineBidiStatus, unsigned consecutiveHyphenatedLines)
{
    RenderStyle* styleToUse = style();
    bool paginated = view()->layoutState() && view()->layoutState()->isPaginated();
    LineMidpointState& lineMidpointState = resolver.midpointState();
    InlineIterator end = resolver.position();
    bool checkForEndLineMatch = layoutState.endLine();
    RenderTextInfo renderTextInfo;
    VerticalPositionCache verticalPositionCache;

    LineBreaker lineBreaker(this);

#if ENABLE(CSS_EXCLUSIONS)
    WrapShapeInfo* wrapShapeInfo = this->wrapShapeInfo();
    // Move to the top of the shape inside to begin layout
    if (wrapShapeInfo && logicalHeight() < wrapShapeInfo->shapeLogicalTop())
        setLogicalHeight(wrapShapeInfo->shapeLogicalTop());
#endif

    while (!end.atEnd()) {
        // FIXME: Is this check necessary before the first iteration or can it be moved to the end?
        if (checkForEndLineMatch) {
            layoutState.setEndLineMatched(matchedEndLine(layoutState, resolver, cleanLineStart, cleanLineBidiStatus));
            if (layoutState.endLineMatched()) {
                resolver.setPosition(InlineIterator(resolver.position().root(), 0, 0), 0);
                break;
            }
        }

        lineMidpointState.reset();

        layoutState.lineInfo().setEmpty(true);
        layoutState.lineInfo().resetRunsFromLeadingWhitespace();

        const InlineIterator oldEnd = end;
        bool isNewUBAParagraph = layoutState.lineInfo().previousLineBrokeCleanly();
        FloatingObject* lastFloatFromPreviousLine = (m_floatingObjects && !m_floatingObjects->set().isEmpty()) ? m_floatingObjects->set().last() : 0;
#if ENABLE(CSS_EXCLUSIONS)
        // FIXME: Bug 89993: If the wrap shape comes from a parent, we will need to adjust
        // the height coordinate
        if (wrapShapeInfo)
            wrapShapeInfo->computeSegmentsForLine(logicalHeight());
#endif
        end = lineBreaker.nextLineBreak(resolver, layoutState.lineInfo(), renderTextInfo, lastFloatFromPreviousLine, consecutiveHyphenatedLines);
        if (resolver.position().atEnd()) {
            // FIXME: We shouldn't be creating any runs in nextLineBreak to begin with!
            // Once BidiRunList is separated from BidiResolver this will not be needed.
            resolver.runs().deleteRuns();
            resolver.markCurrentRunEmpty(); // FIXME: This can probably be replaced by an ASSERT (or just removed).
            layoutState.setCheckForFloatsFromLastLine(true);
            resolver.setPosition(InlineIterator(resolver.position().root(), 0, 0), 0);
            break;
        }
        ASSERT(end != resolver.position());

        // This is a short-cut for empty lines.
        if (layoutState.lineInfo().isEmpty()) {
            if (lastRootBox())
                lastRootBox()->setLineBreakInfo(end.m_obj, end.m_pos, resolver.status());
        } else {
            VisualDirectionOverride override = (styleToUse->rtlOrdering() == VisualOrder ? (styleToUse->direction() == LTR ? VisualLeftToRightOverride : VisualRightToLeftOverride) : NoVisualOverride);

            if (isNewUBAParagraph && styleToUse->unicodeBidi() == Plaintext && !resolver.context()->parent()) {
                TextDirection direction = styleToUse->direction();
                determineDirectionality(direction, resolver.position());
                resolver.setStatus(BidiStatus(direction, isOverride(styleToUse->unicodeBidi())));
            }
            // FIXME: This ownership is reversed. We should own the BidiRunList and pass it to createBidiRunsForLine.
            BidiRunList<BidiRun>& bidiRuns = resolver.runs();
            constructBidiRuns(resolver, bidiRuns, end, override, layoutState.lineInfo().previousLineBrokeCleanly());
            ASSERT(resolver.position() == end);

            BidiRun* trailingSpaceRun = !layoutState.lineInfo().previousLineBrokeCleanly() ? handleTrailingSpaces(bidiRuns, resolver.context()) : 0;

            if (bidiRuns.runCount() && lineBreaker.lineWasHyphenated()) {
                bidiRuns.logicallyLastRun()->m_hasHyphen = true;
                consecutiveHyphenatedLines++;
            } else
                consecutiveHyphenatedLines = 0;

            // Now that the runs have been ordered, we create the line boxes.
            // At the same time we figure out where border/padding/margin should be applied for
            // inline flow boxes.

            LayoutUnit oldLogicalHeight = logicalHeight();
            RootInlineBox* lineBox = createLineBoxesFromBidiRuns(bidiRuns, end, layoutState.lineInfo(), verticalPositionCache, trailingSpaceRun);

            bidiRuns.deleteRuns();
            resolver.markCurrentRunEmpty(); // FIXME: This can probably be replaced by an ASSERT (or just removed).

            if (lineBox) {
                lineBox->setLineBreakInfo(end.m_obj, end.m_pos, resolver.status());
                if (layoutState.usesRepaintBounds())
                    layoutState.updateRepaintRangeFromBox(lineBox);

                if (paginated) {
                    LayoutUnit adjustment = 0;
                    adjustLinePositionForPagination(lineBox, adjustment);
                    if (adjustment) {
                        LayoutUnit oldLineWidth = availableLogicalWidthForLine(oldLogicalHeight, layoutState.lineInfo().isFirstLine());
                        lineBox->adjustBlockDirectionPosition(adjustment);
                        if (layoutState.usesRepaintBounds())
                            layoutState.updateRepaintRangeFromBox(lineBox);

                        if (availableLogicalWidthForLine(oldLogicalHeight + adjustment, layoutState.lineInfo().isFirstLine()) != oldLineWidth) {
                            // We have to delete this line, remove all floats that got added, and let line layout re-run.
                            lineBox->deleteLine(renderArena());
                            removeFloatingObjectsBelow(lastFloatFromPreviousLine, oldLogicalHeight);
                            setLogicalHeight(oldLogicalHeight + adjustment);
                            resolver.setPositionIgnoringNestedIsolates(oldEnd);
                            end = oldEnd;
                            continue;
                        }

                        setLogicalHeight(lineBox->lineBottomWithLeading());
                    }
                }
            }

            for (size_t i = 0; i < lineBreaker.positionedObjects().size(); ++i)
                setStaticPositions(this, lineBreaker.positionedObjects()[i]);

            layoutState.lineInfo().setFirstLine(false);
            newLine(lineBreaker.clear());
        }

        if (m_floatingObjects && lastRootBox()) {
            const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
            FloatingObjectSetIterator it = floatingObjectSet.begin();
            FloatingObjectSetIterator end = floatingObjectSet.end();
            if (layoutState.lastFloat()) {
                FloatingObjectSetIterator lastFloatIterator = floatingObjectSet.find(layoutState.lastFloat());
                ASSERT(lastFloatIterator != end);
                ++lastFloatIterator;
                it = lastFloatIterator;
            }
            for (; it != end; ++it) {
                FloatingObject* f = *it;
                appendFloatingObjectToLastLine(f);
                ASSERT(f->m_renderer == layoutState.floats()[layoutState.floatIndex()].object);
                // If a float's geometry has changed, give up on syncing with clean lines.
                if (layoutState.floats()[layoutState.floatIndex()].rect != f->frameRect())
                    checkForEndLineMatch = false;
                layoutState.setFloatIndex(layoutState.floatIndex() + 1);
            }
            layoutState.setLastFloat(!floatingObjectSet.isEmpty() ? floatingObjectSet.last() : 0);
        }

        lineMidpointState.reset();
        resolver.setPosition(end, numberOfIsolateAncestors(end));
    }
}

void RenderBlock::linkToEndLineIfNeeded(LineLayoutState& layoutState)
{
    if (layoutState.endLine()) {
        if (layoutState.endLineMatched()) {
            bool paginated = view()->layoutState() && view()->layoutState()->isPaginated();
            // Attach all the remaining lines, and then adjust their y-positions as needed.
            LayoutUnit delta = logicalHeight() - layoutState.endLineLogicalTop();
            for (RootInlineBox* line = layoutState.endLine(); line; line = line->nextRootBox()) {
                line->attachLine();
                if (paginated) {
                    delta -= line->paginationStrut();
                    adjustLinePositionForPagination(line, delta);
                }
                if (delta) {
                    layoutState.updateRepaintRangeFromBox(line, delta);
                    line->adjustBlockDirectionPosition(delta);
                }
                if (Vector<RenderBox*>* cleanLineFloats = line->floatsPtr()) {
                    Vector<RenderBox*>::iterator end = cleanLineFloats->end();
                    for (Vector<RenderBox*>::iterator f = cleanLineFloats->begin(); f != end; ++f) {
                        FloatingObject* floatingObject = insertFloatingObject(*f);
                        ASSERT(!floatingObject->m_originatingLine);
                        floatingObject->m_originatingLine = line;
                        setLogicalHeight(logicalTopForChild(*f) - marginBeforeForChild(*f) + delta);
                        positionNewFloats();
                    }
                }
            }
            setLogicalHeight(lastRootBox()->lineBottomWithLeading());
        } else {
            // Delete all the remaining lines.
            deleteLineRange(layoutState, renderArena(), layoutState.endLine());
        }
    }
    
    if (m_floatingObjects && (layoutState.checkForFloatsFromLastLine() || positionNewFloats()) && lastRootBox()) {
        // In case we have a float on the last line, it might not be positioned up to now.
        // This has to be done before adding in the bottom border/padding, or the float will
        // include the padding incorrectly. -dwh
        if (layoutState.checkForFloatsFromLastLine()) {
            LayoutUnit bottomVisualOverflow = lastRootBox()->logicalBottomVisualOverflow();
            LayoutUnit bottomLayoutOverflow = lastRootBox()->logicalBottomLayoutOverflow();
            TrailingFloatsRootInlineBox* trailingFloatsLineBox = new (renderArena()) TrailingFloatsRootInlineBox(this);
            m_lineBoxes.appendLineBox(trailingFloatsLineBox);
            trailingFloatsLineBox->setConstructed();
            GlyphOverflowAndFallbackFontsMap textBoxDataMap;
            VerticalPositionCache verticalPositionCache;
            LayoutUnit blockLogicalHeight = logicalHeight();
            trailingFloatsLineBox->alignBoxesInBlockDirection(blockLogicalHeight, textBoxDataMap, verticalPositionCache);
            trailingFloatsLineBox->setLineTopBottomPositions(blockLogicalHeight, blockLogicalHeight, blockLogicalHeight, blockLogicalHeight);
            trailingFloatsLineBox->setPaginatedLineWidth(availableLogicalWidthForContent(blockLogicalHeight));
            LayoutRect logicalLayoutOverflow(0, blockLogicalHeight, 1, bottomLayoutOverflow - blockLogicalHeight);
            LayoutRect logicalVisualOverflow(0, blockLogicalHeight, 1, bottomVisualOverflow - blockLogicalHeight);
            trailingFloatsLineBox->setOverflowFromLogicalRects(logicalLayoutOverflow, logicalVisualOverflow, trailingFloatsLineBox->lineTop(), trailingFloatsLineBox->lineBottom());
        }

        const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
        FloatingObjectSetIterator it = floatingObjectSet.begin();
        FloatingObjectSetIterator end = floatingObjectSet.end();
        if (layoutState.lastFloat()) {
            FloatingObjectSetIterator lastFloatIterator = floatingObjectSet.find(layoutState.lastFloat());
            ASSERT(lastFloatIterator != end);
            ++lastFloatIterator;
            it = lastFloatIterator;
        }
        for (; it != end; ++it)
            appendFloatingObjectToLastLine(*it);
        layoutState.setLastFloat(!floatingObjectSet.isEmpty() ? floatingObjectSet.last() : 0);
    }
}

void RenderBlock::repaintDirtyFloats(Vector<FloatWithRect>& floats)
{
    size_t floatCount = floats.size();
    // Floats that did not have layout did not repaint when we laid them out. They would have
    // painted by now if they had moved, but if they stayed at (0, 0), they still need to be
    // painted.
    for (size_t i = 0; i < floatCount; ++i) {
        if (!floats[i].everHadLayout) {
            RenderBox* f = floats[i].object;
            if (!f->x() && !f->y() && f->checkForRepaintDuringLayout())
                f->repaint();
        }
    }
}

void RenderBlock::layoutInlineChildren(bool relayoutChildren, LayoutUnit& repaintLogicalTop, LayoutUnit& repaintLogicalBottom)
{
    m_overflow.clear();

    setLogicalHeight(borderBefore() + paddingBefore());
    
    // Lay out our hypothetical grid line as though it occurs at the top of the block.
    if (view()->layoutState() && view()->layoutState()->lineGrid() == this)
        layoutLineGridBox();

    // Figure out if we should clear out our line boxes.
    // FIXME: Handle resize eventually!
    bool isFullLayout = !firstLineBox() || selfNeedsLayout() || relayoutChildren;
    LineLayoutState layoutState(isFullLayout, repaintLogicalTop, repaintLogicalBottom);

    if (isFullLayout)
        lineBoxes()->deleteLineBoxes(renderArena());

    // Text truncation only kicks in if your overflow isn't visible and your text-overflow-mode isn't
    // clip.
    // FIXME: CSS3 says that descendants that are clipped must also know how to truncate.  This is insanely
    // difficult to figure out (especially in the middle of doing layout), and is really an esoteric pile of nonsense
    // anyway, so we won't worry about following the draft here.
    bool hasTextOverflow = style()->textOverflow() && hasOverflowClip();

    // Walk all the lines and delete our ellipsis line boxes if they exist.
    if (hasTextOverflow)
         deleteEllipsisLineBoxes();

    if (firstChild()) {
        // In full layout mode, clear the line boxes of children upfront. Otherwise,
        // siblings can run into stale root lineboxes during layout. Then layout
        // the replaced elements later. In partial layout mode, line boxes are not
        // deleted and only dirtied. In that case, we can layout the replaced
        // elements at the same time.
        bool hasInlineChild = false;
        Vector<RenderBox*> replacedChildren;
        for (InlineWalker walker(this); !walker.atEnd(); walker.advance()) {
            RenderObject* o = walker.current();
            if (!hasInlineChild && o->isInline())
                hasInlineChild = true;

            if (o->isReplaced() || o->isFloating() || o->isOutOfFlowPositioned()) {
                RenderBox* box = toRenderBox(o);

                if (relayoutChildren || box->hasRelativeDimensions())
                    o->setChildNeedsLayout(true, MarkOnlyThis);

                // If relayoutChildren is set and the child has percentage padding or an embedded content box, we also need to invalidate the childs pref widths.
                if (relayoutChildren && box->needsPreferredWidthsRecalculation())
                    o->setPreferredLogicalWidthsDirty(true, MarkOnlyThis);

                if (o->isOutOfFlowPositioned())
                    o->containingBlock()->insertPositionedObject(box);
                else if (o->isFloating())
                    layoutState.floats().append(FloatWithRect(box));
                else if (isFullLayout || o->needsLayout()) {
                    // Replaced element.
                    box->dirtyLineBoxes(isFullLayout);
                    if (isFullLayout)
                        replacedChildren.append(box);
                    else
                        o->layoutIfNeeded();
                }
            } else if (o->isText() || (o->isRenderInline() && !walker.atEndOfInline())) {
                if (!o->isText())
                    toRenderInline(o)->updateAlwaysCreateLineBoxes(layoutState.isFullLayout());
                if (layoutState.isFullLayout() || o->selfNeedsLayout())
                    dirtyLineBoxesForRenderer(o, layoutState.isFullLayout());
                o->setNeedsLayout(false);
            }
        }

        for (size_t i = 0; i < replacedChildren.size(); i++)
             replacedChildren[i]->layoutIfNeeded();

        layoutRunsAndFloats(layoutState, hasInlineChild);
    }

    // Expand the last line to accommodate Ruby and emphasis marks.
    int lastLineAnnotationsAdjustment = 0;
    if (lastRootBox()) {
        LayoutUnit lowestAllowedPosition = max(lastRootBox()->lineBottom(), logicalHeight() + paddingAfter());
        if (!style()->isFlippedLinesWritingMode())
            lastLineAnnotationsAdjustment = lastRootBox()->computeUnderAnnotationAdjustment(lowestAllowedPosition);
        else
            lastLineAnnotationsAdjustment = lastRootBox()->computeOverAnnotationAdjustment(lowestAllowedPosition);
    }

    // Now add in the bottom border/padding.
    setLogicalHeight(logicalHeight() + lastLineAnnotationsAdjustment + borderAfter() + paddingAfter() + scrollbarLogicalHeight());

    if (!firstLineBox() && hasLineIfEmpty())
        setLogicalHeight(logicalHeight() + lineHeight(true, isHorizontalWritingMode() ? HorizontalLine : VerticalLine, PositionOfInteriorLineBoxes));

    // See if we have any lines that spill out of our block.  If we do, then we will possibly need to
    // truncate text.
    if (hasTextOverflow)
        checkLinesForTextOverflow();
}

void RenderBlock::checkFloatsInCleanLine(RootInlineBox* line, Vector<FloatWithRect>& floats, size_t& floatIndex, bool& encounteredNewFloat, bool& dirtiedByFloat)
{
    Vector<RenderBox*>* cleanLineFloats = line->floatsPtr();
    if (!cleanLineFloats)
        return;

    Vector<RenderBox*>::iterator end = cleanLineFloats->end();
    for (Vector<RenderBox*>::iterator it = cleanLineFloats->begin(); it != end; ++it) {
        RenderBox* floatingBox = *it;
        floatingBox->layoutIfNeeded();
        LayoutSize newSize(floatingBox->width() + floatingBox->marginWidth(), floatingBox->height() + floatingBox->marginHeight());
        ASSERT(floatIndex < floats.size());
        if (floats[floatIndex].object != floatingBox) {
            encounteredNewFloat = true;
            return;
        }

        if (floats[floatIndex].rect.size() != newSize) {
            LayoutUnit floatTop = isHorizontalWritingMode() ? floats[floatIndex].rect.y() : floats[floatIndex].rect.x();
            LayoutUnit floatHeight = isHorizontalWritingMode() ? max(floats[floatIndex].rect.height(), newSize.height())
                                                                 : max(floats[floatIndex].rect.width(), newSize.width());
            floatHeight = min(floatHeight, MAX_LAYOUT_UNIT - floatTop);
            line->markDirty();
            markLinesDirtyInBlockRange(line->lineBottomWithLeading(), floatTop + floatHeight, line);
            floats[floatIndex].rect.setSize(newSize);
            dirtiedByFloat = true;
        }
        floatIndex++;
    }
}

RootInlineBox* RenderBlock::determineStartPosition(LineLayoutState& layoutState, InlineBidiResolver& resolver)
{
    RootInlineBox* curr = 0;
    RootInlineBox* last = 0;

    // FIXME: This entire float-checking block needs to be broken into a new function.
    bool dirtiedByFloat = false;
    if (!layoutState.isFullLayout()) {
        // Paginate all of the clean lines.
        bool paginated = view()->layoutState() && view()->layoutState()->isPaginated();
        LayoutUnit paginationDelta = 0;
        size_t floatIndex = 0;
        for (curr = firstRootBox(); curr && !curr->isDirty(); curr = curr->nextRootBox()) {
            if (paginated) {
                if (lineWidthForPaginatedLineChanged(curr)) {
                    curr->markDirty();
                    break;
                }
                paginationDelta -= curr->paginationStrut();
                adjustLinePositionForPagination(curr, paginationDelta);
                if (paginationDelta) {
                    if (containsFloats() || !layoutState.floats().isEmpty()) {
                        // FIXME: Do better eventually.  For now if we ever shift because of pagination and floats are present just go to a full layout.
                        layoutState.markForFullLayout();
                        break;
                    }

                    layoutState.updateRepaintRangeFromBox(curr, paginationDelta);
                    curr->adjustBlockDirectionPosition(paginationDelta);
                }
            }

            // If a new float has been inserted before this line or before its last known float, just do a full layout.
            bool encounteredNewFloat = false;
            checkFloatsInCleanLine(curr, layoutState.floats(), floatIndex, encounteredNewFloat, dirtiedByFloat);
            if (encounteredNewFloat)
                layoutState.markForFullLayout();

            if (dirtiedByFloat || layoutState.isFullLayout())
                break;
        }
        // Check if a new float has been inserted after the last known float.
        if (!curr && floatIndex < layoutState.floats().size())
            layoutState.markForFullLayout();
    }

    if (layoutState.isFullLayout()) {
        // FIXME: This should just call deleteLineBoxTree, but that causes
        // crashes for fast/repaint tests.
        RenderArena* arena = renderArena();
        curr = firstRootBox();
        while (curr) {
            // Note: This uses nextRootBox() insted of nextLineBox() like deleteLineBoxTree does.
            RootInlineBox* next = curr->nextRootBox();
            curr->deleteLine(arena);
            curr = next;
        }
        ASSERT(!firstLineBox() && !lastLineBox());
    } else {
        if (curr) {
            // We have a dirty line.
            if (RootInlineBox* prevRootBox = curr->prevRootBox()) {
                // We have a previous line.
                if (!dirtiedByFloat && (!prevRootBox->endsWithBreak() || (prevRootBox->lineBreakObj()->isText() && prevRootBox->lineBreakPos() >= toRenderText(prevRootBox->lineBreakObj())->textLength())))
                    // The previous line didn't break cleanly or broke at a newline
                    // that has been deleted, so treat it as dirty too.
                    curr = prevRootBox;
            }
        } else {
            // No dirty lines were found.
            // If the last line didn't break cleanly, treat it as dirty.
            if (lastRootBox() && !lastRootBox()->endsWithBreak())
                curr = lastRootBox();
        }

        // If we have no dirty lines, then last is just the last root box.
        last = curr ? curr->prevRootBox() : lastRootBox();
    }

    unsigned numCleanFloats = 0;
    if (!layoutState.floats().isEmpty()) {
        LayoutUnit savedLogicalHeight = logicalHeight();
        // Restore floats from clean lines.
        RootInlineBox* line = firstRootBox();
        while (line != curr) {
            if (Vector<RenderBox*>* cleanLineFloats = line->floatsPtr()) {
                Vector<RenderBox*>::iterator end = cleanLineFloats->end();
                for (Vector<RenderBox*>::iterator f = cleanLineFloats->begin(); f != end; ++f) {
                    FloatingObject* floatingObject = insertFloatingObject(*f);
                    ASSERT(!floatingObject->m_originatingLine);
                    floatingObject->m_originatingLine = line;
                    setLogicalHeight(logicalTopForChild(*f) - marginBeforeForChild(*f));
                    positionNewFloats();
                    ASSERT(layoutState.floats()[numCleanFloats].object == *f);
                    numCleanFloats++;
                }
            }
            line = line->nextRootBox();
        }
        setLogicalHeight(savedLogicalHeight);
    }
    layoutState.setFloatIndex(numCleanFloats);

    layoutState.lineInfo().setFirstLine(!last);
    layoutState.lineInfo().setPreviousLineBrokeCleanly(!last || last->endsWithBreak());

    if (last) {
        setLogicalHeight(last->lineBottomWithLeading());
        InlineIterator iter = InlineIterator(this, last->lineBreakObj(), last->lineBreakPos());
        resolver.setPosition(iter, numberOfIsolateAncestors(iter));
        resolver.setStatus(last->lineBreakBidiStatus());
    } else {
        TextDirection direction = style()->direction();
        if (style()->unicodeBidi() == Plaintext)
            determineDirectionality(direction, InlineIterator(this, bidiFirstSkippingEmptyInlines(this), 0));
        resolver.setStatus(BidiStatus(direction, isOverride(style()->unicodeBidi())));
        InlineIterator iter = InlineIterator(this, bidiFirstSkippingEmptyInlines(this, &resolver), 0);
        resolver.setPosition(iter, numberOfIsolateAncestors(iter));
    }
    return curr;
}

void RenderBlock::determineEndPosition(LineLayoutState& layoutState, RootInlineBox* startLine, InlineIterator& cleanLineStart, BidiStatus& cleanLineBidiStatus)
{
    ASSERT(!layoutState.endLine());
    size_t floatIndex = layoutState.floatIndex();
    RootInlineBox* last = 0;
    for (RootInlineBox* curr = startLine->nextRootBox(); curr; curr = curr->nextRootBox()) {
        if (!curr->isDirty()) {
            bool encounteredNewFloat = false;
            bool dirtiedByFloat = false;
            checkFloatsInCleanLine(curr, layoutState.floats(), floatIndex, encounteredNewFloat, dirtiedByFloat);
            if (encounteredNewFloat)
                return;
        }
        if (curr->isDirty())
            last = 0;
        else if (!last)
            last = curr;
    }

    if (!last)
        return;

    // At this point, |last| is the first line in a run of clean lines that ends with the last line
    // in the block.

    RootInlineBox* prev = last->prevRootBox();
    cleanLineStart = InlineIterator(this, prev->lineBreakObj(), prev->lineBreakPos());
    cleanLineBidiStatus = prev->lineBreakBidiStatus();
    layoutState.setEndLineLogicalTop(prev->lineBottomWithLeading());

    for (RootInlineBox* line = last; line; line = line->nextRootBox())
        line->extractLine(); // Disconnect all line boxes from their render objects while preserving
                             // their connections to one another.

    layoutState.setEndLine(last);
}

bool RenderBlock::checkPaginationAndFloatsAtEndLine(LineLayoutState& layoutState)
{
    LayoutUnit lineDelta = logicalHeight() - layoutState.endLineLogicalTop();

    bool paginated = view()->layoutState() && view()->layoutState()->isPaginated();
    if (paginated && inRenderFlowThread()) {
        // Check all lines from here to the end, and see if the hypothetical new position for the lines will result
        // in a different available line width.
        for (RootInlineBox* lineBox = layoutState.endLine(); lineBox; lineBox = lineBox->nextRootBox()) {
            if (paginated) {
                // This isn't the real move we're going to do, so don't update the line box's pagination
                // strut yet.
                LayoutUnit oldPaginationStrut = lineBox->paginationStrut();
                lineDelta -= oldPaginationStrut;
                adjustLinePositionForPagination(lineBox, lineDelta);
                lineBox->setPaginationStrut(oldPaginationStrut);
            }
            if (lineWidthForPaginatedLineChanged(lineBox, lineDelta))
                return false;
        }
    }
    
    if (!lineDelta || !m_floatingObjects)
        return true;
    
    // See if any floats end in the range along which we want to shift the lines vertically.
    LayoutUnit logicalTop = min(logicalHeight(), layoutState.endLineLogicalTop());

    RootInlineBox* lastLine = layoutState.endLine();
    while (RootInlineBox* nextLine = lastLine->nextRootBox())
        lastLine = nextLine;

    LayoutUnit logicalBottom = lastLine->lineBottomWithLeading() + absoluteValue(lineDelta);

    const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
    FloatingObjectSetIterator end = floatingObjectSet.end();
    for (FloatingObjectSetIterator it = floatingObjectSet.begin(); it != end; ++it) {
        FloatingObject* f = *it;
        if (logicalBottomForFloat(f) >= logicalTop && logicalBottomForFloat(f) < logicalBottom)
            return false;
    }

    return true;
}

bool RenderBlock::matchedEndLine(LineLayoutState& layoutState, const InlineBidiResolver& resolver, const InlineIterator& endLineStart, const BidiStatus& endLineStatus)
{
    if (resolver.position() == endLineStart) {
        if (resolver.status() != endLineStatus)
            return false;
        return checkPaginationAndFloatsAtEndLine(layoutState);
    }

    // The first clean line doesn't match, but we can check a handful of following lines to try
    // to match back up.
    static int numLines = 8; // The # of lines we're willing to match against.
    RootInlineBox* originalEndLine = layoutState.endLine();
    RootInlineBox* line = originalEndLine;
    for (int i = 0; i < numLines && line; i++, line = line->nextRootBox()) {
        if (line->lineBreakObj() == resolver.position().m_obj && line->lineBreakPos() == resolver.position().m_pos) {
            // We have a match.
            if (line->lineBreakBidiStatus() != resolver.status())
                return false; // ...but the bidi state doesn't match.
            
            bool matched = false;
            RootInlineBox* result = line->nextRootBox();
            layoutState.setEndLine(result);
            if (result) {
                layoutState.setEndLineLogicalTop(line->lineBottomWithLeading());
                matched = checkPaginationAndFloatsAtEndLine(layoutState);
            }

            // Now delete the lines that we failed to sync.
            deleteLineRange(layoutState, renderArena(), originalEndLine, result);
            return matched;
        }
    }

    return false;
}

static inline bool skipNonBreakingSpace(const InlineIterator& it, const LineInfo& lineInfo)
{
    if (it.m_obj->style()->nbspMode() != SPACE || it.current() != noBreakSpace)
        return false;

    // FIXME: This is bad.  It makes nbsp inconsistent with space and won't work correctly
    // with m_minWidth/m_maxWidth.
    // Do not skip a non-breaking space if it is the first character
    // on a line after a clean line break (or on the first line, since previousLineBrokeCleanly starts off
    // |true|).
    if (lineInfo.isEmpty() && lineInfo.previousLineBrokeCleanly())
        return false;

    return true;
}

enum WhitespacePosition { LeadingWhitespace, TrailingWhitespace };
static inline bool shouldCollapseWhiteSpace(const RenderStyle* style, const LineInfo& lineInfo, WhitespacePosition whitespacePosition)
{
    // CSS2 16.6.1
    // If a space (U+0020) at the beginning of a line has 'white-space' set to 'normal', 'nowrap', or 'pre-line', it is removed.
    // If a space (U+0020) at the end of a line has 'white-space' set to 'normal', 'nowrap', or 'pre-line', it is also removed.
    // If spaces (U+0020) or tabs (U+0009) at the end of a line have 'white-space' set to 'pre-wrap', UAs may visually collapse them.
    return style->collapseWhiteSpace()
        || (whitespacePosition == TrailingWhitespace && style->whiteSpace() == PRE_WRAP && (!lineInfo.isEmpty() || !lineInfo.previousLineBrokeCleanly()));
}

static bool requiresLineBoxForContent(RenderInline* flow, const LineInfo& lineInfo)
{
    RenderObject* parent = flow->parent();
    if (flow->document()->inNoQuirksMode() 
        && (flow->style(lineInfo.isFirstLine())->lineHeight() != parent->style(lineInfo.isFirstLine())->lineHeight()
        || flow->style()->verticalAlign() != parent->style()->verticalAlign()
        || !parent->style()->font().fontMetrics().hasIdenticalAscentDescentAndLineGap(flow->style()->font().fontMetrics())))
        return true;
    return false;
}

static bool alwaysRequiresLineBox(RenderInline* flow)
{
    // FIXME: Right now, we only allow line boxes for inlines that are truly empty.
    // We need to fix this, though, because at the very least, inlines containing only
    // ignorable whitespace should should also have line boxes.
    return !flow->firstChild() && flow->hasInlineDirectionBordersPaddingOrMargin();
}

static bool requiresLineBox(const InlineIterator& it, const LineInfo& lineInfo = LineInfo(), WhitespacePosition whitespacePosition = LeadingWhitespace)
{
    if (it.m_obj->isFloatingOrOutOfFlowPositioned())
        return false;

    if (it.m_obj->isRenderInline() && !alwaysRequiresLineBox(toRenderInline(it.m_obj)) && !requiresLineBoxForContent(toRenderInline(it.m_obj), lineInfo))
        return false;

    if (!shouldCollapseWhiteSpace(it.m_obj->style(), lineInfo, whitespacePosition) || it.m_obj->isBR())
        return true;

    UChar current = it.current();
    return current != ' ' && current != '\t' && current != softHyphen && (current != '\n' || it.m_obj->preservesNewline())
    && !skipNonBreakingSpace(it, lineInfo);
}

bool RenderBlock::generatesLineBoxesForInlineChild(RenderObject* inlineObj)
{
    ASSERT(inlineObj->parent() == this);

    InlineIterator it(this, inlineObj, 0);
    // FIXME: We should pass correct value for WhitespacePosition.
    while (!it.atEnd() && !requiresLineBox(it))
        it.increment();

    return !it.atEnd();
}

// FIXME: The entire concept of the skipTrailingWhitespace function is flawed, since we really need to be building
// line boxes even for containers that may ultimately collapse away. Otherwise we'll never get positioned
// elements quite right. In other words, we need to build this function's work into the normal line
// object iteration process.
// NB. this function will insert any floating elements that would otherwise
// be skipped but it will not position them.
void RenderBlock::LineBreaker::skipTrailingWhitespace(InlineIterator& iterator, const LineInfo& lineInfo)
{
    while (!iterator.atEnd() && !requiresLineBox(iterator, lineInfo, TrailingWhitespace)) {
        RenderObject* object = iterator.m_obj;
        if (object->isOutOfFlowPositioned())
            setStaticPositions(m_block, toRenderBox(object));
        else if (object->isFloating())
            m_block->insertFloatingObject(toRenderBox(object));
        iterator.increment();
    }
}

void RenderBlock::LineBreaker::skipLeadingWhitespace(InlineBidiResolver& resolver, LineInfo& lineInfo,
                                                     FloatingObject* lastFloatFromPreviousLine, LineWidth& width)
{
    while (!resolver.position().atEnd() && !requiresLineBox(resolver.position(), lineInfo, LeadingWhitespace)) {
        RenderObject* object = resolver.position().m_obj;
        if (object->isOutOfFlowPositioned()) {
            setStaticPositions(m_block, toRenderBox(object));
            if (object->style()->isOriginalDisplayInlineType()) {
                resolver.runs().addRun(createRun(0, 1, object, resolver));
                lineInfo.incrementRunsFromLeadingWhitespace();
            }
        } else if (object->isFloating())
            m_block->positionNewFloatOnLine(m_block->insertFloatingObject(toRenderBox(object)), lastFloatFromPreviousLine, lineInfo, width);
        else if (object->isText() && object->style()->hasTextCombine() && object->isCombineText() && !toRenderCombineText(object)->isCombined()) {
            toRenderCombineText(object)->combineText();
            if (toRenderCombineText(object)->isCombined())
                continue;
        }
        resolver.increment();
    }
    resolver.commitExplicitEmbedding();
}

// This is currently just used for list markers and inline flows that have line boxes. Neither should
// have an effect on whitespace at the start of the line.
static bool shouldSkipWhitespaceAfterStartObject(RenderBlock* block, RenderObject* o, LineMidpointState& lineMidpointState)
{
    RenderObject* next = bidiNextSkippingEmptyInlines(block, o);
    if (next && !next->isBR() && next->isText() && toRenderText(next)->textLength() > 0) {
        RenderText* nextText = toRenderText(next);
        UChar nextChar = nextText->characters()[0];
        if (nextText->style()->isCollapsibleWhiteSpace(nextChar)) {
            addMidpoint(lineMidpointState, InlineIterator(0, o, 0));
            return true;
        }
    }

    return false;
}

static inline float textWidth(RenderText* text, unsigned from, unsigned len, const Font& font, float xPos, bool isFixedPitch, bool collapseWhiteSpace, TextLayout* layout = 0)
{
    if (isFixedPitch || (!from && len == text->textLength()) || text->style()->hasTextCombine())
        return text->width(from, len, font, xPos);

    if (layout)
        return Font::width(*layout, from, len);

    TextRun run = RenderBlock::constructTextRun(text, font, text->characters() + from, len, text->style());
    run.setCharactersLength(text->textLength() - from);
    ASSERT(run.charactersLength() >= run.length());

    run.setCharacterScanForCodePath(!text->canUseSimpleFontCodePath());
    run.setTabSize(!collapseWhiteSpace, text->style()->tabSize());
    run.setXPos(xPos);
    return font.width(run);
}

static void tryHyphenating(RenderText* text, const Font& font, const AtomicString& localeIdentifier, unsigned consecutiveHyphenatedLines, int consecutiveHyphenatedLinesLimit, int minimumPrefixLength, int minimumSuffixLength, int lastSpace, int pos, float xPos, int availableWidth, bool isFixedPitch, bool collapseWhiteSpace, int lastSpaceWordSpacing, InlineIterator& lineBreak, int nextBreakable, bool& hyphenated)
{
    // Map 'hyphenate-limit-{before,after}: auto;' to 2.
    if (minimumPrefixLength < 0)
        minimumPrefixLength = 2;

    if (minimumSuffixLength < 0)
        minimumSuffixLength = 2;

    if (pos - lastSpace <= minimumSuffixLength)
        return;

    if (consecutiveHyphenatedLinesLimit >= 0 && consecutiveHyphenatedLines >= static_cast<unsigned>(consecutiveHyphenatedLinesLimit))
        return;

    int hyphenWidth = measureHyphenWidth(text, font);

    float maxPrefixWidth = availableWidth - xPos - hyphenWidth - lastSpaceWordSpacing;
    // If the maximum width available for the prefix before the hyphen is small, then it is very unlikely
    // that an hyphenation opportunity exists, so do not bother to look for it.
    if (maxPrefixWidth <= font.pixelSize() * 5 / 4)
        return;

    TextRun run = RenderBlock::constructTextRun(text, font, text->characters() + lastSpace, pos - lastSpace, text->style());
    run.setCharactersLength(text->textLength() - lastSpace);
    ASSERT(run.charactersLength() >= run.length());

    run.setTabSize(!collapseWhiteSpace, text->style()->tabSize());
    run.setXPos(xPos + lastSpaceWordSpacing);

    unsigned prefixLength = font.offsetForPosition(run, maxPrefixWidth, false);
    if (prefixLength < static_cast<unsigned>(minimumPrefixLength))
        return;

    prefixLength = lastHyphenLocation(text->characters() + lastSpace, pos - lastSpace, min(prefixLength, static_cast<unsigned>(pos - lastSpace - minimumSuffixLength)) + 1, localeIdentifier);
    if (!prefixLength || prefixLength < static_cast<unsigned>(minimumPrefixLength))
        return;

    // When lastSapce is a space, which it always is except sometimes at the beginning of a line or after collapsed
    // space, it should not count towards hyphenate-limit-before.
    if (prefixLength == static_cast<unsigned>(minimumPrefixLength)) {
        UChar characterAtLastSpace = text->characters()[lastSpace];
        if (characterAtLastSpace == ' ' || characterAtLastSpace == '\n' || characterAtLastSpace == '\t' || characterAtLastSpace == noBreakSpace)
            return;
    }

    ASSERT(pos - lastSpace - prefixLength >= static_cast<unsigned>(minimumSuffixLength));

#if !ASSERT_DISABLED
    float prefixWidth = hyphenWidth + textWidth(text, lastSpace, prefixLength, font, xPos, isFixedPitch, collapseWhiteSpace) + lastSpaceWordSpacing;
    ASSERT(xPos + prefixWidth <= availableWidth);
#else
    UNUSED_PARAM(isFixedPitch);
#endif

    lineBreak.moveTo(text, lastSpace + prefixLength, nextBreakable);
    hyphenated = true;
}

class TrailingObjects {
public:
    TrailingObjects();
    void setTrailingWhitespace(RenderText*);
    void clear();
    void appendBoxIfNeeded(RenderBox*);

    enum CollapseFirstSpaceOrNot { DoNotCollapseFirstSpace, CollapseFirstSpace };

    void updateMidpointsForTrailingBoxes(LineMidpointState&, const InlineIterator& lBreak, CollapseFirstSpaceOrNot);

private:
    RenderText* m_whitespace;
    Vector<RenderBox*, 4> m_boxes;
};

TrailingObjects::TrailingObjects()
    : m_whitespace(0)
{
}

inline void TrailingObjects::setTrailingWhitespace(RenderText* whitespace)
{
    ASSERT(whitespace);
    m_whitespace = whitespace;
}

inline void TrailingObjects::clear()
{
    m_whitespace = 0;
    m_boxes.clear();
}

inline void TrailingObjects::appendBoxIfNeeded(RenderBox* box)
{
    if (m_whitespace)
        m_boxes.append(box);
}

void TrailingObjects::updateMidpointsForTrailingBoxes(LineMidpointState& lineMidpointState, const InlineIterator& lBreak, CollapseFirstSpaceOrNot collapseFirstSpace)
{
    if (!m_whitespace)
        return;

    // This object is either going to be part of the last midpoint, or it is going to be the actual endpoint.
    // In both cases we just decrease our pos by 1 level to exclude the space, allowing it to - in effect - collapse into the newline.
    if (lineMidpointState.numMidpoints % 2) {
        // Find the trailing space object's midpoint.
        int trailingSpaceMidpoint = lineMidpointState.numMidpoints - 1;
        for ( ; trailingSpaceMidpoint > 0 && lineMidpointState.midpoints[trailingSpaceMidpoint].m_obj != m_whitespace; --trailingSpaceMidpoint) { }
        ASSERT(trailingSpaceMidpoint >= 0);
        if (collapseFirstSpace == CollapseFirstSpace)
            lineMidpointState.midpoints[trailingSpaceMidpoint].m_pos--;

        // Now make sure every single trailingPositionedBox following the trailingSpaceMidpoint properly stops and starts
        // ignoring spaces.
        size_t currentMidpoint = trailingSpaceMidpoint + 1;
        for (size_t i = 0; i < m_boxes.size(); ++i) {
            if (currentMidpoint >= lineMidpointState.numMidpoints) {
                // We don't have a midpoint for this box yet.
                InlineIterator ignoreStart(0, m_boxes[i], 0);
                addMidpoint(lineMidpointState, ignoreStart); // Stop ignoring.
                addMidpoint(lineMidpointState, ignoreStart); // Start ignoring again.
            } else {
                ASSERT(lineMidpointState.midpoints[currentMidpoint].m_obj == m_boxes[i]);
                ASSERT(lineMidpointState.midpoints[currentMidpoint + 1].m_obj == m_boxes[i]);
            }
            currentMidpoint += 2;
        }
    } else if (!lBreak.m_obj) {
        ASSERT(m_whitespace->isText());
        ASSERT(collapseFirstSpace == CollapseFirstSpace);
        // Add a new end midpoint that stops right at the very end.
        unsigned length = m_whitespace->textLength();
        unsigned pos = length >= 2 ? length - 2 : UINT_MAX;
        InlineIterator endMid(0, m_whitespace, pos);
        addMidpoint(lineMidpointState, endMid);
        for (size_t i = 0; i < m_boxes.size(); ++i) {
            InlineIterator ignoreStart(0, m_boxes[i], 0);
            addMidpoint(lineMidpointState, ignoreStart); // Stop ignoring spaces.
            addMidpoint(lineMidpointState, ignoreStart); // Start ignoring again.
        }
    }
}

void RenderBlock::LineBreaker::reset()
{
    m_positionedObjects.clear();
    m_hyphenated = false;
    m_clear = CNONE;
}

InlineIterator RenderBlock::LineBreaker::nextLineBreak(InlineBidiResolver& resolver, LineInfo& lineInfo, RenderTextInfo& renderTextInfo, FloatingObject* lastFloatFromPreviousLine, unsigned consecutiveHyphenatedLines)
{
    reset();

    ASSERT(resolver.position().root() == m_block);

    bool appliedStartWidth = resolver.position().m_pos > 0;
    bool includeEndWidth = true;
    LineMidpointState& lineMidpointState = resolver.midpointState();

    LineWidth width(m_block, lineInfo.isFirstLine());

    skipLeadingWhitespace(resolver, lineInfo, lastFloatFromPreviousLine, width);

    if (resolver.position().atEnd())
        return resolver.position();

    // This variable is used only if whitespace isn't set to PRE, and it tells us whether
    // or not we are currently ignoring whitespace.
    bool ignoringSpaces = false;
    InlineIterator ignoreStart;

    // This variable tracks whether the very last character we saw was a space.  We use
    // this to detect when we encounter a second space so we know we have to terminate
    // a run.
    bool currentCharacterIsSpace = false;
    bool currentCharacterIsWS = false;
    TrailingObjects trailingObjects;

    InlineIterator lBreak = resolver.position();

    // FIXME: It is error-prone to split the position object out like this.
    // Teach this code to work with objects instead of this split tuple.
    InlineIterator current = resolver.position();
    RenderObject* last = current.m_obj;
    bool atStart = true;

    bool startingNewParagraph = lineInfo.previousLineBrokeCleanly();
    lineInfo.setPreviousLineBrokeCleanly(false);

    bool autoWrapWasEverTrueOnLine = false;
    bool floatsFitOnLine = true;

    // Firefox and Opera will allow a table cell to grow to fit an image inside it under
    // very specific circumstances (in order to match common WinIE renderings).
    // Not supporting the quirk has caused us to mis-render some real sites. (See Bugzilla 10517.)
    RenderStyle* blockStyle = m_block->style();
    bool allowImagesToBreak = !m_block->document()->inQuirksMode() || !m_block->isTableCell() || !blockStyle->logicalWidth().isIntrinsicOrAuto();

    EWhiteSpace currWS = blockStyle->whiteSpace();
    EWhiteSpace lastWS = currWS;
    while (current.m_obj) {
        RenderStyle* currentStyle = current.m_obj->style();
        RenderObject* next = bidiNextSkippingEmptyInlines(m_block, current.m_obj);
        if (next && next->parent() && !next->parent()->isDescendantOf(current.m_obj->parent()))
            includeEndWidth = true;

        currWS = current.m_obj->isReplaced() ? current.m_obj->parent()->style()->whiteSpace() : currentStyle->whiteSpace();
        lastWS = last->isReplaced() ? last->parent()->style()->whiteSpace() : last->style()->whiteSpace();

        bool autoWrap = RenderStyle::autoWrap(currWS);
        autoWrapWasEverTrueOnLine = autoWrapWasEverTrueOnLine || autoWrap;

#if ENABLE(SVG)
        bool preserveNewline = current.m_obj->isSVGInlineText() ? false : RenderStyle::preserveNewline(currWS);
#else
        bool preserveNewline = RenderStyle::preserveNewline(currWS);
#endif

        bool collapseWhiteSpace = RenderStyle::collapseWhiteSpace(currWS);

        if (current.m_obj->isBR()) {
            if (width.fitsOnLine()) {
                lBreak.moveToStartOf(current.m_obj);
                lBreak.increment();

                // A <br> always breaks a line, so don't let the line be collapsed
                // away. Also, the space at the end of a line with a <br> does not
                // get collapsed away.  It only does this if the previous line broke
                // cleanly.  Otherwise the <br> has no effect on whether the line is
                // empty or not.
                if (startingNewParagraph)
                    lineInfo.setEmpty(false, m_block, &width);
                trailingObjects.clear();
                lineInfo.setPreviousLineBrokeCleanly(true);

                if (!lineInfo.isEmpty())
                    m_clear = currentStyle->clear();
            }
            goto end;
        }

        if (current.m_obj->isOutOfFlowPositioned()) {
            // If our original display wasn't an inline type, then we can
            // go ahead and determine our static inline position now.
            RenderBox* box = toRenderBox(current.m_obj);
            bool isInlineType = box->style()->isOriginalDisplayInlineType();
            if (!isInlineType)
                m_block->setStaticInlinePositionForChild(box, m_block->logicalHeight(), m_block->startOffsetForContent(m_block->logicalHeight()));
            else  {
                // If our original display was an INLINE type, then we can go ahead
                // and determine our static y position now.
                box->layer()->setStaticBlockPosition(m_block->logicalHeight());
            }

            // If we're ignoring spaces, we have to stop and include this object and
            // then start ignoring spaces again.
            if (isInlineType || current.m_obj->container()->isRenderInline()) {
                if (ignoringSpaces) {
                    ignoreStart.m_obj = current.m_obj;
                    ignoreStart.m_pos = 0;
                    addMidpoint(lineMidpointState, ignoreStart); // Stop ignoring spaces.
                    addMidpoint(lineMidpointState, ignoreStart); // Start ignoring again.
                }
                trailingObjects.appendBoxIfNeeded(box);
            } else
                m_positionedObjects.append(box);
        } else if (current.m_obj->isFloating()) {
            RenderBox* floatBox = toRenderBox(current.m_obj);
            FloatingObject* f = m_block->insertFloatingObject(floatBox);
            // check if it fits in the current line.
            // If it does, position it now, otherwise, position
            // it after moving to next line (in newLine() func)
            if (floatsFitOnLine && width.fitsOnLine(m_block->logicalWidthForFloat(f))) {
                m_block->positionNewFloatOnLine(f, lastFloatFromPreviousLine, lineInfo, width);
                if (lBreak.m_obj == current.m_obj) {
                    ASSERT(!lBreak.m_pos);
                    lBreak.increment();
                }
            } else
                floatsFitOnLine = false;
        } else if (current.m_obj->isRenderInline()) {
            // Right now, we should only encounter empty inlines here.
            ASSERT(!current.m_obj->firstChild());

            RenderInline* flowBox = toRenderInline(current.m_obj);

            // Now that some inline flows have line boxes, if we are already ignoring spaces, we need
            // to make sure that we stop to include this object and then start ignoring spaces again.
            // If this object is at the start of the line, we need to behave like list markers and
            // start ignoring spaces.
            bool requiresLineBox = alwaysRequiresLineBox(flowBox);
            if (requiresLineBox || requiresLineBoxForContent(flowBox, lineInfo)) {
                // An empty inline that only has line-height, vertical-align or font-metrics will only get a
                // line box to affect the height of the line if the rest of the line is not empty.
                if (requiresLineBox)
                    lineInfo.setEmpty(false, m_block, &width);
                if (ignoringSpaces) {
                    trailingObjects.clear();
                    addMidpoint(lineMidpointState, InlineIterator(0, current.m_obj, 0)); // Stop ignoring spaces.
                    addMidpoint(lineMidpointState, InlineIterator(0, current.m_obj, 0)); // Start ignoring again.
                } else if (blockStyle->collapseWhiteSpace() && resolver.position().m_obj == current.m_obj
                    && shouldSkipWhitespaceAfterStartObject(m_block, current.m_obj, lineMidpointState)) {
                    // Like with list markers, we start ignoring spaces to make sure that any
                    // additional spaces we see will be discarded.
                    currentCharacterIsSpace = true;
                    currentCharacterIsWS = true;
                    ignoringSpaces = true;
                }
            }

            width.addUncommittedWidth(borderPaddingMarginStart(flowBox) + borderPaddingMarginEnd(flowBox));
        } else if (current.m_obj->isReplaced()) {
            RenderBox* replacedBox = toRenderBox(current.m_obj);

            // Break on replaced elements if either has normal white-space.
            if ((autoWrap || RenderStyle::autoWrap(lastWS)) && (!current.m_obj->isImage() || allowImagesToBreak)) {
                width.commit();
                lBreak.moveToStartOf(current.m_obj);
            }

            if (ignoringSpaces)
                addMidpoint(lineMidpointState, InlineIterator(0, current.m_obj, 0));

            lineInfo.setEmpty(false, m_block, &width);
            ignoringSpaces = false;
            currentCharacterIsSpace = false;
            currentCharacterIsWS = false;
            trailingObjects.clear();

            // Optimize for a common case. If we can't find whitespace after the list
            // item, then this is all moot.
            LayoutUnit replacedLogicalWidth = m_block->logicalWidthForChild(replacedBox) + m_block->marginStartForChild(replacedBox) + m_block->marginEndForChild(replacedBox) + inlineLogicalWidth(current.m_obj);
            if (current.m_obj->isListMarker()) {
                if (blockStyle->collapseWhiteSpace() && shouldSkipWhitespaceAfterStartObject(m_block, current.m_obj, lineMidpointState)) {
                    // Like with inline flows, we start ignoring spaces to make sure that any
                    // additional spaces we see will be discarded.
                    currentCharacterIsSpace = true;
                    currentCharacterIsWS = true;
                    ignoringSpaces = true;
                }
                if (toRenderListMarker(current.m_obj)->isInside())
                    width.addUncommittedWidth(replacedLogicalWidth);
            } else
                width.addUncommittedWidth(replacedLogicalWidth);
            if (current.m_obj->isRubyRun())
                width.applyOverhang(toRenderRubyRun(current.m_obj), last, next);
        } else if (current.m_obj->isText()) {
            if (!current.m_pos)
                appliedStartWidth = false;

            RenderText* t = toRenderText(current.m_obj);

#if ENABLE(SVG)
            bool isSVGText = t->isSVGInlineText();
#endif

            if (t->style()->hasTextCombine() && current.m_obj->isCombineText() && !toRenderCombineText(current.m_obj)->isCombined())
                toRenderCombineText(current.m_obj)->combineText();

            RenderStyle* style = t->style(lineInfo.isFirstLine());
            const Font& f = style->font();
            bool isFixedPitch = f.isFixedPitch();
            bool canHyphenate = style->hyphens() == HyphensAuto && WebCore::canHyphenate(style->locale());

            int lastSpace = current.m_pos;
            float wordSpacing = currentStyle->wordSpacing();
            float lastSpaceWordSpacing = 0;

            // Non-zero only when kerning is enabled, in which case we measure words with their trailing
            // space, then subtract its width.
            float wordTrailingSpaceWidth = f.typesettingFeatures() & Kerning ? f.width(constructTextRun(t, f, &space, 1, style)) + wordSpacing : 0;

            float wrapW = width.uncommittedWidth() + inlineLogicalWidth(current.m_obj, !appliedStartWidth, true);
            float charWidth = 0;
            bool breakNBSP = autoWrap && currentStyle->nbspMode() == SPACE;
            // Auto-wrapping text should wrap in the middle of a word only if it could not wrap before the word,
            // which is only possible if the word is the first thing on the line, that is, if |w| is zero.
            bool breakWords = currentStyle->breakWords() && ((autoWrap && !width.committedWidth()) || currWS == PRE);
            bool midWordBreak = false;
            bool breakAll = currentStyle->wordBreak() == BreakAllWordBreak && autoWrap;
            float hyphenWidth = 0;

            if (t->isWordBreak()) {
                width.commit();
                lBreak.moveToStartOf(current.m_obj);
                ASSERT(current.m_pos == t->textLength());
            }

            if (renderTextInfo.m_text != t) {
                t->updateTextIfNeeded();
                renderTextInfo.m_text = t;
                renderTextInfo.m_font = &f;
                renderTextInfo.m_layout = f.createLayout(t, width.currentWidth(), collapseWhiteSpace);
                renderTextInfo.m_lineBreakIterator.reset(t->characters(), t->textLength(), style->locale());
            } else if (renderTextInfo.m_layout && renderTextInfo.m_font != &f) {
                renderTextInfo.m_font = &f;
                renderTextInfo.m_layout = f.createLayout(t, width.currentWidth(), collapseWhiteSpace);
            }

            TextLayout* textLayout = renderTextInfo.m_layout.get();

            for (; current.m_pos < t->textLength(); current.fastIncrementInTextNode()) {
                bool previousCharacterIsSpace = currentCharacterIsSpace;
                bool previousCharacterIsWS = currentCharacterIsWS;
                UChar c = current.current();
                currentCharacterIsSpace = c == ' ' || c == '\t' || (!preserveNewline && (c == '\n'));

                if (!collapseWhiteSpace || !currentCharacterIsSpace)
                    lineInfo.setEmpty(false, m_block, &width);

                if (c == softHyphen && autoWrap && !hyphenWidth && style->hyphens() != HyphensNone) {
                    hyphenWidth = measureHyphenWidth(t, f);
                    width.addUncommittedWidth(hyphenWidth);
                }

                bool applyWordSpacing = false;

                currentCharacterIsWS = currentCharacterIsSpace || (breakNBSP && c == noBreakSpace);

                if ((breakAll || breakWords) && !midWordBreak) {
                    wrapW += charWidth;
                    bool midWordBreakIsBeforeSurrogatePair = U16_IS_LEAD(c) && current.m_pos + 1 < t->textLength() && U16_IS_TRAIL(t->characters()[current.m_pos + 1]);
                    charWidth = textWidth(t, current.m_pos, midWordBreakIsBeforeSurrogatePair ? 2 : 1, f, width.committedWidth() + wrapW, isFixedPitch, collapseWhiteSpace, textLayout);
                    midWordBreak = width.committedWidth() + wrapW + charWidth > width.availableWidth();
                }

                bool betweenWords = c == '\n' || (currWS != PRE && !atStart && isBreakable(renderTextInfo.m_lineBreakIterator, current.m_pos, current.m_nextBreakablePosition, breakNBSP)
                    && (style->hyphens() != HyphensNone || (current.previousInSameNode() != softHyphen)));

                if (betweenWords || midWordBreak) {
                    bool stoppedIgnoringSpaces = false;
                    if (ignoringSpaces) {
                        if (!currentCharacterIsSpace) {
                            // Stop ignoring spaces and begin at this
                            // new point.
                            ignoringSpaces = false;
                            lastSpaceWordSpacing = 0;
                            lastSpace = current.m_pos; // e.g., "Foo    goo", don't add in any of the ignored spaces.
                            addMidpoint(lineMidpointState, InlineIterator(0, current.m_obj, current.m_pos));
                            stoppedIgnoringSpaces = true;
                        } else {
                            // Just keep ignoring these spaces.
                            continue;
                        }
                    }

                    float additionalTmpW;
                    if (wordTrailingSpaceWidth && currentCharacterIsSpace)
                        additionalTmpW = textWidth(t, lastSpace, current.m_pos + 1 - lastSpace, f, width.currentWidth(), isFixedPitch, collapseWhiteSpace, textLayout) - wordTrailingSpaceWidth + lastSpaceWordSpacing;
                    else
                        additionalTmpW = textWidth(t, lastSpace, current.m_pos - lastSpace, f, width.currentWidth(), isFixedPitch, collapseWhiteSpace, textLayout) + lastSpaceWordSpacing;
                    width.addUncommittedWidth(additionalTmpW);
                    if (!appliedStartWidth) {
                        width.addUncommittedWidth(inlineLogicalWidth(current.m_obj, true, false));
                        appliedStartWidth = true;
                    }

                    applyWordSpacing =  wordSpacing && currentCharacterIsSpace;

                    if (!width.committedWidth() && autoWrap && !width.fitsOnLine())
                        width.fitBelowFloats();

                    if (autoWrap || breakWords) {
                        // If we break only after white-space, consider the current character
                        // as candidate width for this line.
                        bool lineWasTooWide = false;
                        if (width.fitsOnLine() && currentCharacterIsWS && currentStyle->breakOnlyAfterWhiteSpace() && !midWordBreak) {
                            float charWidth = textWidth(t, current.m_pos, 1, f, width.currentWidth(), isFixedPitch, collapseWhiteSpace, textLayout) + (applyWordSpacing ? wordSpacing : 0);
                            // Check if line is too big even without the extra space
                            // at the end of the line. If it is not, do nothing.
                            // If the line needs the extra whitespace to be too long,
                            // then move the line break to the space and skip all
                            // additional whitespace.
                            if (!width.fitsOnLine(charWidth)) {
                                lineWasTooWide = true;
                                lBreak.moveTo(current.m_obj, current.m_pos, current.m_nextBreakablePosition);
                                skipTrailingWhitespace(lBreak, lineInfo);
                            }
                        }
                        if (lineWasTooWide || !width.fitsOnLine()) {
                            if (canHyphenate && !width.fitsOnLine()) {
                                tryHyphenating(t, f, style->locale(), consecutiveHyphenatedLines, blockStyle->hyphenationLimitLines(), style->hyphenationLimitBefore(), style->hyphenationLimitAfter(), lastSpace, current.m_pos, width.currentWidth() - additionalTmpW, width.availableWidth(), isFixedPitch, collapseWhiteSpace, lastSpaceWordSpacing, lBreak, current.m_nextBreakablePosition, m_hyphenated);
                                if (m_hyphenated)
                                    goto end;
                            }
                            if (lBreak.atTextParagraphSeparator()) {
                                if (!stoppedIgnoringSpaces && current.m_pos > 0) {
                                    // We need to stop right before the newline and then start up again.
                                    addMidpoint(lineMidpointState, InlineIterator(0, current.m_obj, current.m_pos - 1)); // Stop
                                    addMidpoint(lineMidpointState, InlineIterator(0, current.m_obj, current.m_pos)); // Start
                                }
                                lBreak.increment();
                                lineInfo.setPreviousLineBrokeCleanly(true);
                            }
                            if (lBreak.m_obj && lBreak.m_pos && lBreak.m_obj->isText() && toRenderText(lBreak.m_obj)->textLength() && toRenderText(lBreak.m_obj)->characters()[lBreak.m_pos - 1] == softHyphen && style->hyphens() != HyphensNone)
                                m_hyphenated = true;
                            goto end; // Didn't fit. Jump to the end.
                        } else {
                            if (!betweenWords || (midWordBreak && !autoWrap))
                                width.addUncommittedWidth(-additionalTmpW);
                            if (hyphenWidth) {
                                // Subtract the width of the soft hyphen out since we fit on a line.
                                width.addUncommittedWidth(-hyphenWidth);
                                hyphenWidth = 0;
                            }
                        }
                    }

                    if (c == '\n' && preserveNewline) {
                        if (!stoppedIgnoringSpaces && current.m_pos > 0) {
                            // We need to stop right before the newline and then start up again.
                            addMidpoint(lineMidpointState, InlineIterator(0, current.m_obj, current.m_pos - 1)); // Stop
                            addMidpoint(lineMidpointState, InlineIterator(0, current.m_obj, current.m_pos)); // Start
                        }
                        lBreak.moveTo(current.m_obj, current.m_pos, current.m_nextBreakablePosition);
                        lBreak.increment();
                        lineInfo.setPreviousLineBrokeCleanly(true);
                        return lBreak;
                    }

                    if (autoWrap && betweenWords) {
                        width.commit();
                        wrapW = 0;
                        lBreak.moveTo(current.m_obj, current.m_pos, current.m_nextBreakablePosition);
                        // Auto-wrapping text should not wrap in the middle of a word once it has had an
                        // opportunity to break after a word.
                        breakWords = false;
                    }

                    if (midWordBreak && !U16_IS_TRAIL(c) && !(category(c) & (Mark_NonSpacing | Mark_Enclosing | Mark_SpacingCombining))) {
                        // Remember this as a breakable position in case
                        // adding the end width forces a break.
                        lBreak.moveTo(current.m_obj, current.m_pos, current.m_nextBreakablePosition);
                        midWordBreak &= (breakWords || breakAll);
                    }

                    if (betweenWords) {
                        lastSpaceWordSpacing = applyWordSpacing ? wordSpacing : 0;
                        lastSpace = current.m_pos;
                    }

                    if (!ignoringSpaces && currentStyle->collapseWhiteSpace()) {
                        // If we encounter a newline, or if we encounter a
                        // second space, we need to go ahead and break up this
                        // run and enter a mode where we start collapsing spaces.
                        if (currentCharacterIsSpace && previousCharacterIsSpace) {
                            ignoringSpaces = true;

                            // We just entered a mode where we are ignoring
                            // spaces. Create a midpoint to terminate the run
                            // before the second space.
                            addMidpoint(lineMidpointState, ignoreStart);
                            trailingObjects.updateMidpointsForTrailingBoxes(lineMidpointState, InlineIterator(), TrailingObjects::DoNotCollapseFirstSpace);
                        }
                    }
                } else if (ignoringSpaces) {
                    // Stop ignoring spaces and begin at this
                    // new point.
                    ignoringSpaces = false;
                    lastSpaceWordSpacing = applyWordSpacing ? wordSpacing : 0;
                    lastSpace = current.m_pos; // e.g., "Foo    goo", don't add in any of the ignored spaces.
                    addMidpoint(lineMidpointState, InlineIterator(0, current.m_obj, current.m_pos));
                }

#if ENABLE(SVG)
                if (isSVGText && current.m_pos > 0) {
                    // Force creation of new InlineBoxes for each absolute positioned character (those that start new text chunks).
                    if (toRenderSVGInlineText(t)->characterStartsNewTextChunk(current.m_pos)) {
                        addMidpoint(lineMidpointState, InlineIterator(0, current.m_obj, current.m_pos - 1));
                        addMidpoint(lineMidpointState, InlineIterator(0, current.m_obj, current.m_pos));
                    }
                }
#endif

                if (currentCharacterIsSpace && !previousCharacterIsSpace) {
                    ignoreStart.m_obj = current.m_obj;
                    ignoreStart.m_pos = current.m_pos;
                }

                if (!currentCharacterIsWS && previousCharacterIsWS) {
                    if (autoWrap && currentStyle->breakOnlyAfterWhiteSpace())
                        lBreak.moveTo(current.m_obj, current.m_pos, current.m_nextBreakablePosition);
                }

                if (collapseWhiteSpace && currentCharacterIsSpace && !ignoringSpaces)
                    trailingObjects.setTrailingWhitespace(toRenderText(current.m_obj));
                else if (!currentStyle->collapseWhiteSpace() || !currentCharacterIsSpace)
                    trailingObjects.clear();

                atStart = false;
            }

            // IMPORTANT: current.m_pos is > length here!
            float additionalTmpW = ignoringSpaces ? 0 : textWidth(t, lastSpace, current.m_pos - lastSpace, f, width.currentWidth(), isFixedPitch, collapseWhiteSpace, textLayout) + lastSpaceWordSpacing;
            width.addUncommittedWidth(additionalTmpW + inlineLogicalWidth(current.m_obj, !appliedStartWidth, includeEndWidth));
            includeEndWidth = false;

            if (!width.fitsOnLine()) {
                if (canHyphenate)
                    tryHyphenating(t, f, style->locale(), consecutiveHyphenatedLines, blockStyle->hyphenationLimitLines(), style->hyphenationLimitBefore(), style->hyphenationLimitAfter(), lastSpace, current.m_pos, width.currentWidth() - additionalTmpW, width.availableWidth(), isFixedPitch, collapseWhiteSpace, lastSpaceWordSpacing, lBreak, current.m_nextBreakablePosition, m_hyphenated);

                if (!m_hyphenated && lBreak.previousInSameNode() == softHyphen && style->hyphens() != HyphensNone)
                    m_hyphenated = true;

                if (m_hyphenated)
                    goto end;
            }
        } else
            ASSERT_NOT_REACHED();

        bool checkForBreak = autoWrap;
        if (width.committedWidth() && !width.fitsOnLine() && lBreak.m_obj && currWS == NOWRAP)
            checkForBreak = true;
        else if (next && current.m_obj->isText() && next->isText() && !next->isBR() && (autoWrap || (next->style()->autoWrap()))) {
            if (currentCharacterIsSpace)
                checkForBreak = true;
            else {
                RenderText* nextText = toRenderText(next);
                if (nextText->textLength()) {
                    UChar c = nextText->characters()[0];
                    checkForBreak = (c == ' ' || c == '\t' || (c == '\n' && !next->preservesNewline()));
                    // If the next item on the line is text, and if we did not end with
                    // a space, then the next text run continues our word (and so it needs to
                    // keep adding to |tmpW|. Just update and continue.
                } else if (nextText->isWordBreak())
                    checkForBreak = true;

                if (!width.fitsOnLine() && !width.committedWidth())
                    width.fitBelowFloats();

                bool canPlaceOnLine = width.fitsOnLine() || !autoWrapWasEverTrueOnLine;
                if (canPlaceOnLine && checkForBreak) {
                    width.commit();
                    lBreak.moveToStartOf(next);
                }
            }
        }

        if (checkForBreak && !width.fitsOnLine()) {
            // if we have floats, try to get below them.
            if (currentCharacterIsSpace && !ignoringSpaces && currentStyle->collapseWhiteSpace())
                trailingObjects.clear();

            if (width.committedWidth())
                goto end;

            width.fitBelowFloats();

            // |width| may have been adjusted because we got shoved down past a float (thus
            // giving us more room), so we need to retest, and only jump to
            // the end label if we still don't fit on the line. -dwh
            if (!width.fitsOnLine())
                goto end;
        }

        if (!current.m_obj->isFloatingOrOutOfFlowPositioned()) {
            last = current.m_obj;
            if (last->isReplaced() && autoWrap && (!last->isImage() || allowImagesToBreak) && (!last->isListMarker() || toRenderListMarker(last)->isInside())) {
                width.commit();
                lBreak.moveToStartOf(next);
            }
        }

        // Clear out our character space bool, since inline <pre>s don't collapse whitespace
        // with adjacent inline normal/nowrap spans.
        if (!collapseWhiteSpace)
            currentCharacterIsSpace = false;

        current.moveToStartOf(next);
        atStart = false;
    }

    if (width.fitsOnLine() || lastWS == NOWRAP)
        lBreak.clear();

 end:
    if (lBreak == resolver.position() && (!lBreak.m_obj || !lBreak.m_obj->isBR())) {
        // we just add as much as possible
        if (blockStyle->whiteSpace() == PRE) {
            // FIXME: Don't really understand this case.
            if (current.m_pos) {
                // FIXME: This should call moveTo which would clear m_nextBreakablePosition
                // this code as-is is likely wrong.
                lBreak.m_obj = current.m_obj;
                lBreak.m_pos = current.m_pos - 1;
            } else
                lBreak.moveTo(last, last->isText() ? last->length() : 0);
        } else if (lBreak.m_obj) {
            // Don't ever break in the middle of a word if we can help it.
            // There's no room at all. We just have to be on this line,
            // even though we'll spill out.
            lBreak.moveTo(current.m_obj, current.m_pos);
        }
    }

    // make sure we consume at least one char/object.
    if (lBreak == resolver.position())
        lBreak.increment();

    // Sanity check our midpoints.
    checkMidpoints(lineMidpointState, lBreak);

    trailingObjects.updateMidpointsForTrailingBoxes(lineMidpointState, lBreak, TrailingObjects::CollapseFirstSpace);

    // We might have made lBreak an iterator that points past the end
    // of the object. Do this adjustment to make it point to the start
    // of the next object instead to avoid confusing the rest of the
    // code.
    if (lBreak.m_pos > 0) {
        lBreak.m_pos--;
        lBreak.increment();
    }

    return lBreak;
}

void RenderBlock::addOverflowFromInlineChildren()
{
    LayoutUnit endPadding = hasOverflowClip() ? paddingEnd() : ZERO_LAYOUT_UNIT;
    // FIXME: Need to find another way to do this, since scrollbars could show when we don't want them to.
    if (hasOverflowClip() && !endPadding && node() && node()->isRootEditableElement() && style()->isLeftToRightDirection())
        endPadding = 1;
    for (RootInlineBox* curr = firstRootBox(); curr; curr = curr->nextRootBox()) {
        addLayoutOverflow(curr->paddedLayoutOverflowRect(endPadding));
        if (!hasOverflowClip())
            addVisualOverflow(curr->visualOverflowRect(curr->lineTop(), curr->lineBottom()));
    }
}

void RenderBlock::deleteEllipsisLineBoxes()
{
    ETextAlign textAlign = style()->textAlign();
    bool ltr = style()->isLeftToRightDirection();
    bool firstLine = true;
    for (RootInlineBox* curr = firstRootBox(); curr; curr = curr->nextRootBox()) {
        if (curr->hasEllipsisBox()) {
            curr->clearTruncation();

            // Shift the line back where it belongs if we cannot accomodate an ellipsis.
            float logicalLeft = pixelSnappedLogicalLeftOffsetForLine(curr->lineTop(), firstLine);
            float availableLogicalWidth = logicalRightOffsetForLine(curr->lineTop(), false) - logicalLeft;
            float totalLogicalWidth = curr->logicalWidth();
            updateLogicalWidthForAlignment(textAlign, 0, logicalLeft, totalLogicalWidth, availableLogicalWidth, 0);

            if (ltr)
                curr->adjustLogicalPosition((logicalLeft - curr->logicalLeft()), 0);
            else
                curr->adjustLogicalPosition(-(curr->logicalLeft() - logicalLeft), 0);
        }
        firstLine = false;
    }
}

void RenderBlock::checkLinesForTextOverflow()
{
    // Determine the width of the ellipsis using the current font.
    // FIXME: CSS3 says this is configurable, also need to use 0x002E (FULL STOP) if horizontal ellipsis is "not renderable"
    const Font& font = style()->font();
    DEFINE_STATIC_LOCAL(AtomicString, ellipsisStr, (&horizontalEllipsis, 1));
    const Font& firstLineFont = firstLineStyle()->font();
    int firstLineEllipsisWidth = firstLineFont.width(constructTextRun(this, firstLineFont, &horizontalEllipsis, 1, firstLineStyle()));
    int ellipsisWidth = (font == firstLineFont) ? firstLineEllipsisWidth : font.width(constructTextRun(this, font, &horizontalEllipsis, 1, style()));

    // For LTR text truncation, we want to get the right edge of our padding box, and then we want to see
    // if the right edge of a line box exceeds that.  For RTL, we use the left edge of the padding box and
    // check the left edge of the line box to see if it is less
    // Include the scrollbar for overflow blocks, which means we want to use "contentWidth()"
    bool ltr = style()->isLeftToRightDirection();
    ETextAlign textAlign = style()->textAlign();
    bool firstLine = true;
    for (RootInlineBox* curr = firstRootBox(); curr; curr = curr->nextRootBox()) {
        LayoutUnit blockRightEdge = logicalRightOffsetForLine(curr->lineTop(), firstLine);
        LayoutUnit blockLeftEdge = logicalLeftOffsetForLine(curr->lineTop(), firstLine);
        LayoutUnit lineBoxEdge = ltr ? curr->x() + curr->logicalWidth() : curr->x();
        if ((ltr && lineBoxEdge > blockRightEdge) || (!ltr && lineBoxEdge < blockLeftEdge)) {
            // This line spills out of our box in the appropriate direction.  Now we need to see if the line
            // can be truncated.  In order for truncation to be possible, the line must have sufficient space to
            // accommodate our truncation string, and no replaced elements (images, tables) can overlap the ellipsis
            // space.

            LayoutUnit width = firstLine ? firstLineEllipsisWidth : ellipsisWidth;
            LayoutUnit blockEdge = ltr ? blockRightEdge : blockLeftEdge;
            if (curr->lineCanAccommodateEllipsis(ltr, blockEdge, lineBoxEdge, width)) {
                float totalLogicalWidth = curr->placeEllipsis(ellipsisStr, ltr, blockLeftEdge, blockRightEdge, width);

                float logicalLeft = 0; // We are only intersted in the delta from the base position.
                float truncatedWidth = pixelSnappedLogicalRightOffsetForLine(curr->lineTop(), firstLine);
                updateLogicalWidthForAlignment(textAlign, 0, logicalLeft, totalLogicalWidth, truncatedWidth, 0);
                if (ltr)
                    curr->adjustLogicalPosition(logicalLeft, 0);
                else
                    curr->adjustLogicalPosition(-(truncatedWidth - (logicalLeft + totalLogicalWidth)), 0);
            }
        }
        firstLine = false;
    }
}

bool RenderBlock::positionNewFloatOnLine(FloatingObject* newFloat, FloatingObject* lastFloatFromPreviousLine, LineInfo& lineInfo, LineWidth& width)
{
    if (!positionNewFloats())
        return false;

    width.shrinkAvailableWidthForNewFloatIfNeeded(newFloat);

    // We only connect floats to lines for pagination purposes if the floats occur at the start of
    // the line and the previous line had a hard break (so this line is either the first in the block
    // or follows a <br>).
    if (!newFloat->m_paginationStrut || !lineInfo.previousLineBrokeCleanly() || !lineInfo.isEmpty())
        return true;

    const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
    ASSERT(floatingObjectSet.last() == newFloat);

    LayoutUnit floatLogicalTop = logicalTopForFloat(newFloat);
    int paginationStrut = newFloat->m_paginationStrut;

    if (floatLogicalTop - paginationStrut != logicalHeight() + lineInfo.floatPaginationStrut())
        return true;

    FloatingObjectSetIterator it = floatingObjectSet.end();
    --it; // Last float is newFloat, skip that one.
    FloatingObjectSetIterator begin = floatingObjectSet.begin();
    while (it != begin) {
        --it;
        FloatingObject* f = *it;
        if (f == lastFloatFromPreviousLine)
            break;
        if (logicalTopForFloat(f) == logicalHeight() + lineInfo.floatPaginationStrut()) {
            f->m_paginationStrut += paginationStrut;
            RenderBox* o = f->m_renderer;
            setLogicalTopForChild(o, logicalTopForChild(o) + marginBeforeForChild(o) + paginationStrut);
            if (o->isRenderBlock())
                toRenderBlock(o)->setChildNeedsLayout(true, MarkOnlyThis);
            o->layoutIfNeeded();
            // Save the old logical top before calling removePlacedObject which will set
            // isPlaced to false. Otherwise it will trigger an assert in logicalTopForFloat.
            LayoutUnit oldLogicalTop = logicalTopForFloat(f);
            m_floatingObjects->removePlacedObject(f);
            setLogicalTopForFloat(f, oldLogicalTop + paginationStrut);
            m_floatingObjects->addPlacedObject(f);
        }
    }

    // Just update the line info's pagination strut without altering our logical height yet. If the line ends up containing
    // no content, then we don't want to improperly grow the height of the block.
    lineInfo.setFloatPaginationStrut(lineInfo.floatPaginationStrut() + paginationStrut);
    return true;
}

LayoutUnit RenderBlock::startAlignedOffsetForLine(LayoutUnit position, bool firstLine)
{
    ETextAlign textAlign = style()->textAlign();

    if (textAlign == TASTART) // FIXME: Handle TAEND here
        return startOffsetForLine(position, firstLine);

    // updateLogicalWidthForAlignment() handles the direction of the block so no need to consider it here
    float totalLogicalWidth = 0;
    float logicalLeft = logicalLeftOffsetForLine(logicalHeight(), false);
    float availableLogicalWidth = logicalRightOffsetForLine(logicalHeight(), false) - logicalLeft;
    updateLogicalWidthForAlignment(textAlign, 0, logicalLeft, totalLogicalWidth, availableLogicalWidth, 0);

    if (!style()->isLeftToRightDirection())
        return logicalWidth() - logicalLeft;
    return logicalLeft;
}


void RenderBlock::layoutLineGridBox()
{
    if (style()->lineGrid() == RenderStyle::initialLineGrid()) {
        setLineGridBox(0);
        return;
    }
    
    setLineGridBox(0);

    RootInlineBox* lineGridBox = new (renderArena()) RootInlineBox(this);
    lineGridBox->setHasTextChildren(); // Needed to make the line ascent/descent actually be honored in quirks mode.
    lineGridBox->setConstructed();
    GlyphOverflowAndFallbackFontsMap textBoxDataMap;
    VerticalPositionCache verticalPositionCache;
    lineGridBox->alignBoxesInBlockDirection(logicalHeight(), textBoxDataMap, verticalPositionCache);
    
    setLineGridBox(lineGridBox);
    
    // FIXME: If any of the characteristics of the box change compared to the old one, then we need to do a deep dirtying
    // (similar to what happens when the page height changes). Ideally, though, we only do this if someone is actually snapping
    // to this grid.
}

}
