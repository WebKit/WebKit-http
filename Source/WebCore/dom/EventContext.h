/*
 * Copyright (C) 2010 Google Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 *
 */

#ifndef EventContext_h
#define EventContext_h

#include "EventTarget.h"
#include "Node.h"
#include "TreeScope.h"
#include <wtf/RefPtr.h>

namespace WebCore {

class Event;

class EventContext {
public:
    // FIXME: Use ContainerNode instead of Node.
    EventContext(PassRefPtr<Node>, PassRefPtr<EventTarget> currentTarget, PassRefPtr<EventTarget> target);

    Node* node() const;
    EventTarget* target() const;
    EventTarget* relatedTarget() const;
    bool currentTargetSameAsTarget() const;
    void handleLocalEvents(Event*) const;
    void setRelatedTarget(PassRefPtr<EventTarget>);

private:
#ifndef NDEBUG
    bool isUnreachableNode(EventTarget*);
    bool isReachable(Node*);
#endif
    RefPtr<Node> m_node;
    RefPtr<EventTarget> m_currentTarget;
    RefPtr<EventTarget> m_target;
    RefPtr<EventTarget> m_relatedTarget;
};

inline Node* EventContext::node() const
{
    return m_node.get();
}

inline EventTarget* EventContext::target() const
{
    return m_target.get();
}

inline bool EventContext::currentTargetSameAsTarget() const
{
    return m_currentTarget.get() == m_target.get();
}

inline EventTarget* EventContext::relatedTarget() const
{
    return m_relatedTarget.get();
}

inline void EventContext::setRelatedTarget(PassRefPtr<EventTarget> relatedTarget)
{
    ASSERT(!isUnreachableNode(relatedTarget.get()));
    m_relatedTarget = relatedTarget;
}

#ifndef NDEBUG
inline bool EventContext::isUnreachableNode(EventTarget* target)
{
    // FIXME: Checks also for SVG elements.
    return target && target->toNode() && !target->toNode()->isSVGElement() && !isReachable(target->toNode());
}

inline bool EventContext::isReachable(Node* target)
{
    ASSERT(target);
    TreeScope* targetScope = target->treeScope();
    for (TreeScope* scope = m_node->treeScope(); scope; scope = scope->parentTreeScope()) {
        if (scope == targetScope)
            return true;
    }
    return false;
}
#endif

}

#endif // EventContext_h
