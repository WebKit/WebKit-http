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

#ifndef IDBDatabaseBackendImpl_h
#define IDBDatabaseBackendImpl_h

#include "IDBCallbacks.h"
#include "IDBDatabase.h"
#include <stdint.h>
#include <wtf/Deque.h>
#include <wtf/HashMap.h>
#include <wtf/ListHashSet.h>

#if ENABLE(INDEXED_DATABASE)

namespace WebCore {

class IDBBackingStore;
class IDBDatabase;
class IDBFactoryBackendImpl;
class IDBObjectStoreBackendImpl;
class IDBTransactionBackendImpl;
class IDBTransactionBackendInterface;
class IDBTransactionCoordinator;

class IDBDatabaseBackendImpl : public IDBDatabaseBackendInterface {
public:
    static PassRefPtr<IDBDatabaseBackendImpl> create(const String& name, IDBBackingStore* database, IDBFactoryBackendImpl*, const String& uniqueIdentifier);
    virtual ~IDBDatabaseBackendImpl();

    PassRefPtr<IDBBackingStore> backingStore() const;

    static const int64_t InvalidId = 0;
    int64_t id() const { return m_metadata.id; }

    void openConnection(PassRefPtr<IDBCallbacks>, PassRefPtr<IDBDatabaseCallbacks>, int64_t transactionId);
    void openConnectionWithVersion(PassRefPtr<IDBCallbacks>, PassRefPtr<IDBDatabaseCallbacks>, int64_t transactionId, int64_t version);
    void deleteDatabase(PassRefPtr<IDBCallbacks>);

    // IDBDatabaseBackendInterface
    virtual IDBDatabaseMetadata metadata() const;
    virtual PassRefPtr<IDBObjectStoreBackendInterface> createObjectStore(int64_t id, const String& name, const IDBKeyPath&, bool autoIncrement, IDBTransactionBackendInterface*, ExceptionCode&);
    virtual void deleteObjectStore(int64_t, IDBTransactionBackendInterface*, ExceptionCode&);
    // FIXME: Remove this method in https://bugs.webkit.org/show_bug.cgi?id=103923.
    virtual PassRefPtr<IDBTransactionBackendInterface> createTransaction(int64_t transactionId, const Vector<int64_t>& objectStoreIds, IDBTransaction::Mode);
    virtual void createTransaction(int64_t transactionId, PassRefPtr<IDBDatabaseCallbacks>, const Vector<int64_t>& objectStoreIds, unsigned short mode);
    virtual void close(PassRefPtr<IDBDatabaseCallbacks>);

    virtual void commit(int64_t transactionId);
    virtual void abort(int64_t transactionId);

    PassRefPtr<IDBObjectStoreBackendImpl> objectStore(int64_t id);
    IDBTransactionCoordinator* transactionCoordinator() const { return m_transactionCoordinator.get(); }
    void transactionStarted(PassRefPtr<IDBTransactionBackendImpl>);
    void transactionFinished(PassRefPtr<IDBTransactionBackendImpl>);
    void transactionFinishedAndCompleteFired(PassRefPtr<IDBTransactionBackendImpl>);
    void transactionFinishedAndAbortFired(PassRefPtr<IDBTransactionBackendImpl>);

    virtual void get(int64_t transactionId, int64_t objectStoreId, int64_t indexId, PassRefPtr<IDBKeyRange>, bool keyOnly, PassRefPtr<IDBCallbacks>) OVERRIDE;
    virtual void put(int64_t transactionId, int64_t objectStoreId, const Vector<uint8_t>&, PassRefPtr<IDBKey>, PutMode, PassRefPtr<IDBCallbacks>, const Vector<int64_t>& indexIds, const Vector<IndexKeys>&) OVERRIDE;
    virtual void setIndexKeys(int64_t transactionId, int64_t objectStoreId, PassRefPtr<IDBKey> prpPrimaryKey, const Vector<int64_t>& indexIds, const Vector<IndexKeys>&) OVERRIDE;
    virtual void setIndexesReady(int64_t transactionId, int64_t objectStoreId, const Vector<int64_t>& indexIds) OVERRIDE;
    virtual void openCursor(int64_t transactionId, int64_t objectStoreId, int64_t indexId, PassRefPtr<IDBKeyRange>, unsigned short direction, bool keyOnly, TaskType, PassRefPtr<IDBCallbacks>) OVERRIDE;
    virtual void count(int64_t transactionId, int64_t objectStoreId, int64_t indexId, PassRefPtr<IDBKeyRange>, PassRefPtr<IDBCallbacks>) OVERRIDE;
    virtual void deleteRange(int64_t transactionId, int64_t objectStoreId, PassRefPtr<IDBKeyRange>, PassRefPtr<IDBCallbacks>) OVERRIDE;
    virtual void clear(int64_t transactionId, int64_t objectStoreId, PassRefPtr<IDBCallbacks>) OVERRIDE;

private:
    IDBDatabaseBackendImpl(const String& name, IDBBackingStore* database, IDBFactoryBackendImpl*, const String& uniqueIdentifier);

    bool openInternal();
    void runIntVersionChangeTransaction(PassRefPtr<IDBCallbacks>, PassRefPtr<IDBDatabaseCallbacks>, int64_t transactionId, int64_t requestedVersion);
    void loadObjectStores();
    size_t connectionCount();
    void processPendingCalls();

    class CreateObjectStoreOperation;
    class DeleteObjectStoreOperation;
    class VersionChangeOperation;

    // When a "versionchange" transaction aborts, these restore the back-end object hierarchy.
    class CreateObjectStoreAbortOperation;
    class DeleteObjectStoreAbortOperation;
    class VersionChangeAbortOperation;

    RefPtr<IDBBackingStore> m_backingStore;
    IDBDatabaseMetadata m_metadata;

    String m_identifier;
    // This might not need to be a RefPtr since the factory's lifetime is that of the page group, but it's better to be conservitive than sorry.
    RefPtr<IDBFactoryBackendImpl> m_factory;

    typedef HashMap<int64_t, RefPtr<IDBObjectStoreBackendImpl> > ObjectStoreMap;
    ObjectStoreMap m_objectStores;

    OwnPtr<IDBTransactionCoordinator> m_transactionCoordinator;
    RefPtr<IDBTransactionBackendImpl> m_runningVersionChangeTransaction;

    typedef HashMap<int64_t, IDBTransactionBackendImpl*> TransactionMap;
    TransactionMap m_transactions;

    class PendingOpenCall;
    Deque<OwnPtr<PendingOpenCall> > m_pendingOpenCalls;

    class PendingOpenWithVersionCall;
    Deque<OwnPtr<PendingOpenWithVersionCall> > m_pendingOpenWithVersionCalls;
    OwnPtr<PendingOpenWithVersionCall> m_pendingSecondHalfOpenWithVersion;

    class PendingDeleteCall;
    Deque<OwnPtr<PendingDeleteCall> > m_pendingDeleteCalls;

    typedef ListHashSet<RefPtr<IDBDatabaseCallbacks> > DatabaseCallbacksSet;
    DatabaseCallbacksSet m_databaseCallbacksSet;

    bool m_closingConnection;
};

} // namespace WebCore

#endif

#endif // IDBDatabaseBackendImpl_h
