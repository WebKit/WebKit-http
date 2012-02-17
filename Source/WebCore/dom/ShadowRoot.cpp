/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
#include "ShadowRoot.h"

#include "Document.h"
#include "Element.h"
#include "HTMLContentElement.h"
#include "HTMLContentSelector.h"
#include "HTMLNames.h"
#include "NodeRareData.h"
#include "ShadowRootList.h"
#include "SVGNames.h"
#include "Text.h"

#if ENABLE(SHADOW_DOM)
#include "RuntimeEnabledFeatures.h"
#endif

namespace WebCore {

ShadowRoot::ShadowRoot(Document* document)
    : DocumentFragment(document, CreateShadowRoot)
    , TreeScope(this)
    , m_prev(0)
    , m_next(0)
    , m_applyAuthorSheets(false)
    , m_needsRecalculateContent(false)
{
    ASSERT(document);
    
    // Assume document as parent scope.
    setParentTreeScope(document);
    // Shadow tree scopes have the scope pointer point to themselves.
    // This way, direct children will receive the correct scope pointer.
    ensureRareData()->setTreeScope(this);
}

ShadowRoot::~ShadowRoot()
{
    ASSERT(!m_prev);
    ASSERT(!m_next);

    // We must call clearRareData() here since a ShadowRoot class inherits TreeScope
    // as well as Node. See a comment on TreeScope.h for the reason.
    if (hasRareData())
        clearRareData();
}

static bool allowsAuthorShadowRoot(Element* element)
{
    // FIXME: MEDIA recreates shadow root dynamically.
    // https://bugs.webkit.org/show_bug.cgi?id=77936
    if (element->hasTagName(HTMLNames::videoTag) || element->hasTagName(HTMLNames::audioTag))
        return false;

    // FIXME: ValidationMessage recreates shadow root dynamically.
    // https://bugs.webkit.org/show_bug.cgi?id=77937
    // Especially, INPUT recreates shadow root dynamically.
    // https://bugs.webkit.org/show_bug.cgi?id=77930
    if (element->isFormControlElement())
        return false;

    // FIXME: We disable multiple shadow subtrees for SVG for while, because there will be problems to support it.
    // https://bugs.webkit.org/show_bug.cgi?id=78205
    // Especially SVG TREF recreates shadow root dynamically.
    // https://bugs.webkit.org/show_bug.cgi?id=77938
    if (element->isSVGElement())
        return false;

    return true;
}

PassRefPtr<ShadowRoot> ShadowRoot::create(Element* element, ExceptionCode& ec)
{
    return create(element, CreatingAuthorShadowRoot, ec);
}

PassRefPtr<ShadowRoot> ShadowRoot::create(Element* element, ShadowRootCreationPurpose purpose, ExceptionCode& ec)
{
#if ENABLE(SHADOW_DOM)
    bool isMultipleShadowSubtreesEnabled = RuntimeEnabledFeatures::multipleShadowSubtreesEnabled();
#else
    bool isMultipleShadowSubtreesEnabled = false;
#endif

    if (!element || (!isMultipleShadowSubtreesEnabled && element->hasShadowRoot())) {
        ec = HIERARCHY_REQUEST_ERR;
        return 0;
    }

    // Since some elements recreates shadow root dynamically, multiple shadow subtrees won't work well in that element.
    // Until they are fixed, we disable adding author shadow root for them.
    if (purpose == CreatingAuthorShadowRoot && !allowsAuthorShadowRoot(element)) {
        ec = HIERARCHY_REQUEST_ERR;
        return 0;
    }

    ASSERT(purpose != CreatingUserAgentShadowRoot || !element->hasShadowRoot());
    RefPtr<ShadowRoot> shadowRoot = adoptRef(new ShadowRoot(element->document()));

    ec = 0;
    element->setShadowRoot(shadowRoot, ec);
    if (ec)
        return 0;

    return shadowRoot.release();
}

String ShadowRoot::nodeName() const
{
    return "#shadow-root";
}

PassRefPtr<Node> ShadowRoot::cloneNode(bool)
{
    // ShadowRoot should not be arbitrarily cloned.
    return 0;
}

bool ShadowRoot::childTypeAllowed(NodeType type) const
{
    switch (type) {
    case ELEMENT_NODE:
    case PROCESSING_INSTRUCTION_NODE:
    case COMMENT_NODE:
    case TEXT_NODE:
    case CDATA_SECTION_NODE:
    case ENTITY_REFERENCE_NODE:
        return true;
    default:
        return false;
    }
}

void ShadowRoot::recalcShadowTreeStyle(StyleChange change)
{
    if (needsReattachHostChildrenAndShadow())
        reattachHostChildrenAndShadow();
    else {
        for (Node* n = firstChild(); n; n = n->nextSibling()) {
            if (n->isElementNode())
                static_cast<Element*>(n)->recalcStyle(change);
            else if (n->isTextNode())
                toText(n)->recalcTextStyle(change);
        }
    }

    clearNeedsReattachHostChildrenAndShadow();
    clearNeedsStyleRecalc();
    clearChildNeedsStyleRecalc();
}

void ShadowRoot::setNeedsReattachHostChildrenAndShadow()
{
    m_needsRecalculateContent = true;
    if (shadowHost())
        shadowHost()->setNeedsStyleRecalc();
}

HTMLContentElement* ShadowRoot::insertionPointFor(Node* node) const
{
    if (!m_selector)
        return 0;
    HTMLContentSelection* found = m_selector->findFor(node);
    if (!found)
        return 0;
    return found->insertionPoint();
}

void ShadowRoot::hostChildrenChanged()
{
    if (!hasContentElement())
        return;

    // This results in forced detaching/attaching of the shadow render tree. See ShadowRoot::recalcStyle().
    setNeedsReattachHostChildrenAndShadow();
}

bool ShadowRoot::isSelectorActive() const
{
    return m_selector && m_selector->hasCandidates();
}

bool ShadowRoot::hasContentElement() const
{
    for (Node* n = firstChild(); n; n = n->traverseNextNode(this)) {
        if (n->isContentElement())
            return true;
    }

    return false;
}

bool ShadowRoot::applyAuthorSheets() const
{
    return m_applyAuthorSheets;
}

void ShadowRoot::setApplyAuthorSheets(bool value)
{
    m_applyAuthorSheets = value;
}

void ShadowRoot::attach()
{
    // Children of m_selector is populated lazily in
    // ensureSelector(), and here we just ensure that
    // it is in clean state.
    ASSERT(!m_selector || !m_selector->hasCandidates());
    DocumentFragment::attach();
    if (m_selector)
        m_selector->didSelect();
}

void ShadowRoot::reattachHostChildrenAndShadow()
{
    Node* hostNode = host();
    if (!hostNode)
        return;

    for (Node* child = hostNode->firstChild(); child; child = child->nextSibling()) {
        if (child->attached())
            child->detach();
    }

    reattach();

    for (Node* child = hostNode->firstChild(); child; child = child->nextSibling()) {
        if (!child->attached())
            child->attach();
    }
}

HTMLContentSelector* ShadowRoot::selector() const
{
    return m_selector.get();
}

HTMLContentSelector* ShadowRoot::ensureSelector()
{
    if (!m_selector)
        m_selector = adoptPtr(new HTMLContentSelector());
    m_selector->willSelectOver(this);
    return m_selector.get();
}


}
