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

#include "config.h"
#include "WebIDBIndexImpl.h"

#if ENABLE(INDEXED_DATABASE)

#include "IDBCallbacksProxy.h"
#include "IDBIndex.h"
#include "IDBKeyRange.h"
#include "WebIDBCallbacks.h"
#include "WebIDBKey.h"
#include "WebIDBKeyRange.h"

using namespace WebCore;

namespace WebKit {

WebIDBIndexImpl::WebIDBIndexImpl(PassRefPtr<IDBIndexBackendInterface> backend)
    : m_backend(backend)
{
}

WebIDBIndexImpl::~WebIDBIndexImpl()
{
}

void WebIDBIndexImpl::openObjectCursor(const WebIDBKeyRange& keyRange, unsigned short direction, WebIDBCallbacks* callbacks, const WebIDBTransaction& transaction, WebExceptionCode& ec)
{
    m_backend->openCursor(keyRange, direction, IDBCallbacksProxy::create(adoptPtr(callbacks)), transaction.getIDBTransactionBackendInterface(), ec);
}

void WebIDBIndexImpl::openKeyCursor(const WebIDBKeyRange& keyRange, unsigned short direction, WebIDBCallbacks* callbacks, const WebIDBTransaction& transaction, WebExceptionCode& ec)
{
    m_backend->openKeyCursor(keyRange, direction, IDBCallbacksProxy::create(adoptPtr(callbacks)), transaction.getIDBTransactionBackendInterface(), ec);
}

void WebIDBIndexImpl::count(const WebIDBKeyRange& keyRange,  WebIDBCallbacks* callbacks, const WebIDBTransaction& transaction, WebExceptionCode& ec)
{
    m_backend->count(keyRange, IDBCallbacksProxy::create(adoptPtr(callbacks)), transaction.getIDBTransactionBackendInterface(), ec);
}

void WebIDBIndexImpl::getObject(const WebIDBKeyRange& keyRange, WebIDBCallbacks* callbacks, const WebIDBTransaction& transaction, WebExceptionCode& ec)
{
    m_backend->get(keyRange, IDBCallbacksProxy::create(adoptPtr(callbacks)), transaction.getIDBTransactionBackendInterface(), ec);
}

void WebIDBIndexImpl::getKey(const WebIDBKeyRange& keyRange, WebIDBCallbacks* callbacks, const WebIDBTransaction& transaction, WebExceptionCode& ec)
{
    m_backend->getKey(keyRange, IDBCallbacksProxy::create(adoptPtr(callbacks)), transaction.getIDBTransactionBackendInterface(), ec);
}

} // namespace WebKit

#endif // ENABLE(INDEXED_DATABASE)
