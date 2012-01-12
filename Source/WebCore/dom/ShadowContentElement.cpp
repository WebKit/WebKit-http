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
#include "ShadowContentElement.h"

#include "HTMLNames.h"
#include "QualifiedName.h"
#include "ShadowContentSelectorQuery.h"
#include "ShadowInclusionSelector.h"
#include "ShadowRoot.h"
#include <wtf/StdLibExtras.h>

namespace WebCore {

static const QualifiedName& selectAttr()
{
    DEFINE_STATIC_LOCAL(QualifiedName, attr, (nullAtom, "select", HTMLNames::xhtmlNamespaceURI));
    return attr;
}

PassRefPtr<ShadowContentElement> ShadowContentElement::create(Document* document, const AtomicString& select)
{
    DEFINE_STATIC_LOCAL(QualifiedName, tagName, (nullAtom, "webkitShadowContent", HTMLNames::divTag.namespaceURI()));
    return adoptRef(new ShadowContentElement(tagName, document, select));
}

ShadowContentElement::ShadowContentElement(const QualifiedName& name, Document* document, const AtomicString& select)
    : StyledElement(name, document, CreateHTMLElement)
    , m_inclusions(adoptPtr(new ShadowInclusionList()))
{
    setAttribute(selectAttr(), select);
}

ShadowContentElement::~ShadowContentElement()
{
}

void ShadowContentElement::attach()
{
    ASSERT(!firstChild()); // Currently doesn't support any light child.
    StyledElement::attach();

    if (ShadowRoot* root = toShadowRoot(shadowTreeRootNode())) {
        ShadowInclusionSelector* selector = root->ensureInclusions();
        selector->unselect(m_inclusions.get());
        selector->select(this, m_inclusions.get());
        for (ShadowInclusion* inclusion = m_inclusions->first(); inclusion; inclusion = inclusion->next())
            inclusion->content()->detach();
        for (ShadowInclusion* inclusion = m_inclusions->first(); inclusion; inclusion = inclusion->next())
            inclusion->content()->attach();
    }
}

void ShadowContentElement::detach()
{
    if (ShadowRoot* root = toShadowRoot(shadowTreeRootNode())) {
        if (ShadowInclusionSelector* selector = root->inclusions())
            selector->unselect(m_inclusions.get());
    }

    ASSERT(m_inclusions->isEmpty());
    StyledElement::detach();
}

const AtomicString& ShadowContentElement::select() const
{
    return getAttribute(selectAttr());
}

}
