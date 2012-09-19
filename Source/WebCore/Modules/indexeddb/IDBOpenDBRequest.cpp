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

#include "config.h"
#include "IDBOpenDBRequest.h"

#if ENABLE(INDEXED_DATABASE)

#include "IDBDatabase.h"
#include "IDBDatabaseCallbacksImpl.h"
#include "IDBPendingTransactionMonitor.h"
#include "IDBTracing.h"
#include "IDBUpgradeNeededEvent.h"
#include "ScriptExecutionContext.h"

namespace WebCore {

PassRefPtr<IDBOpenDBRequest> IDBOpenDBRequest::create(ScriptExecutionContext* context, PassRefPtr<IDBAny> source, PassRefPtr<IDBDatabaseCallbacksImpl> callbacks, int64_t version)
{
    RefPtr<IDBOpenDBRequest> request(adoptRef(new IDBOpenDBRequest(context, source, callbacks, version)));
    request->suspendIfNeeded();
    return request.release();
}

IDBOpenDBRequest::IDBOpenDBRequest(ScriptExecutionContext* context, PassRefPtr<IDBAny> source, PassRefPtr<IDBDatabaseCallbacksImpl> callbacks, int64_t version)
    : IDBRequest(context, source, IDBTransactionBackendInterface::NormalTask, 0)
    , m_databaseCallbacks(callbacks)
    , m_version(version)
{
    ASSERT(!m_result);
}

IDBOpenDBRequest::~IDBOpenDBRequest()
{
}

const AtomicString& IDBOpenDBRequest::interfaceName() const
{
    return eventNames().interfaceForIDBOpenDBRequest;
}

void IDBOpenDBRequest::onBlocked(int64_t oldVersion)
{
    if (!shouldEnqueueEvent())
        return;
    enqueueEvent(IDBUpgradeNeededEvent::create(oldVersion, m_version, eventNames().blockedEvent));
}

void IDBOpenDBRequest::onUpgradeNeeded(int64_t oldVersion, PassRefPtr<IDBTransactionBackendInterface> prpTransactionBackend, PassRefPtr<IDBDatabaseBackendInterface> prpDatabaseBackend)
{
    IDB_TRACE("IDBOpenDBRequest::onUpgradeNeeded()");
    if (!shouldEnqueueEvent())
        return;

    ASSERT(m_databaseCallbacks);

    RefPtr<IDBDatabaseBackendInterface> databaseBackend = prpDatabaseBackend;
    RefPtr<IDBTransactionBackendInterface> transactionBackend = prpTransactionBackend;
    RefPtr<IDBDatabase> idbDatabase = IDBDatabase::create(scriptExecutionContext(), databaseBackend, m_databaseCallbacks);
    m_databaseCallbacks->connect(idbDatabase.get());
    m_databaseCallbacks = 0;

    RefPtr<IDBTransaction> frontend = IDBTransaction::create(scriptExecutionContext(), transactionBackend, IDBTransaction::VERSION_CHANGE, idbDatabase.get(), this);
    transactionBackend->setCallbacks(frontend.get());
    m_transaction = frontend;
    m_result = IDBAny::create(idbDatabase.release());

    if (oldVersion == IDBDatabaseMetadata::NoIntVersion) {
        // This database hasn't had an integer version before.
        oldVersion = IDBDatabaseMetadata::DefaultIntVersion;
    }
    if (m_version == IDBDatabaseMetadata::NoIntVersion)
        m_version = 1;
    enqueueEvent(IDBUpgradeNeededEvent::create(oldVersion, m_version, eventNames().upgradeneededEvent));
}

void IDBOpenDBRequest::onSuccess(PassRefPtr<IDBDatabaseBackendInterface> backend)
{
    IDB_TRACE("IDBOpenDBRequest::onSuccess()");
    if (!shouldEnqueueEvent())
        return;

    RefPtr<IDBDatabase> idbDatabase;
    if (m_result) {
        idbDatabase = m_result->idbDatabase();
        ASSERT(idbDatabase);
        ASSERT(!m_databaseCallbacks);

        // If the connection closed between onUpgradeNeeded and onSuccess, an error
        // should be fired instead of success. The back-end may not be aware of
        // the closing state if the events are asynchronously delivered.
        if (idbDatabase->isClosePending()) {
            m_result.clear();
            onError(IDBDatabaseError::create(IDBDatabaseException::IDB_ABORT_ERR, "The connection was closed."));
            return;
        }
    } else {
        ASSERT(m_databaseCallbacks);
        idbDatabase = IDBDatabase::create(scriptExecutionContext(), backend, m_databaseCallbacks);
        m_databaseCallbacks->connect(idbDatabase.get());
        m_databaseCallbacks = 0;
        m_result = IDBAny::create(idbDatabase.get());
    }
    enqueueEvent(Event::create(eventNames().successEvent, false, false));
}

bool IDBOpenDBRequest::shouldEnqueueEvent() const
{
    if (m_contextStopped || !scriptExecutionContext())
        return false;
    ASSERT(m_readyState == PENDING || m_readyState == DONE);
    if (m_requestAborted)
        return false;
    return true;
}


} // namespace WebCore

#endif
