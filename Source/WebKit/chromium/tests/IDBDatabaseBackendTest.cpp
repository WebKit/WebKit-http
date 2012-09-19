/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
#include "IDBBackingStore.h"
#include "IDBCursorBackendInterface.h"
#include "IDBDatabaseBackendImpl.h"
#include "IDBDatabaseCallbacksProxy.h"
#include "IDBFactoryBackendImpl.h"
#include "IDBFakeBackingStore.h"
#include "IDBIndexBackendImpl.h"
#include "IDBObjectStoreBackendImpl.h"
#include "IDBTransactionCoordinator.h"
#include "WebIDBDatabaseCallbacksImpl.h"
#include "WebIDBDatabaseImpl.h"

#include <gtest/gtest.h>

#if ENABLE(INDEXED_DATABASE)

using namespace WebCore;
using WebKit::IDBDatabaseCallbacksProxy;
using WebKit::WebIDBDatabase;
using WebKit::WebIDBDatabaseImpl;
using WebKit::WebIDBDatabaseCallbacksImpl;

namespace {

TEST(IDBDatabaseBackendTest, BackingStoreRetention)
{
    RefPtr<IDBFakeBackingStore> backingStore = adoptRef(new IDBFakeBackingStore());
    EXPECT_TRUE(backingStore->hasOneRef());

    IDBTransactionCoordinator* coordinator = 0;
    IDBFactoryBackendImpl* factory = 0;
    RefPtr<IDBDatabaseBackendImpl> db = IDBDatabaseBackendImpl::create("db", backingStore.get(), coordinator, factory, "uniqueid");
    EXPECT_GT(backingStore->refCount(), 1);

    const bool autoIncrement = false;
    RefPtr<IDBObjectStoreBackendImpl> store = IDBObjectStoreBackendImpl::create(db.get(), "store", IDBKeyPath("keyPath"), autoIncrement);
    EXPECT_GT(backingStore->refCount(), 1);

    const bool unique = false;
    const bool multiEntry = false;
    RefPtr<IDBIndexBackendImpl> index = IDBIndexBackendImpl::create(db.get(), store.get(), "index", IDBKeyPath("keyPath"), unique, multiEntry);
    EXPECT_GT(backingStore->refCount(), 1);

    db.clear();
    EXPECT_TRUE(backingStore->hasOneRef());
    store.clear();
    EXPECT_TRUE(backingStore->hasOneRef());
    index.clear();
    EXPECT_TRUE(backingStore->hasOneRef());
}

class MockIDBCallbacks : public IDBCallbacks {
public:
    static PassRefPtr<MockIDBCallbacks> create() { return adoptRef(new MockIDBCallbacks()); }
    virtual ~MockIDBCallbacks()
    {
        EXPECT_TRUE(m_wasSuccessDBCalled);
    }
    virtual void onError(PassRefPtr<IDBDatabaseError>) OVERRIDE { }
    virtual void onSuccess(PassRefPtr<DOMStringList>) OVERRIDE { }
    virtual void onSuccess(PassRefPtr<IDBCursorBackendInterface>, PassRefPtr<IDBKey>, PassRefPtr<IDBKey>, PassRefPtr<SerializedScriptValue>) OVERRIDE { }
    virtual void onSuccess(PassRefPtr<IDBDatabaseBackendInterface>) OVERRIDE
    {
        m_wasSuccessDBCalled = true;
    }
    virtual void onSuccess(PassRefPtr<IDBKey>) OVERRIDE { }
    virtual void onSuccess(PassRefPtr<IDBTransactionBackendInterface>) OVERRIDE { }
    virtual void onSuccess(PassRefPtr<SerializedScriptValue>) OVERRIDE { }
    virtual void onSuccess(PassRefPtr<SerializedScriptValue>, PassRefPtr<IDBKey>, const IDBKeyPath&) OVERRIDE { };
    virtual void onSuccess(PassRefPtr<IDBKey>, PassRefPtr<IDBKey>, PassRefPtr<SerializedScriptValue>) OVERRIDE { };
    virtual void onSuccessWithPrefetch(const Vector<RefPtr<IDBKey> >&, const Vector<RefPtr<IDBKey> >&, const Vector<RefPtr<SerializedScriptValue> >&) OVERRIDE { }
    virtual void onBlocked() OVERRIDE { }
private:
    MockIDBCallbacks()
        : m_wasSuccessDBCalled(false) { }
    bool m_wasSuccessDBCalled;
};

class FakeIDBDatabaseCallbacks : public IDBDatabaseCallbacks {
public:
    static PassRefPtr<FakeIDBDatabaseCallbacks> create() { return adoptRef(new FakeIDBDatabaseCallbacks()); }
    virtual ~FakeIDBDatabaseCallbacks() { }
    virtual void onVersionChange(const String& version) OVERRIDE { }
    virtual void onVersionChange(int64_t oldVersion, int64_t newVersion) OVERRIDE { }
    virtual void onForcedClose() OVERRIDE { }
private:
    FakeIDBDatabaseCallbacks() { }
};

TEST(IDBDatabaseBackendTest, DISABLED_ConnectionLifecycle)
{
    RefPtr<IDBFakeBackingStore> backingStore = adoptRef(new IDBFakeBackingStore());
    EXPECT_TRUE(backingStore->hasOneRef());

    IDBTransactionCoordinator* coordinator = 0;
    IDBFactoryBackendImpl* factory = 0;
    RefPtr<IDBDatabaseBackendImpl> db = IDBDatabaseBackendImpl::create("db", backingStore.get(), coordinator, factory, "uniqueid");
    EXPECT_GT(backingStore->refCount(), 1);

    RefPtr<MockIDBCallbacks> request1 = MockIDBCallbacks::create();
    RefPtr<FakeIDBDatabaseCallbacks> connection1 = FakeIDBDatabaseCallbacks::create();
    db->openConnection(request1, connection1);

    RefPtr<MockIDBCallbacks> request2 = MockIDBCallbacks::create();
    RefPtr<FakeIDBDatabaseCallbacks> connection2 = FakeIDBDatabaseCallbacks::create();
    db->openConnection(request2, connection2);

    db->close(connection1);
    EXPECT_GT(backingStore->refCount(), 1);

    db->close(connection2);
    EXPECT_TRUE(backingStore->hasOneRef());
}

class MockIDBDatabaseBackendProxy : public IDBDatabaseBackendInterface {
public:
    static PassRefPtr<MockIDBDatabaseBackendProxy> create(WebIDBDatabaseImpl& database)
    {
        return adoptRef(new MockIDBDatabaseBackendProxy(database));
    }

    ~MockIDBDatabaseBackendProxy()
    {
        EXPECT_TRUE(m_wasCloseCalled);
    }

    virtual IDBDatabaseMetadata metadata() const { return IDBDatabaseMetadata(); }
    virtual PassRefPtr<IDBObjectStoreBackendInterface> createObjectStore(const String& name, const IDBKeyPath&, bool autoIncrement, IDBTransactionBackendInterface*, ExceptionCode&) { return 0; }
    virtual void deleteObjectStore(const String& name, IDBTransactionBackendInterface*, ExceptionCode&) { }
    virtual void setVersion(const String& version, PassRefPtr<IDBCallbacks>, PassRefPtr<IDBDatabaseCallbacks>, ExceptionCode&) { }
    virtual PassRefPtr<IDBTransactionBackendInterface> transaction(DOMStringList* storeNames, unsigned short mode, ExceptionCode&) { return 0; }

    virtual void close(PassRefPtr<IDBDatabaseCallbacks>)
    {
        m_wasCloseCalled = true;
        m_webDatabase.close();
    }

private:
    MockIDBDatabaseBackendProxy(WebIDBDatabaseImpl& webDatabase)
        : m_wasCloseCalled(false)
        , m_webDatabase(webDatabase) { }

    bool m_wasCloseCalled;

    WebIDBDatabaseImpl& m_webDatabase;
};

TEST(IDBDatabaseBackendTest, DISABLED_ForcedClose)
{
    RefPtr<IDBFakeBackingStore> backingStore = adoptRef(new IDBFakeBackingStore());
    EXPECT_TRUE(backingStore->hasOneRef());

    IDBTransactionCoordinator* coordinator = 0;
    IDBFactoryBackendImpl* factory = 0;
    RefPtr<IDBDatabaseBackendImpl> backend = IDBDatabaseBackendImpl::create("db", backingStore.get(), coordinator, factory, "uniqueid");
    EXPECT_GT(backingStore->refCount(), 1);

    RefPtr<FakeIDBDatabaseCallbacks> connection = FakeIDBDatabaseCallbacks::create();
    RefPtr<IDBDatabaseCallbacksProxy> connectionProxy = IDBDatabaseCallbacksProxy::create(adoptPtr(new WebIDBDatabaseCallbacksImpl(connection)));
    WebIDBDatabaseImpl webDatabase(backend, connectionProxy);

    RefPtr<MockIDBDatabaseBackendProxy> proxy = MockIDBDatabaseBackendProxy::create(webDatabase);
    RefPtr<MockIDBCallbacks> request = MockIDBCallbacks::create();
    backend->openConnection(request, connectionProxy);

    ScriptExecutionContext* context = 0;
    RefPtr<IDBDatabase> idbDatabase = IDBDatabase::create(context, proxy, connection);

    webDatabase.forceClose();

    EXPECT_TRUE(backingStore->hasOneRef());
}

} // namespace

#endif // ENABLE(INDEXED_DATABASE)
