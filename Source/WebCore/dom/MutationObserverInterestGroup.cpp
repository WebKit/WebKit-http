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

#include "MutationObserverInterestGroup.h"

#include "MutationObserverRegistration.h"
#include "MutationRecord.h"
#include "Node.h"
#include "QualifiedName.h"

namespace WebCore {

PassOwnPtr<MutationObserverInterestGroup> MutationObserverInterestGroup::createIfNeeded(Node* target, WebKitMutationObserver::MutationType type, const AtomicString& attributeName, MutationRecordDeliveryOptions oldValueFlag)
{
    HashMap<WebKitMutationObserver*, MutationRecordDeliveryOptions> observers;
    target->getRegisteredMutationObserversOfType(observers, type, attributeName);
    if (observers.isEmpty())
        return nullptr;

    return adoptPtr(new MutationObserverInterestGroup(observers, oldValueFlag));
}

MutationObserverInterestGroup::MutationObserverInterestGroup(HashMap<WebKitMutationObserver*, MutationRecordDeliveryOptions>& observers, MutationRecordDeliveryOptions oldValueFlag)
    : m_oldValueFlag(oldValueFlag)
{
    ASSERT(!observers.isEmpty());
    m_observers.swap(observers);
}

bool MutationObserverInterestGroup::isOldValueRequested()
{
    for (HashMap<WebKitMutationObserver*, MutationRecordDeliveryOptions>::iterator iter = m_observers.begin(); iter != m_observers.end(); ++iter) {
        if (hasOldValue(iter->second))
            return true;
    }
    return false;
}

void MutationObserverInterestGroup::enqueueMutationRecord(PassRefPtr<MutationRecord> prpMutation)
{
    RefPtr<MutationRecord> mutation = prpMutation;
    RefPtr<MutationRecord> mutationWithNullOldValue;
    for (HashMap<WebKitMutationObserver*, MutationRecordDeliveryOptions>::iterator iter = m_observers.begin(); iter != m_observers.end(); ++iter) {
        WebKitMutationObserver* observer = iter->first;
        if (hasOldValue(iter->second)) {
            observer->enqueueMutationRecord(mutation);
            continue;
        }
        if (!mutationWithNullOldValue) {
            if (mutation->oldValue().isNull())
                mutationWithNullOldValue = mutation;
            else
                mutationWithNullOldValue = MutationRecord::createWithNullOldValue(mutation).get();
        }
        observer->enqueueMutationRecord(mutationWithNullOldValue);
    }
}

} // namespace WebCore

#endif // ENABLE(MUTATION_OBSERVERS)
