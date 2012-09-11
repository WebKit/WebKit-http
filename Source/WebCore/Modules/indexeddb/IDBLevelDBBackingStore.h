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

#ifndef IDBLevelDBBackingStore_h
#define IDBLevelDBBackingStore_h

#if ENABLE(INDEXED_DATABASE)
#if USE(LEVELDB)

#include "IDBBackingStore.h"
#include <wtf/OwnPtr.h>

namespace WebCore {

class LevelDBComparator;
class LevelDBDatabase;
class LevelDBTransaction;
class IDBFactoryBackendImpl;

class IDBLevelDBBackingStore : public IDBBackingStore {
public:
    static PassRefPtr<IDBBackingStore> open(SecurityOrigin*, const String& pathBase, const String& fileIdentifier, IDBFactoryBackendImpl*);
    virtual ~IDBLevelDBBackingStore();

    virtual void getDatabaseNames(Vector<String>& foundNames);
    virtual bool getIDBDatabaseMetaData(const String& name, String& foundVersion, int64_t& foundIntVersion, int64_t& foundId);
    virtual bool createIDBDatabaseMetaData(const String& name, const String& version, int64_t intVersion, int64_t& rowId);
    virtual bool updateIDBDatabaseMetaData(int64_t rowId, const String& version);
    virtual bool updateIDBDatabaseIntVersion(int64_t rowId, int64_t intVersion);
    virtual bool deleteDatabase(const String& name);

    virtual void getObjectStores(int64_t databaseId, Vector<int64_t>& foundIds, Vector<String>& foundNames, Vector<IDBKeyPath>& foundKeyPaths, Vector<bool>& foundAutoIncrementFlags);
    virtual bool createObjectStore(int64_t databaseId, const String& name, const IDBKeyPath&, bool autoIncrement, int64_t& assignedObjectStoreId);
    virtual void deleteObjectStore(int64_t databaseId, int64_t objectStoreId);
    virtual PassRefPtr<ObjectStoreRecordIdentifier> createInvalidRecordIdentifier();
    virtual String getObjectStoreRecord(int64_t databaseId, int64_t objectStoreId, const IDBKey&);
    virtual bool putObjectStoreRecord(int64_t databaseId, int64_t objectStoreId, const IDBKey&, const String& value, ObjectStoreRecordIdentifier*);
    virtual void clearObjectStore(int64_t databaseId, int64_t objectStoreId);
    virtual void deleteObjectStoreRecord(int64_t databaseId, int64_t objectStoreId, const ObjectStoreRecordIdentifier*);
    virtual int64_t getKeyGeneratorCurrentNumber(int64_t databaseId, int64_t objectStoreId);
    virtual bool maybeUpdateKeyGeneratorCurrentNumber(int64_t databaseId, int64_t objectStoreId, int64_t newState, bool checkCurrent);
    virtual bool keyExistsInObjectStore(int64_t databaseId, int64_t objectStoreId, const IDBKey&, ObjectStoreRecordIdentifier* foundRecordIdentifier);

    virtual bool forEachObjectStoreRecord(int64_t databaseId, int64_t objectStoreId, ObjectStoreRecordCallback&);

    virtual void getIndexes(int64_t databaseId, int64_t objectStoreId, Vector<int64_t>& foundIds, Vector<String>& foundNames, Vector<IDBKeyPath>& foundKeyPaths, Vector<bool>& foundUniqueFlags, Vector<bool>& foundMultiEntryFlags);
    virtual bool createIndex(int64_t databaseId, int64_t objectStoreId, const String& name, const IDBKeyPath&, bool isUnique, bool isMultiEntry, int64_t& indexId);
    virtual void deleteIndex(int64_t databaseId, int64_t objectStoreId, int64_t indexId);
    virtual bool putIndexDataForRecord(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKey&, const ObjectStoreRecordIdentifier*);
    virtual bool deleteIndexDataForRecord(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const ObjectStoreRecordIdentifier*);
    virtual PassRefPtr<IDBKey> getPrimaryKeyViaIndex(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKey&);
    virtual bool keyExistsInIndex(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKey& indexKey, RefPtr<IDBKey>& foundPrimaryKey);

    virtual PassRefPtr<Cursor> openObjectStoreKeyCursor(int64_t databaseId, int64_t objectStoreId, const IDBKeyRange*, IDBCursor::Direction);
    virtual PassRefPtr<Cursor> openObjectStoreCursor(int64_t databaseId, int64_t objectStoreId, const IDBKeyRange*, IDBCursor::Direction);
    virtual PassRefPtr<Cursor> openIndexKeyCursor(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKeyRange*, IDBCursor::Direction);
    virtual PassRefPtr<Cursor> openIndexCursor(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKeyRange*, IDBCursor::Direction);

    virtual PassRefPtr<Transaction> createTransaction();

    static bool backingStoreExists(SecurityOrigin*, const String& name, const String& pathBase);

protected:
    IDBLevelDBBackingStore(const String& identifier, IDBFactoryBackendImpl*, PassOwnPtr<LevelDBDatabase>);

private:
    bool findKeyInIndex(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKey&, Vector<char>& foundEncodedPrimaryKey);

    String m_identifier;
    RefPtr<IDBFactoryBackendImpl> m_factory;
    OwnPtr<LevelDBDatabase> m_db;
    OwnPtr<LevelDBComparator> m_comparator;
    RefPtr<LevelDBTransaction> m_currentTransaction;

    class Transaction : public IDBBackingStore::Transaction {
    public:
        static PassRefPtr<Transaction> create(IDBLevelDBBackingStore*);
        virtual void begin();
        virtual bool commit();
        virtual void rollback();

    private:
        Transaction(IDBLevelDBBackingStore*);
        IDBLevelDBBackingStore* m_backingStore;
    };
};

} // namespace WebCore


#endif // USE(LEVELDB)
#endif // ENABLE(INDEXED_DATABASE)

#endif // IDBLevelDBBackingStore_h
