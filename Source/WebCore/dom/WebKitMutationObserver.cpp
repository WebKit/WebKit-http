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

#include "config.h"

#if ENABLE(MUTATION_OBSERVERS)

#include "WebKitMutationObserver.h"

#include "MutationCallback.h"
#include "MutationRecord.h"
#include "Node.h"
#include <wtf/ListHashSet.h>

namespace WebCore {

PassRefPtr<WebKitMutationObserver> WebKitMutationObserver::create(PassRefPtr<MutationCallback> callback)
{
    return adoptRef(new WebKitMutationObserver(callback));
}

WebKitMutationObserver::WebKitMutationObserver(PassRefPtr<MutationCallback> callback)
    : m_callback(callback)
{
}

WebKitMutationObserver::~WebKitMutationObserver()
{
}

void WebKitMutationObserver::observe(Node* node, MutationObserverOptions options)
{
    // FIXME: More options composition work needs to be done here, e.g., validation.

    if (node->registerMutationObserver(this, options) == Node::MutationObserverRegistered)
        m_observedNodes.append(node);
}

void WebKitMutationObserver::disconnect()
{
    for (Vector<Node*>::iterator iter = m_observedNodes.begin(); iter !=  m_observedNodes.end(); ++iter)
        (*iter)->unregisterMutationObserver(this);

    m_observedNodes.clear();
}

void WebKitMutationObserver::observedNodeDestructed(Node* node)
{
    size_t index = m_observedNodes.find(node);
    ASSERT(index != notFound);
    if (index == notFound)
        return;

    m_observedNodes.remove(index);
}

typedef ListHashSet<RefPtr<WebKitMutationObserver> > MutationObserverSet;

static MutationObserverSet& activeMutationObservers()
{
    DEFINE_STATIC_LOCAL(MutationObserverSet, activeObservers, ());
    return activeObservers;
}

void WebKitMutationObserver::enqueueMutationRecord(PassRefPtr<MutationRecord> mutation)
{
    m_records.append(mutation);
    activeMutationObservers().add(this);
}

void WebKitMutationObserver::deliver()
{
    MutationRecordArray records;
    records.swap(m_records);
    m_callback->handleEvent(&records, this);
}

void WebKitMutationObserver::deliverAllMutations()
{
    while (!activeMutationObservers().isEmpty()) {
        MutationObserverSet::iterator iter = activeMutationObservers().begin();
        RefPtr<WebKitMutationObserver> observer = *iter;
        activeMutationObservers().remove(iter);
        observer->deliver();
    }
}

} // namespace WebCore

#endif // ENABLE(MUTATION_OBSERVERS)
