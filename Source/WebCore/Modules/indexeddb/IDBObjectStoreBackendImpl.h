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

#ifndef IDBObjectStoreBackendImpl_h
#define IDBObjectStoreBackendImpl_h

#include "IDBBackingStore.h"
#include "IDBDatabaseBackendImpl.h"
#include "IDBKeyPath.h"
#include "IDBMetadata.h"
#include "IDBObjectStoreBackendInterface.h"
#include <wtf/HashMap.h>
#include <wtf/text/StringHash.h>

#if ENABLE(INDEXED_DATABASE)

namespace WebCore {

class IDBDatabaseBackendImpl;
class IDBIndexBackendImpl;
class IDBTransactionBackendImpl;
class IDBTransactionBackendInterface;
class ScriptExecutionContext;
struct IDBObjectStoreMetadata;

class IDBObjectStoreBackendImpl : public IDBObjectStoreBackendInterface {
public:
    static PassRefPtr<IDBObjectStoreBackendImpl> create(const IDBDatabaseBackendImpl* database, const IDBObjectStoreMetadata& metadata)
    {
        return adoptRef(new IDBObjectStoreBackendImpl(database, metadata));
    }
    virtual ~IDBObjectStoreBackendImpl();

    typedef HashMap<int64_t, RefPtr<IDBIndexBackendImpl> > IndexMap;

    static const int64_t InvalidId = 0;
    int64_t id() const
    {
        ASSERT(m_metadata.id != InvalidId);
        return m_metadata.id;
    }

    // IDBObjectStoreBackendInterface
    virtual void get(PassRefPtr<IDBKeyRange>, PassRefPtr<IDBCallbacks>, IDBTransactionBackendInterface*, ExceptionCode&);
    virtual void put(PassRefPtr<SerializedScriptValue>, PassRefPtr<IDBKey>, PutMode, PassRefPtr<IDBCallbacks>, IDBTransactionBackendInterface*, const Vector<int64_t>&, const Vector<IndexKeys>&);

    virtual void deleteFunction(PassRefPtr<IDBKeyRange>, PassRefPtr<IDBCallbacks>, IDBTransactionBackendInterface*, ExceptionCode&);
    virtual void clear(PassRefPtr<IDBCallbacks>, IDBTransactionBackendInterface*, ExceptionCode&);

    virtual PassRefPtr<IDBIndexBackendInterface> createIndex(int64_t, const String& name, const IDBKeyPath&, bool unique, bool multiEntry, IDBTransactionBackendInterface*, ExceptionCode&);
    virtual void setIndexKeys(PassRefPtr<IDBKey> prpPrimaryKey, const Vector<int64_t>&, const Vector<IndexKeys>&, IDBTransactionBackendInterface*);
    virtual void setIndexesReady(const Vector<int64_t>&, IDBTransactionBackendInterface*);
    virtual PassRefPtr<IDBIndexBackendInterface> index(int64_t);
    virtual void deleteIndex(int64_t, IDBTransactionBackendInterface*, ExceptionCode&);

    virtual void openCursor(PassRefPtr<IDBKeyRange>, IDBCursor::Direction, PassRefPtr<IDBCallbacks>, IDBTransactionBackendInterface::TaskType, IDBTransactionBackendInterface*, ExceptionCode&);
    virtual void count(PassRefPtr<IDBKeyRange>, PassRefPtr<IDBCallbacks>, IDBTransactionBackendInterface*, ExceptionCode&);

    static bool populateIndex(IDBBackingStore&, int64_t databaseId, int64_t objectStoreId, PassRefPtr<IDBIndexBackendImpl>);

    const IndexMap::iterator iterIndexesBegin() { return m_indexes.begin(); };
    const IndexMap::iterator iterIndexesEnd() { return m_indexes.end(); };

    IDBObjectStoreMetadata metadata() const;
    const String& name() { return m_metadata.name; }
    const IDBKeyPath& keyPath() const { return m_metadata.keyPath; }
    const bool& autoIncrement() const { return m_metadata.autoIncrement; }

    PassRefPtr<IDBBackingStore> backingStore() const { return m_database->backingStore(); }
    int64_t databaseId() const { return m_database->id(); }

private:
    IDBObjectStoreBackendImpl(const IDBDatabaseBackendImpl*, const IDBObjectStoreMetadata&);

    void loadIndexes();
    PassRefPtr<IDBKey> generateKey(PassRefPtr<IDBTransactionBackendImpl>);
    bool updateKeyGenerator(PassRefPtr<IDBTransactionBackendImpl>, const IDBKey*, bool checkCurrent);

    static void getInternal(ScriptExecutionContext*, PassRefPtr<IDBObjectStoreBackendImpl>, PassRefPtr<IDBKeyRange>, PassRefPtr<IDBCallbacks>, PassRefPtr<IDBTransactionBackendImpl>);
    static void putInternal(ScriptExecutionContext*, PassRefPtr<IDBObjectStoreBackendImpl>, PassRefPtr<SerializedScriptValue>, PassRefPtr<IDBKey>, PutMode, PassRefPtr<IDBCallbacks>, PassRefPtr<IDBTransactionBackendImpl>, PassOwnPtr<Vector<int64_t> > popIndexNames, PassOwnPtr<Vector<IndexKeys> >);
    static void deleteInternal(ScriptExecutionContext*, PassRefPtr<IDBObjectStoreBackendImpl>, PassRefPtr<IDBKeyRange>, PassRefPtr<IDBCallbacks>, PassRefPtr<IDBTransactionBackendImpl>);
    static void setIndexesReadyInternal(ScriptExecutionContext*, PassRefPtr<IDBObjectStoreBackendImpl>, PassOwnPtr<Vector<int64_t> > popIndexNames, PassRefPtr<IDBTransactionBackendImpl>);
    static void clearInternal(ScriptExecutionContext*, PassRefPtr<IDBObjectStoreBackendImpl>, PassRefPtr<IDBCallbacks>, PassRefPtr<IDBTransactionBackendImpl>);
    static void createIndexInternal(ScriptExecutionContext*, PassRefPtr<IDBObjectStoreBackendImpl>, PassRefPtr<IDBIndexBackendImpl>, PassRefPtr<IDBTransactionBackendImpl>);
    static void deleteIndexInternal(ScriptExecutionContext*, PassRefPtr<IDBObjectStoreBackendImpl>, PassRefPtr<IDBIndexBackendImpl>, PassRefPtr<IDBTransactionBackendImpl>);
    static void openCursorInternal(ScriptExecutionContext*, PassRefPtr<IDBObjectStoreBackendImpl>, PassRefPtr<IDBKeyRange>, IDBCursor::Direction, PassRefPtr<IDBCallbacks>, IDBTransactionBackendInterface::TaskType, PassRefPtr<IDBTransactionBackendImpl>);
    static void countInternal(ScriptExecutionContext*, PassRefPtr<IDBObjectStoreBackendImpl>, PassRefPtr<IDBKeyRange>, PassRefPtr<IDBCallbacks>, PassRefPtr<IDBTransactionBackendImpl>);

    // These are used as setVersion transaction abort tasks.
    static void removeIndexFromMap(ScriptExecutionContext*, PassRefPtr<IDBObjectStoreBackendImpl>, PassRefPtr<IDBIndexBackendImpl>);
    static void addIndexToMap(ScriptExecutionContext*, PassRefPtr<IDBObjectStoreBackendImpl>, PassRefPtr<IDBIndexBackendImpl>);

    const IDBDatabaseBackendImpl* m_database;

    IDBObjectStoreMetadata m_metadata;
    IndexMap m_indexes;
};

} // namespace WebCore

#endif

#endif // IDBObjectStoreBackendImpl_h
