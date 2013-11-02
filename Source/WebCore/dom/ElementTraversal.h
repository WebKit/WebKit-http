/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
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

#ifndef ElementTraversal_h
#define ElementTraversal_h

#include "Element.h"
#include "NodeTraversal.h"

namespace WebCore {

template <typename ElementType>
class Traversal {
public:
    // First or last ElementType child of the node.
    static ElementType* firstChild(const Node*);
    static ElementType* firstChild(const ContainerNode*);
    static ElementType* lastChild(const Node*);
    static ElementType* lastChild(const ContainerNode*);

    // First or last ElementType descendant of the node. For Elements this is always the same as first/last child.
    static ElementType* firstWithin(const Node*);
    static ElementType* firstWithin(const ContainerNode*);
    static ElementType* lastWithin(const Node*);
    static ElementType* lastWithin(const ContainerNode*);

    // Pre-order traversal skipping non-ElementType nodes.
    static ElementType* next(const Node*);
    static ElementType* next(const Node*, const Node* stayWithin);
    static ElementType* next(const ContainerNode*);
    static ElementType* next(const ContainerNode*, const Node* stayWithin);
    static ElementType* previous(const Node*);
    static ElementType* previous(const Node*, const Node* stayWithin);
    static ElementType* previous(const ContainerNode*);
    static ElementType* previous(const ContainerNode*, const Node* stayWithin);

    // Next or previous ElementType sibling if there is one.
    static ElementType* nextSibling(const Node*);
    static ElementType* nextSibling(const ContainerNode*);
    static ElementType* previousSibling(const Node*);
    static ElementType* previousSibling(const ContainerNode*);

    // Like next, but skips children.
    static ElementType* nextSkippingChildren(const Node*);
    static ElementType* nextSkippingChildren(const Node*, const Node* stayWithin);
    static ElementType* nextSkippingChildren(const ContainerNode*);
    static ElementType* nextSkippingChildren(const ContainerNode*, const Node* stayWithin);

private:
    template <typename CurrentType> static ElementType* firstChildTemplate(CurrentType*);
    template <typename CurrentType> static ElementType* lastChildTemplate(CurrentType*);
    template <typename CurrentType> static ElementType* firstWithinTemplate(CurrentType*);
    template <typename CurrentType> static ElementType* lastWithinTemplate(CurrentType*);
    template <typename CurrentType> static ElementType* nextTemplate(CurrentType*);
    template <typename CurrentType> static ElementType* nextTemplate(CurrentType*, const Node* stayWithin);
    template <typename CurrentType> static ElementType* previousTemplate(CurrentType*);
    template <typename CurrentType> static ElementType* previousTemplate(CurrentType*, const Node* stayWithin);
    template <typename CurrentType> static ElementType* nextSiblingTemplate(CurrentType*);
    template <typename CurrentType> static ElementType* previousSiblingTemplate(CurrentType*);
    template <typename CurrentType> static ElementType* nextSkippingChildrenTemplate(CurrentType*);
    template <typename CurrentType> static ElementType* nextSkippingChildrenTemplate(CurrentType*, const Node* stayWithin);
};

class ElementTraversal : public Traversal<Element> {
public:
    // FIXME: These should go somewhere else.
    // Pre-order traversal including the pseudo-elements.
    static Element* previousIncludingPseudo(const Node*, const Node* = 0);
    static Element* nextIncludingPseudo(const Node*, const Node* = 0);
    static Element* nextIncludingPseudoSkippingChildren(const Node*, const Node* = 0);

    // Utility function to traverse only the element and pseudo-element siblings of a node.
    static Element* pseudoAwarePreviousSibling(const Node*);
};

// Specialized for pure Element to exploit the fact that Elements parent is always either another Element or the root.
template <>
template <typename CurrentType>
inline Element* Traversal<Element>::firstWithinTemplate(CurrentType* current)
{
    return firstChildTemplate(current);
}

template <>
template <typename CurrentType>
inline Element* Traversal<Element>::lastWithinTemplate(CurrentType* current)
{
    return lastChildTemplate(current);
}

template <>
template <typename CurrentType>
inline Element* Traversal<Element>::nextTemplate(CurrentType* current)
{
    Node* node = NodeTraversal::next(current);
    while (node && !node->isElementNode())
        node = NodeTraversal::nextSkippingChildren(node);
    return static_cast<Element*>(node);
}

template <>
template <typename CurrentType>
inline Element* Traversal<Element>::nextTemplate(CurrentType* current, const Node* stayWithin)
{
    Node* node = NodeTraversal::next(current, stayWithin);
    while (node && !node->isElementNode())
        node = NodeTraversal::nextSkippingChildren(node, stayWithin);
    return static_cast<Element*>(node);
}

template <>
template <typename CurrentType>
inline Element* Traversal<Element>::previousTemplate(CurrentType* current)
{
    Node* node = NodeTraversal::previous(current);
    while (node && !node->isElementNode())
        node = NodeTraversal::previousSkippingChildren(node);
    return static_cast<Element*>(node);
}

template <>
template <typename CurrentType>
inline Element* Traversal<Element>::previousTemplate(CurrentType* current, const Node* stayWithin)
{
    Node* node = NodeTraversal::previous(current, stayWithin);
    while (node && !node->isElementNode())
        node = NodeTraversal::previousSkippingChildren(node, stayWithin);
    return static_cast<Element*>(node);
}

// Generic versions.
template <typename ElementType>
template <typename CurrentType>
inline ElementType* Traversal<ElementType>::firstChildTemplate(CurrentType* current)
{
    Node* node = current->firstChild();
    while (node && !isElementOfType<const ElementType>(*node))
        node = node->nextSibling();
    return static_cast<ElementType*>(node);
}

template <typename ElementType>
template <typename CurrentType>
inline ElementType* Traversal<ElementType>::lastChildTemplate(CurrentType* current)
{
    Node* node = current->lastChild();
    while (node && !isElementOfType<const ElementType>(*node))
        node = node->previousSibling();
    return static_cast<ElementType*>(node);
}

template <typename ElementType>
template <typename CurrentType>
inline ElementType* Traversal<ElementType>::firstWithinTemplate(CurrentType* current)
{
    Element* element = Traversal<Element>::firstWithin(current);
    while (element && !isElementOfType<const ElementType>(*element))
        element = Traversal<Element>::next(element, current);
    return static_cast<ElementType*>(element);
}

template <typename ElementType>
template <typename CurrentType>
inline ElementType* Traversal<ElementType>::lastWithinTemplate(CurrentType* current)
{
    Element* element = Traversal<Element>::lastWithin(current);
    while (element && !isElementOfType<const ElementType>(*element))
        element = Traversal<Element>::previous(element, current);
    return static_cast<ElementType*>(element);
}

template <typename ElementType>
template <typename CurrentType>
inline ElementType* Traversal<ElementType>::nextTemplate(CurrentType* current)
{
    Element* element = Traversal<Element>::next(current);
    while (element && !isElementOfType<const ElementType>(*element))
        element = Traversal<Element>::next(element);
    return static_cast<ElementType*>(element);
}

template <typename ElementType>
template <typename CurrentType>
inline ElementType* Traversal<ElementType>::nextTemplate(CurrentType* current, const Node* stayWithin)
{
    Element* element = Traversal<Element>::next(current, stayWithin);
    while (element && !isElementOfType<const ElementType>(*element))
        element = Traversal<Element>::next(element, stayWithin);
    return static_cast<ElementType*>(element);
}

template <typename ElementType>
template <typename CurrentType>
inline ElementType* Traversal<ElementType>::previousTemplate(CurrentType* current)
{
    Element* element = Traversal<Element>::previous(current);
    while (element && !isElementOfType<const ElementType>(*element))
        element = Traversal<Element>::previous(element);
    return static_cast<ElementType*>(element);
}

template <typename ElementType>
template <typename CurrentType>
inline ElementType* Traversal<ElementType>::previousTemplate(CurrentType* current, const Node* stayWithin)
{
    Element* element = Traversal<Element>::previous(current, stayWithin);
    while (element && !isElementOfType<const ElementType>(*element))
        element = Traversal<Element>::previous(element, stayWithin);
    return static_cast<ElementType*>(element);
}

template <typename ElementType>
template <typename CurrentType>
inline ElementType* Traversal<ElementType>::nextSiblingTemplate(CurrentType* current)
{
    Node* node = current->nextSibling();
    while (node && !isElementOfType<const ElementType>(*node))
        node = node->nextSibling();
    return static_cast<ElementType*>(node);
}

template <typename ElementType>
template <typename CurrentType>
inline ElementType* Traversal<ElementType>::previousSiblingTemplate(CurrentType* current)
{
    Node* node = current->previousSibling();
    while (node && !isElementOfType<const ElementType>(*node))
        node = node->previousSibling();
    return static_cast<ElementType*>(node);
}

template <typename ElementType>
template <typename CurrentType>
inline ElementType* Traversal<ElementType>::nextSkippingChildrenTemplate(CurrentType* current)
{
    Node* node = NodeTraversal::nextSkippingChildren(current);
    while (node && !isElementOfType<const ElementType>(*node))
        node = NodeTraversal::nextSkippingChildren(node);
    return static_cast<ElementType*>(node);
}

template <typename ElementType>
template <typename CurrentType>
inline ElementType* Traversal<ElementType>::nextSkippingChildrenTemplate(CurrentType* current, const Node* stayWithin)
{
    Node* node = NodeTraversal::nextSkippingChildren(current, stayWithin);
    while (node && !isElementOfType<const ElementType>(*node))
        node = NodeTraversal::nextSkippingChildren(node, stayWithin);
    return static_cast<ElementType*>(node);
}

template <typename ElementType>
inline ElementType* Traversal<ElementType>::firstChild(const ContainerNode* current) { return firstChildTemplate(current); }
template <typename ElementType>
inline ElementType* Traversal<ElementType>::firstChild(const Node* current) { return firstChildTemplate(current); }
template <typename ElementType>

inline ElementType* Traversal<ElementType>::lastChild(const ContainerNode* current) { return lastChildTemplate(current); }
template <typename ElementType>
inline ElementType* Traversal<ElementType>::lastChild(const Node* current) { return lastChildTemplate(current); }

template <typename ElementType>
inline ElementType* Traversal<ElementType>::firstWithin(const ContainerNode* current) { return firstWithinTemplate(current); }
template <typename ElementType>
inline ElementType* Traversal<ElementType>::firstWithin(const Node* current) { return firstWithinTemplate(current); }
template <typename ElementType>

inline ElementType* Traversal<ElementType>::lastWithin(const ContainerNode* current) { return lastWithinTemplate(current); }
template <typename ElementType>
inline ElementType* Traversal<ElementType>::lastWithin(const Node* current) { return lastWithinTemplate(current); }

template <typename ElementType>
inline ElementType* Traversal<ElementType>::next(const ContainerNode* current) { return nextTemplate(current); }
template <typename ElementType>
inline ElementType* Traversal<ElementType>::next(const Node* current) { return nextTemplate(current); }
template <typename ElementType>
inline ElementType* Traversal<ElementType>::next(const ContainerNode* current, const Node* stayWithin) { return nextTemplate(current, stayWithin); }
template <typename ElementType>
inline ElementType* Traversal<ElementType>::next(const Node* current, const Node* stayWithin) { return nextTemplate(current, stayWithin); }

template <typename ElementType>
inline ElementType* Traversal<ElementType>::previous(const ContainerNode* current) { return previousTemplate(current); }
template <typename ElementType>
inline ElementType* Traversal<ElementType>::previous(const Node* current) { return previousTemplate(current); }
template <typename ElementType>
inline ElementType* Traversal<ElementType>::previous(const ContainerNode* current, const Node* stayWithin) { return previousTemplate(current, stayWithin); }
template <typename ElementType>
inline ElementType* Traversal<ElementType>::previous(const Node* current, const Node* stayWithin) { return previousTemplate(current, stayWithin); }

template <typename ElementType>
inline ElementType* Traversal<ElementType>::nextSibling(const ContainerNode* current) { return nextSiblingTemplate(current); }
template <typename ElementType>
inline ElementType* Traversal<ElementType>::nextSibling(const Node* current) { return nextSiblingTemplate(current); }

template <typename ElementType>
inline ElementType* Traversal<ElementType>::previousSibling(const ContainerNode* current) { return previousSiblingTemplate(current); }
template <typename ElementType>
inline ElementType* Traversal<ElementType>::previousSibling(const Node* current) { return previousSiblingTemplate(current); }

template <typename ElementType>
inline ElementType* Traversal<ElementType>::nextSkippingChildren(const ContainerNode* current) { return nextSkippingChildrenTemplate(current); }
template <typename ElementType>
inline ElementType* Traversal<ElementType>::nextSkippingChildren(const Node* current) { return nextSkippingChildrenTemplate(current); }
template <typename ElementType>
inline ElementType* Traversal<ElementType>::nextSkippingChildren(const ContainerNode* current, const Node* stayWithin) { return nextSkippingChildrenTemplate(current, stayWithin); }
template <typename ElementType>
inline ElementType* Traversal<ElementType>::nextSkippingChildren(const Node* current, const Node* stayWithin) { return nextSkippingChildrenTemplate(current, stayWithin); }

// FIXME: These should go somewhere else.
inline Element* ElementTraversal::previousIncludingPseudo(const Node* current, const Node* stayWithin)
{
    Node* node = NodeTraversal::previousIncludingPseudo(current, stayWithin);
    while (node && !node->isElementNode())
        node = NodeTraversal::previousIncludingPseudo(node, stayWithin);
    return toElement(node);
}

inline Element* ElementTraversal::nextIncludingPseudo(const Node* current, const Node* stayWithin)
{
    Node* node = NodeTraversal::nextIncludingPseudo(current, stayWithin);
    while (node && !node->isElementNode())
        node = NodeTraversal::nextIncludingPseudo(node, stayWithin);
    return toElement(node);
}

inline Element* ElementTraversal::nextIncludingPseudoSkippingChildren(const Node* current, const Node* stayWithin)
{
    Node* node = NodeTraversal::nextIncludingPseudoSkippingChildren(current, stayWithin);
    while (node && !node->isElementNode())
        node = NodeTraversal::nextIncludingPseudoSkippingChildren(node, stayWithin);
    return toElement(node);
}

inline Element* ElementTraversal::pseudoAwarePreviousSibling(const Node* current)
{
    Node* node = current->pseudoAwarePreviousSibling();
    while (node && !node->isElementNode())
        node = node->pseudoAwarePreviousSibling();
    return toElement(node);
}

}

#endif
