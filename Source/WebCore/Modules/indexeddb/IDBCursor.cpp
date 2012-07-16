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
#include "IDBCursor.h"

#if ENABLE(INDEXED_DATABASE)

#include "IDBAny.h"
#include "IDBBindingUtilities.h"
#include "IDBCallbacks.h"
#include "IDBCursorBackendInterface.h"
#include "IDBKey.h"
#include "IDBObjectStore.h"
#include "IDBRequest.h"
#include "IDBTracing.h"
#include "IDBTransaction.h"
#include "ScriptExecutionContext.h"
#include "SerializedScriptValue.h"

namespace WebCore {

PassRefPtr<IDBCursor> IDBCursor::create(PassRefPtr<IDBCursorBackendInterface> backend, Direction direction, IDBRequest* request, IDBAny* source, IDBTransaction* transaction)
{
    return adoptRef(new IDBCursor(backend, direction, request, source, transaction));
}

const AtomicString& IDBCursor::directionNext()
{
    DEFINE_STATIC_LOCAL(AtomicString, next, ("next"));
    return next;
}

const AtomicString& IDBCursor::directionNextUnique()
{
    DEFINE_STATIC_LOCAL(AtomicString, nextunique, ("nextunique"));
    return nextunique;
}

const AtomicString& IDBCursor::directionPrev()
{
    DEFINE_STATIC_LOCAL(AtomicString, prev, ("prev"));
    return prev;
}

const AtomicString& IDBCursor::directionPrevUnique()
{
    DEFINE_STATIC_LOCAL(AtomicString, prevunique, ("prevunique"));
    return prevunique;
}


IDBCursor::IDBCursor(PassRefPtr<IDBCursorBackendInterface> backend, Direction direction, IDBRequest* request, IDBAny* source, IDBTransaction* transaction)
    : m_backend(backend)
    , m_request(request)
    , m_direction(direction)
    , m_source(source)
    , m_transaction(transaction)
    , m_transactionNotifier(transaction, this)
    , m_gotValue(false)
    , m_valueIsDirty(true)
{
    ASSERT(m_backend);
    ASSERT(m_request);
    ASSERT(m_source->type() == IDBAny::IDBObjectStoreType || m_source->type() == IDBAny::IDBIndexType);
    ASSERT(m_transaction);
}

IDBCursor::~IDBCursor()
{
}

const String& IDBCursor::direction() const
{
    IDB_TRACE("IDBCursor::direction");
    ExceptionCode ec = 0;
    const AtomicString& direction = directionToString(m_direction, ec);
    ASSERT(!ec);
    return direction;
}

PassRefPtr<IDBKey> IDBCursor::key() const
{
    IDB_TRACE("IDBCursor::key");
    return m_currentKey;
}

PassRefPtr<IDBKey> IDBCursor::primaryKey() const
{
    IDB_TRACE("IDBCursor::primaryKey");
    return m_currentPrimaryKey;
}

PassRefPtr<IDBAny> IDBCursor::value()
{
    IDB_TRACE("IDBCursor::value");
    m_valueIsDirty = false;
    return m_currentValue;
}

IDBAny* IDBCursor::source() const
{
    return m_source.get();
}

PassRefPtr<IDBRequest> IDBCursor::update(ScriptExecutionContext* context, PassRefPtr<SerializedScriptValue> prpValue, ExceptionCode& ec)
{
    IDB_TRACE("IDBCursor::update");
    RefPtr<SerializedScriptValue> value = prpValue;

    if (!m_gotValue || isKeyCursor()) {
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
    if (value->blobURLs().size() > 0) {
        // FIXME: Add Blob/File/FileList support
        ec = IDBDatabaseException::IDB_DATA_CLONE_ERR;
        return 0;
    }

    RefPtr<IDBObjectStore> objectStore = effectiveObjectStore();
    const IDBKeyPath& keyPath = objectStore->metadata().keyPath;
    const bool usesInLineKeys = !keyPath.isNull();
    if (usesInLineKeys) {
        RefPtr<IDBKey> keyPathKey = createIDBKeyFromSerializedValueAndKeyPath(value, keyPath);
        if (!keyPathKey || !keyPathKey->isEqual(m_currentPrimaryKey.get())) {
            ec = IDBDatabaseException::DATA_ERR;
            return 0;
        }
    }

    RefPtr<IDBRequest> request = IDBRequest::create(context, IDBAny::create(this), m_transaction.get());
    m_backend->update(value, request, ec);
    if (ec) {
        request->markEarlyDeath();
        return 0;
    }
    return request.release();
}

void IDBCursor::advance(unsigned long count, ExceptionCode& ec)
{
    IDB_TRACE("IDBCursor::advance");
    if (!m_gotValue) {
        ec = IDBDatabaseException::IDB_INVALID_STATE_ERR;
        return;
    }

    if (!m_transaction->isActive()) {
        ec = IDBDatabaseException::TRANSACTION_INACTIVE_ERR;
        return;
    }

    if (!count) {
        ec = IDBDatabaseException::IDB_TYPE_ERR;
        return;
    }

    if (!m_request->resetReadyState(m_transaction.get())) {
        ASSERT_NOT_REACHED();
        ec = IDBDatabaseException::IDB_INVALID_STATE_ERR;
        return;
    }
    m_request->setCursor(this);
    m_gotValue = false;
    m_backend->advance(count, m_request, ec);
}

void IDBCursor::continueFunction(PassRefPtr<IDBKey> key, ExceptionCode& ec)
{
    IDB_TRACE("IDBCursor::continue");
    if (key && !key->isValid()) {
        ec = IDBDatabaseException::DATA_ERR;
        return;
    }

    if (!m_transaction->isActive()) {
        ec = IDBDatabaseException::TRANSACTION_INACTIVE_ERR;
        return;
    }

    if (!m_gotValue) {
        ec = IDBDatabaseException::IDB_INVALID_STATE_ERR;
        return;
    }

    // FIXME: We're not using the context from when continue was called, which means the callback
    //        will be on the original context openCursor was called on. Is this right?
    if (m_request->resetReadyState(m_transaction.get())) {
        m_request->setCursor(this);
        m_gotValue = false;
        m_backend->continueFunction(key, m_request, ec);
    } else {
        ASSERT_NOT_REACHED();
        ec = IDBDatabaseException::IDB_INVALID_STATE_ERR;
    }
}

PassRefPtr<IDBRequest> IDBCursor::deleteFunction(ScriptExecutionContext* context, ExceptionCode& ec)
{
    IDB_TRACE("IDBCursor::delete");
    if (!m_transaction->isActive()) {
        ec = IDBDatabaseException::TRANSACTION_INACTIVE_ERR;
        return 0;
    }
    if (m_transaction->isReadOnly()) {
        ec = IDBDatabaseException::READ_ONLY_ERR;
        return 0;
    }

    if (!m_gotValue) {
        ec = IDBDatabaseException::IDB_INVALID_STATE_ERR;
        return 0;
    }
    RefPtr<IDBRequest> request = IDBRequest::create(context, IDBAny::create(this), m_transaction.get());
    m_backend->deleteFunction(request, ec);
    if (ec) {
        request->markEarlyDeath();
        return 0;
    }
    return request.release();
}

void IDBCursor::postSuccessHandlerCallback()
{
    m_backend->postSuccessHandlerCallback();
}

void IDBCursor::close()
{
    ASSERT(m_request);
    m_request->finishCursor();
    m_request.clear();
}

void IDBCursor::setValueReady()
{
    m_currentKey = m_backend->key();
    m_currentPrimaryKey = m_backend->primaryKey();

    RefPtr<SerializedScriptValue> value = m_backend->value();
#ifndef NDEBUG
    if (!isKeyCursor()) {
        // FIXME: Actually inject the primaryKey at the keyPath.
        RefPtr<IDBObjectStore> objectStore = effectiveObjectStore();
        if (objectStore->autoIncrement() && !objectStore->metadata().keyPath.isNull()) {
            const IDBKeyPath& keyPath = objectStore->metadata().keyPath;
            RefPtr<IDBKey> expectedKey = createIDBKeyFromSerializedValueAndKeyPath(value, keyPath);
            ASSERT(m_currentPrimaryKey);
            ASSERT(expectedKey->isEqual(m_currentPrimaryKey.get()));
        }
    }
#endif
    m_currentValue = IDBAny::create(value.release());

    m_gotValue = true;
    m_valueIsDirty = true;
}

PassRefPtr<IDBObjectStore> IDBCursor::effectiveObjectStore()
{
    if (m_source->type() == IDBAny::IDBObjectStoreType)
        return m_source->idbObjectStore();
    RefPtr<IDBIndex> index = m_source->idbIndex();
    return index->objectStore();
}

IDBCursor::Direction IDBCursor::stringToDirection(const String& directionString, ExceptionCode& ec)
{
    if (directionString == IDBCursor::directionNext())
        return IDBCursor::NEXT;
    if (directionString == IDBCursor::directionNextUnique())
        return IDBCursor::NEXT_NO_DUPLICATE;
    if (directionString == IDBCursor::directionPrev())
        return IDBCursor::PREV;
    if (directionString == IDBCursor::directionPrevUnique())
        return IDBCursor::PREV_NO_DUPLICATE;

    ec = IDBDatabaseException::IDB_TYPE_ERR;
    return IDBCursor::NEXT;
}

const AtomicString& IDBCursor::directionToString(unsigned short direction, ExceptionCode& ec)
{
    switch (direction) {
    case IDBCursor::NEXT:
        return IDBCursor::directionNext();

    case IDBCursor::NEXT_NO_DUPLICATE:
        return IDBCursor::directionNextUnique();

    case IDBCursor::PREV:
        return IDBCursor::directionPrev();

    case IDBCursor::PREV_NO_DUPLICATE:
        return IDBCursor::directionPrevUnique();

    default:
        ec = IDBDatabaseException::IDB_TYPE_ERR;
        return IDBCursor::directionNext();
    }
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
