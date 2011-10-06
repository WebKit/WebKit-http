/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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
#include "RenderInline.h"

#include "Chrome.h"
#include "FloatQuad.h"
#include "GraphicsContext.h"
#include "HitTestResult.h"
#include "InlineTextBox.h"
#include "Page.h"
#include "RenderArena.h"
#include "RenderBlock.h"
#include "RenderLayer.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "TransformState.h"
#include "VisiblePosition.h"

#if ENABLE(DASHBOARD_SUPPORT)
#include "Frame.h"
#endif

using namespace std;

namespace WebCore {

RenderInline::RenderInline(Node* node)
    : RenderBoxModelObject(node)
    , m_lineHeight(-1)
    , m_alwaysCreateLineBoxes(false)
{
    setChildrenInline(true);
}

void RenderInline::willBeDestroyed()
{
#ifndef NDEBUG
    // Make sure we do not retain "this" in the continuation outline table map of our containing blocks.
    if (parent() && style()->visibility() == VISIBLE && hasOutline()) {
        bool containingBlockPaintsContinuationOutline = continuation() || isInlineElementContinuation();
        if (containingBlockPaintsContinuationOutline) {
            if (RenderBlock* cb = containingBlock()) {
                if (RenderBlock* cbCb = cb->containingBlock())
                    ASSERT(!cbCb->paintsContinuationOutline(this));
            }
        }
    }
#endif

    // Make sure to destroy anonymous children first while they are still connected to the rest of the tree, so that they will
    // properly dirty line boxes that they are removed from.  Effects that do :before/:after only on hover could crash otherwise.
    children()->destroyLeftoverChildren();

    // Destroy our continuation before anything other than anonymous children.
    // The reason we don't destroy it before anonymous children is that they may
    // have continuations of their own that are anonymous children of our continuation.
    RenderBoxModelObject* continuation = this->continuation();
    if (continuation) {
        continuation->destroy();
        setContinuation(0);
    }
    
    if (!documentBeingDestroyed()) {
        if (firstLineBox()) {
            // We can't wait for RenderBoxModelObject::destroy to clear the selection,
            // because by then we will have nuked the line boxes.
            // FIXME: The FrameSelection should be responsible for this when it
            // is notified of DOM mutations.
            if (isSelectionBorder())
                view()->clearSelection();

            // If line boxes are contained inside a root, that means we're an inline.
            // In that case, we need to remove all the line boxes so that the parent
            // lines aren't pointing to deleted children. If the first line box does
            // not have a parent that means they are either already disconnected or
            // root lines that can just be destroyed without disconnecting.
            if (firstLineBox()->parent()) {
                for (InlineFlowBox* box = firstLineBox(); box; box = box->nextLineBox())
                    box->remove();
            }
        } else if (parent()) 
            parent()->dirtyLinesFromChangedChild(this);
    }

    m_lineBoxes.deleteLineBoxes(renderArena());

    RenderBoxModelObject::willBeDestroyed();
}

RenderInline* RenderInline::inlineElementContinuation() const
{
    RenderBoxModelObject* continuation = this->continuation();
    if (!continuation || continuation->isInline())
        return toRenderInline(continuation);
    return toRenderBlock(continuation)->inlineElementContinuation();
}

void RenderInline::updateBoxModelInfoFromStyle()
{
    RenderBoxModelObject::updateBoxModelInfoFromStyle();

    setInline(true); // Needed for run-ins, since run-in is considered a block display type.

    // FIXME: Support transforms and reflections on inline flows someday.
    setHasTransform(false);
    setHasReflection(false);    
}

void RenderInline::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBoxModelObject::styleDidChange(diff, oldStyle);

    // Ensure that all of the split inlines pick up the new style. We
    // only do this if we're an inline, since we don't want to propagate
    // a block's style to the other inlines.
    // e.g., <font>foo <h4>goo</h4> moo</font>.  The <font> inlines before
    // and after the block share the same style, but the block doesn't
    // need to pass its style on to anyone else.
    for (RenderInline* currCont = inlineElementContinuation(); currCont; currCont = currCont->inlineElementContinuation()) {
        RenderBoxModelObject* nextCont = currCont->continuation();
        currCont->setContinuation(0);
        currCont->setStyle(style());
        currCont->setContinuation(nextCont);
    }

    m_lineHeight = -1;

    if (!m_alwaysCreateLineBoxes) {
        bool alwaysCreateLineBoxes = hasSelfPaintingLayer() || hasBoxDecorations() || style()->hasPadding() || style()->hasMargin() || style()->hasOutline();
        if (oldStyle && alwaysCreateLineBoxes) {
            dirtyLineBoxes(false);
            setNeedsLayout(true);
        }
        m_alwaysCreateLineBoxes = alwaysCreateLineBoxes;
    }

    // Update pseudos for :before and :after now.
    if (!isAnonymous() && document()->usesBeforeAfterRules()) {
        children()->updateBeforeAfterContent(this, BEFORE);
        children()->updateBeforeAfterContent(this, AFTER);
    }
}

void RenderInline::updateAlwaysCreateLineBoxes(bool fullLayout)
{
    // Once we have been tainted once, just assume it will happen again. This way effects like hover highlighting that change the
    // background color will only cause a layout on the first rollover.
    if (m_alwaysCreateLineBoxes)
        return;

    RenderStyle* parentStyle = parent()->style();
    RenderInline* parentRenderInline = parent()->isRenderInline() ? toRenderInline(parent()) : 0;
    bool checkFonts = document()->inNoQuirksMode();
    bool alwaysCreateLineBoxes = (parentRenderInline && parentRenderInline->alwaysCreateLineBoxes())
        || (parentRenderInline && parentStyle->verticalAlign() != BASELINE)
        || style()->verticalAlign() != BASELINE
        || style()->textEmphasisMark() != TextEmphasisMarkNone
        || (checkFonts && (!parentStyle->font().fontMetrics().hasIdenticalAscentDescentAndLineGap(style()->font().fontMetrics())
        || parentStyle->lineHeight() != style()->lineHeight()));

    if (!alwaysCreateLineBoxes && checkFonts && document()->usesFirstLineRules()) {
        // Have to check the first line style as well.
        parentStyle = parent()->style(true);
        RenderStyle* childStyle = style(true);
        alwaysCreateLineBoxes = !parentStyle->font().fontMetrics().hasIdenticalAscentDescentAndLineGap(childStyle->font().fontMetrics())
        || childStyle->verticalAlign() != BASELINE
        || parentStyle->lineHeight() != childStyle->lineHeight();
    }

    if (alwaysCreateLineBoxes) {
        if (!fullLayout)
            dirtyLineBoxes(false);
        m_alwaysCreateLineBoxes = true;
    }
}

void RenderInline::addChild(RenderObject* newChild, RenderObject* beforeChild)
{
    if (continuation())
        return addChildToContinuation(newChild, beforeChild);
    return addChildIgnoringContinuation(newChild, beforeChild);
}

static RenderBoxModelObject* nextContinuation(RenderObject* renderer)
{
    if (renderer->isInline() && !renderer->isReplaced())
        return toRenderInline(renderer)->continuation();
    return toRenderBlock(renderer)->inlineElementContinuation();
}

RenderBoxModelObject* RenderInline::continuationBefore(RenderObject* beforeChild)
{
    if (beforeChild && beforeChild->parent() == this)
        return this;

    RenderBoxModelObject* curr = nextContinuation(this);
    RenderBoxModelObject* nextToLast = this;
    RenderBoxModelObject* last = this;
    while (curr) {
        if (beforeChild && beforeChild->parent() == curr) {
            if (curr->firstChild() == beforeChild)
                return last;
            return curr;
        }

        nextToLast = last;
        last = curr;
        curr = nextContinuation(curr);
    }

    if (!beforeChild && !last->firstChild())
        return nextToLast;
    return last;
}

void RenderInline::addChildIgnoringContinuation(RenderObject* newChild, RenderObject* beforeChild)
{
    // Make sure we don't append things after :after-generated content if we have it.
    if (!beforeChild && isAfterContent(lastChild()))
        beforeChild = lastChild();

    if (!newChild->isInline() && !newChild->isFloatingOrPositioned()) {
        // We are placing a block inside an inline. We have to perform a split of this
        // inline into continuations.  This involves creating an anonymous block box to hold
        // |newChild|.  We then make that block box a continuation of this inline.  We take all of
        // the children after |beforeChild| and put them in a clone of this object.
        RefPtr<RenderStyle> newStyle = RenderStyle::createAnonymousStyle(style());
        newStyle->setDisplay(BLOCK);

        RenderBlock* newBox = new (renderArena()) RenderBlock(document() /* anonymous box */);
        newBox->setStyle(newStyle.release());
        RenderBoxModelObject* oldContinuation = continuation();
        setContinuation(newBox);

        // Someone may have put a <p> inside a <q>, causing a split.  When this happens, the :after content
        // has to move into the inline continuation.  Call updateBeforeAfterContent to ensure that our :after
        // content gets properly destroyed.
        bool isLastChild = (beforeChild == lastChild());
        if (document()->usesBeforeAfterRules())
            children()->updateBeforeAfterContent(this, AFTER);
        if (isLastChild && beforeChild != lastChild())
            beforeChild = 0; // We destroyed the last child, so now we need to update our insertion
                             // point to be 0.  It's just a straight append now.

        splitFlow(beforeChild, newBox, newChild, oldContinuation);
        return;
    }

    RenderBoxModelObject::addChild(newChild, beforeChild);

    newChild->setNeedsLayoutAndPrefWidthsRecalc();
}

RenderInline* RenderInline::cloneInline(RenderInline* src)
{
    RenderInline* o = new (src->renderArena()) RenderInline(src->node());
    o->setStyle(src->style());
    return o;
}

void RenderInline::splitInlines(RenderBlock* fromBlock, RenderBlock* toBlock,
                                RenderBlock* middleBlock,
                                RenderObject* beforeChild, RenderBoxModelObject* oldCont)
{
    // Create a clone of this inline.
    RenderInline* clone = cloneInline(this);
    clone->setContinuation(oldCont);

    // Now take all of the children from beforeChild to the end and remove
    // them from |this| and place them in the clone.
    RenderObject* o = beforeChild;
    while (o) {
        RenderObject* tmp = o;
        o = tmp->nextSibling();
        clone->addChildIgnoringContinuation(children()->removeChildNode(this, tmp), 0);
        tmp->setNeedsLayoutAndPrefWidthsRecalc();
    }

    // Hook |clone| up as the continuation of the middle block.
    middleBlock->setContinuation(clone);

    // We have been reparented and are now under the fromBlock.  We need
    // to walk up our inline parent chain until we hit the containing block.
    // Once we hit the containing block we're done.
    RenderBoxModelObject* curr = toRenderBoxModelObject(parent());
    RenderBoxModelObject* currChild = this;
    
    // FIXME: Because splitting is O(n^2) as tags nest pathologically, we cap the depth at which we're willing to clone.
    // There will eventually be a better approach to this problem that will let us nest to a much
    // greater depth (see bugzilla bug 13430) but for now we have a limit.  This *will* result in
    // incorrect rendering, but the alternative is to hang forever.
    unsigned splitDepth = 1;
    const unsigned cMaxSplitDepth = 200; 
    while (curr && curr != fromBlock) {
        ASSERT(curr->isRenderInline());
        if (splitDepth < cMaxSplitDepth) {
            // Create a new clone.
            RenderInline* cloneChild = clone;
            clone = cloneInline(toRenderInline(curr));

            // Insert our child clone as the first child.
            clone->addChildIgnoringContinuation(cloneChild, 0);

            // Hook the clone up as a continuation of |curr|.
            RenderInline* inlineCurr = toRenderInline(curr);
            oldCont = inlineCurr->continuation();
            inlineCurr->setContinuation(clone);
            clone->setContinuation(oldCont);

            // Someone may have indirectly caused a <q> to split.  When this happens, the :after content
            // has to move into the inline continuation.  Call updateBeforeAfterContent to ensure that the inline's :after
            // content gets properly destroyed.
            if (document()->usesBeforeAfterRules())
                inlineCurr->children()->updateBeforeAfterContent(inlineCurr, AFTER);

            // Now we need to take all of the children starting from the first child
            // *after* currChild and append them all to the clone.
            o = currChild->nextSibling();
            while (o) {
                RenderObject* tmp = o;
                o = tmp->nextSibling();
                clone->addChildIgnoringContinuation(inlineCurr->children()->removeChildNode(curr, tmp), 0);
                tmp->setNeedsLayoutAndPrefWidthsRecalc();
            }
        }
        
        // Keep walking up the chain.
        currChild = curr;
        curr = toRenderBoxModelObject(curr->parent());
        splitDepth++;
    }

    // Now we are at the block level. We need to put the clone into the toBlock.
    toBlock->children()->appendChildNode(toBlock, clone);

    // Now take all the children after currChild and remove them from the fromBlock
    // and put them in the toBlock.
    o = currChild->nextSibling();
    while (o) {
        RenderObject* tmp = o;
        o = tmp->nextSibling();
        toBlock->children()->appendChildNode(toBlock, fromBlock->children()->removeChildNode(fromBlock, tmp));
    }
}

void RenderInline::splitFlow(RenderObject* beforeChild, RenderBlock* newBlockBox,
                             RenderObject* newChild, RenderBoxModelObject* oldCont)
{
    RenderBlock* pre = 0;
    RenderBlock* block = containingBlock();
    
    // Delete our line boxes before we do the inline split into continuations.
    block->deleteLineBoxTree();
    
    bool madeNewBeforeBlock = false;
    if (block->isAnonymousBlock() && (!block->parent() || !block->parent()->createsAnonymousWrapper())) {
        // We can reuse this block and make it the preBlock of the next continuation.
        pre = block;
        pre->removePositionedObjects(0);
        block = block->containingBlock();
    } else {
        // No anonymous block available for use.  Make one.
        pre = block->createAnonymousBlock();
        madeNewBeforeBlock = true;
    }

    RenderBlock* post = block->createAnonymousBlock();

    RenderObject* boxFirst = madeNewBeforeBlock ? block->firstChild() : pre->nextSibling();
    if (madeNewBeforeBlock)
        block->children()->insertChildNode(block, pre, boxFirst);
    block->children()->insertChildNode(block, newBlockBox, boxFirst);
    block->children()->insertChildNode(block, post, boxFirst);
    block->setChildrenInline(false);
    
    if (madeNewBeforeBlock) {
        RenderObject* o = boxFirst;
        while (o) {
            RenderObject* no = o;
            o = no->nextSibling();
            pre->children()->appendChildNode(pre, block->children()->removeChildNode(block, no));
            no->setNeedsLayoutAndPrefWidthsRecalc();
        }
    }

    splitInlines(pre, post, newBlockBox, beforeChild, oldCont);

    // We already know the newBlockBox isn't going to contain inline kids, so avoid wasting
    // time in makeChildrenNonInline by just setting this explicitly up front.
    newBlockBox->setChildrenInline(false);

    // We delayed adding the newChild until now so that the |newBlockBox| would be fully
    // connected, thus allowing newChild access to a renderArena should it need
    // to wrap itself in additional boxes (e.g., table construction).
    newBlockBox->addChild(newChild);

    // Always just do a full layout in order to ensure that line boxes (especially wrappers for images)
    // get deleted properly.  Because objects moves from the pre block into the post block, we want to
    // make new line boxes instead of leaving the old line boxes around.
    pre->setNeedsLayoutAndPrefWidthsRecalc();
    block->setNeedsLayoutAndPrefWidthsRecalc();
    post->setNeedsLayoutAndPrefWidthsRecalc();
}

void RenderInline::addChildToContinuation(RenderObject* newChild, RenderObject* beforeChild)
{
    RenderBoxModelObject* flow = continuationBefore(beforeChild);
    ASSERT(!beforeChild || beforeChild->parent()->isRenderBlock() || beforeChild->parent()->isRenderInline());
    RenderBoxModelObject* beforeChildParent = 0;
    if (beforeChild)
        beforeChildParent = toRenderBoxModelObject(beforeChild->parent());
    else {
        RenderBoxModelObject* cont = nextContinuation(flow);
        if (cont)
            beforeChildParent = cont;
        else
            beforeChildParent = flow;
    }

    if (newChild->isFloatingOrPositioned())
        return beforeChildParent->addChildIgnoringContinuation(newChild, beforeChild);

    // A continuation always consists of two potential candidates: an inline or an anonymous
    // block box holding block children.
    bool childInline = newChild->isInline();
    bool bcpInline = beforeChildParent->isInline();
    bool flowInline = flow->isInline();

    if (flow == beforeChildParent)
        return flow->addChildIgnoringContinuation(newChild, beforeChild);
    else {
        // The goal here is to match up if we can, so that we can coalesce and create the
        // minimal # of continuations needed for the inline.
        if (childInline == bcpInline)
            return beforeChildParent->addChildIgnoringContinuation(newChild, beforeChild);
        else if (flowInline == childInline)
            return flow->addChildIgnoringContinuation(newChild, 0); // Just treat like an append.
        else
            return beforeChildParent->addChildIgnoringContinuation(newChild, beforeChild);
    }
}

void RenderInline::paint(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    m_lineBoxes.paint(this, paintInfo, paintOffset);
}

void RenderInline::absoluteRects(Vector<LayoutRect>& rects, const LayoutPoint& accumulatedOffset) const
{
    if (!alwaysCreateLineBoxes())
        culledInlineAbsoluteRects(this, rects, toLayoutSize(accumulatedOffset));
    else if (InlineFlowBox* curr = firstLineBox()) {
        for (; curr; curr = curr->nextLineBox())
            rects.append(enclosingLayoutRect(FloatRect(accumulatedOffset + curr->topLeft(), curr->size())));
    } else
        rects.append(LayoutRect(accumulatedOffset, LayoutSize()));

    if (continuation()) {
        if (continuation()->isBox()) {
            RenderBox* box = toRenderBox(continuation());
            continuation()->absoluteRects(rects, toLayoutPoint(accumulatedOffset - containingBlock()->location() + box->size()));
        } else
            continuation()->absoluteRects(rects, toLayoutPoint(accumulatedOffset - containingBlock()->location()));
    }
}

void RenderInline::culledInlineAbsoluteRects(const RenderInline* container, Vector<LayoutRect>& rects, const LayoutSize& offset) const
{
    if (!culledInlineFirstLineBox()) {
        rects.append(IntRect(offset.width(), offset.height(), 0, 0));
        return;
    }

    bool isHorizontal = style()->isHorizontalWritingMode();
    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling()) {
        if (curr->isFloatingOrPositioned())
            continue;
            
        // We want to get the margin box in the inline direction, and then use our font ascent/descent in the block
        // direction (aligned to the root box's baseline).
        if (curr->isBox()) {
            RenderBox* currBox = toRenderBox(curr);
            if (currBox->inlineBoxWrapper()) {
                RootInlineBox* rootBox = currBox->inlineBoxWrapper()->root();
                int logicalTop = rootBox->logicalTop() + (rootBox->renderer()->style(rootBox->isFirstLineStyle())->font().fontMetrics().ascent() - container->style(rootBox->isFirstLineStyle())->font().fontMetrics().ascent());
                int logicalHeight = container->style(rootBox->isFirstLineStyle())->font().fontMetrics().height();
                FloatRect result;
                if (isHorizontal)
                    result = FloatRect(offset.width() + currBox->inlineBoxWrapper()->x() - currBox->marginLeft(), offset.height() + logicalTop, currBox->width() + currBox->marginLeft() + currBox->marginRight(), logicalHeight);
                else
                    result = FloatRect(offset.width() + logicalTop, offset.height() + currBox->inlineBoxWrapper()->y() - currBox->marginTop(), logicalHeight, currBox->height() + currBox->marginTop() + currBox->marginBottom());
                rects.append(enclosingIntRect(result));
            }
        } else if (curr->isRenderInline()) {
            // If the child doesn't need line boxes either, then we can recur.
            RenderInline* currInline = toRenderInline(curr);
            if (!currInline->alwaysCreateLineBoxes())
                currInline->culledInlineAbsoluteRects(container, rects, offset);
            else {
                for (InlineFlowBox* childLine = currInline->firstLineBox(); childLine; childLine = childLine->nextLineBox()) {
                    RootInlineBox* rootBox = childLine->root();
                    int logicalTop = rootBox->logicalTop() + (rootBox->renderer()->style(rootBox->isFirstLineStyle())->font().fontMetrics().ascent() - container->style(rootBox->isFirstLineStyle())->font().fontMetrics().ascent());
                    int logicalHeight = container->style(rootBox->isFirstLineStyle())->font().fontMetrics().height();
                    FloatRect result;
                    if (isHorizontal)
                        result = FloatRect(offset.width() + childLine->x() - childLine->marginLogicalLeft(),
                            offset.height() + logicalTop,
                            childLine->logicalWidth() + childLine->marginLogicalLeft() + childLine->marginLogicalRight(),
                            logicalHeight);
                    else
                        result = FloatRect(offset.width() + logicalTop,
                            offset.height() + childLine->y() - childLine->marginLogicalLeft(),
                            logicalHeight,
                            childLine->logicalWidth() + childLine->marginLogicalLeft() + childLine->marginLogicalRight());
                    rects.append(enclosingIntRect(result));
                }
            }
        } else if (curr->isText()) {
            RenderText* currText = toRenderText(curr);
            for (InlineTextBox* childText = currText->firstTextBox(); childText; childText = childText->nextTextBox()) {
                RootInlineBox* rootBox = childText->root();
                int logicalTop = rootBox->logicalTop() + (rootBox->renderer()->style(rootBox->isFirstLineStyle())->font().fontMetrics().ascent() - container->style(rootBox->isFirstLineStyle())->font().fontMetrics().ascent());
                int logicalHeight = container->style(rootBox->isFirstLineStyle())->font().fontMetrics().height();
                FloatRect result;
                if (isHorizontal)
                    result = FloatRect(offset.width() + childText->x(), offset.height() + logicalTop, childText->logicalWidth(), logicalHeight);
                else
                    result = FloatRect(offset.width() + logicalTop, offset.height() + childText->y(), logicalHeight, childText->logicalWidth());
                rects.append(enclosingIntRect(result));
            }
        }
    }
}

void RenderInline::absoluteQuads(Vector<FloatQuad>& quads, bool* wasFixed) const
{
    if (!alwaysCreateLineBoxes())
        culledInlineAbsoluteQuads(this, quads);
    else if (InlineFlowBox* curr = firstLineBox()) {
        for (; curr; curr = curr->nextLineBox()) {
            FloatRect localRect(curr->x(), curr->y(), curr->width(), curr->height());
            quads.append(localToAbsoluteQuad(localRect, false, wasFixed));
        }
    } else
        quads.append(localToAbsoluteQuad(FloatRect(), false, wasFixed));

    if (continuation())
        continuation()->absoluteQuads(quads, wasFixed);
}

void RenderInline::culledInlineAbsoluteQuads(const RenderInline* container, Vector<FloatQuad>& quads) const
{
    if (!culledInlineFirstLineBox()) {
        quads.append(localToAbsoluteQuad(FloatRect()));
        return;
    }

    bool isHorizontal = style()->isHorizontalWritingMode();
    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling()) {
        if (curr->isFloatingOrPositioned())
            continue;
            
        // We want to get the margin box in the inline direction, and then use our font ascent/descent in the block
        // direction (aligned to the root box's baseline).
        if (curr->isBox()) {
            RenderBox* currBox = toRenderBox(curr);
            if (currBox->inlineBoxWrapper()) {
                RootInlineBox* rootBox = currBox->inlineBoxWrapper()->root();
                int logicalTop = rootBox->logicalTop() + (rootBox->renderer()->style(rootBox->isFirstLineStyle())->font().fontMetrics().ascent() - container->style(rootBox->isFirstLineStyle())->font().fontMetrics().ascent());
                int logicalHeight = container->style(rootBox->isFirstLineStyle())->font().fontMetrics().height();
                FloatRect result;
                if (isHorizontal)
                    result = FloatRect(currBox->inlineBoxWrapper()->x() - currBox->marginLeft(), logicalTop, currBox->width() + currBox->marginLeft() + currBox->marginRight(), logicalHeight);
                else
                    result = FloatRect(logicalTop, currBox->inlineBoxWrapper()->y() - currBox->marginTop(), logicalHeight, currBox->height() + currBox->marginTop() + currBox->marginBottom());
                quads.append(localToAbsoluteQuad(result));
            }
        } else if (curr->isRenderInline()) {
            // If the child doesn't need line boxes either, then we can recur.
            RenderInline* currInline = toRenderInline(curr);
            if (!currInline->alwaysCreateLineBoxes())
                currInline->culledInlineAbsoluteQuads(container, quads);
            else {
                for (InlineFlowBox* childLine = currInline->firstLineBox(); childLine; childLine = childLine->nextLineBox()) {
                    RootInlineBox* rootBox = childLine->root();
                    int logicalTop = rootBox->logicalTop() + (rootBox->renderer()->style(rootBox->isFirstLineStyle())->font().fontMetrics().ascent() - container->style(rootBox->isFirstLineStyle())->font().fontMetrics().ascent());
                    int logicalHeight = container->style(rootBox->isFirstLineStyle())->font().fontMetrics().height();
                    FloatRect result;
                    if (isHorizontal)
                        result = FloatRect(childLine->x() - childLine->marginLogicalLeft(),
                            logicalTop,
                            childLine->logicalWidth() + childLine->marginLogicalLeft() + childLine->marginLogicalRight(),
                            logicalHeight);
                    else
                        result = FloatRect(logicalTop,
                            childLine->y() - childLine->marginLogicalLeft(),
                            logicalHeight,
                            childLine->logicalWidth() + childLine->marginLogicalLeft() + childLine->marginLogicalRight());
                    quads.append(localToAbsoluteQuad(result));
                }
            }
        } else if (curr->isText()) {
            RenderText* currText = toRenderText(curr);
            for (InlineTextBox* childText = currText->firstTextBox(); childText; childText = childText->nextTextBox()) {
                RootInlineBox* rootBox = childText->root();
                int logicalTop = rootBox->logicalTop() + (rootBox->renderer()->style(rootBox->isFirstLineStyle())->font().fontMetrics().ascent() - container->style(rootBox->isFirstLineStyle())->font().fontMetrics().ascent());
                int logicalHeight = container->style(rootBox->isFirstLineStyle())->font().fontMetrics().height();
                FloatRect result;
                if (isHorizontal)
                    result = FloatRect(childText->x(), logicalTop, childText->logicalWidth(), logicalHeight);
                else
                    result = FloatRect(logicalTop, childText->y(), logicalHeight, childText->logicalWidth());
                quads.append(localToAbsoluteQuad(result));
            }
        }
    }
}

LayoutUnit RenderInline::offsetLeft() const
{
    LayoutUnit x = RenderBoxModelObject::offsetLeft();
    if (InlineBox* firstBox = firstLineBoxIncludingCulling())
        x += firstBox->x();
    return x;
}

LayoutUnit RenderInline::offsetTop() const
{
    LayoutUnit y = RenderBoxModelObject::offsetTop();
    if (InlineBox* firstBox = firstLineBoxIncludingCulling())
        y += firstBox->y();
    return y;
}

static LayoutUnit computeMargin(const RenderInline* renderer, const Length& margin)
{
    if (margin.isAuto())
        return 0;
    if (margin.isFixed())
        return margin.value();
    if (margin.isPercent())
        return margin.calcMinValue(max<LayoutUnit>(0, renderer->containingBlock()->availableLogicalWidth()));
    return 0;
}

LayoutUnit RenderInline::marginLeft() const
{
    return computeMargin(this, style()->marginLeft());
}

LayoutUnit RenderInline::marginRight() const
{
    return computeMargin(this, style()->marginRight());
}

LayoutUnit RenderInline::marginTop() const
{
    return computeMargin(this, style()->marginTop());
}

LayoutUnit RenderInline::marginBottom() const
{
    return computeMargin(this, style()->marginBottom());
}

LayoutUnit RenderInline::marginStart() const
{
    return computeMargin(this, style()->marginStart());
}

LayoutUnit RenderInline::marginEnd() const
{
    return computeMargin(this, style()->marginEnd());
}

LayoutUnit RenderInline::marginBefore() const
{
    return computeMargin(this, style()->marginBefore());
}

LayoutUnit RenderInline::marginAfter() const
{
    return computeMargin(this, style()->marginAfter());
}

const char* RenderInline::renderName() const
{
    if (isRelPositioned())
        return "RenderInline (relative positioned)";
    if (isAnonymous())
        return "RenderInline (generated)";
    if (isRunIn())
        return "RenderInline (run-in)";
    return "RenderInline";
}

bool RenderInline::nodeAtPoint(const HitTestRequest& request, HitTestResult& result,
                                const LayoutPoint& pointInContainer, const LayoutPoint& accumulatedOffset, HitTestAction hitTestAction)
{
    return m_lineBoxes.hitTest(this, request, result, pointInContainer, accumulatedOffset, hitTestAction);
}

VisiblePosition RenderInline::positionForPoint(const LayoutPoint& point)
{
    // FIXME: Does not deal with relative positioned inlines (should it?)
    RenderBlock* cb = containingBlock();
    if (firstLineBox()) {
        // This inline actually has a line box.  We must have clicked in the border/padding of one of these boxes.  We
        // should try to find a result by asking our containing block.
        return cb->positionForPoint(point);
    }

    // Translate the coords from the pre-anonymous block to the post-anonymous block.
    LayoutPoint parentBlockPoint = cb->location() + point;  
    RenderBoxModelObject* c = continuation();
    while (c) {
        RenderBox* contBlock = c->isInline() ? c->containingBlock() : toRenderBlock(c);
        if (c->isInline() || c->firstChild())
            return c->positionForPoint(parentBlockPoint - contBlock->locationOffset());
        c = toRenderBlock(c)->inlineElementContinuation();
    }
    
    return RenderBoxModelObject::positionForPoint(point);
}

LayoutRect RenderInline::linesBoundingBox() const
{
    if (!alwaysCreateLineBoxes()) {
        ASSERT(!firstLineBox());
        return enclosingLayoutRect(culledInlineBoundingBox(this));
    }

    LayoutRect result;
    
    // See <rdar://problem/5289721>, for an unknown reason the linked list here is sometimes inconsistent, first is non-zero and last is zero.  We have been
    // unable to reproduce this at all (and consequently unable to figure ot why this is happening).  The assert will hopefully catch the problem in debug
    // builds and help us someday figure out why.  We also put in a redundant check of lastLineBox() to avoid the crash for now.
    ASSERT(!firstLineBox() == !lastLineBox());  // Either both are null or both exist.
    if (firstLineBox() && lastLineBox()) {
        // Return the width of the minimal left side and the maximal right side.
        float logicalLeftSide = 0;
        float logicalRightSide = 0;
        for (InlineFlowBox* curr = firstLineBox(); curr; curr = curr->nextLineBox()) {
            if (curr == firstLineBox() || curr->logicalLeft() < logicalLeftSide)
                logicalLeftSide = curr->logicalLeft();
            if (curr == firstLineBox() || curr->logicalRight() > logicalRightSide)
                logicalRightSide = curr->logicalRight();
        }
        
        bool isHorizontal = style()->isHorizontalWritingMode();
        
        float x = isHorizontal ? logicalLeftSide : firstLineBox()->x();
        float y = isHorizontal ? firstLineBox()->y() : logicalLeftSide;
        float width = isHorizontal ? logicalRightSide - logicalLeftSide : lastLineBox()->logicalBottom() - x;
        float height = isHorizontal ? lastLineBox()->logicalBottom() - y : logicalRightSide - logicalLeftSide;
        result = enclosingLayoutRect(FloatRect(x, y, width, height));
    }

    return result;
}

FloatRect RenderInline::culledInlineBoundingBox(const RenderInline* container) const
{
    FloatRect result;
    bool isHorizontal = style()->isHorizontalWritingMode();
    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling()) {
        if (curr->isFloatingOrPositioned())
            continue;
            
        // We want to get the margin box in the inline direction, and then use our font ascent/descent in the block
        // direction (aligned to the root box's baseline).
        if (curr->isBox()) {
            RenderBox* currBox = toRenderBox(curr);
            if (currBox->inlineBoxWrapper()) {
                RootInlineBox* rootBox = currBox->inlineBoxWrapper()->root();
                int logicalTop = rootBox->logicalTop() + (rootBox->renderer()->style(rootBox->isFirstLineStyle())->font().fontMetrics().ascent() - container->style(rootBox->isFirstLineStyle())->font().fontMetrics().ascent());
                int logicalHeight = container->style(rootBox->isFirstLineStyle())->font().fontMetrics().height();
                if (isHorizontal)
                    result.uniteIfNonZero(FloatRect(currBox->inlineBoxWrapper()->x() - currBox->marginLeft(), logicalTop, currBox->width() + currBox->marginLeft() + currBox->marginRight(), logicalHeight));
                else
                    result.uniteIfNonZero(FloatRect(logicalTop, currBox->inlineBoxWrapper()->y() - currBox->marginTop(), logicalHeight, currBox->height() + currBox->marginTop() + currBox->marginBottom()));
            }
        } else if (curr->isRenderInline()) {
            // If the child doesn't need line boxes either, then we can recur.
            RenderInline* currInline = toRenderInline(curr);
            if (!currInline->alwaysCreateLineBoxes())
                result.uniteIfNonZero(currInline->culledInlineBoundingBox(container));
            else {
                for (InlineFlowBox* childLine = currInline->firstLineBox(); childLine; childLine = childLine->nextLineBox()) {
                    RootInlineBox* rootBox = childLine->root();
                    int logicalTop = rootBox->logicalTop() + (rootBox->renderer()->style(rootBox->isFirstLineStyle())->font().fontMetrics().ascent() - container->style(rootBox->isFirstLineStyle())->font().fontMetrics().ascent());
                    int logicalHeight = container->style(rootBox->isFirstLineStyle())->font().fontMetrics().height();
                    if (isHorizontal)
                        result.uniteIfNonZero(FloatRect(childLine->x() - childLine->marginLogicalLeft(),
                            logicalTop,
                            childLine->logicalWidth() + childLine->marginLogicalLeft() + childLine->marginLogicalRight(),
                            logicalHeight));
                    else
                        result.uniteIfNonZero(FloatRect(logicalTop,
                            childLine->y() - childLine->marginLogicalLeft(),
                            logicalHeight,
                            childLine->logicalWidth() + childLine->marginLogicalLeft() + childLine->marginLogicalRight()));
                                     
                }
            }
        } else if (curr->isText()) {
            RenderText* currText = toRenderText(curr);
            for (InlineTextBox* childText = currText->firstTextBox(); childText; childText = childText->nextTextBox()) {
                RootInlineBox* rootBox = childText->root();
                int logicalTop = rootBox->logicalTop() + (rootBox->renderer()->style(rootBox->isFirstLineStyle())->font().fontMetrics().ascent() - container->style(rootBox->isFirstLineStyle())->font().fontMetrics().ascent());
                int logicalHeight = container->style(rootBox->isFirstLineStyle())->font().fontMetrics().height();
                if (isHorizontal)
                    result.uniteIfNonZero(FloatRect(childText->x(), logicalTop, childText->logicalWidth(), logicalHeight));
                else
                    result.uniteIfNonZero(FloatRect(logicalTop, childText->y(), logicalHeight, childText->logicalWidth()));
            }
        }
    }
    return enclosingLayoutRect(result);
}

InlineBox* RenderInline::culledInlineFirstLineBox() const
{
    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling()) {
        if (curr->isFloatingOrPositioned())
            continue;
            
        // We want to get the margin box in the inline direction, and then use our font ascent/descent in the block
        // direction (aligned to the root box's baseline).
        if (curr->isBox())
            return toRenderBox(curr)->inlineBoxWrapper();
        if (curr->isRenderInline()) {
            RenderInline* currInline = toRenderInline(curr);
            InlineBox* result = currInline->firstLineBoxIncludingCulling();
            if (result)
                return result;
        } else if (curr->isText()) {
            RenderText* currText = toRenderText(curr);
            if (currText->firstTextBox())
                return currText->firstTextBox();
        }
    }
    return 0;
}

InlineBox* RenderInline::culledInlineLastLineBox() const
{
    for (RenderObject* curr = lastChild(); curr; curr = curr->previousSibling()) {
        if (curr->isFloatingOrPositioned())
            continue;
            
        // We want to get the margin box in the inline direction, and then use our font ascent/descent in the block
        // direction (aligned to the root box's baseline).
        if (curr->isBox())
            return toRenderBox(curr)->inlineBoxWrapper();
        if (curr->isRenderInline()) {
            RenderInline* currInline = toRenderInline(curr);
            InlineBox* result = currInline->lastLineBoxIncludingCulling();
            if (result)
                return result;
        } else if (curr->isText()) {
            RenderText* currText = toRenderText(curr);
            if (currText->lastTextBox())
                return currText->lastTextBox();
        }
    }
    return 0;
}

LayoutRect RenderInline::culledInlineVisualOverflowBoundingBox() const
{
    LayoutRect result(culledInlineBoundingBox(this));
    bool isHorizontal = style()->isHorizontalWritingMode();
    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling()) {
        if (curr->isFloatingOrPositioned())
            continue;
            
        // For overflow we just have to propagate by hand and recompute it all.
        if (curr->isBox()) {
            RenderBox* currBox = toRenderBox(curr);
            if (!currBox->hasSelfPaintingLayer() && currBox->inlineBoxWrapper()) {
                LayoutRect logicalRect = currBox->logicalVisualOverflowRectForPropagation(style());
                if (isHorizontal) {
                    logicalRect.moveBy(currBox->location());
                    result.uniteIfNonZero(logicalRect);
                } else {
                    logicalRect.moveBy(currBox->location());
                    result.uniteIfNonZero(logicalRect.transposedRect());
                }
            }
        } else if (curr->isRenderInline()) {
            // If the child doesn't need line boxes either, then we can recur.
            RenderInline* currInline = toRenderInline(curr);
            if (!currInline->alwaysCreateLineBoxes())
                result.uniteIfNonZero(currInline->culledInlineVisualOverflowBoundingBox());
            else if (!currInline->hasSelfPaintingLayer())
                result.uniteIfNonZero(currInline->linesVisualOverflowBoundingBox());
        } else if (curr->isText()) {
            // FIXME; Overflow from text boxes is lost. We will need to cache this information in
            // InlineTextBoxes.
            RenderText* currText = toRenderText(curr);
            result.uniteIfNonZero(currText->linesVisualOverflowBoundingBox());
        }
    }
    return result;
}

LayoutRect RenderInline::linesVisualOverflowBoundingBox() const
{
    if (!alwaysCreateLineBoxes())
        return culledInlineVisualOverflowBoundingBox();

    if (!firstLineBox() || !lastLineBox())
        return LayoutRect();

    // Return the width of the minimal left side and the maximal right side.
    LayoutUnit logicalLeftSide = numeric_limits<LayoutUnit>::max();
    LayoutUnit logicalRightSide = numeric_limits<LayoutUnit>::min();
    for (InlineFlowBox* curr = firstLineBox(); curr; curr = curr->nextLineBox()) {
        logicalLeftSide = min(logicalLeftSide, curr->logicalLeftVisualOverflow());
        logicalRightSide = max(logicalRightSide, curr->logicalRightVisualOverflow());
    }

    RootInlineBox* firstRootBox = firstLineBox()->root();
    RootInlineBox* lastRootBox = lastLineBox()->root();
    
    LayoutUnit logicalTop = firstLineBox()->logicalTopVisualOverflow(firstRootBox->lineTop());
    LayoutUnit logicalWidth = logicalRightSide - logicalLeftSide;
    LayoutUnit logicalHeight = lastLineBox()->logicalBottomVisualOverflow(lastRootBox->lineBottom()) - logicalTop;
    
    LayoutRect rect(logicalLeftSide, logicalTop, logicalWidth, logicalHeight);
    if (!style()->isHorizontalWritingMode())
        rect = rect.transposedRect();
    return rect;
}

LayoutRect RenderInline::clippedOverflowRectForRepaint(RenderBoxModelObject* repaintContainer) const
{
    // Only run-ins are allowed in here during layout.
    ASSERT(!view() || !view()->layoutStateEnabled() || isRunIn());

    if (!firstLineBoxIncludingCulling() && !continuation())
        return LayoutRect();

    // Find our leftmost position.
    LayoutRect boundingBox(linesVisualOverflowBoundingBox());
    LayoutUnit left = boundingBox.x();
    LayoutUnit top = boundingBox.y();

    // Now invalidate a rectangle.
    LayoutUnit ow = style() ? style()->outlineSize() : 0;
    
    // We need to add in the relative position offsets of any inlines (including us) up to our
    // containing block.
    RenderBlock* cb = containingBlock();
    for (const RenderObject* inlineFlow = this; inlineFlow && inlineFlow->isRenderInline() && inlineFlow != cb;
         inlineFlow = inlineFlow->parent()) {
         if (inlineFlow->style()->position() == RelativePosition && inlineFlow->hasLayer())
            toRenderInline(inlineFlow)->layer()->relativePositionOffset(left, top);
    }

    LayoutRect r(-ow + left, -ow + top, boundingBox.width() + ow * 2, boundingBox.height() + ow * 2);

    if (cb->hasColumns())
        cb->adjustRectForColumns(r);

    if (cb->hasOverflowClip()) {
        // cb->height() is inaccurate if we're in the middle of a layout of |cb|, so use the
        // layer's size instead.  Even if the layer's size is wrong, the layer itself will repaint
        // anyway if its size does change.
        LayoutRect repaintRect(r);
        repaintRect.move(-cb->layer()->scrolledContentOffset()); // For overflow:auto/scroll/hidden.

        LayoutRect boxRect(LayoutPoint(), cb->layer()->size());
        r = intersection(repaintRect, boxRect);
    }
    
    // FIXME: need to ensure that we compute the correct repaint rect when the repaint container
    // is an inline.
    if (repaintContainer != this)
        cb->computeRectForRepaint(repaintContainer, r);

    if (ow) {
        for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling()) {
            if (!curr->isText()) {
                LayoutRect childRect = curr->rectWithOutlineForRepaint(repaintContainer, ow);
                r.unite(childRect);
            }
        }

        if (continuation() && !continuation()->isInline()) {
            LayoutRect contRect = continuation()->rectWithOutlineForRepaint(repaintContainer, ow);
            r.unite(contRect);
        }
    }

    return r;
}

LayoutRect RenderInline::rectWithOutlineForRepaint(RenderBoxModelObject* repaintContainer, LayoutUnit outlineWidth) const
{
    LayoutRect r(RenderBoxModelObject::rectWithOutlineForRepaint(repaintContainer, outlineWidth));
    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling()) {
        if (!curr->isText())
            r.unite(curr->rectWithOutlineForRepaint(repaintContainer, outlineWidth));
    }
    return r;
}

void RenderInline::computeRectForRepaint(RenderBoxModelObject* repaintContainer, LayoutRect& rect, bool fixed) const
{
    if (RenderView* v = view()) {
        // LayoutState is only valid for root-relative repainting
        if (v->layoutStateEnabled() && !repaintContainer) {
            LayoutState* layoutState = v->layoutState();
            if (style()->position() == RelativePosition && layer())
                rect.move(layer()->relativePositionOffset());
            rect.move(layoutState->m_paintOffset);
            if (layoutState->m_clipped)
                rect.intersect(layoutState->m_clipRect);
            return;
        }
    }

    if (repaintContainer == this)
        return;

    bool containerSkipped;
    RenderObject* o = container(repaintContainer, &containerSkipped);
    if (!o)
        return;

    LayoutPoint topLeft = rect.location();

    if (o->isBlockFlow() && style()->position() != AbsolutePosition && style()->position() != FixedPosition) {
        RenderBlock* cb = toRenderBlock(o);
        if (cb->hasColumns()) {
            LayoutRect repaintRect(topLeft, rect.size());
            cb->adjustRectForColumns(repaintRect);
            topLeft = repaintRect.location();
            rect = repaintRect;
        }
    }

    if (style()->position() == RelativePosition && layer()) {
        // Apply the relative position offset when invalidating a rectangle.  The layer
        // is translated, but the render box isn't, so we need to do this to get the
        // right dirty rect.  Since this is called from RenderObject::setStyle, the relative position
        // flag on the RenderObject has been cleared, so use the one on the style().
        topLeft += layer()->relativePositionOffset();
    }
    
    // FIXME: We ignore the lightweight clipping rect that controls use, since if |o| is in mid-layout,
    // its controlClipRect will be wrong. For overflow clip we use the values cached by the layer.
    if (o->hasOverflowClip()) {
        RenderBox* containerBox = toRenderBox(o);

        // o->height() is inaccurate if we're in the middle of a layout of |o|, so use the
        // layer's size instead.  Even if the layer's size is wrong, the layer itself will repaint
        // anyway if its size does change.
        topLeft -= containerBox->layer()->scrolledContentOffset(); // For overflow:auto/scroll/hidden.

        LayoutRect repaintRect(topLeft, rect.size());
        LayoutRect boxRect(LayoutPoint(), containerBox->layer()->size());
        rect = intersection(repaintRect, boxRect);
        if (rect.isEmpty())
            return;
    } else
        rect.setLocation(topLeft);

    if (containerSkipped) {
        // If the repaintContainer is below o, then we need to map the rect into repaintContainer's coordinates.
        LayoutSize containerOffset = repaintContainer->offsetFromAncestorContainer(o);
        rect.move(-containerOffset);
        return;
    }
    
    o->computeRectForRepaint(repaintContainer, rect, fixed);
}

LayoutSize RenderInline::offsetFromContainer(RenderObject* container, const LayoutPoint& point) const
{
    ASSERT(container == this->container());
    
    LayoutSize offset;    
    if (isRelPositioned())
        offset += relativePositionOffset();

    container->adjustForColumns(offset, point);

    if (container->hasOverflowClip())
        offset -= toRenderBox(container)->layer()->scrolledContentOffset();

    return offset;
}

void RenderInline::mapLocalToContainer(RenderBoxModelObject* repaintContainer, bool fixed, bool useTransforms, TransformState& transformState, bool* wasFixed) const
{
    if (repaintContainer == this)
        return;

    if (RenderView *v = view()) {
        if (v->layoutStateEnabled() && !repaintContainer) {
            LayoutState* layoutState = v->layoutState();
            LayoutSize offset = layoutState->m_paintOffset;
            if (style()->position() == RelativePosition && layer())
                offset += layer()->relativePositionOffset();
            transformState.move(offset);
            return;
        }
    }

    bool containerSkipped;
    RenderObject* o = container(repaintContainer, &containerSkipped);
    if (!o)
        return;

    IntPoint centerPoint = roundedIntPoint(transformState.mappedPoint());
    if (o->isBox() && o->style()->isFlippedBlocksWritingMode())
        transformState.move(toRenderBox(o)->flipForWritingModeIncludingColumns(roundedIntPoint(transformState.mappedPoint())) - centerPoint);

    LayoutSize containerOffset = offsetFromContainer(o, roundedLayoutPoint(transformState.mappedPoint()));

    bool preserve3D = useTransforms && (o->style()->preserves3D() || style()->preserves3D());
    if (useTransforms && shouldUseTransformFromContainer(o)) {
        TransformationMatrix t;
        getTransformFromContainer(o, containerOffset, t);
        transformState.applyTransform(t, preserve3D ? TransformState::AccumulateTransform : TransformState::FlattenTransform);
    } else
        transformState.move(containerOffset.width(), containerOffset.height(), preserve3D ? TransformState::AccumulateTransform : TransformState::FlattenTransform);

    if (containerSkipped) {
        // There can't be a transform between repaintContainer and o, because transforms create containers, so it should be safe
        // to just subtract the delta between the repaintContainer and o.
        LayoutSize containerOffset = repaintContainer->offsetFromAncestorContainer(o);
        transformState.move(-containerOffset.width(), -containerOffset.height(), preserve3D ? TransformState::AccumulateTransform : TransformState::FlattenTransform);
        return;
    }

    o->mapLocalToContainer(repaintContainer, fixed, useTransforms, transformState, wasFixed);
}

void RenderInline::mapAbsoluteToLocalPoint(bool fixed, bool useTransforms, TransformState& transformState) const
{
    // We don't expect this function to be called during layout.
    ASSERT(!view() || !view()->layoutStateEnabled());

    RenderObject* o = container();
    if (!o)
        return;

    o->mapAbsoluteToLocalPoint(fixed, useTransforms, transformState);

    LayoutSize containerOffset = offsetFromContainer(o, LayoutPoint());

    bool preserve3D = useTransforms && (o->style()->preserves3D() || style()->preserves3D());
    if (useTransforms && shouldUseTransformFromContainer(o)) {
        TransformationMatrix t;
        getTransformFromContainer(o, containerOffset, t);
        transformState.applyTransform(t, preserve3D ? TransformState::AccumulateTransform : TransformState::FlattenTransform);
    } else
        transformState.move(containerOffset.width(), containerOffset.height(), preserve3D ? TransformState::AccumulateTransform : TransformState::FlattenTransform);
}

void RenderInline::updateDragState(bool dragOn)
{
    RenderBoxModelObject::updateDragState(dragOn);
    if (continuation())
        continuation()->updateDragState(dragOn);
}

void RenderInline::childBecameNonInline(RenderObject* child)
{
    // We have to split the parent flow.
    RenderBlock* newBox = containingBlock()->createAnonymousBlock();
    RenderBoxModelObject* oldContinuation = continuation();
    setContinuation(newBox);
    RenderObject* beforeChild = child->nextSibling();
    children()->removeChildNode(this, child);
    splitFlow(beforeChild, newBox, child, oldContinuation);
}

void RenderInline::updateHitTestResult(HitTestResult& result, const LayoutPoint& point)
{
    if (result.innerNode())
        return;

    Node* n = node();
    LayoutPoint localPoint(point);
    if (n) {
        if (isInlineElementContinuation()) {
            // We're in the continuation of a split inline.  Adjust our local point to be in the coordinate space
            // of the principal renderer's containing block.  This will end up being the innerNonSharedNode.
            RenderBlock* firstBlock = n->renderer()->containingBlock();
            
            // Get our containing block.
            RenderBox* block = containingBlock();
            localPoint.move(block->x() - firstBlock->x(), block->y() - firstBlock->y());
        }

        result.setInnerNode(n);
        if (!result.innerNonSharedNode())
            result.setInnerNonSharedNode(n);
        result.setLocalPoint(localPoint);
    }
}

void RenderInline::dirtyLineBoxes(bool fullLayout)
{
    if (fullLayout) {
        m_lineBoxes.deleteLineBoxes(renderArena());
        return;
    }

    if (!alwaysCreateLineBoxes()) {
        // We have to grovel into our children in order to dirty the appropriate lines.
        for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling()) {
            if (curr->isFloatingOrPositioned())
                continue;
            if (curr->isBox() && !curr->needsLayout()) {
                RenderBox* currBox = toRenderBox(curr);
                if (currBox->inlineBoxWrapper())
                    currBox->inlineBoxWrapper()->root()->markDirty();
            } else if (!curr->selfNeedsLayout()) {
                if (curr->isRenderInline()) {
                    RenderInline* currInline = toRenderInline(curr);
                    for (InlineFlowBox* childLine = currInline->firstLineBox(); childLine; childLine = childLine->nextLineBox())
                        childLine->root()->markDirty();
                } else if (curr->isText()) {
                    RenderText* currText = toRenderText(curr);
                    for (InlineTextBox* childText = currText->firstTextBox(); childText; childText = childText->nextTextBox())
                        childText->root()->markDirty();
                }
            }
        }
    } else
        m_lineBoxes.dirtyLineBoxes();
}

InlineFlowBox* RenderInline::createInlineFlowBox() 
{
    return new (renderArena()) InlineFlowBox(this);
}

InlineFlowBox* RenderInline::createAndAppendInlineFlowBox()
{
    setAlwaysCreateLineBoxes();
    InlineFlowBox* flowBox = createInlineFlowBox();
    m_lineBoxes.appendLineBox(flowBox);
    return flowBox;
}

LayoutUnit RenderInline::lineHeight(bool firstLine, LineDirectionMode /*direction*/, LinePositionMode /*linePositionMode*/) const
{
    if (firstLine && document()->usesFirstLineRules()) {
        RenderStyle* s = style(firstLine);
        if (s != style())
            return s->computedLineHeight();
    }
    
    if (m_lineHeight == -1)
        m_lineHeight = style()->computedLineHeight();
    
    return m_lineHeight;
}

LayoutUnit RenderInline::baselinePosition(FontBaseline baselineType, bool firstLine, LineDirectionMode direction, LinePositionMode linePositionMode) const
{
    const FontMetrics& fontMetrics = style(firstLine)->fontMetrics();
    return fontMetrics.ascent(baselineType) + (lineHeight(firstLine, direction, linePositionMode) - fontMetrics.height()) / 2;
}

LayoutSize RenderInline::relativePositionedInlineOffset(const RenderBox* child) const
{
    // FIXME: This function isn't right with mixed writing modes.

    ASSERT(isRelPositioned());
    if (!isRelPositioned())
        return LayoutSize();

    // When we have an enclosing relpositioned inline, we need to add in the offset of the first line
    // box from the rest of the content, but only in the cases where we know we're positioned
    // relative to the inline itself.

    LayoutSize logicalOffset;
    LayoutUnit inlinePosition;
    LayoutUnit blockPosition;
    if (firstLineBox()) {
        inlinePosition = roundedLayoutUnit(firstLineBox()->logicalLeft());
        blockPosition = firstLineBox()->logicalTop();
    } else {
        inlinePosition = layer()->staticInlinePosition();
        blockPosition = layer()->staticBlockPosition();
    }

    if (!child->style()->hasStaticInlinePosition(style()->isHorizontalWritingMode()))
        logicalOffset.setWidth(inlinePosition);

    // This is not terribly intuitive, but we have to match other browsers.  Despite being a block display type inside
    // an inline, we still keep our x locked to the left of the relative positioned inline.  Arguably the correct
    // behavior would be to go flush left to the block that contains the inline, but that isn't what other browsers
    // do.
    else if (!child->style()->isOriginalDisplayInlineType())
        // Avoid adding in the left border/padding of the containing block twice.  Subtract it out.
        logicalOffset.setWidth(inlinePosition - child->containingBlock()->borderAndPaddingLogicalLeft());

    if (!child->style()->hasStaticBlockPosition(style()->isHorizontalWritingMode()))
        logicalOffset.setHeight(blockPosition);

    return style()->isHorizontalWritingMode() ? logicalOffset : logicalOffset.transposedSize();
}

void RenderInline::imageChanged(WrappedImagePtr, const IntRect*)
{
    if (!parent())
        return;
        
    // FIXME: We can do better.
    repaint();
}

void RenderInline::addFocusRingRects(Vector<LayoutRect>& rects, const LayoutPoint& additionalOffset)
{
    if (!alwaysCreateLineBoxes())
        culledInlineAbsoluteRects(this, rects, toLayoutSize(additionalOffset));
    else {
        for (InlineFlowBox* curr = firstLineBox(); curr; curr = curr->nextLineBox())
            rects.append(enclosingLayoutRect(FloatRect(additionalOffset.x() + curr->x(), additionalOffset.y() + curr->y(), curr->width(), curr->height())));
    }

    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling()) {
        if (!curr->isText() && !curr->isListMarker()) {
            FloatPoint pos(additionalOffset);
            // FIXME: This doesn't work correctly with transforms.
            if (curr->hasLayer()) 
                pos = curr->localToAbsolute();
            else if (curr->isBox())
                pos.move(toRenderBox(curr)->x(), toRenderBox(curr)->y());
            curr->addFocusRingRects(rects, flooredIntPoint(pos));
        }
    }

    if (continuation()) {
        if (continuation()->isInline())
            continuation()->addFocusRingRects(rects, flooredLayoutPoint(additionalOffset + continuation()->containingBlock()->location() - containingBlock()->location()));
        else
            continuation()->addFocusRingRects(rects, flooredLayoutPoint(additionalOffset + toRenderBox(continuation())->location() - containingBlock()->location()));
    }
}

void RenderInline::paintOutline(GraphicsContext* graphicsContext, const LayoutPoint& paintOffset)
{
    if (!hasOutline())
        return;
    
    RenderStyle* styleToUse = style();
    if (styleToUse->outlineStyleIsAuto() || hasOutlineAnnotation()) {
        if (!theme()->supportsFocusRing(styleToUse)) {
            // Only paint the focus ring by hand if the theme isn't able to draw the focus ring.
            paintFocusRing(graphicsContext, paintOffset, styleToUse);
        }
    }

    if (graphicsContext->paintingDisabled())
        return;

    if (styleToUse->outlineStyleIsAuto() || styleToUse->outlineStyle() == BNONE)
        return;

    Vector<LayoutRect> rects;

    rects.append(LayoutRect());
    for (InlineFlowBox* curr = firstLineBox(); curr; curr = curr->nextLineBox()) {
        RootInlineBox* root = curr->root();
        LayoutUnit top = max(root->lineTop(), curr->logicalTop());
        LayoutUnit bottom = min(root->lineBottom(), curr->logicalBottom());
        rects.append(LayoutRect(curr->x(), top, curr->logicalWidth(), bottom - top));
    }
    rects.append(LayoutRect());

    Color outlineColor = styleToUse->visitedDependentColor(CSSPropertyOutlineColor);
    bool useTransparencyLayer = outlineColor.hasAlpha();
    if (useTransparencyLayer) {
        graphicsContext->beginTransparencyLayer(static_cast<float>(outlineColor.alpha()) / 255);
        outlineColor = Color(outlineColor.red(), outlineColor.green(), outlineColor.blue());
    }

    for (unsigned i = 1; i < rects.size() - 1; i++)
        paintOutlineForLine(graphicsContext, paintOffset, rects.at(i - 1), rects.at(i), rects.at(i + 1), outlineColor);

    if (useTransparencyLayer)
        graphicsContext->endTransparencyLayer();
}

void RenderInline::paintOutlineForLine(GraphicsContext* graphicsContext, const LayoutPoint& paintOffset,
                                       const LayoutRect& lastline, const LayoutRect& thisline, const LayoutRect& nextline,
                                       const Color outlineColor)
{
    RenderStyle* styleToUse = style();
    LayoutUnit outlineWidth = styleToUse->outlineWidth();
    EBorderStyle outlineStyle = styleToUse->outlineStyle();

    bool antialias = shouldAntialiasLines(graphicsContext);

    LayoutUnit offset = style()->outlineOffset();

    LayoutUnit top = paintOffset.y() + thisline.y() - offset;
    LayoutUnit left = paintOffset.x() + thisline.x() - offset;
    LayoutUnit bottom = paintOffset.y() + thisline.maxY() + offset;
    LayoutUnit right = paintOffset.x() + thisline.maxX() + offset;
    
    // left edge
    drawLineForBoxSide(graphicsContext,
        left - outlineWidth,
        top - (lastline.isEmpty() || thisline.x() < lastline.x() || (lastline.maxX() - 1) <= thisline.x() ? outlineWidth : 0),
        left,
        bottom + (nextline.isEmpty() || thisline.x() <= nextline.x() || (nextline.maxX() - 1) <= thisline.x() ? outlineWidth : 0),
        BSLeft,
        outlineColor, outlineStyle,
        (lastline.isEmpty() || thisline.x() < lastline.x() || (lastline.maxX() - 1) <= thisline.x() ? outlineWidth : -outlineWidth),
        (nextline.isEmpty() || thisline.x() <= nextline.x() || (nextline.maxX() - 1) <= thisline.x() ? outlineWidth : -outlineWidth),
        antialias);
    
    // right edge
    drawLineForBoxSide(graphicsContext,
        right,
        top - (lastline.isEmpty() || lastline.maxX() < thisline.maxX() || (thisline.maxX() - 1) <= lastline.x() ? outlineWidth : 0),
        right + outlineWidth,
        bottom + (nextline.isEmpty() || nextline.maxX() <= thisline.maxX() || (thisline.maxX() - 1) <= nextline.x() ? outlineWidth : 0),
        BSRight,
        outlineColor, outlineStyle,
        (lastline.isEmpty() || lastline.maxX() < thisline.maxX() || (thisline.maxX() - 1) <= lastline.x() ? outlineWidth : -outlineWidth),
        (nextline.isEmpty() || nextline.maxX() <= thisline.maxX() || (thisline.maxX() - 1) <= nextline.x() ? outlineWidth : -outlineWidth),
        antialias);
    // upper edge
    if (thisline.x() < lastline.x())
        drawLineForBoxSide(graphicsContext,
            left - outlineWidth,
            top - outlineWidth,
            min(right + outlineWidth, (lastline.isEmpty() ? 1000000 : paintOffset.x() + lastline.x())),
            top,
            BSTop, outlineColor, outlineStyle,
            outlineWidth,
            (!lastline.isEmpty() && paintOffset.x() + lastline.x() + 1 < right + outlineWidth) ? -outlineWidth : outlineWidth,
            antialias);
    
    if (lastline.maxX() < thisline.maxX())
        drawLineForBoxSide(graphicsContext,
            max(lastline.isEmpty() ? -1000000 : paintOffset.x() + lastline.maxX(), left - outlineWidth),
            top - outlineWidth,
            right + outlineWidth,
            top,
            BSTop, outlineColor, outlineStyle,
            (!lastline.isEmpty() && left - outlineWidth < paintOffset.x() + lastline.maxX()) ? -outlineWidth : outlineWidth,
            outlineWidth, antialias);

    if (thisline.x() == thisline.maxX())
          drawLineForBoxSide(graphicsContext,
            left - outlineWidth,
            top - outlineWidth,
            right + outlineWidth,
            top,
            BSTop, outlineColor, outlineStyle,
            outlineWidth,
            outlineWidth,
            antialias);

    // lower edge
    if (thisline.x() < nextline.x())
        drawLineForBoxSide(graphicsContext,
            left - outlineWidth,
            bottom,
            min(right + outlineWidth, !nextline.isEmpty() ? paintOffset.x() + nextline.x() + 1 : 1000000),
            bottom + outlineWidth,
            BSBottom, outlineColor, outlineStyle,
            outlineWidth,
            (!nextline.isEmpty() && paintOffset.x() + nextline.x() + 1 < right + outlineWidth) ? -outlineWidth : outlineWidth,
            antialias);
    
    if (nextline.maxX() < thisline.maxX())
        drawLineForBoxSide(graphicsContext,
            max(!nextline.isEmpty() ? paintOffset.x() + nextline.maxX() : -1000000, left - outlineWidth),
            bottom,
            right + outlineWidth,
            bottom + outlineWidth,
            BSBottom, outlineColor, outlineStyle,
            (!nextline.isEmpty() && left - outlineWidth < paintOffset.x() + nextline.maxX()) ? -outlineWidth : outlineWidth,
            outlineWidth, antialias);

    if (thisline.x() == thisline.maxX())
          drawLineForBoxSide(graphicsContext,
            left - outlineWidth,
            bottom,
            right + outlineWidth,
            bottom + outlineWidth,
            BSBottom, outlineColor, outlineStyle,
            outlineWidth,
            outlineWidth,
            antialias);
}

#if ENABLE(DASHBOARD_SUPPORT)
void RenderInline::addDashboardRegions(Vector<DashboardRegionValue>& regions)
{
    // Convert the style regions to absolute coordinates.
    if (style()->visibility() != VISIBLE)
        return;

    const Vector<StyleDashboardRegion>& styleRegions = style()->dashboardRegions();
    unsigned i, count = styleRegions.size();
    for (i = 0; i < count; i++) {
        StyleDashboardRegion styleRegion = styleRegions[i];

        LayoutRect linesBoundingBox = this->linesBoundingBox();
        LayoutUnit w = linesBoundingBox.width();
        LayoutUnit h = linesBoundingBox.height();

        DashboardRegionValue region;
        region.label = styleRegion.label;
        region.bounds = LayoutRect(linesBoundingBox.x() + styleRegion.offset.left().value(),
                                linesBoundingBox.y() + styleRegion.offset.top().value(),
                                w - styleRegion.offset.left().value() - styleRegion.offset.right().value(),
                                h - styleRegion.offset.top().value() - styleRegion.offset.bottom().value());
        region.type = styleRegion.type;

        RenderObject* container = containingBlock();
        if (!container)
            container = this;

        region.clip = region.bounds;
        container->computeAbsoluteRepaintRect(region.clip);
        if (region.clip.height() < 0) {
            region.clip.setHeight(0);
            region.clip.setWidth(0);
        }

        FloatPoint absPos = container->localToAbsolute();
        region.bounds.setX(absPos.x() + region.bounds.x());
        region.bounds.setY(absPos.y() + region.bounds.y());

        if (frame()) {
            float deviceScaleFactor = frame()->page()->deviceScaleFactor();
            if (deviceScaleFactor != 1.0f) {
                region.bounds.scale(deviceScaleFactor);
                region.clip.scale(deviceScaleFactor);
            }
        }

        regions.append(region);
    }
}
#endif

} // namespace WebCore
