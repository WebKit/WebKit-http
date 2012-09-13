/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef IDBOpenDBRequest_h
#define IDBOpenDBRequest_h

#if ENABLE(INDEXED_DATABASE)

#include "IDBRequest.h"

namespace WebCore {

class IDBDatabaseCallbacksImpl;

class IDBOpenDBRequest : public IDBRequest {
public:
    static PassRefPtr<IDBOpenDBRequest> create(ScriptExecutionContext*, PassRefPtr<IDBAny> source, PassRefPtr<IDBDatabaseCallbacksImpl>, int64_t version);
    virtual ~IDBOpenDBRequest();

    using IDBRequest::onSuccess;

    virtual void onBlocked(int64_t existingVersion) OVERRIDE;
    virtual void onUpgradeNeeded(int64_t oldVersion, PassRefPtr<IDBTransactionBackendInterface>, PassRefPtr<IDBDatabaseBackendInterface>) OVERRIDE;
    virtual void onSuccess(PassRefPtr<IDBDatabaseBackendInterface>) OVERRIDE;

    // EventTarget
    virtual const AtomicString& interfaceName() const;

    DEFINE_ATTRIBUTE_EVENT_LISTENER(blocked);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(upgradeneeded);

protected:
    virtual bool shouldEnqueueEvent() const OVERRIDE;

private:
    IDBOpenDBRequest(ScriptExecutionContext*, PassRefPtr<IDBAny> source, PassRefPtr<IDBDatabaseCallbacksImpl>, int64_t version);

    RefPtr<IDBDatabaseCallbacksImpl> m_databaseCallbacks;
    int64_t m_version;
};

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)

#endif // IDBOpenDBRequest_h
