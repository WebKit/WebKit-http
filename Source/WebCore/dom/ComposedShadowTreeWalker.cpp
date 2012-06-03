/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ComposedShadowTreeWalker.h"

#include "ContentDistributor.h"
#include "Element.h"
#include "ElementShadow.h"
#include "InsertionPoint.h"

namespace WebCore {

static inline ElementShadow* shadowFor(const Node* node)
{
    if (node && node->isElementNode())
        return toElement(node)->shadow();
    return 0;
}

static inline ElementShadow* shadowOfParent(const Node* node)
{
    if (node && node->parentNode())
        return shadowFor(node->parentNode());
    return 0;
}

ComposedShadowTreeWalker::ComposedShadowTreeWalker(const Node* node, Policy policy)
    : m_node(node)
    , m_policy(policy)
{
#ifndef NDEBUG
    if (m_node)
        assertPrecondition();
#endif
}

ComposedShadowTreeWalker ComposedShadowTreeWalker::fromFirstChild(const Node* node, Policy policy)
{
    ComposedShadowTreeWalker walker(node, policy);
    walker.firstChild();
    return walker;
}

void ComposedShadowTreeWalker::firstChild()
{
    assertPrecondition();
    m_node = traverseChild(m_node, TraversalDirectionForward);
    assertPostcondition();
}

Node* ComposedShadowTreeWalker::traverseFirstChild(const Node* node) const
{
    ASSERT(node);
    return traverseChild(node, TraversalDirectionForward);
}

void ComposedShadowTreeWalker::lastChild()
{
    assertPrecondition();
    m_node = traverseLastChild(m_node);
    assertPostcondition();
}

Node* ComposedShadowTreeWalker::traverseLastChild(const Node* node) const
{
    ASSERT(node);
    return traverseChild(node, TraversalDirectionBackward);
}

Node* ComposedShadowTreeWalker::traverseChild(const Node* node, TraversalDirection direction) const
{
    ASSERT(node);
    if (canCrossUpperBoundary()) {
        ElementShadow* shadow = shadowFor(node);
        return shadow ? traverseLightChildren(shadow->youngestShadowRoot(), direction)
            : traverseLightChildren(node, direction);
    }
    if (isShadowHost(node))
        return 0;
    return traverseLightChildren(node, direction);
}

Node* ComposedShadowTreeWalker::traverseLightChildren(const Node* node, TraversalDirection direction)
{
    ASSERT(node);
    if (Node* child = (direction == TraversalDirectionForward ? node->firstChild() : node->lastChild()))
        return traverseNode(child, direction);
    return 0;
}

Node* ComposedShadowTreeWalker::traverseNode(const Node* node, TraversalDirection direction)
{
    ASSERT(node);
    if (!isInsertionPoint(node))
        return const_cast<Node*>(node);
    const InsertionPoint* insertionPoint = toInsertionPoint(node);
    if (!insertionPoint->isActive())
        return const_cast<Node*>(node);
    if (Node* next = (direction == TraversalDirectionForward ? insertionPoint->first() : insertionPoint->last()))
        return traverseNode(next, direction);
    return traverseLightChildren(node, direction);
}

void ComposedShadowTreeWalker::nextSibling()
{
    assertPrecondition();
    m_node = traverseSiblingOrBackToInsertionPoint(m_node, TraversalDirectionForward);
    assertPostcondition();
}

void ComposedShadowTreeWalker::previousSibling()
{
    assertPrecondition();
    m_node = traverseSiblingOrBackToInsertionPoint(m_node, TraversalDirectionBackward);
    assertPostcondition();
}

Node* ComposedShadowTreeWalker::traverseSiblingOrBackToInsertionPoint(const Node* node, TraversalDirection direction)
{
    ASSERT(node);
    ElementShadow* shadow = shadowOfParent(node);
    if (!shadow)
        return traverseSiblingInCurrentTree(node, direction);
    InsertionPoint* insertionPoint = shadow->insertionPointFor(node);
    if (!insertionPoint)
        return traverseSiblingInCurrentTree(node, direction);
    if (Node* next = (direction == TraversalDirectionForward ? insertionPoint->nextTo(node) : insertionPoint->previousTo(node)))
        return traverseNode(next, direction);
    return traverseSiblingOrBackToInsertionPoint(insertionPoint, direction);
}

Node* ComposedShadowTreeWalker::traverseSiblingInCurrentTree(const Node* node, TraversalDirection direction)
{
    ASSERT(node);
    if (Node* next = (direction == TraversalDirectionForward ? node->nextSibling() : node->previousSibling()))
        return traverseNode(next, direction);
    if (Node* next = traverseSiblingOrBackToYoungerShadowRoot(node, direction))
        return next;
    return escapeFallbackContentElement(node, direction);
}

Node* ComposedShadowTreeWalker::traverseSiblingOrBackToYoungerShadowRoot(const Node* node, TraversalDirection direction)
{
    ASSERT(node);
    if (node->parentNode() && node->parentNode()->isShadowRoot()) {
        ShadowRoot* parentShadowRoot = toShadowRoot(node->parentNode());
        if (!parentShadowRoot->isYoungest()) {
            InsertionPoint* assignedInsertionPoint = parentShadowRoot->assignedTo();
            ASSERT(assignedInsertionPoint);
            return traverseSiblingInCurrentTree(assignedInsertionPoint, direction);
        }
    }
    return 0;
}

Node* ComposedShadowTreeWalker::escapeFallbackContentElement(const Node* node, TraversalDirection direction)
{
    ASSERT(node);
    if (node->parentNode() && isActiveInsertionPoint(node->parentNode()))
        return traverseSiblingOrBackToInsertionPoint(node->parentNode(), direction);
    return 0;
}

Node* ComposedShadowTreeWalker::traverseNodeEscapingFallbackContents(const Node* node) const
{
    ASSERT(node);
    if (isActiveInsertionPoint(node))
        return traverseParent(node);
    return const_cast<Node*>(node);
}

void ComposedShadowTreeWalker::parent()
{
    assertPrecondition();
    m_node = traverseParent(m_node);
    assertPostcondition();
}

Node* ComposedShadowTreeWalker::traverseParent(const Node* node) const
{
    if (!canCrossUpperBoundary() && node->isShadowRoot()) {
        ASSERT(toShadowRoot(node)->isYoungest());
        return 0;
    }
    if (ElementShadow* shadow = shadowOfParent(node)) {
        if (InsertionPoint* insertionPoint = shadow->insertionPointFor(node))
            return traverseParent(insertionPoint);
    }
    return traverseParentInCurrentTree(node);
}

Node* ComposedShadowTreeWalker::traverseParentInCurrentTree(const Node* node) const
{
    if (Node* parent = node->parentNode())
        return parent->isShadowRoot() ? traverseParentBackToYoungerShadowRootOrHost(toShadowRoot(parent)) : traverseNodeEscapingFallbackContents(parent);
    return 0;
}

Node* ComposedShadowTreeWalker::traverseParentBackToYoungerShadowRootOrHost(const ShadowRoot* shadowRoot) const
{
    ASSERT(shadowRoot);
    if (shadowRoot->isYoungest()) {
        if (canCrossUpperBoundary())
            return shadowRoot->host();
        return const_cast<ShadowRoot*>(shadowRoot);
    }
    InsertionPoint* assignedInsertionPoint = shadowRoot->assignedTo();
    return assignedInsertionPoint ? traverseParent(assignedInsertionPoint) : 0;
}

Node* ComposedShadowTreeWalker::traverseNextSibling(const Node* node)
{
    ASSERT(node);
    return traverseSiblingOrBackToInsertionPoint(node, TraversalDirectionForward);
}

Node* ComposedShadowTreeWalker::traversePreviousSibling(const Node* node)
{
    ASSERT(node);
    return traverseSiblingOrBackToInsertionPoint(node, TraversalDirectionBackward);
}

void ComposedShadowTreeWalker::next()
{
    assertPrecondition();
    if (Node* next = traverseFirstChild(m_node))
        m_node = next;
    else if (Node* next = traverseNextSibling(m_node))
        m_node = next;
    else {
        const Node* n = m_node;
        while (n && !traverseNextSibling(n))
            n = traverseParent(n);
        m_node = n ? traverseNextSibling(n) : 0;
    }
    assertPostcondition();
}

void ComposedShadowTreeWalker::previous()
{
    assertPrecondition();
    if (Node* n = traversePreviousSibling(m_node)) {
        while (Node* child = traverseLastChild(n))
            n = child;
        m_node = n;
    } else
        parent();
    assertPostcondition();
}

ComposedShadowTreeParentWalker::ComposedShadowTreeParentWalker(const Node* node)
    : m_node(node)
{
}

void ComposedShadowTreeParentWalker::parentIncludingInsertionPointAndShadowRoot()
{
    ASSERT(m_node);
    m_node = traverseParentIncludingInsertionPointAndShadowRoot(m_node);
}

Node* ComposedShadowTreeParentWalker::traverseParentIncludingInsertionPointAndShadowRoot(const Node* node) const
{
    if (ElementShadow* shadow = shadowOfParent(node)) {
        if (InsertionPoint* insertionPoint = shadow->insertionPointFor(node))
            return insertionPoint;
    }
    if (!node->isShadowRoot())
        return node->parentNode();
    const ShadowRoot* shadowRoot = toShadowRoot(node);
    if (InsertionPoint* insertionPoint = shadowRoot->assignedTo())
        return insertionPoint;
    return shadowRoot->host();
}

} // namespace
