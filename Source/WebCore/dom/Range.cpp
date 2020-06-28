/*
 * (C) 1999 Lars Knoll (knoll@kde.org)
 * (C) 2000 Gunnstein Lye (gunnstein@netcom.no)
 * (C) 2000 Frederik Holljen (frederik.holljen@hig.no)
 * (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Motorola Mobility. All rights reserved.
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
#include "Range.h"

#include "Comment.h"
#include "DOMRect.h"
#include "DOMRectList.h"
#include "DocumentFragment.h"
#include "Editing.h"
#include "Event.h"
#include "Frame.h"
#include "FrameView.h"
#include "HTMLBodyElement.h"
#include "HTMLElement.h"
#include "HTMLHtmlElement.h"
#include "HTMLNames.h"
#include "NodeTraversal.h"
#include "NodeWithIndex.h"
#include "ProcessingInstruction.h"
#include "RenderBlock.h"
#include "RenderBoxModelObject.h"
#include "RenderText.h"
#include "RenderView.h"
#include "ScopedEventQueue.h"
#include "TextIterator.h"
#include "VisiblePosition.h"
#include "VisibleUnits.h"
#include "markup.h"
#include <stdio.h>
#include <wtf/RefCountedLeakCounter.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>

#if PLATFORM(IOS_FAMILY)
#include "SelectionRect.h"
#endif

namespace WebCore {

using namespace HTMLNames;

DEFINE_DEBUG_ONLY_GLOBAL(WTF::RefCountedLeakCounter, rangeCounter, ("Range"));

enum ContentsProcessDirection { ProcessContentsForward, ProcessContentsBackward };
enum class CoordinateSpace { Absolute, Client };

static ExceptionOr<void> processNodes(Range::ActionType, Vector<Ref<Node>>&, Node* oldContainer, RefPtr<Node> newContainer);
static ExceptionOr<RefPtr<Node>> processContentsBetweenOffsets(Range::ActionType, RefPtr<DocumentFragment>, RefPtr<Node> container, unsigned startOffset, unsigned endOffset);
static ExceptionOr<RefPtr<Node>> processAncestorsAndTheirSiblings(Range::ActionType, Node* container, ContentsProcessDirection, ExceptionOr<RefPtr<Node>>&& passedClonedContainer, Node* commonRoot);

inline Range::Range(Document& ownerDocument)
    : m_ownerDocument(ownerDocument)
    , m_start(&ownerDocument)
    , m_end(&ownerDocument)
{
#ifndef NDEBUG
    rangeCounter.increment();
#endif

    m_ownerDocument->attachRange(*this);
}

Ref<Range> Range::create(Document& ownerDocument)
{
    return adoptRef(*new Range(ownerDocument));
}

inline Range::Range(Document& ownerDocument, Node* startContainer, int startOffset, Node* endContainer, int endOffset)
    : m_ownerDocument(ownerDocument)
    , m_start(&ownerDocument)
    , m_end(&ownerDocument)
{
#ifndef NDEBUG
    rangeCounter.increment();
#endif

    m_ownerDocument->attachRange(*this);

    // Simply setting the containers and offsets directly would not do any of the checking
    // that setStart and setEnd do, so we call those functions.
    if (startContainer)
        setStart(*startContainer, startOffset);
    if (endContainer)
        setEnd(*endContainer, endOffset);
}

Ref<Range> Range::create(Document& ownerDocument, RefPtr<Node>&& startContainer, int startOffset, RefPtr<Node>&& endContainer, int endOffset)
{
    return adoptRef(*new Range(ownerDocument, startContainer.get(), startOffset, endContainer.get(), endOffset));
}

Ref<Range> Range::create(Document& ownerDocument, const Position& start, const Position& end)
{
    return adoptRef(*new Range(ownerDocument, start.containerNode(), start.computeOffsetInContainerNode(), end.containerNode(), end.computeOffsetInContainerNode()));
}

Ref<Range> Range::create(Document& ownerDocument, const VisiblePosition& visibleStart, const VisiblePosition& visibleEnd)
{
    Position start = visibleStart.deepEquivalent().parentAnchoredEquivalent();
    Position end = visibleEnd.deepEquivalent().parentAnchoredEquivalent();
    return adoptRef(*new Range(ownerDocument, start.anchorNode(), start.deprecatedEditingOffset(), end.anchorNode(), end.deprecatedEditingOffset()));
}

Range::~Range()
{
    m_ownerDocument->detachRange(*this);

#ifndef NDEBUG
    rangeCounter.decrement();
#endif
}

void Range::setDocument(Document& document)
{
    ASSERT(m_ownerDocument.ptr() != &document);
    m_ownerDocument->detachRange(*this);
    m_ownerDocument = document;
    m_start.setToStartOfNode(document);
    m_end.setToStartOfNode(document);
    m_ownerDocument->attachRange(*this);
}

static inline bool checkForDifferentRootContainer(const RangeBoundaryPoint& start, const RangeBoundaryPoint& end)
{
    Node* endRootContainer = end.container();
    while (endRootContainer->parentNode())
        endRootContainer = endRootContainer->parentNode();
    Node* startRootContainer = start.container();
    while (startRootContainer->parentNode())
        startRootContainer = startRootContainer->parentNode();

    return startRootContainer != endRootContainer || Range::compareBoundaryPoints(start, end).releaseReturnValue() > 0;
}

ExceptionOr<void> Range::setStart(Ref<Node>&& refNode, unsigned offset)
{
    bool didMoveDocument = false;
    if (&refNode->document() != &ownerDocument()) {
        setDocument(refNode->document());
        didMoveDocument = true;
    }

    auto childNode = checkNodeWOffset(refNode, offset);
    if (childNode.hasException())
        return childNode.releaseException();

    m_start.set(WTFMove(refNode), offset, childNode.releaseReturnValue());

    if (didMoveDocument || checkForDifferentRootContainer(m_start, m_end))
        collapse(true);

    return { };
}

ExceptionOr<void> Range::setEnd(Ref<Node>&& refNode, unsigned offset)
{
    bool didMoveDocument = false;
    if (&refNode->document() != &ownerDocument()) {
        setDocument(refNode->document());
        didMoveDocument = true;
    }

    auto childNode = checkNodeWOffset(refNode, offset);
    if (childNode.hasException())
        return childNode.releaseException();

    m_end.set(WTFMove(refNode), offset, childNode.releaseReturnValue());

    if (didMoveDocument || checkForDifferentRootContainer(m_start, m_end))
        collapse(false);

    return { };
}

ExceptionOr<void> Range::setStart(const Position& start)
{
    Position parentAnchored = start.parentAnchoredEquivalent();
    if (!parentAnchored.containerNode())
        return Exception { TypeError };
    return setStart(*parentAnchored.containerNode(), parentAnchored.offsetInContainerNode());
}

ExceptionOr<void> Range::setEnd(const Position& end)
{
    Position parentAnchored = end.parentAnchoredEquivalent();
    if (!parentAnchored.containerNode())
        return Exception { TypeError };
    return setEnd(*parentAnchored.containerNode(), parentAnchored.offsetInContainerNode());
}

void Range::collapse(bool toStart)
{
    if (toStart)
        m_end = m_start;
    else
        m_start = m_end;
}

ExceptionOr<bool> Range::isPointInRange(Node& refNode, unsigned offset)
{
    if (&refNode.document() != &ownerDocument())
        return false;

    auto checkNodeResult = checkNodeWOffset(refNode, offset);
    if (checkNodeResult.hasException()) {
        // DOM4 spec requires us to check whether refNode and start container have the same root first
        // but we do it in the reverse order to avoid O(n) operation here in common case.
        if (!commonInclusiveAncestor(refNode, startContainer()))
            return false;
        return checkNodeResult.releaseException();
    }

    auto startCompareResult = compareBoundaryPoints(&refNode, offset, &startContainer(), m_start.offset());
    if (!(!startCompareResult.hasException() && startCompareResult.releaseReturnValue() >= 0))
        return false;
    auto endCompareResult = compareBoundaryPoints(&refNode, offset, &endContainer(), m_end.offset());
    return !endCompareResult.hasException() && endCompareResult.releaseReturnValue() <= 0;
}

ExceptionOr<short> Range::comparePoint(Node& refNode, unsigned offset) const
{
    // http://developer.mozilla.org/en/docs/DOM:range.comparePoint
    // This method returns -1, 0 or 1 depending on if the point described by the 
    // refNode node and an offset within the node is before, same as, or after the range respectively.
    if (&refNode.document() != &ownerDocument())
        return Exception { WrongDocumentError };

    auto checkNodeResult = checkNodeWOffset(refNode, offset);
    if (checkNodeResult.hasException()) {
        // DOM4 spec requires us to check whether refNode and start container have the same root first
        // but we do it in the reverse order to avoid O(n) operation here in common case.
        if (!refNode.isConnected() && !commonInclusiveAncestor(refNode, startContainer()))
            return Exception { WrongDocumentError };
        return checkNodeResult.releaseException();
    }

    // compare to start, and point comes before
    auto startCompareResult = compareBoundaryPoints(&refNode, offset, &startContainer(), m_start.offset());
    if (startCompareResult.hasException())
        return startCompareResult.releaseException();
    if (startCompareResult.releaseReturnValue() < 0)
        return -1;

    // compare to end, and point comes after
    auto endCompareResult = compareBoundaryPoints(&refNode, offset, &endContainer(), m_end.offset());
    if (endCompareResult.hasException())
        return endCompareResult.releaseException();
    if (endCompareResult.releaseReturnValue() > 0)
        return 1;

    // point is in the middle of this range, or on the boundary points
    return 0;
}

ExceptionOr<Range::CompareResults> Range::compareNode(Node& refNode) const
{
    // http://developer.mozilla.org/en/docs/DOM:range.compareNode
    // This method returns 0, 1, 2, or 3 based on if the node is before, after,
    // before and after(surrounds), or inside the range, respectively

    if (!refNode.isConnected()) {
        // Firefox doesn't throw an exception for this case; it returns 0.
        return NODE_BEFORE;
    }

    if (&refNode.document() != &ownerDocument()) {
        // Firefox doesn't throw an exception for this case; it returns 0.
        return NODE_BEFORE;
    }

    auto* parentNode = refNode.parentNode();
    if (!parentNode) {
        // If the node is the top of the tree we should return NODE_BEFORE_AND_AFTER,
        // but we throw to match firefox behavior.
        return Exception { NotFoundError };
    }
    auto nodeIndex = refNode.computeNodeIndex();

    auto nodeStartCompareResult = comparePoint(*parentNode, nodeIndex);
    if (nodeStartCompareResult.hasException())
        return nodeStartCompareResult.releaseException();
    auto nodeEndCompareResult = comparePoint(*parentNode, nodeIndex + 1);
    if (nodeEndCompareResult.hasException())
        return nodeEndCompareResult.releaseException();

    bool nodeStartsBeforeRange = nodeStartCompareResult.releaseReturnValue() < 0;
    bool nodeEndsAfterRange = nodeEndCompareResult.releaseReturnValue() > 0;

    return nodeStartsBeforeRange
        ? (nodeEndsAfterRange ? NODE_BEFORE_AND_AFTER : NODE_BEFORE)
        : (nodeEndsAfterRange ? NODE_AFTER : NODE_INSIDE);
}

static inline Node* top(Node& node)
{
    auto* top = &node;
    while (auto* parent = top->parentNode())
        top = parent;
    return top;
}

ExceptionOr<short> Range::compareBoundaryPoints(CompareHow how, const Range& sourceRange) const
{
    auto* thisContainer = commonAncestorContainer();
    auto* sourceContainer = sourceRange.commonAncestorContainer();
    if (!thisContainer || !sourceContainer || &thisContainer->document() != &sourceContainer->document() || top(*thisContainer) != top(*sourceContainer))
        return Exception { WrongDocumentError };

    switch (how) {
    case START_TO_START:
        return compareBoundaryPoints(m_start, sourceRange.m_start);
    case START_TO_END:
        return compareBoundaryPoints(m_end, sourceRange.m_start);
    case END_TO_END:
        return compareBoundaryPoints(m_end, sourceRange.m_end);
    case END_TO_START:
        return compareBoundaryPoints(m_start, sourceRange.m_end);
    }

    return Exception { SyntaxError };
}

ExceptionOr<short> Range::compareBoundaryPointsForBindings(unsigned short how, const Range& sourceRange) const
{
    switch (how) {
    case START_TO_START:
    case START_TO_END:
    case END_TO_END:
    case END_TO_START:
        return compareBoundaryPoints(static_cast<CompareHow>(how), sourceRange);
    }
    return Exception { NotSupportedError };
}

ExceptionOr<short> Range::compareBoundaryPoints(Node* containerA, unsigned offsetA, Node* containerB, unsigned offsetB)
{
    ASSERT(containerA);
    ASSERT(containerB);

    if (!containerA)
        return -1;
    if (!containerB)
        return 1;

    // see DOM2 traversal & range section 2.5

    // case 1: both points have the same container
    if (containerA == containerB) {
        if (offsetA == offsetB)
            return 0; // A is equal to B
        if (offsetA < offsetB)
            return -1; // A is before B
        return 1; // A is after B
    }

    // case 2: node C (container B or an ancestor) is a child node of A
    Node* c = containerB;
    while (c && c->parentNode() != containerA)
        c = c->parentNode();
    if (c) {
        unsigned offsetC = 0;
        Node* n = containerA->firstChild();
        while (n != c && offsetC < offsetA) {
            offsetC++;
            n = n->nextSibling();
        }
        if (offsetA <= offsetC)
            return -1; // A is before B
        return 1; // A is after B
    }

    // case 3: node C (container A or an ancestor) is a child node of B
    c = containerA;
    while (c && c->parentNode() != containerB)
        c = c->parentNode();
    if (c) {
        unsigned offsetC = 0;
        Node* n = containerB->firstChild();
        while (n != c && offsetC < offsetB) {
            offsetC++;
            n = n->nextSibling();
        }
        if (offsetC < offsetB)
            return -1; // A is before B
        return 1; // A is after B
    }

    // case 4: containers A & B are siblings, or children of siblings
    // ### we need to do a traversal here instead
    auto commonAncestor = commonInclusiveAncestor(*containerA, *containerB);
    if (!commonAncestor)
        return Exception { WrongDocumentError };
    Node* childA = containerA;
    while (childA && childA->parentNode() != commonAncestor)
        childA = childA->parentNode();
    if (!childA)
        childA = commonAncestor.get();
    Node* childB = containerB;
    while (childB && childB->parentNode() != commonAncestor)
        childB = childB->parentNode();
    if (!childB)
        childB = commonAncestor.get();

    if (childA == childB)
        return 0; // A is equal to B

    Node* n = commonAncestor->firstChild();
    while (n) {
        if (n == childA)
            return -1; // A is before B
        if (n == childB)
            return 1; // A is after B
        n = n->nextSibling();
    }

    // Should never reach this point.
    ASSERT_NOT_REACHED();
    return 0;
}

ExceptionOr<short> Range::compareBoundaryPoints(const RangeBoundaryPoint& boundaryA, const RangeBoundaryPoint& boundaryB)
{
    return compareBoundaryPoints(boundaryA.container(), boundaryA.offset(), boundaryB.container(), boundaryB.offset());
}

bool Range::boundaryPointsValid() const
{
    auto result = compareBoundaryPoints(m_start, m_end);
    return !result.hasException() && result.releaseReturnValue() <= 0;
}

ExceptionOr<void> Range::deleteContents()
{
    auto result = processContents(Delete);
    if (result.hasException())
        return result.releaseException();
    return { };
}

ExceptionOr<bool> Range::intersectsNode(Node& refNode) const
{
    if (!refNode.isConnected() || &refNode.document() != &ownerDocument())
        return false;

    auto* parentNode = refNode.parentNode();
    if (!parentNode)
        return true;

    unsigned nodeIndex = refNode.computeNodeIndex();

    // If (parentNode, nodeIndex) is before end and (parentNode, nodeIndex + 1) is after start, return true.
    // Otherwise, return false.
    auto compareEndResult = compareBoundaryPoints(parentNode, nodeIndex, m_end.container(), m_end.offset());
    if (compareEndResult.hasException())
        return compareEndResult.releaseException();
    auto compareStartResult = compareBoundaryPoints(parentNode, nodeIndex + 1, m_start.container(), m_start.offset());
    if (compareStartResult.hasException())
        return compareStartResult.releaseException();
    return compareEndResult.returnValue() == -1 && compareStartResult.returnValue() == 1;
}

static inline Node* highestAncestorUnderCommonRoot(Node* node, Node* commonRoot)
{
    if (node == commonRoot)
        return 0;

    ASSERT(commonRoot->contains(node));

    while (node->parentNode() != commonRoot)
        node = node->parentNode();

    return node;
}

static inline Node* childOfCommonRootBeforeOffset(Node* container, unsigned offset, Node* commonRoot)
{
    ASSERT(container);
    ASSERT(commonRoot);
    
    if (!commonRoot->contains(container))
        return 0;

    if (container == commonRoot) {
        container = container->firstChild();
        for (unsigned i = 0; container && i < offset; i++)
            container = container->nextSibling();
    } else {
        while (container->parentNode() != commonRoot)
            container = container->parentNode();
    }

    return container;
}

static inline unsigned lengthOfContentsInNode(Node& node)
{
    // This switch statement must be consistent with that of Range::processContentsBetweenOffsets.
    switch (node.nodeType()) {
    case Node::DOCUMENT_TYPE_NODE:
    case Node::ATTRIBUTE_NODE:
        return 0;
    case Node::TEXT_NODE:
    case Node::CDATA_SECTION_NODE:
    case Node::COMMENT_NODE:
    case Node::PROCESSING_INSTRUCTION_NODE:
        return downcast<CharacterData>(node).length();
    case Node::ELEMENT_NODE:
    case Node::DOCUMENT_NODE:
    case Node::DOCUMENT_FRAGMENT_NODE:
        return downcast<ContainerNode>(node).countChildNodes();
    }
    ASSERT_NOT_REACHED();
    return 0;
}

ExceptionOr<RefPtr<DocumentFragment>> Range::processContents(ActionType action)
{
    RefPtr<DocumentFragment> fragment;
    if (action == Extract || action == Clone)
        fragment = DocumentFragment::create(ownerDocument());

    if (collapsed())
        return fragment;

    RefPtr<Node> commonRoot = commonAncestorContainer();
    ASSERT(commonRoot);

    if (&startContainer() == &endContainer()) {
        auto result = processContentsBetweenOffsets(action, fragment, &startContainer(), m_start.offset(), m_end.offset());
        if (result.hasException())
            return result.releaseException();
        return fragment;
    }

    // Since mutation events can modify the range during the process, the boundary points need to be saved.
    RangeBoundaryPoint originalStart(m_start);
    RangeBoundaryPoint originalEnd(m_end);

    // what is the highest node that partially selects the start / end of the range?
    RefPtr<Node> partialStart = highestAncestorUnderCommonRoot(originalStart.container(), commonRoot.get());
    RefPtr<Node> partialEnd = highestAncestorUnderCommonRoot(originalEnd.container(), commonRoot.get());

    // Start and end containers are different.
    // There are three possibilities here:
    // 1. Start container == commonRoot (End container must be a descendant)
    // 2. End container == commonRoot (Start container must be a descendant)
    // 3. Neither is commonRoot, they are both descendants
    //
    // In case 3, we grab everything after the start (up until a direct child
    // of commonRoot) into leftContents, and everything before the end (up until
    // a direct child of commonRoot) into rightContents. Then we process all
    // commonRoot children between leftContents and rightContents
    //
    // In case 1 or 2, we skip either processing of leftContents or rightContents,
    // in which case the last lot of nodes either goes from the first or last
    // child of commonRoot.
    //
    // These are deleted, cloned, or extracted (i.e. both) depending on action.

    // Note that we are verifying that our common root hierarchy is still intact
    // after any DOM mutation event, at various stages below. See webkit bug 60350.

    RefPtr<Node> leftContents;
    if (originalStart.container() != commonRoot && commonRoot->contains(originalStart.container())) {
        auto firstResult = processContentsBetweenOffsets(action, nullptr, originalStart.container(), originalStart.offset(), lengthOfContentsInNode(*originalStart.container()));
        auto secondResult = processAncestorsAndTheirSiblings(action, originalStart.container(), ProcessContentsForward, WTFMove(firstResult), commonRoot.get());
        // FIXME: A bit peculiar that we silently ignore the exception here, but we do have at least some regression tests that rely on this behavior.
        if (!secondResult.hasException())
            leftContents = secondResult.releaseReturnValue();
    }

    RefPtr<Node> rightContents;
    if (&endContainer() != commonRoot && commonRoot->contains(originalEnd.container())) {
        auto firstResult = processContentsBetweenOffsets(action, nullptr, originalEnd.container(), 0, originalEnd.offset());
        auto secondResult = processAncestorsAndTheirSiblings(action, originalEnd.container(), ProcessContentsBackward, WTFMove(firstResult), commonRoot.get());
        // FIXME: A bit peculiar that we silently ignore the exception here, but we do have at least some regression tests that rely on this behavior.
        if (!secondResult.hasException())
            rightContents = secondResult.releaseReturnValue();
    }

    // delete all children of commonRoot between the start and end container
    RefPtr<Node> processStart = childOfCommonRootBeforeOffset(originalStart.container(), originalStart.offset(), commonRoot.get());
    if (processStart && originalStart.container() != commonRoot) // processStart contains nodes before m_start.
        processStart = processStart->nextSibling();
    RefPtr<Node> processEnd = childOfCommonRootBeforeOffset(originalEnd.container(), originalEnd.offset(), commonRoot.get());

    // Collapse the range, making sure that the result is not within a node that was partially selected.
    if (action == Extract || action == Delete) {
        if (partialStart && commonRoot->contains(partialStart.get())) {
            auto result = setStart(*partialStart->parentNode(), partialStart->computeNodeIndex() + 1);
            if (result.hasException())
                return result.releaseException();
        } else if (partialEnd && commonRoot->contains(partialEnd.get())) {
            auto result = setStart(*partialEnd->parentNode(), partialEnd->computeNodeIndex());
            if (result.hasException())
                return result.releaseException();
        }
        m_end = m_start;
    }

    // Now add leftContents, stuff in between, and rightContents to the fragment
    // (or just delete the stuff in between)

    if ((action == Extract || action == Clone) && leftContents) {
        auto result = fragment->appendChild(*leftContents);
        if (result.hasException())
            return result.releaseException();
    }

    if (processStart) {
        Vector<Ref<Node>> nodes;
        for (Node* node = processStart.get(); node && node != processEnd; node = node->nextSibling())
            nodes.append(*node);
        auto result = processNodes(action, nodes, commonRoot.get(), fragment.get());
        if (result.hasException())
            return result.releaseException();
    }

    if ((action == Extract || action == Clone) && rightContents) {
        auto result = fragment->appendChild(*rightContents);
        if (result.hasException())
            return result.releaseException();
    }

    return fragment;
}

static inline ExceptionOr<void> deleteCharacterData(CharacterData& data, unsigned startOffset, unsigned endOffset)
{
    if (data.length() - endOffset) {
        auto result = data.deleteData(endOffset, data.length() - endOffset);
        if (result.hasException())
            return result.releaseException();
    }
    if (startOffset) {
        auto result = data.deleteData(0, startOffset);
        if (result.hasException())
            return result.releaseException();
    }
    return { };
}

static ExceptionOr<RefPtr<Node>> processContentsBetweenOffsets(Range::ActionType action, RefPtr<DocumentFragment> fragment, RefPtr<Node> container, unsigned startOffset, unsigned endOffset)
{
    ASSERT(container);
    ASSERT(startOffset <= endOffset);

    RefPtr<Node> result;

    // This switch statement must be consistent with that of lengthOfContentsInNode.
    switch (container->nodeType()) {
    case Node::TEXT_NODE:
    case Node::CDATA_SECTION_NODE:
    case Node::COMMENT_NODE:
        endOffset = std::min(endOffset, downcast<CharacterData>(*container).length());
        startOffset = std::min(startOffset, endOffset);
        if (action == Range::Extract || action == Range::Clone) {
            Ref<CharacterData> characters = downcast<CharacterData>(container->cloneNode(true).get());
            auto deleteResult = deleteCharacterData(characters, startOffset, endOffset);
            if (deleteResult.hasException())
                return deleteResult.releaseException();
            if (fragment) {
                result = fragment;
                auto appendResult = result->appendChild(characters);
                if (appendResult.hasException())
                    return appendResult.releaseException();
            } else
                result = WTFMove(characters);
        }
        if (action == Range::Extract || action == Range::Delete) {
            auto deleteResult = downcast<CharacterData>(*container).deleteData(startOffset, endOffset - startOffset);
            if (deleteResult.hasException())
                return deleteResult.releaseException();
        }
        break;
    case Node::PROCESSING_INSTRUCTION_NODE: {
        auto& instruction = downcast<ProcessingInstruction>(*container);
        endOffset = std::min(endOffset, downcast<ProcessingInstruction>(*container).data().length());
        startOffset = std::min(startOffset, endOffset);
        if (action == Range::Extract || action == Range::Clone) {
            Ref<ProcessingInstruction> processingInstruction = downcast<ProcessingInstruction>(container->cloneNode(true).get());
            processingInstruction->setData(processingInstruction->data().substring(startOffset, endOffset - startOffset));
            if (fragment) {
                result = fragment;
                auto appendResult = result->appendChild(processingInstruction);
                if (appendResult.hasException())
                    return appendResult.releaseException();
            } else
                result = WTFMove(processingInstruction);
        }
        if (action == Range::Extract || action == Range::Delete) {
            String data { instruction.data() };
            data.remove(startOffset, endOffset - startOffset);
            instruction.setData(data);
        }
        break;
    }
    case Node::ELEMENT_NODE:
    case Node::ATTRIBUTE_NODE:
    case Node::DOCUMENT_NODE:
    case Node::DOCUMENT_TYPE_NODE:
    case Node::DOCUMENT_FRAGMENT_NODE:
        // FIXME: Should we assert that some nodes never appear here?
        if (action == Range::Extract || action == Range::Clone) {
            if (fragment)
                result = fragment;
            else
                result = container->cloneNode(false);
        }
        Vector<Ref<Node>> nodes;
        Node* n = container->firstChild();
        for (unsigned i = startOffset; n && i; i--)
            n = n->nextSibling();
        for (unsigned i = startOffset; n && i < endOffset; i++, n = n->nextSibling()) {
            if (action != Range::Delete && n->isDocumentTypeNode()) {
                return Exception { HierarchyRequestError };
            }
            nodes.append(*n);
        }
        auto processResult = processNodes(action, nodes, container.get(), result);
        if (processResult.hasException())
            return processResult.releaseException();
        break;
    }

    return result;
}

static ExceptionOr<void> processNodes(Range::ActionType action, Vector<Ref<Node>>& nodes, Node* oldContainer, RefPtr<Node> newContainer)
{
    for (auto& node : nodes) {
        switch (action) {
        case Range::Delete: {
            auto result = oldContainer->removeChild(node);
            if (result.hasException())
                return result.releaseException();
            break;
        }
        case Range::Extract: {
            auto result = newContainer->appendChild(node); // will remove node from its parent
            if (result.hasException())
                return result.releaseException();
            break;
        }
        case Range::Clone: {
            auto result = newContainer->appendChild(node->cloneNode(true));
            if (result.hasException())
                return result.releaseException();
            break;
        }
        }
    }
    return { };
}

ExceptionOr<RefPtr<Node>> processAncestorsAndTheirSiblings(Range::ActionType action, Node* container, ContentsProcessDirection direction, ExceptionOr<RefPtr<Node>>&& passedClonedContainer, Node* commonRoot)
{
    if (passedClonedContainer.hasException())
        return WTFMove(passedClonedContainer);

    RefPtr<Node> clonedContainer = passedClonedContainer.releaseReturnValue();

    Vector<Ref<ContainerNode>> ancestors;
    for (ContainerNode* ancestor = container->parentNode(); ancestor && ancestor != commonRoot; ancestor = ancestor->parentNode())
        ancestors.append(*ancestor);

    RefPtr<Node> firstChildInAncestorToProcess = direction == ProcessContentsForward ? container->nextSibling() : container->previousSibling();
    for (auto& ancestor : ancestors) {
        if (action == Range::Extract || action == Range::Clone) {
            auto clonedAncestor = ancestor->cloneNode(false); // Might have been removed already during mutation event.
            if (clonedContainer) {
                auto result = clonedAncestor->appendChild(*clonedContainer);
                if (result.hasException())
                    return result.releaseException();
            }
            clonedContainer = WTFMove(clonedAncestor);
        }

        // Copy siblings of an ancestor of start/end containers
        // FIXME: This assertion may fail if DOM is modified during mutation event
        // FIXME: Share code with Range::processNodes
        ASSERT(!firstChildInAncestorToProcess || firstChildInAncestorToProcess->parentNode() == ancestor.ptr());
        
        Vector<Ref<Node>> nodes;
        for (Node* child = firstChildInAncestorToProcess.get(); child;
            child = (direction == ProcessContentsForward) ? child->nextSibling() : child->previousSibling())
            nodes.append(*child);

        for (auto& child : nodes) {
            switch (action) {
            case Range::Delete: {
                auto result = ancestor->removeChild(child);
                if (result.hasException())
                    return result.releaseException();
                break;
            }
            case Range::Extract: // will remove child from ancestor
                if (direction == ProcessContentsForward) {
                    auto result = clonedContainer->appendChild(child);
                    if (result.hasException())
                        return result.releaseException();
                } else {
                    auto result = clonedContainer->insertBefore(child, clonedContainer->firstChild());
                    if (result.hasException())
                        return result.releaseException();
                }
                break;
            case Range::Clone:
                if (direction == ProcessContentsForward) {
                    auto result = clonedContainer->appendChild(child->cloneNode(true));
                    if (result.hasException())
                        return result.releaseException();
                } else {
                    auto result = clonedContainer->insertBefore(child->cloneNode(true), clonedContainer->firstChild());
                    if (result.hasException())
                        return result.releaseException();
                }
                break;
            }
        }
        firstChildInAncestorToProcess = direction == ProcessContentsForward ? ancestor->nextSibling() : ancestor->previousSibling();
    }

    return clonedContainer;
}

ExceptionOr<Ref<DocumentFragment>> Range::extractContents()
{
    auto result = processContents(Extract);
    if (result.hasException())
        return result.releaseException();
    return result.releaseReturnValue().releaseNonNull();
}

ExceptionOr<Ref<DocumentFragment>> Range::cloneContents()
{
    auto result = processContents(Clone);
    if (result.hasException())
        return result.releaseException();
    return result.releaseReturnValue().releaseNonNull();
}

ExceptionOr<void> Range::insertNode(Ref<Node>&& node)
{
    auto startContainerNodeType = startContainer().nodeType();

    if (startContainerNodeType == Node::COMMENT_NODE || startContainerNodeType == Node::PROCESSING_INSTRUCTION_NODE)
        return Exception { HierarchyRequestError };
    bool startIsText = startContainerNodeType == Node::TEXT_NODE;
    if (startIsText && !startContainer().parentNode())
        return Exception { HierarchyRequestError };
    if (node.ptr() == &startContainer())
        return Exception { HierarchyRequestError };

    RefPtr<Node> referenceNode = startIsText ? &startContainer() : startContainer().traverseToChildAt(startOffset());
    Node* parentNode = referenceNode ? referenceNode->parentNode() : &startContainer();
    if (!is<ContainerNode>(parentNode))
        return Exception { HierarchyRequestError };

    Ref<ContainerNode> parent = downcast<ContainerNode>(*parentNode);

    auto result = parent->ensurePreInsertionValidity(node, referenceNode.get());
    if (result.hasException())
        return result.releaseException();

    EventQueueScope scope;
    if (startIsText) {
        auto result = downcast<Text>(startContainer()).splitText(startOffset());
        if (result.hasException())
            return result.releaseException();
        referenceNode = result.releaseReturnValue();
    }

    if (referenceNode == node.ptr())
        referenceNode = referenceNode->nextSibling();

    auto removeResult = node->remove();
    if (removeResult.hasException())
        return removeResult.releaseException();

    unsigned newOffset = referenceNode ? referenceNode->computeNodeIndex() : parent->countChildNodes();
    if (is<DocumentFragment>(node))
        newOffset += downcast<DocumentFragment>(node.get()).countChildNodes();
    else
        ++newOffset;

    auto insertResult = parent->insertBefore(node, referenceNode.get());
    if (insertResult.hasException())
        return insertResult.releaseException();

    if (collapsed())
        return setEnd(WTFMove(parent), newOffset);

    return { };
}

String Range::toString() const
{
    StringBuilder builder;

    Node* pastLast = pastLastNode();
    for (Node* node = firstNode(); node != pastLast; node = NodeTraversal::next(*node)) {
        auto type = node->nodeType();
        if (type == Node::TEXT_NODE || type == Node::CDATA_SECTION_NODE) {
            unsigned start = node == &startContainer() ? m_start.offset() : 0U;
            unsigned end = node == &endContainer() ? std::max(start, m_end.offset()) : std::numeric_limits<unsigned>::max();
            builder.appendSubstring(downcast<CharacterData>(*node).data(), start, end - start);
        }
    }

    return builder.toString();
}

String Range::text() const
{
    // We need to update layout, since plainText uses line boxes in the render tree.
    // FIXME: As with innerText, we'd like this to work even if there are no render objects.
    startContainer().document().updateLayout();

    return plainText(*this);
}

// https://w3c.github.io/DOM-Parsing/#widl-Range-createContextualFragment-DocumentFragment-DOMString-fragment
ExceptionOr<Ref<DocumentFragment>> Range::createContextualFragment(const String& markup)
{
    Node& node = startContainer();
    RefPtr<Element> element;
    if (is<Document>(node) || is<DocumentFragment>(node))
        element = nullptr;
    else if (is<Element>(node))
        element = &downcast<Element>(node);
    else
        element = node.parentElement();
    if (!element || (element->document().isHTMLDocument() && is<HTMLHtmlElement>(*element)))
        element = HTMLBodyElement::create(node.document());
    return WebCore::createContextualFragment(*element, markup, AllowScriptingContentAndDoNotMarkAlreadyStarted);
}

void Range::detach()
{
    // This is now a no-op as per the DOM specification.
}

ExceptionOr<Node*> Range::checkNodeWOffset(Node& node, unsigned offset) const
{
    switch (node.nodeType()) {
        case Node::DOCUMENT_TYPE_NODE:
            return Exception { InvalidNodeTypeError };
        case Node::CDATA_SECTION_NODE:
        case Node::COMMENT_NODE:
        case Node::TEXT_NODE:
        case Node::PROCESSING_INSTRUCTION_NODE:
            if (offset > downcast<CharacterData>(node).length())
                return Exception { IndexSizeError };
            return nullptr;
        case Node::ATTRIBUTE_NODE:
        case Node::DOCUMENT_FRAGMENT_NODE:
        case Node::DOCUMENT_NODE:
        case Node::ELEMENT_NODE: {
            if (!offset)
                return nullptr;
            Node* childBefore = node.traverseToChildAt(offset - 1);
            if (!childBefore)
                return Exception { IndexSizeError };
            return childBefore;
        }
    }
    ASSERT_NOT_REACHED();
    return Exception { InvalidNodeTypeError };
}

Ref<Range> Range::cloneRange() const
{
    return Range::create(ownerDocument(), &startContainer(), m_start.offset(), &endContainer(), m_end.offset());
}

ExceptionOr<void> Range::setStartAfter(Node& refNode)
{
    if (!refNode.parentNode())
        return Exception { InvalidNodeTypeError };
    return setStart(*refNode.parentNode(), refNode.computeNodeIndex() + 1);
}

ExceptionOr<void> Range::setEndBefore(Node& refNode)
{
    if (!refNode.parentNode())
        return Exception { InvalidNodeTypeError };
    return setEnd(*refNode.parentNode(), refNode.computeNodeIndex());
}

ExceptionOr<void> Range::setEndAfter(Node& refNode)
{
    if (!refNode.parentNode())
        return Exception { InvalidNodeTypeError };
    return setEnd(*refNode.parentNode(), refNode.computeNodeIndex() + 1);
}

ExceptionOr<void> Range::selectNode(Node& refNode)
{
    if (!refNode.parentNode())
        return Exception { InvalidNodeTypeError };

    if (&ownerDocument() != &refNode.document())
        setDocument(refNode.document());

    unsigned index = refNode.computeNodeIndex();
    auto result = setStart(*refNode.parentNode(), index);
    if (result.hasException())
        return result.releaseException();
    return setEnd(*refNode.parentNode(), index + 1);
}

ExceptionOr<void> Range::selectNodeContents(Node& refNode)
{
    if (refNode.isDocumentTypeNode())
        return Exception { InvalidNodeTypeError };

    if (&ownerDocument() != &refNode.document())
        setDocument(refNode.document());

    m_start.setToStartOfNode(refNode);
    m_end.setToEndOfNode(refNode);

    return { };
}

// https://dom.spec.whatwg.org/#dom-range-surroundcontents
ExceptionOr<void> Range::surroundContents(Node& newParent)
{
    Ref<Node> protectedNewParent(newParent);

    // Step 1: If a non-Text node is partially contained in the context object, then throw an InvalidStateError.
    Node* startNonTextContainer = &startContainer();
    if (startNonTextContainer->nodeType() == Node::TEXT_NODE)
        startNonTextContainer = startNonTextContainer->parentNode();
    Node* endNonTextContainer = &endContainer();
    if (endNonTextContainer->nodeType() == Node::TEXT_NODE)
        endNonTextContainer = endNonTextContainer->parentNode();
    if (startNonTextContainer != endNonTextContainer)
        return Exception { InvalidStateError };

    // Step 2: If newParent is a Document, DocumentType, or DocumentFragment node, then throw an InvalidNodeTypeError.
    switch (newParent.nodeType()) {
        case Node::ATTRIBUTE_NODE:
        case Node::DOCUMENT_FRAGMENT_NODE:
        case Node::DOCUMENT_NODE:
        case Node::DOCUMENT_TYPE_NODE:
            return Exception { InvalidNodeTypeError };
        case Node::CDATA_SECTION_NODE:
        case Node::COMMENT_NODE:
        case Node::ELEMENT_NODE:
        case Node::PROCESSING_INSTRUCTION_NODE:
        case Node::TEXT_NODE:
            break;
    }

    // Step 3: Let fragment be the result of extracting context object.
    auto fragment = extractContents();
    if (fragment.hasException())
        return fragment.releaseException();

    // Step 4: If newParent has children, replace all with null within newParent.
    if (newParent.hasChildNodes())
        downcast<ContainerNode>(newParent).replaceAllChildren(nullptr);

    // Step 5: Insert newParent into context object.
    auto insertResult = insertNode(newParent);
    if (insertResult.hasException())
        return insertResult.releaseException();

    // Step 6: Append fragment to newParent.
    auto appendResult = newParent.appendChild(fragment.releaseReturnValue());
    if (appendResult.hasException())
        return appendResult.releaseException();

    // Step 7: Select newParent within context object.
    return selectNode(newParent);
}

ExceptionOr<void> Range::setStartBefore(Node& refNode)
{
    if (!refNode.parentNode())
        return Exception { InvalidNodeTypeError };
    return setStart(*refNode.parentNode(), refNode.computeNodeIndex());
}

Node* Range::firstNode() const
{
    if (startContainer().isCharacterDataNode())
        return &startContainer();
    if (Node* child = startContainer().traverseToChildAt(m_start.offset()))
        return child;
    if (!m_start.offset())
        return &startContainer();
    return NodeTraversal::nextSkippingChildren(startContainer());
}

ShadowRoot* Range::shadowRoot() const
{
    return startContainer().containingShadowRoot();
}

Node* Range::pastLastNode() const
{
    if (endContainer().isCharacterDataNode())
        return NodeTraversal::nextSkippingChildren(endContainer());
    if (Node* child = endContainer().traverseToChildAt(m_end.offset()))
        return child;
    return NodeTraversal::nextSkippingChildren(endContainer());
}

IntRect Range::absoluteBoundingBox(OptionSet<BoundingRectBehavior> rectOptions) const
{
    Vector<IntRect> rects;
    absoluteTextRects(rects, false, rectOptions);
    IntRect result;
    for (auto& rect : rects)
        result.unite(rect);
    return result;
}

Vector<FloatRect> Range::absoluteRectsForRangeInText(Node* node, RenderText& renderText, bool useSelectionHeight, bool& isFixed, OptionSet<BoundingRectBehavior> rectOptions) const
{
    unsigned startOffset = node == &startContainer() ? m_start.offset() : 0;
    unsigned endOffset = node == &endContainer() ? m_end.offset() : std::numeric_limits<unsigned>::max();

    auto textQuads = renderText.absoluteQuadsForRange(startOffset, endOffset, useSelectionHeight, rectOptions.contains(BoundingRectBehavior::IgnoreEmptyTextSelections), &isFixed);

    if (rectOptions.contains(BoundingRectBehavior::RespectClipping)) {
        Vector<FloatRect> clippedRects;
        clippedRects.reserveInitialCapacity(textQuads.size());

        auto absoluteClippedOverflowRect = renderText.absoluteClippedOverflowRect();

        for (auto& quad : textQuads) {
            auto clippedRect = intersection(quad.boundingBox(), absoluteClippedOverflowRect);
            if (!clippedRect.isEmpty())
                clippedRects.uncheckedAppend(clippedRect);
        }

        return clippedRects;
    }

    return boundingBoxes(textQuads);
}

void Range::absoluteTextRects(Vector<IntRect>& rects, bool useSelectionHeight, OptionSet<BoundingRectBehavior> rectOptions) const
{
    // FIXME: This function should probably return FloatRects.

    bool allFixed = true;
    bool someFixed = false;

    Node* stopNode = pastLastNode();
    for (Node* node = firstNode(); node != stopNode; node = NodeTraversal::next(*node)) {
        RenderObject* renderer = node->renderer();
        if (!renderer)
            continue;
        bool isFixed = false;
        if (renderer->isBR())
            renderer->absoluteRects(rects, flooredLayoutPoint(renderer->localToAbsolute()));
        else if (is<RenderText>(*renderer)) {
            auto rectsForRenderer = absoluteRectsForRangeInText(node, downcast<RenderText>(*renderer), useSelectionHeight, isFixed, rectOptions);
            for (auto& rect : rectsForRenderer)
                rects.append(enclosingIntRect(rect));
        } else
            continue;
        allFixed &= isFixed;
        someFixed |= isFixed;
    }
}

#if PLATFORM(IOS_FAMILY)
static bool intervalsSufficientlyOverlap(int startA, int endA, int startB, int endB)
{
    if (endA <= startA || endB <= startB)
        return false;

    const float sufficientOverlap = .75;

    int lengthA = endA - startA;
    int lengthB = endB - startB;

    int maxStart = std::max(startA, startB);
    int minEnd = std::min(endA, endB);

    if (maxStart > minEnd)
        return false;

    return minEnd - maxStart >= sufficientOverlap * std::min(lengthA, lengthB);
}

static inline void adjustLineHeightOfSelectionRects(Vector<SelectionRect>& rects, size_t numberOfRects, int lineNumber, int lineTop, int lineHeight)
{
    ASSERT(rects.size() >= numberOfRects);
    for (size_t i = numberOfRects; i; ) {
        --i;
        if (rects[i].lineNumber())
            break;
        rects[i].setLineNumber(lineNumber);
        rects[i].setLogicalTop(lineTop);
        rects[i].setLogicalHeight(lineHeight);
    }
}

static SelectionRect coalesceSelectionRects(const SelectionRect& original, const SelectionRect& previous)
{
    SelectionRect result(unionRect(previous.rect(), original.rect()), original.isHorizontal(), original.pageNumber());
    result.setDirection(original.containsStart() || original.containsEnd() ? original.direction() : previous.direction());
    result.setContainsStart(previous.containsStart() || original.containsStart());
    result.setContainsEnd(previous.containsEnd() || original.containsEnd());
    result.setIsFirstOnLine(previous.isFirstOnLine() || original.isFirstOnLine());
    result.setIsLastOnLine(previous.isLastOnLine() || original.isLastOnLine());
    return result;
}

// This function is similar in spirit to addLineBoxRects, but annotates the returned rectangles
// with additional state which helps iOS draw selections in its unique way.
int Range::collectSelectionRectsWithoutUnionInteriorLines(Vector<SelectionRect>& rects) const
{
    auto& startContainer = this->startContainer();
    auto& endContainer = this->endContainer();
    int startOffset = m_start.offset();
    int endOffset = m_end.offset();

    Vector<SelectionRect> newRects;
    Node* stopNode = pastLastNode();
    bool hasFlippedWritingMode = startContainer.renderer() && startContainer.renderer()->style().isFlippedBlocksWritingMode();
    bool containsDifferentWritingModes = false;
    for (Node* node = firstNode(); node && node != stopNode; node = NodeTraversal::next(*node)) {
        RenderObject* renderer = node->renderer();
        // Only ask leaf render objects for their line box rects.
        if (renderer && !renderer->firstChildSlow() && renderer->style().userSelect() != UserSelect::None) {
            bool isStartNode = renderer->node() == &startContainer;
            bool isEndNode = renderer->node() == &endContainer;
            if (hasFlippedWritingMode != renderer->style().isFlippedBlocksWritingMode())
                containsDifferentWritingModes = true;
            // FIXME: Sending 0 for the startOffset is a weird way of telling the renderer that the selection
            // doesn't start inside it, since we'll also send 0 if the selection *does* start in it, at offset 0.
            //
            // FIXME: Selection endpoints aren't always inside leaves, and we only build SelectionRects for leaves,
            // so we can't accurately determine which SelectionRects contain the selection start and end using
            // only the offsets of the start and end. We need to pass the whole Range.
            int beginSelectionOffset = isStartNode ? startOffset : 0;
            int endSelectionOffset = isEndNode ? endOffset : std::numeric_limits<int>::max();
            renderer->collectSelectionRects(newRects, beginSelectionOffset, endSelectionOffset);
            for (auto& selectionRect : newRects) {
                if (selectionRect.containsStart() && !isStartNode)
                    selectionRect.setContainsStart(false);
                if (selectionRect.containsEnd() && !isEndNode)
                    selectionRect.setContainsEnd(false);
                if (selectionRect.logicalWidth() || selectionRect.logicalHeight())
                    rects.append(selectionRect);
            }
            newRects.shrink(0);
        }
    }

    // The range could span nodes with different writing modes.
    // If this is the case, we use the writing mode of the common ancestor.
    if (containsDifferentWritingModes) {
        if (auto ancestor = commonInclusiveAncestor(startContainer, endContainer))
            hasFlippedWritingMode = ancestor->renderer()->style().isFlippedBlocksWritingMode();
    }

    const size_t numberOfRects = rects.size();

    // If the selection ends in a BR, then add the line break bit to the last
    // rect we have. This will cause its selection rect to extend to the
    // end of the line.
    if (stopNode && stopNode->hasTagName(HTMLNames::brTag) && numberOfRects) {
        // Only set the line break bit if the end of the range actually
        // extends all the way to include the <br>. VisiblePosition helps to
        // figure this out.
        VisiblePosition endPosition(createLegacyEditingPosition(&endContainer, endOffset), VP_DEFAULT_AFFINITY);
        VisiblePosition brPosition(createLegacyEditingPosition(stopNode, 0), VP_DEFAULT_AFFINITY);
        if (endPosition == brPosition)
            rects.last().setIsLineBreak(true);
    }

    int lineTop = std::numeric_limits<int>::max();
    int lineBottom = std::numeric_limits<int>::min();
    int lastLineTop = lineTop;
    int lastLineBottom = lineBottom;
    int lineNumber = 0;

    for (size_t i = 0; i < numberOfRects; ++i) {
        int currentRectTop = rects[i].logicalTop();
        int currentRectBottom = currentRectTop + rects[i].logicalHeight();

        // We don't want to count the ruby text as a separate line.
        if (intervalsSufficientlyOverlap(currentRectTop, currentRectBottom, lineTop, lineBottom) || (i && rects[i].isRubyText())) {
            // Grow the current line bounds.
            lineTop = std::min(lineTop, currentRectTop);
            lineBottom = std::max(lineBottom, currentRectBottom);
            // Avoid overlap with the previous line.
            if (!hasFlippedWritingMode)
                lineTop = std::max(lastLineBottom, lineTop);
            else
                lineBottom = std::min(lastLineTop, lineBottom);
        } else {
            adjustLineHeightOfSelectionRects(rects, i, lineNumber, lineTop, lineBottom - lineTop);
            if (!hasFlippedWritingMode) {
                lastLineTop = lineTop;
                if (currentRectBottom >= lastLineTop) {
                    lastLineBottom = lineBottom;
                    lineTop = lastLineBottom;
                } else {
                    lineTop = currentRectTop;
                    lastLineBottom = std::numeric_limits<int>::min();
                }
                lineBottom = currentRectBottom;
            } else {
                lastLineBottom = lineBottom;
                if (currentRectTop <= lastLineBottom && i && rects[i].pageNumber() == rects[i - 1].pageNumber()) {
                    lastLineTop = lineTop;
                    lineBottom = lastLineTop;
                } else {
                    lastLineTop = std::numeric_limits<int>::max();
                    lineBottom = currentRectBottom;
                }
                lineTop = currentRectTop;
            }
            ++lineNumber;
        }
    }

    // Adjust line height.
    adjustLineHeightOfSelectionRects(rects, numberOfRects, lineNumber, lineTop, lineBottom - lineTop);

    // Sort the rectangles and make sure there are no gaps. The rectangles could be unsorted when
    // there is ruby text and we could have gaps on the line when adjacent elements on the line
    // have a different orientation.
    size_t firstRectWithCurrentLineNumber = 0;
    for (size_t currentRect = 1; currentRect < numberOfRects; ++currentRect) {
        if (rects[currentRect].lineNumber() != rects[currentRect - 1].lineNumber()) {
            firstRectWithCurrentLineNumber = currentRect;
            continue;
        }
        if (rects[currentRect].logicalLeft() >= rects[currentRect - 1].logicalLeft())
            continue;

        SelectionRect selectionRect = rects[currentRect];
        size_t i;
        for (i = currentRect; i > firstRectWithCurrentLineNumber && selectionRect.logicalLeft() < rects[i - 1].logicalLeft(); --i)
            rects[i] = rects[i - 1];
        rects[i] = selectionRect;
    }

    for (size_t j = 1; j < numberOfRects; ++j) {
        if (rects[j].lineNumber() != rects[j - 1].lineNumber())
            continue;
        SelectionRect& previousRect = rects[j - 1];
        bool previousRectMayNotReachRightEdge = (previousRect.direction() == TextDirection::LTR && previousRect.containsEnd()) || (previousRect.direction() == TextDirection::RTL && previousRect.containsStart());
        if (previousRectMayNotReachRightEdge)
            continue;
        int adjustedWidth = rects[j].logicalLeft() - previousRect.logicalLeft();
        if (adjustedWidth > previousRect.logicalWidth())
            previousRect.setLogicalWidth(adjustedWidth);
    }

    int maxLineNumber = lineNumber;

    // Extend rects out to edges as needed.
    for (size_t i = 0; i < numberOfRects; ++i) {
        SelectionRect& selectionRect = rects[i];
        if (!selectionRect.isLineBreak() && selectionRect.lineNumber() >= maxLineNumber)
            continue;
        if (selectionRect.direction() == TextDirection::RTL && selectionRect.isFirstOnLine()) {
            selectionRect.setLogicalWidth(selectionRect.logicalWidth() + selectionRect.logicalLeft() - selectionRect.minX());
            selectionRect.setLogicalLeft(selectionRect.minX());
        } else if (selectionRect.direction() == TextDirection::LTR && selectionRect.isLastOnLine())
            selectionRect.setLogicalWidth(selectionRect.maxX() - selectionRect.logicalLeft());
    }
    
    return maxLineNumber;
}

void Range::collectSelectionRects(Vector<SelectionRect>& rects) const
{
    int maxLineNumber = collectSelectionRectsWithoutUnionInteriorLines(rects);
    const size_t numberOfRects = rects.size();
    
    // Union all the rectangles on interior lines (i.e. not first or last).
    // On first and last lines, just avoid having overlaps by merging intersecting rectangles.
    Vector<SelectionRect> unionedRects;
    IntRect interiorUnionRect;
    for (size_t i = 0; i < numberOfRects; ++i) {
        SelectionRect& currentRect = rects[i];
        if (currentRect.lineNumber() == 1) {
            ASSERT(interiorUnionRect.isEmpty());
            if (!unionedRects.isEmpty()) {
                SelectionRect& previousRect = unionedRects.last();
                if (previousRect.rect().intersects(currentRect.rect())) {
                    previousRect = coalesceSelectionRects(currentRect, previousRect);
                    continue;
                }
            }
            // Couldn't merge with previous rect, so just appending.
            unionedRects.append(currentRect);
        } else if (currentRect.lineNumber() < maxLineNumber) {
            if (interiorUnionRect.isEmpty()) {
                // Start collecting interior rects.
                interiorUnionRect = currentRect.rect();         
            } else if (interiorUnionRect.intersects(currentRect.rect())
                || interiorUnionRect.maxX() == currentRect.rect().x()
                || interiorUnionRect.maxY() == currentRect.rect().y()
                || interiorUnionRect.x() == currentRect.rect().maxX()
                || interiorUnionRect.y() == currentRect.rect().maxY()) {
                // Only union the lines that are attached.
                // For iBooks, the interior lines may cross multiple horizontal pages.
                interiorUnionRect.unite(currentRect.rect());
            } else {
                unionedRects.append(SelectionRect(interiorUnionRect, currentRect.isHorizontal(), currentRect.pageNumber()));
                interiorUnionRect = currentRect.rect();
            }
        } else {
            // Processing last line.
            if (!interiorUnionRect.isEmpty()) {
                unionedRects.append(SelectionRect(interiorUnionRect, currentRect.isHorizontal(), currentRect.pageNumber()));
                interiorUnionRect = IntRect();
            }

            ASSERT(!unionedRects.isEmpty());
            SelectionRect& previousRect = unionedRects.last();
            if (previousRect.logicalTop() == currentRect.logicalTop() && previousRect.rect().intersects(currentRect.rect())) {
                // previousRect is also on the last line, and intersects the current one.
                previousRect = coalesceSelectionRects(currentRect, previousRect);
                continue;
            }
            // Couldn't merge with previous rect, so just appending.
            unionedRects.append(currentRect);
        }
    }

    rects.swap(unionedRects);
}
#endif

#if ENABLE(TREE_DEBUGGING)
void Range::formatForDebugger(char* buffer, unsigned length) const
{
    StringBuilder result;

    const int FormatBufferSize = 1024;
    char s[FormatBufferSize];
    result.appendLiteral("from offset ");
    result.appendNumber(m_start.offset());
    result.appendLiteral(" of ");
    startContainer().formatForDebugger(s, FormatBufferSize);
    result.append(s);
    result.appendLiteral(" to offset ");
    result.appendNumber(m_end.offset());
    result.appendLiteral(" of ");
    endContainer().formatForDebugger(s, FormatBufferSize);
    result.append(s);

    strncpy(buffer, result.toString().utf8().data(), length - 1);
}
#endif

bool Range::contains(const Range& other) const
{
    if (commonAncestorContainer()->ownerDocument() != other.commonAncestorContainer()->ownerDocument())
        return false;

    auto startToStart = compareBoundaryPoints(Range::START_TO_START, other);
    if (startToStart.hasException() || startToStart.releaseReturnValue() > 0)
        return false;

    auto endToEnd = compareBoundaryPoints(Range::END_TO_END, other);
    return !endToEnd.hasException() && endToEnd.releaseReturnValue() >= 0;
}

bool Range::contains(const VisiblePosition& position) const
{
    RefPtr<Range> positionRange = makeRange(position, position);
    if (!positionRange)
        return false;
    return contains(*positionRange);
}

bool areRangesEqual(const Range* a, const Range* b)
{
    if (a == b)
        return true;
    if (!a || !b)
        return false;
    return a->startPosition() == b->startPosition() && a->endPosition() == b->endPosition();
}

bool rangesOverlap(const Range* a, const Range* b)
{
    if (!a || !b)
        return false;

    if (a == b)
        return true;

    if (!areNodesConnectedInSameTreeScope(a->commonAncestorContainer(), b->commonAncestorContainer()))
        return false;

    short startToStart = a->compareBoundaryPoints(Range::START_TO_START, *b).releaseReturnValue();
    short endToEnd = a->compareBoundaryPoints(Range::END_TO_END, *b).releaseReturnValue();

    // First range contains the second range.
    if (startToStart <= 0 && endToEnd >= 0)
        return true;

    // End of first range is inside second range.
    if (a->compareBoundaryPoints(Range::START_TO_END, *b).releaseReturnValue() >= 0 && endToEnd <= 0)
        return true;

    // Start of first range is inside second range.
    if (startToStart >= 0 && a->compareBoundaryPoints(Range::END_TO_START, *b).releaseReturnValue() <= 0)
        return true;

    return false;
}

Ref<Range> rangeOfContents(Node& node)
{
    auto range = Range::create(node.document());
    range->selectNodeContents(node);
    return range;
}

static inline void boundaryNodeChildrenChanged(RangeBoundaryPoint& boundary, ContainerNode& container)
{
    if (!boundary.childBefore())
        return;
    if (boundary.container() != &container)
        return;
    boundary.invalidateOffset();
}

void Range::nodeChildrenChanged(ContainerNode& container)
{
    ASSERT(&container.document() == &ownerDocument());
    boundaryNodeChildrenChanged(m_start, container);
    boundaryNodeChildrenChanged(m_end, container);
}

static inline void boundaryNodeChildrenWillBeRemoved(RangeBoundaryPoint& boundary, ContainerNode& container)
{
    for (Node* nodeToBeRemoved = container.firstChild(); nodeToBeRemoved; nodeToBeRemoved = nodeToBeRemoved->nextSibling()) {
        if (boundary.childBefore() == nodeToBeRemoved) {
            boundary.setToStartOfNode(container);
            return;
        }

        for (Node* n = boundary.container(); n; n = n->parentNode()) {
            if (n == nodeToBeRemoved) {
                boundary.setToStartOfNode(container);
                return;
            }
        }
    }
}

void Range::nodeChildrenWillBeRemoved(ContainerNode& container)
{
    ASSERT(&container.document() == &ownerDocument());
    boundaryNodeChildrenWillBeRemoved(m_start, container);
    boundaryNodeChildrenWillBeRemoved(m_end, container);
}

static inline void boundaryNodeWillBeRemoved(RangeBoundaryPoint& boundary, Node& nodeToBeRemoved)
{
    if (boundary.childBefore() == &nodeToBeRemoved) {
        boundary.childBeforeWillBeRemoved();
        return;
    }

    for (Node* n = boundary.container(); n; n = n->parentNode()) {
        if (n == &nodeToBeRemoved) {
            boundary.setToBeforeChild(nodeToBeRemoved);
            return;
        }
    }
}

void Range::nodeWillBeRemoved(Node& node)
{
    ASSERT(&node.document() == &ownerDocument());
    ASSERT(&node != &ownerDocument());
    ASSERT(node.parentNode());
    boundaryNodeWillBeRemoved(m_start, node);
    boundaryNodeWillBeRemoved(m_end, node);
}

bool Range::parentlessNodeMovedToNewDocumentAffectsRange(Node& node)
{
    return node.containsIncludingShadowDOM(m_start.container());
}

void Range::updateRangeForParentlessNodeMovedToNewDocument(Node& node)
{
    m_ownerDocument->detachRange(*this);
    m_ownerDocument = node.document();
    m_ownerDocument->attachRange(*this);
}

static inline void boundaryTextInserted(RangeBoundaryPoint& boundary, Node& text, unsigned offset, unsigned length)
{
    if (boundary.container() != &text)
        return;
    unsigned boundaryOffset = boundary.offset();
    if (offset >= boundaryOffset)
        return;
    boundary.setOffset(boundaryOffset + length);
}

void Range::textInserted(Node& text, unsigned offset, unsigned length)
{
    ASSERT(&text.document() == &ownerDocument());
    boundaryTextInserted(m_start, text, offset, length);
    boundaryTextInserted(m_end, text, offset, length);
}

static inline void boundaryTextRemoved(RangeBoundaryPoint& boundary, Node& text, unsigned offset, unsigned length)
{
    if (boundary.container() != &text)
        return;
    unsigned boundaryOffset = boundary.offset();
    if (offset >= boundaryOffset)
        return;
    if (offset + length >= boundaryOffset)
        boundary.setOffset(offset);
    else
        boundary.setOffset(boundaryOffset - length);
}

void Range::textRemoved(Node& text, unsigned offset, unsigned length)
{
    ASSERT(&text.document() == &ownerDocument());
    boundaryTextRemoved(m_start, text, offset, length);
    boundaryTextRemoved(m_end, text, offset, length);
}

static inline void boundaryTextNodesMerged(RangeBoundaryPoint& boundary, NodeWithIndex& oldNode, unsigned offset)
{
    if (boundary.container() == oldNode.node())
        boundary.set(*oldNode.node()->previousSibling(), boundary.offset() + offset, 0);
    else if (boundary.container() == oldNode.node()->parentNode() && static_cast<int>(boundary.offset()) == oldNode.index())
        boundary.set(*oldNode.node()->previousSibling(), offset, 0);
}

void Range::textNodesMerged(NodeWithIndex& oldNode, unsigned offset)
{
    ASSERT(oldNode.node());
    ASSERT(&oldNode.node()->document() == &ownerDocument());
    ASSERT(oldNode.node()->parentNode());
    ASSERT(oldNode.node()->isTextNode());
    ASSERT(oldNode.node()->previousSibling());
    ASSERT(oldNode.node()->previousSibling()->isTextNode());
    boundaryTextNodesMerged(m_start, oldNode, offset);
    boundaryTextNodesMerged(m_end, oldNode, offset);
}

static inline void boundaryTextNodesSplit(RangeBoundaryPoint& boundary, Text& oldNode)
{
    auto* parent = oldNode.parentNode();
    if (boundary.container() == &oldNode) {
        unsigned splitOffset = oldNode.length();
        unsigned boundaryOffset = boundary.offset();
        if (boundaryOffset > splitOffset) {
            if (parent)
                boundary.set(*oldNode.nextSibling(), boundaryOffset - splitOffset, 0);
            else
                boundary.setOffset(splitOffset);
        }
        return;
    }
    if (!parent)
        return;
    if (boundary.container() == parent && boundary.childBefore() == &oldNode) {
        auto* newChild = oldNode.nextSibling();
        ASSERT(newChild);
        boundary.setToAfterChild(*newChild);
    }
}

void Range::textNodeSplit(Text& oldNode)
{
    ASSERT(&oldNode.document() == &ownerDocument());
    ASSERT(!oldNode.parentNode() || oldNode.nextSibling());
    ASSERT(!oldNode.parentNode() || oldNode.nextSibling()->isTextNode());
    boundaryTextNodesSplit(m_start, oldNode);
    boundaryTextNodesSplit(m_end, oldNode);
}

ExceptionOr<void> Range::expand(const String& unit)
{
    VisiblePosition start { startPosition() };
    VisiblePosition end { endPosition() };
    if (unit == "word") {
        start = startOfWord(start);
        end = endOfWord(end);
    } else if (unit == "sentence") {
        start = startOfSentence(start);
        end = endOfSentence(end);
    } else if (unit == "block") {
        start = startOfParagraph(start);
        end = endOfParagraph(end);
    } else if (unit == "document") {
        start = startOfDocument(start);
        end = endOfDocument(end);
    } else
        return { };

    auto* startContainer = start.deepEquivalent().containerNode();
    if (!startContainer)
        return Exception { TypeError };
    auto result = setStart(*startContainer, start.deepEquivalent().computeOffsetInContainerNode());
    if (result.hasException())
        return result.releaseException();
    auto* endContainer = end.deepEquivalent().containerNode();
    if (!endContainer)
        return Exception { TypeError };
    return setEnd(*endContainer, end.deepEquivalent().computeOffsetInContainerNode());
}

Ref<DOMRectList> Range::getClientRects() const
{
    return DOMRectList::create(borderAndTextRects(CoordinateSpace::Client));
}

Ref<DOMRect> Range::getBoundingClientRect() const
{
    return DOMRect::create(boundingRect(CoordinateSpace::Client));
}

Vector<FloatRect> Range::borderAndTextRects(CoordinateSpace space, OptionSet<BoundingRectBehavior> rectOptions) const
{
    Vector<FloatRect> rects;

    ownerDocument().updateLayoutIgnorePendingStylesheets();

    Node* stopNode = pastLastNode();
    bool useVisibleBounds = rectOptions.contains(BoundingRectBehavior::UseVisibleBounds);

    HashSet<Node*> selectedElementsSet;
    for (Node* node = firstNode(); node != stopNode; node = NodeTraversal::next(*node)) {
        if (is<Element>(*node))
            selectedElementsSet.add(node);
    }

    // Don't include elements that are only partially selected.
    Node* lastNode = m_end.childBefore() ? m_end.childBefore() : &endContainer();
    for (Node* parent = lastNode->parentNode(); parent; parent = parent->parentNode())
        selectedElementsSet.remove(parent);
    
    OptionSet<RenderObject::VisibleRectContextOption> visibleRectOptions = { RenderObject::VisibleRectContextOption::UseEdgeInclusiveIntersection, RenderObject::VisibleRectContextOption::ApplyCompositedClips, RenderObject::VisibleRectContextOption::ApplyCompositedContainerScrolls };

    for (Node* node = firstNode(); node != stopNode; node = NodeTraversal::next(*node)) {
        if (is<Element>(*node) && selectedElementsSet.contains(node) && (useVisibleBounds || !node->parentNode() || !selectedElementsSet.contains(node->parentNode()))) {
            if (auto* renderer = downcast<Element>(*node).renderBoxModelObject()) {
                if (useVisibleBounds) {
                    auto localBounds = renderer->borderBoundingBox();
                    auto rootClippedBounds = renderer->computeVisibleRectInContainer(localBounds, &renderer->view(), { false, false, visibleRectOptions });
                    if (!rootClippedBounds)
                        continue;
                    auto snappedBounds = snapRectToDevicePixels(*rootClippedBounds, node->document().deviceScaleFactor());
                    if (space == CoordinateSpace::Client)
                        node->document().convertAbsoluteToClientRect(snappedBounds, renderer->style());
                    rects.append(snappedBounds);

                    continue;
                }

                Vector<FloatQuad> elementQuads;
                renderer->absoluteQuads(elementQuads);
                if (space == CoordinateSpace::Client)
                    node->document().convertAbsoluteToClientQuads(elementQuads, renderer->style());

                rects.appendVector(boundingBoxes(elementQuads));
            }
        } else if (is<Text>(*node)) {
            if (auto* renderer = downcast<Text>(*node).renderer()) {
                bool isFixed;
                auto clippedRects = absoluteRectsForRangeInText(node, *renderer, false, isFixed, rectOptions);
                if (space == CoordinateSpace::Client)
                    node->document().convertAbsoluteToClientRects(clippedRects, renderer->style());

                rects.appendVector(clippedRects);
            }
        }
    }

    if (rectOptions.contains(BoundingRectBehavior::IgnoreTinyRects)) {
        rects.removeAllMatching([&] (const FloatRect& rect) -> bool {
            return rect.area() <= 1;
        });
    }

    return rects;
}

FloatRect Range::boundingRect(CoordinateSpace space, OptionSet<BoundingRectBehavior> rectOptions) const
{
    FloatRect result;
    for (auto& rect : borderAndTextRects(space, rectOptions))
        result.uniteIfNonZero(rect);
    return result;
}

FloatRect Range::absoluteBoundingRect(OptionSet<BoundingRectBehavior> rectOptions) const
{
    return boundingRect(CoordinateSpace::Absolute, rectOptions);
}

Ref<Range> createLiveRange(const SimpleRange& range)
{
    return Range::create(range.start.document(), range.start.container.ptr(), range.start.offset, range.end.container.ptr(), range.end.offset);
}

RefPtr<Range> createLiveRange(const Optional<SimpleRange>& range)
{
    if (!range)
        return nullptr;
    return createLiveRange(*range);
}

WTF::TextStream& operator<<(WTF::TextStream& ts, const RangeBoundaryPoint& r)
{
    return ts << r.toPosition();
}
    
WTF::TextStream& operator<<(WTF::TextStream& ts, const Range& r)
{
    return ts << "Range: " << "start: " << r.startPosition() << " end: " << r.endPosition();
}

} // namespace WebCore

#if ENABLE(TREE_DEBUGGING)

void showTree(const WebCore::Range* range)
{
    if (range && range->boundaryPointsValid()) {
        range->startContainer().showTreeAndMark(&range->startContainer(), "S", &range->endContainer(), "E");
        fprintf(stderr, "start offset: %d, end offset: %d\n", range->startOffset(), range->endOffset());
    }
}

#endif
