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

#ifndef IDBCursor_h
#define IDBCursor_h

#if ENABLE(INDEXED_DATABASE)

#include "IDBKey.h"
#include "IDBTransaction.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class IDBAny;
class IDBCallbacks;
class IDBCursorBackendInterface;
class IDBRequest;
class ScriptExecutionContext;

typedef int ExceptionCode;

class IDBCursor : public RefCounted<IDBCursor> {
public:
    enum Direction {
        NEXT = 0,
        NEXT_NO_DUPLICATE = 1,
        PREV = 2,
        PREV_NO_DUPLICATE = 3,
    };

    static const AtomicString& directionNext();
    static const AtomicString& directionNextUnique();
    static const AtomicString& directionPrev();
    static const AtomicString& directionPrevUnique();

    static IDBCursor::Direction stringToDirection(const String& modeString, ExceptionCode&);
    static const AtomicString& directionToString(unsigned short mode, ExceptionCode&);

    static PassRefPtr<IDBCursor> create(PassRefPtr<IDBCursorBackendInterface>, Direction, IDBRequest*, IDBAny* source, IDBTransaction*);
    virtual ~IDBCursor();

    // FIXME: Try to modify the code generator so this is unneeded.
    void continueFunction(ExceptionCode& ec) { continueFunction(0, ec); }

    // Implement the IDL
    const String& direction() const;
    PassRefPtr<IDBKey> key() const;
    PassRefPtr<IDBKey> primaryKey() const;
    PassRefPtr<IDBAny> value();
    IDBAny* source() const;

    PassRefPtr<IDBRequest> update(ScriptExecutionContext*, ScriptValue&, ExceptionCode&);
    void advance(unsigned long, ExceptionCode&);
    void continueFunction(PassRefPtr<IDBKey>, ExceptionCode&);
    PassRefPtr<IDBRequest> deleteFunction(ScriptExecutionContext*, ExceptionCode&);

    void postSuccessHandlerCallback();
    void close();
    void setValueReady(PassRefPtr<IDBKey>, PassRefPtr<IDBKey> primaryKey, ScriptValue&);

    // The spec requires that the script object that wraps the value
    // be unchanged until the value changes as a result of the cursor
    // advancing.
    bool valueIsDirty() { return m_valueIsDirty; }

protected:
    IDBCursor(PassRefPtr<IDBCursorBackendInterface>, Direction, IDBRequest*, IDBAny* source, IDBTransaction*);
    virtual bool isKeyCursor() const { return true; }

private:
    PassRefPtr<IDBObjectStore> effectiveObjectStore();

    RefPtr<IDBCursorBackendInterface> m_backend;
    RefPtr<IDBRequest> m_request;
    const Direction m_direction;
    RefPtr<IDBAny> m_source;
    RefPtr<IDBTransaction> m_transaction;
    IDBTransaction::OpenCursorNotifier m_transactionNotifier;
    bool m_gotValue;
    // These values are held because m_backend may advance while they
    // are still valid for the current success handlers.
    RefPtr<IDBKey> m_currentKey;
    RefPtr<IDBKey> m_currentPrimaryKey;
    RefPtr<IDBAny> m_currentValue;
    bool m_valueIsDirty;
};

} // namespace WebCore

#endif

#endif // IDBCursor_h
