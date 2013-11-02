/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2003, 2004, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All right reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2013 ChangSeok Oh <shivamidow@gmail.com>
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

#include "AXObjectCache.h"
#include "BidiResolver.h"
#include "FloatingObjects.h"
#include "Hyphenation.h"
#include "InlineIterator.h"
#include "InlineTextBox.h"
#include "LineInfo.h"
#include "LineLayoutState.h"
#include "Logging.h"
#include "RenderBlockFlow.h"
#include "RenderCombineText.h"
#include "RenderCounter.h"
#include "RenderFlowThread.h"
#include "RenderInline.h"
#include "RenderLayer.h"
#include "RenderLineBreak.h"
#include "RenderListMarker.h"
#include "RenderRegion.h"
#include "RenderRubyRun.h"
#include "RenderView.h"
#include "Settings.h"
#include "SimpleLineLayoutFunctions.h"
#include "TrailingFloatsRootInlineBox.h"
#include "VerticalPositionCache.h"
#include "break_lines.h"
#include <wtf/RefCountedLeakCounter.h>
#include <wtf/StdLibExtras.h>
#include <wtf/Vector.h>
#include <wtf/unicode/CharacterNames.h>

#if ENABLE(CSS_SHAPES)
#include "ShapeInsideInfo.h"
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

struct RenderTextInfo {
    // Destruction of m_layout requires TextLayout to be a complete type, so the constructor and destructor are made non-inline to avoid compilation errors.
    RenderTextInfo();
    ~RenderTextInfo();

    RenderText* m_text;
    OwnPtr<TextLayout> m_layout;
    LazyLineBreakIterator m_lineBreakIterator;
    const Font* m_font;
};

class LineBreaker {
public:
    friend class BreakingContext;
    LineBreaker(RenderBlockFlow& block)
        : m_block(block)
    {
        reset();
    }

    InlineIterator nextLineBreak(InlineBidiResolver&, LineInfo&, RenderTextInfo&, FloatingObject* lastFloatFromPreviousLine, unsigned consecutiveHyphenatedLines, WordMeasurements&);

    bool lineWasHyphenated() { return m_hyphenated; }
    const Vector<RenderBox*>& positionedObjects() { return m_positionedObjects; }
    EClear clear() { return m_clear; }

private:
    void reset();

    InlineIterator nextSegmentBreak(InlineBidiResolver&, LineInfo&, RenderTextInfo&, FloatingObject* lastFloatFromPreviousLine, unsigned consecutiveHyphenatedLines, WordMeasurements&);
    void skipTrailingWhitespace(InlineIterator&, const LineInfo&);
    void skipLeadingWhitespace(InlineBidiResolver&, LineInfo&, FloatingObject* lastFloatFromPreviousLine, LineWidth&);

    RenderBlockFlow& m_block;
    bool m_hyphenated;
    EClear m_clear;
    Vector<RenderBox*> m_positionedObjects;
};

static inline LayoutUnit borderPaddingMarginStart(const RenderInline& child)
{
    return child.marginStart() + child.paddingStart() + child.borderStart();
}

static inline LayoutUnit borderPaddingMarginEnd(const RenderInline& child)
{
    return child.marginEnd() + child.paddingEnd() + child.borderEnd();
}

static inline bool shouldAddBorderPaddingMargin(RenderObject* child)
{
    // When deciding whether we're at the edge of an inline, adjacent collapsed whitespace is the same as no sibling at all.
    return !child || (child->isText() && !toRenderText(child)->textLength());
}

static RenderObject* previousInFlowSibling(RenderObject* child)
{
    child = child->previousSibling();
    while (child && child->isOutOfFlowPositioned())
        child = child->previousSibling();
    return child;
}

static LayoutUnit inlineLogicalWidth(RenderObject* child, bool checkStartEdge = true, bool checkEndEdge = true)
{
    unsigned lineDepth = 1;
    LayoutUnit extraWidth = 0;
    RenderElement* parent = child->parent();
    while (parent->isRenderInline() && lineDepth++ < cMaxLineDepth) {
        const RenderInline& parentAsRenderInline = toRenderInline(*parent);
        if (!isEmptyInline(parentAsRenderInline)) {
            checkStartEdge = checkStartEdge && shouldAddBorderPaddingMargin(previousInFlowSibling(child));
            if (checkStartEdge)
                extraWidth += borderPaddingMarginStart(parentAsRenderInline);
            checkEndEdge = checkEndEdge && shouldAddBorderPaddingMargin(child->nextSibling());
            if (checkEndEdge)
                extraWidth += borderPaddingMarginEnd(parentAsRenderInline);
            if (!checkStartEdge && !checkEndEdge)
                return extraWidth;
        }
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
            UCharDirection charDirection = u_charDirection(current);
            if (charDirection == U_LEFT_TO_RIGHT) {
                dir = LTR;
                return;
            }
            if (charDirection == U_RIGHT_TO_LEFT || charDirection == U_RIGHT_TO_LEFT_ARABIC) {
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
            if (endpoint.m_obj->style().collapseWhiteSpace() && endpoint.m_obj->isText())
                endpoint.m_pos--;
        }
    }
}

// Don't call this directly. Use one of the descriptive helper functions below.
static void deprecatedAddMidpoint(LineMidpointState& lineMidpointState, const InlineIterator& midpoint)
{
    if (lineMidpointState.midpoints.size() <= lineMidpointState.numMidpoints)
        lineMidpointState.midpoints.grow(lineMidpointState.numMidpoints + 10);

    InlineIterator* midpoints = lineMidpointState.midpoints.data();
    midpoints[lineMidpointState.numMidpoints++] = midpoint;
}

static inline void startIgnoringSpaces(LineMidpointState& lineMidpointState, const InlineIterator& midpoint)
{
    ASSERT(!(lineMidpointState.numMidpoints % 2));
    deprecatedAddMidpoint(lineMidpointState, midpoint);
}

static inline void stopIgnoringSpaces(LineMidpointState& lineMidpointState, const InlineIterator& midpoint)
{
    ASSERT(lineMidpointState.numMidpoints % 2);
    deprecatedAddMidpoint(lineMidpointState, midpoint);
}

// When ignoring spaces, this needs to be called for objects that need line boxes such as RenderInlines or
// hard line breaks to ensure that they're not ignored.
static inline void ensureLineBoxInsideIgnoredSpaces(LineMidpointState& lineMidpointState, RenderObject* renderer)
{
    InlineIterator midpoint(0, renderer, 0);
    stopIgnoringSpaces(lineMidpointState, midpoint);
    startIgnoringSpaces(lineMidpointState, midpoint);
}

// Adding a pair of midpoints before a character will split it out into a new line box.
static inline void ensureCharacterGetsLineBox(LineMidpointState& lineMidpointState, InlineIterator& textParagraphSeparator)
{
    InlineIterator midpoint(0, textParagraphSeparator.m_obj, textParagraphSeparator.m_pos);
    startIgnoringSpaces(lineMidpointState, InlineIterator(0, textParagraphSeparator.m_obj, textParagraphSeparator.m_pos - 1));
    stopIgnoringSpaces(lineMidpointState, InlineIterator(0, textParagraphSeparator.m_obj, textParagraphSeparator.m_pos));
}

static inline BidiRun* createRun(int start, int end, RenderObject* obj, InlineBidiResolver& resolver)
{
    return new BidiRun(start, end, obj, resolver.context(), resolver.dir());
}

void RenderBlockFlow::appendRunsForObject(BidiRunList<BidiRun>& runs, int start, int end, RenderObject* obj, InlineBidiResolver& resolver)
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

RootInlineBox* RenderBlockFlow::createRootInlineBox()
{
    return new RootInlineBox(*this);
}

RootInlineBox* RenderBlockFlow::createAndAppendRootInlineBox()
{
    RootInlineBox* rootBox = createRootInlineBox();
    m_lineBoxes.appendLineBox(rootBox);

    if (UNLIKELY(AXObjectCache::accessibilityEnabled()) && m_lineBoxes.firstLineBox() == rootBox) {
        if (AXObjectCache* cache = document().existingAXObjectCache())
            cache->recomputeIsIgnored(this);
    }

    return rootBox;
}


static inline InlineBox* createInlineBoxForRenderer(RenderObject* obj, bool isRootLineBox, bool isOnlyRun = false)
{
    if (isRootLineBox)
        return toRenderBlockFlow(obj)->createAndAppendRootInlineBox();

    if (obj->isText())
        return toRenderText(obj)->createInlineTextBox();

    if (obj->isBox())
        return toRenderBox(obj)->createInlineBox();

    if (obj->isLineBreak()) {
        InlineBox* inlineBox = toRenderLineBreak(obj)->createInlineBox();
        // We only treat a box as text for a <br> if we are on a line by ourself or in strict mode
        // (Note the use of strict mode. In "almost strict" mode, we don't treat the box for <br> as text.)
        inlineBox->setBehavesLikeText(isOnlyRun || obj->document().inNoQuirksMode() || obj->isLineBreakOpportunity());
        return inlineBox;
    }

    return toRenderInline(obj)->createAndAppendInlineFlowBox();
}

// FIXME: Don't let counters mark themselves as needing pref width recalcs during layout
// so we don't need this hack.
static inline void updateCounterIfNeeded(RenderText& renderText)
{
    if (!renderText.preferredLogicalWidthsDirty() || !renderText.isCounter())
        return;
    toRenderCounter(renderText).updateCounter();
}

static inline void dirtyLineBoxesForRenderer(RenderObject& renderer, bool fullLayout)
{
    if (renderer.isText()) {
        RenderText& renderText = toRenderText(renderer);
        updateCounterIfNeeded(renderText);
        renderText.dirtyLineBoxes(fullLayout);
    } else if (renderer.isLineBreak())
        toRenderLineBreak(renderer).dirtyLineBoxes(fullLayout);
    else
        toRenderInline(renderer).dirtyLineBoxes(fullLayout);
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

InlineFlowBox* RenderBlockFlow::createLineBoxes(RenderObject* obj, const LineInfo& lineInfo, InlineBox* childBox, bool startNewSegment)
{
    // See if we have an unconstructed line box for this object that is also
    // the last item on the line.
    unsigned lineDepth = 1;
    InlineFlowBox* parentBox = 0;
    InlineFlowBox* result = 0;
    bool hasDefaultLineBoxContain = style().lineBoxContain() == RenderStyle::initialLineBoxContain();
    do {
        ASSERT_WITH_SECURITY_IMPLICATION(obj->isRenderInline() || obj == this);

        RenderInline* inlineFlow = (obj != this) ? toRenderInline(obj) : 0;

        // Get the last box we made for this render object.
        parentBox = inlineFlow ? inlineFlow->lastLineBox() : toRenderBlockFlow(obj)->lastLineBox();

        // If this box or its ancestor is constructed then it is from a previous line, and we need
        // to make a new box for our line.  If this box or its ancestor is unconstructed but it has
        // something following it on the line, then we know we have to make a new box
        // as well.  In this situation our inline has actually been split in two on
        // the same line (this can happen with very fancy language mixtures).
        bool constructedNewBox = false;
        bool allowedToConstructNewBox = !hasDefaultLineBoxContain || !inlineFlow || inlineFlow->alwaysCreateLineBoxes();
        bool mustCreateBoxesToRoot = startNewSegment && !(parentBox && parentBox->isRootInlineBox());
        bool canUseExistingParentBox = parentBox && !parentIsConstructedOrHaveNext(parentBox) && !mustCreateBoxesToRoot;
        if (allowedToConstructNewBox && !canUseExistingParentBox) {
            // We need to make a new box for this render object.  Once
            // made, we need to place it at the end of the current line.
            InlineBox* newBox = createInlineBoxForRenderer(obj, obj == this);
            ASSERT_WITH_SECURITY_IMPLICATION(newBox->isInlineFlowBox());
            parentBox = toInlineFlowBox(newBox);
            parentBox->setIsFirstLine(lineInfo.isFirstLine());
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

template <typename CharacterType>
static inline bool endsWithASCIISpaces(const CharacterType* characters, unsigned pos, unsigned end)
{
    while (isASCIISpace(characters[pos])) {
        pos++;
        if (pos >= end)
            return true;
    }
    return false;
}

static bool reachedEndOfTextRenderer(const BidiRunList<BidiRun>& bidiRuns)
{
    BidiRun* run = bidiRuns.logicallyLastRun();
    if (!run)
        return true;
    unsigned pos = run->stop();
    RenderObject* r = run->m_object;
    if (!r->isText())
        return false;
    RenderText* renderText = toRenderText(r);
    unsigned length = renderText->textLength();
    if (pos >= length)
        return true;

    if (renderText->is8Bit())
        return endsWithASCIISpaces(renderText->characters8(), pos, length);
    return endsWithASCIISpaces(renderText->characters16(), pos, length);
}

RootInlineBox* RenderBlockFlow::constructLine(BidiRunList<BidiRun>& bidiRuns, const LineInfo& lineInfo)
{
    ASSERT(bidiRuns.firstRun());

    bool rootHasSelectedChildren = false;
    InlineFlowBox* parentBox = 0;
    int runCount = bidiRuns.runCount() - lineInfo.runsFromLeadingWhitespace();
    for (BidiRun* r = bidiRuns.firstRun(); r; r = r->next()) {
        // Create a box for our object.
        bool isOnlyRun = (runCount == 1);
        if (runCount == 2 && !r->m_object->isListMarker())
            isOnlyRun = (!style().isLeftToRightDirection() ? bidiRuns.lastRun() : bidiRuns.firstRun())->m_object->isListMarker();

        if (lineInfo.isEmpty())
            continue;

        InlineBox* box = createInlineBoxForRenderer(r->m_object, false, isOnlyRun);
        r->m_box = box;

        ASSERT(box);
        if (!box)
            continue;

        if (!rootHasSelectedChildren && box->renderer().selectionState() != RenderObject::SelectionNone)
            rootHasSelectedChildren = true;

        // If we have no parent box yet, or if the run is not simply a sibling,
        // then we need to construct inline boxes as necessary to properly enclose the
        // run's inline box. Segments can only be siblings at the root level, as
        // they are positioned separately.
#if ENABLE(CSS_SHAPES)
        bool runStartsSegment = r->m_startsSegment;
#else
        bool runStartsSegment = false;
#endif
        if (!parentBox || &parentBox->renderer() != r->m_object->parent() || runStartsSegment)
            // Create new inline boxes all the way back to the appropriate insertion point.
            parentBox = createLineBoxes(r->m_object->parent(), lineInfo, box, runStartsSegment);
        else {
            // Append the inline box to this line.
            parentBox->addToLine(box);
        }

        bool visuallyOrdered = r->m_object->style().rtlOrdering() == VisualOrder;
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
        lastLineBox()->root().setHasSelectedChildren(true);

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
    ETextAlign alignment = style().textAlign();
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

void RenderBlockFlow::setMarginsForRubyRun(BidiRun* run, RenderRubyRun& renderer, RenderObject* previousObject, const LineInfo& lineInfo)
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
    renderer.getOverhang(lineInfo.isFirstLine(), renderer.style().isLeftToRightDirection() ? previousObject : nextObject, renderer.style().isLeftToRightDirection() ? nextObject : previousObject, startOverhang, endOverhang);
    setMarginStartForChild(renderer, -startOverhang);
    setMarginEndForChild(renderer, -endOverhang);
}

static inline float measureHyphenWidth(RenderText* renderer, const Font& font, HashSet<const SimpleFontData*>* fallbackFonts = 0)
{
    const RenderStyle& style = renderer->style();
    return font.width(RenderBlock::constructTextRun(renderer, font, style.hyphenString().string(), style), fallbackFonts);
}

class WordMeasurement {
public:
    WordMeasurement()
        : renderer(0)
        , width(0)
        , startOffset(0)
        , endOffset(0)
    {
    }
    
    RenderText* renderer;
    float width;
    int startOffset;
    int endOffset;
    HashSet<const SimpleFontData*> fallbackFonts;
};

static inline const RenderStyle& lineStyle(const RenderElement& renderer, const LineInfo& lineInfo)
{
    return lineInfo.isFirstLine() ? renderer.firstLineStyle() : renderer.style();
}

static inline void setLogicalWidthForTextRun(RootInlineBox* lineBox, BidiRun* run, RenderText* renderer, float xPos, const LineInfo& lineInfo,
    GlyphOverflowAndFallbackFontsMap& textBoxDataMap, VerticalPositionCache& verticalPositionCache, WordMeasurements& wordMeasurements)
{
    HashSet<const SimpleFontData*> fallbackFonts;
    GlyphOverflow glyphOverflow;

    const Font& font = lineStyle(*renderer->parent(), lineInfo).font();
    // Always compute glyph overflow if the block's line-box-contain value is "glyphs".
    if (lineBox->fitsToGlyphs()) {
        // If we don't stick out of the root line's font box, then don't bother computing our glyph overflow. This optimization
        // will keep us from computing glyph bounds in nearly all cases.
        bool includeRootLine = lineBox->includesRootLineBoxFontOrLeading();
        int baselineShift = lineBox->verticalPositionForBox(run->m_box, verticalPositionCache);
        int rootDescent = includeRootLine ? font.fontMetrics().descent() : 0;
        int rootAscent = includeRootLine ? font.fontMetrics().ascent() : 0;
        int boxAscent = font.fontMetrics().ascent() - baselineShift;
        int boxDescent = font.fontMetrics().descent() + baselineShift;
        if (boxAscent > rootDescent ||  boxDescent > rootAscent)
            glyphOverflow.computeBounds = true; 
    }
    
    LayoutUnit hyphenWidth = 0;
    if (toInlineTextBox(run->m_box)->hasHyphen())
        hyphenWidth = measureHyphenWidth(renderer, font, &fallbackFonts);

    float measuredWidth = 0;

    bool kerningIsEnabled = font.typesettingFeatures() & Kerning;
    bool canUseSimpleFontCodePath = renderer->canUseSimpleFontCodePath();
    
    // Since we don't cache glyph overflows, we need to re-measure the run if
    // the style is linebox-contain: glyph.
    
    if (!lineBox->fitsToGlyphs() && canUseSimpleFontCodePath) {
        int lastEndOffset = run->m_start;
        for (size_t i = 0, size = wordMeasurements.size(); i < size && lastEndOffset < run->m_stop; ++i) {
            WordMeasurement& wordMeasurement = wordMeasurements[i];
            if (wordMeasurement.width <= 0 || wordMeasurement.startOffset == wordMeasurement.endOffset)
                continue;
            if (wordMeasurement.renderer != renderer || wordMeasurement.startOffset != lastEndOffset || wordMeasurement.endOffset > run->m_stop)
                continue;

            lastEndOffset = wordMeasurement.endOffset;
            if (kerningIsEnabled && lastEndOffset == run->m_stop) {
                int wordLength = lastEndOffset - wordMeasurement.startOffset;
                GlyphOverflow overflow;
                measuredWidth += renderer->width(wordMeasurement.startOffset, wordLength, xPos + measuredWidth, lineInfo.isFirstLine(),
                    &wordMeasurement.fallbackFonts, &overflow);
                UChar c = renderer->characterAt(wordMeasurement.startOffset);
                if (i > 0 && wordLength == 1 && (c == ' ' || c == '\t'))
                    measuredWidth += renderer->style().wordSpacing();
            } else
                measuredWidth += wordMeasurement.width;
            if (!wordMeasurement.fallbackFonts.isEmpty()) {
                HashSet<const SimpleFontData*>::const_iterator end = wordMeasurement.fallbackFonts.end();
                for (HashSet<const SimpleFontData*>::const_iterator it = wordMeasurement.fallbackFonts.begin(); it != end; ++it)
                    fallbackFonts.add(*it);
            }
        }
        if (measuredWidth && lastEndOffset != run->m_stop) {
            // If we don't have enough cached data, we'll measure the run again.
            measuredWidth = 0;
            fallbackFonts.clear();
        }
    }

    if (!measuredWidth)
        measuredWidth = renderer->width(run->m_start, run->m_stop - run->m_start, xPos, lineInfo.isFirstLine(), &fallbackFonts, &glyphOverflow);

    run->m_box->setLogicalWidth(measuredWidth + hyphenWidth);
    if (!fallbackFonts.isEmpty()) {
        ASSERT(run->m_box->behavesLikeText());
        GlyphOverflowAndFallbackFontsMap::iterator it = textBoxDataMap.add(toInlineTextBox(run->m_box), make_pair(Vector<const SimpleFontData*>(), GlyphOverflow())).iterator;
        ASSERT(it->value.first.isEmpty());
        copyToVector(fallbackFonts, it->value.first);
        run->m_box->parent()->clearDescendantsHaveSameLineHeightAndBaseline();
    }
    if ((glyphOverflow.top || glyphOverflow.bottom || glyphOverflow.left || glyphOverflow.right)) {
        ASSERT(run->m_box->behavesLikeText());
        GlyphOverflowAndFallbackFontsMap::iterator it = textBoxDataMap.add(toInlineTextBox(run->m_box), make_pair(Vector<const SimpleFontData*>(), GlyphOverflow())).iterator;
        it->value.second = glyphOverflow;
        run->m_box->clearKnownToHaveNoOverflow();
    }
}

static inline void computeExpansionForJustifiedText(BidiRun* firstRun, BidiRun* trailingSpaceRun, Vector<unsigned, 16>& expansionOpportunities, unsigned expansionOpportunityCount, float& totalLogicalWidth, float availableLogicalWidth)
{
    if (!expansionOpportunityCount || availableLogicalWidth <= totalLogicalWidth)
        return;

    size_t i = 0;
    for (BidiRun* r = firstRun; r; r = r->next()) {
#if ENABLE(CSS_SHAPES)
        // This method is called once per segment, do not move past the current segment.
        if (r->m_startsSegment)
            break;
#endif
        if (!r->m_box || r == trailingSpaceRun)
            continue;
        
        if (r->m_object->isText()) {
            unsigned opportunitiesInRun = expansionOpportunities[i++];
            
            ASSERT(opportunitiesInRun <= expansionOpportunityCount);
            
            // Only justify text if whitespace is collapsed.
            if (r->m_object->style().collapseWhiteSpace()) {
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
        updateLogicalWidthForLeftAlignedBlock(style().isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
        break;
    case RIGHT:
    case WEBKIT_RIGHT:
        updateLogicalWidthForRightAlignedBlock(style().isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
        break;
    case CENTER:
    case WEBKIT_CENTER:
        updateLogicalWidthForCenterAlignedBlock(style().isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
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
        if (style().isLeftToRightDirection())
            updateLogicalWidthForLeftAlignedBlock(style().isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
        else
            updateLogicalWidthForRightAlignedBlock(style().isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
        break;
    case TAEND:
        if (style().isLeftToRightDirection())
            updateLogicalWidthForRightAlignedBlock(style().isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
        else
            updateLogicalWidthForLeftAlignedBlock(style().isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
        break;
    }
}

static IndentTextOrNot requiresIndent(bool isFirstLine, bool isAfterHardLineBreak, const RenderStyle& style)
{
    IndentTextOrNot shouldIndentText = DoNotIndentText;
    if (isFirstLine)
        shouldIndentText = IndentText;
#if ENABLE(CSS3_TEXT)
    else if (isAfterHardLineBreak && style.textIndentLine() == TextIndentEachLine)
        shouldIndentText = IndentText;

    if (style.textIndentType() == TextIndentHanging)
        shouldIndentText = shouldIndentText == IndentText ? DoNotIndentText : IndentText;
#else
    UNUSED_PARAM(isAfterHardLineBreak);
    UNUSED_PARAM(style);
#endif
    return shouldIndentText;
}

static void updateLogicalInlinePositions(RenderBlockFlow* block, float& lineLogicalLeft, float& lineLogicalRight, float& availableLogicalWidth, bool firstLine, IndentTextOrNot shouldIndentText, LayoutUnit boxLogicalHeight)
{
    LayoutUnit lineLogicalHeight = block->minLineHeightForReplacedRenderer(firstLine, boxLogicalHeight);
    lineLogicalLeft = block->pixelSnappedLogicalLeftOffsetForLine(block->logicalHeight(), shouldIndentText == IndentText, lineLogicalHeight);
    lineLogicalRight = block->pixelSnappedLogicalRightOffsetForLine(block->logicalHeight(), shouldIndentText == IndentText, lineLogicalHeight);
    availableLogicalWidth = lineLogicalRight - lineLogicalLeft;
}

void RenderBlockFlow::computeInlineDirectionPositionsForLine(RootInlineBox* lineBox, const LineInfo& lineInfo, BidiRun* firstRun, BidiRun* trailingSpaceRun, bool reachedEnd,
                                                         GlyphOverflowAndFallbackFontsMap& textBoxDataMap, VerticalPositionCache& verticalPositionCache, WordMeasurements& wordMeasurements)
{
    ETextAlign textAlign = textAlignmentForLine(!reachedEnd && !lineBox->endsWithBreak());
    
    // CSS 2.1: "'Text-indent' only affects a line if it is the first formatted line of an element. For example, the first line of an anonymous block 
    // box is only affected if it is the first child of its parent element."
    // CSS3 "text-indent", "-webkit-each-line" affects the first line of the block container as well as each line after a forced line break,
    // but does not affect lines after a soft wrap break.
    bool isFirstLine = lineInfo.isFirstLine() && !(isAnonymousBlock() && parent()->firstChild() != this);
    bool isAfterHardLineBreak = lineBox->prevRootBox() && lineBox->prevRootBox()->endsWithBreak();
    IndentTextOrNot shouldIndentText = requiresIndent(isFirstLine, isAfterHardLineBreak, style());
    float lineLogicalLeft;
    float lineLogicalRight;
    float availableLogicalWidth;
    updateLogicalInlinePositions(this, lineLogicalLeft, lineLogicalRight, availableLogicalWidth, isFirstLine, shouldIndentText, 0);
    bool needsWordSpacing;
#if ENABLE(CSS_SHAPES)
    ShapeInsideInfo* shapeInsideInfo = layoutShapeInsideInfo();
    if (shapeInsideInfo && shapeInsideInfo->hasSegments()) {
        BidiRun* segmentStart = firstRun;
        const SegmentList& segments = shapeInsideInfo->segments();
        float logicalLeft = max<float>(roundToInt(segments[0].logicalLeft), lineLogicalLeft);
        float logicalRight = min<float>(floorToInt(segments[0].logicalRight), lineLogicalRight);
        float startLogicalLeft = logicalLeft;
        float endLogicalRight = logicalLeft;
        float minLogicalLeft = logicalLeft;
        float maxLogicalRight = logicalLeft;
        lineBox->beginPlacingBoxRangesInInlineDirection(logicalLeft);
        for (size_t i = 0; i < segments.size(); i++) {
            if (i) {
                logicalLeft = max<float>(roundToInt(segments[i].logicalLeft), lineLogicalLeft);
                logicalRight = min<float>(floorToInt(segments[i].logicalRight), lineLogicalRight);
            }
            availableLogicalWidth = logicalRight - logicalLeft;
            BidiRun* newSegmentStart = computeInlineDirectionPositionsForSegment(lineBox, lineInfo, textAlign, logicalLeft, availableLogicalWidth, segmentStart, trailingSpaceRun, textBoxDataMap, verticalPositionCache, wordMeasurements);
            needsWordSpacing = false;
            endLogicalRight = lineBox->placeBoxRangeInInlineDirection(segmentStart->m_box, newSegmentStart ? newSegmentStart->m_box : 0, logicalLeft, minLogicalLeft, maxLogicalRight, needsWordSpacing, textBoxDataMap);
            if (!newSegmentStart || !newSegmentStart->next())
                break;
            ASSERT(newSegmentStart->m_startsSegment);
            // Discard the empty segment start marker bidi runs
            segmentStart = newSegmentStart->next();
        }
        lineBox->endPlacingBoxRangesInInlineDirection(startLogicalLeft, endLogicalRight, minLogicalLeft, maxLogicalRight);
        return;
    }
#endif

    if (firstRun && firstRun->m_object->isReplaced()) {
        RenderBox* renderBox = toRenderBox(firstRun->m_object);
        updateLogicalInlinePositions(this, lineLogicalLeft, lineLogicalRight, availableLogicalWidth, isFirstLine, shouldIndentText, renderBox->logicalHeight());
    }

    computeInlineDirectionPositionsForSegment(lineBox, lineInfo, textAlign, lineLogicalLeft, availableLogicalWidth, firstRun, trailingSpaceRun, textBoxDataMap, verticalPositionCache, wordMeasurements);
    // The widths of all runs are now known. We can now place every inline box (and
    // compute accurate widths for the inline flow boxes).
    needsWordSpacing = false;
    lineBox->placeBoxesInInlineDirection(lineLogicalLeft, needsWordSpacing, textBoxDataMap);
}

BidiRun* RenderBlockFlow::computeInlineDirectionPositionsForSegment(RootInlineBox* lineBox, const LineInfo& lineInfo, ETextAlign textAlign, float& logicalLeft, 
    float& availableLogicalWidth, BidiRun* firstRun, BidiRun* trailingSpaceRun, GlyphOverflowAndFallbackFontsMap& textBoxDataMap, VerticalPositionCache& verticalPositionCache,
    WordMeasurements& wordMeasurements)
{
    bool needsWordSpacing = false;
    float totalLogicalWidth = lineBox->getFlowSpacingLogicalWidth();
    unsigned expansionOpportunityCount = 0;
    bool isAfterExpansion = true;
    Vector<unsigned, 16> expansionOpportunities;
    RenderObject* previousObject = 0;

    BidiRun* r = firstRun;
    for (; r; r = r->next()) {
#if ENABLE(CSS_SHAPES)
        // Once we have reached the start of the next segment, we have finished
        // computing the positions for this segment's contents.
        if (r->m_startsSegment)
            break;
#endif
        if (!r->m_box || r->m_object->isOutOfFlowPositioned() || r->m_box->isLineBreak())
            continue; // Positioned objects are only participating to figure out their
                      // correct static x position.  They have no effect on the width.
                      // Similarly, line break boxes have no effect on the width.
        if (r->m_object->isText()) {
            RenderText* rt = toRenderText(r->m_object);
            if (textAlign == JUSTIFY && r != trailingSpaceRun) {
                if (!isAfterExpansion)
                    toInlineTextBox(r->m_box)->setCanHaveLeadingExpansion(true);
                unsigned opportunitiesInRun;
                if (rt->is8Bit())
                    opportunitiesInRun = Font::expansionOpportunityCount(rt->characters8() + r->m_start, r->m_stop - r->m_start, r->m_box->direction(), isAfterExpansion);
                else
                    opportunitiesInRun = Font::expansionOpportunityCount(rt->characters16() + r->m_start, r->m_stop - r->m_start, r->m_box->direction(), isAfterExpansion);
                expansionOpportunities.append(opportunitiesInRun);
                expansionOpportunityCount += opportunitiesInRun;
            }

            if (int length = rt->textLength()) {
                if (!r->m_start && needsWordSpacing && isSpaceOrNewline(rt->characterAt(r->m_start)))
                    totalLogicalWidth += lineStyle(*rt->parent(), lineInfo).font().wordSpacing();
                needsWordSpacing = !isSpaceOrNewline(rt->characterAt(r->m_stop - 1)) && r->m_stop == length;
            }

            setLogicalWidthForTextRun(lineBox, r, rt, totalLogicalWidth, lineInfo, textBoxDataMap, verticalPositionCache, wordMeasurements);
        } else {
            isAfterExpansion = false;
            if (!r->m_object->isRenderInline()) {
                RenderBox& renderBox = toRenderBox(*r->m_object);
                if (renderBox.isRubyRun())
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

    return r;
}

void RenderBlockFlow::computeBlockDirectionPositionsForLine(RootInlineBox* lineBox, BidiRun* firstRun, GlyphOverflowAndFallbackFontsMap& textBoxDataMap,
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
        else if (r->m_object->isLineBreak())
            toRenderLineBreak(r->m_object)->replaceInlineBoxWrapper(r->m_box);
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
        return !renderer->style().preserveNewline();
    if (character == noBreakSpace)
        return renderer->style().nbspMode() == SPACE;
    return false;
}


static void setStaticPositions(RenderBlockFlow& block, RenderBox& child)
{
    // FIXME: The math here is actually not really right. It's a best-guess approximation that
    // will work for the common cases
    RenderElement* containerBlock = child.container();
    LayoutUnit blockHeight = block.logicalHeight();
    if (containerBlock->isRenderInline()) {
        // A relative positioned inline encloses us. In this case, we also have to determine our
        // position as though we were an inline. Set |staticInlinePosition| and |staticBlockPosition| on the relative positioned
        // inline so that we can obtain the value later.
        toRenderInline(containerBlock)->layer()->setStaticInlinePosition(block.startAlignedOffsetForLine(blockHeight, false));
        toRenderInline(containerBlock)->layer()->setStaticBlockPosition(blockHeight);
    }
    block.updateStaticInlinePositionForChild(child, blockHeight);
    child.layer()->setStaticBlockPosition(blockHeight);
}

template <typename CharacterType>
static inline int findFirstTrailingSpace(RenderText* lastText, const CharacterType* characters, int start, int stop)
{
    int firstSpace = stop;
    while (firstSpace > start) {
        UChar current = characters[firstSpace - 1];
        if (!isCollapsibleSpace(current, lastText))
            break;
        firstSpace--;
    }

    return firstSpace;
}

inline BidiRun* RenderBlockFlow::handleTrailingSpaces(BidiRunList<BidiRun>& bidiRuns, BidiContext* currentContext)
{
    if (!bidiRuns.runCount()
        || !bidiRuns.logicallyLastRun()->m_object->style().breakOnlyAfterWhiteSpace()
        || !bidiRuns.logicallyLastRun()->m_object->style().autoWrap())
        return 0;

    BidiRun* trailingSpaceRun = bidiRuns.logicallyLastRun();
    RenderObject* lastObject = trailingSpaceRun->m_object;
    if (!lastObject->isText())
        return 0;

    RenderText* lastText = toRenderText(lastObject);
    int firstSpace;
    if (lastText->is8Bit())
        firstSpace = findFirstTrailingSpace(lastText, lastText->characters8(), trailingSpaceRun->start(), trailingSpaceRun->stop());
    else
        firstSpace = findFirstTrailingSpace(lastText, lastText->characters16(), trailingSpaceRun->start(), trailingSpaceRun->stop());

    if (firstSpace == trailingSpaceRun->stop())
        return 0;

    TextDirection direction = style().direction();
    bool shouldReorder = trailingSpaceRun != (direction == LTR ? bidiRuns.lastRun() : bidiRuns.firstRun());
    if (firstSpace != trailingSpaceRun->start()) {
        BidiContext* baseContext = currentContext;
        while (BidiContext* parent = baseContext->parent())
            baseContext = parent;

        BidiRun* newTrailingRun = new BidiRun(firstSpace, trailingSpaceRun->m_stop, trailingSpaceRun->m_object, baseContext, U_OTHER_NEUTRAL);
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

void RenderBlockFlow::appendFloatingObjectToLastLine(FloatingObject* floatingObject)
{
    ASSERT(!floatingObject->originatingLine());
    floatingObject->setOriginatingLine(lastRootBox());
    lastRootBox()->appendFloat(floatingObject->renderer());
}

// FIXME: This should be a BidiStatus constructor or create method.
static inline BidiStatus statusWithDirection(TextDirection textDirection, bool isOverride)
{
    UCharDirection direction = textDirection == LTR ? U_LEFT_TO_RIGHT : U_RIGHT_TO_LEFT;
    RefPtr<BidiContext> context = BidiContext::create(textDirection == LTR ? 0 : 1, direction, isOverride, FromStyleOrDOM);

    // This copies BidiStatus and may churn the ref on BidiContext. I doubt it matters.
    return BidiStatus(direction, direction, direction, context.release());
}

// FIXME: BidiResolver should have this logic.
static inline void constructBidiRunsForSegment(InlineBidiResolver& topResolver, BidiRunList<BidiRun>& bidiRuns, const InlineIterator& endOfRuns, VisualDirectionOverride override, bool previousLineBrokeCleanly)
{
    // FIXME: We should pass a BidiRunList into createBidiRunsForLine instead
    // of the resolver owning the runs.
    ASSERT(&topResolver.runs() == &bidiRuns);
    ASSERT(topResolver.position() != endOfRuns);
    RenderObject* currentRoot = topResolver.position().root();
    topResolver.createBidiRunsForLine(endOfRuns, override, previousLineBrokeCleanly);

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
        EUnicodeBidi unicodeBidi = isolatedInline->style().unicodeBidi();
        TextDirection direction;
        if (unicodeBidi == Plaintext)
            determineDirectionality(direction, InlineIterator(isolatedInline, isolatedRun->object(), 0));
        else {
            ASSERT(unicodeBidi == Isolate || unicodeBidi == IsolateOverride);
            direction = isolatedInline->style().direction();
        }
        isolatedResolver.setStatus(statusWithDirection(direction, isOverride(unicodeBidi)));

        // FIXME: The fact that we have to construct an Iterator here
        // currently prevents this code from moving into BidiResolver.
        if (!bidiFirstSkippingEmptyInlines(*isolatedInline, &isolatedResolver))
            continue;

        // The starting position is the beginning of the first run within the isolate that was identified
        // during the earlier call to createBidiRunsForLine. This can be but is not necessarily the
        // first run within the isolate.
        InlineIterator iter = InlineIterator(isolatedInline, startObj, isolatedRun->m_start);
        isolatedResolver.setPositionIgnoringNestedIsolates(iter);

        // We stop at the next end of line; we may re-enter this isolate in the next call to constructBidiRuns().
        // FIXME: What should end and previousLineBrokeCleanly be?
        // rniwa says previousLineBrokeCleanly is just a WinIE hack and could always be false here?
        isolatedResolver.createBidiRunsForLine(endOfRuns, NoVisualOverride, previousLineBrokeCleanly);
        // Note that we do not delete the runs from the resolver.
        // We're not guaranteed to get any BidiRuns in the previous step. If we don't, we allow the placeholder
        // itself to be turned into an InlineBox. We can't remove it here without potentially losing track of
        // the logically last run.
        if (isolatedResolver.runs().runCount())
            bidiRuns.replaceRunWithRuns(isolatedRun, isolatedResolver.runs());

        // If we encountered any nested isolate runs, just move them
        // to the top resolver's list for later processing.
        if (!isolatedResolver.isolatedRuns().isEmpty()) {
            topResolver.isolatedRuns().appendVector(isolatedResolver.isolatedRuns());
            isolatedResolver.isolatedRuns().clear();
            currentRoot = isolatedInline;
        }
    }
}

static inline void constructBidiRunsForLine(const RenderBlockFlow* block, InlineBidiResolver& topResolver, BidiRunList<BidiRun>& bidiRuns, const InlineIterator& endOfLine, VisualDirectionOverride override, bool previousLineBrokeCleanly)
{
#if !ENABLE(CSS_SHAPES)
    UNUSED_PARAM(block);
    constructBidiRunsForSegment(topResolver, bidiRuns, endOfLine, override, previousLineBrokeCleanly);
#else
    ShapeInsideInfo* shapeInsideInfo = block->layoutShapeInsideInfo();
    if (!shapeInsideInfo || !shapeInsideInfo->hasSegments()) {
        constructBidiRunsForSegment(topResolver, bidiRuns, endOfLine, override, previousLineBrokeCleanly);
        return;
    }

    const SegmentRangeList& segmentRanges = shapeInsideInfo->segmentRanges();
    ASSERT(segmentRanges.size());

    for (size_t i = 0; i < segmentRanges.size(); i++) {
        LineSegmentIterator iterator = segmentRanges[i].start;
        InlineIterator segmentStart(iterator.root, iterator.object, iterator.offset);
        iterator = segmentRanges[i].end;
        InlineIterator segmentEnd(iterator.root, iterator.object, iterator.offset);
        if (i) {
            ASSERT(segmentStart.m_obj);
            BidiRun* segmentMarker = createRun(segmentStart.m_pos, segmentStart.m_pos, segmentStart.m_obj, topResolver);
            segmentMarker->m_startsSegment = true;
            bidiRuns.addRun(segmentMarker);
            // Do not collapse midpoints between segments
            topResolver.midpointState().betweenMidpoints = false;
        }
        if (segmentStart == segmentEnd)
            continue;
        topResolver.setPosition(segmentStart, numberOfIsolateAncestors(segmentStart));
        constructBidiRunsForSegment(topResolver, bidiRuns, segmentEnd, override, previousLineBrokeCleanly);
    }
#endif
}

// This function constructs line boxes for all of the text runs in the resolver and computes their position.
RootInlineBox* RenderBlockFlow::createLineBoxesFromBidiRuns(BidiRunList<BidiRun>& bidiRuns, const InlineIterator& end, LineInfo& lineInfo, VerticalPositionCache& verticalPositionCache, BidiRun* trailingSpaceRun, WordMeasurements& wordMeasurements)
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
        computeInlineDirectionPositionsForLine(lineBox, lineInfo, bidiRuns.firstRun(), trailingSpaceRun, end.atEnd(), textBoxDataMap, verticalPositionCache, wordMeasurements);
    
    // Now position our text runs vertically.
    computeBlockDirectionPositionsForLine(lineBox, bidiRuns.firstRun(), textBoxDataMap, verticalPositionCache);
    
#if ENABLE(SVG)
    // SVG text layout code computes vertical & horizontal positions on its own.
    // Note that we still need to execute computeVerticalPositionsForLine() as
    // it calls InlineTextBox::positionLineBox(), which tracks whether the box
    // contains reversed text or not. If we wouldn't do that editing and thus
    // text selection in RTL boxes would not work as expected.
    if (isSVGRootInlineBox) {
        ASSERT_WITH_SECURITY_IMPLICATION(isSVGText());
        static_cast<SVGRootInlineBox*>(lineBox)->computePerCharacterLayoutInformation();
    }
#endif
    
    // Compute our overflow now.
    lineBox->computeOverflow(lineBox->lineTop(), lineBox->lineBottom(), textBoxDataMap);
    
#if PLATFORM(MAC)
    // Highlight acts as an overflow inflation.
    if (style().highlight() != nullAtom)
        lineBox->addHighlightOverflow();
#endif
    return lineBox;
}

static void deleteLineRange(LineLayoutState& layoutState, RootInlineBox* startLine, RootInlineBox* stopLine = 0)
{
    RootInlineBox* boxToDelete = startLine;
    while (boxToDelete && boxToDelete != stopLine) {
        layoutState.updateRepaintRangeFromBox(boxToDelete);
        // Note: deleteLineRange(firstRootBox()) is not identical to deleteLineBoxTree().
        // deleteLineBoxTree uses nextLineBox() instead of nextRootBox() when traversing.
        RootInlineBox* next = boxToDelete->nextRootBox();
        boxToDelete->deleteLine();
        boxToDelete = next;
    }
}

void RenderBlockFlow::layoutRunsAndFloats(LineLayoutState& layoutState, bool hasInlineChild)
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
        setNeedsLayout(MarkOnlyThis); // Mark as needing a full layout to force us to repaint.
        if (!view().doingFullRepaint() && hasLayer()) {
            // Because we waited until we were already inside layout to discover
            // that the block really needed a full layout, we missed our chance to repaint the layer
            // before layout started.  Luckily the layer has cached the repaint rect for its original
            // position and size, and so we can use that to make a repaint happen now.
            repaintUsingContainer(containerForRepaint(), pixelSnappedIntRect(layer()->repaintRect()));
        }
    }

    if (containsFloats())
        layoutState.setLastFloat(m_floatingObjects->set().last().get());

    // We also find the first clean line and extract these lines.  We will add them back
    // if we determine that we're able to synchronize after handling all our dirty lines.
    InlineIterator cleanLineStart;
    BidiStatus cleanLineBidiStatus;
    if (!layoutState.isFullLayout() && startLine)
        determineEndPosition(layoutState, startLine, cleanLineStart, cleanLineBidiStatus);

    if (startLine) {
        if (!layoutState.usesRepaintBounds())
            layoutState.setRepaintRange(logicalHeight());
        deleteLineRange(layoutState, startLine);
    }

    if (!layoutState.isFullLayout() && lastRootBox() && lastRootBox()->endsWithBreak()) {
        // If the last line before the start line ends with a line break that clear floats,
        // adjust the height accordingly.
        // A line break can be either the first or the last object on a line, depending on its direction.
        if (InlineBox* lastLeafChild = lastRootBox()->lastLeafChild()) {
            RenderObject* lastObject = &lastLeafChild->renderer();
            if (!lastObject->isBR())
                lastObject = &lastRootBox()->firstLeafChild()->renderer();
            if (lastObject->isBR()) {
                EClear clear = lastObject->style().clear();
                if (clear != CNONE)
                    newLine(clear);
            }
        }
    }

    layoutRunsAndFloatsInRange(layoutState, resolver, cleanLineStart, cleanLineBidiStatus, consecutiveHyphenatedLines);
    linkToEndLineIfNeeded(layoutState);
    repaintDirtyFloats(layoutState.floats());
}

RenderTextInfo::RenderTextInfo()
    : m_text(0)
    , m_font(0)
{
}

RenderTextInfo::~RenderTextInfo()
{
}

// Before restarting the layout loop with a new logicalHeight, remove all floats that were added and reset the resolver.
inline const InlineIterator& RenderBlockFlow::restartLayoutRunsAndFloatsInRange(LayoutUnit oldLogicalHeight, LayoutUnit newLogicalHeight,  FloatingObject* lastFloatFromPreviousLine, InlineBidiResolver& resolver,  const InlineIterator& oldEnd)
{
    removeFloatingObjectsBelow(lastFloatFromPreviousLine, oldLogicalHeight);
    setLogicalHeight(newLogicalHeight);
    resolver.setPositionIgnoringNestedIsolates(oldEnd);
    return oldEnd;
}

#if ENABLE(CSS_SHAPES)
static inline void pushShapeContentOverflowBelowTheContentBox(RenderBlockFlow* block, ShapeInsideInfo* shapeInsideInfo, LayoutUnit lineTop, LayoutUnit lineHeight)
{
    ASSERT(shapeInsideInfo);

    LayoutUnit logicalLineBottom = lineTop + lineHeight;
    LayoutUnit shapeLogicalBottom = shapeInsideInfo->shapeLogicalBottom();
    LayoutUnit shapeContainingBlockLogicalHeight = shapeInsideInfo->shapeContainingBlockLogicalHeight();

    bool isOverflowPositionedAlready = (shapeContainingBlockLogicalHeight - shapeInsideInfo->owner()->borderAndPaddingAfter() + lineHeight) <= lineTop;

    // If the last line overlaps with the shape, we don't need the segments anymore
    if (lineTop < shapeLogicalBottom && shapeLogicalBottom < logicalLineBottom)
        shapeInsideInfo->clearSegments();

    if (logicalLineBottom <= shapeLogicalBottom || !shapeContainingBlockLogicalHeight || isOverflowPositionedAlready)
        return;

    LayoutUnit newLogicalHeight = block->logicalHeight() + (shapeContainingBlockLogicalHeight - (lineTop + shapeInsideInfo->owner()->borderAndPaddingAfter()));
    block->setLogicalHeight(newLogicalHeight);
}

void RenderBlockFlow::updateShapeAndSegmentsForCurrentLine(ShapeInsideInfo*& shapeInsideInfo, const LayoutSize& logicalOffsetFromShapeContainer, LineLayoutState& layoutState)
{
    if (layoutState.flowThread())
        return updateShapeAndSegmentsForCurrentLineInFlowThread(shapeInsideInfo, layoutState);

    if (!shapeInsideInfo)
        return;

    LayoutUnit lineTop = logicalHeight() + logicalOffsetFromShapeContainer.height();
    LayoutUnit lineLeft = logicalOffsetFromShapeContainer.width();
    LayoutUnit lineHeight = this->lineHeight(layoutState.lineInfo().isFirstLine(), isHorizontalWritingMode() ? HorizontalLine : VerticalLine, PositionOfInteriorLineBoxes);

    // FIXME: Bug 95361: It is possible for a line to grow beyond lineHeight, in which case these segments may be incorrect.
    shapeInsideInfo->updateSegmentsForLine(LayoutSize(lineLeft, lineTop), lineHeight);

    pushShapeContentOverflowBelowTheContentBox(this, shapeInsideInfo, lineTop, lineHeight);
}

void RenderBlockFlow::updateShapeAndSegmentsForCurrentLineInFlowThread(ShapeInsideInfo*& shapeInsideInfo, LineLayoutState& layoutState)
{
    ASSERT(layoutState.flowThread());

    RenderRegion* currentRegion = regionAtBlockOffset(logicalHeight());
    if (!currentRegion || !currentRegion->logicalHeight())
        return;

    shapeInsideInfo = currentRegion->shapeInsideInfo();

    RenderRegion* nextRegion = 0;
    if (!currentRegion->isLastRegion()) {
        RenderRegionList regionList = layoutState.flowThread()->renderRegionList();
        auto it = regionList.find(currentRegion);
        nextRegion = *(++it);
    }

    // We only want to deal regions with shapes, so we check if the next region has a shape
    if (!shapeInsideInfo && nextRegion && !nextRegion->shapeInsideInfo())
        return;

    LayoutUnit lineHeight = this->lineHeight(layoutState.lineInfo().isFirstLine(), isHorizontalWritingMode() ? HorizontalLine : VerticalLine, PositionOfInteriorLineBoxes);
    LayoutUnit logicalLineTopInFlowThread = logicalHeight() + offsetFromLogicalTopOfFirstPage();
    LayoutUnit logicalLineBottomInFlowThread = logicalLineTopInFlowThread + lineHeight;
    LayoutUnit logicalRegionTopInFlowThread = currentRegion->logicalTopForFlowThreadContent();
    LayoutUnit logicalRegionBottomInFlowThread = logicalRegionTopInFlowThread + currentRegion->logicalHeight() - currentRegion->borderAndPaddingBefore() - currentRegion->borderAndPaddingAfter();

    LayoutUnit shapeBottomInFlowThread = LayoutUnit::max();
    if (shapeInsideInfo)
        shapeBottomInFlowThread = shapeInsideInfo->shapeLogicalBottom() + currentRegion->logicalTopForFlowThreadContent();

    bool lineOverLapsWithShapeBottom = shapeBottomInFlowThread < logicalLineBottomInFlowThread;
    bool lineOverLapsWithRegionBottom = logicalLineBottomInFlowThread > logicalRegionBottomInFlowThread;
    bool overFlowsToNextRegion = nextRegion && (lineOverLapsWithShapeBottom || lineOverLapsWithRegionBottom);

    // If the line is between two shapes/regions we position the line to the top of the next shape/region
    if (overFlowsToNextRegion) {
        ASSERT(currentRegion != nextRegion);
        LayoutUnit deltaToNextRegion = logicalRegionBottomInFlowThread - logicalLineTopInFlowThread;
        setLogicalHeight(logicalHeight() + deltaToNextRegion);

        currentRegion = nextRegion;
        shapeInsideInfo = currentRegion->shapeInsideInfo();

        logicalLineTopInFlowThread = logicalHeight() + offsetFromLogicalTopOfFirstPage();
        logicalLineBottomInFlowThread = logicalLineTopInFlowThread + lineHeight;
        logicalRegionTopInFlowThread = currentRegion->logicalTopForFlowThreadContent();
        logicalRegionBottomInFlowThread = logicalRegionTopInFlowThread + currentRegion->logicalHeight() - currentRegion->borderAndPaddingBefore() - currentRegion->borderAndPaddingAfter();
    }

    if (!shapeInsideInfo)
        return;

    bool isFirstLineInRegion = logicalLineBottomInFlowThread <= (logicalRegionTopInFlowThread + lineHeight);
    bool isFirstLineAdjusted = (logicalLineTopInFlowThread - logicalRegionTopInFlowThread) < (layoutState.adjustedLogicalLineTop() - currentRegion->borderAndPaddingBefore());
    // We position the first line to the top of the shape in the region or to the previously adjusted position in the shape
    if (isFirstLineInRegion || isFirstLineAdjusted) {
        LayoutUnit shapeTopOffset = layoutState.adjustedLogicalLineTop();

        if (!shapeTopOffset && (shapeInsideInfo->shapeLogicalTop() > 0))
            shapeTopOffset = shapeInsideInfo->shapeLogicalTop();

        LayoutUnit shapePositionInFlowThread = currentRegion->logicalTopForFlowThreadContent() + shapeTopOffset;
        LayoutUnit shapeTopLineTopDelta = shapePositionInFlowThread - logicalLineTopInFlowThread - currentRegion->borderAndPaddingBefore();

        setLogicalHeight(logicalHeight() + shapeTopLineTopDelta);
        logicalLineTopInFlowThread += shapeTopLineTopDelta;
        layoutState.setAdjustedLogicalLineTop(0);
    }

    LayoutUnit lineTop = logicalLineTopInFlowThread - currentRegion->logicalTopForFlowThreadContent() + currentRegion->borderAndPaddingBefore();

    // FIXME: 118571 - Shape inside on a region does not yet take into account its padding for nested flow blocks
    shapeInsideInfo->updateSegmentsForLine(LayoutSize(0, lineTop), lineHeight);

    if (currentRegion->isLastRegion())
        pushShapeContentOverflowBelowTheContentBox(this, shapeInsideInfo, lineTop, lineHeight);
}

static inline float firstPositiveWidth(const WordMeasurements& wordMeasurements)
{
    for (size_t i = 0; i < wordMeasurements.size(); ++i) {
        if (wordMeasurements[i].width > 0)
            return wordMeasurements[i].width;
    }
    return 0;
}

static inline LayoutUnit adjustLogicalLineTop(ShapeInsideInfo* shapeInsideInfo, InlineIterator start, InlineIterator end, const WordMeasurements& wordMeasurements)
{
    if (!shapeInsideInfo || end != start)
        return 0;

    float minWidth = firstPositiveWidth(wordMeasurements);
    ASSERT(minWidth || wordMeasurements.isEmpty());
    if (minWidth > 0 && shapeInsideInfo->adjustLogicalLineTop(minWidth))
        return shapeInsideInfo->logicalLineTop();

    return shapeInsideInfo->shapeLogicalBottom();
}

bool RenderBlockFlow::adjustLogicalLineTopAndLogicalHeightIfNeeded(ShapeInsideInfo* shapeInsideInfo, LayoutUnit absoluteLogicalTop, LineLayoutState& layoutState, InlineBidiResolver& resolver, FloatingObject* lastFloatFromPreviousLine, InlineIterator& end, WordMeasurements& wordMeasurements)
{
    LayoutUnit adjustedLogicalLineTop = adjustLogicalLineTop(shapeInsideInfo, resolver.position(), end, wordMeasurements);

    if (shapeInsideInfo && containsFloats()) {
        lastFloatFromPreviousLine = m_floatingObjects->set().last().get();
        if (!wordMeasurements.size()) {
            LayoutUnit floatLogicalTopOffset = shapeInsideInfo->computeFirstFitPositionForFloat(logicalSizeForFloat(lastFloatFromPreviousLine));
            if (logicalHeight() < floatLogicalTopOffset)
                adjustedLogicalLineTop = floatLogicalTopOffset;
        }
    }

    if (!adjustedLogicalLineTop)
        return false;

    LayoutUnit newLogicalHeight = adjustedLogicalLineTop - absoluteLogicalTop;

    if (layoutState.flowThread()) {
        layoutState.setAdjustedLogicalLineTop(adjustedLogicalLineTop);
        newLogicalHeight = logicalHeight();
    }

    end = restartLayoutRunsAndFloatsInRange(logicalHeight(), newLogicalHeight, lastFloatFromPreviousLine, resolver, end);
    return true;
}
#endif

void RenderBlockFlow::layoutRunsAndFloatsInRange(LineLayoutState& layoutState, InlineBidiResolver& resolver, const InlineIterator& cleanLineStart, const BidiStatus& cleanLineBidiStatus, unsigned consecutiveHyphenatedLines)
{
    const RenderStyle& styleToUse = style();
    bool paginated = view().layoutState() && view().layoutState()->isPaginated();
    LineMidpointState& lineMidpointState = resolver.midpointState();
    InlineIterator end = resolver.position();
    bool checkForEndLineMatch = layoutState.endLine();
    RenderTextInfo renderTextInfo;
    VerticalPositionCache verticalPositionCache;

    LineBreaker lineBreaker(*this);

#if ENABLE(CSS_SHAPES)
    LayoutSize logicalOffsetFromShapeContainer;
    ShapeInsideInfo* shapeInsideInfo = layoutShapeInsideInfo();
    if (shapeInsideInfo) {
        ASSERT(shapeInsideInfo->owner() == this || allowsShapeInsideInfoSharing());
        if (shapeInsideInfo != this->shapeInsideInfo()) {
            // FIXME Bug 100284: If subsequent LayoutStates are pushed, we will have to add
            // their offsets from the original shape-inside container.
            logicalOffsetFromShapeContainer = logicalOffsetFromShapeAncestorContainer(shapeInsideInfo->owner());
        }
        // Begin layout at the logical top of our shape inside.
        if (logicalHeight() + logicalOffsetFromShapeContainer.height() < shapeInsideInfo->shapeLogicalTop()) {
            LayoutUnit logicalHeight = shapeInsideInfo->shapeLogicalTop() - logicalOffsetFromShapeContainer.height();
            if (layoutState.flowThread())
                logicalHeight -= shapeInsideInfo->owner()->borderAndPaddingBefore();
            setLogicalHeight(logicalHeight);
        }
    }
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
        FloatingObject* lastFloatFromPreviousLine = (containsFloats()) ? m_floatingObjects->set().last().get() : 0;

#if ENABLE(CSS_SHAPES)
        updateShapeAndSegmentsForCurrentLine(shapeInsideInfo, logicalOffsetFromShapeContainer, layoutState);
#endif
        WordMeasurements wordMeasurements;
        end = lineBreaker.nextLineBreak(resolver, layoutState.lineInfo(), renderTextInfo, lastFloatFromPreviousLine, consecutiveHyphenatedLines, wordMeasurements);
        renderTextInfo.m_lineBreakIterator.resetPriorContext();
        if (resolver.position().atEnd()) {
            // FIXME: We shouldn't be creating any runs in nextLineBreak to begin with!
            // Once BidiRunList is separated from BidiResolver this will not be needed.
            resolver.runs().deleteRuns();
            resolver.markCurrentRunEmpty(); // FIXME: This can probably be replaced by an ASSERT (or just removed).
            layoutState.setCheckForFloatsFromLastLine(true);
            resolver.setPosition(InlineIterator(resolver.position().root(), 0, 0), 0);
            break;
        }

#if ENABLE(CSS_SHAPES)
        if (adjustLogicalLineTopAndLogicalHeightIfNeeded(shapeInsideInfo, logicalOffsetFromShapeContainer.height(), layoutState, resolver, lastFloatFromPreviousLine, end, wordMeasurements))
            continue;
#endif
        ASSERT(end != resolver.position());

        // This is a short-cut for empty lines.
        if (layoutState.lineInfo().isEmpty()) {
            if (lastRootBox())
                lastRootBox()->setLineBreakInfo(end.m_obj, end.m_pos, resolver.status());
        } else {
            VisualDirectionOverride override = (styleToUse.rtlOrdering() == VisualOrder ? (styleToUse.direction() == LTR ? VisualLeftToRightOverride : VisualRightToLeftOverride) : NoVisualOverride);

            if (isNewUBAParagraph && styleToUse.unicodeBidi() == Plaintext && !resolver.context()->parent()) {
                TextDirection direction = styleToUse.direction();
                determineDirectionality(direction, resolver.position());
                resolver.setStatus(BidiStatus(direction, isOverride(styleToUse.unicodeBidi())));
            }
            // FIXME: This ownership is reversed. We should own the BidiRunList and pass it to createBidiRunsForLine.
            BidiRunList<BidiRun>& bidiRuns = resolver.runs();
            constructBidiRunsForLine(this, resolver, bidiRuns, end, override, layoutState.lineInfo().previousLineBrokeCleanly());
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
            RootInlineBox* lineBox = createLineBoxesFromBidiRuns(bidiRuns, end, layoutState.lineInfo(), verticalPositionCache, trailingSpaceRun, wordMeasurements);

            bidiRuns.deleteRuns();
            resolver.markCurrentRunEmpty(); // FIXME: This can probably be replaced by an ASSERT (or just removed).

            if (lineBox) {
                lineBox->setLineBreakInfo(end.m_obj, end.m_pos, resolver.status());
                if (layoutState.usesRepaintBounds())
                    layoutState.updateRepaintRangeFromBox(lineBox);

                if (paginated) {
                    LayoutUnit adjustment = 0;
                    adjustLinePositionForPagination(lineBox, adjustment, layoutState.flowThread());
                    if (adjustment) {
                        LayoutUnit oldLineWidth = availableLogicalWidthForLine(oldLogicalHeight, layoutState.lineInfo().isFirstLine());
                        lineBox->adjustBlockDirectionPosition(adjustment);
                        if (layoutState.usesRepaintBounds())
                            layoutState.updateRepaintRangeFromBox(lineBox);

                        if (availableLogicalWidthForLine(oldLogicalHeight + adjustment, layoutState.lineInfo().isFirstLine()) != oldLineWidth) {
                            // We have to delete this line, remove all floats that got added, and let line layout re-run.
                            lineBox->deleteLine();
                            end = restartLayoutRunsAndFloatsInRange(oldLogicalHeight, oldLogicalHeight + adjustment, lastFloatFromPreviousLine, resolver, oldEnd);
                            continue;
                        }

                        setLogicalHeight(lineBox->lineBottomWithLeading());
                    }

                    if (layoutState.flowThread())
                        updateRegionForLine(lineBox);
                }
            }
        }

        for (size_t i = 0; i < lineBreaker.positionedObjects().size(); ++i)
            setStaticPositions(*this, *lineBreaker.positionedObjects()[i]);

        if (!layoutState.lineInfo().isEmpty()) {
            layoutState.lineInfo().setFirstLine(false);
            newLine(lineBreaker.clear());
        }

        if (m_floatingObjects && lastRootBox()) {
            const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
            auto it = floatingObjectSet.begin();
            auto end = floatingObjectSet.end();
            if (layoutState.lastFloat()) {
                auto lastFloatIterator = floatingObjectSet.find<FloatingObject&, FloatingObjectHashTranslator>(*layoutState.lastFloat());
                ASSERT(lastFloatIterator != end);
                ++lastFloatIterator;
                it = lastFloatIterator;
            }
            for (; it != end; ++it) {
                FloatingObject* f = it->get();
                appendFloatingObjectToLastLine(f);
                ASSERT(&f->renderer() == &layoutState.floats()[layoutState.floatIndex()].object);
                // If a float's geometry has changed, give up on syncing with clean lines.
                if (layoutState.floats()[layoutState.floatIndex()].rect != f->frameRect())
                    checkForEndLineMatch = false;
                layoutState.setFloatIndex(layoutState.floatIndex() + 1);
            }
            layoutState.setLastFloat(!floatingObjectSet.isEmpty() ? floatingObjectSet.last().get() : nullptr);
        }

        lineMidpointState.reset();
        resolver.setPosition(end, numberOfIsolateAncestors(end));
    }

    // In case we already adjusted the line positions during this layout to avoid widows
    // then we need to ignore the possibility of having a new widows situation.
    // Otherwise, we risk leaving empty containers which is against the block fragmentation principles.
    if (paginated && !style().hasAutoWidows() && !didBreakAtLineToAvoidWidow()) {
        // Check the line boxes to make sure we didn't create unacceptable widows.
        // However, we'll prioritize orphans - so nothing we do here should create
        // a new orphan.

        RootInlineBox* lineBox = lastRootBox();

        // Count from the end of the block backwards, to see how many hanging
        // lines we have.
        RootInlineBox* firstLineInBlock = firstRootBox();
        int numLinesHanging = 1;
        while (lineBox && lineBox != firstLineInBlock && !lineBox->isFirstAfterPageBreak()) {
            ++numLinesHanging;
            lineBox = lineBox->prevRootBox();
        }

        // If there were no breaks in the block, we didn't create any widows.
        if (!lineBox || !lineBox->isFirstAfterPageBreak() || lineBox == firstLineInBlock)
            return;

        if (numLinesHanging < style().widows()) {
            // We have detected a widow. Now we need to work out how many
            // lines there are on the previous page, and how many we need
            // to steal.
            int numLinesNeeded = style().widows() - numLinesHanging;
            RootInlineBox* currentFirstLineOfNewPage = lineBox;

            // Count the number of lines in the previous page.
            lineBox = lineBox->prevRootBox();
            int numLinesInPreviousPage = 1;
            while (lineBox && lineBox != firstLineInBlock && !lineBox->isFirstAfterPageBreak()) {
                ++numLinesInPreviousPage;
                lineBox = lineBox->prevRootBox();
            }

            // If there was an explicit value for orphans, respect that. If not, we still
            // shouldn't create a situation where we make an orphan bigger than the initial value.
            // This means that setting widows implies we also care about orphans, but given
            // the specification says the initial orphan value is non-zero, this is ok. The
            // author is always free to set orphans explicitly as well.
            int orphans = style().hasAutoOrphans() ? style().initialOrphans() : style().orphans();
            int numLinesAvailable = numLinesInPreviousPage - orphans;
            if (numLinesAvailable <= 0)
                return;

            int numLinesToTake = min(numLinesAvailable, numLinesNeeded);
            // Wind back from our first widowed line.
            lineBox = currentFirstLineOfNewPage;
            for (int i = 0; i < numLinesToTake; ++i)
                lineBox = lineBox->prevRootBox();

            // We now want to break at this line. Remember for next layout and trigger relayout.
            setBreakAtLineToAvoidWidow(lineCount(lineBox));
            markLinesDirtyInBlockRange(lastRootBox()->lineBottomWithLeading(), lineBox->lineBottomWithLeading(), lineBox);
        }
    }

    clearDidBreakAtLineToAvoidWidow();
}

void RenderBlockFlow::linkToEndLineIfNeeded(LineLayoutState& layoutState)
{
    if (layoutState.endLine()) {
        if (layoutState.endLineMatched()) {
            bool paginated = view().layoutState() && view().layoutState()->isPaginated();
            // Attach all the remaining lines, and then adjust their y-positions as needed.
            LayoutUnit delta = logicalHeight() - layoutState.endLineLogicalTop();
            for (RootInlineBox* line = layoutState.endLine(); line; line = line->nextRootBox()) {
                line->attachLine();
                if (paginated) {
                    delta -= line->paginationStrut();
                    adjustLinePositionForPagination(line, delta, layoutState.flowThread());
                }
                if (delta) {
                    layoutState.updateRepaintRangeFromBox(line, delta);
                    line->adjustBlockDirectionPosition(delta);
                }
                if (layoutState.flowThread())
                    updateRegionForLine(line);
                if (Vector<RenderBox*>* cleanLineFloats = line->floatsPtr()) {
                    for (auto it = cleanLineFloats->begin(), end = cleanLineFloats->end(); it != end; ++it) {
                        RenderBox* floatingBox = *it;
                        FloatingObject* floatingObject = insertFloatingObject(*floatingBox);
                        ASSERT(!floatingObject->originatingLine());
                        floatingObject->setOriginatingLine(line);
                        setLogicalHeight(logicalTopForChild(*floatingBox) - marginBeforeForChild(*floatingBox) + delta);
                        positionNewFloats();
                    }
                }
            }
            setLogicalHeight(lastRootBox()->lineBottomWithLeading());
        } else {
            // Delete all the remaining lines.
            deleteLineRange(layoutState, layoutState.endLine());
        }
    }
    
    if (m_floatingObjects && (layoutState.checkForFloatsFromLastLine() || positionNewFloats()) && lastRootBox()) {
        // In case we have a float on the last line, it might not be positioned up to now.
        // This has to be done before adding in the bottom border/padding, or the float will
        // include the padding incorrectly. -dwh
        if (layoutState.checkForFloatsFromLastLine()) {
            LayoutUnit bottomVisualOverflow = lastRootBox()->logicalBottomVisualOverflow();
            LayoutUnit bottomLayoutOverflow = lastRootBox()->logicalBottomLayoutOverflow();
            TrailingFloatsRootInlineBox* trailingFloatsLineBox = new TrailingFloatsRootInlineBox(*this);
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
            if (layoutState.flowThread())
                updateRegionForLine(trailingFloatsLineBox);
        }

        const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
        auto it = floatingObjectSet.begin();
        auto end = floatingObjectSet.end();
        if (layoutState.lastFloat()) {
            auto lastFloatIterator = floatingObjectSet.find<FloatingObject&, FloatingObjectHashTranslator>(*layoutState.lastFloat());
            ASSERT(lastFloatIterator != end);
            ++lastFloatIterator;
            it = lastFloatIterator;
        }
        for (; it != end; ++it)
            appendFloatingObjectToLastLine(it->get());
        layoutState.setLastFloat(!floatingObjectSet.isEmpty() ? floatingObjectSet.last().get() : nullptr);
    }
}

void RenderBlockFlow::repaintDirtyFloats(Vector<FloatWithRect>& floats)
{
    size_t floatCount = floats.size();
    // Floats that did not have layout did not repaint when we laid them out. They would have
    // painted by now if they had moved, but if they stayed at (0, 0), they still need to be
    // painted.
    for (size_t i = 0; i < floatCount; ++i) {
        if (!floats[i].everHadLayout) {
            RenderBox& box = floats[i].object;
            if (!box.x() && !box.y() && box.checkForRepaintDuringLayout())
                box.repaint();
        }
    }
}

void RenderBlockFlow::layoutLineBoxes(bool relayoutChildren, LayoutUnit& repaintLogicalTop, LayoutUnit& repaintLogicalBottom)
{
    ASSERT(!m_simpleLineLayout);

    setLogicalHeight(borderAndPaddingBefore());
    
    // Lay out our hypothetical grid line as though it occurs at the top of the block.
    if (view().layoutState() && view().layoutState()->lineGrid() == this)
        layoutLineGridBox();

    RenderFlowThread* flowThread = flowThreadContainingBlock();
    bool clearLinesForPagination = firstLineBox() && flowThread && !flowThread->hasRegions();

    // Figure out if we should clear out our line boxes.
    // FIXME: Handle resize eventually!
    bool isFullLayout = !firstLineBox() || selfNeedsLayout() || relayoutChildren || clearLinesForPagination;
    LineLayoutState layoutState(isFullLayout, repaintLogicalTop, repaintLogicalBottom, flowThread);

    if (isFullLayout)
        lineBoxes().deleteLineBoxes();

    // Text truncation kicks in in two cases:
    //     1) If your overflow isn't visible and your text-overflow-mode isn't clip.
    //     2) If you're an anonymous block with a block parent that satisfies #1.
    // FIXME: CSS3 says that descendants that are clipped must also know how to truncate.  This is insanely
    // difficult to figure out in general (especially in the middle of doing layout), so we only handle the
    // simple case of an anonymous block truncating when it's parent is clipped.
    bool hasTextOverflow = (style().textOverflow() && hasOverflowClip())
        || (isAnonymousBlock() && parent() && parent()->isRenderBlock() && parent()->style().textOverflow() && parent()->hasOverflowClip());

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
        for (InlineWalker walker(*this); !walker.atEnd(); walker.advance()) {
            RenderObject& o = *walker.current();

            if (!hasInlineChild && o.isInline())
                hasInlineChild = true;

            if (o.isReplaced() || o.isFloating() || o.isOutOfFlowPositioned()) {
                RenderBox& box = toRenderBox(o);

                if (relayoutChildren || box.hasRelativeDimensions())
                    box.setChildNeedsLayout(MarkOnlyThis);

                // If relayoutChildren is set and the child has percentage padding or an embedded content box, we also need to invalidate the childs pref widths.
                if (relayoutChildren && box.needsPreferredWidthsRecalculation())
                    box.setPreferredLogicalWidthsDirty(true, MarkOnlyThis);

                if (box.isOutOfFlowPositioned())
                    box.containingBlock()->insertPositionedObject(box);
                else if (box.isFloating())
                    layoutState.floats().append(FloatWithRect(box));
                else if (isFullLayout || box.needsLayout()) {
                    // Replaced element.
                    box.dirtyLineBoxes(isFullLayout);
                    if (isFullLayout)
                        replacedChildren.append(&box);
                    else
                        box.layoutIfNeeded();
                }
            } else if (o.isTextOrLineBreak() || (o.isRenderInline() && !walker.atEndOfInline())) {
                if (o.isRenderInline())
                    toRenderInline(o).updateAlwaysCreateLineBoxes(layoutState.isFullLayout());
                if (layoutState.isFullLayout() || o.selfNeedsLayout())
                    dirtyLineBoxesForRenderer(o, layoutState.isFullLayout());
                o.clearNeedsLayout();
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
        if (!style().isFlippedLinesWritingMode())
            lastLineAnnotationsAdjustment = lastRootBox()->computeUnderAnnotationAdjustment(lowestAllowedPosition);
        else
            lastLineAnnotationsAdjustment = lastRootBox()->computeOverAnnotationAdjustment(lowestAllowedPosition);
    }

    // Now add in the bottom border/padding.
    setLogicalHeight(logicalHeight() + lastLineAnnotationsAdjustment + borderAndPaddingAfter() + scrollbarLogicalHeight());

    if (!firstLineBox() && hasLineIfEmpty())
        setLogicalHeight(logicalHeight() + lineHeight(true, isHorizontalWritingMode() ? HorizontalLine : VerticalLine, PositionOfInteriorLineBoxes));

    // See if we have any lines that spill out of our block.  If we do, then we will possibly need to
    // truncate text.
    if (hasTextOverflow)
        checkLinesForTextOverflow();
}

void RenderBlockFlow::checkFloatsInCleanLine(RootInlineBox* line, Vector<FloatWithRect>& floats, size_t& floatIndex, bool& encounteredNewFloat, bool& dirtiedByFloat)
{
    Vector<RenderBox*>* cleanLineFloats = line->floatsPtr();
    if (!cleanLineFloats)
        return;

    for (auto it = cleanLineFloats->begin(), end = cleanLineFloats->end(); it != end; ++it) {
        RenderBox* floatingBox = *it;
        floatingBox->layoutIfNeeded();
        LayoutSize newSize(floatingBox->width() + floatingBox->marginWidth(), floatingBox->height() + floatingBox->marginHeight());
        ASSERT_WITH_SECURITY_IMPLICATION(floatIndex < floats.size());
        if (&floats[floatIndex].object != floatingBox) {
            encounteredNewFloat = true;
            return;
        }

        if (floats[floatIndex].rect.size() != newSize) {
            LayoutUnit floatTop = isHorizontalWritingMode() ? floats[floatIndex].rect.y() : floats[floatIndex].rect.x();
            LayoutUnit floatHeight = isHorizontalWritingMode() ? max(floats[floatIndex].rect.height(), newSize.height()) : max(floats[floatIndex].rect.width(), newSize.width());
            floatHeight = min(floatHeight, LayoutUnit::max() - floatTop);
            line->markDirty();
            markLinesDirtyInBlockRange(line->lineBottomWithLeading(), floatTop + floatHeight, line);
            floats[floatIndex].rect.setSize(newSize);
            dirtiedByFloat = true;
        }
        floatIndex++;
    }
}

RootInlineBox* RenderBlockFlow::determineStartPosition(LineLayoutState& layoutState, InlineBidiResolver& resolver)
{
    RootInlineBox* curr = 0;
    RootInlineBox* last = 0;

    // FIXME: This entire float-checking block needs to be broken into a new function.
    bool dirtiedByFloat = false;
    if (!layoutState.isFullLayout()) {
        // Paginate all of the clean lines.
        bool paginated = view().layoutState() && view().layoutState()->isPaginated();
        LayoutUnit paginationDelta = 0;
        size_t floatIndex = 0;
        for (curr = firstRootBox(); curr && !curr->isDirty(); curr = curr->nextRootBox()) {
            if (paginated) {
                if (lineWidthForPaginatedLineChanged(curr, 0, layoutState.flowThread())) {
                    curr->markDirty();
                    break;
                }
                paginationDelta -= curr->paginationStrut();
                adjustLinePositionForPagination(curr, paginationDelta, layoutState.flowThread());
                if (paginationDelta) {
                    if (containsFloats() || !layoutState.floats().isEmpty()) {
                        // FIXME: Do better eventually.  For now if we ever shift because of pagination and floats are present just go to a full layout.
                        layoutState.markForFullLayout();
                        break;
                    }

                    layoutState.updateRepaintRangeFromBox(curr, paginationDelta);
                    curr->adjustBlockDirectionPosition(paginationDelta);
                }
                if (layoutState.flowThread())
                    updateRegionForLine(curr);
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
        m_lineBoxes.deleteLineBoxTree();
        curr = 0;

        ASSERT(!firstLineBox() && !lastLineBox());
    } else {
        if (curr) {
            // We have a dirty line.
            if (RootInlineBox* prevRootBox = curr->prevRootBox()) {
                // We have a previous line.
                if (!dirtiedByFloat && (!prevRootBox->endsWithBreak() || !prevRootBox->lineBreakObj() || (prevRootBox->lineBreakObj()->isText() && prevRootBox->lineBreakPos() >= toRenderText(prevRootBox->lineBreakObj())->textLength())))
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
                for (auto it = cleanLineFloats->begin(), end = cleanLineFloats->end(); it != end; ++it) {
                    RenderBox* floatingBox = *it;
                    FloatingObject* floatingObject = insertFloatingObject(*floatingBox);
                    ASSERT(!floatingObject->originatingLine());
                    floatingObject->setOriginatingLine(line);
                    setLogicalHeight(logicalTopForChild(*floatingBox) - marginBeforeForChild(*floatingBox));
                    positionNewFloats();
                    ASSERT(&layoutState.floats()[numCleanFloats].object == floatingBox);
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
        TextDirection direction = style().direction();
        if (style().unicodeBidi() == Plaintext)
            determineDirectionality(direction, InlineIterator(this, bidiFirstSkippingEmptyInlines(*this), 0));
        resolver.setStatus(BidiStatus(direction, isOverride(style().unicodeBidi())));
        InlineIterator iter = InlineIterator(this, bidiFirstSkippingEmptyInlines(*this, &resolver), 0);
        resolver.setPosition(iter, numberOfIsolateAncestors(iter));
    }
    return curr;
}

void RenderBlockFlow::determineEndPosition(LineLayoutState& layoutState, RootInlineBox* startLine, InlineIterator& cleanLineStart, BidiStatus& cleanLineBidiStatus)
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

bool RenderBlockFlow::checkPaginationAndFloatsAtEndLine(LineLayoutState& layoutState)
{
    LayoutUnit lineDelta = logicalHeight() - layoutState.endLineLogicalTop();

    bool paginated = view().layoutState() && view().layoutState()->isPaginated();
    if (paginated && layoutState.flowThread()) {
        // Check all lines from here to the end, and see if the hypothetical new position for the lines will result
        // in a different available line width.
        for (RootInlineBox* lineBox = layoutState.endLine(); lineBox; lineBox = lineBox->nextRootBox()) {
            if (paginated) {
                // This isn't the real move we're going to do, so don't update the line box's pagination
                // strut yet.
                LayoutUnit oldPaginationStrut = lineBox->paginationStrut();
                lineDelta -= oldPaginationStrut;
                adjustLinePositionForPagination(lineBox, lineDelta, layoutState.flowThread());
                lineBox->setPaginationStrut(oldPaginationStrut);
            }
            if (lineWidthForPaginatedLineChanged(lineBox, lineDelta, layoutState.flowThread()))
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
    auto end = floatingObjectSet.end();
    for (auto it = floatingObjectSet.begin(); it != end; ++it) {
        FloatingObject* floatingObject = it->get();
        if (logicalBottomForFloat(floatingObject) >= logicalTop && logicalBottomForFloat(floatingObject) < logicalBottom)
            return false;
    }

    return true;
}

bool RenderBlockFlow::lineWidthForPaginatedLineChanged(RootInlineBox* rootBox, LayoutUnit lineDelta, RenderFlowThread* flowThread) const
{
    if (!flowThread)
        return false;

    RenderRegion* currentRegion = regionAtBlockOffset(rootBox->lineTopWithLeading() + lineDelta);
    // Just bail if the region didn't change.
    if (rootBox->containingRegion() == currentRegion)
        return false;
    return rootBox->paginatedLineWidth() != availableLogicalWidthForContent(currentRegion);
}

bool RenderBlockFlow::matchedEndLine(LineLayoutState& layoutState, const InlineBidiResolver& resolver, const InlineIterator& endLineStart, const BidiStatus& endLineStatus)
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
            deleteLineRange(layoutState, originalEndLine, result);
            return matched;
        }
    }

    return false;
}

static inline bool skipNonBreakingSpace(const InlineIterator& it, const LineInfo& lineInfo)
{
    if (it.m_obj->style().nbspMode() != SPACE || it.current() != noBreakSpace)
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

static bool requiresLineBoxForContent(const RenderInline& flow, const LineInfo& lineInfo)
{
    RenderElement* parent = flow.parent();
    if (flow.document().inNoQuirksMode()) {
        const RenderStyle& flowStyle = lineStyle(flow, lineInfo);
        const RenderStyle& parentStyle = lineStyle(*parent, lineInfo);
        if (flowStyle.lineHeight() != parentStyle.lineHeight()
            || flowStyle.verticalAlign() != parentStyle.verticalAlign()
            || !parentStyle.font().fontMetrics().hasIdenticalAscentDescentAndLineGap(flowStyle.font().fontMetrics()))
        return true;
    }
    return false;
}

static bool hasInlineDirectionBordersPaddingOrMargin(const RenderInline& flow)
{
    // Where an empty inline is split across anonymous blocks we should only give lineboxes to the 'sides' of the
    // inline that have borders, padding or margin.
    bool shouldApplyStartBorderPaddingOrMargin = !flow.parent()->isAnonymousBlock() || !flow.isInlineElementContinuation();
    if (shouldApplyStartBorderPaddingOrMargin && (flow.borderStart() || flow.marginStart() || flow.paddingStart()))
        return true;

    bool shouldApplyEndBorderPaddingOrMargin = !flow.parent()->isAnonymousBlock() || flow.isInlineElementContinuation() || !flow.inlineElementContinuation();
    return shouldApplyEndBorderPaddingOrMargin && (flow.borderEnd() || flow.marginEnd() || flow.paddingEnd());
}

static bool alwaysRequiresLineBox(const RenderInline& flow)
{
    // FIXME: Right now, we only allow line boxes for inlines that are truly empty.
    // We need to fix this, though, because at the very least, inlines containing only
    // ignorable whitespace should should also have line boxes.
    return isEmptyInline(flow) && hasInlineDirectionBordersPaddingOrMargin(flow);
}

static bool requiresLineBox(const InlineIterator& it, const LineInfo& lineInfo = LineInfo(), WhitespacePosition whitespacePosition = LeadingWhitespace)
{
    if (it.m_obj->isFloatingOrOutOfFlowPositioned())
        return false;

    if (it.m_obj->isBR())
        return true;

    bool rendererIsEmptyInline = false;
    if (it.m_obj->isRenderInline()) {
        const RenderInline& inlineRenderer = toRenderInline(*it.m_obj);
        if (!alwaysRequiresLineBox(inlineRenderer) && !requiresLineBoxForContent(inlineRenderer, lineInfo))
            return false;
        rendererIsEmptyInline = isEmptyInline(inlineRenderer);
    }

    if (!shouldCollapseWhiteSpace(&it.m_obj->style(), lineInfo, whitespacePosition))
        return true;

    UChar current = it.current();
    bool notJustWhitespace = current != ' ' && current != '\t' && current != softHyphen && (current != '\n' || it.m_obj->preservesNewline()) && !skipNonBreakingSpace(it, lineInfo);
    return notJustWhitespace || rendererIsEmptyInline;
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
void LineBreaker::skipTrailingWhitespace(InlineIterator& iterator, const LineInfo& lineInfo)
{
    while (!iterator.atEnd() && !requiresLineBox(iterator, lineInfo, TrailingWhitespace)) {
        RenderObject& object = *iterator.m_obj;
        if (object.isOutOfFlowPositioned())
            setStaticPositions(m_block, toRenderBox(object));
        else if (object.isFloating())
            m_block.insertFloatingObject(toRenderBox(object));
        iterator.increment();
    }
}

void LineBreaker::skipLeadingWhitespace(InlineBidiResolver& resolver, LineInfo& lineInfo, FloatingObject* lastFloatFromPreviousLine, LineWidth& width)
{
    while (!resolver.position().atEnd() && !requiresLineBox(resolver.position(), lineInfo, LeadingWhitespace)) {
        RenderObject& object = *resolver.position().m_obj;
        if (object.isOutOfFlowPositioned()) {
            setStaticPositions(m_block, toRenderBox(object));
            if (object.style().isOriginalDisplayInlineType()) {
                resolver.runs().addRun(createRun(0, 1, &object, resolver));
                lineInfo.incrementRunsFromLeadingWhitespace();
            }
        } else if (object.isFloating()) {
            // The top margin edge of a self-collapsing block that clears a float intrudes up into it by the height of the margin,
            // so in order to place this first child float at the top content edge of the self-collapsing block add the margin back in before placement.
            LayoutUnit marginOffset = (!object.previousSibling() && m_block.isSelfCollapsingBlock() && m_block.style().clear() && m_block.getClearDelta(m_block, LayoutUnit())) ? m_block.collapsedMarginBeforeForChild(m_block) : LayoutUnit();
            LayoutUnit oldLogicalHeight = m_block.logicalHeight();
            m_block.setLogicalHeight(oldLogicalHeight + marginOffset);
            m_block.positionNewFloatOnLine(m_block.insertFloatingObject(toRenderBox(object)), lastFloatFromPreviousLine, lineInfo, width);
            m_block.setLogicalHeight(oldLogicalHeight);
        } else if (object.isText() && object.style().hasTextCombine() && object.isCombineText() && !toRenderCombineText(object).isCombined()) {
            toRenderCombineText(object).combineText();
            if (toRenderCombineText(object).isCombined())
                continue;
        }
        resolver.increment();
    }
    resolver.commitExplicitEmbedding();
}

// This is currently just used for list markers and inline flows that have line boxes. Neither should
// have an effect on whitespace at the start of the line.
static bool shouldSkipWhitespaceAfterStartObject(RenderBlockFlow& block, RenderObject* o, LineMidpointState& lineMidpointState)
{
    RenderObject* next = bidiNextSkippingEmptyInlines(block, o);
    while (next && next->isFloatingOrOutOfFlowPositioned())
        next = bidiNextSkippingEmptyInlines(block, next);

    if (next && next->isText() && toRenderText(next)->textLength() > 0) {
        RenderText* nextText = toRenderText(next);
        UChar nextChar = nextText->characterAt(0);
        if (nextText->style().isCollapsibleWhiteSpace(nextChar)) {
            startIgnoringSpaces(lineMidpointState, InlineIterator(0, o, 0));
            return true;
        }
    }

    return false;
}

static ALWAYS_INLINE float textWidth(RenderText* text, unsigned from, unsigned len, const Font& font, float xPos, bool isFixedPitch, bool collapseWhiteSpace, HashSet<const SimpleFontData*>& fallbackFonts, TextLayout* layout = 0)
{
    const RenderStyle& style = text->style();

    GlyphOverflow glyphOverflow;
    if (isFixedPitch || (!from && len == text->textLength()) || style.hasTextCombine())
        return text->width(from, len, font, xPos, &fallbackFonts, &glyphOverflow);

    if (layout)
        return Font::width(*layout, from, len, &fallbackFonts);

    TextRun run = RenderBlock::constructTextRun(text, font, text, from, len, style);
    run.setCharactersLength(text->textLength() - from);
    ASSERT(run.charactersLength() >= run.length());

    run.setCharacterScanForCodePath(!text->canUseSimpleFontCodePath());
    run.setTabSize(!collapseWhiteSpace, style.tabSize());
    run.setXPos(xPos);
    return font.width(run, &fallbackFonts, &glyphOverflow);
}

static void tryHyphenating(RenderText* text, const Font& font, const AtomicString& localeIdentifier, unsigned consecutiveHyphenatedLines, int consecutiveHyphenatedLinesLimit, int minimumPrefixLimit, int minimumSuffixLimit, unsigned lastSpace, unsigned pos, float xPos, int availableWidth, bool isFixedPitch, bool collapseWhiteSpace, int lastSpaceWordSpacing, InlineIterator& lineBreak, int nextBreakable, bool& hyphenated)
{
    // Map 'hyphenate-limit-{before,after}: auto;' to 2.
    unsigned minimumPrefixLength;
    unsigned minimumSuffixLength;

    if (minimumPrefixLimit < 0)
        minimumPrefixLength = 2;
    else
        minimumPrefixLength = static_cast<unsigned>(minimumPrefixLimit);

    if (minimumSuffixLimit < 0)
        minimumSuffixLength = 2;
    else
        minimumSuffixLength = static_cast<unsigned>(minimumSuffixLimit);

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

    const RenderStyle& style = text->style();
    TextRun run = RenderBlock::constructTextRun(text, font, text, lastSpace, pos - lastSpace, style);
    run.setCharactersLength(text->textLength() - lastSpace);
    ASSERT(run.charactersLength() >= run.length());

    run.setTabSize(!collapseWhiteSpace, style.tabSize());
    run.setXPos(xPos + lastSpaceWordSpacing);

    unsigned prefixLength = font.offsetForPosition(run, maxPrefixWidth, false);
    if (prefixLength < minimumPrefixLength)
        return;

    prefixLength = lastHyphenLocation(text->characters() + lastSpace, pos - lastSpace, min(prefixLength, pos - lastSpace - minimumSuffixLength) + 1, localeIdentifier);
    if (!prefixLength || prefixLength < minimumPrefixLength)
        return;

    // When lastSapce is a space, which it always is except sometimes at the beginning of a line or after collapsed
    // space, it should not count towards hyphenate-limit-before.
    if (prefixLength == minimumPrefixLength) {
        UChar characterAtLastSpace = text->characterAt(lastSpace);
        if (characterAtLastSpace == ' ' || characterAtLastSpace == '\n' || characterAtLastSpace == '\t' || characterAtLastSpace == noBreakSpace)
            return;
    }

    ASSERT(pos - lastSpace - prefixLength >= minimumSuffixLength);

#if !ASSERT_DISABLED
    HashSet<const SimpleFontData*> fallbackFonts;
    float prefixWidth = hyphenWidth + textWidth(text, lastSpace, prefixLength, font, xPos, isFixedPitch, collapseWhiteSpace, fallbackFonts) + lastSpaceWordSpacing;
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
    void appendBoxIfNeeded(RenderBoxModelObject*);

    enum CollapseFirstSpaceOrNot { DoNotCollapseFirstSpace, CollapseFirstSpace };

    void updateMidpointsForTrailingBoxes(LineMidpointState&, const InlineIterator& lBreak, CollapseFirstSpaceOrNot);

private:
    RenderText* m_whitespace;
    Vector<RenderBoxModelObject*, 4> m_boxes;
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
    m_boxes.shrink(0); // Use shrink(0) instead of clear() to retain our capacity.
}

inline void TrailingObjects::appendBoxIfNeeded(RenderBoxModelObject* box)
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
                ensureLineBoxInsideIgnoredSpaces(lineMidpointState, m_boxes[i]);
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
        startIgnoringSpaces(lineMidpointState, endMid);
        for (size_t i = 0; i < m_boxes.size(); ++i) {
            ensureLineBoxInsideIgnoredSpaces(lineMidpointState, m_boxes[i]);
        }
    }
}

void LineBreaker::reset()
{
    m_positionedObjects.clear();
    m_hyphenated = false;
    m_clear = CNONE;
}

InlineIterator LineBreaker::nextLineBreak(InlineBidiResolver& resolver, LineInfo& lineInfo, RenderTextInfo& renderTextInfo, FloatingObject* lastFloatFromPreviousLine, unsigned consecutiveHyphenatedLines, WordMeasurements& wordMeasurements)
{
#if !ENABLE(CSS_SHAPES)
    return nextSegmentBreak(resolver, lineInfo, renderTextInfo, lastFloatFromPreviousLine, consecutiveHyphenatedLines, wordMeasurements);
#else
    ShapeInsideInfo* shapeInsideInfo = m_block.layoutShapeInsideInfo();

    if (!shapeInsideInfo || !shapeInsideInfo->lineOverlapsShapeBounds())
        return nextSegmentBreak(resolver, lineInfo, renderTextInfo, lastFloatFromPreviousLine, consecutiveHyphenatedLines, wordMeasurements);

    InlineIterator end = resolver.position();
    InlineIterator oldEnd = end;

    if (!shapeInsideInfo->hasSegments()) {
        end = nextSegmentBreak(resolver, lineInfo, renderTextInfo, lastFloatFromPreviousLine, consecutiveHyphenatedLines, wordMeasurements);
        resolver.setPositionIgnoringNestedIsolates(oldEnd);
        return oldEnd;
    }

    const SegmentList& segments = shapeInsideInfo->segments();
    SegmentRangeList& segmentRanges = shapeInsideInfo->segmentRanges();

    for (unsigned i = 0; i < segments.size() && !end.atEnd(); i++) {
        InlineIterator segmentStart = resolver.position();
        end = nextSegmentBreak(resolver, lineInfo, renderTextInfo, lastFloatFromPreviousLine, consecutiveHyphenatedLines, wordMeasurements);

        ASSERT(segmentRanges.size() == i);
        if (resolver.position().atEnd()) {
            segmentRanges.append(LineSegmentRange(segmentStart, end));
            break;
        }
        if (resolver.position() == end) {
            // Nothing fit this segment
            end = segmentStart;
            segmentRanges.append(LineSegmentRange(segmentStart, segmentStart));
            resolver.setPositionIgnoringNestedIsolates(segmentStart);
        } else {
            // Note that resolver.position is already skipping some of the white space at the beginning of the line,
            // so that's why segmentStart might be different than resolver.position().
            LineSegmentRange range(resolver.position(), end);
            segmentRanges.append(range);
            resolver.setPosition(end, numberOfIsolateAncestors(end));

            if (lineInfo.previousLineBrokeCleanly()) {
                // If we hit a new line break, just stop adding anything to this line.
                break;
            }
        }
    }
    resolver.setPositionIgnoringNestedIsolates(oldEnd);
    return end;
#endif
}

static inline bool iteratorIsBeyondEndOfRenderCombineText(const InlineIterator& iter, RenderCombineText& renderer)
{
    return iter.m_obj == &renderer && iter.m_pos >= renderer.textLength();
}

#if ENABLE(CSS_SHAPES)
static void updateSegmentsForShapes(RenderBlockFlow& block, const FloatingObject* lastFloatFromPreviousLine, const WordMeasurements& wordMeasurements, LineWidth& width, bool isFirstLine)
{
    ASSERT(lastFloatFromPreviousLine);

    ShapeInsideInfo* shapeInsideInfo = block.layoutShapeInsideInfo();
    if (!lastFloatFromPreviousLine->isPlaced() || !shapeInsideInfo)
        return;

    bool isHorizontalWritingMode = block.isHorizontalWritingMode();
    LayoutUnit logicalOffsetFromShapeContainer = block.logicalOffsetFromShapeAncestorContainer(shapeInsideInfo->owner()).height();

    LayoutUnit lineLogicalTop = block.logicalHeight() + logicalOffsetFromShapeContainer;
    LayoutUnit lineLogicalHeight = block.lineHeight(isFirstLine, isHorizontalWritingMode ? HorizontalLine : VerticalLine, PositionOfInteriorLineBoxes);
    LayoutUnit lineLogicalBottom = lineLogicalTop + lineLogicalHeight;

    LayoutUnit floatLogicalTop = block.logicalTopForFloat(lastFloatFromPreviousLine);
    LayoutUnit floatLogicalBottom = block.logicalBottomForFloat(lastFloatFromPreviousLine);

    bool lineOverlapsWithFloat = (floatLogicalTop < lineLogicalBottom) && (lineLogicalTop < floatLogicalBottom);
    if (!lineOverlapsWithFloat)
        return;

    float minSegmentWidth = firstPositiveWidth(wordMeasurements);

    LayoutUnit floatLogicalWidth = block.logicalWidthForFloat(lastFloatFromPreviousLine);
    LayoutUnit availableLogicalWidth = block.logicalWidth() - block.logicalRightForFloat(lastFloatFromPreviousLine);
    if (availableLogicalWidth < minSegmentWidth)
        block.setLogicalHeight(floatLogicalBottom);

    if (block.logicalHeight() < floatLogicalTop) {
        shapeInsideInfo->adjustLogicalLineTop(minSegmentWidth + floatLogicalWidth);
        block.setLogicalHeight(shapeInsideInfo->logicalLineTop() - logicalOffsetFromShapeContainer);
    }

    lineLogicalTop = block.logicalHeight() + logicalOffsetFromShapeContainer;

    shapeInsideInfo->updateSegmentsForLine(lineLogicalTop, lineLogicalHeight);
    width.updateCurrentShapeSegment();
    width.updateAvailableWidth();
}
#endif

class BreakingContext {
public:
    BreakingContext(LineBreaker& lineBreaker, InlineBidiResolver& resolver, LineInfo& inLineInfo, LineWidth& lineWidth, RenderTextInfo& inRenderTextInfo, FloatingObject* inLastFloatFromPreviousLine, bool appliedStartWidth, RenderBlockFlow& block)
        : m_lineBreaker(lineBreaker)
        , m_resolver(resolver)
        , m_current(resolver.position())
        , m_lineBreak(resolver.position())
        , m_block(block)
        , m_lastObject(m_current.m_obj)
        , m_nextObject(0)
        , m_currentStyle(0)
        , m_blockStyle(block.style())
        , m_lineInfo(inLineInfo)
        , m_renderTextInfo(inRenderTextInfo)
        , m_lastFloatFromPreviousLine(inLastFloatFromPreviousLine)
        , m_width(lineWidth)
        , m_currWS(NORMAL)
        , m_lastWS(NORMAL)
        , m_preservesNewline(false)
        , m_atStart(true)
        , m_ignoringSpaces(false)
        , m_currentCharacterIsSpace(false)
        , m_currentCharacterIsWS(false)
        , m_appliedStartWidth(appliedStartWidth)
        , m_includeEndWidth(true)
        , m_autoWrap(false)
        , m_autoWrapWasEverTrueOnLine(false)
        , m_floatsFitOnLine(true)
        , m_collapseWhiteSpace(false)
        , m_startingNewParagraph(m_lineInfo.previousLineBrokeCleanly())
        , m_allowImagesToBreak(!block.document().inQuirksMode() || !block.isTableCell() || !m_blockStyle.logicalWidth().isIntrinsicOrAuto())
        , m_atEnd(false)
        , m_hadUncommittedWidthBeforeCurrent(false)
        , m_lineMidpointState(resolver.midpointState())
    {
        m_lineInfo.setPreviousLineBrokeCleanly(false);
    }

    RenderObject* currentObject() { return m_current.m_obj; }
    InlineIterator lineBreak() { return m_lineBreak; }
    InlineIterator& lineBreakRef() {return m_lineBreak; }
    LineWidth& lineWidth() { return m_width; }
    bool atEnd() { return m_atEnd; }

    void initializeForCurrentObject();

    void increment();

    void handleBR(EClear&);
    void handleOutOfFlowPositioned(Vector<RenderBox*>& positionedObjects);
    void handleFloat();
    void handleEmptyInline();
    void handleReplaced();
    bool handleText(WordMeasurements&, bool& hyphenated, unsigned& consecutiveHyphenatedLines);
    bool canBreakAtThisPosition();
    void commitAndUpdateLineBreakIfNeeded();
    InlineIterator handleEndOfLine();

    void clearLineBreakIfFitsOnLine(bool ignoringTrailingSpace = false)
    {
        if (m_width.fitsOnLine(ignoringTrailingSpace) || m_lastWS == NOWRAP)
            m_lineBreak.clear();
    }

    void commitLineBreakAtCurrentWidth(RenderObject* object, unsigned offset = 0, int nextBreak = -1)
    {
        m_width.commit();
        m_lineBreak.moveTo(object, offset, nextBreak);
    }

private:
    LineBreaker& m_lineBreaker;
    InlineBidiResolver& m_resolver;

    InlineIterator m_current;
    InlineIterator m_lineBreak;
    InlineIterator m_startOfIgnoredSpaces;

    RenderBlockFlow& m_block;
    RenderObject* m_lastObject;
    RenderObject* m_nextObject;

    RenderStyle* m_currentStyle;

    // Firefox and Opera will allow a table cell to grow to fit an image inside it under
    // very specific circumstances (in order to match common WinIE renderings).
    // Not supporting the quirk has caused us to mis-render some real sites. (See Bugzilla 10517.)
    RenderStyle& m_blockStyle;

    LineInfo& m_lineInfo;

    RenderTextInfo& m_renderTextInfo;

    FloatingObject* m_lastFloatFromPreviousLine;

    LineWidth m_width;

    EWhiteSpace m_currWS;
    EWhiteSpace m_lastWS;

    bool m_preservesNewline;
    bool m_atStart;

    // This variable is used only if whitespace isn't set to PRE, and it tells us whether
    // or not we are currently ignoring whitespace.
    bool m_ignoringSpaces;

    // This variable tracks whether the very last character we saw was a space.  We use
    // this to detect when we encounter a second space so we know we have to terminate
    // a run.
    bool m_currentCharacterIsSpace;
    bool m_currentCharacterIsWS;
    bool m_appliedStartWidth;
    bool m_includeEndWidth;
    bool m_autoWrap;
    bool m_autoWrapWasEverTrueOnLine;
    bool m_floatsFitOnLine;
    bool m_collapseWhiteSpace;
    bool m_startingNewParagraph;
    bool m_allowImagesToBreak;
    bool m_atEnd;
    bool m_hadUncommittedWidthBeforeCurrent;

    LineMidpointState& m_lineMidpointState;

    TrailingObjects m_trailingObjects;
};

inline void BreakingContext::initializeForCurrentObject()
{
    m_hadUncommittedWidthBeforeCurrent = !!m_width.uncommittedWidth();

    m_currentStyle = &m_current.m_obj->style();

    ASSERT(m_currentStyle);

    m_nextObject = bidiNextSkippingEmptyInlines(m_block, m_current.m_obj);
    if (m_nextObject && m_nextObject->parent() && !m_nextObject->parent()->isDescendantOf(m_current.m_obj->parent()))
        m_includeEndWidth = true;

    m_currWS = m_current.m_obj->isReplaced() ? m_current.m_obj->parent()->style().whiteSpace() : m_currentStyle->whiteSpace();
    m_lastWS = m_lastObject->isReplaced() ? m_lastObject->parent()->style().whiteSpace() : m_lastObject->style().whiteSpace();

    m_autoWrap = RenderStyle::autoWrap(m_currWS);
    m_autoWrapWasEverTrueOnLine = m_autoWrapWasEverTrueOnLine || m_autoWrap;

#if ENABLE(SVG)
    m_preservesNewline = m_current.m_obj->isSVGInlineText() ? false : RenderStyle::preserveNewline(m_currWS);
#else
    m_preservesNewline = RenderStyle::preserveNewline(m_currWS);
#endif

    m_collapseWhiteSpace = RenderStyle::collapseWhiteSpace(m_currWS);
}

inline void BreakingContext::increment()
{
    // Clear out our character space bool, since inline <pre>s don't collapse whitespace
    // with adjacent inline normal/nowrap spans.
    if (!m_collapseWhiteSpace)
        m_currentCharacterIsSpace = false;

    m_current.moveToStartOf(m_nextObject);
    m_atStart = false;
}

inline void BreakingContext::handleBR(EClear& clear)
{
    if (m_width.fitsOnLine()) {
        RenderObject* br = m_current.m_obj;
        m_lineBreak.moveToStartOf(br);
        m_lineBreak.increment();

        // A <br> always breaks a line, so don't let the line be collapsed
        // away. Also, the space at the end of a line with a <br> does not
        // get collapsed away. It only does this if the previous line broke
        // cleanly. Otherwise the <br> has no effect on whether the line is
        // empty or not.
        if (m_startingNewParagraph)
            m_lineInfo.setEmpty(false, &m_block, &m_width);
        m_trailingObjects.clear();
        m_lineInfo.setPreviousLineBrokeCleanly(true);

        // A <br> with clearance always needs a linebox in case the lines below it get dirtied later and
        // need to check for floats to clear - so if we're ignoring spaces, stop ignoring them and add a
        // run for this object.
        if (m_ignoringSpaces && m_currentStyle->clear() != CNONE)
            ensureLineBoxInsideIgnoredSpaces(m_lineMidpointState, br);
        // If we were preceded by collapsing space and are in a right-aligned container we need to ensure the space gets
        // collapsed away so that it doesn't push the text out from the container's right-hand edge.
        // FIXME: Do this regardless of the container's alignment - will require rebaselining a lot of test results.
        else if (m_ignoringSpaces && (m_blockStyle.textAlign() == RIGHT || m_blockStyle.textAlign() == WEBKIT_RIGHT))
            stopIgnoringSpaces(m_lineMidpointState, InlineIterator(0, m_current.m_obj, m_current.m_pos));

        if (!m_lineInfo.isEmpty())
            clear = m_currentStyle->clear();
    }
    m_atEnd = true;
}

inline void BreakingContext::handleOutOfFlowPositioned(Vector<RenderBox*>& positionedObjects)
{
    // If our original display wasn't an inline type, then we can
    // go ahead and determine our static inline position now.
    RenderBox* box = toRenderBox(m_current.m_obj);
    bool isInlineType = box->style().isOriginalDisplayInlineType();
    if (!isInlineType)
        m_block.setStaticInlinePositionForChild(*box, m_block.logicalHeight(), m_block.startOffsetForContent(m_block.logicalHeight()));
    else {
        // If our original display was an INLINE type, then we can go ahead
        // and determine our static y position now.
        box->layer()->setStaticBlockPosition(m_block.logicalHeight());
    }

    // If we're ignoring spaces, we have to stop and include this object and
    // then start ignoring spaces again.
    if (isInlineType || box->container()->isRenderInline()) {
        if (m_ignoringSpaces)
            ensureLineBoxInsideIgnoredSpaces(m_lineMidpointState, box);
        m_trailingObjects.appendBoxIfNeeded(box);
    } else
        positionedObjects.append(box);

    m_width.addUncommittedWidth(inlineLogicalWidth(box));
    // Reset prior line break context characters.
    m_renderTextInfo.m_lineBreakIterator.resetPriorContext();
}

inline void BreakingContext::handleFloat()
{
    RenderBox& floatBox = toRenderBox(*m_current.m_obj);
    FloatingObject* floatingObject = m_block.insertFloatingObject(floatBox);
    // check if it fits in the current line.
    // If it does, position it now, otherwise, position
    // it after moving to next line (in newLine() func)
    // FIXME: Bug 110372: Properly position multiple stacked floats with non-rectangular shape outside.
    if (m_floatsFitOnLine && m_width.fitsOnLineExcludingTrailingWhitespace(m_block.logicalWidthForFloat(floatingObject))) {
        m_block.positionNewFloatOnLine(floatingObject, m_lastFloatFromPreviousLine, m_lineInfo, m_width);
        if (m_lineBreak.m_obj == m_current.m_obj) {
            ASSERT(!m_lineBreak.m_pos);
            m_lineBreak.increment();
        }
    } else
        m_floatsFitOnLine = false;
    // Update prior line break context characters, using U+FFFD (OBJECT REPLACEMENT CHARACTER) for floating element.
    m_renderTextInfo.m_lineBreakIterator.updatePriorContext(replacementCharacter);
}

inline void BreakingContext::handleEmptyInline()
{
    RenderInline& flowBox = toRenderInline(*m_current.m_obj);

    // This should only end up being called on empty inlines
    ASSERT(isEmptyInline(flowBox));

    // Now that some inline flows have line boxes, if we are already ignoring spaces, we need
    // to make sure that we stop to include this object and then start ignoring spaces again.
    // If this object is at the start of the line, we need to behave like list markers and
    // start ignoring spaces.
    bool requiresLineBox = alwaysRequiresLineBox(flowBox);
    if (requiresLineBox || requiresLineBoxForContent(flowBox, m_lineInfo)) {
        // An empty inline that only has line-height, vertical-align or font-metrics will only get a
        // line box to affect the height of the line if the rest of the line is not empty.
        if (requiresLineBox)
            m_lineInfo.setEmpty(false, &m_block, &m_width);
        if (m_ignoringSpaces) {
            m_trailingObjects.clear();
            ensureLineBoxInsideIgnoredSpaces(m_lineMidpointState, m_current.m_obj);
        } else if (m_blockStyle.collapseWhiteSpace() && m_resolver.position().m_obj == m_current.m_obj
            && shouldSkipWhitespaceAfterStartObject(m_block, m_current.m_obj, m_lineMidpointState)) {
            // Like with list markers, we start ignoring spaces to make sure that any
            // additional spaces we see will be discarded.
            m_currentCharacterIsSpace = true;
            m_currentCharacterIsWS = true;
            m_ignoringSpaces = true;
        } else
            m_trailingObjects.appendBoxIfNeeded(&flowBox);
    }

    m_width.addUncommittedWidth(inlineLogicalWidth(m_current.m_obj) + borderPaddingMarginStart(flowBox) + borderPaddingMarginEnd(flowBox));
}

inline void BreakingContext::handleReplaced()
{
    RenderBox& replacedBox = toRenderBox(*m_current.m_obj);

    if (m_atStart)
        m_width.updateAvailableWidth(replacedBox.logicalHeight());

    // Break on replaced elements if either has normal white-space.
    if ((m_autoWrap || RenderStyle::autoWrap(m_lastWS)) && (!m_current.m_obj->isImage() || m_allowImagesToBreak)) {
        m_width.commit();
        m_lineBreak.moveToStartOf(m_current.m_obj);
    }

    if (m_ignoringSpaces)
        stopIgnoringSpaces(m_lineMidpointState, InlineIterator(0, m_current.m_obj, 0));

    m_lineInfo.setEmpty(false, &m_block, &m_width);
    m_ignoringSpaces = false;
    m_currentCharacterIsSpace = false;
    m_currentCharacterIsWS = false;
    m_trailingObjects.clear();

    // Optimize for a common case. If we can't find whitespace after the list
    // item, then this is all moot.
    LayoutUnit replacedLogicalWidth = m_block.logicalWidthForChild(replacedBox) + m_block.marginStartForChild(replacedBox) + m_block.marginEndForChild(replacedBox) + inlineLogicalWidth(m_current.m_obj);
    if (m_current.m_obj->isListMarker()) {
        if (m_blockStyle.collapseWhiteSpace() && shouldSkipWhitespaceAfterStartObject(m_block, m_current.m_obj, m_lineMidpointState)) {
            // Like with inline flows, we start ignoring spaces to make sure that any
            // additional spaces we see will be discarded.
            m_currentCharacterIsSpace = true;
            m_currentCharacterIsWS = false;
            m_ignoringSpaces = true;
        }
        if (toRenderListMarker(*m_current.m_obj).isInside())
            m_width.addUncommittedWidth(replacedLogicalWidth);
    } else
        m_width.addUncommittedWidth(replacedLogicalWidth);
    if (m_current.m_obj->isRubyRun())
        m_width.applyOverhang(toRenderRubyRun(m_current.m_obj), m_lastObject, m_nextObject);
    // Update prior line break context characters, using U+FFFD (OBJECT REPLACEMENT CHARACTER) for replaced element.
    m_renderTextInfo.m_lineBreakIterator.updatePriorContext(replacementCharacter);
}


static inline void nextCharacter(UChar& currentCharacter, UChar& lastCharacter, UChar& secondToLastCharacter)
{
    secondToLastCharacter = lastCharacter;
    lastCharacter = currentCharacter;
}

inline bool BreakingContext::handleText(WordMeasurements& wordMeasurements, bool& hyphenated,  unsigned& consecutiveHyphenatedLines)
{
    if (!m_current.m_pos)
        m_appliedStartWidth = false;

    RenderText* renderText = toRenderText(m_current.m_obj);

#if ENABLE(SVG)
    bool isSVGText = renderText->isSVGInlineText();
#endif

    // If we have left a no-wrap inline and entered an autowrap inline while ignoring spaces
    // then we need to mark the start of the autowrap inline as a potential linebreak now.
    if (m_autoWrap && !RenderStyle::autoWrap(m_lastWS) && m_ignoringSpaces)
        commitLineBreakAtCurrentWidth(m_current.m_obj);

    if (renderText->style().hasTextCombine() && m_current.m_obj->isCombineText() && !toRenderCombineText(*m_current.m_obj).isCombined()) {
        RenderCombineText& combineRenderer = toRenderCombineText(*m_current.m_obj);
        combineRenderer.combineText();
        // The length of the renderer's text may have changed. Increment stale iterator positions
        if (iteratorIsBeyondEndOfRenderCombineText(m_lineBreak, combineRenderer)) {
            ASSERT(iteratorIsBeyondEndOfRenderCombineText(m_resolver.position(), combineRenderer));
            m_lineBreak.increment();
            m_resolver.increment();
        }
    }

    const RenderStyle& style = lineStyle(*renderText->parent(), m_lineInfo);
    const Font& font = style.font();
    bool isFixedPitch = font.isFixedPitch();
    bool canHyphenate = style.hyphens() == HyphensAuto && WebCore::canHyphenate(style.locale());

    unsigned lastSpace = m_current.m_pos;
    float wordSpacing = m_currentStyle->wordSpacing();
    float lastSpaceWordSpacing = 0;
    float wordSpacingForWordMeasurement = 0;

    float wrapW = m_width.uncommittedWidth() + inlineLogicalWidth(m_current.m_obj, !m_appliedStartWidth, true);
    float charWidth = 0;
    bool breakNBSP = m_autoWrap && m_currentStyle->nbspMode() == SPACE;
    // Auto-wrapping text should wrap in the middle of a word only if it could not wrap before the word,
    // which is only possible if the word is the first thing on the line, that is, if |w| is zero.
    bool breakWords = m_currentStyle->breakWords() && ((m_autoWrap && !m_width.committedWidth()) || m_currWS == PRE);
    bool midWordBreak = false;
    bool breakAll = m_currentStyle->wordBreak() == BreakAllWordBreak && m_autoWrap;
    float hyphenWidth = 0;
#if ENABLE(SVG)
    if (isSVGText) {
        breakWords = false;
        breakAll = false;
    }
#endif

    if (m_renderTextInfo.m_text != renderText) {
        updateCounterIfNeeded(*renderText);
        m_renderTextInfo.m_text = renderText;
        m_renderTextInfo.m_font = &font;
        m_renderTextInfo.m_layout = font.createLayout(renderText, m_width.currentWidth(), m_collapseWhiteSpace);
        m_renderTextInfo.m_lineBreakIterator.resetStringAndReleaseIterator(renderText->text(), style.locale());
    } else if (m_renderTextInfo.m_layout && m_renderTextInfo.m_font != &font) {
        m_renderTextInfo.m_font = &font;
        m_renderTextInfo.m_layout = font.createLayout(renderText, m_width.currentWidth(), m_collapseWhiteSpace);
    }

    TextLayout* textLayout = m_renderTextInfo.m_layout.get();

    // Non-zero only when kerning is enabled and TextLayout isn't used, in which case we measure
    // words with their trailing space, then subtract its width.
    HashSet<const SimpleFontData*> fallbackFonts;
    float wordTrailingSpaceWidth = (font.typesettingFeatures() & Kerning) && !textLayout ? font.width(RenderBlock::constructTextRun(renderText, font, &space, 1, style), &fallbackFonts) + wordSpacing : 0;

    UChar lastCharacter = m_renderTextInfo.m_lineBreakIterator.lastCharacter();
    UChar secondToLastCharacter = m_renderTextInfo.m_lineBreakIterator.secondToLastCharacter();
    for (; m_current.m_pos < renderText->textLength(); m_current.fastIncrementInTextNode()) {
        bool previousCharacterIsSpace = m_currentCharacterIsSpace;
        bool previousCharacterIsWS = m_currentCharacterIsWS;
        UChar c = m_current.current();
        m_currentCharacterIsSpace = c == ' ' || c == '\t' || (!m_preservesNewline && (c == '\n'));

        if (!m_collapseWhiteSpace || !m_currentCharacterIsSpace)
            m_lineInfo.setEmpty(false, &m_block, &m_width);

        if (c == softHyphen && m_autoWrap && !hyphenWidth && style.hyphens() != HyphensNone) {
            hyphenWidth = measureHyphenWidth(renderText, font, &fallbackFonts);
            m_width.addUncommittedWidth(hyphenWidth);
        }

        bool applyWordSpacing = false;

        m_currentCharacterIsWS = m_currentCharacterIsSpace || (breakNBSP && c == noBreakSpace);

        if ((breakAll || breakWords) && !midWordBreak) {
            wrapW += charWidth;
            bool midWordBreakIsBeforeSurrogatePair = U16_IS_LEAD(c) && m_current.m_pos + 1 < renderText->textLength() && U16_IS_TRAIL((*renderText)[m_current.m_pos + 1]);
            charWidth = textWidth(renderText, m_current.m_pos, midWordBreakIsBeforeSurrogatePair ? 2 : 1, font, m_width.committedWidth() + wrapW, isFixedPitch, m_collapseWhiteSpace, fallbackFonts, textLayout);
            midWordBreak = m_width.committedWidth() + wrapW + charWidth > m_width.availableWidth();
        }

        bool betweenWords = c == '\n' || (m_currWS != PRE && !m_atStart && isBreakable(m_renderTextInfo.m_lineBreakIterator, m_current.m_pos, m_current.m_nextBreakablePosition, breakNBSP)
            && (style.hyphens() != HyphensNone || (m_current.previousInSameNode() != softHyphen)));

        if (betweenWords || midWordBreak) {
            bool stoppedIgnoringSpaces = false;
            if (m_ignoringSpaces) {
                lastSpaceWordSpacing = 0;
                if (!m_currentCharacterIsSpace) {
                    // Stop ignoring spaces and begin at this
                    // new point.
                    m_ignoringSpaces = false;
                    wordSpacingForWordMeasurement = 0;
                    lastSpace = m_current.m_pos; // e.g., "Foo    goo", don't add in any of the ignored spaces.
                    stopIgnoringSpaces(m_lineMidpointState, InlineIterator(0, m_current.m_obj, m_current.m_pos));
                    stoppedIgnoringSpaces = true;
                } else {
                    // Just keep ignoring these spaces.
                    nextCharacter(c, lastCharacter, secondToLastCharacter);
                    continue;
                }
            }

            wordMeasurements.grow(wordMeasurements.size() + 1);
            WordMeasurement& wordMeasurement = wordMeasurements.last();

            wordMeasurement.renderer = renderText;
            wordMeasurement.endOffset = m_current.m_pos;
            wordMeasurement.startOffset = lastSpace;

            float additionalTempWidth;
            if (wordTrailingSpaceWidth && c == ' ')
                additionalTempWidth = textWidth(renderText, lastSpace, m_current.m_pos + 1 - lastSpace, font, m_width.currentWidth(), isFixedPitch, m_collapseWhiteSpace, wordMeasurement.fallbackFonts, textLayout) - wordTrailingSpaceWidth;
            else
                additionalTempWidth = textWidth(renderText, lastSpace, m_current.m_pos - lastSpace, font, m_width.currentWidth(), isFixedPitch, m_collapseWhiteSpace, wordMeasurement.fallbackFonts, textLayout);

            if (wordMeasurement.fallbackFonts.isEmpty() && !fallbackFonts.isEmpty())
                wordMeasurement.fallbackFonts.swap(fallbackFonts);
            fallbackFonts.clear();

            wordMeasurement.width = additionalTempWidth + wordSpacingForWordMeasurement;
            additionalTempWidth += lastSpaceWordSpacing;
            m_width.addUncommittedWidth(additionalTempWidth);

            if (m_collapseWhiteSpace && previousCharacterIsSpace && m_currentCharacterIsSpace && additionalTempWidth)
                m_width.setTrailingWhitespaceWidth(additionalTempWidth);

            if (!m_appliedStartWidth) {
                m_width.addUncommittedWidth(inlineLogicalWidth(m_current.m_obj, true, false));
                m_appliedStartWidth = true;
            }

#if ENABLE(CSS_SHAPES)
            if (m_lastFloatFromPreviousLine)
                updateSegmentsForShapes(m_block, m_lastFloatFromPreviousLine, wordMeasurements, m_width, m_lineInfo.isFirstLine());
#endif
            applyWordSpacing = wordSpacing && m_currentCharacterIsSpace;

            if (!m_width.committedWidth() && m_autoWrap && !m_width.fitsOnLine())
                m_width.fitBelowFloats();

            if (m_autoWrap || breakWords) {
                // If we break only after white-space, consider the current character
                // as candidate width for this line.
                bool lineWasTooWide = false;
                if (m_width.fitsOnLine() && m_currentCharacterIsSpace && m_currentStyle->breakOnlyAfterWhiteSpace() && !midWordBreak) {
                    float charWidth = textWidth(renderText, m_current.m_pos, 1, font, m_width.currentWidth(), isFixedPitch, m_collapseWhiteSpace, wordMeasurement.fallbackFonts, textLayout) + (applyWordSpacing ? wordSpacing : 0);
                    // Check if line is too big even without the extra space
                    // at the end of the line. If it is not, do nothing.
                    // If the line needs the extra whitespace to be too long,
                    // then move the line break to the space and skip all
                    // additional whitespace.
                    if (!m_width.fitsOnLineIncludingExtraWidth(charWidth)) {
                        lineWasTooWide = true;
                        m_lineBreak.moveTo(m_current.m_obj, m_current.m_pos, m_current.m_nextBreakablePosition);
                        m_lineBreaker.skipTrailingWhitespace(m_lineBreak, m_lineInfo);
                    }
                }
                if (lineWasTooWide || !m_width.fitsOnLine()) {
                    if (canHyphenate && !m_width.fitsOnLine()) {
                        tryHyphenating(renderText, font, style.locale(), consecutiveHyphenatedLines, m_blockStyle.hyphenationLimitLines(), style.hyphenationLimitBefore(), style.hyphenationLimitAfter(), lastSpace, m_current.m_pos, m_width.currentWidth() - additionalTempWidth, m_width.availableWidth(), isFixedPitch, m_collapseWhiteSpace, lastSpaceWordSpacing, m_lineBreak, m_current.m_nextBreakablePosition, m_lineBreaker.m_hyphenated);
                        if (m_lineBreaker.m_hyphenated) {
                            m_atEnd = true;
                            return false;
                        }
                    }
                    if (m_lineBreak.atTextParagraphSeparator()) {
                        if (!stoppedIgnoringSpaces && m_current.m_pos > 0)
                            ensureCharacterGetsLineBox(m_lineMidpointState, m_current);
                        m_lineBreak.increment();
                        m_lineInfo.setPreviousLineBrokeCleanly(true);
                        wordMeasurement.endOffset = m_lineBreak.m_pos;
                    }
                    if (m_lineBreak.m_obj && m_lineBreak.m_pos && m_lineBreak.m_obj->isText() && toRenderText(m_lineBreak.m_obj)->textLength() && toRenderText(m_lineBreak.m_obj)->characterAt(m_lineBreak.m_pos - 1) == softHyphen && style.hyphens() != HyphensNone)
                        hyphenated = true;
                    if (m_lineBreak.m_pos && m_lineBreak.m_pos != (unsigned)wordMeasurement.endOffset && !wordMeasurement.width) {
                        if (charWidth) {
                            wordMeasurement.endOffset = m_lineBreak.m_pos;
                            wordMeasurement.width = charWidth;
                        }
                    }
                    // Didn't fit. Jump to the end unless there's still an opportunity to collapse whitespace.
                    if (m_ignoringSpaces || !m_collapseWhiteSpace || !m_currentCharacterIsSpace || !previousCharacterIsSpace) {
                        m_atEnd = true;
                        return false;
                    }
                } else {
                    if (!betweenWords || (midWordBreak && !m_autoWrap))
                        m_width.addUncommittedWidth(-additionalTempWidth);
                    if (hyphenWidth) {
                        // Subtract the width of the soft hyphen out since we fit on a line.
                        m_width.addUncommittedWidth(-hyphenWidth);
                        hyphenWidth = 0;
                    }
                }
            }

            if (c == '\n' && m_preservesNewline) {
                if (!stoppedIgnoringSpaces && m_current.m_pos > 0)
                    ensureCharacterGetsLineBox(m_lineMidpointState, m_current);
                m_lineBreak.moveTo(m_current.m_obj, m_current.m_pos, m_current.m_nextBreakablePosition);
                m_lineBreak.increment();
                m_lineInfo.setPreviousLineBrokeCleanly(true);
                return true;
            }

            if (m_autoWrap && betweenWords) {
                m_width.commit();
                wrapW = 0;
                m_lineBreak.moveTo(m_current.m_obj, m_current.m_pos, m_current.m_nextBreakablePosition);
                // Auto-wrapping text should not wrap in the middle of a word once it has had an
                // opportunity to break after a word.
                breakWords = false;
            }

            if (midWordBreak && !U16_IS_TRAIL(c) && !(U_GET_GC_MASK(c) & U_GC_M_MASK)) {
                // Remember this as a breakable position in case
                // adding the end width forces a break.
                m_lineBreak.moveTo(m_current.m_obj, m_current.m_pos, m_current.m_nextBreakablePosition);
                midWordBreak &= (breakWords || breakAll);
            }

            if (betweenWords) {
                lastSpaceWordSpacing = applyWordSpacing ? wordSpacing : 0;
                wordSpacingForWordMeasurement = (applyWordSpacing && wordMeasurement.width) ? wordSpacing : 0;
                lastSpace = m_current.m_pos;
            }

            if (!m_ignoringSpaces && m_currentStyle->collapseWhiteSpace()) {
                // If we encounter a newline, or if we encounter a
                // second space, we need to go ahead and break up this
                // run and enter a mode where we start collapsing spaces.
                if (m_currentCharacterIsSpace && previousCharacterIsSpace) {
                    m_ignoringSpaces = true;

                    // We just entered a mode where we are ignoring
                    // spaces. Create a midpoint to terminate the run
                    // before the second space.
                    startIgnoringSpaces(m_lineMidpointState, m_startOfIgnoredSpaces);
                    m_trailingObjects.updateMidpointsForTrailingBoxes(m_lineMidpointState, InlineIterator(), TrailingObjects::DoNotCollapseFirstSpace);
                }
            }
        } else if (m_ignoringSpaces) {
            // Stop ignoring spaces and begin at this
            // new point.
            m_ignoringSpaces = false;
            lastSpaceWordSpacing = applyWordSpacing ? wordSpacing : 0;
            wordSpacingForWordMeasurement = (applyWordSpacing && wordMeasurements.last().width) ? wordSpacing : 0;
            lastSpace = m_current.m_pos; // e.g., "Foo    goo", don't add in any of the ignored spaces.
            stopIgnoringSpaces(m_lineMidpointState, InlineIterator(0, m_current.m_obj, m_current.m_pos));
        }
#if ENABLE(SVG)
        if (isSVGText && m_current.m_pos > 0) {
            // Force creation of new InlineBoxes for each absolute positioned character (those that start new text chunks).
            if (toRenderSVGInlineText(renderText)->characterStartsNewTextChunk(m_current.m_pos))
                ensureCharacterGetsLineBox(m_lineMidpointState, m_current);
        }
#endif

        if (m_currentCharacterIsSpace && !previousCharacterIsSpace) {
            m_startOfIgnoredSpaces.m_obj = m_current.m_obj;
            m_startOfIgnoredSpaces.m_pos = m_current.m_pos;
            // Spaces after right-aligned text and before a line-break get collapsed away completely so that the trailing
            // space doesn't seem to push the text out from the right-hand edge.
            // FIXME: Do this regardless of the container's alignment - will require rebaselining a lot of test results.
            if (m_nextObject && m_nextObject->isBR() && (m_blockStyle.textAlign() == RIGHT || m_blockStyle.textAlign() == WEBKIT_RIGHT)) {
                m_startOfIgnoredSpaces.m_pos--;
                // If there's just a single trailing space start ignoring it now so it collapses away.
                if (m_current.m_pos == renderText->textLength() - 1)
                    startIgnoringSpaces(m_lineMidpointState, m_startOfIgnoredSpaces);
            }
        }

        if (!m_currentCharacterIsSpace && previousCharacterIsWS) {
            if (m_autoWrap && m_currentStyle->breakOnlyAfterWhiteSpace())
                m_lineBreak.moveTo(m_current.m_obj, m_current.m_pos, m_current.m_nextBreakablePosition);
        }

        if (m_collapseWhiteSpace && m_currentCharacterIsSpace && !m_ignoringSpaces)
            m_trailingObjects.setTrailingWhitespace(toRenderText(m_current.m_obj));
        else if (!m_currentStyle->collapseWhiteSpace() || !m_currentCharacterIsSpace)
            m_trailingObjects.clear();

        m_atStart = false;
        nextCharacter(c, lastCharacter, secondToLastCharacter);
    }

    m_renderTextInfo.m_lineBreakIterator.setPriorContext(lastCharacter, secondToLastCharacter);

    wordMeasurements.grow(wordMeasurements.size() + 1);
    WordMeasurement& wordMeasurement = wordMeasurements.last();
    wordMeasurement.renderer = renderText;

    // IMPORTANT: current.m_pos is > length here!
    float additionalTempWidth = m_ignoringSpaces ? 0 : textWidth(renderText, lastSpace, m_current.m_pos - lastSpace, font, m_width.currentWidth(), isFixedPitch, m_collapseWhiteSpace, wordMeasurement.fallbackFonts, textLayout);
    wordMeasurement.startOffset = lastSpace;
    wordMeasurement.endOffset = m_current.m_pos;
    wordMeasurement.width = m_ignoringSpaces ? 0 : additionalTempWidth + wordSpacingForWordMeasurement;
    additionalTempWidth += lastSpaceWordSpacing;

    float inlineLogicalTempWidth = inlineLogicalWidth(m_current.m_obj, !m_appliedStartWidth, m_includeEndWidth);
    m_width.addUncommittedWidth(additionalTempWidth + inlineLogicalTempWidth);

    if (wordMeasurement.fallbackFonts.isEmpty() && !fallbackFonts.isEmpty())
        wordMeasurement.fallbackFonts.swap(fallbackFonts);
    fallbackFonts.clear();

    if (m_collapseWhiteSpace && m_currentCharacterIsSpace && additionalTempWidth)
        m_width.setTrailingWhitespaceWidth(additionalTempWidth, inlineLogicalTempWidth);

    m_includeEndWidth = false;

    if (!m_width.fitsOnLine()) {
        if (canHyphenate)
            tryHyphenating(renderText, font, style.locale(), consecutiveHyphenatedLines, m_blockStyle.hyphenationLimitLines(), style.hyphenationLimitBefore(), style.hyphenationLimitAfter(), lastSpace, m_current.m_pos, m_width.currentWidth() - additionalTempWidth, m_width.availableWidth(), isFixedPitch, m_collapseWhiteSpace, lastSpaceWordSpacing, m_lineBreak, m_current.m_nextBreakablePosition, m_lineBreaker.m_hyphenated);

        if (!hyphenated && m_lineBreak.previousInSameNode() == softHyphen && style.hyphens() != HyphensNone) {
            hyphenated = true;
            m_atEnd = true;
        }
    }
    return false;
}

static bool textBeginsWithBreakablePosition(RenderObject* next)
{
    ASSERT(next->isText());
    RenderText* nextText = toRenderText(next);
    if (!nextText->textLength())
        return false;
    UChar c = nextText->characterAt(0);
    return c == ' ' || c == '\t' || (c == '\n' && !nextText->preservesNewline());
}

inline bool BreakingContext::canBreakAtThisPosition()
{
    // If we are no-wrap and have found a line-breaking opportunity already then we should take it.
    if (m_width.committedWidth() && !m_width.fitsOnLine(m_currentCharacterIsSpace) && m_currWS == NOWRAP)
        return true;

    // Avoid breaking before empty inlines.
    if (m_nextObject && m_nextObject->isRenderInline() && isEmptyInline(toRenderInline(*m_nextObject)))
        return false;

    // Return early if we autowrap and the current character is a space as we will always want to break at such a position.
    if (m_autoWrap && m_currentCharacterIsSpace)
        return true;

    if (m_nextObject && m_nextObject->isLineBreakOpportunity())
        return m_autoWrap;

    bool nextIsAutoWrappingText = (m_nextObject && m_nextObject->isText() && (m_autoWrap || m_nextObject->style().autoWrap()));
    if (!nextIsAutoWrappingText)
        return m_autoWrap;
    bool currentIsTextOrEmptyInline = m_current.m_obj->isText() || (m_current.m_obj->isRenderInline() && isEmptyInline(toRenderInline(*m_current.m_obj)));
    if (!currentIsTextOrEmptyInline)
        return m_autoWrap;

    bool canBreakHere = !m_currentCharacterIsSpace && textBeginsWithBreakablePosition(m_nextObject);

    // See if attempting to fit below floats creates more available width on the line.
    if (!m_width.fitsOnLine() && !m_width.committedWidth())
        m_width.fitBelowFloats();

    bool canPlaceOnLine = m_width.fitsOnLine() || !m_autoWrapWasEverTrueOnLine;

    if (canPlaceOnLine && canBreakHere)
        commitLineBreakAtCurrentWidth(m_nextObject);

    return canBreakHere;
}

inline void BreakingContext::commitAndUpdateLineBreakIfNeeded()
{
    bool checkForBreak = canBreakAtThisPosition();

    if (checkForBreak && !m_width.fitsOnLine(m_ignoringSpaces)) {
        // if we have floats, try to get below them.
        if (m_currentCharacterIsSpace && !m_ignoringSpaces && m_currentStyle->collapseWhiteSpace())
            m_trailingObjects.clear();

        if (m_width.committedWidth()) {
            m_atEnd = true;
            return;
        }

        m_width.fitBelowFloats();

        // |width| may have been adjusted because we got shoved down past a float (thus
        // giving us more room), so we need to retest, and only jump to
        // the end label if we still don't fit on the line. -dwh
        if (!m_width.fitsOnLine(m_ignoringSpaces)) {
            m_atEnd = true;
            return;
        }
    } else if (m_blockStyle.autoWrap() && !m_width.fitsOnLine() && !m_width.committedWidth()) {
        // If the container autowraps but the current child does not then we still need to ensure that it
        // wraps and moves below any floats.
        m_width.fitBelowFloats();
    }

    if (!m_current.m_obj->isFloatingOrOutOfFlowPositioned()) {
        m_lastObject = m_current.m_obj;
        if (m_lastObject->isReplaced() && m_autoWrap && (!m_lastObject->isImage() || m_allowImagesToBreak) && (!m_lastObject->isListMarker() || toRenderListMarker(*m_lastObject).isInside())) {
            m_width.commit();
            m_lineBreak.moveToStartOf(m_nextObject);
        }
    }
}

InlineIterator BreakingContext::handleEndOfLine()
{
#if ENABLE(CSS_SHAPES)
    ShapeInsideInfo* shapeInfo = m_block.layoutShapeInsideInfo();
    bool segmentAllowsOverflow = !shapeInfo || !shapeInfo->hasSegments();
#else
    bool segmentAllowsOverflow = true;
#endif
    if (segmentAllowsOverflow) {
        if (m_lineBreak == m_resolver.position()) {
            if (!m_lineBreak.m_obj || !m_lineBreak.m_obj->isBR()) {
                // we just add as much as possible
                if (m_blockStyle.whiteSpace() == PRE && !m_current.m_pos) {
                    m_lineBreak.moveTo(m_lastObject, m_lastObject->isText() ? m_lastObject->length() : 0);
                } else if (m_lineBreak.m_obj) {
                    // Don't ever break in the middle of a word if we can help it.
                    // There's no room at all. We just have to be on this line,
                    // even though we'll spill out.
                    m_lineBreak.moveTo(m_current.m_obj, m_current.m_pos);
                }
            }
            // make sure we consume at least one char/object.
            if (m_lineBreak == m_resolver.position())
                m_lineBreak.increment();
        } else if (!m_current.m_pos && !m_width.committedWidth() && m_width.uncommittedWidth() && !m_hadUncommittedWidthBeforeCurrent) {
            // Do not push the current object to the next line, when this line has some content, but it is still considered empty.
            // Empty inline elements like <span></span> can produce such lines and now we just ignore these break opportunities
            // at the start of a line, if no width has been committed yet.
            // Behave as if it was actually empty and consume at least one object.
            m_lineBreak.increment();
        }
    }

    // Sanity check our midpoints.
    checkMidpoints(m_lineMidpointState, m_lineBreak);

    m_trailingObjects.updateMidpointsForTrailingBoxes(m_lineMidpointState, m_lineBreak, TrailingObjects::CollapseFirstSpace);

    // We might have made lineBreak an iterator that points past the end
    // of the object. Do this adjustment to make it point to the start
    // of the next object instead to avoid confusing the rest of the
    // code.
    if (m_lineBreak.m_pos > 0) {
        m_lineBreak.m_pos--;
        m_lineBreak.increment();
    }

    return m_lineBreak;
}

InlineIterator LineBreaker::nextSegmentBreak(InlineBidiResolver& resolver, LineInfo& lineInfo, RenderTextInfo& renderTextInfo, FloatingObject* lastFloatFromPreviousLine, unsigned consecutiveHyphenatedLines, WordMeasurements& wordMeasurements)
{
    reset();

    ASSERT(resolver.position().root() == &m_block);

    bool appliedStartWidth = resolver.position().m_pos > 0;

    LineWidth width(m_block, lineInfo.isFirstLine(), requiresIndent(lineInfo.isFirstLine(), lineInfo.previousLineBrokeCleanly(), m_block.style()));

    skipLeadingWhitespace(resolver, lineInfo, lastFloatFromPreviousLine, width);

    if (resolver.position().atEnd())
        return resolver.position();

    BreakingContext context(*this, resolver, lineInfo, width, renderTextInfo, lastFloatFromPreviousLine, appliedStartWidth, m_block);

    while (context.currentObject()) {
        context.initializeForCurrentObject();
        if (context.currentObject()->isBR()) {
            context.handleBR(m_clear);
        } else if (context.currentObject()->isOutOfFlowPositioned()) {
            context.handleOutOfFlowPositioned(m_positionedObjects);
        } else if (context.currentObject()->isFloating()) {
            context.handleFloat();
        } else if (context.currentObject()->isRenderInline()) {
            context.handleEmptyInline();
        } else if (context.currentObject()->isReplaced()) {
            context.handleReplaced();
        } else if (context.currentObject()->isText()) {
            if (context.handleText(wordMeasurements, m_hyphenated, consecutiveHyphenatedLines)) {
                // We've hit a hard text line break. Our line break iterator is updated, so go ahead and early return.
                return context.lineBreak();
            }
        } else if (context.currentObject()->isLineBreakOpportunity())
            context.commitLineBreakAtCurrentWidth(context.currentObject());
        else
            ASSERT_NOT_REACHED();

        if (context.atEnd())
            return context.handleEndOfLine();

        context.commitAndUpdateLineBreakIfNeeded();

        if (context.atEnd())
            return context.handleEndOfLine();

        context.increment();
    }

    context.clearLineBreakIfFitsOnLine(true);

    return context.handleEndOfLine();
}

void RenderBlockFlow::addOverflowFromInlineChildren()
{
    if (auto layout = simpleLineLayout()) {
        ASSERT(!hasOverflowClip());
        SimpleLineLayout::collectFlowOverflow(*this, *layout);
        return;
    }
    LayoutUnit endPadding = hasOverflowClip() ? paddingEnd() : LayoutUnit();
    // FIXME: Need to find another way to do this, since scrollbars could show when we don't want them to.
    if (hasOverflowClip() && !endPadding && element() && element()->isRootEditableElement() && style().isLeftToRightDirection())
        endPadding = 1;
    for (RootInlineBox* curr = firstRootBox(); curr; curr = curr->nextRootBox()) {
        addLayoutOverflow(curr->paddedLayoutOverflowRect(endPadding));
        RenderRegion* region = curr->containingRegion();
        if (region)
            region->addLayoutOverflowForBox(this, curr->paddedLayoutOverflowRect(endPadding));
        if (!hasOverflowClip()) {
            addVisualOverflow(curr->visualOverflowRect(curr->lineTop(), curr->lineBottom()));
            if (region)
                region->addVisualOverflowForBox(this, curr->visualOverflowRect(curr->lineTop(), curr->lineBottom()));
        }
    }
}

void RenderBlockFlow::deleteEllipsisLineBoxes()
{
    ETextAlign textAlign = style().textAlign();
    bool ltr = style().isLeftToRightDirection();
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

void RenderBlockFlow::checkLinesForTextOverflow()
{
    // Determine the width of the ellipsis using the current font.
    // FIXME: CSS3 says this is configurable, also need to use 0x002E (FULL STOP) if horizontal ellipsis is "not renderable"
    const Font& font = style().font();
    DEFINE_STATIC_LOCAL(AtomicString, ellipsisStr, (&horizontalEllipsis, 1));
    const Font& firstLineFont = firstLineStyle().font();
    int firstLineEllipsisWidth = firstLineFont.width(constructTextRun(this, firstLineFont, &horizontalEllipsis, 1, firstLineStyle()));
    int ellipsisWidth = (font == firstLineFont) ? firstLineEllipsisWidth : font.width(constructTextRun(this, font, &horizontalEllipsis, 1, style()));

    // For LTR text truncation, we want to get the right edge of our padding box, and then we want to see
    // if the right edge of a line box exceeds that.  For RTL, we use the left edge of the padding box and
    // check the left edge of the line box to see if it is less
    // Include the scrollbar for overflow blocks, which means we want to use "contentWidth()"
    bool ltr = style().isLeftToRightDirection();
    ETextAlign textAlign = style().textAlign();
    bool firstLine = true;
    for (RootInlineBox* curr = firstRootBox(); curr; curr = curr->nextRootBox()) {
        // FIXME: Use pixelSnappedLogicalRightOffsetForLine instead of snapping it ourselves once the column workaround in said method has been fixed.
        // https://bugs.webkit.org/show_bug.cgi?id=105461
        int blockRightEdge = snapSizeToPixel(logicalRightOffsetForLine(curr->lineTop(), firstLine), curr->x());
        int blockLeftEdge = pixelSnappedLogicalLeftOffsetForLine(curr->lineTop(), firstLine);
        int lineBoxEdge = ltr ? snapSizeToPixel(curr->x() + curr->logicalWidth(), curr->x()) : snapSizeToPixel(curr->x(), 0);
        if ((ltr && lineBoxEdge > blockRightEdge) || (!ltr && lineBoxEdge < blockLeftEdge)) {
            // This line spills out of our box in the appropriate direction.  Now we need to see if the line
            // can be truncated.  In order for truncation to be possible, the line must have sufficient space to
            // accommodate our truncation string, and no replaced elements (images, tables) can overlap the ellipsis
            // space.

            LayoutUnit width = firstLine ? firstLineEllipsisWidth : ellipsisWidth;
            LayoutUnit blockEdge = ltr ? blockRightEdge : blockLeftEdge;
            if (curr->lineCanAccommodateEllipsis(ltr, blockEdge, lineBoxEdge, width)) {
                float totalLogicalWidth = curr->placeEllipsis(ellipsisStr, ltr, blockLeftEdge, blockRightEdge, width);

                float logicalLeft = 0; // We are only interested in the delta from the base position.
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

bool RenderBlockFlow::positionNewFloatOnLine(FloatingObject* newFloat, FloatingObject* lastFloatFromPreviousLine, LineInfo& lineInfo, LineWidth& width)
{
    if (!positionNewFloats())
        return false;

    width.shrinkAvailableWidthForNewFloatIfNeeded(newFloat);

    // We only connect floats to lines for pagination purposes if the floats occur at the start of
    // the line and the previous line had a hard break (so this line is either the first in the block
    // or follows a <br>).
    if (!newFloat->paginationStrut() || !lineInfo.previousLineBrokeCleanly() || !lineInfo.isEmpty())
        return true;

    const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
    ASSERT(floatingObjectSet.last().get() == newFloat);

    LayoutUnit floatLogicalTop = logicalTopForFloat(newFloat);
    int paginationStrut = newFloat->paginationStrut();

    if (floatLogicalTop - paginationStrut != logicalHeight() + lineInfo.floatPaginationStrut())
        return true;

    auto it = floatingObjectSet.end();
    --it; // Last float is newFloat, skip that one.
    auto begin = floatingObjectSet.begin();
    while (it != begin) {
        --it;
        FloatingObject* floatingObject = it->get();
        if (floatingObject == lastFloatFromPreviousLine)
            break;
        if (logicalTopForFloat(floatingObject) == logicalHeight() + lineInfo.floatPaginationStrut()) {
            floatingObject->setPaginationStrut(paginationStrut + floatingObject->paginationStrut());
            RenderBox& floatBox = floatingObject->renderer();
            setLogicalTopForChild(floatBox, logicalTopForChild(floatBox) + marginBeforeForChild(floatBox) + paginationStrut);
            if (floatBox.isRenderBlock())
                toRenderBlock(floatBox).setChildNeedsLayout(MarkOnlyThis);
            floatBox.layoutIfNeeded();
            // Save the old logical top before calling removePlacedObject which will set
            // isPlaced to false. Otherwise it will trigger an assert in logicalTopForFloat.
            LayoutUnit oldLogicalTop = logicalTopForFloat(floatingObject);
            m_floatingObjects->removePlacedObject(floatingObject);
            setLogicalTopForFloat(floatingObject, oldLogicalTop + paginationStrut);
            m_floatingObjects->addPlacedObject(floatingObject);
        }
    }

    // Just update the line info's pagination strut without altering our logical height yet. If the line ends up containing
    // no content, then we don't want to improperly grow the height of the block.
    lineInfo.setFloatPaginationStrut(lineInfo.floatPaginationStrut() + paginationStrut);
    return true;
}

LayoutUnit RenderBlock::startAlignedOffsetForLine(LayoutUnit position, bool firstLine)
{
    ETextAlign textAlign = style().textAlign();

    if (textAlign == TASTART) // FIXME: Handle TAEND here
        return startOffsetForLine(position, firstLine);

    // updateLogicalWidthForAlignment() handles the direction of the block so no need to consider it here
    float totalLogicalWidth = 0;
    float logicalLeft = logicalLeftOffsetForLine(logicalHeight(), false);
    float availableLogicalWidth = logicalRightOffsetForLine(logicalHeight(), false) - logicalLeft;
    updateLogicalWidthForAlignment(textAlign, 0, logicalLeft, totalLogicalWidth, availableLogicalWidth, 0);

    if (!style().isLeftToRightDirection())
        return logicalWidth() - logicalLeft;
    return logicalLeft;
}

void RenderBlockFlow::updateRegionForLine(RootInlineBox* lineBox) const
{
    ASSERT(lineBox);
    lineBox->setContainingRegion(regionAtBlockOffset(lineBox->lineTopWithLeading()));

    RootInlineBox* prevLineBox = lineBox->prevRootBox();
    if (!prevLineBox)
        return;

    // This check is more accurate than the one in |adjustLinePositionForPagination| because it takes into
    // account just the container changes between lines. The before mentioned function doesn't set the flag
    // correctly if the line is positioned at the top of the last fragment container.
    if (lineBox->containingRegion() != prevLineBox->containingRegion())
        lineBox->setIsFirstAfterPageBreak(true);
}

}
