/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
#include "WebIDBFactoryImpl.h"

#if ENABLE(INDEXED_DATABASE)

#include "DOMStringList.h"
#include "IDBCallbacksProxy.h"
#include "IDBFactoryBackendImpl.h"
#include "SecurityOrigin.h"
#include "WebIDBDatabaseError.h"
#include <wtf/OwnPtr.h>

using namespace WebCore;

namespace WebKit {

WebIDBFactory* WebIDBFactory::create()
{
    return new WebIDBFactoryImpl();
}

WebIDBFactoryImpl::WebIDBFactoryImpl()
    : m_idbFactoryBackend(IDBFactoryBackendImpl::create())
{
}

WebIDBFactoryImpl::~WebIDBFactoryImpl()
{
}

void WebIDBFactoryImpl::getDatabaseNames(WebIDBCallbacks* callbacks, const WebSecurityOrigin& origin, WebFrame*, const WebString& dataDir, unsigned long long maximumSize, BackingStoreType backingStoreType)
{
    WebString path = dataDir;

    if (backingStoreType == DefaultBackingStore)
        backingStoreType = LevelDBBackingStore;

    m_idbFactoryBackend->getDatabaseNames(IDBCallbacksProxy::create(adoptPtr(callbacks)), origin, 0, path, maximumSize, static_cast<IDBFactoryBackendInterface::BackingStoreType>(backingStoreType));
}


void WebIDBFactoryImpl::open(const WebString& name, WebIDBCallbacks* callbacks, const WebSecurityOrigin& origin, WebFrame*, const WebString& dataDir, unsigned long long maximumSize, BackingStoreType backingStoreType)
{
    WebString path = dataDir;

    if (backingStoreType == DefaultBackingStore)
        backingStoreType = LevelDBBackingStore;

    m_idbFactoryBackend->open(name, IDBCallbacksProxy::create(adoptPtr(callbacks)), origin, 0, path, maximumSize, static_cast<IDBFactoryBackendInterface::BackingStoreType>(backingStoreType));
}

} // namespace WebKit

#endif // ENABLE(INDEXED_DATABASE)
