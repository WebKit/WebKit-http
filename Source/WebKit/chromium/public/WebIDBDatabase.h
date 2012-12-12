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

#ifndef WebIDBDatabase_h
#define WebIDBDatabase_h

#include "WebDOMStringList.h"
#include "WebExceptionCode.h"
#include "WebIDBMetadata.h"
#include "WebIDBTransaction.h"
#include "platform/WebCommon.h"

namespace WebKit {

class WebFrame;
class WebIDBCallbacks;
class WebIDBDatabaseCallbacks;
class WebIDBKey;
class WebIDBKeyPath;
class WebIDBKeyRange;
class WebIDBObjectStore;
class WebIDBTransaction;

// See comment in WebIDBFactory for a high level overview of these classes.
class WebIDBDatabase {
public:
    virtual ~WebIDBDatabase() { }

    virtual WebIDBMetadata metadata() const
    {
        WEBKIT_ASSERT_NOT_REACHED();
        return WebIDBMetadata();
    }
    virtual WebIDBObjectStore* createObjectStore(long long, const WebString&, const WebIDBKeyPath&, bool, const WebIDBTransaction&, WebExceptionCode&)
    {
        WEBKIT_ASSERT_NOT_REACHED();
        return 0;
    }
    virtual void deleteObjectStore(long long objectStoreId, const WebIDBTransaction& transaction, WebExceptionCode& ec) { WEBKIT_ASSERT_NOT_REACHED(); }
    // FIXME: Remove this method in https://bugs.webkit.org/show_bug.cgi?id=103923.
    virtual WebIDBTransaction* createTransaction(long long id, const WebVector<long long>&, unsigned short mode) { WEBKIT_ASSERT_NOT_REACHED(); return 0; }
    virtual void createTransaction(long long id, WebIDBDatabaseCallbacks* callbacks, const WebVector<long long>&, unsigned short mode) { WEBKIT_ASSERT_NOT_REACHED(); }
    virtual void close() { WEBKIT_ASSERT_NOT_REACHED(); }
    virtual void forceClose() { WEBKIT_ASSERT_NOT_REACHED(); }

    virtual void abort(long long transactionId) { WEBKIT_ASSERT_NOT_REACHED(); }
    virtual void commit(long long transactionId) { WEBKIT_ASSERT_NOT_REACHED(); }

    enum TaskType {
        NormalTask = 0,
        PreemptiveTask
    };

    enum PutMode {
        AddOrUpdate,
        AddOnly,
        CursorUpdate
    };

    typedef WebVector<WebIDBKey> WebIndexKeys;

    virtual void get(long long transactionId, long long objectStoreId, long long indexId, const WebIDBKeyRange&, bool keyOnly, WebIDBCallbacks*) { WEBKIT_ASSERT_NOT_REACHED(); }
    virtual void put(long long transactionId, long long objectStoreId, const WebVector<unsigned char>& value, const WebIDBKey&, PutMode, WebIDBCallbacks*, const WebVector<long long>& indexIds, const WebVector<WebIndexKeys>&) { WEBKIT_ASSERT_NOT_REACHED(); }
    virtual void setIndexKeys(long long transactionId, long long objectStoreId, const WebIDBKey&, const WebVector<long long>& indexIds, const WebVector<WebIndexKeys>&) { WEBKIT_ASSERT_NOT_REACHED(); }
    virtual void setIndexesReady(long long transactionId, long long objectStoreId, const WebVector<long long>& indexIds) { WEBKIT_ASSERT_NOT_REACHED(); }
    virtual void openCursor(long long transactionId, long long objectStoreId, long long indexId, const WebIDBKeyRange&, unsigned short direction, bool keyOnly, TaskType, WebIDBCallbacks*) { WEBKIT_ASSERT_NOT_REACHED(); }
    virtual void count(long long transactionId, long long objectStoreId, long long indexId, const WebIDBKeyRange&, WebIDBCallbacks*) { WEBKIT_ASSERT_NOT_REACHED(); }
    virtual void deleteRange(long long transactionId, long long objectStoreId, const WebIDBKeyRange&, WebIDBCallbacks*) { WEBKIT_ASSERT_NOT_REACHED(); }
    virtual void clear(long long transactionId, long long objectStoreId, WebIDBCallbacks*)  { WEBKIT_ASSERT_NOT_REACHED(); }

protected:
    WebIDBDatabase() { }
};

} // namespace WebKit

#endif // WebIDBDatabase_h
