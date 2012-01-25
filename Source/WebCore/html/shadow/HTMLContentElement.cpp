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

#include "ContentInclusionSelector.h"
#include "ContentSelectorQuery.h"
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
    , m_inclusions(adoptPtr(new ShadowInclusionList()))
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
        ContentInclusionSelector* selector = root->ensureInclusions();
        selector->unselect(m_inclusions.get());
        selector->select(this, m_inclusions.get());
    }

    HTMLElement::attach();

    if (root) {
        for (ShadowInclusion* inclusion = m_inclusions->first(); inclusion; inclusion = inclusion->next())
            inclusion->content()->detach();
        for (ShadowInclusion* inclusion = m_inclusions->first(); inclusion; inclusion = inclusion->next())
            inclusion->content()->attach();
    }
}

void HTMLContentElement::detach()
{
    if (ShadowRoot* root = toShadowRoot(shadowTreeRootNode())) {
        if (ContentInclusionSelector* selector = root->inclusions())
            selector->unselect(m_inclusions.get());
    }

    ASSERT(m_inclusions->isEmpty());
    HTMLElement::detach();
}

const AtomicString& HTMLContentElement::select() const
{
    return getAttribute(selectAttr);
}

void HTMLContentElement::setSelect(const AtomicString& selectValue)
{
    setAttribute(selectAttr, selectValue);
}

}
