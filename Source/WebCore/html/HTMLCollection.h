/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2011, 2012 Apple Inc. All rights reserved.
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

#ifndef HTMLCollection_h
#define HTMLCollection_h

#include "Node.h"
#include "CollectionType.h"
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class Document;
class Element;
class NodeList;

class HTMLCollection {
public:
    static PassOwnPtr<HTMLCollection> create(Node* base, CollectionType);
    virtual ~HTMLCollection();

    void ref() { m_base->ref(); }
    void deref() { m_base->deref(); }

    // DOM API
    unsigned length() const;
    virtual Node* item(unsigned index) const;
    virtual Node* namedItem(const AtomicString& name) const;
    PassRefPtr<NodeList> tags(const String&);

    // Non-DOM API
    bool hasNamedItem(const AtomicString& name) const;
    void namedItems(const AtomicString& name, Vector<RefPtr<Node> >&) const;
    bool isEmpty() const
    {
        invalidateCacheIfNeeded();
        if (m_cache.hasLength)
            return !m_cache.length;
        return !m_cache.current && !item(0);
    }
    bool hasExactlyOneItem() const
    {
        invalidateCacheIfNeeded();
        if (m_cache.hasLength)
            return m_cache.length == 1;
        if (m_cache.current)
            return !m_cache.position && !item(1);
        return item(0) && !item(1);
    }

    Node* base() const { return m_base; }
    CollectionType type() const { return static_cast<CollectionType>(m_type); }

    void clearCache();
    void invalidateCacheIfNeeded() const;
protected:
    HTMLCollection(Node* base, CollectionType);

    virtual void updateNameCache() const;
    virtual Element* itemAfter(Element*) const;

    typedef HashMap<AtomicStringImpl*, OwnPtr<Vector<Element*> > > NodeCacheMap;
    static void append(NodeCacheMap&, const AtomicString&, Element*);

    mutable struct {
        NodeCacheMap idCache;
        NodeCacheMap nameCache;
        uint64_t version;
        Element* current;
        unsigned position;
        unsigned length;
        int elementsArrayPosition;
        bool hasLength;
        bool hasNameCache;

        void clear()
        {
            idCache.clear();
            nameCache.clear();
            version = 0;
            current = 0;
            position = 0;
            length = 0;
            elementsArrayPosition = 0;
            hasLength = false;
            hasNameCache = false;
        }
    } m_cache;

private:
    bool checkForNameMatch(Element*, bool checkName, const AtomicString& name) const;

    virtual unsigned calcLength() const;

    bool isAcceptableElement(Element*) const;

    bool m_includeChildren : 1;
    unsigned m_type : 5; // CollectionType

    Node* m_base;
};

} // namespace

#endif
