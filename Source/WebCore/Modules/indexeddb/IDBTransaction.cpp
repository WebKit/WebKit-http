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
#include "IDBTransaction.h"

#if ENABLE(INDEXED_DATABASE)

#include "EventException.h"
#include "EventQueue.h"
#include "IDBDatabase.h"
#include "IDBDatabaseException.h"
#include "IDBEventDispatcher.h"
#include "IDBIndex.h"
#include "IDBObjectStore.h"
#include "IDBObjectStoreBackendInterface.h"
#include "IDBOpenDBRequest.h"
#include "IDBPendingTransactionMonitor.h"
#include "IDBTracing.h"
#include "ScriptCallStack.h"
#include "ScriptExecutionContext.h"

namespace WebCore {

PassRefPtr<IDBTransaction> IDBTransaction::create(ScriptExecutionContext* context, int64_t id, PassRefPtr<IDBTransactionBackendInterface> backend, const Vector<String>& objectStoreNames, IDBTransaction::Mode mode, IDBDatabase* db)
{
    IDBOpenDBRequest* openDBRequest = 0;
    return create(context, id, backend, objectStoreNames, mode, db, openDBRequest);
}

PassRefPtr<IDBTransaction> IDBTransaction::create(ScriptExecutionContext* context, int64_t id, PassRefPtr<IDBTransactionBackendInterface> backend, const Vector<String>& objectStoreNames, IDBTransaction::Mode mode, IDBDatabase* db, IDBOpenDBRequest* openDBRequest)
{
    RefPtr<IDBTransaction> transaction(adoptRef(new IDBTransaction(context, id, backend, objectStoreNames, mode, db, openDBRequest)));
    transaction->suspendIfNeeded();
    return transaction.release();
}

const AtomicString& IDBTransaction::modeReadOnly()
{
    DEFINE_STATIC_LOCAL(AtomicString, readonly, ("readonly", AtomicString::ConstructFromLiteral));
    return readonly;
}

const AtomicString& IDBTransaction::modeReadWrite()
{
    DEFINE_STATIC_LOCAL(AtomicString, readwrite, ("readwrite", AtomicString::ConstructFromLiteral));
    return readwrite;
}

const AtomicString& IDBTransaction::modeVersionChange()
{
    DEFINE_STATIC_LOCAL(AtomicString, versionchange, ("versionchange", AtomicString::ConstructFromLiteral));
    return versionchange;
}

const AtomicString& IDBTransaction::modeReadOnlyLegacy()
{
    DEFINE_STATIC_LOCAL(AtomicString, readonly, ("0", AtomicString::ConstructFromLiteral));
    return readonly;
}

const AtomicString& IDBTransaction::modeReadWriteLegacy()
{
    DEFINE_STATIC_LOCAL(AtomicString, readwrite, ("1", AtomicString::ConstructFromLiteral));
    return readwrite;
}


IDBTransaction::IDBTransaction(ScriptExecutionContext* context, int64_t id, PassRefPtr<IDBTransactionBackendInterface> backend, const Vector<String>& objectStoreNames, IDBTransaction::Mode mode, IDBDatabase* db, IDBOpenDBRequest* openDBRequest)
    : ActiveDOMObject(context, this)
    , m_backend(backend)
    , m_id(id)
    , m_database(db)
    , m_objectStoreNames(objectStoreNames)
    , m_openDBRequest(openDBRequest)
    , m_mode(mode)
    , m_state(Active)
    , m_hasPendingActivity(true)
    , m_contextStopped(false)
{
    ASSERT(m_backend);

    if (mode == VERSION_CHANGE) {
        // Not active until the callback.
        m_state = Inactive;
    }

    // We pass a reference of this object before it can be adopted.
    relaxAdoptionRequirement();
    if (m_state == Active)
        IDBPendingTransactionMonitor::addNewTransaction(this);
    m_database->transactionCreated(this);
}

IDBTransaction::~IDBTransaction()
{
    ASSERT(m_state == Finished);
}

IDBTransactionBackendInterface* IDBTransaction::backend() const
{
    return m_backend.get();
}

const String& IDBTransaction::mode() const
{
    ExceptionCode ec = 0;
    const AtomicString& mode = modeToString(m_mode, ec);
    ASSERT(!ec);
    return mode;
}

void IDBTransaction::setError(PassRefPtr<DOMError> error)
{
    ASSERT(m_state != Finished);
    ASSERT(error);

    // The first error to be set is the true cause of the
    // transaction abort.
    if (!m_error)
        m_error = error;
}

PassRefPtr<IDBObjectStore> IDBTransaction::objectStore(const String& name, ExceptionCode& ec)
{
    if (m_state == Finished) {
        ec = IDBDatabaseException::IDB_INVALID_STATE_ERR;
        return 0;
    }

    IDBObjectStoreMap::iterator it = m_objectStoreMap.find(name);
    if (it != m_objectStoreMap.end())
        return it->value;

    if (!isVersionChange() && !m_objectStoreNames.contains(name)) {
        ec = IDBDatabaseException::IDB_NOT_FOUND_ERR;
        return 0;
    }

    int64_t objectStoreId = m_database->findObjectStoreId(name);
    if (objectStoreId == IDBObjectStoreMetadata::InvalidId) {
        ASSERT(isVersionChange());
        ec = IDBDatabaseException::IDB_NOT_FOUND_ERR;
        return 0;
    }

    RefPtr<IDBObjectStoreBackendInterface> objectStoreBackend = m_backend->objectStore(objectStoreId, ec);
    ASSERT(!ec && objectStoreBackend);

    const IDBDatabaseMetadata& metadata = m_database->metadata();

    RefPtr<IDBObjectStore> objectStore = IDBObjectStore::create(metadata.objectStores.get(objectStoreId), objectStoreBackend, this);
    objectStoreCreated(name, objectStore);
    return objectStore.release();
}

void IDBTransaction::objectStoreCreated(const String& name, PassRefPtr<IDBObjectStore> prpObjectStore)
{
    ASSERT(m_state != Finished);
    RefPtr<IDBObjectStore> objectStore = prpObjectStore;
    m_objectStoreMap.set(name, objectStore);
    if (isVersionChange())
        m_objectStoreCleanupMap.set(objectStore, objectStore->metadata());
}

void IDBTransaction::objectStoreDeleted(const String& name)
{
    ASSERT(m_state != Finished);
    ASSERT(isVersionChange());
    IDBObjectStoreMap::iterator it = m_objectStoreMap.find(name);
    if (it != m_objectStoreMap.end()) {
        RefPtr<IDBObjectStore> objectStore = it->value;
        m_objectStoreMap.remove(name);
        objectStore->markDeleted();
        m_objectStoreCleanupMap.set(objectStore, objectStore->metadata());
        m_deletedObjectStores.add(objectStore);
    }
}

void IDBTransaction::setActive(bool active)
{
    ASSERT_WITH_MESSAGE(m_state != Finished, "A finished transaction tried to setActive(%s)", active ? "true" : "false");
    if (m_state == Finishing)
        return;
    ASSERT(active != (m_state == Active));
    m_state = active ? Active : Inactive;

    if (!active && m_requestList.isEmpty())
        m_backend->commit();
}

void IDBTransaction::abort(ExceptionCode& ec)
{
    if (m_state == Finishing || m_state == Finished) {
        ec = IDBDatabaseException::IDB_INVALID_STATE_ERR;
        return;
    }

    m_state = Finishing;

    while (!m_requestList.isEmpty()) {
        IDBRequest* request = *m_requestList.begin();
        m_requestList.remove(request);
        request->abort();
    }

    RefPtr<IDBTransaction> selfRef = this;
    if (m_backend)
        m_backend->abort();
}

IDBTransaction::OpenCursorNotifier::OpenCursorNotifier(PassRefPtr<IDBTransaction> transaction, IDBCursor* cursor)
    : m_transaction(transaction),
      m_cursor(cursor)
{
    m_transaction->registerOpenCursor(m_cursor);
}

IDBTransaction::OpenCursorNotifier::~OpenCursorNotifier()
{
    if (m_cursor)
        m_transaction->unregisterOpenCursor(m_cursor);
}

void IDBTransaction::OpenCursorNotifier::cursorFinished()
{
    if (m_cursor) {
        m_transaction->unregisterOpenCursor(m_cursor);
        m_cursor = 0;
        m_transaction.clear();
    }
}

void IDBTransaction::registerOpenCursor(IDBCursor* cursor)
{
    m_openCursors.add(cursor);
}

void IDBTransaction::unregisterOpenCursor(IDBCursor* cursor)
{
    m_openCursors.remove(cursor);
}

void IDBTransaction::closeOpenCursors()
{
    HashSet<IDBCursor*> cursors;
    cursors.swap(m_openCursors);
    for (HashSet<IDBCursor*>::iterator i = cursors.begin(); i != cursors.end(); ++i)
        (*i)->close();
}

void IDBTransaction::registerRequest(IDBRequest* request)
{
    ASSERT(request);
    ASSERT(m_state == Active);
    m_requestList.add(request);
}

void IDBTransaction::unregisterRequest(IDBRequest* request)
{
    ASSERT(request);
    // If we aborted the request, it will already have been removed.
    m_requestList.remove(request);
}

void IDBTransaction::onAbort(PassRefPtr<IDBDatabaseError> error)
{
    IDB_TRACE("IDBTransaction::onAbort");
    ASSERT(m_state != Finished);

    if (m_state != Finishing) {
        ASSERT(error.get());
        setError(DOMError::create(error->name()));

        // Abort was not triggered by front-end, so outstanding requests must
        // be aborted now.
        while (!m_requestList.isEmpty()) {
            IDBRequest* request = *m_requestList.begin();
            m_requestList.remove(request);
            request->abort();
        }
        m_state = Finishing;
    }

    if (isVersionChange()) {
        for (IDBObjectStoreMetadataMap::iterator it = m_objectStoreCleanupMap.begin(); it != m_objectStoreCleanupMap.end(); ++it)
            it->key->setMetadata(it->value);
    }
    m_objectStoreCleanupMap.clear();
    closeOpenCursors();

    // Enqueue events before notifying database, as database may close which enqueues more events and order matters.
    enqueueEvent(Event::create(eventNames().abortEvent, true, false));
    m_database->transactionFinished(this);
}

void IDBTransaction::onComplete()
{
    IDB_TRACE("IDBTransaction::onComplete");
    ASSERT(m_state != Finished);
    m_state = Finishing;
    m_objectStoreCleanupMap.clear();
    closeOpenCursors();

    // Enqueue events before notifying database, as database may close which enqueues more events and order matters.
    enqueueEvent(Event::create(eventNames().completeEvent, false, false));
    m_database->transactionFinished(this);
}

bool IDBTransaction::hasPendingActivity() const
{
    // FIXME: In an ideal world, we should return true as long as anyone has a or can
    //        get a handle to us or any child request object and any of those have
    //        event listeners. This is  in order to handle user generated events properly.
    return m_hasPendingActivity || ActiveDOMObject::hasPendingActivity();
}

IDBTransaction::Mode IDBTransaction::stringToMode(const String& modeString, ScriptExecutionContext* context, ExceptionCode& ec)
{
    if (modeString.isNull()
        || modeString == IDBTransaction::modeReadOnly())
        return IDBTransaction::READ_ONLY;
    if (modeString == IDBTransaction::modeReadWrite())
        return IDBTransaction::READ_WRITE;

    ec = TypeError;
    return IDBTransaction::READ_ONLY;
}

const AtomicString& IDBTransaction::modeToString(IDBTransaction::Mode mode, ExceptionCode& ec)
{
    switch (mode) {
    case IDBTransaction::READ_ONLY:
        return IDBTransaction::modeReadOnly();
        break;

    case IDBTransaction::READ_WRITE:
        return IDBTransaction::modeReadWrite();
        break;

    case IDBTransaction::VERSION_CHANGE:
        return IDBTransaction::modeVersionChange();
        break;

    default:
        ec = TypeError;
        return IDBTransaction::modeReadOnly();
    }
}

const AtomicString& IDBTransaction::interfaceName() const
{
    return eventNames().interfaceForIDBTransaction;
}

ScriptExecutionContext* IDBTransaction::scriptExecutionContext() const
{
    return ActiveDOMObject::scriptExecutionContext();
}

bool IDBTransaction::dispatchEvent(PassRefPtr<Event> event)
{
    IDB_TRACE("IDBTransaction::dispatchEvent");
    ASSERT(m_state != Finished);
    ASSERT(m_hasPendingActivity);
    ASSERT(scriptExecutionContext());
    ASSERT(event->target() == this);
    m_state = Finished;

    // Break reference cycles.
    for (IDBObjectStoreMap::iterator it = m_objectStoreMap.begin(); it != m_objectStoreMap.end(); ++it)
        it->value->transactionFinished();
    m_objectStoreMap.clear();
    for (IDBObjectStoreSet::iterator it = m_deletedObjectStores.begin(); it != m_deletedObjectStores.end(); ++it)
        (*it)->transactionFinished();
    m_deletedObjectStores.clear();

    Vector<RefPtr<EventTarget> > targets;
    targets.append(this);
    targets.append(db());

    // FIXME: When we allow custom event dispatching, this will probably need to change.
    ASSERT(event->type() == eventNames().completeEvent || event->type() == eventNames().abortEvent);
    bool returnValue = IDBEventDispatcher::dispatch(event.get(), targets);
    // FIXME: Try to construct a test where |this| outlives openDBRequest and we
    // get a crash.
    if (m_openDBRequest) {
        ASSERT(isVersionChange());
        m_openDBRequest->transactionDidFinishAndDispatch();
    }
    m_hasPendingActivity = false;
    return returnValue;
}

bool IDBTransaction::canSuspend() const
{
    // FIXME: Technically we can suspend before the first request is schedule
    //        and after the complete/abort event is enqueued.
    return m_state == Finished;
}

void IDBTransaction::stop()
{
    ActiveDOMObject::stop();
    m_contextStopped = true;

    ExceptionCode unused;
    abort(unused);
}

void IDBTransaction::enqueueEvent(PassRefPtr<Event> event)
{
    ASSERT_WITH_MESSAGE(m_state != Finished, "A finished transaction tried to enqueue an event of type %s.", event->type().string().utf8().data());
    if (m_contextStopped || !scriptExecutionContext())
        return;

    EventQueue* eventQueue = scriptExecutionContext()->eventQueue();
    event->setTarget(this);
    eventQueue->enqueueEvent(event);
}

EventTargetData* IDBTransaction::eventTargetData()
{
    return &m_eventTargetData;
}

EventTargetData* IDBTransaction::ensureEventTargetData()
{
    return &m_eventTargetData;
}

}

#endif // ENABLE(INDEXED_DATABASE)
