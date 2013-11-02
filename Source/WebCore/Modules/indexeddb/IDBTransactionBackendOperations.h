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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef IDBTransactionBackendOperations_h
#define IDBTransactionBackendOperations_h

#include "IDBDatabaseBackendImpl.h"
#include "IDBOperation.h"
#include "IDBTransactionBackendInterface.h"

#if ENABLE(INDEXED_DATABASE)

namespace WebCore {

class CreateObjectStoreOperation : public IDBOperation {
public:
    static PassOwnPtr<IDBOperation> create(IDBTransactionBackendInterface* transaction, IDBBackingStoreInterface* backingStore, const IDBObjectStoreMetadata& objectStoreMetadata)
    {
        return adoptPtr(new CreateObjectStoreOperation(transaction, backingStore, objectStoreMetadata));
    }
    virtual void perform() OVERRIDE FINAL;
private:
    CreateObjectStoreOperation(IDBTransactionBackendInterface* transaction, IDBBackingStoreInterface* backingStore, const IDBObjectStoreMetadata& objectStoreMetadata)
        : m_transaction(transaction)
        , m_backingStore(backingStore)
        , m_objectStoreMetadata(objectStoreMetadata)
    {
    }
    
    RefPtr<IDBTransactionBackendInterface> m_transaction;
    const RefPtr<IDBBackingStoreInterface> m_backingStore;
    const IDBObjectStoreMetadata m_objectStoreMetadata;
};

class DeleteObjectStoreOperation : public IDBOperation {
public:
    static PassOwnPtr<IDBOperation> create(IDBTransactionBackendInterface* transaction, IDBBackingStoreInterface* backingStore, const IDBObjectStoreMetadata& objectStoreMetadata)
    {
        return adoptPtr(new DeleteObjectStoreOperation(transaction, backingStore, objectStoreMetadata));
    }
    virtual void perform() OVERRIDE FINAL;
private:
    DeleteObjectStoreOperation(IDBTransactionBackendInterface* transaction, IDBBackingStoreInterface* backingStore, const IDBObjectStoreMetadata& objectStoreMetadata)
        : m_transaction(transaction)
        , m_backingStore(backingStore)
        , m_objectStoreMetadata(objectStoreMetadata)
    {
    }

    RefPtr<IDBTransactionBackendInterface> m_transaction;
    const RefPtr<IDBBackingStoreInterface> m_backingStore;
    const IDBObjectStoreMetadata m_objectStoreMetadata;
};

class IDBDatabaseBackendImpl::VersionChangeOperation : public IDBOperation {
public:
    static PassOwnPtr<IDBOperation> create(IDBTransactionBackendInterface* transaction, int64_t transactionId, int64_t version, PassRefPtr<IDBCallbacks> callbacks, PassRefPtr<IDBDatabaseCallbacks> databaseCallbacks)
    {
        return adoptPtr(new VersionChangeOperation(transaction, transactionId, version, callbacks, databaseCallbacks));
    }
    virtual void perform() OVERRIDE FINAL;
private:
    VersionChangeOperation(IDBTransactionBackendInterface* transaction, int64_t transactionId, int64_t version, PassRefPtr<IDBCallbacks> callbacks, PassRefPtr<IDBDatabaseCallbacks> databaseCallbacks)
        : m_transaction(transaction)
        , m_transactionId(transactionId)
        , m_version(version)
        , m_callbacks(callbacks)
        , m_databaseCallbacks(databaseCallbacks)
    {
    }

    RefPtr<IDBTransactionBackendInterface> m_transaction;
    int64_t m_transactionId;
    int64_t m_version;
    RefPtr<IDBCallbacks> m_callbacks;
    RefPtr<IDBDatabaseCallbacks> m_databaseCallbacks;
};

class CreateObjectStoreAbortOperation : public IDBOperation {
public:
    static PassOwnPtr<IDBOperation> create(IDBTransactionBackendInterface* transaction, int64_t objectStoreId)
    {
        return adoptPtr(new CreateObjectStoreAbortOperation(transaction, objectStoreId));
    }
    virtual void perform() OVERRIDE FINAL;
private:
    CreateObjectStoreAbortOperation(IDBTransactionBackendInterface* transaction, int64_t objectStoreId)
        : m_transaction(transaction)
        , m_objectStoreId(objectStoreId)
    {
    }

    RefPtr<IDBTransactionBackendInterface> m_transaction;
    const int64_t m_objectStoreId;
};

class DeleteObjectStoreAbortOperation : public IDBOperation {
public:
    static PassOwnPtr<IDBOperation> create(IDBTransactionBackendInterface* transaction, const IDBObjectStoreMetadata& objectStore)
    {
        return adoptPtr(new DeleteObjectStoreAbortOperation(transaction, objectStore));
    }
    virtual void perform() OVERRIDE FINAL;
private:
    DeleteObjectStoreAbortOperation(IDBTransactionBackendInterface* transaction, const IDBObjectStoreMetadata& objectStoreMetadata)
        : m_transaction(transaction)
        , m_objectStoreMetadata(objectStoreMetadata)
    {
    }

    RefPtr<IDBTransactionBackendInterface> m_transaction;
    IDBObjectStoreMetadata m_objectStoreMetadata;
};

class IDBDatabaseBackendImpl::VersionChangeAbortOperation : public IDBOperation {
public:
    static PassOwnPtr<IDBOperation> create(IDBTransactionBackendInterface* transaction, const String& previousVersion, int64_t previousIntVersion)
    {
        return adoptPtr(new VersionChangeAbortOperation(transaction, previousVersion, previousIntVersion));
    }
    virtual void perform() OVERRIDE FINAL;
private:
    VersionChangeAbortOperation(IDBTransactionBackendInterface* transaction, const String& previousVersion, int64_t previousIntVersion)
        : m_transaction(transaction)
        , m_previousVersion(previousVersion)
        , m_previousIntVersion(previousIntVersion)
    {
    }

    RefPtr<IDBTransactionBackendInterface> m_transaction;
    String m_previousVersion;
    int64_t m_previousIntVersion;
};

class CreateIndexOperation : public IDBOperation {
public:
    static PassOwnPtr<IDBOperation> create(IDBTransactionBackendInterface* transaction, IDBBackingStoreInterface* backingStore, int64_t objectStoreId, const IDBIndexMetadata& indexMetadata)
    {
        return adoptPtr(new CreateIndexOperation(transaction, backingStore, objectStoreId, indexMetadata));
    }
    virtual void perform() OVERRIDE FINAL;
private:
    CreateIndexOperation(IDBTransactionBackendInterface* transaction, IDBBackingStoreInterface* backingStore, int64_t objectStoreId, const IDBIndexMetadata& indexMetadata)
        : m_transaction(transaction)
        , m_backingStore(backingStore)
        , m_objectStoreId(objectStoreId)
        , m_indexMetadata(indexMetadata)
    {
    }

    RefPtr<IDBTransactionBackendInterface> m_transaction;
    const RefPtr<IDBBackingStoreInterface> m_backingStore;
    const int64_t m_objectStoreId;
    const IDBIndexMetadata m_indexMetadata;
};

class CreateIndexAbortOperation : public IDBOperation {
public:
    static PassOwnPtr<IDBOperation> create(IDBTransactionBackendInterface* transaction, int64_t objectStoreId, int64_t indexId)
    {
        return adoptPtr(new CreateIndexAbortOperation(transaction, objectStoreId, indexId));
    }
    virtual void perform() OVERRIDE FINAL;
private:
    CreateIndexAbortOperation(IDBTransactionBackendInterface* transaction, int64_t objectStoreId, int64_t indexId)
        : m_transaction(transaction)
        , m_objectStoreId(objectStoreId)
        , m_indexId(indexId)
    {
    }

    const RefPtr<IDBTransactionBackendInterface> m_transaction;
    const int64_t m_objectStoreId;
    const int64_t m_indexId;
};

class DeleteIndexOperation : public IDBOperation {
public:
    static PassOwnPtr<IDBOperation> create(IDBTransactionBackendInterface* transaction, IDBBackingStoreInterface* backingStore, int64_t objectStoreId, const IDBIndexMetadata& indexMetadata)
    {
        return adoptPtr(new DeleteIndexOperation(transaction, backingStore, objectStoreId, indexMetadata));
    }
    virtual void perform() OVERRIDE FINAL;
private:
    DeleteIndexOperation(IDBTransactionBackendInterface* transaction, IDBBackingStoreInterface* backingStore, int64_t objectStoreId, const IDBIndexMetadata& indexMetadata)
        : m_transaction(transaction)
        , m_backingStore(backingStore)
        , m_objectStoreId(objectStoreId)
        , m_indexMetadata(indexMetadata)
    {
    }

    RefPtr<IDBTransactionBackendInterface> m_transaction;
    const RefPtr<IDBBackingStoreInterface> m_backingStore;
    const int64_t m_objectStoreId;
    const IDBIndexMetadata m_indexMetadata;
};

class DeleteIndexAbortOperation : public IDBOperation {
public:
    static PassOwnPtr<IDBOperation> create(IDBTransactionBackendInterface* transaction, int64_t objectStoreId, const IDBIndexMetadata& indexMetadata)
    {
        return adoptPtr(new DeleteIndexAbortOperation(transaction, objectStoreId, indexMetadata));
    }
    virtual void perform() OVERRIDE FINAL;
private:
    DeleteIndexAbortOperation(IDBTransactionBackendInterface* transaction, int64_t objectStoreId, const IDBIndexMetadata& indexMetadata)
        : m_transaction(transaction)
        , m_objectStoreId(objectStoreId)
        , m_indexMetadata(indexMetadata)
    {
    }

    RefPtr<IDBTransactionBackendInterface> m_transaction;
    const int64_t m_objectStoreId;
    const IDBIndexMetadata m_indexMetadata;
};

class GetOperation : public IDBOperation {
public:
    static PassOwnPtr<IDBOperation> create(IDBTransactionBackendInterface* transaction, IDBBackingStoreInterface* backingStore, const IDBDatabaseMetadata& metadata, int64_t objectStoreId, int64_t indexId, PassRefPtr<IDBKeyRange> keyRange, IndexedDB::CursorType cursorType, PassRefPtr<IDBCallbacks> callbacks)
    {
        return adoptPtr(new GetOperation(transaction, backingStore, metadata, objectStoreId, indexId, keyRange, cursorType, callbacks));
    }
    virtual void perform() OVERRIDE FINAL;
private:
    GetOperation(IDBTransactionBackendInterface* transaction, IDBBackingStoreInterface* backingStore, const IDBDatabaseMetadata& metadata, int64_t objectStoreId, int64_t indexId, PassRefPtr<IDBKeyRange> keyRange, IndexedDB::CursorType cursorType, PassRefPtr<IDBCallbacks> callbacks)
        : m_transaction(transaction)
        , m_backingStore(backingStore)
        , m_databaseId(metadata.id)
        , m_objectStoreId(objectStoreId)
        , m_indexId(indexId)
        , m_keyPath(metadata.objectStores.get(objectStoreId).keyPath)
        , m_autoIncrement(metadata.objectStores.get(objectStoreId).autoIncrement)
        , m_keyRange(keyRange)
        , m_cursorType(cursorType)
        , m_callbacks(callbacks)
    {
        ASSERT(metadata.objectStores.contains(objectStoreId));
        ASSERT(metadata.objectStores.get(objectStoreId).id == objectStoreId);
    }

    RefPtr<IDBTransactionBackendInterface> m_transaction;
    const RefPtr<IDBBackingStoreInterface> m_backingStore;
    const int64_t m_databaseId;
    const int64_t m_objectStoreId;
    const int64_t m_indexId;
    const IDBKeyPath m_keyPath;
    const bool m_autoIncrement;
    const RefPtr<IDBKeyRange> m_keyRange;
    const IndexedDB::CursorType m_cursorType;
    const RefPtr<IDBCallbacks> m_callbacks;
};

class PutOperation : public IDBOperation {
public:
    static PassOwnPtr<IDBOperation> create(IDBTransactionBackendInterface* transaction, IDBBackingStoreInterface* backingStore, int64_t databaseId, const IDBObjectStoreMetadata& objectStore, PassRefPtr<SharedBuffer> value, PassRefPtr<IDBKey> key, IDBDatabaseBackendInterface::PutMode putMode, PassRefPtr<IDBCallbacks> callbacks, const Vector<int64_t>& indexIds, const Vector<IndexKeys>& indexKeys)
    {
        return adoptPtr(new PutOperation(transaction, backingStore, databaseId, objectStore, value, key, putMode, callbacks, indexIds, indexKeys));
    }
    virtual void perform() OVERRIDE FINAL;
private:
    PutOperation(IDBTransactionBackendInterface* transaction, IDBBackingStoreInterface* backingStore, int64_t databaseId, const IDBObjectStoreMetadata& objectStore, PassRefPtr<SharedBuffer>& value, PassRefPtr<IDBKey> key, IDBDatabaseBackendInterface::PutMode putMode, PassRefPtr<IDBCallbacks> callbacks, const Vector<int64_t>& indexIds, const Vector<IndexKeys>& indexKeys)
        : m_transaction(transaction)
        , m_backingStore(backingStore)
        , m_databaseId(databaseId)
        , m_objectStore(objectStore)
        , m_value(value)
        , m_key(key)
        , m_putMode(putMode)
        , m_callbacks(callbacks)
        , m_indexIds(indexIds)
        , m_indexKeys(indexKeys)
    {
    }

    RefPtr<IDBTransactionBackendInterface> m_transaction;
    const RefPtr<IDBBackingStoreInterface> m_backingStore;
    const int64_t m_databaseId;
    const IDBObjectStoreMetadata m_objectStore;
    const RefPtr<SharedBuffer> m_value;
    const RefPtr<IDBKey> m_key;
    const IDBDatabaseBackendInterface::PutMode m_putMode;
    const RefPtr<IDBCallbacks> m_callbacks;
    const Vector<int64_t> m_indexIds;
    const Vector<IndexKeys> m_indexKeys;
};

class SetIndexesReadyOperation : public IDBOperation {
public:
    static PassOwnPtr<IDBOperation> create(IDBTransactionBackendInterface* transaction, size_t indexCount)
    {
        return adoptPtr(new SetIndexesReadyOperation(transaction, indexCount));
    }
    virtual void perform() OVERRIDE FINAL;
private:
    SetIndexesReadyOperation(IDBTransactionBackendInterface* transaction, size_t indexCount)
        : m_transaction(transaction)
        , m_indexCount(indexCount)
    {
    }

    RefPtr<IDBTransactionBackendInterface> m_transaction;
    const size_t m_indexCount;
};

class OpenCursorOperation : public IDBOperation {
public:
    static PassOwnPtr<IDBOperation> create(IDBTransactionBackendInterface* transaction, IDBBackingStoreInterface* backingStore, int64_t databaseId, int64_t objectStoreId, int64_t indexId, PassRefPtr<IDBKeyRange> keyRange, IndexedDB::CursorDirection direction, IndexedDB::CursorType cursorType, IDBDatabaseBackendInterface::TaskType taskType, PassRefPtr<IDBCallbacks> callbacks)
    {
        return adoptPtr(new OpenCursorOperation(transaction, backingStore, databaseId, objectStoreId, indexId, keyRange, direction, cursorType, taskType, callbacks));
    }
    virtual void perform() OVERRIDE FINAL;
private:
    OpenCursorOperation(IDBTransactionBackendInterface* transaction, IDBBackingStoreInterface* backingStore, int64_t databaseId, int64_t objectStoreId, int64_t indexId, PassRefPtr<IDBKeyRange> keyRange, IndexedDB::CursorDirection direction, IndexedDB::CursorType cursorType, IDBDatabaseBackendInterface::TaskType taskType, PassRefPtr<IDBCallbacks> callbacks)
        : m_transaction(transaction)
        , m_backingStore(backingStore)
        , m_databaseId(databaseId)
        , m_objectStoreId(objectStoreId)
        , m_indexId(indexId)
        , m_keyRange(keyRange)
        , m_direction(direction)
        , m_cursorType(cursorType)
        , m_taskType(taskType)
        , m_callbacks(callbacks)
    {
    }

    RefPtr<IDBTransactionBackendInterface> m_transaction;
    const RefPtr<IDBBackingStoreInterface> m_backingStore;
    const int64_t m_databaseId;
    const int64_t m_objectStoreId;
    const int64_t m_indexId;
    const PassRefPtr<IDBKeyRange> m_keyRange;
    const IndexedDB::CursorDirection m_direction;
    const IndexedDB::CursorType m_cursorType;
    const IDBDatabaseBackendInterface::TaskType m_taskType;
    const RefPtr<IDBCallbacks> m_callbacks;
};

class CountOperation : public IDBOperation {
public:
    static PassOwnPtr<IDBOperation> create(IDBTransactionBackendInterface* transaction, IDBBackingStoreInterface* backingStore, int64_t databaseId, int64_t objectStoreId, int64_t indexId, PassRefPtr<IDBKeyRange> keyRange, PassRefPtr<IDBCallbacks> callbacks)
    {
        return adoptPtr(new CountOperation(transaction, backingStore, databaseId, objectStoreId, indexId, keyRange, callbacks));
    }
    virtual void perform() OVERRIDE FINAL;
private:
    CountOperation(IDBTransactionBackendInterface* transaction, IDBBackingStoreInterface* backingStore, int64_t databaseId, int64_t objectStoreId, int64_t indexId, PassRefPtr<IDBKeyRange> keyRange, PassRefPtr<IDBCallbacks> callbacks)
        : m_transaction(transaction)
        , m_backingStore(backingStore)
        , m_databaseId(databaseId)
        , m_objectStoreId(objectStoreId)
        , m_indexId(indexId)
        , m_keyRange(keyRange)
        , m_callbacks(callbacks)
    {
    }

    RefPtr<IDBTransactionBackendInterface> m_transaction;
    const RefPtr<IDBBackingStoreInterface> m_backingStore;
    const int64_t m_databaseId;
    const int64_t m_objectStoreId;
    const int64_t m_indexId;
    const RefPtr<IDBKeyRange> m_keyRange;
    const RefPtr<IDBCallbacks> m_callbacks;
};

class DeleteRangeOperation : public IDBOperation {
public:
    static PassOwnPtr<IDBOperation> create(IDBTransactionBackendInterface* transaction, IDBBackingStoreInterface* backingStore, int64_t databaseId, int64_t objectStoreId, PassRefPtr<IDBKeyRange> keyRange, PassRefPtr<IDBCallbacks> callbacks)
    {
        return adoptPtr(new DeleteRangeOperation(transaction, backingStore, databaseId, objectStoreId, keyRange, callbacks));
    }
    virtual void perform() OVERRIDE FINAL;
private:
    DeleteRangeOperation(IDBTransactionBackendInterface* transaction, IDBBackingStoreInterface* backingStore, int64_t databaseId, int64_t objectStoreId, PassRefPtr<IDBKeyRange> keyRange, PassRefPtr<IDBCallbacks> callbacks)
        : m_transaction(transaction)
        , m_backingStore(backingStore)
        , m_databaseId(databaseId)
        , m_objectStoreId(objectStoreId)
        , m_keyRange(keyRange)
        , m_callbacks(callbacks)
    {
    }

    RefPtr<IDBTransactionBackendInterface> m_transaction;
    const RefPtr<IDBBackingStoreInterface> m_backingStore;
    const int64_t m_databaseId;
    const int64_t m_objectStoreId;
    const RefPtr<IDBKeyRange> m_keyRange;
    const RefPtr<IDBCallbacks> m_callbacks;
};

class ClearOperation : public IDBOperation {
public:
    static PassOwnPtr<IDBOperation> create(IDBTransactionBackendInterface* transaction, IDBBackingStoreInterface* backingStore, int64_t databaseId, int64_t objectStoreId, PassRefPtr<IDBCallbacks> callbacks)
    {
        return adoptPtr(new ClearOperation(transaction, backingStore, databaseId, objectStoreId, callbacks));
    }
    virtual void perform() OVERRIDE FINAL;
private:
    ClearOperation(IDBTransactionBackendInterface* transaction, IDBBackingStoreInterface* backingStore, int64_t databaseId, int64_t objectStoreId, PassRefPtr<IDBCallbacks> callbacks)
        : m_transaction(transaction)
        , m_backingStore(backingStore)
        , m_databaseId(databaseId)
        , m_objectStoreId(objectStoreId)
        , m_callbacks(callbacks)
    {
    }

    RefPtr<IDBTransactionBackendInterface> m_transaction;
    const RefPtr<IDBBackingStoreInterface> m_backingStore;
    const int64_t m_databaseId;
    const int64_t m_objectStoreId;
    const RefPtr<IDBCallbacks> m_callbacks;
};

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
#endif // IDBTransactionBackendOperations_h
