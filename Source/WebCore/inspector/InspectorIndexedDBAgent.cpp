/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(INSPECTOR) && ENABLE(INDEXED_DATABASE)

#include "InspectorIndexedDBAgent.h"

#include "DOMStringList.h"
#include "DOMWindow.h"
#include "Document.h"
#include "ExceptionCode.h"
#include "Frame.h"
#include "GroupSettings.h"
#include "IDBCallbacks.h"
#include "IDBCursor.h"
#include "IDBCursorBackendInterface.h"
#include "IDBDatabaseBackendInterface.h"
#include "IDBDatabaseCallbacks.h"
#include "IDBFactoryBackendInterface.h"
#include "IDBIndexBackendInterface.h"
#include "IDBKey.h"
#include "IDBKeyPath.h"
#include "IDBKeyRange.h"
#include "IDBMetadata.h"
#include "IDBObjectStoreBackendInterface.h"
#include "IDBPendingTransactionMonitor.h"
#include "IDBTransaction.h"
#include "IDBTransactionBackendInterface.h"
#include "InjectedScript.h"
#include "InspectorFrontend.h"
#include "InspectorPageAgent.h"
#include "InspectorState.h"
#include "InspectorValues.h"
#include "InstrumentingAgents.h"
#include "Page.h"
#include "PageGroup.h"
#include "PageGroupIndexedDatabase.h"
#include "SecurityOrigin.h"

#include <wtf/Vector.h>

using WebCore::TypeBuilder::Array;
using WebCore::TypeBuilder::IndexedDB::SecurityOriginWithDatabaseNames;
using WebCore::TypeBuilder::IndexedDB::DatabaseWithObjectStores;
using WebCore::TypeBuilder::IndexedDB::DataEntry;
using WebCore::TypeBuilder::IndexedDB::Key;
using WebCore::TypeBuilder::IndexedDB::KeyPath;
using WebCore::TypeBuilder::IndexedDB::KeyRange;
using WebCore::TypeBuilder::IndexedDB::ObjectStore;
using WebCore::TypeBuilder::IndexedDB::ObjectStoreIndex;

typedef WebCore::InspectorBackendDispatcher::IndexedDBCommandHandler::RequestDatabaseNamesForFrameCallback RequestDatabaseNamesForFrameCallback;
typedef WebCore::InspectorBackendDispatcher::IndexedDBCommandHandler::RequestDatabaseCallback RequestDatabaseCallback;
typedef WebCore::InspectorBackendDispatcher::IndexedDBCommandHandler::RequestDataCallback RequestDataCallback;

namespace WebCore {

namespace IndexedDBAgentState {
static const char indexedDBAgentEnabled[] = "indexedDBAgentEnabled";
};

namespace {

class InspectorIDBCallback : public IDBCallbacks {
public:
    virtual ~InspectorIDBCallback() { }

    virtual void onError(PassRefPtr<IDBDatabaseError>) OVERRIDE { }
    virtual void onSuccess(PassRefPtr<DOMStringList>) OVERRIDE { }
    virtual void onSuccess(PassRefPtr<IDBCursorBackendInterface>, PassRefPtr<IDBKey>, PassRefPtr<IDBKey>, PassRefPtr<SerializedScriptValue>) OVERRIDE { }
    virtual void onSuccess(PassRefPtr<IDBDatabaseBackendInterface>) OVERRIDE { }
    virtual void onSuccess(PassRefPtr<IDBKey>) OVERRIDE { }
    virtual void onSuccess(PassRefPtr<IDBTransactionBackendInterface>) OVERRIDE { }
    virtual void onSuccess(PassRefPtr<SerializedScriptValue>) OVERRIDE { }
    virtual void onSuccess(PassRefPtr<SerializedScriptValue>, PassRefPtr<IDBKey>, const IDBKeyPath&) OVERRIDE { }
    virtual void onSuccess(PassRefPtr<IDBKey>, PassRefPtr<IDBKey>, PassRefPtr<SerializedScriptValue>) OVERRIDE { }
    virtual void onSuccessWithPrefetch(const Vector<RefPtr<IDBKey> >&, const Vector<RefPtr<IDBKey> >&, const Vector<RefPtr<SerializedScriptValue> >&) OVERRIDE { }
    virtual void onBlocked() OVERRIDE { }
};

class InspectorIDBDatabaseCallbacks : public IDBDatabaseCallbacks {
public:
    static PassRefPtr<InspectorIDBDatabaseCallbacks> create()
    {
        return adoptRef(new InspectorIDBDatabaseCallbacks());
    }

    virtual ~InspectorIDBDatabaseCallbacks() { }

    virtual void onVersionChange(const String& version) { }
    virtual void onVersionChange(int64_t oldVersion, int64_t newVersion) { }
    virtual void onForcedClose() { }
private:
    InspectorIDBDatabaseCallbacks() { }
};


class InspectorIDBTransactionCallback : public IDBTransactionCallbacks {
public:
    static PassRefPtr<InspectorIDBTransactionCallback> create()
    {
        return adoptRef(new InspectorIDBTransactionCallback());
    }

    virtual ~InspectorIDBTransactionCallback() { }

    virtual void onAbort() { }
    virtual void onComplete() { }
private:
    InspectorIDBTransactionCallback() { }
};

class GetDatabaseNamesCallback : public InspectorIDBCallback {
public:
    static PassRefPtr<GetDatabaseNamesCallback> create(PassRefPtr<RequestDatabaseNamesForFrameCallback> requestCallback, const String& securityOrigin)
    {
        return adoptRef(new GetDatabaseNamesCallback(requestCallback, securityOrigin));
    }

    virtual ~GetDatabaseNamesCallback() { }

    virtual void onSuccess(PassRefPtr<DOMStringList> databaseNamesList)
    {
        if (!m_requestCallback->isActive())
            return;

        RefPtr<TypeBuilder::Array<String> > databaseNames = TypeBuilder::Array<String>::create();
        for (size_t i = 0; i < databaseNamesList->length(); ++i)
            databaseNames->addItem(databaseNamesList->item(i));

        RefPtr<SecurityOriginWithDatabaseNames> result = SecurityOriginWithDatabaseNames::create()
            .setSecurityOrigin(m_securityOrigin)
            .setDatabaseNames(databaseNames);

        m_requestCallback->sendSuccess(result);
    }

private:
    GetDatabaseNamesCallback(PassRefPtr<RequestDatabaseNamesForFrameCallback> requestCallback, const String& securityOrigin)
        : m_requestCallback(requestCallback)
        , m_securityOrigin(securityOrigin) { }
    RefPtr<RequestDatabaseNamesForFrameCallback> m_requestCallback;
    String m_securityOrigin;
};

class DatabaseConnection {
public:
    DatabaseConnection()
        : m_idbDatabaseCallbacks(InspectorIDBDatabaseCallbacks::create()) { }

    ~DatabaseConnection()
    {
        if (m_idbDatabase)
            m_idbDatabase->close(m_idbDatabaseCallbacks);
    }

    void connect(PassRefPtr<IDBDatabaseBackendInterface> database) { m_idbDatabase = database; }
    PassRefPtr<IDBDatabaseCallbacks> callbacks() { return m_idbDatabaseCallbacks; }

private:
    RefPtr<IDBDatabaseBackendInterface> m_idbDatabase;
    RefPtr<IDBDatabaseCallbacks> m_idbDatabaseCallbacks;
};

class ExecutableWithDatabase : public RefCounted<ExecutableWithDatabase> {
public:
    virtual ~ExecutableWithDatabase() { };
    void start(IDBFactoryBackendInterface*, SecurityOrigin*, ScriptExecutionContext*, const String& databaseName);
    void connect(PassRefPtr<IDBDatabaseBackendInterface> database) { m_connection.connect(database); }
    virtual void execute(PassRefPtr<IDBDatabaseBackendInterface>) = 0;
private:
    DatabaseConnection m_connection;
};

class OpenDatabaseCallback : public InspectorIDBCallback {
public:
    static PassRefPtr<OpenDatabaseCallback> create(ExecutableWithDatabase* executableWithDatabase)
    {
        return adoptRef(new OpenDatabaseCallback(executableWithDatabase));
    }

    virtual ~OpenDatabaseCallback() { }

    virtual void onSuccess(PassRefPtr<IDBDatabaseBackendInterface> prpDatabase)
    {
        RefPtr<IDBDatabaseBackendInterface> idbDatabase = prpDatabase;
        m_executableWithDatabase->connect(idbDatabase);
        m_executableWithDatabase->execute(idbDatabase);
    }

private:
    OpenDatabaseCallback(ExecutableWithDatabase* executableWithDatabase)
        : m_executableWithDatabase(executableWithDatabase) { }
    RefPtr<ExecutableWithDatabase> m_executableWithDatabase;
};

void ExecutableWithDatabase::start(IDBFactoryBackendInterface* idbFactory, SecurityOrigin* securityOrigin, ScriptExecutionContext* context, const String& databaseName)
{
    RefPtr<OpenDatabaseCallback> callback = OpenDatabaseCallback::create(this);
    idbFactory->open(databaseName, IDBDatabaseMetadata::NoIntVersion, callback, m_connection.callbacks(), securityOrigin, context, String());
}

static PassRefPtr<IDBTransactionBackendInterface> transactionForDatabase(IDBDatabaseBackendInterface* idbDatabase, const String& objectStoreName)
{
    ExceptionCode ec = 0;
    RefPtr<DOMStringList> transactionObjectStoreNamesList = DOMStringList::create();
    transactionObjectStoreNamesList->append(objectStoreName);
    RefPtr<IDBTransactionBackendInterface> idbTransaction = idbDatabase->transaction(transactionObjectStoreNamesList.get(), IDBTransaction::READ_ONLY, ec);
    if (ec)
        return 0;
    return idbTransaction;
}

static PassRefPtr<IDBObjectStoreBackendInterface> objectStoreForTransaction(IDBTransactionBackendInterface* idbTransaction, const String& objectStoreName)
{
    ExceptionCode ec = 0;
    RefPtr<IDBObjectStoreBackendInterface> idbObjectStore = idbTransaction->objectStore(objectStoreName, ec);
    if (ec)
        return 0;
    return idbObjectStore;
}

static PassRefPtr<IDBIndexBackendInterface> indexForObjectStore(IDBObjectStoreBackendInterface* idbObjectStore, const String& indexName)
{
    ExceptionCode ec = 0;
    RefPtr<IDBIndexBackendInterface> idbIndex = idbObjectStore->index(indexName, ec);
    if (ec)
        return 0;
    return idbIndex;
}

static PassRefPtr<KeyPath> keyPathFromIDBKeyPath(const IDBKeyPath& idbKeyPath)
{
    RefPtr<KeyPath> keyPath;
    switch (idbKeyPath.type()) {
    case IDBKeyPath::NullType:
        keyPath = KeyPath::create().setType(KeyPath::Type::Null);
        break;
    case IDBKeyPath::StringType:
        keyPath = KeyPath::create().setType(KeyPath::Type::String);
        keyPath->setString(idbKeyPath.string());
        break;
    case IDBKeyPath::ArrayType: {
        keyPath = KeyPath::create().setType(KeyPath::Type::Array);
        RefPtr<TypeBuilder::Array<String> > array = TypeBuilder::Array<String>::create();
        const Vector<String>& stringArray = idbKeyPath.array();
        for (size_t i = 0; i < stringArray.size(); ++i)
            array->addItem(stringArray[i]);
        keyPath->setArray(array);
        break;
    }
    default:
        ASSERT_NOT_REACHED();
    }

    return keyPath.release();
}

class DatabaseLoaderCallback : public ExecutableWithDatabase {
public:
    static PassRefPtr<DatabaseLoaderCallback> create(PassRefPtr<RequestDatabaseCallback> requestCallback)
    {
        return adoptRef(new DatabaseLoaderCallback(requestCallback));
    }

    virtual ~DatabaseLoaderCallback() { }

    virtual void execute(PassRefPtr<IDBDatabaseBackendInterface> prpDatabase)
    {
        RefPtr<IDBDatabaseBackendInterface> idbDatabase = prpDatabase;
        if (!m_requestCallback->isActive())
            return;

        const IDBDatabaseMetadata databaseMetadata = idbDatabase->metadata();

        RefPtr<TypeBuilder::Array<TypeBuilder::IndexedDB::ObjectStore> > objectStores = TypeBuilder::Array<TypeBuilder::IndexedDB::ObjectStore>::create();

        for (IDBDatabaseMetadata::ObjectStoreMap::const_iterator it = databaseMetadata.objectStores.begin(); it != databaseMetadata.objectStores.end(); ++it) {
            const IDBObjectStoreMetadata& objectStoreMetadata = it->second;

            RefPtr<TypeBuilder::Array<TypeBuilder::IndexedDB::ObjectStoreIndex> > indexes = TypeBuilder::Array<TypeBuilder::IndexedDB::ObjectStoreIndex>::create();

            for (IDBObjectStoreMetadata::IndexMap::const_iterator it = objectStoreMetadata.indexes.begin(); it != objectStoreMetadata.indexes.end(); ++it) {
                const IDBIndexMetadata& indexMetadata = it->second;

                RefPtr<ObjectStoreIndex> objectStoreIndex = ObjectStoreIndex::create()
                    .setName(indexMetadata.name)
                    .setKeyPath(keyPathFromIDBKeyPath(indexMetadata.keyPath))
                    .setUnique(indexMetadata.unique)
                    .setMultiEntry(indexMetadata.multiEntry);
                indexes->addItem(objectStoreIndex);
            }

            RefPtr<ObjectStore> objectStore = ObjectStore::create()
                .setName(objectStoreMetadata.name)
                .setKeyPath(keyPathFromIDBKeyPath(objectStoreMetadata.keyPath))
                .setAutoIncrement(objectStoreMetadata.autoIncrement)
                .setIndexes(indexes);
            objectStores->addItem(objectStore);
        }
        RefPtr<DatabaseWithObjectStores> result = DatabaseWithObjectStores::create()
            .setName(databaseMetadata.name)
            .setVersion(databaseMetadata.version)
            .setObjectStores(objectStores);

        m_requestCallback->sendSuccess(result);
    }

private:
    DatabaseLoaderCallback(PassRefPtr<RequestDatabaseCallback> requestCallback)
        : m_requestCallback(requestCallback) { }
    RefPtr<RequestDatabaseCallback> m_requestCallback;
};

static PassRefPtr<IDBKey> idbKeyFromInspectorObject(InspectorObject* key)
{
    RefPtr<IDBKey> idbKey;

    String type;
    if (!key->getString("type", &type))
        return 0;

    DEFINE_STATIC_LOCAL(String, number, (ASCIILiteral("number")));
    DEFINE_STATIC_LOCAL(String, string, (ASCIILiteral("string")));
    DEFINE_STATIC_LOCAL(String, date, (ASCIILiteral("date")));
    DEFINE_STATIC_LOCAL(String, array, (ASCIILiteral("array")));

    if (type == number) {
        double number;
        if (!key->getNumber("number", &number))
            return 0;
        idbKey = IDBKey::createNumber(number);
    } else if (type == string) {
        String string;
        if (!key->getString("string", &string))
            return 0;
        idbKey = IDBKey::createString(string);
    } else if (type == date) {
        double date;
        if (!key->getNumber("date", &date))
            return 0;
        idbKey = IDBKey::createDate(date);
    } else if (type == array) {
        IDBKey::KeyArray keyArray;
        RefPtr<InspectorArray> array = key->getArray("array");
        for (size_t i = 0; i < array->length(); ++i) {
            RefPtr<InspectorValue> value = array->get(i);
            RefPtr<InspectorObject> object;
            if (!value->asObject(&object))
                return 0;
            keyArray.append(idbKeyFromInspectorObject(object.get()));
        }
        idbKey = IDBKey::createArray(keyArray);
    } else
        return 0;

    return idbKey.release();
}

static PassRefPtr<IDBKeyRange> idbKeyRangeFromKeyRange(InspectorObject* keyRange)
{
    RefPtr<InspectorObject> lower = keyRange->getObject("lower");
    RefPtr<IDBKey> idbLower = lower ? idbKeyFromInspectorObject(lower.get()) : 0;
    if (lower && !idbLower)
        return 0;

    RefPtr<InspectorObject> upper = keyRange->getObject("upper");
    RefPtr<IDBKey> idbUpper = upper ? idbKeyFromInspectorObject(upper.get()) : 0;
    if (upper && !idbUpper)
        return 0;

    bool lowerOpen;
    if (!keyRange->getBoolean("lowerOpen", &lowerOpen))
        return 0;
    IDBKeyRange::LowerBoundType lowerBoundType = lowerOpen ? IDBKeyRange::LowerBoundOpen : IDBKeyRange::LowerBoundClosed;

    bool upperOpen;
    if (!keyRange->getBoolean("upperOpen", &upperOpen))
        return 0;
    IDBKeyRange::UpperBoundType upperBoundType = upperOpen ? IDBKeyRange::UpperBoundOpen : IDBKeyRange::UpperBoundClosed;

    RefPtr<IDBKeyRange> idbKeyRange = IDBKeyRange::create(idbLower, idbUpper, lowerBoundType, upperBoundType);
    return idbKeyRange.release();
}

static PassRefPtr<Key> keyFromIDBKey(IDBKey* idbKey)
{
    if (!idbKey || !idbKey->isValid())
        return 0;

    RefPtr<Key> key;
    switch (idbKey->type()) {
    case IDBKey::InvalidType:
    case IDBKey::MinType:
        return 0;
    case IDBKey::NumberType: {
        RefPtr<Key> tmpKey = Key::create().setType(Key::Type::Number);
        key = tmpKey;
        key->setNumber(idbKey->number());
        break;
    }
    case IDBKey::StringType: {
        RefPtr<Key> tmpKey = Key::create().setType(Key::Type::String);
        key = tmpKey;
        key->setString(idbKey->string());
        break;
    }
    case IDBKey::DateType: {
        RefPtr<Key> tmpKey = Key::create().setType(Key::Type::Date);
        key = tmpKey;
        key->setDate(idbKey->date());
        break;
    }
    case IDBKey::ArrayType: {
        RefPtr<Key> tmpKey = Key::create().setType(Key::Type::Array);
        key = tmpKey;
        RefPtr<TypeBuilder::Array<TypeBuilder::IndexedDB::Key> > array = TypeBuilder::Array<TypeBuilder::IndexedDB::Key>::create();
        IDBKey::KeyArray keyArray = idbKey->array();
        for (size_t i = 0; i < keyArray.size(); ++i)
            array->addItem(keyFromIDBKey(keyArray[i].get()));
        key->setArray(array);
        break;
    }
    }
    return key.release();
}

class DataLoaderCallback;

class OpenCursorCallback : public InspectorIDBCallback {
public:
    static PassRefPtr<OpenCursorCallback> create(InjectedScript injectedScript, PassRefPtr<DataLoaderCallback> dataLoaderCallback, PassRefPtr<IDBTransactionBackendInterface> idbTransaction, PassRefPtr<RequestDataCallback> requestCallback, int skipCount, unsigned pageSize)
    {
        return adoptRef(new OpenCursorCallback(injectedScript, dataLoaderCallback, idbTransaction, requestCallback, skipCount, pageSize));
    }

    virtual ~OpenCursorCallback() { }

    virtual void onSuccess(PassRefPtr<SerializedScriptValue>)
    {
        end(false);
    }

    virtual void onSuccess(PassRefPtr<IDBCursorBackendInterface> idbCursor, PassRefPtr<IDBKey> key, PassRefPtr<IDBKey> primaryKey, PassRefPtr<SerializedScriptValue> value)
    {
        m_idbCursor = idbCursor;
        onSuccess(key, primaryKey, value);
    }

    virtual void onSuccess(PassRefPtr<IDBKey> key, PassRefPtr<IDBKey> primaryKey, PassRefPtr<SerializedScriptValue> value)
    {
        if (m_skipCount) {
            --m_skipCount;
            next();
            return;
        }

        if (m_result->length() == m_pageSize) {
            end(true);
            return;
        }

        RefPtr<TypeBuilder::Runtime::RemoteObject> wrappedValue = m_injectedScript.wrapSerializedObject(value.get(), String());
        RefPtr<DataEntry> dataEntry = DataEntry::create()
            .setKey(keyFromIDBKey(key.get()))
            .setPrimaryKey(keyFromIDBKey(primaryKey.get()))
            .setValue(wrappedValue);
        m_result->addItem(dataEntry);

        next();
    }

    void next()
    {
        ExceptionCode ec = 0;
        m_idbCursor->continueFunction(0, this, ec);
        m_idbCursor->postSuccessHandlerCallback();
        m_idbTransaction->didCompleteTaskEvents();
    }

    void end(bool hasMore)
    {
        m_dataLoaderCallback.clear();
        if (!m_requestCallback->isActive())
            return;

        if (m_idbCursor)
            m_idbCursor->postSuccessHandlerCallback();
        m_idbTransaction->didCompleteTaskEvents();

        m_requestCallback->sendSuccess(m_result.release(), hasMore);
    }

private:
    OpenCursorCallback(InjectedScript injectedScript, PassRefPtr<DataLoaderCallback> dataLoaderCallback, PassRefPtr<IDBTransactionBackendInterface> idbTransaction, PassRefPtr<RequestDataCallback> requestCallback, int skipCount, unsigned pageSize)
        : m_injectedScript(injectedScript)
        , m_dataLoaderCallback(dataLoaderCallback)
        , m_idbTransaction(idbTransaction)
        , m_requestCallback(requestCallback)
        , m_skipCount(skipCount)
        , m_pageSize(pageSize)
    {
        m_result = Array<DataEntry>::create();
        m_idbTransaction->setCallbacks(InspectorIDBTransactionCallback::create().get());
    }
    InjectedScript m_injectedScript;
    RefPtr<DataLoaderCallback> m_dataLoaderCallback;
    RefPtr<IDBTransactionBackendInterface> m_idbTransaction;
    RefPtr<RequestDataCallback> m_requestCallback;
    int m_skipCount;
    unsigned m_pageSize;
    RefPtr<Array<DataEntry> > m_result;
    RefPtr<IDBCursorBackendInterface> m_idbCursor;
};

class DataLoaderCallback : public ExecutableWithDatabase {
public:
    static PassRefPtr<DataLoaderCallback> create(PassRefPtr<RequestDataCallback> requestCallback, const InjectedScript& injectedScript, const String& objectStoreName, const String& indexName, PassRefPtr<IDBKeyRange> idbKeyRange, int skipCount, unsigned pageSize)
    {
        return adoptRef(new DataLoaderCallback(requestCallback, injectedScript, objectStoreName, indexName, idbKeyRange, skipCount, pageSize));
    }

    virtual ~DataLoaderCallback() { }

    virtual void execute(PassRefPtr<IDBDatabaseBackendInterface> prpDatabase)
    {
        RefPtr<IDBDatabaseBackendInterface> idbDatabase = prpDatabase;
        if (!m_requestCallback->isActive())
            return;

        RefPtr<IDBTransactionBackendInterface> idbTransaction = transactionForDatabase(idbDatabase.get(), m_objectStoreName);
        if (!idbTransaction)
            return;
        RefPtr<IDBObjectStoreBackendInterface> idbObjectStore = objectStoreForTransaction(idbTransaction.get(), m_objectStoreName);
        if (!idbObjectStore)
            return;

        RefPtr<OpenCursorCallback> openCursorCallback = OpenCursorCallback::create(m_injectedScript, this, idbTransaction.get(), m_requestCallback, m_skipCount, m_pageSize);

        if (!m_indexName.isEmpty()) {
            RefPtr<IDBIndexBackendInterface> idbIndex = indexForObjectStore(idbObjectStore.get(), m_indexName);
            if (!idbIndex)
                return;

            ExceptionCode ec = 0;
            idbIndex->openCursor(m_idbKeyRange, IDBCursor::NEXT, openCursorCallback, idbTransaction.get(), ec);
        } else {
            ExceptionCode ec = 0;
            idbObjectStore->openCursor(m_idbKeyRange, IDBCursor::NEXT, openCursorCallback, IDBTransactionBackendInterface::NormalTask, idbTransaction.get(), ec);
        }
    }

private:
    DataLoaderCallback(PassRefPtr<RequestDataCallback> requestCallback, const InjectedScript& injectedScript, const String& objectStoreName, const String& indexName, PassRefPtr<IDBKeyRange> idbKeyRange, int skipCount, unsigned pageSize)
        : m_requestCallback(requestCallback)
        , m_injectedScript(injectedScript)
        , m_objectStoreName(objectStoreName)
        , m_indexName(indexName)
        , m_idbKeyRange(idbKeyRange)
        , m_skipCount(skipCount)
        , m_pageSize(pageSize) { }
    RefPtr<RequestDataCallback> m_requestCallback;
    InjectedScript m_injectedScript;
    String m_objectStoreName;
    String m_indexName;
    RefPtr<IDBKeyRange> m_idbKeyRange;
    int m_skipCount;
    unsigned m_pageSize;
};

} // namespace

InspectorIndexedDBAgent::InspectorIndexedDBAgent(InstrumentingAgents* instrumentingAgents, InspectorState* state, InjectedScriptManager* injectedScriptManager, InspectorPageAgent* pageAgent)
    : InspectorBaseAgent<InspectorIndexedDBAgent>("IndexedDB", instrumentingAgents, state)
    , m_injectedScriptManager(injectedScriptManager)
    , m_pageAgent(pageAgent)
{
}

InspectorIndexedDBAgent::~InspectorIndexedDBAgent()
{
}

void InspectorIndexedDBAgent::clearFrontend()
{
    disable(0);
}

void InspectorIndexedDBAgent::restore()
{
    if (m_state->getBoolean(IndexedDBAgentState::indexedDBAgentEnabled)) {
        ErrorString error;
        enable(&error);
    }
}

void InspectorIndexedDBAgent::enable(ErrorString*)
{
    m_state->setBoolean(IndexedDBAgentState::indexedDBAgentEnabled, true);
}

void InspectorIndexedDBAgent::disable(ErrorString*)
{
    m_state->setBoolean(IndexedDBAgentState::indexedDBAgentEnabled, false);
}

static Frame* assertFrame(ErrorString* errorString, const String& frameId, InspectorPageAgent* pageAgent)
{
    Frame* frame = pageAgent->frameForId(frameId);

    if (!frame)
        *errorString = "Frame not found";

    return frame;
}

static Document* assertDocument(ErrorString* errorString, Frame* frame)
{
    Document* document = frame ? frame->document() : 0;

    if (!document)
        *errorString = "No document for given frame found";

    return document;
}

static Document* assertDocument(ErrorString* errorString, const String& frameId, InspectorPageAgent* pageAgent)
{
    Frame* frame = pageAgent->frameForId(frameId);
    Document* document = frame ? frame->document() : 0;

    if (!document)
        *errorString = "No document for given frame found";

    return document;
}

static IDBFactoryBackendInterface* assertIDBFactory(ErrorString* errorString, Document* document)
{
    Page* page = document ? document->page() : 0;
    IDBFactoryBackendInterface* idbFactory = page ? PageGroupIndexedDatabase::from(page->group())->factoryBackend() : 0;

    if (!idbFactory)
        *errorString = "No IndexedDB factory for given frame found";

    return idbFactory;
}

void InspectorIndexedDBAgent::requestDatabaseNamesForFrame(ErrorString* errorString, const String& frameId, PassRefPtr<RequestDatabaseNamesForFrameCallback> requestCallback)
{
    Document* document = assertDocument(errorString, frameId, m_pageAgent);
    if (!document)
        return;
    IDBFactoryBackendInterface* idbFactory = assertIDBFactory(errorString, document);
    if (!idbFactory)
        return;

    RefPtr<GetDatabaseNamesCallback> callback = GetDatabaseNamesCallback::create(requestCallback, document->securityOrigin()->toString());
    GroupSettings* groupSettings = document->page()->group().groupSettings();
    idbFactory->getDatabaseNames(callback.get(), document->securityOrigin(), document, groupSettings->indexedDBDatabasePath());
}

void InspectorIndexedDBAgent::requestDatabase(ErrorString* errorString, const String& frameId, const String& databaseName, PassRefPtr<RequestDatabaseCallback> requestCallback)
{
    Document* document = assertDocument(errorString, frameId, m_pageAgent);
    if (!document)
        return;
    IDBFactoryBackendInterface* idbFactory = assertIDBFactory(errorString, document);
    if (!idbFactory)
        return;

    RefPtr<DatabaseLoaderCallback> databaseLoaderCallback = DatabaseLoaderCallback::create(requestCallback);
    databaseLoaderCallback->start(idbFactory, document->securityOrigin(), document, databaseName);
}

void InspectorIndexedDBAgent::requestData(ErrorString* errorString, const String& frameId, const String& databaseName, const String& objectStoreName, const String& indexName, int skipCount, int pageSize, const RefPtr<InspectorObject>* keyRange, PassRefPtr<RequestDataCallback> requestCallback)
{
    Frame* frame = assertFrame(errorString, frameId, m_pageAgent);
    if (!frame)
        return;
    Document* document = assertDocument(errorString, frame);
    if (!document)
        return;
    IDBFactoryBackendInterface* idbFactory = assertIDBFactory(errorString, document);
    if (!idbFactory)
        return;

    InjectedScript injectedScript = m_injectedScriptManager->injectedScriptFor(mainWorldScriptState(frame));

    RefPtr<IDBKeyRange> idbKeyRange = keyRange ? idbKeyRangeFromKeyRange(keyRange->get()) : 0;
    if (keyRange && !idbKeyRange) {
        *errorString = "Can not parse key range.";
        return;
    }

    RefPtr<DataLoaderCallback> dataLoaderCallback = DataLoaderCallback::create(requestCallback, injectedScript, objectStoreName, indexName, idbKeyRange, skipCount, pageSize);
    dataLoaderCallback->start(idbFactory, document->securityOrigin(), document, databaseName);
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR) && ENABLE(INDEXED_DATABASE)
