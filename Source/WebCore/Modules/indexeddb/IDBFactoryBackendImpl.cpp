/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
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

#include "config.h"
#include "IDBFactoryBackendImpl.h"

#include "DOMStringList.h"
#include "IDBDatabaseBackendImpl.h"
#include "IDBDatabaseException.h"
#include "IDBLevelDBBackingStore.h"
#include "IDBTransactionCoordinator.h"
#include "SecurityOrigin.h"
#include <wtf/Threading.h>
#include <wtf/UnusedParam.h>

#if ENABLE(INDEXED_DATABASE)

namespace WebCore {

static String computeFileIdentifier(SecurityOrigin* securityOrigin)
{
    static const char kLevelDBFileSuffix[] = "@1";
    return securityOrigin->databaseIdentifier() + kLevelDBFileSuffix;
}

static String computeUniqueIdentifier(const String& name, SecurityOrigin* securityOrigin)
{
    return computeFileIdentifier(securityOrigin) + name;
}

IDBFactoryBackendImpl::IDBFactoryBackendImpl()
    : m_transactionCoordinator(IDBTransactionCoordinator::create())
{
}

IDBFactoryBackendImpl::~IDBFactoryBackendImpl()
{
}

void IDBFactoryBackendImpl::removeIDBDatabaseBackend(const String& uniqueIdentifier)
{
    ASSERT(m_databaseBackendMap.contains(uniqueIdentifier));
    m_databaseBackendMap.remove(uniqueIdentifier);
}

void IDBFactoryBackendImpl::addIDBBackingStore(const String& fileIdentifier, IDBBackingStore* backingStore)
{
    ASSERT(!m_backingStoreMap.contains(fileIdentifier));
    m_backingStoreMap.set(fileIdentifier, backingStore);
}

void IDBFactoryBackendImpl::removeIDBBackingStore(const String& fileIdentifier)
{
    ASSERT(m_backingStoreMap.contains(fileIdentifier));
    m_backingStoreMap.remove(fileIdentifier);
}

void IDBFactoryBackendImpl::getDatabaseNames(PassRefPtr<IDBCallbacks> callbacks, PassRefPtr<SecurityOrigin> securityOrigin, Frame*, const String& dataDirectory)
{
    RefPtr<IDBBackingStore> backingStore = openBackingStore(securityOrigin, dataDirectory);
    if (!backingStore) {
        callbacks->onError(IDBDatabaseError::create(IDBDatabaseException::UNKNOWN_ERR, "Internal error."));
        return;
    }

    RefPtr<DOMStringList> databaseNames = DOMStringList::create();

    Vector<String> foundNames;
    backingStore->getDatabaseNames(foundNames);
    for (Vector<String>::const_iterator it = foundNames.begin(); it != foundNames.end(); ++it)
        databaseNames->append(*it);

    callbacks->onSuccess(databaseNames.release());
}

void IDBFactoryBackendImpl::open(const String& name, IDBCallbacks* callbacks, PassRefPtr<SecurityOrigin> securityOrigin, Frame*, const String& dataDirectory)
{
    openInternal(name, callbacks, securityOrigin, dataDirectory);
}

void IDBFactoryBackendImpl::openFromWorker(const String& name, IDBCallbacks* callbacks, PassRefPtr<SecurityOrigin> securityOrigin, WorkerContext*, const String& dataDirectory)
{
    openInternal(name, callbacks, securityOrigin, dataDirectory);
}

void IDBFactoryBackendImpl::deleteDatabase(const String& name, PassRefPtr<IDBCallbacks> callbacks, PassRefPtr<SecurityOrigin> securityOrigin, Frame*, const String& dataDirectory)
{
    const String uniqueIdentifier = computeUniqueIdentifier(name, securityOrigin.get());

    IDBDatabaseBackendMap::iterator it = m_databaseBackendMap.find(uniqueIdentifier);
    if (it != m_databaseBackendMap.end()) {
        // If there are any connections to the database, directly delete the
        // database.
        it->second->deleteDatabase(callbacks);
        return;
    }

    // FIXME: Everything from now on should be done on another thread.
    RefPtr<IDBBackingStore> backingStore = openBackingStore(securityOrigin, dataDirectory);
    if (!backingStore) {
        callbacks->onError(IDBDatabaseError::create(IDBDatabaseException::UNKNOWN_ERR, "Internal error."));
        return;
    }

    RefPtr<IDBDatabaseBackendImpl> databaseBackend = IDBDatabaseBackendImpl::create(name, backingStore.get(), m_transactionCoordinator.get(), this, uniqueIdentifier);
    m_databaseBackendMap.set(uniqueIdentifier, databaseBackend.get());
    databaseBackend->deleteDatabase(callbacks);
}

PassRefPtr<IDBBackingStore> IDBFactoryBackendImpl::openBackingStore(PassRefPtr<SecurityOrigin> securityOrigin, const String& dataDirectory)
{
    const String fileIdentifier = computeFileIdentifier(securityOrigin.get());

    RefPtr<IDBBackingStore> backingStore;
    IDBBackingStoreMap::iterator it2 = m_backingStoreMap.find(fileIdentifier);
    if (it2 != m_backingStoreMap.end())
        backingStore = it2->second;
    else {
#if USE(LEVELDB)
        backingStore = IDBLevelDBBackingStore::open(securityOrigin.get(), dataDirectory, fileIdentifier, this);
#else
        ASSERT_NOT_REACHED();
#endif
    }

    if (backingStore)
        return backingStore.release();

    return 0;
}

void IDBFactoryBackendImpl::openInternal(const String& name, IDBCallbacks* callbacks, PassRefPtr<SecurityOrigin> prpSecurityOrigin, const String& dataDirectory)
{
    RefPtr<SecurityOrigin> securityOrigin = prpSecurityOrigin;
    const String uniqueIdentifier = computeUniqueIdentifier(name, securityOrigin.get());

    IDBDatabaseBackendMap::iterator it = m_databaseBackendMap.find(uniqueIdentifier);
    if (it != m_databaseBackendMap.end()) {
        // If it's already been opened, we have to wait for any pending
        // setVersion calls to complete.
        it->second->openConnection(callbacks);
        return;
    }

    // FIXME: Everything from now on should be done on another thread.
    RefPtr<IDBBackingStore> backingStore = openBackingStore(securityOrigin, dataDirectory);
    if (!backingStore) {
        callbacks->onError(IDBDatabaseError::create(IDBDatabaseException::UNKNOWN_ERR, "Internal error."));
        return;
    }

    RefPtr<IDBDatabaseBackendImpl> databaseBackend = IDBDatabaseBackendImpl::create(name, backingStore.get(), m_transactionCoordinator.get(), this, uniqueIdentifier);
    callbacks->onSuccess(RefPtr<IDBDatabaseBackendInterface>(databaseBackend.get()).release());
    m_databaseBackendMap.set(uniqueIdentifier, databaseBackend.get());
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
