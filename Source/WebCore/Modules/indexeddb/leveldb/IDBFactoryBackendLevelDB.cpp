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

#include "config.h"
#include "IDBFactoryBackendLevelDB.h"

#include "DOMStringList.h"
#include "IDBBackingStoreLevelDB.h"
#include "IDBCursorBackend.h"
#include "IDBDatabaseBackend.h"
#include "IDBDatabaseException.h"
#include "IDBServerConnectionLevelDB.h"
#include "IDBTransactionBackend.h"
#include "IDBTransactionCoordinator.h"
#include "Logging.h"
#include "SecurityOrigin.h"

#if ENABLE(INDEXED_DATABASE) && USE(LEVELDB)

namespace WebCore {

template<typename K, typename M>
static void cleanWeakMap(HashMap<K, WeakPtr<M> >& map)
{
    HashMap<K, WeakPtr<M> > other;
    other.swap(map);

    typename HashMap<K, WeakPtr<M> >::const_iterator iter = other.begin();
    while (iter != other.end()) {
        if (iter->value.get())
            map.set(iter->key, iter->value);
        ++iter;
    }
}

static String computeFileIdentifier(const SecurityOrigin& securityOrigin)
{
    static const char levelDBFileSuffix[] = "@1";
    return securityOrigin.databaseIdentifier() + levelDBFileSuffix;
}

static String computeUniqueIdentifier(const String& name, const SecurityOrigin& securityOrigin)
{
    return computeFileIdentifier(securityOrigin) + name;
}

IDBFactoryBackendLevelDB::IDBFactoryBackendLevelDB(const String& databaseDirectory)
    : m_databaseDirectory(databaseDirectory)
{
}

IDBFactoryBackendLevelDB::~IDBFactoryBackendLevelDB()
{
}

void IDBFactoryBackendLevelDB::removeIDBDatabaseBackend(const String& uniqueIdentifier)
{
    ASSERT(m_databaseBackendMap.contains(uniqueIdentifier));
    m_databaseBackendMap.remove(uniqueIdentifier);
}

void IDBFactoryBackendLevelDB::getDatabaseNames(PassRefPtr<IDBCallbacks> callbacks, const SecurityOrigin& openingOrigin, const SecurityOrigin&, ScriptExecutionContext*, const String& dataDirectory)
{
    LOG(StorageAPI, "IDBFactoryBackendLevelDB::getDatabaseNames");
    RefPtr<IDBBackingStoreLevelDB> backingStore = openBackingStore(openingOrigin, dataDirectory);
    if (!backingStore) {
        callbacks->onError(IDBDatabaseError::create(IDBDatabaseException::UnknownError, "Internal error opening backing store for indexedDB.webkitGetDatabaseNames."));
        return;
    }

    RefPtr<DOMStringList> databaseNames = DOMStringList::create();

    Vector<String> foundNames = backingStore->getDatabaseNames();
    for (Vector<String>::const_iterator it = foundNames.begin(); it != foundNames.end(); ++it)
        databaseNames->append(*it);

    callbacks->onSuccess(databaseNames.release());
}

void IDBFactoryBackendLevelDB::deleteDatabase(const String& name, const SecurityOrigin& openingOrigin, const SecurityOrigin&, PassRefPtr<IDBCallbacks> callbacks, ScriptExecutionContext*, const String& dataDirectory)
{
    LOG(StorageAPI, "IDBFactoryBackendLevelDB::deleteDatabase");
    const String uniqueIdentifier = computeUniqueIdentifier(name, openingOrigin);

    IDBDatabaseBackendMap::iterator it = m_databaseBackendMap.find(uniqueIdentifier);
    if (it != m_databaseBackendMap.end()) {
        // If there are any connections to the database, directly delete the
        // database.
        it->value->deleteDatabase(callbacks);
        return;
    }

    // FIXME: Everything from now on should be done on another thread.
    RefPtr<IDBBackingStoreLevelDB> backingStore = openBackingStore(openingOrigin, dataDirectory);
    if (!backingStore) {
        callbacks->onError(IDBDatabaseError::create(IDBDatabaseException::UnknownError, "Internal error opening backing store for indexedDB.deleteDatabase."));
        return;
    }

    RefPtr<IDBServerConnection> serverConnection = IDBServerConnectionLevelDB::create(name, backingStore.get());
    RefPtr<IDBDatabaseBackend> databaseBackend = IDBDatabaseBackend::create(name, uniqueIdentifier, this, *serverConnection);
    if (databaseBackend) {
        m_databaseBackendMap.set(uniqueIdentifier, databaseBackend.get());
        databaseBackend->deleteDatabase(callbacks);
        m_databaseBackendMap.remove(uniqueIdentifier);
    } else
        callbacks->onError(IDBDatabaseError::create(IDBDatabaseException::UnknownError, "Internal error creating database backend for indexedDB.deleteDatabase."));
}

PassRefPtr<IDBBackingStoreLevelDB> IDBFactoryBackendLevelDB::openBackingStore(const SecurityOrigin& securityOrigin, const String& dataDirectory)
{
    const String fileIdentifier = computeFileIdentifier(securityOrigin);
    const bool openInMemory = dataDirectory.isEmpty();

    IDBBackingStoreLevelDBMap::iterator it2 = m_backingStoreMap.find(fileIdentifier);
    if (it2 != m_backingStoreMap.end() && it2->value.get())
        return it2->value.get();

    RefPtr<IDBBackingStoreLevelDB> backingStore;
    if (openInMemory)
        backingStore = IDBBackingStoreLevelDB::openInMemory(fileIdentifier);
    else
        backingStore = IDBBackingStoreLevelDB::open(securityOrigin, dataDirectory, fileIdentifier);

    if (backingStore) {
        cleanWeakMap(m_backingStoreMap);
        m_backingStoreMap.set(fileIdentifier, backingStore->createWeakPtr());
        // If an in-memory database, bind lifetime to this factory instance.
        if (openInMemory)
            m_sessionOnlyBackingStores.add(backingStore);

        // All backing stores associated with this factory should be of the same type.
        ASSERT(m_sessionOnlyBackingStores.isEmpty() || openInMemory);

        return backingStore.release();
    }

    return 0;
}

void IDBFactoryBackendLevelDB::open(const String& name, uint64_t version, int64_t transactionId, PassRefPtr<IDBCallbacks> callbacks, PassRefPtr<IDBDatabaseCallbacks> databaseCallbacks, const SecurityOrigin& openingOrigin, const SecurityOrigin&)
{
    LOG(StorageAPI, "IDBFactoryBackendLevelDB::open");
    const String uniqueIdentifier = computeUniqueIdentifier(name, openingOrigin);

    RefPtr<IDBDatabaseBackend> databaseBackend;
    IDBDatabaseBackendMap::iterator it = m_databaseBackendMap.find(uniqueIdentifier);
    if (it == m_databaseBackendMap.end()) {
        RefPtr<IDBBackingStoreLevelDB> backingStore = openBackingStore(openingOrigin, m_databaseDirectory);
        if (!backingStore) {
            callbacks->onError(IDBDatabaseError::create(IDBDatabaseException::UnknownError, "Internal error opening backing store for indexedDB.open."));
            return;
        }

        RefPtr<IDBServerConnection> serverConnection = IDBServerConnectionLevelDB::create(name, backingStore.get());
        databaseBackend = IDBDatabaseBackend::create(name, uniqueIdentifier, this, *serverConnection);
        if (databaseBackend)
            m_databaseBackendMap.set(uniqueIdentifier, databaseBackend.get());
        else {
            callbacks->onError(IDBDatabaseError::create(IDBDatabaseException::UnknownError, "Internal error creating database backend for indexeddb.open."));
            return;
        }
    } else
        databaseBackend = it->value;

    databaseBackend->openConnection(callbacks, databaseCallbacks, transactionId, version);
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE) && USE(LEVELDB)
