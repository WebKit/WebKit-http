/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#ifndef InspectorDOMStorageAgent_h
#define InspectorDOMStorageAgent_h

#include "InspectorWebAgentBase.h"
#include "InspectorWebBackendDispatchers.h"
#include "StorageArea.h"
#include <wtf/HashMap.h>
#include <wtf/text/WTFString.h>

namespace Inspector {
class InspectorArray;
class InspectorDOMStorageFrontendDispatcher;
}

namespace WebCore {

class Frame;
class InspectorPageAgent;
class InstrumentingAgents;
class Page;
class SecurityOrigin;
class Storage;

typedef String ErrorString;

class InspectorDOMStorageAgent : public InspectorAgentBase, public Inspector::InspectorDOMStorageBackendDispatcherHandler {
    WTF_MAKE_FAST_ALLOCATED;
public:
    InspectorDOMStorageAgent(InstrumentingAgents*, InspectorPageAgent*);
    ~InspectorDOMStorageAgent();

    virtual void didCreateFrontendAndBackend(Inspector::InspectorFrontendChannel*, Inspector::InspectorBackendDispatcher*) override;
    virtual void willDestroyFrontendAndBackend(Inspector::InspectorDisconnectReason) override;

    // Called from the front-end.
    virtual void enable(ErrorString*) override;
    virtual void disable(ErrorString*) override;
    virtual void getDOMStorageItems(ErrorString*, const RefPtr<Inspector::InspectorObject>& storageId, RefPtr<Inspector::TypeBuilder::Array<Inspector::TypeBuilder::Array<String>>>& items) override;
    virtual void setDOMStorageItem(ErrorString*, const RefPtr<Inspector::InspectorObject>& storageId, const String& key, const String& value) override;
    virtual void removeDOMStorageItem(ErrorString*, const RefPtr<Inspector::InspectorObject>& storageId, const String& key) override;

    // Called from the injected script.
    String storageId(Storage*);
    PassRefPtr<Inspector::TypeBuilder::DOMStorage::StorageId> storageId(SecurityOrigin*, bool isLocalStorage);

    // Called from InspectorInstrumentation
    void didDispatchDOMStorageEvent(const String& key, const String& oldValue, const String& newValue, StorageType, SecurityOrigin*, Page*);

private:
    PassRefPtr<StorageArea> findStorageArea(ErrorString*, const RefPtr<Inspector::InspectorObject>&, Frame*&);

    InspectorPageAgent* m_pageAgent;
    std::unique_ptr<Inspector::InspectorDOMStorageFrontendDispatcher> m_frontendDispatcher;
    RefPtr<Inspector::InspectorDOMStorageBackendDispatcher> m_backendDispatcher;
    bool m_enabled;
};

} // namespace WebCore

#endif // !defined(InspectorDOMStorageAgent_h)
