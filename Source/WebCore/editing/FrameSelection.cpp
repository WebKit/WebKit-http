/*
 * Copyright (C) 2004, 2008, 2009, 2010, 2014 Apple Inc. All rights reserved.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */
  
#include "config.h"
#include "FrameSelection.h"

#include "CharacterData.h"
#include "DeleteSelectionCommand.h"
#include "Document.h"
#include "Editor.h"
#include "EditorClient.h"
#include "Element.h"
#include "ElementIterator.h"
#include "EventHandler.h"
#include "ExceptionCode.h"
#include "FloatQuad.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameTree.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "HTMLFormElement.h"
#include "HTMLFrameElementBase.h"
#include "HTMLInputElement.h"
#include "HTMLSelectElement.h"
#include "HTMLNames.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "InlineTextBox.h"
#include "Page.h"
#include "RenderText.h"
#include "RenderTextControl.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "RenderWidget.h"
#include "RenderedPosition.h"
#include "Settings.h"
#include "SpatialNavigation.h"
#include "StyleProperties.h"
#include "TypingCommand.h"
#include "VisibleUnits.h"
#include "htmlediting.h"
#include <stdio.h>
#include <wtf/text/CString.h>

#if PLATFORM(IOS)
#include "Chrome.h"
#include "ChromeClient.h"
#include "Color.h"
#include "RenderLayer.h"
#include "RenderObject.h"
#include "RenderStyle.h"
#endif

#define EDIT_DEBUG 0

namespace WebCore {

using namespace HTMLNames;

static inline LayoutUnit NoXPosForVerticalArrowNavigation()
{
    return LayoutUnit::min();
}

CaretBase::CaretBase(CaretVisibility visibility)
    : m_caretRectNeedsUpdate(true)
    , m_caretVisibility(visibility)
{
}

DragCaretController::DragCaretController()
    : CaretBase(Visible)
{
}

bool DragCaretController::isContentRichlyEditable() const
{
    return isRichlyEditablePosition(m_position.deepEquivalent());
}

static inline bool shouldAlwaysUseDirectionalSelection(Frame* frame)
{
    return !frame || frame->editor().behavior().shouldConsiderSelectionAsDirectional();
}

FrameSelection::FrameSelection(Frame* frame)
    : m_frame(frame)
    , m_xPosForVerticalArrowNavigation(NoXPosForVerticalArrowNavigation())
    , m_granularity(CharacterGranularity)
    , m_caretBlinkTimer(this, &FrameSelection::caretBlinkTimerFired)
    , m_absCaretBoundsDirty(true)
    , m_caretPaint(true)
    , m_isCaretBlinkingSuspended(false)
    , m_focused(frame && frame->page() && frame->page()->focusController().focusedFrame() == frame)
    , m_shouldShowBlockCursor(false)
    , m_pendingSelectionUpdate(false)
    , m_shouldRevealSelection(false)
    , m_alwaysAlignCursorOnScrollWhenRevealingSelection(false)
#if PLATFORM(IOS)
    , m_updateAppearanceEnabled(false)
    , m_caretBlinks(true)
    , m_scrollingSuppressCount(0)
#endif
{
    if (shouldAlwaysUseDirectionalSelection(m_frame))
        m_selection.setIsDirectional(true);
}

Element* FrameSelection::rootEditableElementOrDocumentElement() const
{
    Element* selectionRoot = m_selection.rootEditableElement();
    return selectionRoot ? selectionRoot : m_frame->document()->documentElement();
}

void FrameSelection::moveTo(const VisiblePosition &pos, EUserTriggered userTriggered, CursorAlignOnScroll align)
{
    setSelection(VisibleSelection(pos.deepEquivalent(), pos.deepEquivalent(), pos.affinity(), m_selection.isDirectional()),
        defaultSetSelectionOptions(userTriggered), align);
}

void FrameSelection::moveTo(const VisiblePosition &base, const VisiblePosition &extent, EUserTriggered userTriggered)
{
    const bool selectionHasDirection = true;
    setSelection(VisibleSelection(base.deepEquivalent(), extent.deepEquivalent(), base.affinity(), selectionHasDirection), defaultSetSelectionOptions(userTriggered));
}

void FrameSelection::moveTo(const Position &pos, EAffinity affinity, EUserTriggered userTriggered)
{
    setSelection(VisibleSelection(pos, affinity, m_selection.isDirectional()), defaultSetSelectionOptions(userTriggered));
}

void FrameSelection::moveTo(const Range* range)
{
    VisibleSelection selection = range ? VisibleSelection(range->startPosition(), range->endPosition()) : VisibleSelection();
    setSelection(selection);
}

void FrameSelection::moveTo(const Position &base, const Position &extent, EAffinity affinity, EUserTriggered userTriggered)
{
    const bool selectionHasDirection = true;
    setSelection(VisibleSelection(base, extent, affinity, selectionHasDirection), defaultSetSelectionOptions(userTriggered));
}

void FrameSelection::moveWithoutValidationTo(const Position& base, const Position& extent, bool selectionHasDirection, bool shouldSetFocus)
{
    VisibleSelection newSelection;
    newSelection.setWithoutValidation(base, extent);
    newSelection.setIsDirectional(selectionHasDirection);
    setSelection(newSelection, defaultSetSelectionOptions() | (shouldSetFocus ? 0 : DoNotSetFocus));
}

void DragCaretController::setCaretPosition(const VisiblePosition& position)
{
    if (Node* node = m_position.deepEquivalent().deprecatedNode())
        invalidateCaretRect(node);
    m_position = position;
    setCaretRectNeedsUpdate();
    Document* document = 0;
    if (Node* node = m_position.deepEquivalent().deprecatedNode()) {
        invalidateCaretRect(node);
        document = &node->document();
    }
    if (m_position.isNull() || m_position.isOrphan())
        clearCaretRect();
    else
        updateCaretRect(document, m_position);
}

static void adjustEndpointsAtBidiBoundary(VisiblePosition& visibleBase, VisiblePosition& visibleExtent)
{
    RenderedPosition base(visibleBase);
    RenderedPosition extent(visibleExtent);

    if (base.isNull() || extent.isNull() || base.isEquivalent(extent))
        return;

    if (base.atLeftBoundaryOfBidiRun()) {
        if (!extent.atRightBoundaryOfBidiRun(base.bidiLevelOnRight())
            && base.isEquivalent(extent.leftBoundaryOfBidiRun(base.bidiLevelOnRight()))) {
            visibleBase = base.positionAtLeftBoundaryOfBiDiRun();
            return;
        }
        return;
    }

    if (base.atRightBoundaryOfBidiRun()) {
        if (!extent.atLeftBoundaryOfBidiRun(base.bidiLevelOnLeft())
            && base.isEquivalent(extent.rightBoundaryOfBidiRun(base.bidiLevelOnLeft()))) {
            visibleBase = base.positionAtRightBoundaryOfBiDiRun();
            return;
        }
        return;
    }

    if (extent.atLeftBoundaryOfBidiRun() && extent.isEquivalent(base.leftBoundaryOfBidiRun(extent.bidiLevelOnRight()))) {
        visibleExtent = extent.positionAtLeftBoundaryOfBiDiRun();
        return;
    }

    if (extent.atRightBoundaryOfBidiRun() && extent.isEquivalent(base.rightBoundaryOfBidiRun(extent.bidiLevelOnLeft()))) {
        visibleExtent = extent.positionAtRightBoundaryOfBiDiRun();
        return;
    }
}

void FrameSelection::setSelectionByMouseIfDifferent(const VisibleSelection& passedNewSelection, TextGranularity granularity,
    EndPointsAdjustmentMode endpointsAdjustmentMode)
{
    VisibleSelection newSelection = passedNewSelection;
    bool isDirectional = shouldAlwaysUseDirectionalSelection(m_frame) || newSelection.isDirectional();

    VisiblePosition base = m_originalBase.isNotNull() ? m_originalBase : newSelection.visibleBase();
    VisiblePosition newBase = base;
    VisiblePosition extent = newSelection.visibleExtent();
    VisiblePosition newExtent = extent;
    if (endpointsAdjustmentMode == AdjustEndpointsAtBidiBoundary)
        adjustEndpointsAtBidiBoundary(newBase, newExtent);

    if (newBase != base || newExtent != extent) {
        m_originalBase = base;
        newSelection.setBase(newBase);
        newSelection.setExtent(newExtent);
    } else if (m_originalBase.isNotNull()) {
        if (m_selection.base() == newSelection.base())
            newSelection.setBase(m_originalBase);
        m_originalBase.clear();
    }

    newSelection.setIsDirectional(isDirectional); // Adjusting base and extent will make newSelection always directional
    if (m_selection == newSelection || !shouldChangeSelection(newSelection))
        return;

    setSelection(newSelection, defaultSetSelectionOptions() | FireSelectEvent, AlignCursorOnScrollIfNeeded, granularity);
}

bool FrameSelection::setSelectionWithoutUpdatingAppearance(const VisibleSelection& newSelectionPossiblyWithoutDirection, SetSelectionOptions options, CursorAlignOnScroll align, TextGranularity granularity)
{
    bool closeTyping = options & CloseTyping;
    bool shouldClearTypingStyle = options & ClearTypingStyle;

    VisibleSelection newSelection = newSelectionPossiblyWithoutDirection;
    if (shouldAlwaysUseDirectionalSelection(m_frame))
        newSelection.setIsDirectional(true);

    if (!m_frame) {
        m_selection = newSelection;
        return false;
    }

    // <http://bugs.webkit.org/show_bug.cgi?id=23464>: Infinite recursion at FrameSelection::setSelection
    // if document->frame() == m_frame we can get into an infinite loop
    if (Document* newSelectionDocument = newSelection.base().document()) {
        if (RefPtr<Frame> newSelectionFrame = newSelectionDocument->frame()) {
            if (newSelectionFrame != m_frame && newSelectionDocument != m_frame->document()) {
                newSelectionFrame->selection().setSelection(newSelection, options, align, granularity);
                // It's possible that during the above set selection, this FrameSelection has been modified by
                // selectFrameElementInParentIfFullySelected, but that the selection is no longer valid since
                // the frame is about to be destroyed. If this is the case, clear our selection.
                if (newSelectionFrame->hasOneRef() && !m_selection.isNonOrphanedCaretOrRange())
                    clear();
                return false;
            }
        }
    }

    m_granularity = granularity;

    if (closeTyping)
        TypingCommand::closeTyping(m_frame);

    if (shouldClearTypingStyle)
        clearTypingStyle();

    VisibleSelection oldSelection = m_selection;
    m_selection = newSelection;

    // Selection offsets should increase when LF is inserted before the caret in InsertLineBreakCommand. See <https://webkit.org/b/56061>.
    if (HTMLTextFormControlElement* textControl = enclosingTextFormControl(newSelection.start()))
        textControl->selectionChanged(options & FireSelectEvent);

    if (oldSelection == newSelection)
        return false;

    setCaretRectNeedsUpdate();

    if (!newSelection.isNone() && !(options & DoNotSetFocus))
        setFocusedElementIfNeeded();

    // Always clear the x position used for vertical arrow navigation.
    // It will be restored by the vertical arrow navigation code if necessary.
    m_xPosForVerticalArrowNavigation = NoXPosForVerticalArrowNavigation();
    selectFrameElementInParentIfFullySelected();
    m_frame->editor().respondToChangedSelection(oldSelection, options);
    m_frame->document()->enqueueDocumentEvent(Event::create(eventNames().selectionchangeEvent, false, false));

    return true;
}

void FrameSelection::setSelection(const VisibleSelection& selection, SetSelectionOptions options, CursorAlignOnScroll align, TextGranularity granularity)
{
    if (!setSelectionWithoutUpdatingAppearance(selection, options, align, granularity))
        return;

    Document* document = m_frame->document();
    if (!document)
        return;

    m_shouldRevealSelection = options & RevealSelection;
    m_alwaysAlignCursorOnScrollWhenRevealingSelection = align == AlignCursorOnScrollAlways;

    m_pendingSelectionUpdate = true;

    if (document->hasPendingStyleRecalc())
        return;

    FrameView* frameView = document->view();
    if (frameView && frameView->layoutPending())
        return;

    updateAndRevealSelection();
}

static void updateSelectionByUpdatingLayoutOrStyle(Frame& frame)
{
#if ENABLE(TEXT_CARET)
    frame.document()->updateLayoutIgnorePendingStylesheets();
#else
    frame.document()->updateStyleIfNeeded();
#endif
}

void FrameSelection::setNeedsSelectionUpdate()
{
    m_pendingSelectionUpdate = true;
    if (RenderView* view = m_frame->contentRenderer())
        view->clearSelection();
}

void FrameSelection::updateAndRevealSelection()
{
    if (!m_pendingSelectionUpdate)
        return;

    m_pendingSelectionUpdate = false;

    updateAppearance();

    if (m_shouldRevealSelection) {
        ScrollAlignment alignment;

        if (m_frame->editor().behavior().shouldCenterAlignWhenSelectionIsRevealed())
            alignment = m_alwaysAlignCursorOnScrollWhenRevealingSelection ? ScrollAlignment::alignCenterAlways : ScrollAlignment::alignCenterIfNeeded;
        else
            alignment = m_alwaysAlignCursorOnScrollWhenRevealingSelection ? ScrollAlignment::alignTopAlways : ScrollAlignment::alignToEdgeIfNeeded;

        revealSelection(alignment, RevealExtent);
    }

    notifyAccessibilityForSelectionChange();
}

void FrameSelection::updateDataDetectorsForSelection()
{
#if ENABLE(TELEPHONE_NUMBER_DETECTION) && !PLATFORM(IOS)
    m_frame->editor().scanSelectionForTelephoneNumbers();
#endif
}

static bool removingNodeRemovesPosition(Node* node, const Position& position)
{
    if (!position.anchorNode())
        return false;

    if (position.anchorNode() == node)
        return true;

    if (!node->isElementNode())
        return false;

    Element* element = toElement(node);
    return element->containsIncludingShadowDOM(position.anchorNode());
}

static void clearRenderViewSelection(Node& node)
{
    Ref<Document> document(node.document());
    document->updateStyleIfNeeded();
    if (RenderView* view = document->renderView())
        view->clearSelection();
}

void DragCaretController::nodeWillBeRemoved(Node* node)
{
    if (!hasCaret() || (node && !node->inDocument()))
        return;

    if (!removingNodeRemovesPosition(node, m_position.deepEquivalent()))
        return;

    clearRenderViewSelection(*node);
    clear();
}

void FrameSelection::nodeWillBeRemoved(Node* node)
{
    // There can't be a selection inside a fragment, so if a fragment's node is being removed,
    // the selection in the document that created the fragment needs no adjustment.
    if (isNone() || (node && !node->inDocument()))
        return;

    respondToNodeModification(node, removingNodeRemovesPosition(node, m_selection.base()), removingNodeRemovesPosition(node, m_selection.extent()),
        removingNodeRemovesPosition(node, m_selection.start()), removingNodeRemovesPosition(node, m_selection.end()));
}

void FrameSelection::respondToNodeModification(Node* node, bool baseRemoved, bool extentRemoved, bool startRemoved, bool endRemoved)
{
    bool clearRenderTreeSelection = false;
    bool clearDOMTreeSelection = false;

    if (startRemoved || endRemoved) {
        Position start = m_selection.start();
        Position end = m_selection.end();
        if (startRemoved)
            updatePositionForNodeRemoval(start, node);
        if (endRemoved)
            updatePositionForNodeRemoval(end, node);

        if (start.isNotNull() && end.isNotNull()) {
            if (m_selection.isBaseFirst())
                m_selection.setWithoutValidation(start, end);
            else
                m_selection.setWithoutValidation(end, start);
        } else
            clearDOMTreeSelection = true;

        clearRenderTreeSelection = true;
    } else if (baseRemoved || extentRemoved) {
        // The base and/or extent are about to be removed, but the start and end aren't.
        // Change the base and extent to the start and end, but don't re-validate the
        // selection, since doing so could move the start and end into the node
        // that is about to be removed.
        if (m_selection.isBaseFirst())
            m_selection.setWithoutValidation(m_selection.start(), m_selection.end());
        else
            m_selection.setWithoutValidation(m_selection.end(), m_selection.start());
    } else if (RefPtr<Range> range = m_selection.firstRange()) {
        ExceptionCode ec = 0;
        Range::CompareResults compareResult = range->compareNode(node, ec);
        if (!ec && (compareResult == Range::NODE_BEFORE_AND_AFTER || compareResult == Range::NODE_INSIDE)) {
            // If we did nothing here, when this node's renderer was destroyed, the rect that it 
            // occupied would be invalidated, but, selection gaps that change as a result of 
            // the removal wouldn't be invalidated.
            // FIXME: Don't do so much unnecessary invalidation.
            clearRenderTreeSelection = true;
        }
    }

    if (clearRenderTreeSelection) {
        clearRenderViewSelection(*node);

        // Trigger a selection update so the selection will be set again.
        if (auto* renderView = node->document().renderView()) {
            m_pendingSelectionUpdate = true;
            renderView->setNeedsLayout();
        }
    }

    if (clearDOMTreeSelection)
        setSelection(VisibleSelection(), DoNotSetFocus);
}

static void updatePositionAfterAdoptingTextReplacement(Position& position, CharacterData* node, unsigned offset, unsigned oldLength, unsigned newLength)
{
    if (!position.anchorNode() || position.anchorNode() != node || position.anchorType() != Position::PositionIsOffsetInAnchor)
        return;

    // See: http://www.w3.org/TR/DOM-Level-2-Traversal-Range/ranges.html#Level-2-Range-Mutation
    ASSERT(position.offsetInContainerNode() >= 0);
    unsigned positionOffset = static_cast<unsigned>(position.offsetInContainerNode());
    // Replacing text can be viewed as a deletion followed by insertion.
    if (positionOffset >= offset && positionOffset <= offset + oldLength)
        position.moveToOffset(offset);

    // Adjust the offset if the position is after the end of the deleted contents
    // (positionOffset > offset + oldLength) to avoid having a stale offset.
    if (positionOffset > offset + oldLength)
        position.moveToOffset(positionOffset - oldLength + newLength);

    ASSERT(static_cast<unsigned>(position.offsetInContainerNode()) <= node->length());
}

void FrameSelection::textWasReplaced(CharacterData* node, unsigned offset, unsigned oldLength, unsigned newLength)
{
    // The fragment check is a performance optimization. See http://trac.webkit.org/changeset/30062.
    if (isNone() || !node || !node->inDocument())
        return;

    Position base = m_selection.base();
    Position extent = m_selection.extent();
    Position start = m_selection.start();
    Position end = m_selection.end();
    updatePositionAfterAdoptingTextReplacement(base, node, offset, oldLength, newLength);
    updatePositionAfterAdoptingTextReplacement(extent, node, offset, oldLength, newLength);
    updatePositionAfterAdoptingTextReplacement(start, node, offset, oldLength, newLength);
    updatePositionAfterAdoptingTextReplacement(end, node, offset, oldLength, newLength);

    if (base != m_selection.base() || extent != m_selection.extent() || start != m_selection.start() || end != m_selection.end()) {
        VisibleSelection newSelection;
        if (base != extent)
            newSelection.setWithoutValidation(base, extent);
        else if (m_selection.isDirectional() && !m_selection.isBaseFirst())
            newSelection.setWithoutValidation(end, start);
        else
            newSelection.setWithoutValidation(start, end);

        setSelection(newSelection, DoNotSetFocus);
    }
}

TextDirection FrameSelection::directionOfEnclosingBlock()
{
    return WebCore::directionOfEnclosingBlock(m_selection.extent());
}

TextDirection FrameSelection::directionOfSelection()
{
    InlineBox* startBox = 0;
    InlineBox* endBox = 0;
    int unusedOffset;
    // Cache the VisiblePositions because visibleStart() and visibleEnd()
    // can cause layout, which has the potential to invalidate lineboxes.
    VisiblePosition startPosition = m_selection.visibleStart();
    VisiblePosition endPosition = m_selection.visibleEnd();
    if (startPosition.isNotNull())
        startPosition.getInlineBoxAndOffset(startBox, unusedOffset);
    if (endPosition.isNotNull())
        endPosition.getInlineBoxAndOffset(endBox, unusedOffset);
    if (startBox && endBox && startBox->direction() == endBox->direction())
        return startBox->direction();

    return directionOfEnclosingBlock();
}

void FrameSelection::willBeModified(EAlteration alter, SelectionDirection direction)
{
    if (alter != AlterationExtend)
        return;

    Position start = m_selection.start();
    Position end = m_selection.end();

    bool baseIsStart = true;

    if (m_selection.isDirectional()) {
        // Make base and extent match start and end so we extend the user-visible selection.
        // This only matters for cases where base and extend point to different positions than
        // start and end (e.g. after a double-click to select a word).
        if (m_selection.isBaseFirst())
            baseIsStart = true;
        else
            baseIsStart = false;
    } else {
        switch (direction) {
        case DirectionRight:
            if (directionOfSelection() == LTR)
                baseIsStart = true;
            else
                baseIsStart = false;
            break;
        case DirectionForward:
            baseIsStart = true;
            break;
        case DirectionLeft:
            if (directionOfSelection() == LTR)
                baseIsStart = false;
            else
                baseIsStart = true;
            break;
        case DirectionBackward:
            baseIsStart = false;
            break;
        }
    }
    if (baseIsStart) {
        m_selection.setBase(start);
        m_selection.setExtent(end);
    } else {
        m_selection.setBase(end);
        m_selection.setExtent(start);
    }
}

VisiblePosition FrameSelection::positionForPlatform(bool isGetStart) const
{
    if (m_frame && m_frame->settings().editingBehaviorType() == EditingMacBehavior)
        return isGetStart ? m_selection.visibleStart() : m_selection.visibleEnd();
    // Linux and Windows always extend selections from the extent endpoint.
    // FIXME: VisibleSelection should be fixed to ensure as an invariant that
    // base/extent always point to the same nodes as start/end, but which points
    // to which depends on the value of isBaseFirst. Then this can be changed
    // to just return m_sel.extent().
    return m_selection.isBaseFirst() ? m_selection.visibleEnd() : m_selection.visibleStart();
}

VisiblePosition FrameSelection::startForPlatform() const
{
    return positionForPlatform(true);
}

VisiblePosition FrameSelection::endForPlatform() const
{
    return positionForPlatform(false);
}

VisiblePosition FrameSelection::nextWordPositionForPlatform(const VisiblePosition &originalPosition)
{
    VisiblePosition positionAfterCurrentWord = nextWordPosition(originalPosition);

    if (m_frame && m_frame->editor().behavior().shouldSkipSpaceWhenMovingRight()) {
        // In order to skip spaces when moving right, we advance one
        // word further and then move one word back. Given the
        // semantics of previousWordPosition() this will put us at the
        // beginning of the word following.
        VisiblePosition positionAfterSpacingAndFollowingWord = nextWordPosition(positionAfterCurrentWord);
        if (positionAfterSpacingAndFollowingWord != positionAfterCurrentWord)
            positionAfterCurrentWord = previousWordPosition(positionAfterSpacingAndFollowingWord);

        bool movingBackwardsMovedPositionToStartOfCurrentWord = positionAfterCurrentWord == previousWordPosition(nextWordPosition(originalPosition));
        if (movingBackwardsMovedPositionToStartOfCurrentWord)
            positionAfterCurrentWord = positionAfterSpacingAndFollowingWord;
    }
    return positionAfterCurrentWord;
}

#if ENABLE(USERSELECT_ALL)
static void adjustPositionForUserSelectAll(VisiblePosition& pos, bool isForward)
{
    if (Node* rootUserSelectAll = Position::rootUserSelectAllForNode(pos.deepEquivalent().anchorNode()))
        pos = isForward ? positionAfterNode(rootUserSelectAll).downstream(CanCrossEditingBoundary) : positionBeforeNode(rootUserSelectAll).upstream(CanCrossEditingBoundary);
}
#endif

VisiblePosition FrameSelection::modifyExtendingRight(TextGranularity granularity)
{
    VisiblePosition pos(m_selection.extent(), m_selection.affinity());

    // The difference between modifyExtendingRight and modifyExtendingForward is:
    // modifyExtendingForward always extends forward logically.
    // modifyExtendingRight behaves the same as modifyExtendingForward except for extending character or word,
    // it extends forward logically if the enclosing block is LTR direction,
    // but it extends backward logically if the enclosing block is RTL direction.
    switch (granularity) {
    case CharacterGranularity:
        if (directionOfEnclosingBlock() == LTR)
            pos = pos.next(CannotCrossEditingBoundary);
        else
            pos = pos.previous(CannotCrossEditingBoundary);
        break;
    case WordGranularity:
        if (directionOfEnclosingBlock() == LTR)
            pos = nextWordPositionForPlatform(pos);
        else
            pos = previousWordPosition(pos);
        break;
    case LineBoundary:
        if (directionOfEnclosingBlock() == LTR)
            pos = modifyExtendingForward(granularity);
        else
            pos = modifyExtendingBackward(granularity);
        break;
    case SentenceGranularity:
    case LineGranularity:
    case ParagraphGranularity:
    case SentenceBoundary:
    case ParagraphBoundary:
    case DocumentBoundary:
        // FIXME: implement all of the above?
        pos = modifyExtendingForward(granularity);
        break;
    case DocumentGranularity:
        ASSERT_NOT_REACHED();
        break;
    }
#if ENABLE(USERSELECT_ALL)
    adjustPositionForUserSelectAll(pos, directionOfEnclosingBlock() == LTR);
#endif
    return pos;
}

VisiblePosition FrameSelection::modifyExtendingForward(TextGranularity granularity)
{
    VisiblePosition pos(m_selection.extent(), m_selection.affinity());
    switch (granularity) {
    case CharacterGranularity:
        pos = pos.next(CannotCrossEditingBoundary);
        break;
    case WordGranularity:
        pos = nextWordPositionForPlatform(pos);
        break;
    case SentenceGranularity:
        pos = nextSentencePosition(pos);
        break;
    case LineGranularity:
        pos = nextLinePosition(pos, lineDirectionPointForBlockDirectionNavigation(EXTENT));
        break;
    case ParagraphGranularity:
        pos = nextParagraphPosition(pos, lineDirectionPointForBlockDirectionNavigation(EXTENT));
        break;
    case DocumentGranularity:
        ASSERT_NOT_REACHED();
        break;
    case SentenceBoundary:
        pos = endOfSentence(endForPlatform());
        break;
    case LineBoundary:
        pos = logicalEndOfLine(endForPlatform());
        break;
    case ParagraphBoundary:
        pos = endOfParagraph(endForPlatform());
        break;
    case DocumentBoundary:
        pos = endForPlatform();
        if (isEditablePosition(pos.deepEquivalent()))
            pos = endOfEditableContent(pos);
        else
            pos = endOfDocument(pos);
        break;
    }
#if ENABLE(USERSELECT_ALL)
     adjustPositionForUserSelectAll(pos, directionOfEnclosingBlock() == LTR);
#endif
    return pos;
}

VisiblePosition FrameSelection::modifyMovingRight(TextGranularity granularity)
{
    VisiblePosition pos;
    switch (granularity) {
    case CharacterGranularity:
        if (isRange()) {
            if (directionOfSelection() == LTR)
                pos = VisiblePosition(m_selection.end(), m_selection.affinity());
            else
                pos = VisiblePosition(m_selection.start(), m_selection.affinity());
        } else
            pos = VisiblePosition(m_selection.extent(), m_selection.affinity()).right(true);
        break;
    case WordGranularity: {
        // Visual word movement relies on isWordTextBreak which is not implemented in WinCE and QT.
        // https://bugs.webkit.org/show_bug.cgi?id=81136.
        bool skipsSpaceWhenMovingRight = m_frame && m_frame->editor().behavior().shouldSkipSpaceWhenMovingRight();
        pos = rightWordPosition(VisiblePosition(m_selection.extent(), m_selection.affinity()), skipsSpaceWhenMovingRight);
        break;
    }
    case SentenceGranularity:
    case LineGranularity:
    case ParagraphGranularity:
    case SentenceBoundary:
    case ParagraphBoundary:
    case DocumentBoundary:
        // FIXME: Implement all of the above.
        pos = modifyMovingForward(granularity);
        break;
    case LineBoundary:
        pos = rightBoundaryOfLine(startForPlatform(), directionOfEnclosingBlock());
        break;
    case DocumentGranularity:
        ASSERT_NOT_REACHED();
        break;
    }
    return pos;
}

VisiblePosition FrameSelection::modifyMovingForward(TextGranularity granularity)
{
    VisiblePosition pos;
    // FIXME: Stay in editable content for the less common granularities.
    switch (granularity) {
    case CharacterGranularity:
        if (isRange())
            pos = VisiblePosition(m_selection.end(), m_selection.affinity());
        else
            pos = VisiblePosition(m_selection.extent(), m_selection.affinity()).next(CannotCrossEditingBoundary);
        break;
    case WordGranularity:
        pos = nextWordPositionForPlatform(VisiblePosition(m_selection.extent(), m_selection.affinity()));
        break;
    case SentenceGranularity:
        pos = nextSentencePosition(VisiblePosition(m_selection.extent(), m_selection.affinity()));
        break;
    case LineGranularity: {
        // down-arrowing from a range selection that ends at the start of a line needs
        // to leave the selection at that line start (no need to call nextLinePosition!)
        pos = endForPlatform();
        if (!isRange() || !isStartOfLine(pos))
            pos = nextLinePosition(pos, lineDirectionPointForBlockDirectionNavigation(START));
        break;
    }
    case ParagraphGranularity:
        pos = nextParagraphPosition(endForPlatform(), lineDirectionPointForBlockDirectionNavigation(START));
        break;
    case DocumentGranularity:
        ASSERT_NOT_REACHED();
        break;
    case SentenceBoundary:
        pos = endOfSentence(endForPlatform());
        break;
    case LineBoundary:
        pos = logicalEndOfLine(endForPlatform());
        break;
    case ParagraphBoundary:
        pos = endOfParagraph(endForPlatform());
        break;
    case DocumentBoundary:
        pos = endForPlatform();
        if (isEditablePosition(pos.deepEquivalent()))
            pos = endOfEditableContent(pos);
        else
            pos = endOfDocument(pos);
        break;
    }
    return pos;
}

VisiblePosition FrameSelection::modifyExtendingLeft(TextGranularity granularity)
{
    VisiblePosition pos(m_selection.extent(), m_selection.affinity());

    // The difference between modifyExtendingLeft and modifyExtendingBackward is:
    // modifyExtendingBackward always extends backward logically.
    // modifyExtendingLeft behaves the same as modifyExtendingBackward except for extending character or word,
    // it extends backward logically if the enclosing block is LTR direction,
    // but it extends forward logically if the enclosing block is RTL direction.
    switch (granularity) {
    case CharacterGranularity:
        if (directionOfEnclosingBlock() == LTR)
            pos = pos.previous(CannotCrossEditingBoundary);
        else
            pos = pos.next(CannotCrossEditingBoundary);
        break;
    case WordGranularity:
        if (directionOfEnclosingBlock() == LTR)
            pos = previousWordPosition(pos);
        else
            pos = nextWordPositionForPlatform(pos);
        break;
    case LineBoundary:
        if (directionOfEnclosingBlock() == LTR)
            pos = modifyExtendingBackward(granularity);
        else
            pos = modifyExtendingForward(granularity);
        break;
    case SentenceGranularity:
    case LineGranularity:
    case ParagraphGranularity:
    case SentenceBoundary:
    case ParagraphBoundary:
    case DocumentBoundary:
        pos = modifyExtendingBackward(granularity);
        break;
    case DocumentGranularity:
        ASSERT_NOT_REACHED();
        break;
    }
#if ENABLE(USERSELECT_ALL)
    adjustPositionForUserSelectAll(pos, !(directionOfEnclosingBlock() == LTR));
#endif
    return pos;
}
       
VisiblePosition FrameSelection::modifyExtendingBackward(TextGranularity granularity)
{
    VisiblePosition pos(m_selection.extent(), m_selection.affinity());

    // Extending a selection backward by word or character from just after a table selects
    // the table.  This "makes sense" from the user perspective, esp. when deleting.
    // It was done here instead of in VisiblePosition because we want VPs to iterate
    // over everything.
    switch (granularity) {
    case CharacterGranularity:
        pos = pos.previous(CannotCrossEditingBoundary);
        break;
    case WordGranularity:
        pos = previousWordPosition(pos);
        break;
    case SentenceGranularity:
        pos = previousSentencePosition(pos);
        break;
    case LineGranularity:
        pos = previousLinePosition(pos, lineDirectionPointForBlockDirectionNavigation(EXTENT));
        break;
    case ParagraphGranularity:
        pos = previousParagraphPosition(pos, lineDirectionPointForBlockDirectionNavigation(EXTENT));
        break;
    case SentenceBoundary:
        pos = startOfSentence(startForPlatform());
        break;
    case LineBoundary:
        pos = logicalStartOfLine(startForPlatform());
        break;
    case ParagraphBoundary:
        pos = startOfParagraph(startForPlatform());
        break;
    case DocumentBoundary:
        pos = startForPlatform();
        if (isEditablePosition(pos.deepEquivalent()))
            pos = startOfEditableContent(pos);
        else
            pos = startOfDocument(pos);
        break;
    case DocumentGranularity:
        ASSERT_NOT_REACHED();
        break;
    }
#if ENABLE(USERSELECT_ALL)
    adjustPositionForUserSelectAll(pos, !(directionOfEnclosingBlock() == LTR));
#endif
    return pos;
}

VisiblePosition FrameSelection::modifyMovingLeft(TextGranularity granularity)
{
    VisiblePosition pos;
    switch (granularity) {
    case CharacterGranularity:
        if (isRange())
            if (directionOfSelection() == LTR)
                pos = VisiblePosition(m_selection.start(), m_selection.affinity());
            else
                pos = VisiblePosition(m_selection.end(), m_selection.affinity());
        else
            pos = VisiblePosition(m_selection.extent(), m_selection.affinity()).left(true);
        break;
    case WordGranularity: {
        bool skipsSpaceWhenMovingRight = m_frame && m_frame->editor().behavior().shouldSkipSpaceWhenMovingRight();
        pos = leftWordPosition(VisiblePosition(m_selection.extent(), m_selection.affinity()), skipsSpaceWhenMovingRight);
        break;
    }
    case SentenceGranularity:
    case LineGranularity:
    case ParagraphGranularity:
    case SentenceBoundary:
    case ParagraphBoundary:
    case DocumentBoundary:
        // FIXME: Implement all of the above.
        pos = modifyMovingBackward(granularity);
        break;
    case LineBoundary:
        pos = leftBoundaryOfLine(startForPlatform(), directionOfEnclosingBlock());
        break;
    case DocumentGranularity:
        ASSERT_NOT_REACHED();
        break;
    }
    return pos;
}

VisiblePosition FrameSelection::modifyMovingBackward(TextGranularity granularity)
{
    VisiblePosition pos;
    switch (granularity) {
    case CharacterGranularity:
        if (isRange())
            pos = VisiblePosition(m_selection.start(), m_selection.affinity());
        else
            pos = VisiblePosition(m_selection.extent(), m_selection.affinity()).previous(CannotCrossEditingBoundary);
        break;
    case WordGranularity:
        pos = previousWordPosition(VisiblePosition(m_selection.extent(), m_selection.affinity()));
        break;
    case SentenceGranularity:
        pos = previousSentencePosition(VisiblePosition(m_selection.extent(), m_selection.affinity()));
        break;
    case LineGranularity:
        pos = previousLinePosition(startForPlatform(), lineDirectionPointForBlockDirectionNavigation(START));
        break;
    case ParagraphGranularity:
        pos = previousParagraphPosition(startForPlatform(), lineDirectionPointForBlockDirectionNavigation(START));
        break;
    case SentenceBoundary:
        pos = startOfSentence(startForPlatform());
        break;
    case LineBoundary:
        pos = logicalStartOfLine(startForPlatform());
        break;
    case ParagraphBoundary:
        pos = startOfParagraph(startForPlatform());
        break;
    case DocumentBoundary:
        pos = startForPlatform();
        if (isEditablePosition(pos.deepEquivalent()))
            pos = startOfEditableContent(pos);
        else
            pos = startOfDocument(pos);
        break;
    case DocumentGranularity:
        ASSERT_NOT_REACHED();
        break;
    }
    return pos;
}

static bool isBoundary(TextGranularity granularity)
{
    return granularity == LineBoundary || granularity == ParagraphBoundary || granularity == DocumentBoundary;
}    

bool FrameSelection::modify(EAlteration alter, SelectionDirection direction, TextGranularity granularity, EUserTriggered userTriggered)
{
    if (userTriggered == UserTriggered) {
        FrameSelection trialFrameSelection;
        trialFrameSelection.setSelection(m_selection);
        trialFrameSelection.modify(alter, direction, granularity, NotUserTriggered);

        bool change = shouldChangeSelection(trialFrameSelection.selection());
        if (!change)
            return false;

        if (trialFrameSelection.selection().isRange() && m_selection.isCaret() && !dispatchSelectStart())
            return false;
    }

    willBeModified(alter, direction);

    bool wasRange = m_selection.isRange();
    Position originalStartPosition = m_selection.start();
    VisiblePosition position;
    switch (direction) {
    case DirectionRight:
        if (alter == AlterationMove)
            position = modifyMovingRight(granularity);
        else
            position = modifyExtendingRight(granularity);
        break;
    case DirectionForward:
        if (alter == AlterationExtend)
            position = modifyExtendingForward(granularity);
        else
            position = modifyMovingForward(granularity);
        break;
    case DirectionLeft:
        if (alter == AlterationMove)
            position = modifyMovingLeft(granularity);
        else
            position = modifyExtendingLeft(granularity);
        break;
    case DirectionBackward:
        if (alter == AlterationExtend)
            position = modifyExtendingBackward(granularity);
        else
            position = modifyMovingBackward(granularity);
        break;
    }

    if (position.isNull())
        return false;

    if (isSpatialNavigationEnabled(m_frame))
        if (!wasRange && alter == AlterationMove && position == originalStartPosition)
            return false;

    // Some of the above operations set an xPosForVerticalArrowNavigation.
    // Setting a selection will clear it, so save it to possibly restore later.
    // Note: the START position type is arbitrary because it is unused, it would be
    // the requested position type if there were no xPosForVerticalArrowNavigation set.
    LayoutUnit x = lineDirectionPointForBlockDirectionNavigation(START);
    m_selection.setIsDirectional(shouldAlwaysUseDirectionalSelection(m_frame) || alter == AlterationExtend);

    switch (alter) {
    case AlterationMove:
        moveTo(position, userTriggered);
        break;
    case AlterationExtend:

        if (!m_selection.isCaret()
            && (granularity == WordGranularity || granularity == ParagraphGranularity || granularity == LineGranularity)
            && m_frame && !m_frame->editor().behavior().shouldExtendSelectionByWordOrLineAcrossCaret()) {
            // Don't let the selection go across the base position directly. Needed to match mac
            // behavior when, for instance, word-selecting backwards starting with the caret in
            // the middle of a word and then word-selecting forward, leaving the caret in the
            // same place where it was, instead of directly selecting to the end of the word.
            VisibleSelection newSelection = m_selection;
            newSelection.setExtent(position);
            if (m_selection.isBaseFirst() != newSelection.isBaseFirst())
                position = m_selection.base();
        }

        // Standard Mac behavior when extending to a boundary is grow the selection rather than leaving the
        // base in place and moving the extent. Matches NSTextView.
        if (!m_frame || !m_frame->editor().behavior().shouldAlwaysGrowSelectionWhenExtendingToBoundary() || m_selection.isCaret() || !isBoundary(granularity))
            setExtent(position, userTriggered);
        else {
            TextDirection textDirection = directionOfEnclosingBlock();
            if (direction == DirectionForward || (textDirection == LTR && direction == DirectionRight) || (textDirection == RTL && direction == DirectionLeft))
                setEnd(position, userTriggered);
            else
                setStart(position, userTriggered);
        }
        break;
    }
    
    if (granularity == LineGranularity || granularity == ParagraphGranularity)
        m_xPosForVerticalArrowNavigation = x;

    if (userTriggered == UserTriggered)
        m_granularity = CharacterGranularity;

    setCaretRectNeedsUpdate();

    return true;
}

// FIXME: Maybe baseline would be better?
static bool absoluteCaretY(const VisiblePosition &c, int &y)
{
    IntRect rect = c.absoluteCaretBounds();
    if (rect.isEmpty())
        return false;
    y = rect.y() + rect.height() / 2;
    return true;
}

bool FrameSelection::modify(EAlteration alter, unsigned verticalDistance, VerticalDirection direction, EUserTriggered userTriggered, CursorAlignOnScroll align)
{
    if (!verticalDistance)
        return false;

    if (userTriggered == UserTriggered) {
        FrameSelection trialFrameSelection;
        trialFrameSelection.setSelection(m_selection);
        trialFrameSelection.modify(alter, verticalDistance, direction, NotUserTriggered);

        bool change = shouldChangeSelection(trialFrameSelection.selection());
        if (!change)
            return false;
    }

    willBeModified(alter, direction == DirectionUp ? DirectionBackward : DirectionForward);

    VisiblePosition pos;
    LayoutUnit xPos = 0;
    switch (alter) {
    case AlterationMove:
        pos = VisiblePosition(direction == DirectionUp ? m_selection.start() : m_selection.end(), m_selection.affinity());
        xPos = lineDirectionPointForBlockDirectionNavigation(direction == DirectionUp ? START : END);
        m_selection.setAffinity(direction == DirectionUp ? UPSTREAM : DOWNSTREAM);
        break;
    case AlterationExtend:
        pos = VisiblePosition(m_selection.extent(), m_selection.affinity());
        xPos = lineDirectionPointForBlockDirectionNavigation(EXTENT);
        m_selection.setAffinity(DOWNSTREAM);
        break;
    }

    int startY;
    if (!absoluteCaretY(pos, startY))
        return false;
    if (direction == DirectionUp)
        startY = -startY;
    int lastY = startY;

    VisiblePosition result;
    VisiblePosition next;
    for (VisiblePosition p = pos; ; p = next) {
        if (direction == DirectionUp)
            next = previousLinePosition(p, xPos);
        else
            next = nextLinePosition(p, xPos);

        if (next.isNull() || next == p)
            break;
        int nextY;
        if (!absoluteCaretY(next, nextY))
            break;
        if (direction == DirectionUp)
            nextY = -nextY;
        if (nextY - startY > static_cast<int>(verticalDistance))
            break;
        if (nextY >= lastY) {
            lastY = nextY;
            result = next;
        }
    }

    if (result.isNull())
        return false;

    switch (alter) {
    case AlterationMove:
        moveTo(result, userTriggered, align);
        break;
    case AlterationExtend:
        setExtent(result, userTriggered);
        break;
    }

    if (userTriggered == UserTriggered)
        m_granularity = CharacterGranularity;

    m_selection.setIsDirectional(shouldAlwaysUseDirectionalSelection(m_frame) || alter == AlterationExtend);

    return true;
}

LayoutUnit FrameSelection::lineDirectionPointForBlockDirectionNavigation(EPositionType type)
{
    LayoutUnit x = 0;

    if (isNone())
        return x;

    Position pos;
    switch (type) {
    case START:
        pos = m_selection.start();
        break;
    case END:
        pos = m_selection.end();
        break;
    case BASE:
        pos = m_selection.base();
        break;
    case EXTENT:
        pos = m_selection.extent();
        break;
    }

    Frame* frame = pos.anchorNode()->document().frame();
    if (!frame)
        return x;
        
    if (m_xPosForVerticalArrowNavigation == NoXPosForVerticalArrowNavigation()) {
        VisiblePosition visiblePosition(pos, m_selection.affinity());
        // VisiblePosition creation can fail here if a node containing the selection becomes visibility:hidden
        // after the selection is created and before this function is called.
        x = visiblePosition.isNotNull() ? visiblePosition.lineDirectionPointForBlockDirectionNavigation() : 0;
        m_xPosForVerticalArrowNavigation = x;
    } else
        x = m_xPosForVerticalArrowNavigation;
        
    return x;
}

void FrameSelection::clear()
{
    m_granularity = CharacterGranularity;
    setSelection(VisibleSelection());
}

void FrameSelection::prepareForDestruction()
{
    m_granularity = CharacterGranularity;

#if ENABLE(TEXT_CARET)
    m_caretBlinkTimer.stop();
#endif

    RenderView* view = m_frame->contentRenderer();
    if (view)
        view->clearSelection();

    setSelectionWithoutUpdatingAppearance(VisibleSelection(), defaultSetSelectionOptions(), AlignCursorOnScrollIfNeeded, CharacterGranularity);
    m_previousCaretNode.clear();
}

void FrameSelection::setStart(const VisiblePosition &pos, EUserTriggered trigger)
{
    if (m_selection.isBaseFirst())
        setBase(pos, trigger);
    else
        setExtent(pos, trigger);
}

void FrameSelection::setEnd(const VisiblePosition &pos, EUserTriggered trigger)
{
    if (m_selection.isBaseFirst())
        setExtent(pos, trigger);
    else
        setBase(pos, trigger);
}

void FrameSelection::setBase(const VisiblePosition &pos, EUserTriggered userTriggered)
{
    const bool selectionHasDirection = true;
    setSelection(VisibleSelection(pos.deepEquivalent(), m_selection.extent(), pos.affinity(), selectionHasDirection), defaultSetSelectionOptions(userTriggered));
}

void FrameSelection::setExtent(const VisiblePosition &pos, EUserTriggered userTriggered)
{
    const bool selectionHasDirection = true;
    setSelection(VisibleSelection(m_selection.base(), pos.deepEquivalent(), pos.affinity(), selectionHasDirection), defaultSetSelectionOptions(userTriggered));
}

void FrameSelection::setBase(const Position &pos, EAffinity affinity, EUserTriggered userTriggered)
{
    const bool selectionHasDirection = true;
    setSelection(VisibleSelection(pos, m_selection.extent(), affinity, selectionHasDirection), defaultSetSelectionOptions(userTriggered));
}

void FrameSelection::setExtent(const Position &pos, EAffinity affinity, EUserTriggered userTriggered)
{
    const bool selectionHasDirection = true;
    setSelection(VisibleSelection(m_selection.base(), pos, affinity, selectionHasDirection), defaultSetSelectionOptions(userTriggered));
}

void CaretBase::clearCaretRect()
{
    m_caretLocalRect = LayoutRect();
}

bool CaretBase::updateCaretRect(Document* document, const VisiblePosition& caretPosition)
{
    document->updateLayoutIgnorePendingStylesheets();
    m_caretRectNeedsUpdate = false;
    RenderObject* renderer;
    m_caretLocalRect = localCaretRectInRendererForCaretPainting(caretPosition, renderer);
    return !m_caretLocalRect.isEmpty();
}

RenderObject* FrameSelection::caretRendererWithoutUpdatingLayout() const
{
    return rendererForCaretPainting(m_selection.start().deprecatedNode());
}

RenderObject* DragCaretController::caretRenderer() const
{
    return rendererForCaretPainting(m_position.deepEquivalent().deprecatedNode());
}

static bool isNonOrphanedCaret(const VisibleSelection& selection)
{
    return selection.isCaret() && !selection.start().isOrphan() && !selection.end().isOrphan();
}

IntRect FrameSelection::absoluteCaretBounds()
{
    if (!m_frame)
        return IntRect();
    updateSelectionByUpdatingLayoutOrStyle(*m_frame);
    recomputeCaretRect();
    return m_absCaretBounds;
}

static void repaintCaretForLocalRect(Node* node, const LayoutRect& rect)
{
    RenderObject* caretPainter = rendererForCaretPainting(node);
    if (!caretPainter)
        return;

    caretPainter->repaintRectangle(rect);
}

bool FrameSelection::recomputeCaretRect()
{
    if (!shouldUpdateCaretRect())
        return false;

    if (!m_frame)
        return false;

    FrameView* v = m_frame->document()->view();
    if (!v)
        return false;

    LayoutRect oldRect = localCaretRectWithoutUpdate();

    RefPtr<Node> caretNode = m_previousCaretNode;
    if (shouldUpdateCaretRect()) {
        if (!isNonOrphanedCaret(m_selection))
            clearCaretRect();
        else {
            VisiblePosition visibleStart = m_selection.visibleStart();
            if (updateCaretRect(m_frame->document(), visibleStart)) {
                caretNode = visibleStart.deepEquivalent().deprecatedNode();
                m_absCaretBoundsDirty = true;
            }
        }
    }
    LayoutRect newRect = localCaretRectWithoutUpdate();

    if (caretNode == m_previousCaretNode && oldRect == newRect && !m_absCaretBoundsDirty)
        return false;

    IntRect oldAbsCaretBounds = m_absCaretBounds;
    m_absCaretBounds = absoluteBoundsForLocalCaretRect(rendererForCaretPainting(caretNode.get()), newRect);

    if (m_absCaretBoundsDirty && m_selection.isCaret()) // We should be able to always assert this condition.
        ASSERT(m_absCaretBounds == m_selection.visibleStart().absoluteCaretBounds());

    m_absCaretBoundsDirty = false;

    if (caretNode == m_previousCaretNode && oldAbsCaretBounds == m_absCaretBounds)
        return false;

#if ENABLE(TEXT_CARET)
    if (RenderView* view = m_frame->document()->renderView()) {
        bool previousOrNewCaretNodeIsContentEditable = m_selection.isContentEditable() || (m_previousCaretNode && m_previousCaretNode->isContentEditable());
        if (shouldRepaintCaret(view, previousOrNewCaretNodeIsContentEditable)) {
            if (m_previousCaretNode)
                repaintCaretForLocalRect(m_previousCaretNode.get(), oldRect);
            m_previousCaretNode = caretNode;
            repaintCaretForLocalRect(caretNode.get(), newRect);
        }
    }
#endif
    return true;
}

bool CaretBase::shouldRepaintCaret(const RenderView* view, bool isContentEditable) const
{
    ASSERT(view);
    Frame* frame = &view->frameView().frame(); // The frame where the selection started.
    bool caretBrowsing = frame && frame->settings().caretBrowsingEnabled();
    return (caretBrowsing || isContentEditable);
}

void FrameSelection::invalidateCaretRect()
{
    if (!isCaret())
        return;

    CaretBase::invalidateCaretRect(m_selection.start().deprecatedNode(), recomputeCaretRect());
}

void CaretBase::invalidateCaretRect(Node* node, bool caretRectChanged)
{
    // EDIT FIXME: This is an unfortunate hack.
    // Basically, we can't trust this layout position since we 
    // can't guarantee that the check to see if we are in unrendered 
    // content will work at this point. We may have to wait for
    // a layout and re-render of the document to happen. So, resetting this
    // flag will cause another caret layout to happen the first time
    // that we try to paint the caret after this call. That one will work since
    // it happens after the document has accounted for any editing
    // changes which may have been done.
    // And, we need to leave this layout here so the caret moves right 
    // away after clicking.
    m_caretRectNeedsUpdate = true;

    if (caretRectChanged)
        return;

    if (RenderView* view = node->document().renderView()) {
        if (shouldRepaintCaret(view, node->isContentEditable(Node::UserSelectAllIsAlwaysNonEditable)))
            repaintCaretForLocalRect(node, localCaretRectWithoutUpdate());
    }
}

void FrameSelection::paintCaret(GraphicsContext* context, const LayoutPoint& paintOffset, const LayoutRect& clipRect)
{
    if (m_selection.isCaret() && m_caretPaint)
        CaretBase::paintCaret(m_selection.start().deprecatedNode(), context, paintOffset, clipRect);
}

void CaretBase::paintCaret(Node* node, GraphicsContext* context, const LayoutPoint& paintOffset, const LayoutRect& clipRect) const
{
#if ENABLE(TEXT_CARET)
    if (m_caretVisibility == Hidden)
        return;

    LayoutRect drawingRect = localCaretRectWithoutUpdate();
    RenderObject* renderer = rendererForCaretPainting(node);
    if (renderer && renderer->isBox())
        toRenderBox(renderer)->flipForWritingMode(drawingRect);
    drawingRect.moveBy(roundedIntPoint(paintOffset));
    LayoutRect caret = intersection(drawingRect, clipRect);
    if (caret.isEmpty())
        return;

    Color caretColor = Color::black;
    ColorSpace colorSpace = ColorSpaceDeviceRGB;
    Element* element = node->isElementNode() ? toElement(node) : node->parentElement();
    Element* rootEditableElement = node->rootEditableElement();

    if (element && element->renderer()) {
        bool setToRootEditableElement = false;
        if (rootEditableElement && rootEditableElement->renderer()) {
            const auto& rootEditableStyle = rootEditableElement->renderer()->style();
            const auto& elementStyle = element->renderer()->style();
            if (rootEditableStyle.visitedDependentColor(CSSPropertyBackgroundColor) == elementStyle.visitedDependentColor(CSSPropertyBackgroundColor)) {
                caretColor = rootEditableElement->renderer()->style().visitedDependentColor(CSSPropertyColor);
                colorSpace = rootEditableElement->renderer()->style().colorSpace();
                setToRootEditableElement = true;
            }
        }
        if (!setToRootEditableElement) {
            caretColor = element->renderer()->style().visitedDependentColor(CSSPropertyColor);
            colorSpace = element->renderer()->style().colorSpace();
        }
    }

    context->fillRect(caret, caretColor, colorSpace);
#else
    UNUSED_PARAM(node);
    UNUSED_PARAM(context);
    UNUSED_PARAM(paintOffset);
    UNUSED_PARAM(clipRect);
#endif
}

void FrameSelection::debugRenderer(RenderObject* r, bool selected) const
{
    if (r->node()->isElementNode()) {
        Element* element = toElement(r->node());
        fprintf(stderr, "%s%s\n", selected ? "==> " : "    ", element->localName().string().utf8().data());
    } else if (r->isText()) {
        RenderText* textRenderer = toRenderText(r);
        if (!textRenderer->textLength() || !textRenderer->firstTextBox()) {
            fprintf(stderr, "%s#text (empty)\n", selected ? "==> " : "    ");
            return;
        }
        
        static const int max = 36;
        String text = textRenderer->text();
        int textLength = text.length();
        if (selected) {
            int offset = 0;
            if (r->node() == m_selection.start().containerNode())
                offset = m_selection.start().computeOffsetInContainerNode();
            else if (r->node() == m_selection.end().containerNode())
                offset = m_selection.end().computeOffsetInContainerNode();

            int pos;
            InlineTextBox* box = textRenderer->findNextInlineTextBox(offset, pos);
            text = text.substring(box->start(), box->len());
            
            String show;
            int mid = max / 2;
            int caret = 0;
            
            // text is shorter than max
            if (textLength < max) {
                show = text;
                caret = pos;
            } else if (pos - mid < 0) {
                // too few characters to left
                show = text.left(max - 3) + "...";
                caret = pos;
            } else if (pos - mid >= 0 && pos + mid <= textLength) {
                // enough characters on each side
                show = "..." + text.substring(pos - mid + 3, max - 6) + "...";
                caret = mid;
            } else {
                // too few characters on right
                show = "..." + text.right(max - 3);
                caret = pos - (textLength - show.length());
            }
            
            show.replace('\n', ' ');
            show.replace('\r', ' ');
            fprintf(stderr, "==> #text : \"%s\" at offset %d\n", show.utf8().data(), pos);
            fprintf(stderr, "           ");
            for (int i = 0; i < caret; i++)
                fprintf(stderr, " ");
            fprintf(stderr, "^\n");
        } else {
            if ((int)text.length() > max)
                text = text.left(max - 3) + "...";
            else
                text = text.left(max);
            fprintf(stderr, "    #text : \"%s\"\n", text.utf8().data());
        }
    }
}

bool FrameSelection::contains(const LayoutPoint& point)
{
    Document* document = m_frame->document();
    
    // Treat a collapsed selection like no selection.
    if (!isRange())
        return false;
    if (!document->renderView()) 
        return false;
    
    HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::Active | HitTestRequest::DisallowShadowContent);
    HitTestResult result(point);
    document->renderView()->hitTest(request, result);
    Node* innerNode = result.innerNode();
    if (!innerNode || !innerNode->renderer())
        return false;
    
    VisiblePosition visiblePos(innerNode->renderer()->positionForPoint(result.localPoint(), nullptr));
    if (visiblePos.isNull())
        return false;
        
    if (m_selection.visibleStart().isNull() || m_selection.visibleEnd().isNull())
        return false;
        
    Position start(m_selection.visibleStart().deepEquivalent());
    Position end(m_selection.visibleEnd().deepEquivalent());
    Position p(visiblePos.deepEquivalent());

    return comparePositions(start, p) <= 0 && comparePositions(p, end) <= 0;
}

// Workaround for the fact that it's hard to delete a frame.
// Call this after doing user-triggered selections to make it easy to delete the frame you entirely selected.
// Can't do this implicitly as part of every setSelection call because in some contexts it might not be good
// for the focus to move to another frame. So instead we call it from places where we are selecting with the
// mouse or the keyboard after setting the selection.
void FrameSelection::selectFrameElementInParentIfFullySelected()
{
    // Find the parent frame; if there is none, then we have nothing to do.
    Frame* parent = m_frame->tree().parent();
    if (!parent)
        return;
    Page* page = m_frame->page();
    if (!page)
        return;

    // Check if the selection contains the entire frame contents; if not, then there is nothing to do.
    if (!isRange())
        return;
    if (!isStartOfDocument(selection().visibleStart()))
        return;
    if (!isEndOfDocument(selection().visibleEnd()))
        return;

    // Get to the <iframe> or <frame> (or even <object>) element in the parent frame.
    Element* ownerElement = m_frame->ownerElement();
    if (!ownerElement)
        return;
    ContainerNode* ownerElementParent = ownerElement->parentNode();
    if (!ownerElementParent)
        return;
        
    // This method's purpose is it to make it easier to select iframes (in order to delete them).  Don't do anything if the iframe isn't deletable.
    if (!ownerElementParent->hasEditableStyle())
        return;

    // Create compute positions before and after the element.
    unsigned ownerElementNodeIndex = ownerElement->nodeIndex();
    VisiblePosition beforeOwnerElement(VisiblePosition(Position(ownerElementParent, ownerElementNodeIndex, Position::PositionIsOffsetInAnchor)));
    VisiblePosition afterOwnerElement(VisiblePosition(Position(ownerElementParent, ownerElementNodeIndex + 1, Position::PositionIsOffsetInAnchor), VP_UPSTREAM_IF_POSSIBLE));

    // Focus on the parent frame, and then select from before this element to after.
    VisibleSelection newSelection(beforeOwnerElement, afterOwnerElement);
    if (parent->selection().shouldChangeSelection(newSelection)) {
        page->focusController().setFocusedFrame(parent);
        parent->selection().setSelection(newSelection);
    }
}

void FrameSelection::selectAll()
{
    Document* document = m_frame->document();

    Element* focusedElement = document->focusedElement();
    if (focusedElement && focusedElement->hasTagName(selectTag)) {
        HTMLSelectElement* selectElement = toHTMLSelectElement(document->focusedElement());
        if (selectElement->canSelectAll()) {
            selectElement->selectAll();
            return;
        }
    }

    RefPtr<Node> root = 0;
    Node* selectStartTarget = 0;
    if (m_selection.isContentEditable()) {
        root = highestEditableRoot(m_selection.start());
        if (Node* shadowRoot = m_selection.nonBoundaryShadowTreeRootNode())
            selectStartTarget = shadowRoot->shadowHost();
        else
            selectStartTarget = root.get();
    } else {
        if (m_selection.isNone() && focusedElement) {
            if (focusedElement->isTextFormControl()) {
                toHTMLTextFormControlElement(focusedElement)->select();
                return;
            }
            root = focusedElement->nonBoundaryShadowTreeRootNode();
        } else
            root = m_selection.nonBoundaryShadowTreeRootNode();

        if (root)
            selectStartTarget = root->shadowHost();
        else {
            root = document->documentElement();
            selectStartTarget = document->body();
        }
    }
    if (!root)
        return;

    if (selectStartTarget && !selectStartTarget->dispatchEvent(Event::create(eventNames().selectstartEvent, true, true)))
        return;

    VisibleSelection newSelection(VisibleSelection::selectionFromContentsOfNode(root.get()));

    if (shouldChangeSelection(newSelection))
        setSelection(newSelection, defaultSetSelectionOptions() | FireSelectEvent);
}

bool FrameSelection::setSelectedRange(Range* range, EAffinity affinity, bool closeTyping)
{
    if (!range || !range->startContainer() || !range->endContainer())
        return false;
    ASSERT(&range->startContainer()->document() == &range->endContainer()->document());

    VisibleSelection newSelection(range, affinity);

#if PLATFORM(IOS)
    // FIXME: Why do we need this check only in iOS?
    if (range->startContainer() && range->endContainer() && newSelection.isNone())
        return false;
#endif

    setSelection(newSelection, ClearTypingStyle | (closeTyping ? CloseTyping : 0));
    return true;
}

void FrameSelection::focusedOrActiveStateChanged()
{
    bool activeAndFocused = isFocusedAndActive();
    Ref<Document> document(*m_frame->document());

    document->updateStyleIfNeeded();

#if USE(UIKIT_EDITING)
    // Caret blinking (blinks | does not blink)
    if (activeAndFocused)
        setSelectionFromNone();
    setCaretVisible(activeAndFocused);
#else
    // Because RenderObject::selectionBackgroundColor() and
    // RenderObject::selectionForegroundColor() check if the frame is active,
    // we have to update places those colors were painted.
    if (RenderView* view = document->renderView())
        view->repaintSelection();

    // Caret appears in the active frame.
    if (activeAndFocused)
        setSelectionFromNone();
    setCaretVisibility(activeAndFocused ? Visible : Hidden);

    // Update for caps lock state
    m_frame->eventHandler().capsLockStateMayHaveChanged();

    // Because StyleResolver::checkOneSelector() and
    // RenderTheme::isFocused() check if the frame is active, we have to
    // update style and theme state that depended on those.
    if (Element* element = document->focusedElement()) {
        element->setNeedsStyleRecalc();
        if (RenderObject* renderer = element->renderer())
            if (renderer && renderer->style().hasAppearance())
                renderer->theme().stateChanged(*renderer, ControlStates::FocusState);
    }
#endif
}

void FrameSelection::pageActivationChanged()
{
    focusedOrActiveStateChanged();
}

void FrameSelection::setFocused(bool flag)
{
    if (m_focused == flag)
        return;
    m_focused = flag;

    focusedOrActiveStateChanged();
}

bool FrameSelection::isFocusedAndActive() const
{
    return m_focused && m_frame->page() && m_frame->page()->focusController().isActive();
}

#if ENABLE(TEXT_CARET)
inline static bool shouldStopBlinkingDueToTypingCommand(Frame* frame)
{
    return frame->editor().lastEditCommand() && frame->editor().lastEditCommand()->shouldStopCaretBlinking();
}
#endif

void FrameSelection::updateAppearance()
{
#if PLATFORM(IOS)
    if (!m_updateAppearanceEnabled)
        return;
#endif

    // Paint a block cursor instead of a caret in overtype mode unless the caret is at the end of a line (in this case
    // the FrameSelection will paint a blinking caret as usual).
    VisibleSelection oldSelection = selection();
    VisiblePosition forwardPosition;
    if (m_shouldShowBlockCursor && oldSelection.isCaret()) {
        forwardPosition = modifyExtendingForward(CharacterGranularity);
        m_caretPaint = forwardPosition.isNull();
    }

#if ENABLE(TEXT_CARET)
    bool caretRectChangedOrCleared = recomputeCaretRect();

    bool caretBrowsing = m_frame->settings().caretBrowsingEnabled();
    bool shouldBlink = caretIsVisible() && isCaret() && (oldSelection.isContentEditable() || caretBrowsing) && forwardPosition.isNull();

    // If the caret moved, stop the blink timer so we can restart with a
    // black caret in the new location.
    if (caretRectChangedOrCleared || !shouldBlink || shouldStopBlinkingDueToTypingCommand(m_frame))
        m_caretBlinkTimer.stop();

    // Start blinking with a black caret. Be sure not to restart if we're
    // already blinking in the right location.
    if (shouldBlink && !m_caretBlinkTimer.isActive()) {
        if (double blinkInterval = m_frame->page()->theme().caretBlinkInterval())
            m_caretBlinkTimer.startRepeating(blinkInterval);

        if (!m_caretPaint) {
            m_caretPaint = true;
            invalidateCaretRect();
        }
    }
#endif

    RenderView* view = m_frame->contentRenderer();
    if (!view)
        return;

    // Construct a new VisibleSolution, since m_selection is not necessarily valid, and the following steps
    // assume a valid selection. See <https://bugs.webkit.org/show_bug.cgi?id=69563> and <rdar://problem/10232866>.
    VisibleSelection selection(oldSelection.visibleStart(), forwardPosition.isNotNull() ? forwardPosition : oldSelection.visibleEnd());

    if (!selection.isRange()) {
        view->clearSelection();
        return;
    }

    // Use the rightmost candidate for the start of the selection, and the leftmost candidate for the end of the selection.
    // Example: foo <a>bar</a>.  Imagine that a line wrap occurs after 'foo', and that 'bar' is selected.   If we pass [foo, 3]
    // as the start of the selection, the selection painting code will think that content on the line containing 'foo' is selected
    // and will fill the gap before 'bar'.
    Position startPos = selection.start();
    Position candidate = startPos.downstream();
    if (candidate.isCandidate())
        startPos = candidate;
    Position endPos = selection.end();
    candidate = endPos.upstream();
    if (candidate.isCandidate())
        endPos = candidate;

    // We can get into a state where the selection endpoints map to the same VisiblePosition when a selection is deleted
    // because we don't yet notify the FrameSelection of text removal.
    if (startPos.isNotNull() && endPos.isNotNull() && selection.visibleStart() != selection.visibleEnd()) {
        RenderObject* startRenderer = startPos.deprecatedNode()->renderer();
        RenderObject* endRenderer = endPos.deprecatedNode()->renderer();
        view->setSelection(startRenderer, startPos.deprecatedEditingOffset(), endRenderer, endPos.deprecatedEditingOffset());
    }
}

void FrameSelection::setCaretVisibility(CaretVisibility visibility)
{
    if (caretVisibility() == visibility)
        return;

    // FIXME: We shouldn't trigger a synchrnously layout here.
    if (m_frame)
        updateSelectionByUpdatingLayoutOrStyle(*m_frame);

#if ENABLE(TEXT_CARET)
    if (m_caretPaint) {
        m_caretPaint = false;
        invalidateCaretRect();
    }
    CaretBase::setCaretVisibility(visibility);
#endif

    updateAppearance();
}

void FrameSelection::caretBlinkTimerFired(Timer<FrameSelection>&)
{
#if ENABLE(TEXT_CARET)
    ASSERT(caretIsVisible());
    ASSERT(isCaret());
    bool caretPaint = m_caretPaint;
    if (isCaretBlinkingSuspended() && caretPaint)
        return;
    m_caretPaint = !caretPaint;
    invalidateCaretRect();
#endif
}

// Helper function that tells whether a particular node is an element that has an entire
// Frame and FrameView, a <frame>, <iframe>, or <object>.
static bool isFrameElement(const Node* n)
{
    if (!n)
        return false;
    RenderObject* renderer = n->renderer();
    if (!renderer || !renderer->isWidget())
        return false;
    Widget* widget = toRenderWidget(renderer)->widget();
    return widget && widget->isFrameView();
}

void FrameSelection::setFocusedElementIfNeeded()
{
    if (isNone() || !isFocused())
        return;

    bool caretBrowsing = m_frame->settings().caretBrowsingEnabled();
    if (caretBrowsing) {
        if (Element* anchor = enclosingAnchorElement(m_selection.base())) {
            m_frame->page()->focusController().setFocusedElement(anchor, m_frame);
            return;
        }
    }

    if (Element* target = m_selection.rootEditableElement()) {
        // Walk up the DOM tree to search for an element to focus.
        while (target) {
            // We don't want to set focus on a subframe when selecting in a parent frame,
            // so add the !isFrameElement check here. There's probably a better way to make this
            // work in the long term, but this is the safest fix at this time.
            if (target->isMouseFocusable() && !isFrameElement(target)) {
                m_frame->page()->focusController().setFocusedElement(target, m_frame);
                return;
            }
            target = target->parentOrShadowHostElement();
        }
        m_frame->document()->setFocusedElement(0);
    }

    if (caretBrowsing)
        m_frame->page()->focusController().setFocusedElement(0, m_frame);
}

void DragCaretController::paintDragCaret(Frame* frame, GraphicsContext* p, const LayoutPoint& paintOffset, const LayoutRect& clipRect) const
{
#if ENABLE(TEXT_CARET)
    if (m_position.deepEquivalent().deprecatedNode()->document().frame() == frame)
        paintCaret(m_position.deepEquivalent().deprecatedNode(), p, paintOffset, clipRect);
#else
    UNUSED_PARAM(frame);
    UNUSED_PARAM(p);
    UNUSED_PARAM(paintOffset);
    UNUSED_PARAM(clipRect);
#endif
}

PassRefPtr<MutableStyleProperties> FrameSelection::copyTypingStyle() const
{
    if (!m_typingStyle || !m_typingStyle->style())
        return 0;
    return m_typingStyle->style()->mutableCopy();
}

bool FrameSelection::shouldDeleteSelection(const VisibleSelection& selection) const
{
#if PLATFORM(IOS)
    if (m_frame->selectionChangeCallbacksDisabled())
        return true;
#endif
    return m_frame->editor().client()->shouldDeleteRange(selection.toNormalizedRange().get());
}

FloatRect FrameSelection::selectionBounds(bool clipToVisibleContent) const
{
    if (!m_frame->document())
        return LayoutRect();

    updateSelectionByUpdatingLayoutOrStyle(*m_frame);
    RenderView* root = m_frame->contentRenderer();
    FrameView* view = m_frame->view();
    if (!root || !view)
        return LayoutRect();

    LayoutRect selectionRect = root->selectionBounds(clipToVisibleContent);
    return clipToVisibleContent ? intersection(selectionRect, view->visibleContentRect(ScrollableArea::LegacyIOSDocumentVisibleRect)) : selectionRect;
}

void FrameSelection::getClippedVisibleTextRectangles(Vector<FloatRect>& rectangles) const
{
    RenderView* root = m_frame->contentRenderer();
    if (!root)
        return;

    FloatRect visibleContentRect = m_frame->view()->visibleContentRect(ScrollableArea::LegacyIOSDocumentVisibleRect);

    Vector<FloatQuad> quads;
    toNormalizedRange()->textQuads(quads, true);

    size_t size = quads.size();
    for (size_t i = 0; i < size; ++i) {
        FloatRect intersectionRect = intersection(quads[i].enclosingBoundingBox(), visibleContentRect);
        if (!intersectionRect.isEmpty())
            rectangles.append(intersectionRect);
    }
}

// Scans logically forward from "start", including any child frames.
static HTMLFormElement* scanForForm(Element* start)
{
    if (!start)
        return nullptr;

    auto descendants = descendantsOfType<HTMLElement>(start->document());
    for (auto it = descendants.from(*start), end = descendants.end(); it != end; ++it) {
        HTMLElement& element = *it;
        if (isHTMLFormElement(&element))
            return toHTMLFormElement(&element);
        if (isHTMLFormControlElement(element))
            return toHTMLFormControlElement(element).form();
        if (isHTMLFrameElementBase(element)) {
            Document* contentDocument = toHTMLFrameElementBase(element).contentDocument();
            if (!contentDocument)
                continue;
            if (HTMLFormElement* frameResult = scanForForm(contentDocument->documentElement()))
                return frameResult;
        }
    }
    return nullptr;
}

// We look for either the form containing the current focus, or for one immediately after it
HTMLFormElement* FrameSelection::currentForm() const
{
    // Start looking either at the active (first responder) node, or where the selection is.
    Element* start = m_frame->document()->focusedElement();
    if (!start)
        start = m_selection.start().element();
    if (!start)
        return nullptr;

    if (auto form = lineageOfType<HTMLFormElement>(*start).first())
        return form;
    if (auto formControl = lineageOfType<HTMLFormControlElement>(*start).first())
        return formControl->form();

    // Try walking forward in the node tree to find a form element.
    return scanForForm(start);
}

void FrameSelection::revealSelection(const ScrollAlignment& alignment, RevealExtentOption revealExtentOption)
{
    LayoutRect rect;

    switch (m_selection.selectionType()) {
    case VisibleSelection::NoSelection:
        return;
    case VisibleSelection::CaretSelection:
        rect = absoluteCaretBounds();
        break;
    case VisibleSelection::RangeSelection:
        rect = revealExtentOption == RevealExtent ? VisiblePosition(m_selection.extent()).absoluteCaretBounds() : enclosingIntRect(selectionBounds(false));
        break;
    }

    Position start = m_selection.start();
    ASSERT(start.deprecatedNode());
    if (start.deprecatedNode() && start.deprecatedNode()->renderer()) {
#if PLATFORM(IOS)
        if (RenderLayer* layer = start.deprecatedNode()->renderer()->enclosingLayer()) {
            if (!m_scrollingSuppressCount) {
                layer->setAdjustForIOSCaretWhenScrolling(true);
                layer->scrollRectToVisible(rect, alignment, alignment);
                layer->setAdjustForIOSCaretWhenScrolling(false);
                updateAppearance();
                if (m_frame->page())
                    m_frame->page()->chrome().client().notifyRevealedSelectionByScrollingFrame(m_frame);
            }
        }
#else
        // FIXME: This code only handles scrolling the startContainer's layer, but
        // the selection rect could intersect more than just that.
        // See <rdar://problem/4799899>.
        if (start.deprecatedNode()->renderer()->scrollRectToVisible(rect, alignment, alignment))
            updateAppearance();
#endif
    }
}

void FrameSelection::setSelectionFromNone()
{
    // Put a caret inside the body if the entire frame is editable (either the
    // entire WebView is editable or designMode is on for this document).

    Document* document = m_frame->document();
#if !PLATFORM(IOS)
    bool caretBrowsing = m_frame->settings().caretBrowsingEnabled();
    if (!isNone() || !(document->hasEditableStyle() || caretBrowsing))
        return;
#else
    if (!document || !(isNone() || isStartOfDocument(VisiblePosition(m_selection.start(), m_selection.affinity()))) || !document->hasEditableStyle())
        return;
#endif

    Node* node = document->documentElement();
    while (node && !node->hasTagName(bodyTag))
        node = NodeTraversal::next(node);
    if (node)
        setSelection(VisibleSelection(firstPositionInOrBeforeNode(node), DOWNSTREAM));
}

bool FrameSelection::shouldChangeSelection(const VisibleSelection& newSelection) const
{
#if PLATFORM(IOS)
    if (m_frame->selectionChangeCallbacksDisabled())
        return true;
#endif
    return m_frame->editor().shouldChangeSelection(selection(), newSelection, newSelection.affinity(), false);
}

bool FrameSelection::dispatchSelectStart()
{
    Node* selectStartTarget = m_selection.extent().containerNode();
    if (!selectStartTarget)
        return true;

    return selectStartTarget->dispatchEvent(Event::create(eventNames().selectstartEvent, true, true));
}

void FrameSelection::setShouldShowBlockCursor(bool shouldShowBlockCursor)
{
    m_shouldShowBlockCursor = shouldShowBlockCursor;

    m_frame->document()->updateLayoutIgnorePendingStylesheets();

    updateAppearance();
}

void FrameSelection::didLayout()
{
    setCaretRectNeedsUpdate();
    updateAndRevealSelection();
    updateDataDetectorsForSelection();
}

#ifndef NDEBUG

void FrameSelection::formatForDebugger(char* buffer, unsigned length) const
{
    m_selection.formatForDebugger(buffer, length);
}

void FrameSelection::showTreeForThis() const
{
    m_selection.showTreeForThis();
}

#endif

#if PLATFORM(IOS)
void FrameSelection::expandSelectionToElementContainingCaretSelection()
{
    RefPtr<Range> range = elementRangeContainingCaretSelection();
    if (!range)
        return;
    VisibleSelection selection(range.get(), DOWNSTREAM);
    setSelection(selection);
}

PassRefPtr<Range> FrameSelection::elementRangeContainingCaretSelection() const
{
    if (m_selection.isNone())
        return nullptr;

    VisibleSelection selection = m_selection;
    if (selection.isNone())
        return nullptr;

    VisiblePosition visiblePos(selection.start(), VP_DEFAULT_AFFINITY);
    if (visiblePos.isNull())
        return nullptr;

    Node* node = visiblePos.deepEquivalent().deprecatedNode();
    Element* element = deprecatedEnclosingBlockFlowElement(node);
    if (!element)
        return nullptr;

    Position startPos = createLegacyEditingPosition(element, 0);
    Position endPos = createLegacyEditingPosition(element, element->childNodeCount());
    
    VisiblePosition startVisiblePos(startPos, VP_DEFAULT_AFFINITY);
    VisiblePosition endVisiblePos(endPos, VP_DEFAULT_AFFINITY);
    if (startVisiblePos.isNull() || endVisiblePos.isNull())
        return nullptr;

    selection.setBase(startVisiblePos);
    selection.setExtent(endVisiblePos);

    return selection.toNormalizedRange();
}

void FrameSelection::expandSelectionToWordContainingCaretSelection()
{
    VisibleSelection selection(wordSelectionContainingCaretSelection(m_selection));        
    if (selection.isCaretOrRange())
        setSelection(selection);
}

PassRefPtr<Range> FrameSelection::wordRangeContainingCaretSelection()
{
    return wordSelectionContainingCaretSelection(m_selection).toNormalizedRange();
}

void FrameSelection::expandSelectionToStartOfWordContainingCaretSelection()
{
    if (m_selection.isNone() || isStartOfDocument(m_selection.start()))
        return;

    VisiblePosition s1(m_selection.start());
    VisiblePosition e1(m_selection.end());

    VisibleSelection expanded(wordSelectionContainingCaretSelection(m_selection));
    VisiblePosition s2(expanded.start());

    // Don't allow the start to become greater after the expansion.
    if (s2.isNull() || s2 > s1)
        s2 = s1;

    moveTo(s2, e1);
}

UChar FrameSelection::characterInRelationToCaretSelection(int amount) const
{
    if (m_selection.isNone())
        return 0;

    VisibleSelection selection = m_selection;
    ASSERT(selection.isCaretOrRange());

    VisiblePosition visiblePosition(selection.start(), VP_DEFAULT_AFFINITY);

    if (amount < 0) {
        int count = abs(amount);
        for (int i = 0; i < count; i++)
            visiblePosition = visiblePosition.previous();    
        return visiblePosition.characterBefore();
    }
    for (int i = 0; i < amount; i++)
        visiblePosition = visiblePosition.next();    
    return visiblePosition.characterAfter();
}

UChar FrameSelection::characterBeforeCaretSelection() const
{
    if (m_selection.isNone())
        return 0;

    VisibleSelection selection = m_selection;
    ASSERT(selection.isCaretOrRange());

    VisiblePosition visiblePosition(selection.start(), VP_DEFAULT_AFFINITY);
    return visiblePosition.characterBefore();
}

UChar FrameSelection::characterAfterCaretSelection() const
{
    if (m_selection.isNone())
        return 0;

    VisibleSelection selection = m_selection;
    ASSERT(selection.isCaretOrRange());

    VisiblePosition visiblePosition(selection.end(), VP_DEFAULT_AFFINITY);
    return visiblePosition.characterAfter();
}

int FrameSelection::wordOffsetInRange(const Range *range) const
{
    if (!range)
        return -1;

    VisibleSelection selection = m_selection;
    if (!selection.isCaret())
        return -1;

    // FIXME: This will only work in cases where the selection remains in
    // the same node after it is expanded. Improve to handle more complicated
    // cases.
    ExceptionCode ec = 0;
    int result = selection.start().deprecatedEditingOffset() - range->startOffset(ec);
    ASSERT(!ec);
    if (result < 0)
        result = 0;
    return result;
}

bool FrameSelection::spaceFollowsWordInRange(const Range *range) const
{
    if (!range)
        return false;
    ExceptionCode ec = 0;
    Node* node = range->endContainer(ec);
    ASSERT(!ec);
    int endOffset = range->endOffset(ec);
    ASSERT(!ec);
    VisiblePosition pos(createLegacyEditingPosition(node, endOffset), VP_DEFAULT_AFFINITY);
    return isSpaceOrNewline(pos.characterAfter());
}

bool FrameSelection::selectionAtDocumentStart() const
{
    VisibleSelection selection = m_selection;
    if (selection.isNone())
        return false;

    Position startPos(selection.start());
    VisiblePosition pos(createLegacyEditingPosition(startPos.deprecatedNode(), startPos.deprecatedEditingOffset()), VP_DEFAULT_AFFINITY);
    if (pos.isNull())
        return false;

    return isStartOfDocument(pos);
}

bool FrameSelection::selectionAtSentenceStart() const
{
    VisibleSelection selection = m_selection;
    if (selection.isNone())
        return false;

    return actualSelectionAtSentenceStart(selection);
}

bool FrameSelection::selectionAtWordStart() const
{
    VisibleSelection selection = m_selection;
    if (selection.isNone())
        return false;

    Position startPos(selection.start());
    VisiblePosition pos(createLegacyEditingPosition(startPos.deprecatedNode(), startPos.deprecatedEditingOffset()), VP_DEFAULT_AFFINITY);
    if (pos.isNull())
        return false;

    if (isStartOfParagraph(pos))
        return true;
        
    bool result = true;
    unsigned previousCount = 0;
    for (pos = pos.previous(); !pos.isNull(); pos = pos.previous()) {
        previousCount++;
        if (isStartOfParagraph(pos)) {
            if (previousCount == 1)
                result = false;
            break;
        }
        UChar c(pos.characterAfter());
        if (c) {
            result = isSpaceOrNewline(c) || c == 0xA0 || (u_ispunct(c) && c != ',' && c != '-' && c != '\'');
            break;
        }
    }

    return result;
}

PassRefPtr<Range> FrameSelection::rangeByMovingCurrentSelection(int amount) const
{
    return rangeByAlteringCurrentSelection(AlterationMove, amount);
}

PassRefPtr<Range> FrameSelection::rangeByExtendingCurrentSelection(int amount) const
{
    return rangeByAlteringCurrentSelection(AlterationExtend, amount);
}

void FrameSelection::selectRangeOnElement(unsigned location, unsigned length, Node* node)
{
    RefPtr<Range> resultRange = m_frame->document()->createRange();
    ExceptionCode ec = 0;
    resultRange->setStart(node, location, ec);
    ASSERT(!ec);
    resultRange->setEnd(node, location + length, ec);
    ASSERT(!ec);
    VisibleSelection selection = VisibleSelection(resultRange.get(), SEL_DEFAULT_AFFINITY);
    setSelection(selection, true);
}

VisibleSelection FrameSelection::wordSelectionContainingCaretSelection(const VisibleSelection& selection)
{
    if (selection.isNone())
        return VisibleSelection();

    ASSERT(selection.isCaretOrRange());
    FrameSelection frameSelection;
    frameSelection.setSelection(selection);

    Position startPosBeforeExpansion(selection.start());
    Position endPosBeforeExpansion(selection.end());
    VisiblePosition startVisiblePosBeforeExpansion(startPosBeforeExpansion, VP_DEFAULT_AFFINITY);
    VisiblePosition endVisiblePosBeforeExpansion(endPosBeforeExpansion, VP_DEFAULT_AFFINITY);
    if (endVisiblePosBeforeExpansion.isNull())
        return VisibleSelection();

    if (isEndOfParagraph(endVisiblePosBeforeExpansion)) {
        UChar c(endVisiblePosBeforeExpansion.characterBefore());
        if (isSpaceOrNewline(c) || c == 0xA0) {
            // End of paragraph with space.
            return VisibleSelection();
        }
    }

    // If at end of paragraph, move backwards one character.
    // This has the effect of selecting the word on the line (which is
    // what we want, rather than selecting past the end of the line).
    if (isEndOfParagraph(endVisiblePosBeforeExpansion) && !isStartOfParagraph(endVisiblePosBeforeExpansion))
        frameSelection.modify(FrameSelection::AlterationMove, DirectionBackward, CharacterGranularity);

    VisibleSelection newSelection = frameSelection.selection();
    newSelection.expandUsingGranularity(WordGranularity);
    frameSelection.setSelection(newSelection, defaultSetSelectionOptions(), AlignCursorOnScrollIfNeeded, frameSelection.granularity());

    Position startPos(frameSelection.selection().start());
    Position endPos(frameSelection.selection().end());

    // Expansion cannot be allowed to change selection so that it is no longer
    // touches (or contains) the original, unexpanded selection.
    // Enforce this on the way into these additional calculations to give them
    // the best chance to yield a suitable answer.
    if (startPos > startPosBeforeExpansion)
        startPos = startPosBeforeExpansion;
    if (endPos < endPosBeforeExpansion)
        endPos = endPosBeforeExpansion;

    VisiblePosition startVisiblePos(startPos, VP_DEFAULT_AFFINITY);
    VisiblePosition endVisiblePos(endPos, VP_DEFAULT_AFFINITY);

    if (startVisiblePos.isNull() || endVisiblePos.isNull()) {
        // Start or end is nil
        return VisibleSelection();
    }

    if (isEndOfLine(endVisiblePosBeforeExpansion)) {
        VisiblePosition previous(endVisiblePos.previous());
        if (previous == endVisiblePos) {
            // Empty document
            return VisibleSelection();
        }
        UChar c(previous.characterAfter());
        if (isSpaceOrNewline(c) || c == 0xA0) {
            // Space at end of line
            return VisibleSelection();
        }
    }

    // Expansion has selected past end of line.
    // Try repositioning backwards.
    if (isEndOfLine(startVisiblePos) && isStartOfLine(endVisiblePos)) {
        VisiblePosition previous(startVisiblePos.previous());
        if (isEndOfLine(previous)) {
            // On empty line
            return VisibleSelection();
        }
        UChar c(previous.characterAfter());
        if (isSpaceOrNewline(c) || c == 0xA0) {
            // Space at end of line
            return VisibleSelection();
        }
        frameSelection.moveTo(startVisiblePos);
        frameSelection.modify(FrameSelection::AlterationExtend, DirectionBackward, WordGranularity);
        startPos = frameSelection.selection().start();
        endPos = frameSelection.selection().end();
        startVisiblePos = VisiblePosition(startPos, VP_DEFAULT_AFFINITY);
        endVisiblePos = VisiblePosition(endPos, VP_DEFAULT_AFFINITY);
        if (startVisiblePos.isNull() || endVisiblePos.isNull()) {
            // Start or end is nil
            return VisibleSelection();
        }
    }

    // Now loop backwards until we find a non-space.
    while (endVisiblePos != startVisiblePos) {
        VisiblePosition previous(endVisiblePos.previous());
        UChar c(previous.characterAfter());
        if (!isSpaceOrNewline(c) && c != 0xA0)
            break;
        endVisiblePos = previous;
    }

    // Expansion cannot be allowed to change selection so that it is no longer
    // touches (or contains) the original, unexpanded selection.
    // Enforce this on the way out of the function to preserve the invariant.
    if (startVisiblePos > startVisiblePosBeforeExpansion)
        startVisiblePos = startVisiblePosBeforeExpansion;
    if (endVisiblePos < endVisiblePosBeforeExpansion)
        endVisiblePos = endVisiblePosBeforeExpansion;

    return VisibleSelection(startVisiblePos, endVisiblePos);    
}

bool FrameSelection::actualSelectionAtSentenceStart(const VisibleSelection& sel) const
{
    Position startPos(sel.start());
    VisiblePosition pos(createLegacyEditingPosition(startPos.deprecatedNode(), startPos.deprecatedEditingOffset()), VP_DEFAULT_AFFINITY);
    if (pos.isNull())
        return false;

    if (isStartOfParagraph(pos))
        return true;
 
    bool result = true;
    bool sawSpace = false;
    unsigned previousCount = 0;
    for (pos = pos.previous(); !pos.isNull(); pos = pos.previous()) {
        previousCount++;
        if (isStartOfParagraph(pos)) {
            if (previousCount == 1 || (previousCount == 2 && sawSpace))
                result = false;
            break;
        }
        UChar c(pos.characterAfter());
        if (c) {
            if (isSpaceOrNewline(c) || c == 0xA0) {
                sawSpace = true;
            }
            else {
                result = (c == '.' || c == '!' || c == '?');
                break;
            }
        }
    }
    
    return result;
}

PassRefPtr<Range> FrameSelection::rangeByAlteringCurrentSelection(EAlteration alteration, int amount) const
{
    if (m_selection.isNone())
        return nullptr;

    if (!amount)
        return toNormalizedRange();

    FrameSelection frameSelection;
    frameSelection.setSelection(m_selection);
    SelectionDirection direction = amount > 0 ? DirectionForward : DirectionBackward;
    for (int i = 0; i < abs(amount); i++)
        frameSelection.modify(alteration, direction, CharacterGranularity);
    return frameSelection.toNormalizedRange();
}

void FrameSelection::clearCurrentSelection()
{
    setSelection(VisibleSelection());
}

void FrameSelection::setCaretBlinks(bool caretBlinks)
{
    if (m_caretBlinks == caretBlinks)
        return;
#if ENABLE(TEXT_CARET)
    m_frame->document()->updateLayoutIgnorePendingStylesheets(); 
    if (m_caretPaint) {
        m_caretPaint = false; 
        invalidateCaretRect(); 
    }
#endif
    if (caretBlinks)
        setFocusedElementIfNeeded();
    m_caretBlinks = caretBlinks;
    updateAppearance();
}

void FrameSelection::setCaretColor(const Color& caretColor)
{
    if (m_caretColor != caretColor) {
        m_caretColor = caretColor;
        if (caretIsVisible() && m_caretBlinks && isCaret())
            invalidateCaretRect();
    }
}
#endif // PLATFORM(IOS)

}

#ifndef NDEBUG

void showTree(const WebCore::FrameSelection& sel)
{
    sel.showTreeForThis();
}

void showTree(const WebCore::FrameSelection* sel)
{
    if (sel)
        sel->showTreeForThis();
}

#endif
