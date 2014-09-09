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

#ifndef MutationObserverRegistration_h
#define MutationObserverRegistration_h

#include "MutationObserver.h"
#include <wtf/HashSet.h>
#include <wtf/text/AtomicString.h>
#include <wtf/text/AtomicStringHash.h>

namespace WebCore {

class QualifiedName;

class MutationObserverRegistration {
public:
    static PassOwnPtr<MutationObserverRegistration> create(PassRefPtr<MutationObserver>, Node*, MutationObserverOptions, const HashSet<AtomicString>& attributeFilter);
    ~MutationObserverRegistration();

    void resetObservation(MutationObserverOptions, const HashSet<AtomicString>& attributeFilter);
    void observedSubtreeNodeWillDetach(Node*);
    void clearTransientRegistrations();
    bool hasTransientRegistrations() const { return m_transientRegistrationNodes && !m_transientRegistrationNodes->isEmpty(); }
    static void unregisterAndDelete(MutationObserverRegistration*);

    bool shouldReceiveMutationFrom(Node*, MutationObserver::MutationType, const QualifiedName* attributeName) const;
    bool isSubtree() const { return m_options & MutationObserver::Subtree; }

    MutationObserver* observer() const { return m_observer.get(); }
    MutationRecordDeliveryOptions deliveryOptions() const { return m_options & (MutationObserver::AttributeOldValue | MutationObserver::CharacterDataOldValue); }
    MutationObserverOptions mutationTypes() const { return m_options & MutationObserver::AllMutationTypes; }

    void addRegistrationNodesToSet(HashSet<Node*>&) const;

private:
    MutationObserverRegistration(PassRefPtr<MutationObserver>, Node*, MutationObserverOptions, const HashSet<AtomicString>& attributeFilter);

    RefPtr<MutationObserver> m_observer;
    Node* m_registrationNode;
    RefPtr<Node> m_registrationNodeKeepAlive;
    typedef HashSet<RefPtr<Node>> NodeHashSet;
    OwnPtr<NodeHashSet> m_transientRegistrationNodes;

    MutationObserverOptions m_options;
    HashSet<AtomicString> m_attributeFilter;
};

} // namespace WebCore

#endif // MutationObserverRegistration_h
