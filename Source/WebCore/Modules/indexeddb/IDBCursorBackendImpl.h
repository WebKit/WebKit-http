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


#ifndef IDBCursorBackendImpl_h
#define IDBCursorBackendImpl_h

#if ENABLE(INDEXED_DATABASE)

#include "IDBBackingStoreInterface.h"
#include "IDBCursorBackendInterface.h"
#include "IDBDatabaseBackendImpl.h"
#include "IDBTransactionBackendImpl.h"
#include "SharedBuffer.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class IDBKeyRange;

class IDBCursorBackendImpl : public IDBCursorBackendInterface {
public:
    static PassRefPtr<IDBCursorBackendImpl> create(PassRefPtr<IDBBackingStoreInterface::Cursor> cursor, IndexedDB::CursorType cursorType, IDBTransactionBackendInterface* transaction, int64_t objectStoreId)
    {
        return adoptRef(new IDBCursorBackendImpl(cursor, cursorType, IDBDatabaseBackendInterface::NormalTask, transaction, objectStoreId));
    }
    static PassRefPtr<IDBCursorBackendImpl> create(PassRefPtr<IDBBackingStoreInterface::Cursor> cursor, IndexedDB::CursorType cursorType, IDBDatabaseBackendInterface::TaskType taskType, IDBTransactionBackendInterface* transaction, int64_t objectStoreId)
    {
        return adoptRef(new IDBCursorBackendImpl(cursor, cursorType, taskType, transaction, objectStoreId));
    }
    virtual ~IDBCursorBackendImpl();

    // IDBCursorBackendInterface
    virtual void advance(unsigned long, PassRefPtr<IDBCallbacks>, ExceptionCode&);
    virtual void continueFunction(PassRefPtr<IDBKey>, PassRefPtr<IDBCallbacks>, ExceptionCode&);
    virtual void deleteFunction(PassRefPtr<IDBCallbacks>, ExceptionCode&);
    virtual void prefetchContinue(int numberToFetch, PassRefPtr<IDBCallbacks>, ExceptionCode&);
    virtual void prefetchReset(int usedPrefetches, int unusedPrefetches);
    virtual void postSuccessHandlerCallback() { }

    virtual IDBKey* key() const OVERRIDE { return m_cursor->key().get(); }
    virtual IDBKey* primaryKey() const OVERRIDE { return m_cursor->primaryKey().get(); }
    virtual SharedBuffer* value() const OVERRIDE { return (m_cursorType == IndexedDB::CursorKeyOnly) ? 0 : m_cursor->value().get(); }

    virtual void close() OVERRIDE;

private:
    IDBCursorBackendImpl(PassRefPtr<IDBBackingStoreInterface::Cursor>, IndexedDB::CursorType, IDBDatabaseBackendInterface::TaskType, IDBTransactionBackendInterface*, int64_t objectStoreId);

    class CursorIterationOperation;
    class CursorAdvanceOperation;
    class CursorPrefetchIterationOperation;

    IDBDatabaseBackendInterface::TaskType m_taskType;
    IndexedDB::CursorType m_cursorType;
    const RefPtr<IDBDatabaseBackendInterface> m_database;
    RefPtr<IDBTransactionBackendInterface> m_transaction;
    const int64_t m_objectStoreId;

    RefPtr<IDBBackingStoreInterface::Cursor> m_cursor; // Must be destroyed before m_transaction.
    RefPtr<IDBBackingStoreInterface::Cursor> m_savedCursor; // Must be destroyed before m_transaction.

    bool m_closed;
};

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)

#endif // IDBCursorBackendImpl_h
