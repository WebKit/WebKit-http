/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
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

#ifndef ContentDistributor_h
#define ContentDistributor_h

#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

namespace WebCore {

class ContainerNode;
class Element;
class InsertionPoint;
class Node;
class ShadowRoot;

class ContentDistributor {
    WTF_MAKE_NONCOPYABLE(ContentDistributor);
public:
    enum Validity {
        Valid = 0,
        Invalidated = 1,
        Invalidating = 2,
        Undetermined = 3
    };

    ContentDistributor();
    ~ContentDistributor();

    void invalidateInsertionPointList();
    
    InsertionPoint* findInsertionPointFor(const Node* key) const;

    void distributeSelectionsTo(InsertionPoint*, Element* host);

    void invalidateDistribution(Element* host);
    void didShadowBoundaryChange(Element* host);

    static void ensureDistribution(ShadowRoot*);
    void distribute(Element* host);
    bool needsDistribution() const;

private:
    const Vector<RefPtr<InsertionPoint>>& ensureInsertionPointList(ShadowRoot*);

    bool invalidate(Element* host);

    void setValidity(Validity validity) { m_validity = validity; }
    bool isValid() const { return m_validity == Valid; }
    bool needsInvalidation() const { return m_validity != Invalidated; }

    Vector<RefPtr<InsertionPoint>> m_insertionPointList;
    HashMap<const Node*, RefPtr<InsertionPoint>> m_nodeToInsertionPoint;
    bool m_insertionPointListIsValid;
    unsigned m_validity : 2;
};

inline bool ContentDistributor::needsDistribution() const
{
    // During the invalidation, re-distribution should be supressed.
    return m_validity != Valid && m_validity != Invalidating;
}

}

#endif
