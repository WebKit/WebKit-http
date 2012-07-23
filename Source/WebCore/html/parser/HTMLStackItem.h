/*
 * Copyright (C) 2012 Company 100, Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GOOGLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef HTMLStackItem_h
#define HTMLStackItem_h

#include "Element.h"
#include "HTMLNames.h"
#include "HTMLToken.h"

#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/text/AtomicString.h>

namespace WebCore {

class ContainerNode;

class HTMLStackItem : public RefCounted<HTMLStackItem> {
public:
    // DocumentFragment case.
    static PassRefPtr<HTMLStackItem> create(PassRefPtr<ContainerNode> node)
    {
        return adoptRef(new HTMLStackItem(node));
    }

    // Used by HTMLElementStack and HTMLFormattingElementList.
    static PassRefPtr<HTMLStackItem> create(PassRefPtr<ContainerNode> node, PassRefPtr<AtomicHTMLToken> token, const AtomicString& namespaceURI = HTMLNames::xhtmlNamespaceURI)
    {
        return adoptRef(new HTMLStackItem(node, token, namespaceURI));
    }

    Element* element() const { return toElement(m_node.get()); }
    ContainerNode* node() const { return m_node.get(); }

    AtomicHTMLToken* token() { return m_token.get(); }
    const AtomicString& namespaceURI() const { return m_namespaceURI; }

private:
    HTMLStackItem(PassRefPtr<ContainerNode> node)
        : m_node(node)
        , m_isDocumentFragmentNode(true)
    {
    }

    HTMLStackItem(PassRefPtr<ContainerNode> node, PassRefPtr<AtomicHTMLToken> token, const AtomicString& namespaceURI = HTMLNames::xhtmlNamespaceURI)
        : m_node(node)
        , m_token(token)
        , m_namespaceURI(namespaceURI)
        , m_isDocumentFragmentNode(false)
    {
    }

    RefPtr<ContainerNode> m_node;

    RefPtr<AtomicHTMLToken> m_token;
    AtomicString m_namespaceURI;
    bool m_isDocumentFragmentNode;
};

} // namespace WebCore

#endif // HTMLStackItem_h
