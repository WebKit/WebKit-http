/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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
#include "WebStorageAreaImpl.h"

#if ENABLE(DOM_STORAGE)

#include "ExceptionCode.h"

#include "WebString.h"
#include "WebURL.h"

namespace WebKit {

const WebURL* WebStorageAreaImpl::storageEventURL = 0;

WebStorageAreaImpl::WebStorageAreaImpl(PassRefPtr<WebCore::StorageArea> storageArea)
    : m_storageArea(storageArea)
{
}

WebStorageAreaImpl::~WebStorageAreaImpl()
{
}

unsigned WebStorageAreaImpl::length()
{
    return m_storageArea->length(0);
}

WebString WebStorageAreaImpl::key(unsigned index)
{
    return m_storageArea->key(index, 0);
}

WebString WebStorageAreaImpl::getItem(const WebString& key)
{
    return m_storageArea->getItem(key, 0);
}

void WebStorageAreaImpl::setItem(const WebString& key, const WebString& value, const WebURL& url, Result& result, WebString& oldValue)
{
    int exceptionCode = 0;

    ScopedStorageEventURL scope(url);
    oldValue = m_storageArea->setItem(key, value, exceptionCode, 0);

    if (exceptionCode) {
        ASSERT(exceptionCode == WebCore::QUOTA_EXCEEDED_ERR);
        result = ResultBlockedByQuota;
    } else
        result = ResultOK;
}

void WebStorageAreaImpl::removeItem(const WebString& key, const WebURL& url, WebString& oldValue)
{
    ScopedStorageEventURL scope(url);
    oldValue = m_storageArea->removeItem(key, 0);
}

void WebStorageAreaImpl::clear(const WebURL& url, bool& somethingCleared)
{
    ScopedStorageEventURL scope(url);
    somethingCleared = m_storageArea->clear(0);
}

} // namespace WebKit

#endif // ENABLE(DOM_STORAGE)
