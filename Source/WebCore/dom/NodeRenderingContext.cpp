/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
#include "NodeRenderingContext.h"

#include "ContainerNode.h"
#include "HTMLContentElement.h"
#include "HTMLContentSelector.h"
#include "Node.h"
#include "RenderFlowThread.h"
#include "RenderFullScreen.h"
#include "RenderObject.h"
#include "RenderView.h"
#include "ShadowRoot.h"
#include "ShadowRootList.h"

#if ENABLE(SVG)
#include "SVGNames.h"
#endif

namespace WebCore {

NodeRenderingContext::NodeRenderingContext(Node* node)
    : m_phase(AttachingNotInTree)
    , m_node(node)
    , m_parentNodeForRenderingAndStyle(0)
    , m_visualParentShadowRoot(0)
    , m_insertionPoint(0)
    , m_style(0)
    , m_parentFlowRenderer(0)
{
    ContainerNode* parent = m_node->parentOrHostNode();
    if (!parent)
        return;

    if (parent->isShadowRoot()) {
        m_phase = AttachingShadowChild;
        m_parentNodeForRenderingAndStyle = parent->shadowHost();
        return;
    }

    if (parent->isElementNode()) {
        if (toElement(parent)->hasShadowRoot())
            m_visualParentShadowRoot = toElement(parent)->shadowRootList()->youngestShadowRoot();

        if (m_visualParentShadowRoot) {
            if ((m_insertionPoint = m_visualParentShadowRoot->insertionPointFor(m_node))
                && m_visualParentShadowRoot->isSelectorActive()) {
                m_phase = AttachingDistributed;
                m_parentNodeForRenderingAndStyle = NodeRenderingContext(m_insertionPoint).parentNodeForRenderingAndStyle();
                return;
            }

            m_phase = AttachingNotDistributed;
            m_parentNodeForRenderingAndStyle = parent;
            return;
        }

        if (parent->isContentElement()) {
            HTMLContentElement* shadowContentElement = toHTMLContentElement(parent);
            if (!shadowContentElement->hasSelection()) {
                m_phase = AttachingFallback;
                m_parentNodeForRenderingAndStyle = NodeRenderingContext(parent).parentNodeForRenderingAndStyle();
                return;
            }
        }
    }

    m_phase = AttachingStraight;
    m_parentNodeForRenderingAndStyle = parent;
}

NodeRenderingContext::NodeRenderingContext(Node* node, RenderStyle* style)
    : m_phase(Calculating)
    , m_node(node)
    , m_parentNodeForRenderingAndStyle(0)
    , m_visualParentShadowRoot(0)
    , m_insertionPoint(0)
    , m_style(style)
    , m_parentFlowRenderer(0)
{
}

NodeRenderingContext::~NodeRenderingContext()
{
}

void NodeRenderingContext::setStyle(PassRefPtr<RenderStyle> style)
{
    m_style = style;
    moveToFlowThreadIfNeeded();
}

PassRefPtr<RenderStyle> NodeRenderingContext::releaseStyle()
{
    return m_style.release();
}

static RenderObject* nextRendererOf(HTMLContentElement* parent, Node* current)
{
    HTMLContentSelection* currentSelection = parent->selections()->find(current);
    if (!currentSelection)
        return 0;

    for (HTMLContentSelection* selection = currentSelection->next(); selection; selection = selection->next()) {
        if (RenderObject* renderer = selection->node()->renderer())
            return renderer;
    }

    return 0;
}

static RenderObject* previousRendererOf(HTMLContentElement* parent, Node* current)
{
    RenderObject* lastRenderer = 0;

    for (HTMLContentSelection* selection = parent->selections()->first(); selection; selection = selection->next()) {
        if (selection->node() == current)
            break;
        if (RenderObject* renderer = selection->node()->renderer())
            lastRenderer = renderer;
    }

    return lastRenderer;
}

static RenderObject* firstRendererOf(HTMLContentElement* parent)
{
    for (HTMLContentSelection* selection = parent->selections()->first(); selection; selection = selection->next()) {
        if (RenderObject* renderer = selection->node()->renderer())
            return renderer;
    }

    return 0;
}

static RenderObject* lastRendererOf(HTMLContentElement* parent)
{
    for (HTMLContentSelection* selection = parent->selections()->last(); selection; selection = selection->previous()) {
        if (RenderObject* renderer = selection->node()->renderer())
            return renderer;
    }

    return 0;
}

RenderObject* NodeRenderingContext::nextRenderer() const
{
    ASSERT(m_node->renderer() || m_phase != Calculating);
    if (RenderObject* renderer = m_node->renderer())
        return renderer->nextSibling();

    if (m_parentFlowRenderer)
        return m_parentFlowRenderer->nextRendererForNode(m_node);

    if (m_phase == AttachingDistributed) {
        if (RenderObject* found = nextRendererOf(m_insertionPoint, m_node))
            return found;
        return NodeRenderingContext(m_insertionPoint).nextRenderer();
    }

    // Avoid an O(n^2) problem with this function by not checking for
    // nextRenderer() when the parent element hasn't attached yet.
    if (m_node->parentOrHostNode() && !m_node->parentOrHostNode()->attached() && m_phase != AttachingFallback)
        return 0;

    for (Node* node = m_node->nextSibling(); node; node = node->nextSibling()) {
        if (node->renderer()) {
            // Do not return elements that are attached to a different flow-thread.
            if (node->renderer()->style() && !node->renderer()->style()->flowThread().isEmpty())
                continue;
            return node->renderer();
        }
        if (node->isContentElement()) {
            if (RenderObject* first = firstRendererOf(toHTMLContentElement(node)))
                return first;
        }
    }

    if (m_phase == AttachingFallback)
        return NodeRenderingContext(m_node->parentNode()).nextRenderer();

    return 0;
}

RenderObject* NodeRenderingContext::previousRenderer() const
{
    ASSERT(m_node->renderer() || m_phase != Calculating);

    if (RenderObject* renderer = m_node->renderer())
        return renderer->previousSibling();

    if (m_parentFlowRenderer)
        return m_parentFlowRenderer->previousRendererForNode(m_node);

    if (m_phase == AttachingDistributed) {
        if (RenderObject* found = previousRendererOf(m_insertionPoint, m_node))
            return found;
        return NodeRenderingContext(m_insertionPoint).previousRenderer();
    }

    // FIXME: We should have the same O(N^2) avoidance as nextRenderer does
    // however, when I tried adding it, several tests failed.
    for (Node* node = m_node->previousSibling(); node; node = node->previousSibling()) {
        if (node->renderer()) {
            // Do not return elements that are attached to a different flow-thread.
            if (node->renderer()->style() && !node->renderer()->style()->flowThread().isEmpty())
                continue;
            return node->renderer();
        }
        if (node->isContentElement()) {
            if (RenderObject* last = lastRendererOf(toHTMLContentElement(node)))
                return last;
        }
    }

    if (m_phase == AttachingFallback)
        return NodeRenderingContext(m_node->parentNode()).previousRenderer();

    return 0;
}

RenderObject* NodeRenderingContext::parentRenderer() const
{
    if (RenderObject* renderer = m_node->renderer()) {
        ASSERT(m_phase == Calculating);
        return renderer->parent();
    }

    if (m_parentFlowRenderer)
        return m_parentFlowRenderer;

    ASSERT(m_phase != Calculating);
    return m_parentNodeForRenderingAndStyle ? m_parentNodeForRenderingAndStyle->renderer() : 0;
}

void NodeRenderingContext::hostChildrenChanged()
{
    if (m_phase == AttachingNotDistributed)
        m_visualParentShadowRoot->hostChildrenChanged();
}

bool NodeRenderingContext::shouldCreateRenderer() const
{
    ASSERT(m_phase != Calculating);
    ASSERT(parentNodeForRenderingAndStyle());

    if (m_phase == AttachingNotInTree || m_phase == AttachingNotDistributed)
        return false;

    RenderObject* parentRenderer = this->parentRenderer();
    if (!parentRenderer)
        return false;

    if (m_phase == AttachingStraight) {
        // FIXME: Ignoring canHaveChildren() in a case of shadow children might be wrong.
        // See https://bugs.webkit.org/show_bug.cgi?id=52423
        if (!parentRenderer->canHaveChildren())
            return false;

        if (m_visualParentShadowRoot)
            return false;
    }

    if (!m_parentNodeForRenderingAndStyle->childShouldCreateRenderer(m_node))
        return false;

    return true;
}

void NodeRenderingContext::moveToFlowThreadIfNeeded()
{
    if (!m_node->document()->cssRegionsEnabled())
        return;

    if (!m_node->isElementNode() || !m_style || m_style->flowThread().isEmpty())
        return;

#if ENABLE(SVG)
    // Allow only svg root elements to be directly collected by a render flow thread.
    if (m_node->isSVGElement()
        && (!(m_node->hasTagName(SVGNames::svgTag) && m_node->parentNode() && !m_node->parentNode()->isSVGElement())))
        return;
#endif

    m_flowThread = m_style->flowThread();
    ASSERT(m_node->document()->renderView());
    m_parentFlowRenderer = m_node->document()->renderView()->ensureRenderFlowThreadWithName(m_flowThread);
}

NodeRendererFactory::NodeRendererFactory(Node* node)
    : m_context(node)
{
}

RenderObject* NodeRendererFactory::createRenderer()
{
    Node* node = m_context.node();
    RenderObject* newRenderer = node->createRenderer(node->document()->renderArena(), m_context.style());
    if (!newRenderer)
        return 0;

    if (!m_context.parentRenderer()->isChildAllowed(newRenderer, m_context.style())) {
        newRenderer->destroy();
        return 0;
    }

    node->setRenderer(newRenderer);
    newRenderer->setAnimatableStyle(m_context.releaseStyle()); // setAnimatableStyle() can depend on renderer() already being set.
    return newRenderer;
}

void NodeRendererFactory::createRendererIfNeeded()
{
    Node* node = m_context.node();
    Document* document = node->document();
    if (!document->shouldCreateRenderers())
        return;

    ASSERT(!node->renderer());
    ASSERT(document->shouldCreateRenderers());

    // FIXME: This side effect should be visible from attach() code.
    m_context.hostChildrenChanged();

    if (!m_context.shouldCreateRenderer())
        return;

    Element* element = node->isElementNode() ? toElement(node) : 0;
    if (element)
        m_context.setStyle(element->styleForRenderer());
    else if (RenderObject* parentRenderer = m_context.parentRenderer())
        m_context.setStyle(parentRenderer->style());

    if (!node->rendererIsNeeded(m_context)) {
        if (element && m_context.style()->affectedByEmpty())
            element->setStyleAffectedByEmpty();
        return;
    }

    RenderObject* parentRenderer = m_context.hasFlowThreadParent() ? m_context.parentFlowRenderer() : m_context.parentRenderer();
    // Do not call m_context.nextRenderer() here in the first clause, because it expects to have
    // the renderer added to its parent already.
    RenderObject* nextRenderer = m_context.hasFlowThreadParent() ? m_context.parentFlowRenderer()->nextRendererForNode(node) : m_context.nextRenderer();
    RenderObject* newRenderer = createRenderer();

#if ENABLE(FULLSCREEN_API)
    if (document->webkitIsFullScreen() && document->webkitCurrentFullScreenElement() == node)
        newRenderer = RenderFullScreen::wrapRenderer(newRenderer, document);
#endif

    if (!newRenderer)
        return;

    // Note: Adding newRenderer instead of renderer(). renderer() may be a child of newRenderer.
    parentRenderer->addChild(newRenderer, nextRenderer);
}

}
