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

#include "config.h"
#include "IDBBackingStore.h"

#if ENABLE(INDEXED_DATABASE)

#include "FileSystem.h"
#include "HistogramSupport.h"
#include "IDBFactoryBackendImpl.h"
#include "IDBKey.h"
#include "IDBKeyPath.h"
#include "IDBKeyRange.h"
#include "IDBLevelDBCoding.h"
#include "IDBMetadata.h"
#include "IDBTracing.h"
#include "LevelDBComparator.h"
#include "LevelDBDatabase.h"
#include "LevelDBIterator.h"
#include "LevelDBSlice.h"
#include "LevelDBTransaction.h"
#include "SecurityOrigin.h"
#include <wtf/Assertions.h>

namespace WebCore {

using namespace IDBLevelDBCoding;

const int64_t KeyGeneratorInitialNumber = 1; // From the IndexedDB specification.

enum IDBLevelDBBackingStoreInternalErrorType {
    IDBLevelDBBackingStoreReadError,
    IDBLevelDBBackingStoreWriteError,
    IDBLevelDBBackingStoreConsistencyError,
    IDBLevelDBBackingStoreReadErrorFindKeyInIndex,
    IDBLevelDBBackingStoreReadErrorGetIDBDatabaseMetaData,
    IDBLevelDBBackingStoreReadErrorGetIndexes,
    IDBLevelDBBackingStoreReadErrorGetKeyGeneratorCurrentNumber,
    IDBLevelDBBackingStoreReadErrorGetObjectStores,
    IDBLevelDBBackingStoreReadErrorGetRecord,
    IDBLevelDBBackingStoreReadErrorKeyExistsInObjectStore,
    IDBLevelDBBackingStoreReadErrorLoadCurrentRow,
    IDBLevelDBBackingStoreReadErrorSetupMetadata,
    IDBLevelDBBackingStoreReadErrorGetPrimaryKeyViaIndex,
    IDBLevelDBBackingStoreReadErrorKeyExistsInIndex,
    IDBLevelDBBackingStoreReadErrorVersionExists,
    IDBLevelDBBackingStoreReadErrorDeleteObjectStore,
    IDBLevelDBBackingStoreReadErrorSetMaxObjectStoreId,
    IDBLevelDBBackingStoreReadErrorSetMaxIndexId,
    IDBLevelDBBackingStoreReadErrorGetNewDatabaseId,
    IDBLevelDBBackingStoreReadErrorGetNewVersionNumber,
    IDBLevelDBBackingStoreInternalErrorMax,
};
static inline void recordInternalError(IDBLevelDBBackingStoreInternalErrorType type)
{
    HistogramSupport::histogramEnumeration("WebCore.IndexedDB.BackingStore.InternalError", type, IDBLevelDBBackingStoreInternalErrorMax);
}

// Use to signal conditions that usually indicate developer error, but could be caused by data corruption.
// A macro is used instead of an inline function so that the assert and log report the line number.
#define InternalError(type) \
    do { \
        ASSERT_NOT_REACHED(); \
        LOG_ERROR("Internal IndexedDB Error: %s", #type); \
        recordInternalError(type); \
    } while (0)

static void putBool(LevelDBTransaction* transaction, const LevelDBSlice& key, bool value)
{
    transaction->put(key, encodeBool(value));
}

template <typename DBOrTransaction>
static bool getInt(DBOrTransaction* db, const LevelDBSlice& key, int64_t& foundInt, bool& found)
{
    Vector<char> result;
    bool ok = db->safeGet(key, result, found);
    if (!ok)
        return false;
    if (!found)
        return true;

    foundInt = decodeInt(result.begin(), result.end());
    return true;
}

static void putInt(LevelDBTransaction* transaction, const LevelDBSlice& key, int64_t value)
{
    ASSERT(value >= 0);
    transaction->put(key, encodeInt(value));
}

template <typename DBOrTransaction>
WARN_UNUSED_RETURN static bool getVarInt(DBOrTransaction* db, const LevelDBSlice& key, int64_t& foundInt, bool& found)
{
    Vector<char> result;
    bool ok = db->safeGet(key, result, found);
    if (!ok)
        return false;
    if (!found)
        return true;

    found = decodeVarInt(result.begin(), result.end(), foundInt) == result.end();
    return true;
}

static void putVarInt(LevelDBTransaction* transaction, const LevelDBSlice& key, int64_t value)
{
    transaction->put(key, encodeVarInt(value));
}

template <typename DBOrTransaction>
WARN_UNUSED_RETURN static bool getString(DBOrTransaction* db, const LevelDBSlice& key, String& foundString, bool& found)
{
    Vector<char> result;
    found = false;
    bool ok = db->safeGet(key, result, found);
    if (!ok)
        return false;
    if (!found)
        return true;

    foundString = decodeString(result.begin(), result.end());
    return true;
}

static void putString(LevelDBTransaction* transaction, const LevelDBSlice& key, const String& value)
{
    transaction->put(key, encodeString(value));
}

static void putIDBKeyPath(LevelDBTransaction* transaction, const LevelDBSlice& key, const IDBKeyPath& value)
{
    transaction->put(key, encodeIDBKeyPath(value));
}

static int compareKeys(const LevelDBSlice& a, const LevelDBSlice& b)
{
    return compare(a, b);
}

static int compareIndexKeys(const LevelDBSlice& a, const LevelDBSlice& b)
{
    return compare(a, b, true);
}

class Comparator : public LevelDBComparator {
public:
    virtual int compare(const LevelDBSlice& a, const LevelDBSlice& b) const { return IDBLevelDBCoding::compare(a, b); }
    virtual const char* name() const { return "idb_cmp1"; }
};

// 0 - Initial version.
// 1 - Adds UserIntVersion to DatabaseMetaData.
// 2 - Adds DataVersion to to global metadata.
const int64_t latestKnownSchemaVersion = 2;
WARN_UNUSED_RETURN static bool isSchemaKnown(LevelDBDatabase* db, bool& known)
{
    int64_t dbSchemaVersion = 0;
    bool found = false;
    bool ok = getInt(db, SchemaVersionKey::encode(), dbSchemaVersion, found);
    if (!ok)
        return false;
    if (!found) {
        known = true;
        return true;
    }
    if (dbSchemaVersion > latestKnownSchemaVersion) {
        known = false;
        return true;
    }

    const uint32_t latestKnownDataVersion = SerializedScriptValue::wireFormatVersion();
    int64_t dbDataVersion = 0;
    ok = getInt(db, DataVersionKey::encode(), dbDataVersion, found);
    if (!ok)
        return false;
    if (!found) {
        known = true;
        return true;
    }

    if (dbDataVersion > latestKnownDataVersion) {
        known = false;
        return true;
    }

    known = true;
    return true;
}

WARN_UNUSED_RETURN static bool setUpMetadata(LevelDBDatabase* db, const String& origin)
{
    const uint32_t latestKnownDataVersion = SerializedScriptValue::wireFormatVersion();
    const Vector<char> schemaVersionKey = SchemaVersionKey::encode();
    const Vector<char> dataVersionKey = DataVersionKey::encode();

    RefPtr<LevelDBTransaction> transaction = LevelDBTransaction::create(db);

    int64_t dbSchemaVersion = 0;
    int64_t dbDataVersion = 0;
    bool found = false;
    bool ok = getInt(transaction.get(), schemaVersionKey, dbSchemaVersion, found);
    if (!ok) {
        InternalError(IDBLevelDBBackingStoreReadErrorSetupMetadata);
        return false;
    }
    if (!found) {
        // Initialize new backing store.
        dbSchemaVersion = latestKnownSchemaVersion;
        putInt(transaction.get(), schemaVersionKey, dbSchemaVersion);
        dbDataVersion = latestKnownDataVersion;
        putInt(transaction.get(), dataVersionKey, dbDataVersion);
    } else {
        // Upgrade old backing store.
        ASSERT(dbSchemaVersion <= latestKnownSchemaVersion);
        if (dbSchemaVersion < 1) {
            dbSchemaVersion = 1;
            putInt(transaction.get(), schemaVersionKey, dbSchemaVersion);
            const Vector<char> startKey = DatabaseNameKey::encodeMinKeyForOrigin(origin);
            const Vector<char> stopKey = DatabaseNameKey::encodeStopKeyForOrigin(origin);
            OwnPtr<LevelDBIterator> it = db->createIterator();
            for (it->seek(startKey); it->isValid() && compareKeys(it->key(), stopKey) < 0; it->next()) {
                int64_t databaseId = 0;
                found = false;
                bool ok = getInt(transaction.get(), it->key(), databaseId, found);
                if (!ok) {
                    InternalError(IDBLevelDBBackingStoreReadErrorSetupMetadata);
                    return false;
                }
                if (!found) {
                    InternalError(IDBLevelDBBackingStoreConsistencyError);
                    return false;
                }
                Vector<char> intVersionKey = DatabaseMetaDataKey::encode(databaseId, DatabaseMetaDataKey::UserIntVersion);
                putVarInt(transaction.get(), intVersionKey, IDBDatabaseMetadata::DefaultIntVersion);
            }
        }
        if (dbSchemaVersion < 2) {
            dbSchemaVersion = 2;
            putInt(transaction.get(), schemaVersionKey, dbSchemaVersion);
            dbDataVersion = SerializedScriptValue::wireFormatVersion();
            putInt(transaction.get(), dataVersionKey, dbDataVersion);
        }
    }

    // All new values will be written using this serialization version.
    found = false;
    ok = getInt(transaction.get(), dataVersionKey, dbDataVersion, found);
    if (!ok) {
        InternalError(IDBLevelDBBackingStoreReadErrorSetupMetadata);
        return false;
    }
    if (!found) {
        InternalError(IDBLevelDBBackingStoreConsistencyError);
        return false;
    }
    if (dbDataVersion < latestKnownDataVersion) {
        dbDataVersion = latestKnownDataVersion;
        putInt(transaction.get(), dataVersionKey, dbDataVersion);
    }

    ASSERT(dbSchemaVersion == latestKnownSchemaVersion);
    ASSERT(dbDataVersion == latestKnownDataVersion);

    if (!transaction->commit()) {
        InternalError(IDBLevelDBBackingStoreWriteError);
        return false;
    }
    return true;
}

template <typename DBOrTransaction>
WARN_UNUSED_RETURN static bool getMaxObjectStoreId(DBOrTransaction* db, int64_t databaseId, int64_t& maxObjectStoreId)
{
    const Vector<char> maxObjectStoreIdKey = DatabaseMetaDataKey::encode(databaseId, DatabaseMetaDataKey::MaxObjectStoreId);
    bool ok = getMaxObjectStoreId(db, maxObjectStoreIdKey, maxObjectStoreId);
    return ok;
}

template <typename DBOrTransaction>
WARN_UNUSED_RETURN static bool getMaxObjectStoreId(DBOrTransaction* db, const Vector<char>& maxObjectStoreIdKey, int64_t& maxObjectStoreId)
{
    maxObjectStoreId = -1;
    bool found = false;
    bool ok = getInt(db, maxObjectStoreIdKey, maxObjectStoreId, found);
    if (!ok)
        return false;
    if (!found)
        maxObjectStoreId = 0;

    ASSERT(maxObjectStoreId >= 0);
    return true;
}

IDBBackingStore::IDBBackingStore(const String& identifier, IDBFactoryBackendImpl* factory, PassOwnPtr<LevelDBDatabase> db)
    : m_identifier(identifier)
    , m_factory(factory)
    , m_db(db)
{
    m_factory->addIDBBackingStore(identifier, this);
}

IDBBackingStore::IDBBackingStore()
{
}

IDBBackingStore::~IDBBackingStore()
{
    // Only null in tests.
    if (m_factory)
        m_factory->removeIDBBackingStore(m_identifier);

    // m_db's destructor uses m_comparator. The order of destruction is important.
    m_db.clear();
    m_comparator.clear();
}

enum IDBLevelDBBackingStoreOpenResult {
    IDBLevelDBBackingStoreOpenMemory,
    IDBLevelDBBackingStoreOpenSuccess,
    IDBLevelDBBackingStoreOpenFailedDirectory,
    IDBLevelDBBackingStoreOpenFailedUnknownSchema,
    IDBLevelDBBackingStoreOpenCleanupDestroyFailed,
    IDBLevelDBBackingStoreOpenCleanupReopenFailed,
    IDBLevelDBBackingStoreOpenCleanupReopenSuccess,
    IDBLevelDBBackingStoreOpenFailedIOErrCheckingSchema,
    IDBLevelDBBackingStoreOpenMax,
};

PassRefPtr<IDBBackingStore> IDBBackingStore::open(SecurityOrigin* securityOrigin, const String& pathBaseArg, const String& fileIdentifier, IDBFactoryBackendImpl* factory)
{
    IDB_TRACE("IDBBackingStore::open");
    String pathBase = pathBaseArg;

    OwnPtr<LevelDBComparator> comparator = adoptPtr(new Comparator());
    OwnPtr<LevelDBDatabase> db;

    if (pathBase.isEmpty()) {
        HistogramSupport::histogramEnumeration("WebCore.IndexedDB.BackingStore.OpenStatus", IDBLevelDBBackingStoreOpenMemory, IDBLevelDBBackingStoreOpenMax);
        db = LevelDBDatabase::openInMemory(comparator.get());
    } else {
        if (!makeAllDirectories(pathBase)) {
            LOG_ERROR("Unable to create IndexedDB database path %s", pathBase.utf8().data());
            HistogramSupport::histogramEnumeration("WebCore.IndexedDB.BackingStore.OpenStatus", IDBLevelDBBackingStoreOpenFailedDirectory, IDBLevelDBBackingStoreOpenMax);
            return PassRefPtr<IDBBackingStore>();
        }

        String path = pathByAppendingComponent(pathBase, securityOrigin->databaseIdentifier() + ".indexeddb.leveldb");

        db = LevelDBDatabase::open(path, comparator.get());
        if (db) {
            bool known = false;
            bool ok = isSchemaKnown(db.get(), known);
            if (!ok) {
                HistogramSupport::histogramEnumeration("WebCore.IndexedDB.BackingStore.OpenStatus", IDBLevelDBBackingStoreOpenFailedIOErrCheckingSchema, IDBLevelDBBackingStoreOpenMax);
                return PassRefPtr<IDBBackingStore>();
            }
            if (!known) {
                LOG_ERROR("IndexedDB backing store had unknown schema, treating it as failure to open");
                HistogramSupport::histogramEnumeration("WebCore.IndexedDB.BackingStore.OpenStatus", IDBLevelDBBackingStoreOpenFailedUnknownSchema, IDBLevelDBBackingStoreOpenMax);
                db.clear();
            }
        }

        if (db)
            HistogramSupport::histogramEnumeration("WebCore.IndexedDB.BackingStore.OpenStatus", IDBLevelDBBackingStoreOpenSuccess, IDBLevelDBBackingStoreOpenMax);
        else {
            LOG_ERROR("IndexedDB backing store open failed, attempting cleanup");
            bool success = LevelDBDatabase::destroy(path);
            if (!success) {
                LOG_ERROR("IndexedDB backing store cleanup failed");
                HistogramSupport::histogramEnumeration("WebCore.IndexedDB.BackingStore.OpenStatus", IDBLevelDBBackingStoreOpenCleanupDestroyFailed, IDBLevelDBBackingStoreOpenMax);
                return PassRefPtr<IDBBackingStore>();
            }

            LOG_ERROR("IndexedDB backing store cleanup succeeded, reopening");
            db = LevelDBDatabase::open(path, comparator.get());
            if (!db) {
                LOG_ERROR("IndexedDB backing store reopen after recovery failed");
                HistogramSupport::histogramEnumeration("WebCore.IndexedDB.BackingStore.OpenStatus", IDBLevelDBBackingStoreOpenCleanupReopenFailed, IDBLevelDBBackingStoreOpenMax);
                return PassRefPtr<IDBBackingStore>();
            }
            HistogramSupport::histogramEnumeration("WebCore.IndexedDB.BackingStore.OpenStatus", IDBLevelDBBackingStoreOpenCleanupReopenSuccess, IDBLevelDBBackingStoreOpenMax);
        }
    }

    if (!db)
        return PassRefPtr<IDBBackingStore>();

    // FIXME: Handle comparator name changes.

    RefPtr<IDBBackingStore> backingStore(adoptRef(new IDBBackingStore(fileIdentifier, factory, db.release())));
    backingStore->m_comparator = comparator.release();

    if (!setUpMetadata(backingStore->m_db.get(), fileIdentifier))
        return PassRefPtr<IDBBackingStore>();

    return backingStore.release();
}

Vector<String> IDBBackingStore::getDatabaseNames()
{
    Vector<String> foundNames;
    const Vector<char> startKey = DatabaseNameKey::encodeMinKeyForOrigin(m_identifier);
    const Vector<char> stopKey = DatabaseNameKey::encodeStopKeyForOrigin(m_identifier);

    ASSERT(foundNames.isEmpty());

    OwnPtr<LevelDBIterator> it = m_db->createIterator();
    for (it->seek(startKey); it->isValid() && compareKeys(it->key(), stopKey) < 0; it->next()) {
        const char* p = it->key().begin();
        const char* limit = it->key().end();

        DatabaseNameKey databaseNameKey;
        p = DatabaseNameKey::decode(p, limit, &databaseNameKey);
        ASSERT(p);

        foundNames.append(databaseNameKey.databaseName());
    }
    return foundNames;
}

bool IDBBackingStore::getIDBDatabaseMetaData(const String& name, IDBDatabaseMetadata* metadata, bool& found)
{
    const Vector<char> key = DatabaseNameKey::encode(m_identifier, name);
    found = false;

    bool ok = getInt(m_db.get(), key, metadata->id, found);
    if (!ok) {
        InternalError(IDBLevelDBBackingStoreReadErrorGetIDBDatabaseMetaData);
        return false;
    }
    if (!found)
        return true;

    ok = getString(m_db.get(), DatabaseMetaDataKey::encode(metadata->id, DatabaseMetaDataKey::UserVersion), metadata->version, found);
    if (!ok) {
        InternalError(IDBLevelDBBackingStoreReadErrorGetIDBDatabaseMetaData);
        return false;
    }
    if (!found) {
        InternalError(IDBLevelDBBackingStoreConsistencyError);
        return false;
    }

    ok = getVarInt(m_db.get(), DatabaseMetaDataKey::encode(metadata->id, DatabaseMetaDataKey::UserIntVersion), metadata->intVersion, found);
    if (!ok) {
        InternalError(IDBLevelDBBackingStoreReadErrorGetIDBDatabaseMetaData);
        return false;
    }
    if (!found) {
        InternalError(IDBLevelDBBackingStoreConsistencyError);
        return false;
    }

    if (metadata->intVersion == IDBDatabaseMetadata::DefaultIntVersion)
        metadata->intVersion = IDBDatabaseMetadata::NoIntVersion;

    ok = getMaxObjectStoreId(m_db.get(), metadata->id, metadata->maxObjectStoreId);
    if (!ok) {
        InternalError(IDBLevelDBBackingStoreReadErrorGetIDBDatabaseMetaData);
        return false;
    }

    return true;
}

WARN_UNUSED_RETURN static bool getNewDatabaseId(LevelDBDatabase* db, int64_t& newId)
{
    RefPtr<LevelDBTransaction> transaction = LevelDBTransaction::create(db);

    newId = -1;
    int64_t maxDatabaseId = -1;
    bool found = false;
    bool ok = getInt(transaction.get(), MaxDatabaseIdKey::encode(), maxDatabaseId, found);
    if (!ok) {
        InternalError(IDBLevelDBBackingStoreReadErrorGetNewDatabaseId);
        return false;
    }
    if (!found)
        maxDatabaseId = 0;

    ASSERT(maxDatabaseId >= 0);

    int64_t databaseId = maxDatabaseId + 1;
    putInt(transaction.get(), MaxDatabaseIdKey::encode(), databaseId);
    if (!transaction->commit()) {
        InternalError(IDBLevelDBBackingStoreWriteError);
        return false;
    }
    newId = databaseId;
    return true;
}

bool IDBBackingStore::createIDBDatabaseMetaData(const String& name, const String& version, int64_t intVersion, int64_t& rowId)
{
    bool ok = getNewDatabaseId(m_db.get(), rowId);
    if (!ok)
        return false;
    ASSERT(rowId >= 0);

    if (intVersion == IDBDatabaseMetadata::NoIntVersion)
        intVersion = IDBDatabaseMetadata::DefaultIntVersion;

    RefPtr<LevelDBTransaction> transaction = LevelDBTransaction::create(m_db.get());
    putInt(transaction.get(), DatabaseNameKey::encode(m_identifier, name), rowId);
    putString(transaction.get(), DatabaseMetaDataKey::encode(rowId, DatabaseMetaDataKey::UserVersion), version);
    putVarInt(transaction.get(), DatabaseMetaDataKey::encode(rowId, DatabaseMetaDataKey::UserIntVersion), intVersion);
    if (!transaction->commit()) {
        InternalError(IDBLevelDBBackingStoreWriteError);
        return false;
    }
    return true;
}

bool IDBBackingStore::updateIDBDatabaseIntVersion(IDBBackingStore::Transaction* transaction, int64_t rowId, int64_t intVersion)
{
    if (intVersion == IDBDatabaseMetadata::NoIntVersion)
        intVersion = IDBDatabaseMetadata::DefaultIntVersion;
    ASSERT_WITH_MESSAGE(intVersion >= 0, "intVersion was %lld", static_cast<long long>(intVersion));
    putVarInt(Transaction::levelDBTransactionFrom(transaction), DatabaseMetaDataKey::encode(rowId, DatabaseMetaDataKey::UserIntVersion), intVersion);
    return true;
}

bool IDBBackingStore::updateIDBDatabaseMetaData(IDBBackingStore::Transaction* transaction, int64_t rowId, const String& version)
{
    putString(Transaction::levelDBTransactionFrom(transaction), DatabaseMetaDataKey::encode(rowId, DatabaseMetaDataKey::UserVersion), version);
    return true;
}

static void deleteRange(LevelDBTransaction* transaction, const Vector<char>& begin, const Vector<char>& end)
{
    OwnPtr<LevelDBIterator> it = transaction->createIterator();
    for (it->seek(begin); it->isValid() && compareKeys(it->key(), end) < 0; it->next())
        transaction->remove(it->key());
}


bool IDBBackingStore::deleteDatabase(const String& name)
{
    IDB_TRACE("IDBBackingStore::deleteDatabase");
    OwnPtr<LevelDBWriteOnlyTransaction> transaction = LevelDBWriteOnlyTransaction::create(m_db.get());

    IDBDatabaseMetadata metadata;
    bool success = false;
    bool ok = getIDBDatabaseMetaData(name, &metadata, success);
    if (!ok)
        return false;
    if (!success)
        return true;

    const Vector<char> startKey = DatabaseMetaDataKey::encode(metadata.id, DatabaseMetaDataKey::OriginName);
    const Vector<char> stopKey = DatabaseMetaDataKey::encode(metadata.id + 1, DatabaseMetaDataKey::OriginName);
    OwnPtr<LevelDBIterator> it = m_db->createIterator();
    for (it->seek(startKey); it->isValid() && compareKeys(it->key(), stopKey) < 0; it->next())
        transaction->remove(it->key());

    const Vector<char> key = DatabaseNameKey::encode(m_identifier, name);
    transaction->remove(key);

    return transaction->commit();
}

static bool checkObjectStoreAndMetaDataType(const LevelDBIterator* it, const Vector<char>& stopKey, int64_t objectStoreId, int64_t metaDataType)
{
    if (!it->isValid() || compareKeys(it->key(), stopKey) >= 0)
        return false;

    ObjectStoreMetaDataKey metaDataKey;
    const char* p = ObjectStoreMetaDataKey::decode(it->key().begin(), it->key().end(), &metaDataKey);
    ASSERT_UNUSED(p, p);
    if (metaDataKey.objectStoreId() != objectStoreId)
        return false;
    if (metaDataKey.metaDataType() != metaDataType)
        return false;
    return true;
}

Vector<IDBObjectStoreMetadata> IDBBackingStore::getObjectStores(int64_t databaseId)
{
    IDB_TRACE("IDBBackingStore::getObjectStores");
    Vector<IDBObjectStoreMetadata> objectStores;
    const Vector<char> startKey = ObjectStoreMetaDataKey::encode(databaseId, 1, 0);
    const Vector<char> stopKey = ObjectStoreMetaDataKey::encodeMaxKey(databaseId);

    OwnPtr<LevelDBIterator> it = m_db->createIterator();
    it->seek(startKey);
    while (it->isValid() && compareKeys(it->key(), stopKey) < 0) {
        const char* p = it->key().begin();
        const char* limit = it->key().end();

        ObjectStoreMetaDataKey metaDataKey;
        p = ObjectStoreMetaDataKey::decode(p, limit, &metaDataKey);
        ASSERT(p);
        if (metaDataKey.metaDataType() != ObjectStoreMetaDataKey::Name) {
            InternalError(IDBLevelDBBackingStoreReadErrorGetObjectStores);
            // Possible stale metadata, but don't fail the load.
            it->next();
            continue;
        }

        int64_t objectStoreId = metaDataKey.objectStoreId();

        // FIXME: Do this by direct key lookup rather than iteration, to simplify.
        String objectStoreName = decodeString(it->value().begin(), it->value().end());

        it->next();
        if (!checkObjectStoreAndMetaDataType(it.get(), stopKey, objectStoreId, ObjectStoreMetaDataKey::KeyPath)) {
            InternalError(IDBLevelDBBackingStoreReadErrorGetObjectStores);
            break;
        }
        IDBKeyPath keyPath = decodeIDBKeyPath(it->value().begin(), it->value().end());

        it->next();
        if (!checkObjectStoreAndMetaDataType(it.get(), stopKey, objectStoreId, ObjectStoreMetaDataKey::AutoIncrement)) {
            InternalError(IDBLevelDBBackingStoreReadErrorGetObjectStores);
            break;
        }
        bool autoIncrement = decodeBool(it->value().begin(), it->value().end());

        it->next(); // Is evicatble.
        if (!checkObjectStoreAndMetaDataType(it.get(), stopKey, objectStoreId, ObjectStoreMetaDataKey::Evictable)) {
            InternalError(IDBLevelDBBackingStoreReadErrorGetObjectStores);
            break;
        }

        it->next(); // Last version.
        if (!checkObjectStoreAndMetaDataType(it.get(), stopKey, objectStoreId, ObjectStoreMetaDataKey::LastVersion)) {
            InternalError(IDBLevelDBBackingStoreReadErrorGetObjectStores);
            break;
        }

        it->next(); // Maximum index id allocated.
        if (!checkObjectStoreAndMetaDataType(it.get(), stopKey, objectStoreId, ObjectStoreMetaDataKey::MaxIndexId)) {
            InternalError(IDBLevelDBBackingStoreReadErrorGetObjectStores);
            break;
        }
        int64_t maxIndexId = decodeInt(it->value().begin(), it->value().end());

        it->next(); // [optional] has key path (is not null)
        if (checkObjectStoreAndMetaDataType(it.get(), stopKey, objectStoreId, ObjectStoreMetaDataKey::HasKeyPath)) {
            bool hasKeyPath = decodeBool(it->value().begin(), it->value().end());
            // This check accounts for two layers of legacy coding:
            // (1) Initially, hasKeyPath was added to distinguish null vs. string.
            // (2) Later, null vs. string vs. array was stored in the keyPath itself.
            // So this check is only relevant for string-type keyPaths.
            if (!hasKeyPath && (keyPath.type() == IDBKeyPath::StringType && !keyPath.string().isEmpty())) {
                InternalError(IDBLevelDBBackingStoreReadErrorGetObjectStores);
                break;
            }
            if (!hasKeyPath)
                keyPath = IDBKeyPath();
            it->next();
        }

        int64_t keyGeneratorCurrentNumber = -1;
        if (checkObjectStoreAndMetaDataType(it.get(), stopKey, objectStoreId, ObjectStoreMetaDataKey::KeyGeneratorCurrentNumber)) {
            keyGeneratorCurrentNumber = decodeInt(it->value().begin(), it->value().end());
            // FIXME: Return keyGeneratorCurrentNumber, cache in object store, and write lazily to backing store.
            // For now, just assert that if it was written it was valid.
            ASSERT_UNUSED(keyGeneratorCurrentNumber, keyGeneratorCurrentNumber >= KeyGeneratorInitialNumber);
            it->next();
        }

        objectStores.append(IDBObjectStoreMetadata(objectStoreName, objectStoreId, keyPath, autoIncrement, maxIndexId));
    }
    return objectStores;
}

WARN_UNUSED_RETURN static bool setMaxObjectStoreId(LevelDBTransaction* transaction, int64_t databaseId, int64_t objectStoreId)
{
    const Vector<char> maxObjectStoreIdKey = DatabaseMetaDataKey::encode(databaseId, DatabaseMetaDataKey::MaxObjectStoreId);
    int64_t maxObjectStoreId = -1;
    bool ok = getMaxObjectStoreId(transaction, maxObjectStoreIdKey, maxObjectStoreId);
    if (!ok) {
        InternalError(IDBLevelDBBackingStoreReadErrorSetMaxObjectStoreId);
        return false;
    }

    if (objectStoreId <= maxObjectStoreId) {
        InternalError(IDBLevelDBBackingStoreConsistencyError);
        return false;
    }
    putInt(transaction, maxObjectStoreIdKey, objectStoreId);
    return true;
}

bool IDBBackingStore::createObjectStore(IDBBackingStore::Transaction* transaction, int64_t databaseId, int64_t objectStoreId, const String& name, const IDBKeyPath& keyPath, bool autoIncrement)
{
    IDB_TRACE("IDBBackingStore::createObjectStore");
    LevelDBTransaction* levelDBTransaction = IDBBackingStore::Transaction::levelDBTransactionFrom(transaction);
    if (!setMaxObjectStoreId(levelDBTransaction, databaseId, objectStoreId))
        return false;

    const Vector<char> nameKey = ObjectStoreMetaDataKey::encode(databaseId, objectStoreId, ObjectStoreMetaDataKey::Name);
    const Vector<char> keyPathKey = ObjectStoreMetaDataKey::encode(databaseId, objectStoreId, ObjectStoreMetaDataKey::KeyPath);
    const Vector<char> autoIncrementKey = ObjectStoreMetaDataKey::encode(databaseId, objectStoreId, ObjectStoreMetaDataKey::AutoIncrement);
    const Vector<char> evictableKey = ObjectStoreMetaDataKey::encode(databaseId, objectStoreId, ObjectStoreMetaDataKey::Evictable);
    const Vector<char> lastVersionKey = ObjectStoreMetaDataKey::encode(databaseId, objectStoreId, ObjectStoreMetaDataKey::LastVersion);
    const Vector<char> maxIndexIdKey = ObjectStoreMetaDataKey::encode(databaseId, objectStoreId, ObjectStoreMetaDataKey::MaxIndexId);
    const Vector<char> hasKeyPathKey  = ObjectStoreMetaDataKey::encode(databaseId, objectStoreId, ObjectStoreMetaDataKey::HasKeyPath);
    const Vector<char> keyGeneratorCurrentNumberKey = ObjectStoreMetaDataKey::encode(databaseId, objectStoreId, ObjectStoreMetaDataKey::KeyGeneratorCurrentNumber);
    const Vector<char> namesKey = ObjectStoreNamesKey::encode(databaseId, name);

    putString(levelDBTransaction, nameKey, name);
    putIDBKeyPath(levelDBTransaction, keyPathKey, keyPath);
    putInt(levelDBTransaction, autoIncrementKey, autoIncrement);
    putInt(levelDBTransaction, evictableKey, false);
    putInt(levelDBTransaction, lastVersionKey, 1);
    putInt(levelDBTransaction, maxIndexIdKey, MinimumIndexId);
    putBool(levelDBTransaction, hasKeyPathKey, !keyPath.isNull());
    putInt(levelDBTransaction, keyGeneratorCurrentNumberKey, KeyGeneratorInitialNumber);
    putInt(levelDBTransaction, namesKey, objectStoreId);
    return true;
}

bool IDBBackingStore::deleteObjectStore(IDBBackingStore::Transaction* transaction, int64_t databaseId, int64_t objectStoreId)
{
    IDB_TRACE("IDBBackingStore::deleteObjectStore");
    LevelDBTransaction* levelDBTransaction = IDBBackingStore::Transaction::levelDBTransactionFrom(transaction);

    String objectStoreName;
    bool found = false;
    bool ok = getString(levelDBTransaction, ObjectStoreMetaDataKey::encode(databaseId, objectStoreId, ObjectStoreMetaDataKey::Name), objectStoreName, found);
    if (!ok) {
        InternalError(IDBLevelDBBackingStoreReadErrorDeleteObjectStore);
        return false;
    }
    if (!found) {
        InternalError(IDBLevelDBBackingStoreConsistencyError);
        return false;
    }

    deleteRange(levelDBTransaction, ObjectStoreMetaDataKey::encode(databaseId, objectStoreId, 0), ObjectStoreMetaDataKey::encodeMaxKey(databaseId, objectStoreId));

    levelDBTransaction->remove(ObjectStoreNamesKey::encode(databaseId, objectStoreName));

    deleteRange(levelDBTransaction, IndexFreeListKey::encode(databaseId, objectStoreId, 0), IndexFreeListKey::encodeMaxKey(databaseId, objectStoreId));
    deleteRange(levelDBTransaction, IndexMetaDataKey::encode(databaseId, objectStoreId, 0, 0), IndexMetaDataKey::encodeMaxKey(databaseId, objectStoreId));

    clearObjectStore(transaction, databaseId, objectStoreId);
    return true;
}

bool IDBBackingStore::getRecord(IDBBackingStore::Transaction* transaction, int64_t databaseId, int64_t objectStoreId, const IDBKey& key, String& record)
{
    IDB_TRACE("IDBBackingStore::getRecord");
    LevelDBTransaction* levelDBTransaction = IDBBackingStore::Transaction::levelDBTransactionFrom(transaction);

    const Vector<char> leveldbKey = ObjectStoreDataKey::encode(databaseId, objectStoreId, key);
    Vector<char> data;

    bool found = false;
    bool ok = levelDBTransaction->safeGet(leveldbKey, data, found);
    if (!ok) {
        record = String();
        InternalError(IDBLevelDBBackingStoreReadErrorGetRecord);
        return false;
    }

    int64_t version;
    const char* p = decodeVarInt(data.begin(), data.end(), version);
    if (!p) {
        InternalError(IDBLevelDBBackingStoreReadErrorGetRecord);
        record = String();
        return false;
    }

    record = decodeString(p, data.end());
    return true;
}

WARN_UNUSED_RETURN static bool getNewVersionNumber(LevelDBTransaction* transaction, int64_t databaseId, int64_t objectStoreId, int64_t& newVersionNumber)
{
    const Vector<char> lastVersionKey = ObjectStoreMetaDataKey::encode(databaseId, objectStoreId, ObjectStoreMetaDataKey::LastVersion);

    newVersionNumber = -1;
    int64_t lastVersion = -1;
    bool found = false;
    bool ok = getInt(transaction, lastVersionKey, lastVersion, found);
    if (!ok) {
        InternalError(IDBLevelDBBackingStoreReadErrorGetNewVersionNumber);
        return false;
    }
    if (!found)
        lastVersion = 0;

    ASSERT(lastVersion >= 0);

    int64_t version = lastVersion + 1;
    putInt(transaction, lastVersionKey, version);

    ASSERT(version > lastVersion); // FIXME: Think about how we want to handle the overflow scenario.

    newVersionNumber = version;
    return true;
}

bool IDBBackingStore::putRecord(IDBBackingStore::Transaction* transaction, int64_t databaseId, int64_t objectStoreId, const IDBKey& key, const String& value, RecordIdentifier* recordIdentifier)
{
    IDB_TRACE("IDBBackingStore::putRecord");
    ASSERT(key.isValid());
    LevelDBTransaction* levelDBTransaction = IDBBackingStore::Transaction::levelDBTransactionFrom(transaction);
    int64_t version = -1;
    bool ok = getNewVersionNumber(levelDBTransaction, databaseId, objectStoreId, version);
    if (!ok)
        return false;
    ASSERT(version >= 0);
    const Vector<char> objectStoredataKey = ObjectStoreDataKey::encode(databaseId, objectStoreId, key);

    Vector<char> v;
    v.append(encodeVarInt(version));
    v.append(encodeString(value));

    levelDBTransaction->put(objectStoredataKey, v);

    const Vector<char> existsEntryKey = ExistsEntryKey::encode(databaseId, objectStoreId, key);
    levelDBTransaction->put(existsEntryKey, encodeInt(version));

    recordIdentifier->reset(encodeIDBKey(key), version);
    return true;
}

void IDBBackingStore::clearObjectStore(IDBBackingStore::Transaction* transaction, int64_t databaseId, int64_t objectStoreId)
{
    IDB_TRACE("IDBBackingStore::clearObjectStore");
    LevelDBTransaction* levelDBTransaction = IDBBackingStore::Transaction::levelDBTransactionFrom(transaction);
    const Vector<char> startKey = KeyPrefix(databaseId, objectStoreId, 0).encode();
    const Vector<char> stopKey = KeyPrefix(databaseId, objectStoreId + 1, 0).encode();

    deleteRange(levelDBTransaction, startKey, stopKey);
}

void IDBBackingStore::deleteRecord(IDBBackingStore::Transaction* transaction, int64_t databaseId, int64_t objectStoreId, const RecordIdentifier& recordIdentifier)
{
    IDB_TRACE("IDBBackingStore::deleteRecord");
    LevelDBTransaction* levelDBTransaction = IDBBackingStore::Transaction::levelDBTransactionFrom(transaction);

    const Vector<char> objectStoreDataKey = ObjectStoreDataKey::encode(databaseId, objectStoreId, recordIdentifier.primaryKey());
    levelDBTransaction->remove(objectStoreDataKey);

    const Vector<char> existsEntryKey = ExistsEntryKey::encode(databaseId, objectStoreId, recordIdentifier.primaryKey());
    levelDBTransaction->remove(existsEntryKey);
}


bool IDBBackingStore::getKeyGeneratorCurrentNumber(IDBBackingStore::Transaction* transaction, int64_t databaseId, int64_t objectStoreId, int64_t& keyGeneratorCurrentNumber)
{
    LevelDBTransaction* levelDBTransaction = IDBBackingStore::Transaction::levelDBTransactionFrom(transaction);

    const Vector<char> keyGeneratorCurrentNumberKey = ObjectStoreMetaDataKey::encode(databaseId, objectStoreId, ObjectStoreMetaDataKey::KeyGeneratorCurrentNumber);

    keyGeneratorCurrentNumber = -1;
    Vector<char> data;

    bool found = false;
    bool ok = levelDBTransaction->safeGet(keyGeneratorCurrentNumberKey, data, found);
    if (!ok) {
        InternalError(IDBLevelDBBackingStoreReadErrorGetKeyGeneratorCurrentNumber);
        return false;
    }
    if (found)
        keyGeneratorCurrentNumber = decodeInt(data.begin(), data.end());
    else {
        // Previously, the key generator state was not stored explicitly but derived from the
        // maximum numeric key present in existing data. This violates the spec as the data may
        // be cleared but the key generator state must be preserved.
        const Vector<char> startKey = ObjectStoreDataKey::encode(databaseId, objectStoreId, minIDBKey());
        const Vector<char> stopKey = ObjectStoreDataKey::encode(databaseId, objectStoreId, maxIDBKey());

        OwnPtr<LevelDBIterator> it = levelDBTransaction->createIterator();
        int64_t maxNumericKey = 0;

        for (it->seek(startKey); it->isValid() && compareKeys(it->key(), stopKey) < 0; it->next()) {
            const char* p = it->key().begin();
            const char* limit = it->key().end();

            ObjectStoreDataKey dataKey;
            p = ObjectStoreDataKey::decode(p, limit, &dataKey);
            ASSERT(p);

            if (dataKey.userKey()->type() == IDBKey::NumberType) {
                int64_t n = static_cast<int64_t>(dataKey.userKey()->number());
                if (n > maxNumericKey)
                    maxNumericKey = n;
            }
        }

        keyGeneratorCurrentNumber = maxNumericKey + 1;
    }

    return keyGeneratorCurrentNumber;
}

bool IDBBackingStore::maybeUpdateKeyGeneratorCurrentNumber(IDBBackingStore::Transaction* transaction, int64_t databaseId, int64_t objectStoreId, int64_t newNumber, bool checkCurrent)
{
    LevelDBTransaction* levelDBTransaction = IDBBackingStore::Transaction::levelDBTransactionFrom(transaction);

    if (checkCurrent) {
        int64_t currentNumber;
        bool ok = getKeyGeneratorCurrentNumber(transaction, databaseId, objectStoreId, currentNumber);
        if (!ok)
            return false;
        if (newNumber <= currentNumber)
            return true;
    }

    const Vector<char> keyGeneratorCurrentNumberKey = ObjectStoreMetaDataKey::encode(databaseId, objectStoreId, ObjectStoreMetaDataKey::KeyGeneratorCurrentNumber);
    putInt(levelDBTransaction, keyGeneratorCurrentNumberKey, newNumber);
    return true;
}

bool IDBBackingStore::keyExistsInObjectStore(IDBBackingStore::Transaction* transaction, int64_t databaseId, int64_t objectStoreId, const IDBKey& key, RecordIdentifier* foundRecordIdentifier, bool& found)
{
    IDB_TRACE("IDBBackingStore::keyExistsInObjectStore");
    found = false;
    LevelDBTransaction* levelDBTransaction = IDBBackingStore::Transaction::levelDBTransactionFrom(transaction);
    const Vector<char> leveldbKey = ObjectStoreDataKey::encode(databaseId, objectStoreId, key);
    Vector<char> data;

    bool ok = levelDBTransaction->safeGet(leveldbKey, data, found);
    if (!ok) {
        InternalError(IDBLevelDBBackingStoreReadErrorKeyExistsInObjectStore);
        return false;
    }
    if (!found)
        return true;

    int64_t version;
    if (!decodeVarInt(data.begin(), data.end(), version))
        return false;

    foundRecordIdentifier->reset(encodeIDBKey(key), version);
    return true;
}

static bool checkIndexAndMetaDataKey(const LevelDBIterator* it, const Vector<char>& stopKey, int64_t indexId, unsigned char metaDataType)
{
    if (!it->isValid() || compareKeys(it->key(), stopKey) >= 0)
        return false;

    IndexMetaDataKey metaDataKey;
    const char* p = IndexMetaDataKey::decode(it->key().begin(), it->key().end(), &metaDataKey);
    ASSERT_UNUSED(p, p);
    if (metaDataKey.indexId() != indexId)
        return false;
    if (metaDataKey.metaDataType() != metaDataType)
        return false;
    return true;
}


Vector<IDBIndexMetadata> IDBBackingStore::getIndexes(int64_t databaseId, int64_t objectStoreId)
{
    IDB_TRACE("IDBBackingStore::getIndexes");
    Vector<IDBIndexMetadata> indexes;
    const Vector<char> startKey = IndexMetaDataKey::encode(databaseId, objectStoreId, 0, 0);
    const Vector<char> stopKey = IndexMetaDataKey::encode(databaseId, objectStoreId + 1, 0, 0);

    ASSERT(indexes.isEmpty());

    OwnPtr<LevelDBIterator> it = m_db->createIterator();
    it->seek(startKey);
    while (it->isValid() && compareKeys(it->key(), stopKey) < 0) {
        const char* p = it->key().begin();
        const char* limit = it->key().end();

        IndexMetaDataKey metaDataKey;
        p = IndexMetaDataKey::decode(p, limit, &metaDataKey);
        ASSERT(p);
        if (metaDataKey.metaDataType() != IndexMetaDataKey::Name) {
            InternalError(IDBLevelDBBackingStoreReadErrorGetIndexes);
            // Possible stale metadata due to http://webkit.org/b/85557 but don't fail the load.
            it->next();
            continue;
        }

        // FIXME: Do this by direct key lookup rather than iteration, to simplify.
        int64_t indexId = metaDataKey.indexId();
        String indexName = decodeString(it->value().begin(), it->value().end());

        it->next(); // unique flag
        if (!checkIndexAndMetaDataKey(it.get(), stopKey, indexId, IndexMetaDataKey::Unique)) {
            InternalError(IDBLevelDBBackingStoreReadErrorGetIndexes);
            break;
        }
        bool indexUnique = decodeBool(it->value().begin(), it->value().end());

        it->next(); // keyPath
        if (!checkIndexAndMetaDataKey(it.get(), stopKey, indexId, IndexMetaDataKey::KeyPath)) {
            InternalError(IDBLevelDBBackingStoreReadErrorGetIndexes);
            break;
        }
        IDBKeyPath keyPath = decodeIDBKeyPath(it->value().begin(), it->value().end());

        it->next(); // [optional] multiEntry flag
        bool indexMultiEntry = false;
        if (checkIndexAndMetaDataKey(it.get(), stopKey, indexId, IndexMetaDataKey::MultiEntry)) {
            indexMultiEntry = decodeBool(it->value().begin(), it->value().end());
            it->next();
        }

        indexes.append(IDBIndexMetadata(indexName, indexId, keyPath, indexUnique, indexMultiEntry));
    }
    return indexes;
}

WARN_UNUSED_RETURN static bool setMaxIndexId(LevelDBTransaction* transaction, int64_t databaseId, int64_t objectStoreId, int64_t indexId)
{
    int64_t maxIndexId = -1;
    const Vector<char> maxIndexIdKey = ObjectStoreMetaDataKey::encode(databaseId, objectStoreId, ObjectStoreMetaDataKey::MaxIndexId);
    bool found = false;
    bool ok = getInt(transaction, maxIndexIdKey, maxIndexId, found);
    if (!ok) {
        InternalError(IDBLevelDBBackingStoreReadErrorSetMaxIndexId);
        return false;
    }
    if (!found)
        maxIndexId = MinimumIndexId;

    if (indexId <= maxIndexId) {
        InternalError(IDBLevelDBBackingStoreConsistencyError);
        return false;
    }

    putInt(transaction, maxIndexIdKey, indexId);
    return true;
}

bool IDBBackingStore::createIndex(IDBBackingStore::Transaction* transaction, int64_t databaseId, int64_t objectStoreId, int64_t indexId, const String& name, const IDBKeyPath& keyPath, bool isUnique, bool isMultiEntry)
{
    IDB_TRACE("IDBBackingStore::createIndex");
    LevelDBTransaction* levelDBTransaction = IDBBackingStore::Transaction::levelDBTransactionFrom(transaction);
    if (!setMaxIndexId(levelDBTransaction, databaseId, objectStoreId, indexId))
        return false;

    const Vector<char> nameKey = IndexMetaDataKey::encode(databaseId, objectStoreId, indexId, IndexMetaDataKey::Name);
    const Vector<char> uniqueKey = IndexMetaDataKey::encode(databaseId, objectStoreId, indexId, IndexMetaDataKey::Unique);
    const Vector<char> keyPathKey = IndexMetaDataKey::encode(databaseId, objectStoreId, indexId, IndexMetaDataKey::KeyPath);
    const Vector<char> multiEntryKey = IndexMetaDataKey::encode(databaseId, objectStoreId, indexId, IndexMetaDataKey::MultiEntry);

    putString(levelDBTransaction, nameKey, name);
    putBool(levelDBTransaction, uniqueKey, isUnique);
    putIDBKeyPath(levelDBTransaction, keyPathKey, keyPath);
    putBool(levelDBTransaction, multiEntryKey, isMultiEntry);
    return true;
}

void IDBBackingStore::deleteIndex(IDBBackingStore::Transaction* transaction, int64_t databaseId, int64_t objectStoreId, int64_t indexId)
{
    IDB_TRACE("IDBBackingStore::deleteIndex");
    LevelDBTransaction* levelDBTransaction = IDBBackingStore::Transaction::levelDBTransactionFrom(transaction);

    const Vector<char> indexMetaDataStart = IndexMetaDataKey::encode(databaseId, objectStoreId, indexId, 0);
    const Vector<char> indexMetaDataEnd = IndexMetaDataKey::encodeMaxKey(databaseId, objectStoreId, indexId);
    deleteRange(levelDBTransaction, indexMetaDataStart, indexMetaDataEnd);

    const Vector<char> indexDataStart = IndexDataKey::encodeMinKey(databaseId, objectStoreId, indexId);
    const Vector<char> indexDataEnd = IndexDataKey::encodeMaxKey(databaseId, objectStoreId, indexId);
    deleteRange(levelDBTransaction, indexDataStart, indexDataEnd);
}

void IDBBackingStore::putIndexDataForRecord(IDBBackingStore::Transaction* transaction, int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKey& key, const RecordIdentifier& recordIdentifier)
{
    IDB_TRACE("IDBBackingStore::putIndexDataForRecord");
    ASSERT(key.isValid());
    ASSERT(indexId >= MinimumIndexId);

    LevelDBTransaction* levelDBTransaction = IDBBackingStore::Transaction::levelDBTransactionFrom(transaction);
    const Vector<char> indexDataKey = IndexDataKey::encode(databaseId, objectStoreId, indexId, encodeIDBKey(key), recordIdentifier.primaryKey());

    Vector<char> data;
    data.append(encodeVarInt(recordIdentifier.version()));
    data.append(recordIdentifier.primaryKey());

    levelDBTransaction->put(indexDataKey, data);
}

static bool findGreatestKeyLessThanOrEqual(LevelDBTransaction* transaction, const Vector<char>& target, Vector<char>& foundKey)
{
    OwnPtr<LevelDBIterator> it = transaction->createIterator();
    it->seek(target);

    if (!it->isValid()) {
        it->seekToLast();
        if (!it->isValid())
            return false;
    }

    while (compareIndexKeys(it->key(), target) > 0) {
        it->prev();
        if (!it->isValid())
            return false;
    }

    do {
        foundKey.clear();
        foundKey.append(it->key().begin(), it->key().end() - it->key().begin());

        // There can be several index keys that compare equal. We want the last one.
        it->next();
    } while (it->isValid() && !compareIndexKeys(it->key(), target));

    return true;
}

static bool versionExists(LevelDBTransaction* transaction, int64_t databaseId, int64_t objectStoreId, int64_t version, const Vector<char>& encodedPrimaryKey, bool& exists)
{
    const Vector<char> key = ExistsEntryKey::encode(databaseId, objectStoreId, encodedPrimaryKey);
    Vector<char> data;

    bool ok = transaction->safeGet(key, data, exists);
    if (!ok) {
        InternalError(IDBLevelDBBackingStoreReadErrorVersionExists);
        return false;
    }
    if (!exists)
        return true;

    exists = (decodeInt(data.begin(), data.end()) == version);
    return true;
}

bool IDBBackingStore::findKeyInIndex(IDBBackingStore::Transaction* transaction, int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKey& key, Vector<char>& foundEncodedPrimaryKey, bool& found)
{
    IDB_TRACE("IDBBackingStore::findKeyInIndex");
    ASSERT(foundEncodedPrimaryKey.isEmpty());
    found = false;

    LevelDBTransaction* levelDBTransaction = IDBBackingStore::Transaction::levelDBTransactionFrom(transaction);
    const Vector<char> leveldbKey = IndexDataKey::encode(databaseId, objectStoreId, indexId, key);
    OwnPtr<LevelDBIterator> it = levelDBTransaction->createIterator();
    it->seek(leveldbKey);

    for (;;) {
        if (!it->isValid())
            return true;
        if (compareIndexKeys(it->key(), leveldbKey) > 0)
            return true;

        int64_t version;
        const char* p = decodeVarInt(it->value().begin(), it->value().end(), version);
        if (!p) {
            InternalError(IDBLevelDBBackingStoreReadErrorFindKeyInIndex);
            return false;
        }
        foundEncodedPrimaryKey.append(p, it->value().end() - p);

        bool exists = false;
        bool ok = versionExists(levelDBTransaction, databaseId, objectStoreId, version, foundEncodedPrimaryKey, exists);
        if (!ok)
            return false;
        if (!exists) {
            // Delete stale index data entry and continue.
            levelDBTransaction->remove(it->key());
            it->next();
            continue;
        }
        found = true;
        return true;
    }
}

bool IDBBackingStore::getPrimaryKeyViaIndex(IDBBackingStore::Transaction* transaction, int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKey& key, RefPtr<IDBKey>& primaryKey)
{
    IDB_TRACE("IDBBackingStore::getPrimaryKeyViaIndex");

    bool found = false;
    Vector<char> foundEncodedPrimaryKey;
    bool ok = findKeyInIndex(transaction, databaseId, objectStoreId, indexId, key, foundEncodedPrimaryKey, found);
    if (!ok) {
        InternalError(IDBLevelDBBackingStoreReadErrorGetPrimaryKeyViaIndex);
        return false;
    }
    if (found) {
        decodeIDBKey(foundEncodedPrimaryKey.begin(), foundEncodedPrimaryKey.end(), primaryKey);
        return true;
    }

    return true;
}

bool IDBBackingStore::keyExistsInIndex(IDBBackingStore::Transaction* transaction, int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKey& indexKey, RefPtr<IDBKey>& foundPrimaryKey, bool& exists)
{
    IDB_TRACE("IDBBackingStore::keyExistsInIndex");

    exists = false;
    Vector<char> foundEncodedPrimaryKey;
    bool ok = findKeyInIndex(transaction, databaseId, objectStoreId, indexId, indexKey, foundEncodedPrimaryKey, exists);
    if (!ok) {
        InternalError(IDBLevelDBBackingStoreReadErrorKeyExistsInIndex);
        return false;
    }
    if (!exists)
        return true;

    decodeIDBKey(foundEncodedPrimaryKey.begin(), foundEncodedPrimaryKey.end(), foundPrimaryKey);
    return true;
}

IDBBackingStore::Cursor::Cursor(const IDBBackingStore::Cursor* other)
    : m_transaction(other->m_transaction)
    , m_cursorOptions(other->m_cursorOptions)
    , m_currentKey(other->m_currentKey)
{
    if (other->m_iterator) {
        m_iterator = m_transaction->createIterator();

        if (other->m_iterator->isValid()) {
            m_iterator->seek(other->m_iterator->key());
            ASSERT(m_iterator->isValid());
        }
    }
}

bool IDBBackingStore::Cursor::firstSeek()
{
    m_iterator = m_transaction->createIterator();
    if (m_cursorOptions.forward)
        m_iterator->seek(m_cursorOptions.lowKey);
    else
        m_iterator->seek(m_cursorOptions.highKey);

    return continueFunction(0, Ready);
}

bool IDBBackingStore::Cursor::advance(unsigned long count)
{
    while (count--) {
        if (!continueFunction())
            return false;
    }
    return true;
}

bool IDBBackingStore::Cursor::continueFunction(const IDBKey* key, IteratorState nextState)
{
    RefPtr<IDBKey> previousKey = m_currentKey;

    // When iterating with PREV_NO_DUPLICATE, spec requires that the
    // value we yield for each key is the first duplicate in forwards
    // order.
    RefPtr<IDBKey> lastDuplicateKey;

    bool forward = m_cursorOptions.forward;

    for (;;) {
        if (nextState == Seek) {
            if (forward)
                m_iterator->next();
            else
                m_iterator->prev();
        } else
            nextState = Seek; // for subsequent iterations

        if (!m_iterator->isValid()) {
            if (!forward && lastDuplicateKey.get()) {
                // We need to walk forward because we hit the end of
                // the data.
                forward = true;
                continue;
            }

            return false;
        }

        if (isPastBounds()) {
            if (!forward && lastDuplicateKey.get()) {
                // We need to walk forward because now we're beyond the
                // bounds defined by the cursor.
                forward = true;
                continue;
            }

            return false;
        }

        if (!haveEnteredRange())
            continue;

        // The row may not load because there's a stale entry in the
        // index. This is not fatal.
        if (!loadCurrentRow())
            continue;

        if (key) {
            if (forward) {
                if (m_currentKey->isLessThan(key))
                    continue;
            } else {
                if (key->isLessThan(m_currentKey.get()))
                    continue;
            }
        }

        if (m_cursorOptions.unique) {

            if (m_currentKey->isEqual(previousKey.get())) {
                // We should never be able to walk forward all the way
                // to the previous key.
                ASSERT(!lastDuplicateKey.get());
                continue;
            }

            if (!forward) {
                if (!lastDuplicateKey.get()) {
                    lastDuplicateKey = m_currentKey;
                    continue;
                }

                // We need to walk forward because we hit the boundary
                // between key ranges.
                if (!lastDuplicateKey->isEqual(m_currentKey.get())) {
                    forward = true;
                    continue;
                }

                continue;
            }
        }
        break;
    }

    ASSERT(!lastDuplicateKey.get() || (forward && lastDuplicateKey->isEqual(m_currentKey.get())));
    return true;
}

bool IDBBackingStore::Cursor::haveEnteredRange() const
{
    if (m_cursorOptions.forward) {
        if (m_cursorOptions.lowOpen)
            return compareIndexKeys(m_iterator->key(), m_cursorOptions.lowKey) > 0;

        return compareIndexKeys(m_iterator->key(), m_cursorOptions.lowKey) >= 0;
    }
    if (m_cursorOptions.highOpen)
        return compareIndexKeys(m_iterator->key(), m_cursorOptions.highKey) < 0;

    return compareIndexKeys(m_iterator->key(), m_cursorOptions.highKey) <= 0;
}

bool IDBBackingStore::Cursor::isPastBounds() const
{
    if (m_cursorOptions.forward) {
        if (m_cursorOptions.highOpen)
            return compareIndexKeys(m_iterator->key(), m_cursorOptions.highKey) >= 0;
        return compareIndexKeys(m_iterator->key(), m_cursorOptions.highKey) > 0;
    }

    if (m_cursorOptions.lowOpen)
        return compareIndexKeys(m_iterator->key(), m_cursorOptions.lowKey) <= 0;
    return compareIndexKeys(m_iterator->key(), m_cursorOptions.lowKey) < 0;
}

class ObjectStoreKeyCursorImpl : public IDBBackingStore::Cursor {
public:
    static PassRefPtr<ObjectStoreKeyCursorImpl> create(LevelDBTransaction* transaction, const IDBBackingStore::Cursor::CursorOptions& cursorOptions)
    {
        return adoptRef(new ObjectStoreKeyCursorImpl(transaction, cursorOptions));
    }

    virtual PassRefPtr<IDBBackingStore::Cursor> clone()
    {
        return adoptRef(new ObjectStoreKeyCursorImpl(this));
    }

    // IDBBackingStore::Cursor
    virtual String value() const { ASSERT_NOT_REACHED(); return String(); }
    virtual bool loadCurrentRow();

private:
    ObjectStoreKeyCursorImpl(LevelDBTransaction* transaction, const IDBBackingStore::Cursor::CursorOptions& cursorOptions)
        : IDBBackingStore::Cursor(transaction, cursorOptions)
    {
    }

    ObjectStoreKeyCursorImpl(const ObjectStoreKeyCursorImpl* other)
        : IDBBackingStore::Cursor(other)
    {
    }

};

bool ObjectStoreKeyCursorImpl::loadCurrentRow()
{
    const char* keyPosition = m_iterator->key().begin();
    const char* keyLimit = m_iterator->key().end();

    ObjectStoreDataKey objectStoreDataKey;
    keyPosition = ObjectStoreDataKey::decode(keyPosition, keyLimit, &objectStoreDataKey);
    if (!keyPosition) {
        InternalError(IDBLevelDBBackingStoreReadErrorLoadCurrentRow);
        return false;
    }

    m_currentKey = objectStoreDataKey.userKey();

    int64_t version;
    const char* valuePosition = decodeVarInt(m_iterator->value().begin(), m_iterator->value().end(), version);
    if (!valuePosition) {
        InternalError(IDBLevelDBBackingStoreReadErrorLoadCurrentRow);
        return false;
    }

    // FIXME: This re-encodes what was just decoded; try and optimize.
    m_recordIdentifier.reset(encodeIDBKey(*m_currentKey), version);

    return true;
}

class ObjectStoreCursorImpl : public IDBBackingStore::Cursor {
public:
    static PassRefPtr<ObjectStoreCursorImpl> create(LevelDBTransaction* transaction, const IDBBackingStore::Cursor::CursorOptions& cursorOptions)
    {
        return adoptRef(new ObjectStoreCursorImpl(transaction, cursorOptions));
    }

    virtual PassRefPtr<IDBBackingStore::Cursor> clone()
    {
        return adoptRef(new ObjectStoreCursorImpl(this));
    }

    // IDBBackingStore::Cursor
    virtual String value() const { return m_currentValue; }
    virtual bool loadCurrentRow();

private:
    ObjectStoreCursorImpl(LevelDBTransaction* transaction, const IDBBackingStore::Cursor::CursorOptions& cursorOptions)
        : IDBBackingStore::Cursor(transaction, cursorOptions)
    {
    }

    ObjectStoreCursorImpl(const ObjectStoreCursorImpl* other)
        : IDBBackingStore::Cursor(other)
        , m_currentValue(other->m_currentValue)
    {
    }

    String m_currentValue;
};

bool ObjectStoreCursorImpl::loadCurrentRow()
{
    const char* keyPosition = m_iterator->key().begin();
    const char* keyLimit = m_iterator->key().end();

    ObjectStoreDataKey objectStoreDataKey;
    keyPosition = ObjectStoreDataKey::decode(keyPosition, keyLimit, &objectStoreDataKey);
    if (!keyPosition) {
        InternalError(IDBLevelDBBackingStoreReadErrorLoadCurrentRow);
        return false;
    }

    m_currentKey = objectStoreDataKey.userKey();

    int64_t version;
    const char* valuePosition = decodeVarInt(m_iterator->value().begin(), m_iterator->value().end(), version);
    if (!valuePosition) {
        InternalError(IDBLevelDBBackingStoreReadErrorLoadCurrentRow);
        return false;
    }

    // FIXME: This re-encodes what was just decoded; try and optimize.
    m_recordIdentifier.reset(encodeIDBKey(*m_currentKey), version);

    m_currentValue = decodeString(valuePosition, m_iterator->value().end());

    return true;
}

class IndexKeyCursorImpl : public IDBBackingStore::Cursor {
public:
    static PassRefPtr<IndexKeyCursorImpl> create(LevelDBTransaction* transaction, const IDBBackingStore::Cursor::CursorOptions& cursorOptions)
    {
        return adoptRef(new IndexKeyCursorImpl(transaction, cursorOptions));
    }

    virtual PassRefPtr<IDBBackingStore::Cursor> clone()
    {
        return adoptRef(new IndexKeyCursorImpl(this));
    }

    // IDBBackingStore::Cursor
    virtual String value() const { ASSERT_NOT_REACHED(); return String(); }
    virtual PassRefPtr<IDBKey> primaryKey() const { return m_primaryKey; }
    virtual const IDBBackingStore::RecordIdentifier& recordIdentifier() const { ASSERT_NOT_REACHED(); return m_recordIdentifier; }
    virtual bool loadCurrentRow();

private:
    IndexKeyCursorImpl(LevelDBTransaction* transaction, const IDBBackingStore::Cursor::CursorOptions& cursorOptions)
        : IDBBackingStore::Cursor(transaction, cursorOptions)
    {
    }

    IndexKeyCursorImpl(const IndexKeyCursorImpl* other)
        : IDBBackingStore::Cursor(other)
        , m_primaryKey(other->m_primaryKey)
    {
    }

    RefPtr<IDBKey> m_primaryKey;
};

bool IndexKeyCursorImpl::loadCurrentRow()
{
    const char* keyPosition = m_iterator->key().begin();
    const char* keyLimit = m_iterator->key().end();

    IndexDataKey indexDataKey;
    keyPosition = IndexDataKey::decode(keyPosition, keyLimit, &indexDataKey);

    m_currentKey = indexDataKey.userKey();

    int64_t indexDataVersion;
    const char* valuePosition = decodeVarInt(m_iterator->value().begin(), m_iterator->value().end(), indexDataVersion);
    if (!valuePosition) {
        InternalError(IDBLevelDBBackingStoreReadErrorLoadCurrentRow);
        return false;
    }

    valuePosition = decodeIDBKey(valuePosition, m_iterator->value().end(), m_primaryKey);
    if (!valuePosition) {
        InternalError(IDBLevelDBBackingStoreReadErrorLoadCurrentRow);
        return false;
    }

    Vector<char> primaryLevelDBKey = ObjectStoreDataKey::encode(indexDataKey.databaseId(), indexDataKey.objectStoreId(), *m_primaryKey);

    Vector<char> result;
    if (!m_transaction->get(primaryLevelDBKey, result)) {
        m_transaction->remove(m_iterator->key());
        return false;
    }

    int64_t objectStoreDataVersion;
    const char* t = decodeVarInt(result.begin(), result.end(), objectStoreDataVersion);
    if (!t) {
        InternalError(IDBLevelDBBackingStoreReadErrorLoadCurrentRow);
        return false;
    }

    if (objectStoreDataVersion != indexDataVersion) {
        m_transaction->remove(m_iterator->key());
        return false;
    }

    return true;
}

class IndexCursorImpl : public IDBBackingStore::Cursor {
public:
    static PassRefPtr<IndexCursorImpl> create(LevelDBTransaction* transaction, const IDBBackingStore::Cursor::CursorOptions& cursorOptions)
    {
        return adoptRef(new IndexCursorImpl(transaction, cursorOptions));
    }

    virtual PassRefPtr<IDBBackingStore::Cursor> clone()
    {
        return adoptRef(new IndexCursorImpl(this));
    }

    // IDBBackingStore::Cursor
    virtual String value() const { return m_value; }
    virtual PassRefPtr<IDBKey> primaryKey() const { return m_primaryKey; }
    virtual const IDBBackingStore::RecordIdentifier& recordIdentifier() const { ASSERT_NOT_REACHED(); return m_recordIdentifier; }
    bool loadCurrentRow();

private:
    IndexCursorImpl(LevelDBTransaction* transaction, const IDBBackingStore::Cursor::CursorOptions& cursorOptions)
        : IDBBackingStore::Cursor(transaction, cursorOptions)
    {
    }

    IndexCursorImpl(const IndexCursorImpl* other)
        : IDBBackingStore::Cursor(other)
        , m_primaryKey(other->m_primaryKey)
        , m_value(other->m_value)
        , m_primaryLevelDBKey(other->m_primaryLevelDBKey)
    {
    }

    RefPtr<IDBKey> m_primaryKey;
    String m_value;
    Vector<char> m_primaryLevelDBKey;
};

bool IndexCursorImpl::loadCurrentRow()
{
    const char* keyPosition = m_iterator->key().begin();
    const char* keyLimit = m_iterator->key().end();

    IndexDataKey indexDataKey;
    keyPosition = IndexDataKey::decode(keyPosition, keyLimit, &indexDataKey);

    m_currentKey = indexDataKey.userKey();

    const char* valuePosition = m_iterator->value().begin();
    const char* valueLimit = m_iterator->value().end();

    int64_t indexDataVersion;
    valuePosition = decodeVarInt(valuePosition, valueLimit, indexDataVersion);
    if (!valuePosition) {
        InternalError(IDBLevelDBBackingStoreReadErrorLoadCurrentRow);
        return false;
    }
    valuePosition = decodeIDBKey(valuePosition, valueLimit, m_primaryKey);
    if (!valuePosition) {
        InternalError(IDBLevelDBBackingStoreReadErrorLoadCurrentRow);
        return false;
    }

    m_primaryLevelDBKey = ObjectStoreDataKey::encode(indexDataKey.databaseId(), indexDataKey.objectStoreId(), *m_primaryKey);

    Vector<char> result;
    if (!m_transaction->get(m_primaryLevelDBKey, result)) {
        m_transaction->remove(m_iterator->key());
        return false;
    }

    int64_t objectStoreDataVersion;
    const char* t = decodeVarInt(result.begin(), result.end(), objectStoreDataVersion);
    if (!t) {
        InternalError(IDBLevelDBBackingStoreReadErrorLoadCurrentRow);
        return false;
    }

    if (objectStoreDataVersion != indexDataVersion) {
        m_transaction->remove(m_iterator->key());
        return false;
    }

    m_value = decodeString(t, result.end());
    return true;
}

bool objectStoreCursorOptions(LevelDBTransaction* transaction, int64_t databaseId, int64_t objectStoreId, const IDBKeyRange* range, IDBCursor::Direction direction, IDBBackingStore::Cursor::CursorOptions& cursorOptions)
{
    bool lowerBound = range && range->lower();
    bool upperBound = range && range->upper();
    cursorOptions.forward = (direction == IDBCursor::NEXT_NO_DUPLICATE || direction == IDBCursor::NEXT);
    cursorOptions.unique = (direction == IDBCursor::NEXT_NO_DUPLICATE || direction == IDBCursor::PREV_NO_DUPLICATE);

    if (!lowerBound) {
        cursorOptions.lowKey = ObjectStoreDataKey::encode(databaseId, objectStoreId, minIDBKey());
        cursorOptions.lowOpen = true; // Not included.
    } else {
        cursorOptions.lowKey = ObjectStoreDataKey::encode(databaseId, objectStoreId, *range->lower());
        cursorOptions.lowOpen = range->lowerOpen();
    }

    if (!upperBound) {
        cursorOptions.highKey = ObjectStoreDataKey::encode(databaseId, objectStoreId, maxIDBKey());

        if (cursorOptions.forward)
            cursorOptions.highOpen = true; // Not included.
        else {
            // We need a key that exists.
            if (!findGreatestKeyLessThanOrEqual(transaction, cursorOptions.highKey, cursorOptions.highKey))
                return false;
            cursorOptions.highOpen = false;
        }
    } else {
        cursorOptions.highKey = ObjectStoreDataKey::encode(databaseId, objectStoreId, *range->upper());
        cursorOptions.highOpen = range->upperOpen();

        if (!cursorOptions.forward) {
            // For reverse cursors, we need a key that exists.
            Vector<char> foundHighKey;
            if (!findGreatestKeyLessThanOrEqual(transaction, cursorOptions.highKey, foundHighKey))
                return false;

            // If the target key should not be included, but we end up with a smaller key, we should include that.
            if (cursorOptions.highOpen && compareIndexKeys(foundHighKey, cursorOptions.highKey) < 0)
                cursorOptions.highOpen = false;

            cursorOptions.highKey = foundHighKey;
        }
    }

    return true;
}

bool indexCursorOptions(LevelDBTransaction* transaction, int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKeyRange* range, IDBCursor::Direction direction, IDBBackingStore::Cursor::CursorOptions& cursorOptions)
{
    ASSERT(transaction);
    bool lowerBound = range && range->lower();
    bool upperBound = range && range->upper();
    cursorOptions.forward = (direction == IDBCursor::NEXT_NO_DUPLICATE || direction == IDBCursor::NEXT);
    cursorOptions.unique = (direction == IDBCursor::NEXT_NO_DUPLICATE || direction == IDBCursor::PREV_NO_DUPLICATE);

    if (!lowerBound) {
        cursorOptions.lowKey = IndexDataKey::encodeMinKey(databaseId, objectStoreId, indexId);
        cursorOptions.lowOpen = false; // Included.
    } else {
        cursorOptions.lowKey = IndexDataKey::encode(databaseId, objectStoreId, indexId, *range->lower());
        cursorOptions.lowOpen = range->lowerOpen();
    }

    if (!upperBound) {
        cursorOptions.highKey = IndexDataKey::encodeMaxKey(databaseId, objectStoreId, indexId);
        cursorOptions.highOpen = false; // Included.

        if (!cursorOptions.forward) { // We need a key that exists.
            if (!findGreatestKeyLessThanOrEqual(transaction, cursorOptions.highKey, cursorOptions.highKey))
                return false;
            cursorOptions.highOpen = false;
        }
    } else {
        cursorOptions.highKey = IndexDataKey::encode(databaseId, objectStoreId, indexId, *range->upper());
        cursorOptions.highOpen = range->upperOpen();

        Vector<char> foundHighKey;
        if (!findGreatestKeyLessThanOrEqual(transaction, cursorOptions.highKey, foundHighKey)) // Seek to the *last* key in the set of non-unique keys.
            return false;

        // If the target key should not be included, but we end up with a smaller key, we should include that.
        if (cursorOptions.highOpen && compareIndexKeys(foundHighKey, cursorOptions.highKey) < 0)
            cursorOptions.highOpen = false;

        cursorOptions.highKey = foundHighKey;
    }

    return true;
}

PassRefPtr<IDBBackingStore::Cursor> IDBBackingStore::openObjectStoreCursor(IDBBackingStore::Transaction* transaction, int64_t databaseId, int64_t objectStoreId, const IDBKeyRange* range, IDBCursor::Direction direction)
{
    IDB_TRACE("IDBBackingStore::openObjectStoreCursor");
    LevelDBTransaction* levelDBTransaction = IDBBackingStore::Transaction::levelDBTransactionFrom(transaction);
    IDBBackingStore::Cursor::CursorOptions cursorOptions;
    if (!objectStoreCursorOptions(levelDBTransaction, databaseId, objectStoreId, range, direction, cursorOptions))
        return 0;
    RefPtr<ObjectStoreCursorImpl> cursor = ObjectStoreCursorImpl::create(levelDBTransaction, cursorOptions);
    if (!cursor->firstSeek())
        return 0;

    return cursor.release();
}

PassRefPtr<IDBBackingStore::Cursor> IDBBackingStore::openObjectStoreKeyCursor(IDBBackingStore::Transaction* transaction, int64_t databaseId, int64_t objectStoreId, const IDBKeyRange* range, IDBCursor::Direction direction)
{
    IDB_TRACE("IDBBackingStore::openObjectStoreKeyCursor");
    LevelDBTransaction* levelDBTransaction = IDBBackingStore::Transaction::levelDBTransactionFrom(transaction);
    IDBBackingStore::Cursor::CursorOptions cursorOptions;
    if (!objectStoreCursorOptions(levelDBTransaction, databaseId, objectStoreId, range, direction, cursorOptions))
        return 0;
    RefPtr<ObjectStoreKeyCursorImpl> cursor = ObjectStoreKeyCursorImpl::create(levelDBTransaction, cursorOptions);
    if (!cursor->firstSeek())
        return 0;

    return cursor.release();
}

PassRefPtr<IDBBackingStore::Cursor> IDBBackingStore::openIndexKeyCursor(IDBBackingStore::Transaction* transaction, int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKeyRange* range, IDBCursor::Direction direction)
{
    IDB_TRACE("IDBBackingStore::openIndexKeyCursor");
    LevelDBTransaction* levelDBTransaction = IDBBackingStore::Transaction::levelDBTransactionFrom(transaction);
    IDBBackingStore::Cursor::CursorOptions cursorOptions;
    if (!indexCursorOptions(levelDBTransaction, databaseId, objectStoreId, indexId, range, direction, cursorOptions))
        return 0;
    RefPtr<IndexKeyCursorImpl> cursor = IndexKeyCursorImpl::create(levelDBTransaction, cursorOptions);
    if (!cursor->firstSeek())
        return 0;

    return cursor.release();
}

PassRefPtr<IDBBackingStore::Cursor> IDBBackingStore::openIndexCursor(IDBBackingStore::Transaction* transaction, int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKeyRange* range, IDBCursor::Direction direction)
{
    IDB_TRACE("IDBBackingStore::openIndexCursor");
    LevelDBTransaction* levelDBTransaction = IDBBackingStore::Transaction::levelDBTransactionFrom(transaction);
    IDBBackingStore::Cursor::CursorOptions cursorOptions;
    if (!indexCursorOptions(levelDBTransaction, databaseId, objectStoreId, indexId, range, direction, cursorOptions))
        return 0;
    RefPtr<IndexCursorImpl> cursor = IndexCursorImpl::create(levelDBTransaction, cursorOptions);
    if (!cursor->firstSeek())
        return 0;

    return cursor.release();
}

IDBBackingStore::Transaction::Transaction(IDBBackingStore* backingStore)
    : m_backingStore(backingStore)
{
}

void IDBBackingStore::Transaction::begin()
{
    IDB_TRACE("IDBBackingStore::Transaction::begin");
    ASSERT(!m_transaction);
    m_transaction = LevelDBTransaction::create(m_backingStore->m_db.get());
}

bool IDBBackingStore::Transaction::commit()
{
    IDB_TRACE("IDBBackingStore::Transaction::commit");
    ASSERT(m_transaction);
    bool result = m_transaction->commit();
    m_transaction.clear();
    return result;
}

void IDBBackingStore::Transaction::rollback()
{
    IDB_TRACE("IDBBackingStore::Transaction::rollback");
    ASSERT(m_transaction);
    m_transaction->rollback();
    m_transaction.clear();
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
