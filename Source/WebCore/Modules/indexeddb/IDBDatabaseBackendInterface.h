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

#ifndef IDBDatabaseBackendInterface_h
#define IDBDatabaseBackendInterface_h

#include "PlatformString.h"
#include <wtf/PassRefPtr.h>
#include <wtf/Threading.h>

#if ENABLE(INDEXED_DATABASE)

namespace WebCore {

class DOMStringList;
class Frame;
class IDBCallbacks;
class IDBDatabaseCallbacks;
class IDBObjectStoreBackendInterface;
class IDBTransactionBackendInterface;

typedef int ExceptionCode;

// This class is shared by IDBDatabase (async) and IDBDatabaseSync (sync).
// This is implemented by IDBDatabaseBackendImpl and optionally others (in order to proxy
// calls across process barriers). All calls to these classes should be non-blocking and
// trigger work on a background thread if necessary.
class IDBDatabaseBackendInterface : public ThreadSafeRefCounted<IDBDatabaseBackendInterface> {
public:
    virtual ~IDBDatabaseBackendInterface() { }

    virtual String name() const = 0;
    virtual String version() const = 0;
    virtual PassRefPtr<DOMStringList> objectStoreNames() const = 0;

    virtual PassRefPtr<IDBObjectStoreBackendInterface> createObjectStore(const String& name, const String& keyPath, bool autoIncrement, IDBTransactionBackendInterface*, ExceptionCode&) = 0;
    virtual void deleteObjectStore(const String& name, IDBTransactionBackendInterface*, ExceptionCode&) = 0;
    virtual void setVersion(const String& version, PassRefPtr<IDBCallbacks>, PassRefPtr<IDBDatabaseCallbacks>, ExceptionCode&) = 0;
    virtual PassRefPtr<IDBTransactionBackendInterface> transaction(DOMStringList* storeNames, unsigned short mode, ExceptionCode&) = 0;
    virtual void close(PassRefPtr<IDBDatabaseCallbacks>) = 0;

    virtual void open(PassRefPtr<IDBDatabaseCallbacks>) = 0;
};

} // namespace WebCore

#endif

#endif // IDBDatabaseBackendInterface_h
