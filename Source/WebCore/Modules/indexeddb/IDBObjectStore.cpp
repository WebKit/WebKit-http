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
#include "IDBObjectStore.h"

#if ENABLE(INDEXED_DATABASE)

#include "DOMStringList.h"
#include "IDBAny.h"
#include "IDBBindingUtilities.h"
#include "IDBCursorBackendInterface.h"
#include "IDBCursorWithValue.h"
#include "IDBDatabase.h"
#include "IDBDatabaseException.h"
#include "IDBIndex.h"
#include "IDBKey.h"
#include "IDBKeyPath.h"
#include "IDBKeyRange.h"
#include "IDBTracing.h"
#include "IDBTransaction.h"
#include "ScriptExecutionContext.h"
#include "SerializedScriptValue.h"
#include <wtf/UnusedParam.h>

namespace WebCore {

static const unsigned short defaultDirection = IDBCursor::NEXT;

IDBObjectStore::IDBObjectStore(const IDBObjectStoreMetadata& metadata, PassRefPtr<IDBObjectStoreBackendInterface> idbObjectStore, IDBTransaction* transaction)
    : m_metadata(metadata)
    , m_backend(idbObjectStore)
    , m_transaction(transaction)
    , m_deleted(false)
{
    ASSERT(m_backend);
    ASSERT(m_transaction);
    // We pass a reference to this object before it can be adopted.
    relaxAdoptionRequirement();
}

PassRefPtr<DOMStringList> IDBObjectStore::indexNames() const
{
    IDB_TRACE("IDBObjectStore::indexNames");
    RefPtr<DOMStringList> indexNames = DOMStringList::create();
    for (IDBObjectStoreMetadata::IndexMap::const_iterator it = m_metadata.indexes.begin(); it != m_metadata.indexes.end(); ++it)
        indexNames->append(it->value.name);
    indexNames->sort();
    return indexNames.release();
}

PassRefPtr<IDBRequest> IDBObjectStore::get(ScriptExecutionContext* context, PassRefPtr<IDBKeyRange> keyRange, ExceptionCode& ec)
{
    IDB_TRACE("IDBObjectStore::get");
    if (m_deleted) {
        ec = IDBDatabaseException::IDB_INVALID_STATE_ERR;
        return 0;
    }
    if (!keyRange) {
        ec = IDBDatabaseException::DATA_ERR;
        return 0;
    }
    if (!m_transaction->isActive()) {
        ec = IDBDatabaseException::TRANSACTION_INACTIVE_ERR;
        return 0;
    }
    RefPtr<IDBRequest> request = IDBRequest::create(context, IDBAny::create(this), m_transaction.get());
    m_backend->get(keyRange, request, m_transaction->backend(), ec);
    ASSERT(!ec);
    return request.release();
}

PassRefPtr<IDBRequest> IDBObjectStore::get(ScriptExecutionContext* context, PassRefPtr<IDBKey> key, ExceptionCode& ec)
{
    IDB_TRACE("IDBObjectStore::get");
    RefPtr<IDBKeyRange> keyRange = IDBKeyRange::only(key, ec);
    if (ec)
        return 0;
    return get(context, keyRange.release(), ec);
}

static void generateIndexKeysForValue(DOMRequestState* requestState, const IDBIndexMetadata& indexMetadata, const ScriptValue& objectValue, IDBObjectStore::IndexKeys* indexKeys)
{
    ASSERT(indexKeys);
    RefPtr<IDBKey> indexKey = createIDBKeyFromScriptValueAndKeyPath(requestState, objectValue, indexMetadata.keyPath);

    if (!indexKey)
        return;

    if (!indexMetadata.multiEntry || indexKey->type() != IDBKey::ArrayType) {
        if (!indexKey->isValid())
            return;

        indexKeys->append(indexKey);
    } else {
        ASSERT(indexMetadata.multiEntry);
        ASSERT(indexKey->type() == IDBKey::ArrayType);
        indexKey = IDBKey::createMultiEntryArray(indexKey->array());

        for (size_t i = 0; i < indexKey->array().size(); ++i)
            indexKeys->append(indexKey->array()[i]);
    }
}

PassRefPtr<IDBRequest> IDBObjectStore::add(ScriptState* state, ScriptValue& value, PassRefPtr<IDBKey> key, ExceptionCode& ec)
{
    IDB_TRACE("IDBObjectStore::add");
    return put(IDBObjectStoreBackendInterface::AddOnly, IDBAny::create(this), state, value, key, ec);
}

PassRefPtr<IDBRequest> IDBObjectStore::put(ScriptState* state, ScriptValue& value, PassRefPtr<IDBKey> key, ExceptionCode& ec)
{
    IDB_TRACE("IDBObjectStore::put");
    return put(IDBObjectStoreBackendInterface::AddOrUpdate, IDBAny::create(this), state, value, key, ec);
}

PassRefPtr<IDBRequest> IDBObjectStore::put(IDBObjectStoreBackendInterface::PutMode putMode, PassRefPtr<IDBAny> source, ScriptState* state, ScriptValue& value, PassRefPtr<IDBKey> prpKey, ExceptionCode& ec)
{
    IDB_TRACE("IDBObjectStore::put");
    RefPtr<IDBKey> key = prpKey;
    if (m_deleted) {
        ec = IDBDatabaseException::IDB_INVALID_STATE_ERR;
        return 0;
    }
    if (!m_transaction->isActive()) {
        ec = IDBDatabaseException::TRANSACTION_INACTIVE_ERR;
        return 0;
    }
    if (m_transaction->isReadOnly()) {
        ec = IDBDatabaseException::READ_ONLY_ERR;
        return 0;
    }

    RefPtr<SerializedScriptValue> serializedValue = value.serialize(state);
    if (state->hadException()) {
        ec = IDBDatabaseException::IDB_DATA_CLONE_ERR;
        return 0;
    }

    if (serializedValue->blobURLs().size() > 0) {
        // FIXME: Add Blob/File/FileList support
        ec = IDBDatabaseException::IDB_DATA_CLONE_ERR;
        return 0;
    }

    const IDBKeyPath& keyPath = m_metadata.keyPath;
    const bool usesInLineKeys = !keyPath.isNull();
    const bool hasKeyGenerator = autoIncrement();

    ScriptExecutionContext* context = scriptExecutionContextFromScriptState(state);
    DOMRequestState requestState(context);

    if (putMode != IDBObjectStoreBackendInterface::CursorUpdate && usesInLineKeys && key) {
        ec = IDBDatabaseException::DATA_ERR;
        return 0;
    }
    if (!usesInLineKeys && !hasKeyGenerator && !key) {
        ec = IDBDatabaseException::DATA_ERR;
        return 0;
    }
    if (usesInLineKeys) {
        RefPtr<IDBKey> keyPathKey = createIDBKeyFromScriptValueAndKeyPath(&requestState, value, keyPath);
        if (keyPathKey && !keyPathKey->isValid()) {
            ec = IDBDatabaseException::DATA_ERR;
            return 0;
        }
        if (!hasKeyGenerator && !keyPathKey) {
            ec = IDBDatabaseException::DATA_ERR;
            return 0;
        }
        if (hasKeyGenerator && !keyPathKey) {
            if (!canInjectIDBKeyIntoScriptValue(&requestState, value, keyPath)) {
                ec = IDBDatabaseException::DATA_ERR;
                return 0;
            }
        }
        if (keyPathKey)
            key = keyPathKey;
    }
    if (key && !key->isValid()) {
        ec = IDBDatabaseException::DATA_ERR;
        return 0;
    }

    Vector<int64_t> indexIds;
    Vector<IndexKeys> indexKeys;
    for (IDBObjectStoreMetadata::IndexMap::const_iterator it = m_metadata.indexes.begin(); it != m_metadata.indexes.end(); ++it) {
        IndexKeys keys;
        generateIndexKeysForValue(&requestState, it->value, value, &keys);
        indexIds.append(it->key);
        indexKeys.append(keys);
    }

    RefPtr<IDBRequest> request = IDBRequest::create(context, source, m_transaction.get());
    m_backend->put(serializedValue.release(), key.release(), putMode, request, m_transaction->backend(), indexIds, indexKeys);
    ASSERT(!ec);
    return request.release();
}

PassRefPtr<IDBRequest> IDBObjectStore::deleteFunction(ScriptExecutionContext* context, PassRefPtr<IDBKeyRange> keyRange, ExceptionCode& ec)
{
    IDB_TRACE("IDBObjectStore::delete");
    if (m_deleted) {
        ec = IDBDatabaseException::IDB_INVALID_STATE_ERR;
        return 0;
    }
    if (!m_transaction->isActive()) {
        ec = IDBDatabaseException::TRANSACTION_INACTIVE_ERR;
        return 0;
    }
    if (m_transaction->isReadOnly()) {
        ec = IDBDatabaseException::READ_ONLY_ERR;
        return 0;
    }
    if (!keyRange) {
        ec = IDBDatabaseException::DATA_ERR;
        return 0;
    }

    RefPtr<IDBRequest> request = IDBRequest::create(context, IDBAny::create(this), m_transaction.get());
    m_backend->deleteFunction(keyRange, request, m_transaction->backend(), ec);
    ASSERT(!ec);
    return request.release();
}

PassRefPtr<IDBRequest> IDBObjectStore::deleteFunction(ScriptExecutionContext* context, PassRefPtr<IDBKey> key, ExceptionCode& ec)
{
    IDB_TRACE("IDBObjectStore::delete");
    RefPtr<IDBKeyRange> keyRange = IDBKeyRange::only(key, ec);
    if (ec)
        return 0;
    return deleteFunction(context, keyRange.release(), ec);
}

PassRefPtr<IDBRequest> IDBObjectStore::clear(ScriptExecutionContext* context, ExceptionCode& ec)
{
    IDB_TRACE("IDBObjectStore::clear");
    if (m_deleted) {
        ec = IDBDatabaseException::IDB_INVALID_STATE_ERR;
        return 0;
    }
    if (!m_transaction->isActive()) {
        ec = IDBDatabaseException::TRANSACTION_INACTIVE_ERR;
        return 0;
    }
    if (m_transaction->isReadOnly()) {
        ec = IDBDatabaseException::READ_ONLY_ERR;
        return 0;
    }

    RefPtr<IDBRequest> request = IDBRequest::create(context, IDBAny::create(this), m_transaction.get());
    m_backend->clear(request, m_transaction->backend(), ec);
    ASSERT(!ec);
    return request.release();
}

namespace {
// This class creates the index keys for a given index by extracting
// them from the SerializedScriptValue, for all the existing values in
// the objectStore. It only needs to be kept alive by virtue of being
// a listener on an IDBRequest object, in the same way that JavaScript
// cursor success handlers are kept alive.
class IndexPopulator : public EventListener {
public:
    static PassRefPtr<IndexPopulator> create(PassRefPtr<IDBObjectStoreBackendInterface> backend, PassRefPtr<IDBTransactionBackendInterface> transaction, PassRefPtr<IDBRequest> request, const IDBIndexMetadata& indexMetadata)
    {
        return adoptRef(new IndexPopulator(backend, transaction, indexMetadata));
    }

    virtual bool operator==(const EventListener& other)
    {
        return this == &other;
    }

private:
    IndexPopulator(PassRefPtr<IDBObjectStoreBackendInterface> backend,
                   PassRefPtr<IDBTransactionBackendInterface> transaction,
                   const IDBIndexMetadata& indexMetadata)
        : EventListener(CPPEventListenerType)
        , m_objectStoreBackend(backend)
        , m_transaction(transaction)
        , m_indexMetadata(indexMetadata)
    {
    }

    virtual void handleEvent(ScriptExecutionContext* context, Event* event)
    {
        ASSERT(event->type() == eventNames().successEvent);
        EventTarget* target = event->target();
        IDBRequest* request = static_cast<IDBRequest*>(target);

        ExceptionCode ec = 0;
        RefPtr<IDBAny> cursorAny = request->result(ec);
        ASSERT(!ec);
        RefPtr<IDBCursorWithValue> cursor;
        if (cursorAny->type() == IDBAny::IDBCursorWithValueType)
            cursor = cursorAny->idbCursorWithValue();

        Vector<int64_t, 1> indexIds;
        indexIds.append(m_indexMetadata.id);
        if (cursor) {
            cursor->continueFunction(ec);
            ASSERT(!ec);

            RefPtr<IDBKey> primaryKey = cursor->idbPrimaryKey();
            ScriptValue value = cursor->value();

            IDBObjectStore::IndexKeys indexKeys;
            generateIndexKeysForValue(request->requestState(), m_indexMetadata, value, &indexKeys);

            Vector<IDBObjectStore::IndexKeys, 1> indexKeysList;
            indexKeysList.append(indexKeys);

            m_objectStoreBackend->setIndexKeys(primaryKey, indexIds, indexKeysList, m_transaction.get());

        } else {
            // Now that we are done indexing, tell the backend to go
            // back to processing tasks of type NormalTask.
            m_objectStoreBackend->setIndexesReady(indexIds, m_transaction.get());
            m_objectStoreBackend.clear();
            m_transaction.clear();
        }

    }

    RefPtr<IDBObjectStoreBackendInterface> m_objectStoreBackend;
    RefPtr<IDBTransactionBackendInterface> m_transaction;
    IDBIndexMetadata m_indexMetadata;
};
}

PassRefPtr<IDBIndex> IDBObjectStore::createIndex(ScriptExecutionContext* context, const String& name, const IDBKeyPath& keyPath, const Dictionary& options, ExceptionCode& ec)
{
    IDB_TRACE("IDBObjectStore::createIndex");
    if (!m_transaction->isVersionChange() || m_deleted) {
        ec = IDBDatabaseException::IDB_INVALID_STATE_ERR;
        return 0;
    }
    if (!m_transaction->isActive()) {
        ec = IDBDatabaseException::TRANSACTION_INACTIVE_ERR;
        return 0;
    }
    if (!keyPath.isValid()) {
        ec = IDBDatabaseException::IDB_SYNTAX_ERR;
        return 0;
    }
    if (name.isNull()) {
        ec = TypeError;
        return 0;
    }
    if (containsIndex(name)) {
        ec = IDBDatabaseException::CONSTRAINT_ERR;
        return 0;
    }

    bool unique = false;
    options.get("unique", unique);

    bool multiEntry = false;
    options.get("multiEntry", multiEntry);

    if (keyPath.type() == IDBKeyPath::ArrayType && multiEntry) {
        ec = IDBDatabaseException::IDB_INVALID_ACCESS_ERR;
        return 0;
    }

    int64_t indexId = m_metadata.maxIndexId + 1;
    RefPtr<IDBIndexBackendInterface> indexBackend = m_backend->createIndex(indexId, name, keyPath, unique, multiEntry, m_transaction->backend(), ec);
    ASSERT(!indexBackend != !ec); // If we didn't get an index, we should have gotten an exception code. And vice versa.
    if (ec)
        return 0;

    ++m_metadata.maxIndexId;

    IDBIndexMetadata metadata(name, indexId, keyPath, unique, multiEntry);
    RefPtr<IDBIndex> index = IDBIndex::create(metadata, indexBackend.release(), this, m_transaction.get());
    m_indexMap.set(name, index);
    m_metadata.indexes.set(indexId, metadata);

    ASSERT(!ec);
    if (ec)
        return 0;

    RefPtr<IDBRequest> indexRequest = openCursor(context, static_cast<IDBKeyRange*>(0), IDBCursor::directionNext(), IDBTransactionBackendInterface::PreemptiveTask, ec);
    ASSERT(!ec);
    if (ec)
        return 0;
    indexRequest->preventPropagation();

    // This is kept alive by being the success handler of the request, which is in turn kept alive by the owning transaction.
    RefPtr<IndexPopulator> indexPopulator = IndexPopulator::create(m_backend, m_transaction->backend(), indexRequest, metadata);
    indexRequest->setOnsuccess(indexPopulator);

    return index.release();
}

PassRefPtr<IDBIndex> IDBObjectStore::index(const String& name, ExceptionCode& ec)
{
    IDB_TRACE("IDBObjectStore::index");
    if (m_deleted) {
        ec = IDBDatabaseException::IDB_INVALID_STATE_ERR;
        return 0;
    }
    if (m_transaction->isFinished()) {
        ec = IDBDatabaseException::IDB_INVALID_STATE_ERR;
        return 0;
    }

    IDBIndexMap::iterator it = m_indexMap.find(name);
    if (it != m_indexMap.end())
        return it->value;

    int64_t indexId = findIndexId(name);
    if (indexId == IDBIndexMetadata::InvalidId) {
        ec = IDBDatabaseException::IDB_NOT_FOUND_ERR;
        return 0;
    }

    RefPtr<IDBIndexBackendInterface> indexBackend = m_backend->index(indexId);
    ASSERT(!ec && indexBackend);

    const IDBIndexMetadata* indexMetadata(0);
    for (IDBObjectStoreMetadata::IndexMap::const_iterator it = m_metadata.indexes.begin(); it != m_metadata.indexes.end(); ++it) {
        if (it->value.name == name) {
            indexMetadata = &it->value;
            break;
        }
    }
    ASSERT(indexMetadata);

    RefPtr<IDBIndex> index = IDBIndex::create(*indexMetadata, indexBackend.release(), this, m_transaction.get());
    m_indexMap.set(name, index);
    return index.release();
}

void IDBObjectStore::deleteIndex(const String& name, ExceptionCode& ec)
{
    if (!m_transaction->isVersionChange() || m_deleted) {
        ec = IDBDatabaseException::IDB_INVALID_STATE_ERR;
        return;
    }
    if (!m_transaction->isActive()) {
        ec = IDBDatabaseException::TRANSACTION_INACTIVE_ERR;
        return;
    }
    int64_t indexId = findIndexId(name);
    if (indexId == IDBIndexMetadata::InvalidId) {
        ec = IDBDatabaseException::IDB_NOT_FOUND_ERR;
        return;
    }

    m_backend->deleteIndex(indexId, m_transaction->backend(), ec);
    if (!ec) {
        IDBIndexMap::iterator it = m_indexMap.find(name);
        if (it != m_indexMap.end()) {
            m_metadata.indexes.remove(it->value->id());
            it->value->markDeleted();
            m_indexMap.remove(name);
        }
    }
}

PassRefPtr<IDBRequest> IDBObjectStore::openCursor(ScriptExecutionContext* context, PassRefPtr<IDBKeyRange> range, const String& directionString, IDBTransactionBackendInterface::TaskType taskType, ExceptionCode& ec)
{
    IDB_TRACE("IDBObjectStore::openCursor");
    if (m_deleted) {
        ec = IDBDatabaseException::IDB_INVALID_STATE_ERR;
        return 0;
    }
    if (!m_transaction->isActive()) {
        ec = IDBDatabaseException::TRANSACTION_INACTIVE_ERR;
        return 0;
    }
    IDBCursor::Direction direction = IDBCursor::stringToDirection(directionString, context, ec);
    if (ec)
        return 0;

    RefPtr<IDBRequest> request = IDBRequest::create(context, IDBAny::create(this), m_transaction.get());
    request->setCursorDetails(IDBCursorBackendInterface::ObjectStoreCursor, direction);
    m_backend->openCursor(range, direction, request, taskType, m_transaction->backend(), ec);
    ASSERT(!ec);
    return request.release();
}

PassRefPtr<IDBRequest> IDBObjectStore::openCursor(ScriptExecutionContext* context, PassRefPtr<IDBKey> key, const String& direction, ExceptionCode& ec)
{
    IDB_TRACE("IDBObjectStore::openCursor");
    RefPtr<IDBKeyRange> keyRange = IDBKeyRange::only(key, ec);
    if (ec)
        return 0;
    return openCursor(context, keyRange.release(), direction, ec);
}

PassRefPtr<IDBRequest> IDBObjectStore::count(ScriptExecutionContext* context, PassRefPtr<IDBKeyRange> range, ExceptionCode& ec)
{
    IDB_TRACE("IDBObjectStore::count");
    if (m_deleted) {
        ec = IDBDatabaseException::IDB_INVALID_STATE_ERR;
        return 0;
    }
    if (!m_transaction->isActive()) {
        ec = IDBDatabaseException::TRANSACTION_INACTIVE_ERR;
        return 0;
    }
    RefPtr<IDBRequest> request = IDBRequest::create(context, IDBAny::create(this), m_transaction.get());
    m_backend->count(range, request, m_transaction->backend(), ec);
    ASSERT(!ec);
    return request.release();
}

PassRefPtr<IDBRequest> IDBObjectStore::count(ScriptExecutionContext* context, PassRefPtr<IDBKey> key, ExceptionCode& ec)
{
    IDB_TRACE("IDBObjectStore::count");
    RefPtr<IDBKeyRange> keyRange = IDBKeyRange::only(key, ec);
    if (ec)
        return 0;
    return count(context, keyRange.release(), ec);
}

void IDBObjectStore::transactionFinished()
{
    ASSERT(m_transaction->isFinished());

    // Break reference cycles.
    m_indexMap.clear();
}

int64_t IDBObjectStore::findIndexId(const String& name) const
{
    for (IDBObjectStoreMetadata::IndexMap::const_iterator it = m_metadata.indexes.begin(); it != m_metadata.indexes.end(); ++it) {
        if (it->value.name == name) {
            ASSERT(it->key != IDBIndexMetadata::InvalidId);
            return it->key;
        }
    }
    return IDBIndexMetadata::InvalidId;
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
