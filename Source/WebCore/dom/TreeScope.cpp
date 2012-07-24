/*
 * Copyright (C) 2011 Google Inc. All Rights Reserved.
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
#include "TreeScope.h"

#include "ComposedShadowTreeWalker.h"
#include "ContainerNode.h"
#include "ContextFeatures.h"
#include "DOMSelection.h"
#include "DOMWindow.h"
#include "Document.h"
#include "Element.h"
#include "FocusController.h"
#include "Frame.h"
#include "HTMLAnchorElement.h"
#include "HTMLFrameOwnerElement.h"
#include "HTMLMapElement.h"
#include "HTMLNames.h"
#include "IdTargetObserverRegistry.h"
#include "InsertionPoint.h"
#include "Page.h"
#include "ShadowRoot.h"
#include "TreeScopeAdopter.h"
#include <wtf/Vector.h>
#include <wtf/text/AtomicString.h>
#include <wtf/text/CString.h>

namespace WebCore {

using namespace HTMLNames;

TreeScope::TreeScope(ContainerNode* rootNode)
    : m_rootNode(rootNode)
    , m_parentTreeScope(0)
    , m_idTargetObserverRegistry(IdTargetObserverRegistry::create())
{
    ASSERT(rootNode);
}

TreeScope::~TreeScope()
{
    if (m_selection) {
        m_selection->clearTreeScope();
        m_selection = 0;
    }
}

void TreeScope::destroyTreeScopeData()
{
    m_elementsById.clear();
    m_imageMapsByName.clear();
}

void TreeScope::setParentTreeScope(TreeScope* newParentScope)
{
    // A document node cannot be re-parented.
    ASSERT(!rootNode()->isDocumentNode());
    // Every scope other than document needs a parent scope.
    ASSERT(newParentScope);

    m_parentTreeScope = newParentScope;
}

Element* TreeScope::getElementById(const AtomicString& elementId) const
{
    if (elementId.isEmpty())
        return 0;
    return m_elementsById.getElementById(elementId.impl(), this);
}

void TreeScope::addElementById(const AtomicString& elementId, Element* element)
{
    m_elementsById.add(elementId.impl(), element);
    m_idTargetObserverRegistry->notifyObservers(elementId);
}

void TreeScope::removeElementById(const AtomicString& elementId, Element* element)
{
    m_elementsById.remove(elementId.impl(), element);
    m_idTargetObserverRegistry->notifyObservers(elementId);
}

Node* TreeScope::ancestorInThisScope(Node* node) const
{
    while (node) {
        if (node->treeScope() == this)
            return node;
        if (!node->isInShadowTree())
            return 0;
        node = node->shadowAncestorNode();
    }

    return 0;
}

void TreeScope::addImageMap(HTMLMapElement* imageMap)
{
    AtomicStringImpl* name = imageMap->getName().impl();
    if (!name)
        return;
    m_imageMapsByName.add(name, imageMap);
}

void TreeScope::removeImageMap(HTMLMapElement* imageMap)
{
    AtomicStringImpl* name = imageMap->getName().impl();
    if (!name)
        return;
    m_imageMapsByName.remove(name, imageMap);
}

HTMLMapElement* TreeScope::getImageMap(const String& url) const
{
    if (url.isNull())
        return 0;
    size_t hashPos = url.find('#');
    String name = (hashPos == notFound ? url : url.substring(hashPos + 1)).impl();
    if (rootNode()->document()->isHTMLDocument())
        return static_cast<HTMLMapElement*>(m_imageMapsByName.getElementByLowercasedMapName(AtomicString(name.lower()).impl(), this));
    return static_cast<HTMLMapElement*>(m_imageMapsByName.getElementByMapName(AtomicString(name).impl(), this));
}

DOMSelection* TreeScope::getSelection() const
{
    if (!rootNode()->document()->frame())
        return 0;

    if (m_selection)
        return m_selection.get();

    // FIXME: The correct selection in Shadow DOM requires that Position can have a ShadowRoot
    // as a container. It is now enabled only if runtime Shadow DOM feature is enabled.
    // See https://bugs.webkit.org/show_bug.cgi?id=82697
#if ENABLE(SHADOW_DOM)
    if (ContextFeatures::shadowDOMEnabled(rootNode()->document())) {
        m_selection = DOMSelection::create(this);
        return m_selection.get();
    }
#endif

    if (this != rootNode()->document())
        return rootNode()->document()->getSelection();

    m_selection = DOMSelection::create(rootNode()->document());
    return m_selection.get();
}

Element* TreeScope::findAnchor(const String& name)
{
    if (name.isEmpty())
        return 0;
    if (Element* element = getElementById(name))
        return element;
    for (Node* node = rootNode(); node; node = node->traverseNextNode()) {
        if (node->hasTagName(aTag)) {
            HTMLAnchorElement* anchor = static_cast<HTMLAnchorElement*>(node);
            if (rootNode()->document()->inQuirksMode()) {
                // Quirks mode, case insensitive comparison of names.
                if (equalIgnoringCase(anchor->name(), name))
                    return anchor;
            } else {
                // Strict mode, names need to match exactly.
                if (anchor->name() == name)
                    return anchor;
            }
        }
    }
    return 0;
}

bool TreeScope::applyAuthorStyles() const
{
    return true;
}

bool TreeScope::resetStyleInheritance() const
{
    return false;
}

void TreeScope::adoptIfNeeded(Node* node)
{
    ASSERT(this);
    ASSERT(node);
    ASSERT(!node->isDocumentNode());
    ASSERT(!node->m_deletionHasBegun);
    TreeScopeAdopter adopter(node, this);
    if (adopter.needsScopeChange())
        adopter.execute();
}

static Node* focusedFrameOwnerElement(Frame* focusedFrame, Frame* currentFrame)
{
    for (; focusedFrame; focusedFrame = focusedFrame->tree()->parent()) {
        if (focusedFrame->tree()->parent() == currentFrame)
            return focusedFrame->ownerElement();
    }
    return 0;
}

Node* TreeScope::focusedNode()
{
    Document* document = rootNode()->document();
    Node* node = document->focusedNode();
    if (!node && document->page())
        node = focusedFrameOwnerElement(document->page()->focusController()->focusedFrame(), document->frame());
    if (!node)
        return 0;
    Vector<Node*> targetStack;
    Node* last = 0;
    for (ComposedShadowTreeParentWalker walker(node); walker.get(); walker.parentIncludingInsertionPointAndShadowRoot()) {
        Node* node = walker.get();
        if (targetStack.isEmpty())
            targetStack.append(node);
        else if (isInsertionPoint(node) && toInsertionPoint(node)->contains(last))
            targetStack.append(targetStack.last());
        if (node == rootNode())
            return targetStack.last();
        last = node;
        if (node->isShadowRoot()) {
            ASSERT(!targetStack.isEmpty());
            targetStack.removeLast();
        }
    }
    return 0;
}

static void listTreeScopes(Node* node, Vector<TreeScope*, 5>& treeScopes)
{
    while (true) {
        treeScopes.append(node->treeScope());
        Element* ancestor = node->shadowHost();
        if (!ancestor)
            break;
        node = ancestor;
    }
}

TreeScope* commonTreeScope(Node* nodeA, Node* nodeB)
{
    if (!nodeA || !nodeB)
        return 0;

    if (nodeA->treeScope() == nodeB->treeScope())
        return nodeA->treeScope();

    Vector<TreeScope*, 5> treeScopesA;
    listTreeScopes(nodeA, treeScopesA);

    Vector<TreeScope*, 5> treeScopesB;
    listTreeScopes(nodeB, treeScopesB);

    size_t indexA = treeScopesA.size();
    size_t indexB = treeScopesB.size();

    for (; indexA > 0 && indexB > 0 && treeScopesA[indexA - 1] == treeScopesB[indexB - 1]; --indexA, --indexB) { }

    return treeScopesA[indexA] == treeScopesB[indexB] ? treeScopesA[indexA] : 0;
}

} // namespace WebCore

