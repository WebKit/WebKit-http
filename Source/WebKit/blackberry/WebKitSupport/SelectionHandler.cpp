/*
 * Copyright (C) 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "SelectionHandler.h"

#include "DOMSupport.h"
#include "Document.h"
#include "Editor.h"
#include "EditorClient.h"
#include "FatFingers.h"
#include "Frame.h"
#include "FrameSelection.h"
#include "FrameView.h"
#include "HTMLAnchorElement.h"
#include "HTMLAreaElement.h"
#include "HitTestResult.h"
#include "InputHandler.h"
#include "IntRect.h"
#include "Page.h"
#include "RenderPart.h"
#include "TextGranularity.h"
#include "TouchEventHandler.h"
#include "WebPage.h"
#include "WebPageClient.h"
#include "WebPage_p.h"

#include "htmlediting.h"
#include "visible_units.h"

#include <sys/keycodes.h>

#define SHOWDEBUG_SELECTIONHANDLER 0

#if SHOWDEBUG_SELECTIONHANDLER
#define DEBUG_SELECTION(severity, format, ...) BlackBerry::Platform::logAlways(severity, format, ## __VA_ARGS__)
#else
#define DEBUG_SELECTION(severity, format, ...)
#endif // SHOWDEBUG_SELECTIONHANDLER

using namespace WebCore;

namespace BlackBerry {
namespace WebKit {

SelectionHandler::SelectionHandler(WebPagePrivate* page)
    : m_webPage(page)
    , m_selectionActive(false)
    , m_caretActive(false)
    , m_lastUpdatedEndPointIsValid(false)
{
}

SelectionHandler::~SelectionHandler()
{
}

void SelectionHandler::cancelSelection()
{
    m_selectionActive = false;
    m_lastSelectionRegion = BlackBerry::Platform::IntRectRegion();

    DEBUG_SELECTION(BlackBerry::Platform::LogLevelInfo, "SelectionHandler::cancelSelection");

    if (m_webPage->m_inputHandler->isInputMode())
        m_webPage->m_inputHandler->cancelSelection();
    else
        m_webPage->focusedOrMainFrame()->selection()->clear();
}

WebString SelectionHandler::selectedText() const
{
    return m_webPage->focusedOrMainFrame()->editor()->selectedText();
}

bool SelectionHandler::findNextString(const WTF::String& searchString, bool forward)
{
    if (searchString.isEmpty()) {
        cancelSelection();
        return false;
    }

    ASSERT(m_webPage->m_page);

    m_webPage->m_page->unmarkAllTextMatches();

    bool result = m_webPage->m_page->findString(searchString, WTF::TextCaseInsensitive, forward ? FindDirectionForward : FindDirectionBackward, true /* should wrap */);
    if (result && m_webPage->focusedOrMainFrame()->selection()->selectionType() == VisibleSelection::NoSelection) {
        // Word was found but could not be selected on this page.
        result = m_webPage->m_page->markAllMatchesForText(searchString, WTF::TextCaseInsensitive, true /* should highlight */, 0 /* limit to match 0 = unlimited */);
    }

    // Defocus the input field if one is active.
    if (m_webPage->m_inputHandler->isInputMode())
        m_webPage->m_inputHandler->nodeFocused(0);

    return result;
}

void SelectionHandler::getConsolidatedRegionOfTextQuadsForSelection(const VisibleSelection& selection, BlackBerry::Platform::IntRectRegion& region) const
{
    ASSERT(region.isEmpty());

    if (!selection.isRange())
        return;

    ASSERT(selection.firstRange());

    Vector<FloatQuad> quadList;
    DOMSupport::visibleTextQuads(*(selection.firstRange()), quadList, true /* use selection height */);

    if (!quadList.isEmpty()) {
        FrameView* frameView = m_webPage->focusedOrMainFrame()->view();

        // frameRect is in frame coordinates.
        IntRect frameRect(IntPoint(0, 0), frameView->contentsSize());

        // framePosition is in main frame coordinates.
        IntPoint framePosition = m_webPage->frameOffset(m_webPage->focusedOrMainFrame());

        // The ranges rect list is based on render elements and may include multiple adjacent rects.
        // Use BlackBerry::Platform::IntRectRegion to consolidate these rects into bands as well as a container to pass
        // to the client.
        for (unsigned i = 0; i < quadList.size(); i++) {
            IntRect enclosingRect = quadList[i].enclosingBoundingBox();
            enclosingRect.intersect(frameRect);
            enclosingRect.move(framePosition.x(), framePosition.y());
            region = unionRegions(region, BlackBerry::Platform::IntRectRegion(enclosingRect));
        }
    }
}

static VisiblePosition visiblePositionForPointIgnoringClipping(const Frame& frame, const IntPoint& framePoint)
{
    // Frame::visiblePositionAtPoint hard-codes ignoreClipping=false in the
    // call to hitTestResultAtPoint. This has a bug where some pages (such as
    // metafilter) will return the wrong VisiblePosition for points that are
    // outside the visible rect. To work around the bug, this is a copy of
    // visiblePositionAtPoint which which passes ignoreClipping=true.
    // See RIM Bug #4315.
    HitTestResult result = frame.eventHandler()->hitTestResultAtPoint(framePoint, true /* allowShadowContent */, true /* ignoreClipping */);

    Node* node = result.innerNode();
    if (!node)
        return VisiblePosition();

    RenderObject* renderer = node->renderer();
    if (!renderer)
        return VisiblePosition();

    VisiblePosition visiblePos = renderer->positionForPoint(result.localPoint());
    if (visiblePos.isNull())
        visiblePos = VisiblePosition(Position(createLegacyEditingPosition(node, 0)));

    return visiblePos;
}

static unsigned short directionOfPointRelativeToRect(const IntPoint& point, const IntRect& rect)
{
    ASSERT(!rect.contains(point));

    // Padding to prevent accidental trigger of up/down when intending to do horizontal movement.
    const int verticalPadding = 5;

    // Do height movement check first but add padding. We may be off on both x & y axis and only
    // want to move in one direction at a time.
    if (point.y() + verticalPadding < rect.y())
        return KEYCODE_UP;
    if (point.y() > rect.maxY() + verticalPadding)
        return KEYCODE_DOWN;
    if (point.x() < rect.location().x())
        return KEYCODE_LEFT;
    if (point.x() > rect.maxX())
        return KEYCODE_RIGHT;

    return 0;
}

bool SelectionHandler::shouldUpdateSelectionOrCaretForPoint(const IntPoint& point, const IntRect& caretRect, bool startCaret) const
{
    ASSERT(m_webPage->m_inputHandler->isInputMode());

    // If the point isn't valid don't block change as it is not actually changing.
    if (point == DOMSupport::InvalidPoint)
        return true;

    VisibleSelection currentSelection = m_webPage->focusedOrMainFrame()->selection()->selection();

    // If the input field is single line or we are on the first or last
    // line of a multiline input field only horizontal movement is supported.
    bool aboveCaret = point.y() < caretRect.y();
    bool belowCaret = point.y() >= caretRect.maxY();

    DEBUG_SELECTION(BlackBerry::Platform::LogLevelInfo, "SelectionHandler::shouldUpdateSelectionOrCaretForPoint multiline = %s above = %s below = %s first line = %s last line = %s start = %s \n"
            , m_webPage->m_inputHandler->isMultilineInputMode() ? "true" : "false", aboveCaret ? "true" : "false", belowCaret ? "true" : "false"
            , inSameLine(currentSelection.visibleStart(), startOfEditableContent(currentSelection.visibleStart())) ? "true" : "false"
            , inSameLine(currentSelection.visibleEnd(), endOfEditableContent(currentSelection.visibleEnd())) ? "true" : "false"
            , startCaret ? "true" : "false");

    if (!m_webPage->m_inputHandler->isMultilineInputMode() && (aboveCaret || belowCaret))
        return false;
    if (startCaret && inSameLine(currentSelection.visibleStart(), startOfEditableContent(currentSelection.visibleStart())) && aboveCaret)
        return false;
    if (!startCaret && inSameLine(currentSelection.visibleEnd(), endOfEditableContent(currentSelection.visibleEnd())) && belowCaret)
        return false;

    return true;
}

void SelectionHandler::setCaretPosition(const IntPoint &position)
{
    if (!m_webPage->m_inputHandler->isInputMode())
        return;

    m_caretActive = true;

    DEBUG_SELECTION(BlackBerry::Platform::LogLevelInfo, "SelectionHandler::setCaretPosition requested point %d, %d", position.x(), position.y());

    Frame* focusedFrame = m_webPage->focusedOrMainFrame();
    FrameSelection* controller = focusedFrame->selection();
    IntPoint relativePoint = DOMSupport::convertPointToFrame(m_webPage->mainFrame(), focusedFrame, position);
    IntRect currentCaretRect = controller->selection().visibleStart().absoluteCaretBounds();

    if (relativePoint == DOMSupport::InvalidPoint || !shouldUpdateSelectionOrCaretForPoint(relativePoint, currentCaretRect)) {
        selectionPositionChanged();
        return;
    }

    VisiblePosition visibleCaretPosition(focusedFrame->visiblePositionForPoint(relativePoint));

    if (!DOMSupport::isPositionInNode(m_webPage->focusedOrMainFrame()->document()->focusedNode(), visibleCaretPosition.deepEquivalent())) {
        if (unsigned short character = directionOfPointRelativeToRect(relativePoint, currentCaretRect))
            m_webPage->m_inputHandler->handleNavigationMove(character, false /* shiftDown */, false /* altDown */, false /* canExitField */);

        selectionPositionChanged();
        return;
    }

    VisibleSelection newSelection(visibleCaretPosition);
    if (controller->selection() == newSelection) {
        selectionPositionChanged();
        return;
    }

    controller->setSelection(newSelection);

    DEBUG_SELECTION(BlackBerry::Platform::LogLevelInfo, "SelectionHandler::setCaretPosition point valid, cursor updated");
}

// This function makes sure we are not reducing the selection to a caret selection.
static bool shouldExtendSelectionInDirection(const VisibleSelection& selection, unsigned short character)
{
    FrameSelection tempSelection;
    tempSelection.setSelection(selection);
    switch (character) {
    case KEYCODE_LEFT:
        tempSelection.modify(FrameSelection::AlterationExtend, DirectionLeft, CharacterGranularity);
        break;
    case KEYCODE_RIGHT:
        tempSelection.modify(FrameSelection::AlterationExtend, DirectionRight, CharacterGranularity);
        break;
    case KEYCODE_UP:
        tempSelection.modify(FrameSelection::AlterationExtend, DirectionBackward, LineGranularity);
        break;
    case KEYCODE_DOWN:
        tempSelection.modify(FrameSelection::AlterationExtend, DirectionForward, LineGranularity);
        break;
    default:
        break;
    }

    if ((character == KEYCODE_LEFT || character == KEYCODE_RIGHT)
        && (!inSameLine(selection.visibleStart(), tempSelection.selection().visibleStart())
           || !inSameLine(selection.visibleEnd(), tempSelection.selection().visibleEnd())))
        return false;

    return tempSelection.selection().selectionType() == VisibleSelection::RangeSelection;
}

static VisiblePosition directionalVisiblePositionAtExtentOfBox(Frame* frame, const IntRect& boundingBox, unsigned short direction, const IntPoint& basePoint)
{
    ASSERT(frame);

    if (!frame)
        return VisiblePosition();

    switch (direction) {
    case KEYCODE_LEFT:
        // Extend x to start without modifying y.
        return frame->visiblePositionForPoint(IntPoint(boundingBox.x(), basePoint.y()));
    case KEYCODE_RIGHT:
        // Extend x to end without modifying y.
        return frame->visiblePositionForPoint(IntPoint(boundingBox.maxX(), basePoint.y()));
    case KEYCODE_UP:
        // Extend y to top without modifying x.
        return frame->visiblePositionForPoint(IntPoint(basePoint.x(), boundingBox.y()));
    case KEYCODE_DOWN:
        // Extend y to bottom without modifying x.
        return frame->visiblePositionForPoint(IntPoint(basePoint.x(), boundingBox.maxY()));
    default:
        break;
    }

    return frame->visiblePositionForPoint(IntPoint(basePoint.x(), basePoint.y()));
}

static bool pointIsOutsideOfBoundingBoxInDirection(unsigned direction, const IntPoint& selectionPoint, const IntRect& boundingBox)
{
    if ((direction == KEYCODE_LEFT && selectionPoint.x() < boundingBox.x())
        || (direction == KEYCODE_UP && selectionPoint.y() < boundingBox.y())
        || (direction == KEYCODE_RIGHT && selectionPoint.x() > boundingBox.maxX())
        || (direction == KEYCODE_DOWN && selectionPoint.y() > boundingBox.maxY()))
        return true;

    return false;
}

unsigned short SelectionHandler::extendSelectionToFieldBoundary(bool isStartHandle, const IntPoint& selectionPoint, VisibleSelection& newSelection)
{
    Frame* focusedFrame = m_webPage->focusedOrMainFrame();
    if (!focusedFrame->document()->focusedNode() || !focusedFrame->document()->focusedNode()->renderer())
        return 0;

    FrameSelection* controller = focusedFrame->selection();

    IntRect caretRect = isStartHandle ? controller->selection().visibleStart().absoluteCaretBounds()
                                      : controller->selection().visibleEnd().absoluteCaretBounds();

    IntRect nodeBoundingBox = focusedFrame->document()->focusedNode()->renderer()->absoluteBoundingBoxRect();

    // Start handle is outside of the field. Treat it as the changed handle and move
    // relative to the start caret rect.
    unsigned short character = directionOfPointRelativeToRect(selectionPoint, caretRect);

    // Prevent incorrect movement, handles can only extend the selection this way
    // to prevent inversion of the handles.
    if (isStartHandle && (character == KEYCODE_RIGHT || character == KEYCODE_DOWN)
        || !isStartHandle && (character == KEYCODE_LEFT || character == KEYCODE_UP))
        character = 0;

    VisiblePosition newVisiblePosition = controller->selection().start();
    // Extend the selection to the bounds of the box before doing incremental scroll if the point is outside the node.
    // Don't extend selection and handle the character at the same time.
    if (pointIsOutsideOfBoundingBoxInDirection(character, selectionPoint, nodeBoundingBox))
        newVisiblePosition = directionalVisiblePositionAtExtentOfBox(focusedFrame, nodeBoundingBox, character, selectionPoint);

    if (isStartHandle)
        newSelection = VisibleSelection(newVisiblePosition, newSelection.visibleEnd(), true /*isDirectional*/);
    else
        newSelection = VisibleSelection(newSelection.visibleStart(), newVisiblePosition, true /*isDirectional*/);

    // If no selection will be changed, return the character to extend using navigation.
    if (controller->selection() == newSelection)
        return character;

    // Selection has been updated.
    return 0;
}

// Returns true if handled.
bool SelectionHandler::updateOrHandleInputSelection(VisibleSelection& newSelection, const IntPoint& relativeStart
                                                    , const IntPoint& relativeEnd)
{
    ASSERT(m_webPage->m_inputHandler->isInputMode());

    Frame* focusedFrame = m_webPage->focusedOrMainFrame();
    Node* focusedNode = focusedFrame->document()->focusedNode();
    if (!focusedNode || !focusedNode->renderer())
        return false;

    FrameSelection* controller = focusedFrame->selection();

    IntRect currentStartCaretRect = controller->selection().visibleStart().absoluteCaretBounds();
    IntRect currentEndCaretRect = controller->selection().visibleEnd().absoluteCaretBounds();

    // Check if the handle movement is valid.
    if (!shouldUpdateSelectionOrCaretForPoint(relativeStart, currentStartCaretRect, true /* startCaret */)
        || !shouldUpdateSelectionOrCaretForPoint(relativeEnd, currentEndCaretRect, false /* startCaret */)) {
        selectionPositionChanged();
        return true;
    }

    IntRect nodeBoundingBox = focusedNode->renderer()->absoluteBoundingBoxRect();

    // Only do special handling if one handle is outside of the node.
    bool startIsOutsideOfField = relativeStart != DOMSupport::InvalidPoint && !nodeBoundingBox.contains(relativeStart);
    bool endIsOutsideOfField = relativeEnd != DOMSupport::InvalidPoint && !nodeBoundingBox.contains(relativeEnd);
    if (startIsOutsideOfField && endIsOutsideOfField)
        return false;

    unsigned short character = 0;
    if (startIsOutsideOfField) {
        character = extendSelectionToFieldBoundary(true /* isStartHandle */, relativeStart, newSelection);
        if (character) {
            // Invert the selection so that the cursor point is at the beginning.
            controller->setSelection(VisibleSelection(controller->selection().end(), controller->selection().start()));
        }
    } else if (endIsOutsideOfField) {
        character = extendSelectionToFieldBoundary(false /* isStartHandle */, relativeEnd, newSelection);
        if (character) {
            // Reset the selection so that the end is the edit point.
            controller->setSelection(VisibleSelection(controller->selection().start(), controller->selection().end()));
        }
    }

    if (!character)
        return false;

    DEBUG_SELECTION(BlackBerry::Platform::LogLevelInfo, "SelectionHandler::setSelection making selection change attempt using navigation");

    if (shouldExtendSelectionInDirection(controller->selection(), character))
        m_webPage->m_inputHandler->handleNavigationMove(character, true /* shiftDown */, false /* altDown */, false /* canExitField */);

    // Must send the selectionPositionChanged every time, sometimes this will duplicate but an accepted
    // handleNavigationMove may not make an actual selection change.
    selectionPositionChanged();
    return true;
}

void SelectionHandler::setSelection(const IntPoint& start, const IntPoint& end)
{
    m_selectionActive = true;

    ASSERT(m_webPage);
    ASSERT(m_webPage->focusedOrMainFrame());
    ASSERT(m_webPage->focusedOrMainFrame()->selection());

    Frame* focusedFrame = m_webPage->focusedOrMainFrame();
    FrameSelection* controller = focusedFrame->selection();

    DEBUG_SELECTION(BlackBerry::Platform::LogLevelInfo, "SelectionHandler::setSelection adjusted points %d, %d, %d, %d", start.x(), start.y(), end.x(), end.y());

    // Note that IntPoint(-1, -1) is being our sentinel so far for
    // clipped out selection starting or ending location.
    bool startIsValid = start != DOMSupport::InvalidPoint;
    m_lastUpdatedEndPointIsValid = end != DOMSupport::InvalidPoint;

    // At least one of the locations must be valid.
    ASSERT(startIsValid || m_lastUpdatedEndPointIsValid);

    IntPoint relativeStart = start;
    IntPoint relativeEnd = end;

    VisibleSelection newSelection(controller->selection());

    // We need the selection to be ordered base then extent.
    if (!controller->selection().isBaseFirst())
        controller->setSelection(VisibleSelection(controller->selection().start(), controller->selection().end()));

    if (startIsValid) {
        relativeStart = DOMSupport::convertPointToFrame(m_webPage->mainFrame(), focusedFrame, start);

        // Set the selection with validation.
        newSelection.setBase(visiblePositionForPointIgnoringClipping(*focusedFrame, relativeStart));

        // Reset the selection using the existing extent without validation.
        newSelection.setWithoutValidation(newSelection.base(), controller->selection().end());
    }

    if (m_lastUpdatedEndPointIsValid) {
        relativeEnd = DOMSupport::convertPointToFrame(m_webPage->mainFrame(), focusedFrame, end);

        // Set the selection with validation.
        newSelection.setExtent(visiblePositionForPointIgnoringClipping(*focusedFrame, relativeEnd));

        // Reset the selection using the existing base without validation.
        newSelection.setWithoutValidation(controller->selection().start(), newSelection.extent());
    }

    if (m_webPage->m_inputHandler->isInputMode()) {
        if (updateOrHandleInputSelection(newSelection, relativeStart, relativeEnd))
            return;
    }

    if (controller->selection() == newSelection) {
        selectionPositionChanged();
        return;
    }

    // If the selection size is reduce to less than a character, selection type becomes
    // Caret. As long as it is still a range, it's a valid selection. Selection cannot
    // be cancelled through this function.
    BlackBerry::Platform::IntRectRegion region;
    getConsolidatedRegionOfTextQuadsForSelection(newSelection, region);
    clipRegionToVisibleContainer(region);
    if (!region.isEmpty()) {
        // Check if the handles reversed position.
        if (m_selectionActive && !newSelection.isBaseFirst())
            m_webPage->m_client->notifySelectionHandlesReversed();

        controller->setSelection(newSelection);

        DEBUG_SELECTION(BlackBerry::Platform::LogLevelInfo, "SelectionHandler::setSelection selection points valid, selection updated");
    } else {
        // Requested selection results in an empty selection, skip this change.
        selectionPositionChanged();

        DEBUG_SELECTION(BlackBerry::Platform::LogLevelWarn, "SelectionHandler::setSelection selection points invalid, selection not updated");
    }
}

// FIXME re-use this in context. Must be updated to include an option to return the href.
// This function should be moved to a new unit file. Names suggetions include DOMQueries
// and NodeTypes. Functions currently in InputHandler.cpp, SelectionHandler.cpp and WebPage.cpp
// can all be moved in.
static Node* enclosingLinkEventParentForNode(Node* node)
{
    if (!node)
        return 0;

    Node* linkNode = node->enclosingLinkEventParentOrSelf();
    return linkNode && linkNode->isLink() ? linkNode : 0;
}

void SelectionHandler::selectAtPoint(const IntPoint& location)
{
    // If point is invalid trigger selection based expansion.
    if (location == DOMSupport::InvalidPoint) {
        selectObject(WordGranularity);
        return;
    }

    Node* targetNode;
    IntPoint targetPosition;
    // FIXME: Factory this get right fat finger code into a helper.
    const FatFingersResult lastFatFingersResult = m_webPage->m_touchEventHandler->lastFatFingersResult();
    if (lastFatFingersResult.positionWasAdjusted() && lastFatFingersResult.nodeAsElementIfApplicable()) {
        targetNode = lastFatFingersResult.validNode();
        targetPosition = lastFatFingersResult.adjustedPosition();
    } else {
        FatFingersResult newFatFingersResult = FatFingers(m_webPage, location, FatFingers::Text).findBestPoint();
        if (!newFatFingersResult.positionWasAdjusted())
            return;

        targetPosition = newFatFingersResult.adjustedPosition();
        targetNode = newFatFingersResult.validNode();
    }

    ASSERT(targetNode);

    // If the node at the point is a link, focus on the entire link, not a word.
    if (Node* link = enclosingLinkEventParentForNode(targetNode)) {
        selectObject(link);
        return;
    }

    // selectAtPoint API currently only supports WordGranularity but may be extended in the future.
    selectObject(targetPosition, WordGranularity);
}

static bool expandSelectionToGranularity(Frame* frame, VisibleSelection selection, TextGranularity granularity, bool isInputMode)
{
    ASSERT(frame);
    ASSERT(frame->selection());

    if (!(selection.start().anchorNode() && selection.start().anchorNode()->isTextNode()))
        return false;

    if (granularity == WordGranularity)
        selection = DOMSupport::visibleSelectionForClosestActualWordStart(selection);

    selection.expandUsingGranularity(granularity);
    RefPtr<Range> newRange = selection.toNormalizedRange();
    RefPtr<Range> oldRange = frame->selection()->selection().toNormalizedRange();
    EAffinity affinity = frame->selection()->affinity();

    if (isInputMode && !frame->editor()->client()->shouldChangeSelectedRange(oldRange.get(), newRange.get(), affinity, false))
        return false;

    return frame->selection()->setSelectedRange(newRange.get(), affinity, true);
}

void SelectionHandler::selectObject(const IntPoint& location, TextGranularity granularity)
{
    ASSERT(location.x() >= 0 && location.y() >= 0);
    ASSERT(m_webPage && m_webPage->focusedOrMainFrame() && m_webPage->focusedOrMainFrame()->selection());
    Frame* focusedFrame = m_webPage->focusedOrMainFrame();

    DEBUG_SELECTION(BlackBerry::Platform::LogLevelInfo, "SelectionHandler::selectObject adjusted points %d, %d", location.x(), location.y());

    IntPoint relativePoint = DOMSupport::convertPointToFrame(m_webPage->mainFrame(), focusedFrame, location);
    VisiblePosition pointLocation(focusedFrame->visiblePositionForPoint(relativePoint));
    VisibleSelection selection = VisibleSelection(pointLocation, pointLocation);

    m_selectionActive = expandSelectionToGranularity(focusedFrame, selection, granularity, m_webPage->m_inputHandler->isInputMode());
}

void SelectionHandler::selectObject(TextGranularity granularity)
{
    ASSERT(m_webPage && m_webPage->m_inputHandler);
    // Using caret location, must be inside an input field.
    if (!m_webPage->m_inputHandler->isInputMode())
        return;

    ASSERT(m_webPage->focusedOrMainFrame() && m_webPage->focusedOrMainFrame()->selection());
    Frame* focusedFrame = m_webPage->focusedOrMainFrame();

    DEBUG_SELECTION(BlackBerry::Platform::LogLevelInfo, "SelectionHandler::selectObject using current selection");

    // Use the current selection as the selection point.
    ASSERT(focusedFrame->selection()->selectionType() != VisibleSelection::NoSelection);
    m_selectionActive = expandSelectionToGranularity(focusedFrame, focusedFrame->selection()->selection(), granularity, true /* isInputMode */);
}

void SelectionHandler::selectObject(Node* node)
{
    if (!node)
        return;

    m_selectionActive = true;

    ASSERT(m_webPage && m_webPage->focusedOrMainFrame() && m_webPage->focusedOrMainFrame()->selection());
    Frame* focusedFrame = m_webPage->focusedOrMainFrame();

    DEBUG_SELECTION(BlackBerry::Platform::LogLevelInfo, "SelectionHandler::selectNode");

    VisibleSelection selection = VisibleSelection::selectionFromContentsOfNode(node);
    focusedFrame->selection()->setSelection(selection);
}

static TextDirection directionOfEnclosingBlock(FrameSelection* selection)
{
    Node* enclosingBlockNode = enclosingBlock(selection->selection().extent().deprecatedNode());
    if (!enclosingBlockNode)
        return LTR;

    if (RenderObject* renderer = enclosingBlockNode->renderer())
        return renderer->style()->direction();

    return LTR;
}

// Returns > 0 if p1 is "closer" to referencePoint, < 0 if p2 is "closer", 0 if they are equidistant.
// Because text is usually arranged in horizontal rows, distance is measured along the y-axis, with x-axis used only to break ties.
// If rightGravity is true, the right-most x-coordinate is chosen, otherwise teh left-most coordinate is chosen.
static inline int comparePointsToReferencePoint(const IntPoint& p1, const IntPoint& p2, const IntPoint& referencePoint, bool rightGravity)
{
    int dy1 = abs(referencePoint.y() - p1.y());
    int dy2 = abs(referencePoint.y() - p2.y());
    if (dy1 != dy2)
        return dy2 - dy1;

    // Same y-coordinate, choose the farthest right (or left) point.
    if (p1.x() == p2.x())
        return 0;

    if (p1.x() > p2.x())
        return rightGravity ? 1 : -1;

    return rightGravity ? -1 : 1;
}

// NOTE/FIXME: Due to r77286, we are getting off-by-one results in the IntRect class counterpart implementation of the
//             methods below. As done in r89803, r77928 and a few others, lets use local method to fix it.
//             We should keep our eyes very open on it, since it can affect BackingStore very badly.
static IntPoint minXMinYCorner(const IntRect& rect) { return rect.location(); } // typically topLeft
static IntPoint maxXMinYCorner(const IntRect& rect) { return IntPoint(rect.x() + rect.width() - 1, rect.y()); } // typically topRight
static IntPoint minXMaxYCorner(const IntRect& rect) { return IntPoint(rect.x(), rect.y() + rect.height() - 1); } // typically bottomLeft
static IntPoint maxXMaxYCorner(const IntRect& rect) { return IntPoint(rect.x() + rect.width() - 1, rect.y() + rect.height() - 1); } // typically bottomRight

// The caret is a one-pixel wide line down either the right or left edge of a
// rect, depending on the text direction.
static inline bool caretIsOnLeft(bool isStartCaret, bool isRTL)
{
    if (isStartCaret)
        return !isRTL;

    return isRTL;
}

static inline IntPoint caretLocationForRect(const IntRect& rect, bool isStartCaret, bool isRTL)
{
    return caretIsOnLeft(isStartCaret, isRTL) ? minXMinYCorner(rect) : maxXMinYCorner(rect);
}

static inline IntPoint caretComparisonPointForRect(const IntRect& rect, bool isStartCaret, bool isRTL)
{
    if (isStartCaret)
        return caretIsOnLeft(isStartCaret, isRTL) ? minXMinYCorner(rect) : maxXMinYCorner(rect);

    return caretIsOnLeft(isStartCaret, isRTL) ? minXMaxYCorner(rect) : maxXMaxYCorner(rect);
}

static void adjustCaretRects(IntRect& startCaret, bool isStartCaretClippedOut,
                             IntRect& endCaret, bool isEndCaretClippedOut,
                             const std::vector<BlackBerry::Platform::IntRect> rectList,
                             const IntPoint& startReferencePoint,
                             const IntPoint& endReferencePoint,
                             bool isRTL)
{
    // startReferencePoint is the best guess at the top left of the selection; endReferencePoint is the best guess at the bottom right.
    if (isStartCaretClippedOut)
        startCaret.setLocation(DOMSupport::InvalidPoint);
    else {
        startCaret = rectList[0];
        startCaret.setLocation(caretLocationForRect(startCaret, true, isRTL));
    }

    if (isEndCaretClippedOut)
        endCaret.setLocation(DOMSupport::InvalidPoint);
    else {
        endCaret = rectList[0];
        endCaret.setLocation(caretLocationForRect(endCaret, false, isRTL));
    }

    if (isStartCaretClippedOut && isEndCaretClippedOut)
        return;

    // Reset width to 1 as we are strictly interested in caret location.
    startCaret.setWidth(1);
    endCaret.setWidth(1);

    for (unsigned i = 1; i < rectList.size(); i++) {
        IntRect currentRect(rectList[i]);

        // Compare and update the start and end carets with their respective reference points.
        if (!isStartCaretClippedOut && comparePointsToReferencePoint(
                    caretComparisonPointForRect(currentRect, true, isRTL),
                    caretComparisonPointForRect(startCaret, true, isRTL),
                    startReferencePoint, isRTL) > 0) {
            startCaret.setLocation(caretLocationForRect(currentRect, true, isRTL));
            startCaret.setHeight(currentRect.height());
        }

        if (!isEndCaretClippedOut && comparePointsToReferencePoint(
                    caretComparisonPointForRect(currentRect, false, isRTL),
                    caretComparisonPointForRect(endCaret, false, isRTL),
                    endReferencePoint, !isRTL) > 0) {
            endCaret.setLocation(caretLocationForRect(currentRect, false, isRTL));
            endCaret.setHeight(currentRect.height());
        }
    }
}

void SelectionHandler::clipRegionToVisibleContainer(BlackBerry::Platform::IntRectRegion& region)
{
    ASSERT(m_webPage->m_mainFrame && m_webPage->m_mainFrame->view());

    Frame* frame = m_webPage->focusedOrMainFrame();

    // Don't allow the region to extend outside of the all its ancestor frames' visible area.
    if (frame != m_webPage->mainFrame()) {
        IntRect containingContentRect;
        containingContentRect = m_webPage->getRecursiveVisibleWindowRect(frame->view(), true /* no clip to main frame window */);
        containingContentRect = m_webPage->m_mainFrame->view()->windowToContents(containingContentRect);
        region = intersectRegions(BlackBerry::Platform::IntRectRegion(containingContentRect), region);
    }

    // Don't allow the region to extend outside of the input field.
    if (m_webPage->m_inputHandler->isInputMode()
        && frame->document()->focusedNode()
        && frame->document()->focusedNode()->renderer()) {

        // Adjust the bounding box to the frame offset.
        IntRect boundingBox(frame->document()->focusedNode()->renderer()->absoluteBoundingBoxRect());
        boundingBox = m_webPage->mainFrame()->view()->windowToContents(frame->view()->contentsToWindow(boundingBox));

        region = intersectRegions(BlackBerry::Platform::IntRectRegion(boundingBox), region);
    }
}

static IntPoint referencePoint(const VisiblePosition& position, const IntRect& boundingRect, const IntPoint& framePosition, bool isStartCaret, bool isRTL)
{
    // If one of the carets is invalid (this happens, for instance, if the
    // selection ends in an empty div) fall back to using the corner of the
    // entire region (which is already in frame coordinates so doesn't need
    // adjusting).
    IntRect startCaretBounds(position.absoluteCaretBounds());
    if (startCaretBounds.isEmpty())
        startCaretBounds = boundingRect;
    else
        startCaretBounds.move(framePosition.x(), framePosition.y());

    return caretComparisonPointForRect(startCaretBounds, isStartCaret, isRTL);
}

// Note: This is the only function in SelectionHandler in which the coordinate
// system is not entirely WebKit.
void SelectionHandler::selectionPositionChanged(bool visualChangeOnly)
{
    DEBUG_SELECTION(BlackBerry::Platform::LogLevelInfo, "SelectionHandler::selectionPositionChanged visibleChangeOnly = %s", visualChangeOnly ? "true" : "false");

    // This method can get called during WebPage shutdown process.
    // If that is the case, just bail out since the client is not
    // in a safe state of trust to request anything else from it.
    if (!m_webPage->m_mainFrame)
        return;

    if (m_webPage->m_inputHandler->isInputMode() && m_webPage->m_inputHandler->processingChange()) {
        m_webPage->m_client->cancelSelectionVisuals();
        return;
    }

    if (m_caretActive || (m_webPage->m_inputHandler->isInputMode() && m_webPage->focusedOrMainFrame()->selection()->isCaret())) {
        // This may update the caret to no longer be active.
        caretPositionChanged();
    }

    // Enter selection mode if selection type is RangeSelection, and disable selection if
    // selection is active and becomes caret selection.
    Frame* frame = m_webPage->focusedOrMainFrame();
    IntPoint framePos = m_webPage->frameOffset(frame);
    if (m_selectionActive && (m_caretActive || frame->selection()->isNone()))
        m_selectionActive = false;
    else if (frame->selection()->isRange())
        m_selectionActive = true;
    else if (!m_selectionActive)
        return;

    IntRect startCaret;
    IntRect endCaret;

    // Get the text rects from the selections range.
    BlackBerry::Platform::IntRectRegion region;
    getConsolidatedRegionOfTextQuadsForSelection(frame->selection()->selection(), region);

    // If there is no change in selected text and the visual rects
    // have not changed then don't bother notifying anything.
    if (visualChangeOnly && m_lastSelectionRegion.isEqual(region))
        return;

    m_lastSelectionRegion = region;

    if (!region.isEmpty()) {
        IntRect unclippedStartCaret;
        IntRect unclippedEndCaret;

        bool isRTL = directionOfEnclosingBlock(frame->selection()) == RTL;

        std::vector<BlackBerry::Platform::IntRect> rectList = region.rects();

        IntPoint startCaretReferencePoint = referencePoint(frame->selection()->selection().visibleStart(), region.extents(), framePos, true /* isStartCaret */, isRTL);
        IntPoint endCaretReferencePoint = referencePoint(frame->selection()->selection().visibleEnd(), region.extents(), framePos, false /* isStartCaret */, isRTL);

        adjustCaretRects(unclippedStartCaret, false /* unclipped */, unclippedEndCaret, false /* unclipped */, rectList, startCaretReferencePoint, endCaretReferencePoint, isRTL);

        clipRegionToVisibleContainer(region);

#if SHOWDEBUG_SELECTIONHANDLER // Don't rely just on DEBUG_SELECTION to avoid loop.
        for (unsigned int i = 0; i < rectList.size(); i++)
            DEBUG_SELECTION(BlackBerry::Platform::LogLevelCritical, "Rect list - Unmodified #%d, (%d, %d) (%d x %d)", i, rectList[i].x(), rectList[i].y(), rectList[i].width(), rectList[i].height());
        for (unsigned int i = 0; i < region.numRects(); i++)
            DEBUG_SELECTION(BlackBerry::Platform::LogLevelCritical, "Rect list  - Consolidated #%d, (%d, %d) (%d x %d)", i, region.rects()[i].x(), region.rects()[i].y(), region.rects()[i].width(), region.rects()[i].height());
#endif

        bool shouldCareAboutPossibleClippedOutSelection = frame != m_webPage->mainFrame() || m_webPage->m_inputHandler->isInputMode();

        if (!region.isEmpty() || shouldCareAboutPossibleClippedOutSelection) {
            // Adjust the handle markers to be at the end of the painted rect. When selecting links
            // and other elements that may have a larger visible area than needs to be rendered a gap
            // can exist between the handle and overlay region.

            bool shouldClipStartCaret = !region.isRectInRegion(unclippedStartCaret);
            bool shouldClipEndCaret = !region.isRectInRegion(unclippedEndCaret);

            // Find the top corner and bottom corner.
            std::vector<BlackBerry::Platform::IntRect> clippedRectList = region.rects();
            adjustCaretRects(startCaret, shouldClipStartCaret, endCaret, shouldClipEndCaret, clippedRectList, startCaretReferencePoint, endCaretReferencePoint, isRTL);

            // Translate the caret values as they must be in transformed coordinates.
            if (!shouldClipStartCaret) {
                startCaret = m_webPage->mapToTransformed(startCaret);
                m_webPage->clipToTransformedContentsRect(startCaret);
            }

            if (!shouldClipEndCaret) {
                endCaret = m_webPage->mapToTransformed(endCaret);
                m_webPage->clipToTransformedContentsRect(endCaret);
            }
        }
    }

    DEBUG_SELECTION(BlackBerry::Platform::LogLevelInfo, "SelectionHandler::selectionPositionChanged Start Rect=%s End Rect=%s",
                        startCaret.toString().utf8().data(), endCaret.toString().utf8().data());

    m_webPage->m_client->notifySelectionDetailsChanged(startCaret, endCaret, region);
}

// NOTE: This function is not in WebKit coordinates.
void SelectionHandler::caretPositionChanged()
{
    DEBUG_SELECTION(BlackBerry::Platform::LogLevelInfo, "SelectionHandler::caretPositionChanged");

    IntRect caretLocation;
    // If the input field is not active, we must be turning off the caret.
    if (!m_webPage->m_inputHandler->isInputMode() && m_caretActive) {
        m_caretActive = false;
        // Send an empty caret change to turn off the caret.
        m_webPage->m_client->notifyCaretChanged(caretLocation, m_webPage->m_touchEventHandler->lastFatFingersResult().isTextInput() /* userTouchTriggered */);
        return;
    }

    ASSERT(m_webPage && m_webPage->focusedOrMainFrame() && m_webPage->focusedOrMainFrame()->selection());

    // This function should only reach this point if input mode is active.
    ASSERT(m_webPage->m_inputHandler->isInputMode());

    if (m_webPage->focusedOrMainFrame()->selection()->selectionType() == VisibleSelection::CaretSelection) {
        IntPoint frameOffset = m_webPage->frameOffset(m_webPage->focusedOrMainFrame());

        caretLocation = m_webPage->focusedOrMainFrame()->selection()->selection().visibleStart().absoluteCaretBounds();
        caretLocation.move(frameOffset.x(), frameOffset.y());

        // Clip against the containing frame and node boundaries.
        BlackBerry::Platform::IntRectRegion region(caretLocation);
        clipRegionToVisibleContainer(region);
        caretLocation = region.extents();
    }

    m_caretActive = !caretLocation.isEmpty();

    DEBUG_SELECTION(BlackBerry::Platform::LogLevelInfo, "SelectionHandler::caretPositionChanged caret Rect %d, %d, %dx%d",
                        caretLocation.x(), caretLocation.y(), caretLocation.width(), caretLocation.height());

    caretLocation = m_webPage->mapToTransformed(caretLocation);
    m_webPage->clipToTransformedContentsRect(caretLocation);

    m_webPage->m_client->notifyCaretChanged(caretLocation, m_webPage->m_touchEventHandler->lastFatFingersResult().isTextInput() /* userTouchTriggered */);
}

bool SelectionHandler::selectionContains(const IntPoint& point)
{
    ASSERT(m_webPage && m_webPage->focusedOrMainFrame() && m_webPage->focusedOrMainFrame()->selection());
    return m_webPage->focusedOrMainFrame()->selection()->contains(point);
}

}
}
