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
#include "HTMLContentElement.h"

#include "ContentSelectorQuery.h"
#include "HTMLContentSelector.h"
#include "HTMLNames.h"
#include "QualifiedName.h"
#include "ShadowRoot.h"
#include <wtf/StdLibExtras.h>

namespace WebCore {

using HTMLNames::selectAttr;

static const QualifiedName& contentTagName()
{
#if ENABLE(SHADOW_DOM)
    return HTMLNames::contentTag;
#else
    DEFINE_STATIC_LOCAL(QualifiedName, tagName, (nullAtom, "webkitShadowContent", HTMLNames::divTag.namespaceURI()));
    return tagName;
#endif
}

PassRefPtr<HTMLContentElement> HTMLContentElement::create(Document* document)
{
    return adoptRef(new HTMLContentElement(contentTagName(), document));
}

PassRefPtr<HTMLContentElement> HTMLContentElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLContentElement(tagName, document));
}

HTMLContentElement::HTMLContentElement(const QualifiedName& name, Document* document)
    : HTMLElement(name, document)
    , m_selections(adoptPtr(new HTMLContentSelectionList()))
{
}

HTMLContentElement::~HTMLContentElement()
{
}

void HTMLContentElement::attach()
{
    ShadowRoot* root = toShadowRoot(shadowTreeRootNode());

    // Before calling StyledElement::attach, selector must be calculated.
    if (root) {
        HTMLContentSelector* selector = root->ensureSelector();
        selector->unselect(m_selections.get());
        selector->select(this, m_selections.get());
    }

    HTMLElement::attach();

    if (root) {
        for (HTMLContentSelection* selection = m_selections->first(); selection; selection = selection->next())
            selection->node()->attach();
    }
}

void HTMLContentElement::detach()
{
    if (ShadowRoot* root = toShadowRoot(shadowTreeRootNode())) {
        if (HTMLContentSelector* selector = root->selector())
            selector->unselect(m_selections.get());

        // When content element is detached, shadow tree should be recreated to re-calculate selector for
        // other content elements.
        root->setNeedsReattachHostChildrenAndShadow();
    }

    ASSERT(m_selections->isEmpty());
    HTMLElement::detach();
}

const AtomicString& HTMLContentElement::select() const
{
    return getAttribute(selectAttr);
}

bool HTMLContentElement::isSelectValid() const
{
    ContentSelectorQuery query(this);
    return query.isValidSelector();
}

void HTMLContentElement::setSelect(const AtomicString& selectValue)
{
    setAttribute(selectAttr, selectValue);
}

void HTMLContentElement::parseAttribute(Attribute* attr)
{
    if (attr->name() == selectAttr) {
        if (ShadowRoot* root = toShadowRoot(shadowTreeRootNode()))
            root->setNeedsReattachHostChildrenAndShadow();
    } else
        HTMLElement::parseAttribute(attr);
}

}
