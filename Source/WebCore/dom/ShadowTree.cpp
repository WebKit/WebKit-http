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
#include "ShadowTree.h"

#include "Document.h"
#include "Element.h"
#include "HTMLContentSelector.h"
#include "InspectorInstrumentation.h"
#include "RuntimeEnabledFeatures.h"
#include "ShadowRoot.h"
#include "Text.h"

namespace WebCore {

ShadowTree::ShadowTree()
    : m_needsRecalculateContent(false)
{
}

ShadowTree::~ShadowTree()
{
    if (hasShadowRoot())
        removeAllShadowRoots();
}

static bool validateShadowRoot(Document* document, ShadowRoot* shadowRoot, ExceptionCode& ec)
{
    if (!shadowRoot)
        return true;

    if (shadowRoot->shadowHost()) {
        ec = HIERARCHY_REQUEST_ERR;
        return false;
    }

    if (shadowRoot->document() != document) {
        ec = WRONG_DOCUMENT_ERR;
        return false;
    }

    return true;
}

void ShadowTree::addShadowRoot(Element* shadowHost, PassRefPtr<ShadowRoot> shadowRoot, ExceptionCode& ec)
{
    ASSERT(shadowHost);
    ASSERT(shadowRoot);
#if ENABLE(SHADOW_DOM)
    ASSERT(!hasShadowRoot() || RuntimeEnabledFeatures::multipleShadowSubtreesEnabled());
#else
    ASSERT(!hasShadowRoot());
#endif

    if (!validateShadowRoot(shadowHost->document(), shadowRoot.get(), ec))
        return;

    shadowRoot->setShadowHost(shadowHost);

    if (shadowHost->inDocument())
        shadowRoot->insertedIntoDocument();
    if (shadowHost->attached()) {
        shadowRoot->lazyAttach();
        detach();
        shadowHost->detachChildren();
    }

    m_shadowRoots.push(shadowRoot.get());
    InspectorInstrumentation::didPushShadowRoot(shadowHost, shadowRoot.get());
}

void ShadowTree::removeAllShadowRoots()
{
    if (!hasShadowRoot())
        return;

    Element* shadowHost = host();

    while (RefPtr<ShadowRoot> oldRoot = m_shadowRoots.removeHead()) {
        InspectorInstrumentation::willPopShadowRoot(shadowHost, oldRoot.get());
        shadowHost->document()->removeFocusedNodeOfSubtree(oldRoot.get());

        if (oldRoot->attached())
            oldRoot->detach();

        oldRoot->setShadowHost(0);
        oldRoot->setPrev(0);
        oldRoot->setNext(0);
        shadowHost->document()->adoptIfNeeded(oldRoot.get());
        if (oldRoot->inDocument())
            oldRoot->removedFromDocument();
        else
            oldRoot->removedFromTree(true);
    }

    if (shadowHost->attached())
        shadowHost->attachChildrenLazily();
}

void ShadowTree::insertedIntoDocument()
{
    for (ShadowRoot* root = youngestShadowRoot(); root; root = root->olderShadowRoot())
        root->insertedIntoDocument();
}

void ShadowTree::removedFromDocument()
{
    for (ShadowRoot* root = youngestShadowRoot(); root; root = root->olderShadowRoot())
        root->removedFromDocument();
}

void ShadowTree::insertedIntoTree(bool deep)
{
    for (ShadowRoot* root = youngestShadowRoot(); root; root = root->olderShadowRoot())
        root->insertedIntoTree(deep);
}

void ShadowTree::removedFromTree(bool deep)
{
    for (ShadowRoot* root = youngestShadowRoot(); root; root = root->olderShadowRoot())
        root->removedFromTree(deep);
}

void ShadowTree::willRemove()
{
    for (ShadowRoot* root = youngestShadowRoot(); root; root = root->olderShadowRoot())
        root->willRemove();
}

void ShadowTree::setParentTreeScope(TreeScope* scope)
{
    for (ShadowRoot* root = youngestShadowRoot(); root; root = root->olderShadowRoot())
        root->setParentTreeScope(scope);
}

void ShadowTree::attach()
{
    // Children of m_selector is populated lazily in
    // ensureSelector(), and here we just ensure that it is in clean state.
    ASSERT(!selector() || !selector()->hasCandidates());

    for (ShadowRoot* root = youngestShadowRoot(); root; root = root->olderShadowRoot()) {
        if (!root->attached())
            root->attach();
    }

    if (HTMLContentSelector* contentSelector = selector())
        contentSelector->didSelect();
}

void ShadowTree::attachHost(Element* host)
{
    attach();
    host->attachChildrenIfNeeded();
    host->attachAsNode();
}


void ShadowTree::detach()
{
    for (ShadowRoot* root = youngestShadowRoot(); root; root = root->olderShadowRoot()) {
        if (root->attached())
            root->detach();
    }
}

void ShadowTree::detachHost(Element* host)
{
    host->detachChildrenIfNeeded();
    detach();
    host->detachAsNode();
}

InsertionPoint* ShadowTree::insertionPointFor(Node* node) const
{
    if (!m_selector)
        return 0;
    HTMLContentSelection* found = m_selector->findFor(node);
    if (!found)
        return 0;
    return found->insertionPoint();
}

bool ShadowTree::isSelectorActive() const
{
    return m_selector && m_selector->hasCandidates();
}

void ShadowTree::reattach()
{
    detach();
    attach();
}

bool ShadowTree::childNeedsStyleRecalc()
{
    ASSERT(youngestShadowRoot());
    for (ShadowRoot* root = youngestShadowRoot(); root; root = root->olderShadowRoot())
        if (root->childNeedsStyleRecalc())
            return true;

    return false;
}

bool ShadowTree::needsStyleRecalc()
{
    ASSERT(youngestShadowRoot());
    for (ShadowRoot* root = youngestShadowRoot(); root; root = root->olderShadowRoot())
        if (root->needsStyleRecalc())
            return true;

    return false;
}

void ShadowTree::recalcShadowTreeStyle(Node::StyleChange change)
{
    ShadowRoot* youngest = youngestShadowRoot();
    if (!youngest)
        return;

    if (needsReattachHostChildrenAndShadow())
        reattachHostChildrenAndShadow();
    else {
        for (Node* n = youngest->firstChild(); n; n = n->nextSibling()) {
            if (n->isElementNode())
                static_cast<Element*>(n)->recalcStyle(change);
            else if (n->isTextNode())
                toText(n)->recalcTextStyle(change);
        }
    }

    clearNeedsReattachHostChildrenAndShadow();
    for (ShadowRoot* root = youngestShadowRoot(); root; root = root->olderShadowRoot()) {
        root->clearNeedsStyleRecalc();
        root->clearChildNeedsStyleRecalc();
    }
}

bool ShadowTree::needsReattachHostChildrenAndShadow()
{
    return m_needsRecalculateContent || (youngestShadowRoot() && youngestShadowRoot()->hasInsertionPoint());
}

void ShadowTree::hostChildrenChanged()
{
    ASSERT(youngestShadowRoot());

    if (!youngestShadowRoot()->hasInsertionPoint())
        return;

    // This results in forced detaching/attaching of the shadow render tree. See ShadowRoot::recalcStyle().
    setNeedsReattachHostChildrenAndShadow();
}

void ShadowTree::setNeedsReattachHostChildrenAndShadow()
{
    m_needsRecalculateContent = true;

    host()->setNeedsStyleRecalc();
}

void ShadowTree::reattachHostChildrenAndShadow()
{
    ASSERT(youngestShadowRoot());

    Element* hostNode = youngestShadowRoot()->host();
    hostNode->detachChildrenIfNeeded();
    reattach();
    hostNode->attachChildrenIfNeeded();
}

HTMLContentSelector* ShadowTree::ensureSelector()
{
    if (!m_selector)
        m_selector = adoptPtr(new HTMLContentSelector());
    m_selector->willSelectOver(host());
    return m_selector.get();
}

} // namespace
