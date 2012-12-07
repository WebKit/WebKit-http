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

#ifndef IDBTransactionBackendImpl_h
#define IDBTransactionBackendImpl_h

#if ENABLE(INDEXED_DATABASE)

#include "IDBBackingStore.h"
#include "IDBDatabaseError.h"
#include "IDBTransaction.h"
#include "IDBTransactionBackendInterface.h"
#include "IDBTransactionCallbacks.h"
#include "Timer.h"
#include <wtf/Deque.h>
#include <wtf/HashSet.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class IDBDatabaseBackendImpl;

class IDBTransactionBackendImpl : public IDBTransactionBackendInterface {
public:
    static PassRefPtr<IDBTransactionBackendImpl> create(int64_t transactionId, const Vector<int64_t>&, IDBTransaction::Mode, IDBDatabaseBackendImpl*);
    static IDBTransactionBackendImpl* from(IDBTransactionBackendInterface* interface)
    {
        return static_cast<IDBTransactionBackendImpl*>(interface);
    }
    virtual ~IDBTransactionBackendImpl();

    // IDBTransactionBackendInterface
    virtual PassRefPtr<IDBObjectStoreBackendInterface> objectStore(int64_t, ExceptionCode&);
    virtual void abort();
    void commit();
    virtual void setCallbacks(IDBTransactionCallbacks* callbacks) { m_callbacks = callbacks; }

    class Operation {
    public:
        virtual ~Operation() { }
        virtual void perform(IDBTransactionBackendImpl*) = 0;
    };

    void abort(PassRefPtr<IDBDatabaseError>);
    void run();
    IDBTransaction::Mode mode() const { return m_mode; }
    const HashSet<int64_t>& scope() const { return m_objectStoreIds; }
    bool isFinished() const { return m_state == Finished; }
    bool scheduleTask(PassOwnPtr<Operation> task, PassOwnPtr<Operation> abortTask = nullptr) { return scheduleTask(NormalTask, task, abortTask); }
    bool scheduleTask(TaskType, PassOwnPtr<Operation>, PassOwnPtr<Operation> abortTask = nullptr);
    void registerOpenCursor(IDBCursorBackendImpl*);
    void unregisterOpenCursor(IDBCursorBackendImpl*);
    void addPreemptiveEvent() { m_pendingPreemptiveEvents++; }
    void didCompletePreemptiveEvent() { m_pendingPreemptiveEvents--; ASSERT(m_pendingPreemptiveEvents >= 0); }
    IDBBackingStore::Transaction* backingStoreTransaction() { return &m_transaction; }
    int64_t id() const { return m_id; }

private:
    IDBTransactionBackendImpl(int64_t id, const HashSet<int64_t>& objectStoreIds, IDBTransaction::Mode, IDBDatabaseBackendImpl*);

    enum State {
        Unused, // Created, but no tasks yet.
        StartPending, // Enqueued tasks, but backing store transaction not yet started.
        Running, // Backing store transaction started but not yet finished.
        Finished, // Either aborted or committed.
    };

    void start();

    bool isTaskQueueEmpty() const;
    bool hasPendingTasks() const;

    void taskTimerFired(Timer<IDBTransactionBackendImpl>*);
    void closeOpenCursors();

    const int64_t m_id;
    const HashSet<int64_t> m_objectStoreIds;
    const IDBTransaction::Mode m_mode;

    State m_state;
    bool m_commitPending;
    RefPtr<IDBTransactionCallbacks> m_callbacks;
    RefPtr<IDBDatabaseBackendImpl> m_database;

    typedef Deque<OwnPtr<Operation> > TaskQueue;
    TaskQueue m_taskQueue;
    TaskQueue m_preemptiveTaskQueue;
    TaskQueue m_abortTaskQueue;

    IDBBackingStore::Transaction m_transaction;

    // FIXME: delete the timer once we have threads instead.
    Timer<IDBTransactionBackendImpl> m_taskTimer;
    int m_pendingPreemptiveEvents;

    HashSet<IDBCursorBackendImpl*> m_openCursors;
};

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)

#endif // IDBTransactionBackendImpl_h
