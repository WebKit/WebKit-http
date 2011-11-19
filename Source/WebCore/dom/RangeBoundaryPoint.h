/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef RangeBoundaryPoint_h
#define RangeBoundaryPoint_h

#include "Node.h"
#include "Position.h"

namespace WebCore {

class RangeBoundaryPoint {
public:
    explicit RangeBoundaryPoint(PassRefPtr<Node> container);

    const Position toPosition() const;

    Node* container() const;
    int offset() const;
    Node* childBefore() const;

    void clear();

    void set(PassRefPtr<Node> container, int offset, Node* childBefore);
    void setOffset(int offset);

    void setToBeforeChild(Node*);
    void setToStartOfNode(PassRefPtr<Node>);
    void setToEndOfNode(PassRefPtr<Node>);

    void childBeforeWillBeRemoved();
    void invalidateOffset() const;
    void ensureOffsetIsValid() const;

private:
    static const int invalidOffset = -1;
    
    RefPtr<Node> m_containerNode;
    mutable int m_offsetInContainer;
    RefPtr<Node> m_childBeforeBoundary;
};

inline RangeBoundaryPoint::RangeBoundaryPoint(PassRefPtr<Node> container)
    : m_containerNode(container)
    , m_offsetInContainer(0)
    , m_childBeforeBoundary(0)
{
    ASSERT(m_containerNode);
}

inline Node* RangeBoundaryPoint::container() const
{
    return m_containerNode.get();
}

inline Node* RangeBoundaryPoint::childBefore() const
{
    return m_childBeforeBoundary.get();
}

inline void RangeBoundaryPoint::ensureOffsetIsValid() const
{
    if (m_offsetInContainer >= 0)
        return;

    ASSERT(m_childBeforeBoundary);
    m_offsetInContainer = m_childBeforeBoundary->nodeIndex() + 1;
}

inline const Position RangeBoundaryPoint::toPosition() const
{
    ensureOffsetIsValid();
    return createLegacyEditingPosition(m_containerNode.get(), m_offsetInContainer);
}

inline int RangeBoundaryPoint::offset() const
{
    ensureOffsetIsValid();
    return m_offsetInContainer;
}

inline void RangeBoundaryPoint::clear()
{
    m_containerNode.clear();
    m_offsetInContainer = 0;
    m_childBeforeBoundary = 0;
}

inline void RangeBoundaryPoint::set(PassRefPtr<Node> container, int offset, Node* childBefore)
{
    ASSERT(container);
    ASSERT(offset >= 0);
    ASSERT(childBefore == (offset ? container->childNode(offset - 1) : 0));
    m_containerNode = container;
    m_offsetInContainer = offset;
    m_childBeforeBoundary = childBefore;
}

inline void RangeBoundaryPoint::setOffset(int offset)
{
    ASSERT(m_containerNode);
    ASSERT(m_containerNode->offsetInCharacters());
    ASSERT(m_offsetInContainer >= 0);
    ASSERT(!m_childBeforeBoundary);
    m_offsetInContainer = offset;
}

inline void RangeBoundaryPoint::setToBeforeChild(Node* child)
{
    ASSERT(child);
    ASSERT(child->parentNode());
    m_childBeforeBoundary = child->previousSibling();
    m_containerNode = child->parentNode();
    m_offsetInContainer = m_childBeforeBoundary ? invalidOffset : 0;
}

inline void RangeBoundaryPoint::setToStartOfNode(PassRefPtr<Node> container)
{
    ASSERT(container);
    m_containerNode = container;
    m_offsetInContainer = 0;
    m_childBeforeBoundary = 0;
}

inline void RangeBoundaryPoint::setToEndOfNode(PassRefPtr<Node> container)
{
    ASSERT(container);
    m_containerNode = container;
    if (m_containerNode->offsetInCharacters()) {
        m_offsetInContainer = m_containerNode->maxCharacterOffset();
        m_childBeforeBoundary = 0;
    } else {
        m_childBeforeBoundary = m_containerNode->lastChild();
        m_offsetInContainer = m_childBeforeBoundary ? invalidOffset : 0;
    }
}

inline void RangeBoundaryPoint::childBeforeWillBeRemoved()
{
    ASSERT(m_offsetInContainer);
    m_childBeforeBoundary = m_childBeforeBoundary->previousSibling();
    if (!m_childBeforeBoundary)
        m_offsetInContainer = 0;
    else if (m_offsetInContainer > 0)
        --m_offsetInContainer;
}

inline void RangeBoundaryPoint::invalidateOffset() const
{
    m_offsetInContainer = invalidOffset;
}

inline bool operator==(const RangeBoundaryPoint& a, const RangeBoundaryPoint& b)
{
    if (a.container() != b.container())
        return false;
    if (a.childBefore() || b.childBefore()) {
        if (a.childBefore() != b.childBefore())
            return false;
    } else {
        if (a.offset() != b.offset())
            return false;
    }
    return true;
}

}

#endif
