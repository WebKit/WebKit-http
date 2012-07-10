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

#ifndef WebIDBTransactionImpl_h
#define WebIDBTransactionImpl_h

#if ENABLE(INDEXED_DATABASE)

#include "platform/WebCommon.h"
#include "WebIDBTransaction.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebKit {

// See comment in WebIndexedDatabase for a high level overview these classes.
class WebIDBTransactionImpl: public WebIDBTransaction {
public:
    WebIDBTransactionImpl(WTF::PassRefPtr<WebCore::IDBTransactionBackendInterface>);
    virtual ~WebIDBTransactionImpl();  

    virtual WebIDBObjectStore* objectStore(const WebString& name, WebExceptionCode&);
    virtual void commit();
    virtual void abort();
    virtual void didCompleteTaskEvents();
    virtual void setCallbacks(WebIDBTransactionCallbacks*);

    virtual WebCore::IDBTransactionBackendInterface* getIDBTransactionBackendInterface() const;

private:
    WTF::RefPtr<WebCore::IDBTransactionBackendInterface> m_backend;
};

} // namespace WebKit

#endif // ENABLE(INDEXED_DATABASE)

#endif // WebIDBTransactionImpl_h
