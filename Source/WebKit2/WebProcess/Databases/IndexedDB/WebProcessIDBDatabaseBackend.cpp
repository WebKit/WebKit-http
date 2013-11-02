/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE, INC. ``AS IS'' AND ANY
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

#include "config.h"
#include "WebProcessIDBDatabaseBackend.h"

#include "DatabaseProcessIDBDatabaseBackendMessages.h"
#include "DatabaseToWebProcessConnectionMessages.h"
#include "WebIDBFactoryBackend.h"
#include "WebProcess.h"
#include "WebToDatabaseProcessConnection.h"
#include <WebCore/IDBMetadata.h>
#include <WebCore/SecurityOrigin.h>

#if ENABLE(INDEXED_DATABASE)
#if ENABLE(DATABASE_PROCESS)

using namespace WebCore;

namespace WebKit {

static uint64_t generateBackendIdentifier()
{
    DEFINE_STATIC_LOCAL(uint64_t, identifier, (1));
    return identifier++;
}

WebProcessIDBDatabaseBackend::WebProcessIDBDatabaseBackend(WebIDBFactoryBackend& factory, const String& name)
    : m_databaseName(name)
    , m_backendIdentifier(generateBackendIdentifier())
    , m_factory(&factory)
{
}

WebProcessIDBDatabaseBackend::~WebProcessIDBDatabaseBackend()
{
}

void WebProcessIDBDatabaseBackend::openConnection(PassRefPtr<IDBCallbacks> prpCallbacks, PassRefPtr<IDBDatabaseCallbacks> prpDatabaseCallbacks, int64_t transactionId, uint64_t version)
{
    // FIXME: Make this message include necessary arguments, and save off these callbacks and arguments for later use.

    send(Messages::DatabaseProcessIDBDatabaseBackend::OpenConnection());
}

void WebProcessIDBDatabaseBackend::createObjectStore(int64_t transactionId, int64_t objectStoreId, const String& name, const IDBKeyPath&, bool autoIncrement)
{
    ASSERT_NOT_REACHED();
}

void WebProcessIDBDatabaseBackend::deleteObjectStore(int64_t transactionId, int64_t objectStoreId)
{
    ASSERT_NOT_REACHED();
}

void WebProcessIDBDatabaseBackend::createTransaction(int64_t transactionId, PassRefPtr<IDBDatabaseCallbacks>, const Vector<int64_t>& objectStoreIds, unsigned short )
{
    ASSERT_NOT_REACHED();
}

void WebProcessIDBDatabaseBackend::close(PassRefPtr<IDBDatabaseCallbacks>)
{
    ASSERT_NOT_REACHED();
}

void WebProcessIDBDatabaseBackend::commit(int64_t transactionId)
{
    ASSERT_NOT_REACHED();
}

void WebProcessIDBDatabaseBackend::abort(int64_t transactionId)
{
    ASSERT_NOT_REACHED();
}

void WebProcessIDBDatabaseBackend::abort(int64_t transactionId, PassRefPtr<IDBDatabaseError>)
{
    ASSERT_NOT_REACHED();
}

void WebProcessIDBDatabaseBackend::createIndex(int64_t transactionId, int64_t objectStoreId, int64_t indexId, const String& name, const IDBKeyPath&, bool unique, bool iEntry)
{
    ASSERT_NOT_REACHED();
}

void WebProcessIDBDatabaseBackend::deleteIndex(int64_t transactionId, int64_t objectStoreId, int64_t indexId)
{
    ASSERT_NOT_REACHED();
}

void WebProcessIDBDatabaseBackend::get(int64_t transactionId, int64_t objectStoreId, int64_t indexId, PassRefPtr<IDBKeyRange>, bool keyOnly, PassRefPtr<IDBCallbacks>)
{
    ASSERT_NOT_REACHED();
}

void WebProcessIDBDatabaseBackend::put(int64_t transactionId, int64_t objectStoreId, PassRefPtr<SharedBuffer> value, PassRefPtr<IDBKey>, PutMode, PassRefPtr<IDBCallbacks>, const Vector<int64_t>& indexIds, const Vector<IndexKeys>&)
{
    ASSERT_NOT_REACHED();
}

void WebProcessIDBDatabaseBackend::setIndexKeys(int64_t transactionId, int64_t objectStoreId, PassRefPtr<IDBKey> prpPrimaryKey, const Vector<int64_t>& indexIds, const Vector<IndexKeys>&)
{
    ASSERT_NOT_REACHED();
}

void WebProcessIDBDatabaseBackend::setIndexesReady(int64_t transactionId, int64_t objectStoreId, const Vector<int64_t>& indexIds)
{
    ASSERT_NOT_REACHED();
}

void WebProcessIDBDatabaseBackend::openCursor(int64_t transactionId, int64_t objectStoreId, int64_t indexId, PassRefPtr<IDBKeyRange>, IndexedDB::CursorDirection, bool keyOnly, TaskType, PassRefPtr<IDBCallbacks>)
{
    ASSERT_NOT_REACHED();
}

void WebProcessIDBDatabaseBackend::count(int64_t transactionId, int64_t objectStoreId, int64_t indexId, PassRefPtr<IDBKeyRange>, PassRefPtr<IDBCallbacks>)
{
    ASSERT_NOT_REACHED();
}

void WebProcessIDBDatabaseBackend::deleteRange(int64_t transactionId, int64_t objectStoreId, PassRefPtr<IDBKeyRange>, PassRefPtr<IDBCallbacks>)
{
    ASSERT_NOT_REACHED();
}

void WebProcessIDBDatabaseBackend::clear(int64_t transactionId, int64_t objectStoreId, PassRefPtr<IDBCallbacks>)
{
    ASSERT_NOT_REACHED();
}

CoreIPC::Connection* WebProcessIDBDatabaseBackend::messageSenderConnection()
{
    return WebProcess::shared().webToDatabaseProcessConnection()->connection();
}

void WebProcessIDBDatabaseBackend::establishDatabaseProcessBackend()
{
    send(Messages::DatabaseToWebProcessConnection::EstablishIDBDatabaseBackend(m_backendIdentifier));
}

IDBBackingStoreInterface* WebProcessIDBDatabaseBackend::backingStore() const
{
    ASSERT_NOT_REACHED();
    return 0;
}

int64_t WebProcessIDBDatabaseBackend::id() const
{
    ASSERT_NOT_REACHED();
    return 0;
}

void WebProcessIDBDatabaseBackend::addObjectStore(const IDBObjectStoreMetadata&, int64_t newMaxObjectStoreId)
{
    ASSERT_NOT_REACHED();
}

void WebProcessIDBDatabaseBackend::removeObjectStore(int64_t objectStoreId)
{
    ASSERT_NOT_REACHED();
}

void WebProcessIDBDatabaseBackend::addIndex(int64_t objectStoreId, const IDBIndexMetadata&, int64_t newMaxIndexId)
{
    ASSERT_NOT_REACHED();
}

void WebProcessIDBDatabaseBackend::removeIndex(int64_t objectStoreId, int64_t indexId)
{
    ASSERT_NOT_REACHED();
}

const IDBDatabaseMetadata& WebProcessIDBDatabaseBackend::metadata() const
{
    ASSERT_NOT_REACHED();
    return *((IDBDatabaseMetadata*)0);
}

void WebProcessIDBDatabaseBackend::setCurrentVersion(uint64_t)
{
    ASSERT_NOT_REACHED();
}

bool WebProcessIDBDatabaseBackend::hasPendingSecondHalfOpen()
{
    ASSERT_NOT_REACHED();
    return false;
}

void WebProcessIDBDatabaseBackend::setPendingSecondHalfOpen(PassOwnPtr<IDBPendingOpenCall>)
{
    ASSERT_NOT_REACHED();
}

IDBFactoryBackendInterface& WebProcessIDBDatabaseBackend::factoryBackend()
{
    ASSERT_NOT_REACHED();
    return *((IDBFactoryBackendInterface*)0);
}

} // namespace WebKit

#endif // ENABLE(DATABASE_PROCESS)
#endif // ENABLE(INDEXED_DATABASE)
