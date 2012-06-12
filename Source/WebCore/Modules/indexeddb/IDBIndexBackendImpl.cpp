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
#include "IDBIndexBackendImpl.h"

#if ENABLE(INDEXED_DATABASE)

#include "CrossThreadTask.h"
#include "IDBBackingStore.h"
#include "IDBCallbacks.h"
#include "IDBCursorBackendImpl.h"
#include "IDBDatabaseBackendImpl.h"
#include "IDBDatabaseException.h"
#include "IDBKey.h"
#include "IDBKeyRange.h"
#include "IDBObjectStoreBackendImpl.h"
#include "IDBTracing.h"

namespace WebCore {

IDBIndexBackendImpl::IDBIndexBackendImpl(const IDBDatabaseBackendImpl* database, IDBObjectStoreBackendImpl* objectStoreBackend, int64_t id, const String& name, const IDBKeyPath& keyPath, bool unique, bool multiEntry)
    : m_database(database)
    , m_objectStoreBackend(objectStoreBackend)
    , m_id(id)
    , m_name(name)
    , m_keyPath(keyPath)
    , m_unique(unique)
    , m_multiEntry(multiEntry)
{
}

IDBIndexBackendImpl::IDBIndexBackendImpl(const IDBDatabaseBackendImpl* database, IDBObjectStoreBackendImpl* objectStoreBackend, const String& name, const IDBKeyPath& keyPath, bool unique, bool multiEntry)
    : m_database(database)
    , m_objectStoreBackend(objectStoreBackend)
    , m_id(InvalidId)
    , m_name(name)
    , m_keyPath(keyPath)
    , m_unique(unique)
    , m_multiEntry(multiEntry)
{
}

IDBIndexBackendImpl::~IDBIndexBackendImpl()
{
}

void IDBIndexBackendImpl::openCursorInternal(ScriptExecutionContext*, PassRefPtr<IDBIndexBackendImpl> index, PassRefPtr<IDBKeyRange> range, unsigned short untypedDirection, IDBCursorBackendInterface::CursorType cursorType, PassRefPtr<IDBCallbacks> callbacks, PassRefPtr<IDBTransactionBackendInterface> transaction)
{
    IDB_TRACE("IDBIndexBackendImpl::openCursorInternal");
    IDBCursor::Direction direction = static_cast<IDBCursor::Direction>(untypedDirection);

    RefPtr<IDBBackingStore::Cursor> backingStoreCursor;

    switch (cursorType) {
    case IDBCursorBackendInterface::IndexKeyCursor:
        backingStoreCursor = index->backingStore()->openIndexKeyCursor(index->databaseId(), index->m_objectStoreBackend->id(), index->id(), range.get(), direction);
        break;
    case IDBCursorBackendInterface::IndexCursor:
        backingStoreCursor = index->backingStore()->openIndexCursor(index->databaseId(), index->m_objectStoreBackend->id(), index->id(), range.get(), direction);
        break;
    case IDBCursorBackendInterface::ObjectStoreCursor:
    case IDBCursorBackendInterface::InvalidCursorType:
        ASSERT_NOT_REACHED();
        break;
    }

    if (!backingStoreCursor) {
        callbacks->onSuccess(SerializedScriptValue::nullValue());
        return;
    }

    RefPtr<IDBCursorBackendInterface> cursor = IDBCursorBackendImpl::create(backingStoreCursor.get(), direction, cursorType, transaction.get(), index->m_objectStoreBackend);
    callbacks->onSuccess(cursor.release());
}

void IDBIndexBackendImpl::openCursor(PassRefPtr<IDBKeyRange> prpKeyRange, unsigned short direction, PassRefPtr<IDBCallbacks> prpCallbacks, IDBTransactionBackendInterface* transactionPtr, ExceptionCode& ec)
{
    IDB_TRACE("IDBIndexBackendImpl::openCursor");
    RefPtr<IDBIndexBackendImpl> index = this;
    RefPtr<IDBKeyRange> keyRange = prpKeyRange;
    RefPtr<IDBCallbacks> callbacks = prpCallbacks;
    RefPtr<IDBTransactionBackendInterface> transaction = transactionPtr;
    if (!transaction->scheduleTask(createCallbackTask(&openCursorInternal, index, keyRange, direction, IDBCursorBackendInterface::IndexCursor, callbacks, transaction)))
        ec = IDBDatabaseException::TRANSACTION_INACTIVE_ERR;
}

void IDBIndexBackendImpl::openKeyCursor(PassRefPtr<IDBKeyRange> prpKeyRange, unsigned short direction, PassRefPtr<IDBCallbacks> prpCallbacks, IDBTransactionBackendInterface* transactionPtr, ExceptionCode& ec)
{
    IDB_TRACE("IDBIndexBackendImpl::openKeyCursor");
    RefPtr<IDBIndexBackendImpl> index = this;
    RefPtr<IDBKeyRange> keyRange = prpKeyRange;
    RefPtr<IDBCallbacks> callbacks = prpCallbacks;
    RefPtr<IDBTransactionBackendInterface> transaction = transactionPtr;
    if (!transaction->scheduleTask(createCallbackTask(&openCursorInternal, index, keyRange, direction, IDBCursorBackendInterface::IndexKeyCursor, callbacks, transaction)))
        ec = IDBDatabaseException::TRANSACTION_INACTIVE_ERR;
}

void IDBIndexBackendImpl::countInternal(ScriptExecutionContext*, PassRefPtr<IDBIndexBackendImpl> index, PassRefPtr<IDBKeyRange> range, PassRefPtr<IDBCallbacks> callbacks, PassRefPtr<IDBTransactionBackendInterface>)
{
    IDB_TRACE("IDBIndexBackendImpl::countInternal");
    uint32_t count = 0;

    RefPtr<IDBBackingStore::Cursor> backingStoreCursor = index->backingStore()->openIndexKeyCursor(index->databaseId(), index->m_objectStoreBackend->id(), index->id(), range.get(), IDBCursor::NEXT);
    if (!backingStoreCursor) {
        callbacks->onSuccess(SerializedScriptValue::numberValue(count));
        return;
    }

    do {
        ++count;
    } while (backingStoreCursor->continueFunction(0));
    backingStoreCursor->close();
    callbacks->onSuccess(SerializedScriptValue::numberValue(count));
}

void IDBIndexBackendImpl::count(PassRefPtr<IDBKeyRange> range, PassRefPtr<IDBCallbacks> callbacks, IDBTransactionBackendInterface* transaction, ExceptionCode& ec)
{
    IDB_TRACE("IDBIndexBackendImpl::count");
    if (!transaction->scheduleTask(createCallbackTask(&countInternal, this, range, callbacks, transaction)))
        ec = IDBDatabaseException::TRANSACTION_INACTIVE_ERR;
}

void IDBIndexBackendImpl::getInternal(ScriptExecutionContext*, PassRefPtr<IDBIndexBackendImpl> index, PassRefPtr<IDBKeyRange> keyRange, PassRefPtr<IDBCallbacks> callbacks)
{
    IDB_TRACE("IDBIndexBackendImpl::getInternal");

    RefPtr<IDBKey> key;

    if (keyRange->isOnlyKey())
        key = keyRange->lower();
    else {
        RefPtr<IDBBackingStore::Cursor> backingStoreCursor = index->backingStore()->openIndexCursor(index->databaseId(), index->m_objectStoreBackend->id(), index->id(), keyRange.get(), IDBCursor::NEXT);

        if (!backingStoreCursor) {
            callbacks->onSuccess(SerializedScriptValue::undefinedValue());
            return;
        }
        key = backingStoreCursor->key();
        backingStoreCursor->close();
    }

    String value = index->backingStore()->getObjectViaIndex(index->databaseId(), index->m_objectStoreBackend->id(), index->id(), *key);
    if (value.isNull()) {
        callbacks->onSuccess(SerializedScriptValue::undefinedValue());
        return;
    }
    callbacks->onSuccess(SerializedScriptValue::createFromWire(value));
}

void IDBIndexBackendImpl::getKeyInternal(ScriptExecutionContext* context, PassRefPtr<IDBIndexBackendImpl> index, PassRefPtr<IDBKeyRange> keyRange, PassRefPtr<IDBCallbacks> callbacks)
{
    IDB_TRACE("IDBIndexBackendImpl::getInternal");

    RefPtr<IDBBackingStore::Cursor> backingStoreCursor =
            index->backingStore()->openIndexKeyCursor(index->databaseId(), index->m_objectStoreBackend->id(), index->id(), keyRange.get(), IDBCursor::NEXT);

    if (!backingStoreCursor) {
        callbacks->onSuccess(static_cast<IDBKey*>(0));
        return;
    }

    RefPtr<IDBKey> keyResult = index->backingStore()->getPrimaryKeyViaIndex(index->databaseId(), index->m_objectStoreBackend->id(), index->id(), *backingStoreCursor->key());
    if (!keyResult) {
        callbacks->onSuccess(static_cast<IDBKey*>(0));
        backingStoreCursor->close();
        return;
    }
    callbacks->onSuccess(keyResult.get());
    backingStoreCursor->close();
}


void IDBIndexBackendImpl::get(PassRefPtr<IDBKeyRange> prpKeyRange, PassRefPtr<IDBCallbacks> prpCallbacks, IDBTransactionBackendInterface* transaction, ExceptionCode& ec)
{
    IDB_TRACE("IDBIndexBackendImpl::get");
    RefPtr<IDBIndexBackendImpl> index = this;
    RefPtr<IDBKeyRange> keyRange = prpKeyRange;
    RefPtr<IDBCallbacks> callbacks = prpCallbacks;
    if (!transaction->scheduleTask(createCallbackTask(&getInternal, index, keyRange, callbacks)))
        ec = IDBDatabaseException::TRANSACTION_INACTIVE_ERR;
}

void IDBIndexBackendImpl::getKey(PassRefPtr<IDBKeyRange> prpKeyRange, PassRefPtr<IDBCallbacks> prpCallbacks, IDBTransactionBackendInterface* transaction, ExceptionCode& ec)
{
    IDB_TRACE("IDBIndexBackendImpl::getKey");
    RefPtr<IDBIndexBackendImpl> index = this;
    RefPtr<IDBKeyRange> keyRange = prpKeyRange;
    RefPtr<IDBCallbacks> callbacks = prpCallbacks;
    if (!transaction->scheduleTask(createCallbackTask(&getKeyInternal, index, keyRange, callbacks)))
        ec = IDBDatabaseException::TRANSACTION_INACTIVE_ERR;
}

bool IDBIndexBackendImpl::addingKeyAllowed(const IDBKey* indexKey, const IDBKey* primaryKey)
{
    if (!m_unique)
        return true;

    RefPtr<IDBKey> foundPrimaryKey;
    bool found = backingStore()->keyExistsInIndex(databaseId(), m_objectStoreBackend->id(), m_id, *indexKey, foundPrimaryKey);
    if (!found)
        return true;
    if (primaryKey && foundPrimaryKey->isEqual(primaryKey))
        return true;
    return false;
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
