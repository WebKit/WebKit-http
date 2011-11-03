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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
#include "IDBCallbacksProxy.h"

#if ENABLE(INDEXED_DATABASE)

#include "IDBDatabaseBackendProxy.h"
#include "IDBDatabaseError.h"
#include "WebIDBCallbacks.h"
#include "WebIDBCursorImpl.h"
#include "WebIDBDatabaseImpl.h"
#include "WebIDBDatabaseError.h"
#include "WebIDBKey.h"
#include "WebIDBTransactionImpl.h"
#include "WebSerializedScriptValue.h"

using namespace WebCore;

namespace WebKit {

PassRefPtr<IDBCallbacksProxy> IDBCallbacksProxy::create(PassOwnPtr<WebIDBCallbacks> callbacks)
{
    return adoptRef(new IDBCallbacksProxy(callbacks));
}

IDBCallbacksProxy::IDBCallbacksProxy(PassOwnPtr<WebIDBCallbacks> callbacks)
    : m_callbacks(callbacks)
{
}

IDBCallbacksProxy::~IDBCallbacksProxy()
{
}

void IDBCallbacksProxy::onError(PassRefPtr<IDBDatabaseError> idbDatabaseError)
{
    m_callbacks->onError(WebIDBDatabaseError(idbDatabaseError));
}

void IDBCallbacksProxy::onSuccess(PassRefPtr<IDBCursorBackendInterface> idbCursorBackend)
{
    m_callbacks->onSuccess(new WebIDBCursorImpl(idbCursorBackend));
}

void IDBCallbacksProxy::onSuccess(PassRefPtr<IDBDatabaseBackendInterface> backend)
{
    m_callbacks->onSuccess(new WebIDBDatabaseImpl(backend));
}

void IDBCallbacksProxy::onSuccess(PassRefPtr<IDBKey> idbKey)
{
    m_callbacks->onSuccess(WebIDBKey(idbKey));
}

void IDBCallbacksProxy::onSuccess(PassRefPtr<IDBTransactionBackendInterface> backend)
{
    m_callbacks->onSuccess(new WebIDBTransactionImpl(backend));
}

void IDBCallbacksProxy::onSuccess(PassRefPtr<DOMStringList> domStringList)
{
    m_callbacks->onSuccess(WebDOMStringList(domStringList));
}

void IDBCallbacksProxy::onSuccess(PassRefPtr<SerializedScriptValue> serializedScriptValue)
{
    m_callbacks->onSuccess(WebSerializedScriptValue(serializedScriptValue));
}

void IDBCallbacksProxy::onSuccessWithContinuation()
{
    m_callbacks->onSuccessWithContinuation();
}

void IDBCallbacksProxy::onBlocked()
{
    m_callbacks->onBlocked();
}

} // namespace WebKit

#endif // ENABLE(INDEXED_DATABASE)
