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
#include "IDBDatabaseCallbacksProxy.h"

#if ENABLE(INDEXED_DATABASE)

#include "WebIDBDatabaseCallbacks.h"

using namespace WebCore;

namespace WebKit {

PassRefPtr<IDBDatabaseCallbacksProxy> IDBDatabaseCallbacksProxy::create(PassOwnPtr<WebIDBDatabaseCallbacks> callbacks)
{
    return adoptRef(new IDBDatabaseCallbacksProxy(callbacks));
}

IDBDatabaseCallbacksProxy::IDBDatabaseCallbacksProxy(PassOwnPtr<WebIDBDatabaseCallbacks> callbacks)
    : m_callbacks(callbacks)
{
}

IDBDatabaseCallbacksProxy::~IDBDatabaseCallbacksProxy()
{
}

void IDBDatabaseCallbacksProxy::onForcedClose()
{
    m_callbacks->onForcedClose();
}

void IDBDatabaseCallbacksProxy::onVersionChange(int64_t oldVersion, int64_t newVersion)
{
    m_callbacks->onVersionChange(oldVersion, newVersion);
}

void IDBDatabaseCallbacksProxy::onVersionChange(const String& requestedVersion)
{
    m_callbacks->onVersionChange(requestedVersion);
}

} // namespace WebKit

#endif // ENABLE(INDEXED_DATABASE)
