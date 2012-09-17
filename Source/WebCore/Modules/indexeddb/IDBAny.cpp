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

#include "config.h"
#include "IDBAny.h"

#if ENABLE(INDEXED_DATABASE)

#include "IDBCursorWithValue.h"
#include "IDBDatabase.h"
#include "IDBFactory.h"
#include "IDBIndex.h"
#include "IDBKeyPath.h"
#include "IDBObjectStore.h"

namespace WebCore {

PassRefPtr<IDBAny> IDBAny::createInvalid()
{
    return adoptRef(new IDBAny());
}

PassRefPtr<IDBAny> IDBAny::createNull()
{
    RefPtr<IDBAny> idbAny = adoptRef(new IDBAny());
    idbAny->setNull();
    return idbAny.release();
}

PassRefPtr<IDBAny> IDBAny::createString(const String& value)
{
    RefPtr<IDBAny> idbAny = adoptRef(new IDBAny());
    idbAny->set(value);
    return idbAny.release();
}

IDBAny::IDBAny()
    : m_type(UndefinedType)
{
}

IDBAny::~IDBAny()
{
}

PassRefPtr<DOMStringList> IDBAny::domStringList()
{
    ASSERT(m_type == DOMStringListType);
    return m_domStringList;
}

PassRefPtr<IDBCursor> IDBAny::idbCursor()
{
    ASSERT(m_type == IDBCursorType);
    return m_idbCursor;
}

PassRefPtr<IDBCursorWithValue> IDBAny::idbCursorWithValue()
{
    ASSERT(m_type == IDBCursorWithValueType);
    return m_idbCursorWithValue;
}

PassRefPtr<IDBDatabase> IDBAny::idbDatabase()
{
    ASSERT(m_type == IDBDatabaseType);
    return m_idbDatabase;
}

PassRefPtr<IDBFactory> IDBAny::idbFactory()
{
    ASSERT(m_type == IDBFactoryType);
    return m_idbFactory;
}

PassRefPtr<IDBIndex> IDBAny::idbIndex()
{
    ASSERT(m_type == IDBIndexType);
    return m_idbIndex;
}

PassRefPtr<IDBKey> IDBAny::idbKey()
{
    ASSERT(m_type == IDBKeyType);
    return m_idbKey;
}

PassRefPtr<IDBObjectStore> IDBAny::idbObjectStore()
{
    ASSERT(m_type == IDBObjectStoreType);
    return m_idbObjectStore;
}

PassRefPtr<IDBTransaction> IDBAny::idbTransaction()
{
    ASSERT(m_type == IDBTransactionType);
    return m_idbTransaction;
}

ScriptValue IDBAny::scriptValue()
{
    ASSERT(m_type == ScriptValueType);
    return m_scriptValue;
}

const String& IDBAny::string()
{
    ASSERT(m_type == StringType);
    return m_string;
}

int64_t IDBAny::integer()
{
    ASSERT(m_type == IntegerType);
    return m_integer;
}

void IDBAny::setNull()
{
    ASSERT(m_type == UndefinedType);
    m_type = NullType;
}

void IDBAny::set(PassRefPtr<DOMStringList> value)
{
    ASSERT(m_type == UndefinedType);
    m_type = DOMStringListType;
    m_domStringList = value;
}

void IDBAny::set(PassRefPtr<IDBCursorWithValue> value)
{
    ASSERT(m_type == UndefinedType);
    m_type = IDBCursorWithValueType;
    m_idbCursorWithValue = value;
}

void IDBAny::set(PassRefPtr<IDBCursor> value)
{
    ASSERT(m_type == UndefinedType);
    m_type = IDBCursorType;
    m_idbCursor = value;
}

void IDBAny::set(PassRefPtr<IDBDatabase> value)
{
    ASSERT(m_type == UndefinedType);
    m_type = IDBDatabaseType;
    m_idbDatabase = value;
}

void IDBAny::set(PassRefPtr<IDBFactory> value)
{
    ASSERT(m_type == UndefinedType);
    m_type = IDBFactoryType;
    m_idbFactory = value;
}

void IDBAny::set(PassRefPtr<IDBIndex> value)
{
    ASSERT(m_type == UndefinedType);
    m_type = IDBIndexType;
    m_idbIndex = value;
}

void IDBAny::set(PassRefPtr<IDBKey> value)
{
    ASSERT(m_type == UndefinedType);
    m_type = IDBKeyType;
    m_idbKey = value;
}

void IDBAny::set(PassRefPtr<IDBTransaction> value)
{
    ASSERT(m_type == UndefinedType);
    m_type = IDBTransactionType;
    m_idbTransaction = value;
}

void IDBAny::set(PassRefPtr<IDBObjectStore> value)
{
    ASSERT(m_type == UndefinedType);
    m_type = IDBObjectStoreType;
    m_idbObjectStore = value;
}

void IDBAny::set(const ScriptValue& value)
{
    ASSERT(m_type == UndefinedType);
    m_type = ScriptValueType;
    m_scriptValue = value;
}

void IDBAny::set(const IDBKeyPath& value)
{
    ASSERT(m_type == UndefinedType);
    switch (value.type()) {
    case IDBKeyPath::NullType:
        m_type = NullType;
        break;
    case IDBKeyPath::StringType:
        m_type = StringType;
        m_string = value.string();
        break;
    case IDBKeyPath::ArrayType:
        RefPtr<DOMStringList> keyPaths = DOMStringList::create();
        for (Vector<String>::const_iterator it = value.array().begin(); it != value.array().end(); ++it)
            keyPaths->append(*it);
        m_type = DOMStringListType;
        m_domStringList = keyPaths.release();
        break;
    }
}

void IDBAny::set(const String& value)
{
    ASSERT(m_type == UndefinedType);
    m_type = StringType;
    m_string = value;
}

void IDBAny::set(int64_t value)
{
    ASSERT(m_type == UndefinedType);
    m_type = IntegerType;
    m_integer = value;
}

} // namespace WebCore

#endif
