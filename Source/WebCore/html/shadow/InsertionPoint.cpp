/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
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
#include "InsertionPoint.h"

#include "ElementShadow.h"
#include "ShadowRoot.h"

namespace WebCore {

InsertionPoint::InsertionPoint(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document)
{
}

InsertionPoint::~InsertionPoint()
{
}

void InsertionPoint::attach()
{
    if (ShadowRoot* root = shadowRoot())
        root->owner()->ensureDistribution();
    for (size_t i = 0; i < m_distribution.size(); ++i) {
        if (!m_distribution.at(i)->attached())
            m_distribution.at(i)->attach();
    }

    HTMLElement::attach();
}

void InsertionPoint::detach()
{
    if (ShadowRoot* root = shadowRoot())
        root->owner()->ensureDistribution();
    for (size_t i = 0; i < m_distribution.size(); ++i)
        m_distribution.at(i)->detach();

    HTMLElement::detach();
}

bool InsertionPoint::isShadowBoundary() const
{
    return treeScope()->rootNode()->isShadowRoot() && isActive();
}

bool InsertionPoint::isActive() const
{
    if (!shadowRoot())
        return false;
    const Node* node = parentNode();
    while (node) {
        if (WebCore::isInsertionPoint(node))
            return false;

        node = node->parentNode();
    }
    return true;
}

bool InsertionPoint::rendererIsNeeded(const NodeRenderingContext& context)
{
    return !isShadowBoundary() && HTMLElement::rendererIsNeeded(context);
}

Node* InsertionPoint::nextTo(const Node* node) const
{
    size_t index = m_distribution.find(node);
    if (index == notFound || index + 1 == m_distribution.size())
        return 0;
    return m_distribution.at(index + 1).get();
}

Node* InsertionPoint::previousTo(const Node* node) const
{
    size_t index = m_distribution.find(node);
    if (index == notFound || !index)
        return 0;
    return m_distribution.at(index - 1).get();
}

void InsertionPoint::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    HTMLElement::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);
    if (ShadowRoot* root = shadowRoot())
        root->owner()->invalidateDistribution();
}

Node::InsertionNotificationRequest InsertionPoint::insertedInto(ContainerNode* insertionPoint)
{
    HTMLElement::insertedInto(insertionPoint);
    if (insertionPoint->inDocument()) {
        if (ShadowRoot* root = shadowRoot()) {
            root->owner()->setValidityUndetermined();
            root->owner()->invalidateDistribution();
        }
    }

    return InsertionDone;
}

void InsertionPoint::removedFrom(ContainerNode* insertionPoint)
{
    if (insertionPoint->inDocument()) {
        ShadowRoot* root = shadowRoot();
        if (!root)
            root = insertionPoint->shadowRoot();

        // host can be null when removedFrom() is called from ElementShadow destructor.
        if (root && root->host())
            root->owner()->invalidateDistribution();

        // Since this insertion point is no longer visible from the shadow subtree, it need to clean itself up.
        clearDistribution();
    }

    HTMLElement::removedFrom(insertionPoint);
}

} // namespace WebCore
