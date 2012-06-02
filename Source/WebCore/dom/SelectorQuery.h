/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef SelectorQuery_h
#define SelectorQuery_h

#include "CSSSelectorList.h"
#include "SelectorChecker.h"
#include <wtf/Vector.h>

namespace WebCore {

typedef int ExceptionCode;
    
class CSSSelector;
class Element;
class Node;
class NodeList;

class SelectorDataList {
public:
    SelectorDataList();

    void initialize(const CSSSelectorList&);

    int size() const { return m_selectors.size(); }

    bool matches(const SelectorChecker&, Element*) const;
    PassRefPtr<NodeList> queryAll(const SelectorChecker&, Node* rootNode) const;
    PassRefPtr<Element> queryFirst(const SelectorChecker&, Node* rootNode) const;

private:
    struct SelectorData {
        SelectorData(CSSSelector* selector, bool isFastCheckable) : selector(selector), isFastCheckable(isFastCheckable) { }
        CSSSelector* selector;
        bool isFastCheckable;
    };

    bool canUseIdLookup(Node* rootNode) const;
    template <bool firstMatchOnly>
    void execute(const SelectorChecker&, Node* rootNode, Vector<RefPtr<Node> >&) const;

    Vector<SelectorData> m_selectors;
};

class SelectorQuery {
    WTF_MAKE_NONCOPYABLE(SelectorQuery);
public:
    SelectorQuery() { };
    void initialize(const CSSSelectorList&);
    PassRefPtr<NodeList> queryAll(Node* rootNode) const;
    PassRefPtr<Element> queryFirst(Node* rootNode) const;
private:
    SelectorDataList m_selectors;
};

class SelectorQueryCache {
    WTF_MAKE_NONCOPYABLE(SelectorQueryCache);
    WTF_MAKE_FAST_ALLOCATED;
public:
    SelectorQueryCache() { };

    SelectorQuery* add(const AtomicString&, Document*, ExceptionCode&);
    void invalidate();

private:

    class Entry {
        WTF_MAKE_NONCOPYABLE(Entry);
        WTF_MAKE_FAST_ALLOCATED;
    public:
        explicit Entry(CSSSelectorList&);
        SelectorQuery* selectorQuery() { return &m_selectorQuery; };

    private:
        // m_querySelectorList keeps the lifetime of CSSSelectors in m_selectorQuery.
        // Since m_selectorQuery just holds pointers to CSSSelector objects,
        // m_querySelectorList must not be destructed before m_selectorQuery is destructed.
        CSSSelectorList m_querySelectorList;
        SelectorQuery m_selectorQuery;
    };

    HashMap<AtomicString, OwnPtr<SelectorQueryCache::Entry> > m_entries;
};

}

#endif
