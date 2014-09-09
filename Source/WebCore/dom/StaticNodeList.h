/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef StaticNodeList_h
#define StaticNodeList_h

#include "Element.h"
#include "NodeList.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class StaticNodeList final : public NodeList {
public:
    static PassRefPtr<StaticNodeList> adopt(Vector<Ref<Node>>& nodes)
    {
        RefPtr<StaticNodeList> nodeList = adoptRef(new StaticNodeList);
        nodeList->m_nodes.swap(nodes);
        return nodeList.release();
    }

    static PassRefPtr<StaticNodeList> createEmpty()
    {
        return adoptRef(new StaticNodeList);
    }

    virtual unsigned length() const override;
    virtual Node* item(unsigned index) const override;
    virtual Node* namedItem(const AtomicString&) const override;

private:
    StaticNodeList() { }

    Vector<Ref<Node>> m_nodes;
};

class StaticElementList final : public NodeList {
public:
    static PassRefPtr<StaticElementList> adopt(Vector<Ref<Element>>& elements)
    {
        RefPtr<StaticElementList> nodeList = adoptRef(new StaticElementList);
        nodeList->m_elements.swap(elements);
        return nodeList.release();
    }

    static PassRefPtr<StaticElementList> createEmpty()
    {
        return adoptRef(new StaticElementList);
    }

    virtual unsigned length() const override;
    virtual Node* item(unsigned index) const override;
    virtual Node* namedItem(const AtomicString&) const override;

private:
    StaticElementList()
    {
    }

    Vector<Ref<Element>> m_elements;
};

} // namespace WebCore

#endif // StaticNodeList_h
