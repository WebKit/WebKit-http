/*
 * Copyright (C) 2004, 2006, 2008 Apple Inc. All rights reserved.
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

#ifndef htmlediting_h
#define htmlediting_h

#include "EditingBoundary.h"
#include "Position.h"
#include "TextDirection.h"
#include <wtf/Forward.h>
#include <wtf/unicode/CharacterNames.h>

namespace WebCore {

class Document;
class Element;
class HTMLElement;
class HTMLTextFormControlElement;
class Node;
class Position;
class Range;
class VisiblePosition;
class VisibleSelection;

// This file contains a set of helper functions used by the editing commands

// -------------------------------------------------------------------------
// Node
// -------------------------------------------------------------------------

// Functions returning Node

Node* highestAncestor(Node*);
Node* highestEditableRoot(const Position&, EditableType = ContentIsEditable);

Node* highestEnclosingNodeOfType(const Position&, bool (*nodeIsOfType)(const Node*),
    EditingBoundaryCrossingRule = CannotCrossEditingBoundary, Node* stayWithin = 0);
Node* highestNodeToRemoveInPruning(Node*);
Node* lowestEditableAncestor(Node*);

Element* deprecatedEnclosingBlockFlowElement(Node*); // Use enclosingBlock instead.
Element* enclosingBlock(Node*, EditingBoundaryCrossingRule = CannotCrossEditingBoundary);
Node* enclosingTableCell(const Position&);
Node* enclosingEmptyListItem(const VisiblePosition&);
Element* enclosingAnchorElement(const Position&);
Element* enclosingElementWithTag(const Position&, const QualifiedName&);
Node* enclosingNodeOfType(const Position&, bool (*nodeIsOfType)(const Node*), EditingBoundaryCrossingRule = CannotCrossEditingBoundary);

Node* tabSpanNode(const Node*);
Node* isLastPositionBeforeTable(const VisiblePosition&);
Node* isFirstPositionAfterTable(const VisiblePosition&);

// These two deliver leaf nodes as if the whole DOM tree were a linear chain of its leaf nodes.
Node* nextLeafNode(const Node*);
Node* previousLeafNode(const Node*);

// offset functions on Node

int lastOffsetForEditing(const Node*);
int caretMinOffset(const Node*);
int caretMaxOffset(const Node*);

// boolean functions on Node

// FIXME: editingIgnoresContent, canHaveChildrenForEditing, and isAtomicNode
// should be renamed to reflect its usage.

// Returns true for nodes that either have no content, or have content that is ignored (skipped over) while editing.
// There are no VisiblePositions inside these nodes.
inline bool editingIgnoresContent(const Node* node)
{
    return !node->canContainRangeEndPoint();
}

inline bool canHaveChildrenForEditing(const Node* node)
{
    return !node->isTextNode() && node->canContainRangeEndPoint();
}

bool isAtomicNode(const Node*);
bool isBlock(const Node*);
bool isBlockFlowElement(const Node*);
bool isInline(const Node*);
bool isSpecialElement(const Node*);
bool isTabSpanNode(const Node*);
bool isTabSpanTextNode(const Node*);
bool isMailBlockquote(const Node*);
bool isRenderedTable(const Node*);
bool isTableCell(const Node*);
bool isEmptyTableCell(const Node*);
bool isTableStructureNode(const Node*);
bool isListElement(Node*);
bool isListItem(const Node*);
bool isNodeRendered(const Node*);
bool isNodeVisiblyContainedWithin(Node*, const Range*);
bool isRenderedAsNonInlineTableImageOrHR(const Node*);
bool areIdenticalElements(const Node*, const Node*);
bool isNonTableCellHTMLBlockElement(const Node*);

TextDirection directionOfEnclosingBlock(const Position&);

// -------------------------------------------------------------------------
// Position
// -------------------------------------------------------------------------
    
// Functions returning Position
    
Position nextCandidate(const Position&);
Position previousCandidate(const Position&);
    
Position nextVisuallyDistinctCandidate(const Position&);
Position previousVisuallyDistinctCandidate(const Position&);

Position positionOutsideTabSpan(const Position&);
Position positionBeforeContainingSpecialElement(const Position&, Node** containingSpecialElement = 0);
Position positionAfterContainingSpecialElement(const Position&, Node** containingSpecialElement = 0);
Position positionOutsideContainingSpecialElement(const Position&, Node** containingSpecialElement = 0);

inline Position firstPositionInOrBeforeNode(Node* node)
{
    if (!node)
        return Position();
    return editingIgnoresContent(node) ? positionBeforeNode(node) : firstPositionInNode(node);
}

inline Position lastPositionInOrAfterNode(Node* node)
{
    if (!node)
        return Position();
    return editingIgnoresContent(node) ? positionAfterNode(node) : lastPositionInNode(node);
}

// comparision functions on Position
    
int comparePositions(const Position&, const Position&);

// boolean functions on Position

enum EUpdateStyle { UpdateStyle, DoNotUpdateStyle };
bool isEditablePosition(const Position&, EditableType = ContentIsEditable, EUpdateStyle = UpdateStyle);
bool isRichlyEditablePosition(const Position&, EditableType = ContentIsEditable);
bool isFirstVisiblePositionInSpecialElement(const Position&);
bool isLastVisiblePositionInSpecialElement(const Position&);
bool lineBreakExistsAtPosition(const Position&);
bool isVisiblyAdjacent(const Position& first, const Position& second);
bool isAtUnsplittableElement(const Position&);

// miscellaneous functions on Position

unsigned numEnclosingMailBlockquotes(const Position&);
void updatePositionForNodeRemoval(Position&, Node*);

// -------------------------------------------------------------------------
// VisiblePosition
// -------------------------------------------------------------------------
    
// Functions returning VisiblePosition
    
VisiblePosition firstEditablePositionAfterPositionInRoot(const Position&, Node*);
VisiblePosition lastEditablePositionBeforePositionInRoot(const Position&, Node*);
VisiblePosition visiblePositionBeforeNode(Node*);
VisiblePosition visiblePositionAfterNode(Node*);

bool lineBreakExistsAtVisiblePosition(const VisiblePosition&);
    
int comparePositions(const VisiblePosition&, const VisiblePosition&);

int indexForVisiblePosition(const VisiblePosition&, RefPtr<ContainerNode>& scope);
int indexForVisiblePosition(Node*, const VisiblePosition&, bool forSelectionPreservation);
VisiblePosition visiblePositionForIndex(int index, ContainerNode* scope);
VisiblePosition visiblePositionForIndexUsingCharacterIterator(Node*, int index); // FIXME: Why do we need this version?

// -------------------------------------------------------------------------
// HTMLElement
// -------------------------------------------------------------------------
    
// Functions returning HTMLElement
    
PassRefPtr<HTMLElement> createDefaultParagraphElement(Document&);
PassRefPtr<HTMLElement> createBreakElement(Document&);
PassRefPtr<HTMLElement> createOrderedListElement(Document&);
PassRefPtr<HTMLElement> createUnorderedListElement(Document&);
PassRefPtr<HTMLElement> createListItemElement(Document&);
PassRefPtr<HTMLElement> createHTMLElement(Document&, const QualifiedName&);
PassRefPtr<HTMLElement> createHTMLElement(Document&, const AtomicString&);

HTMLElement* enclosingList(Node*);
HTMLElement* outermostEnclosingList(Node*, Node* rootList = 0);
Node* enclosingListChild(Node*);

// -------------------------------------------------------------------------
// Element
// -------------------------------------------------------------------------

PassRefPtr<Element> createTabSpanElement(Document&);
PassRefPtr<Element> createTabSpanElement(Document&, PassRefPtr<Node> tabTextNode);
PassRefPtr<Element> createTabSpanElement(Document&, const String& tabText);
PassRefPtr<Element> createBlockPlaceholderElement(Document&);

Element* editableRootForPosition(const Position&, EditableType = ContentIsEditable);
Element* unsplittableElementForPosition(const Position&);

bool canMergeLists(Element* firstList, Element* secondList);
    
// -------------------------------------------------------------------------
// VisibleSelection
// -------------------------------------------------------------------------

VisibleSelection selectionForParagraphIteration(const VisibleSelection&);

Position adjustedSelectionStartForStyleComputation(const VisibleSelection&);
    
// -------------------------------------------------------------------------

// FIXME: This is only one of many definitions of whitespace, so the name is not specific enough.
inline bool isWhitespace(UChar c)
{
    return c == noBreakSpace || c == ' ' || c == '\n' || c == '\t';
}

// FIXME: Can't really answer this question correctly without knowing the white-space mode.
inline bool deprecatedIsCollapsibleWhitespace(UChar c)
{
    return c == ' ' || c == '\n';
}

inline bool isAmbiguousBoundaryCharacter(UChar character)
{
    // These are characters that can behave as word boundaries, but can appear within words.
    // If they are just typed, i.e. if they are immediately followed by a caret, we want to delay text checking until the next character has been typed.
    // FIXME: this is required until 6853027 is fixed and text checking can do this for us.
    return character == '\'' || character == rightSingleQuotationMark || character == hebrewPunctuationGershayim;
}

String stringWithRebalancedWhitespace(const String&, bool startIsStartOfParagraph, bool endIsEndOfParagraph);
const String& nonBreakingSpaceString();

// Miscellaaneous functions that for caret rendering

RenderObject* rendererForCaretPainting(Node*);
LayoutRect localCaretRectInRendererForCaretPainting(const VisiblePosition&, RenderObject*&);
IntRect absoluteBoundsForLocalCaretRect(RenderObject* rendererForCaretPainting, const LayoutRect&);

}

#endif
