/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if ENABLE(INDEXED_DATABASE) && ENABLE(DATABASE_PROCESS)

#include "ArgumentCoders.h"

namespace WTF {
    class String;
}

namespace WebCore {
    class IDBValue;
    struct IDBKeyRangeData;
    class IDBTransactionInfo;
    class IDBIndexInfo;
    class IDBCursorInfo;
    class IDBResourceIdentifier;
    struct SecurityOriginData;
    class IDBKeyData;
    class IDBObjectStoreInfo;
    struct IDBGetRecordData;
    class IDBRequestData;
}

namespace Messages {
namespace WebIDBConnectionToClient {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("WebIDBConnectionToClient");
}

class DeleteDatabase {
public:
    typedef std::tuple<const WebCore::IDBRequestData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DeleteDatabase"); }
    static const bool isSync = false;

    explicit DeleteDatabase(const WebCore::IDBRequestData& requestData)
        : m_arguments(requestData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class OpenDatabase {
public:
    typedef std::tuple<const WebCore::IDBRequestData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("OpenDatabase"); }
    static const bool isSync = false;

    explicit OpenDatabase(const WebCore::IDBRequestData& requestData)
        : m_arguments(requestData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class AbortTransaction {
public:
    typedef std::tuple<const WebCore::IDBResourceIdentifier&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("AbortTransaction"); }
    static const bool isSync = false;

    explicit AbortTransaction(const WebCore::IDBResourceIdentifier& transactionIdentifier)
        : m_arguments(transactionIdentifier)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class CommitTransaction {
public:
    typedef std::tuple<const WebCore::IDBResourceIdentifier&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CommitTransaction"); }
    static const bool isSync = false;

    explicit CommitTransaction(const WebCore::IDBResourceIdentifier& transactionIdentifier)
        : m_arguments(transactionIdentifier)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidFinishHandlingVersionChangeTransaction {
public:
    typedef std::tuple<uint64_t, const WebCore::IDBResourceIdentifier&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidFinishHandlingVersionChangeTransaction"); }
    static const bool isSync = false;

    DidFinishHandlingVersionChangeTransaction(uint64_t databaseConnectionIdentifier, const WebCore::IDBResourceIdentifier& transactionIdentifier)
        : m_arguments(databaseConnectionIdentifier, transactionIdentifier)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class CreateObjectStore {
public:
    typedef std::tuple<const WebCore::IDBRequestData&, const WebCore::IDBObjectStoreInfo&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CreateObjectStore"); }
    static const bool isSync = false;

    CreateObjectStore(const WebCore::IDBRequestData& requestData, const WebCore::IDBObjectStoreInfo& info)
        : m_arguments(requestData, info)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DeleteObjectStore {
public:
    typedef std::tuple<const WebCore::IDBRequestData&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DeleteObjectStore"); }
    static const bool isSync = false;

    DeleteObjectStore(const WebCore::IDBRequestData& requestData, const String& objectStoreName)
        : m_arguments(requestData, objectStoreName)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ClearObjectStore {
public:
    typedef std::tuple<const WebCore::IDBRequestData&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ClearObjectStore"); }
    static const bool isSync = false;

    ClearObjectStore(const WebCore::IDBRequestData& requestData, uint64_t objectStoreIdentifier)
        : m_arguments(requestData, objectStoreIdentifier)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class CreateIndex {
public:
    typedef std::tuple<const WebCore::IDBRequestData&, const WebCore::IDBIndexInfo&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CreateIndex"); }
    static const bool isSync = false;

    CreateIndex(const WebCore::IDBRequestData& requestData, const WebCore::IDBIndexInfo& info)
        : m_arguments(requestData, info)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DeleteIndex {
public:
    typedef std::tuple<const WebCore::IDBRequestData&, uint64_t, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DeleteIndex"); }
    static const bool isSync = false;

    DeleteIndex(const WebCore::IDBRequestData& requestData, uint64_t objectStoreIdentifier, const String& indexName)
        : m_arguments(requestData, objectStoreIdentifier, indexName)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class PutOrAdd {
public:
    typedef std::tuple<const WebCore::IDBRequestData&, const WebCore::IDBKeyData&, const WebCore::IDBValue&, const unsigned&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PutOrAdd"); }
    static const bool isSync = false;

    PutOrAdd(const WebCore::IDBRequestData& requestData, const WebCore::IDBKeyData& key, const WebCore::IDBValue& value, const unsigned& overwriteMode)
        : m_arguments(requestData, key, value, overwriteMode)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class GetRecord {
public:
    typedef std::tuple<const WebCore::IDBRequestData&, const WebCore::IDBGetRecordData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetRecord"); }
    static const bool isSync = false;

    GetRecord(const WebCore::IDBRequestData& requestData, const WebCore::IDBGetRecordData& getRecordData)
        : m_arguments(requestData, getRecordData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class GetCount {
public:
    typedef std::tuple<const WebCore::IDBRequestData&, const WebCore::IDBKeyRangeData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetCount"); }
    static const bool isSync = false;

    GetCount(const WebCore::IDBRequestData& requestData, const WebCore::IDBKeyRangeData& range)
        : m_arguments(requestData, range)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DeleteRecord {
public:
    typedef std::tuple<const WebCore::IDBRequestData&, const WebCore::IDBKeyRangeData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DeleteRecord"); }
    static const bool isSync = false;

    DeleteRecord(const WebCore::IDBRequestData& requestData, const WebCore::IDBKeyRangeData& range)
        : m_arguments(requestData, range)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class OpenCursor {
public:
    typedef std::tuple<const WebCore::IDBRequestData&, const WebCore::IDBCursorInfo&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("OpenCursor"); }
    static const bool isSync = false;

    OpenCursor(const WebCore::IDBRequestData& requestData, const WebCore::IDBCursorInfo& info)
        : m_arguments(requestData, info)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class IterateCursor {
public:
    typedef std::tuple<const WebCore::IDBRequestData&, const WebCore::IDBKeyData&, uint32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("IterateCursor"); }
    static const bool isSync = false;

    IterateCursor(const WebCore::IDBRequestData& requestData, const WebCore::IDBKeyData& key, uint32_t count)
        : m_arguments(requestData, key, count)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class EstablishTransaction {
public:
    typedef std::tuple<uint64_t, const WebCore::IDBTransactionInfo&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("EstablishTransaction"); }
    static const bool isSync = false;

    EstablishTransaction(uint64_t databaseConnectionIdentifier, const WebCore::IDBTransactionInfo& info)
        : m_arguments(databaseConnectionIdentifier, info)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DatabaseConnectionClosed {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DatabaseConnectionClosed"); }
    static const bool isSync = false;

    explicit DatabaseConnectionClosed(uint64_t databaseConnectionIdentifier)
        : m_arguments(databaseConnectionIdentifier)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class AbortOpenAndUpgradeNeeded {
public:
    typedef std::tuple<uint64_t, const WebCore::IDBResourceIdentifier&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("AbortOpenAndUpgradeNeeded"); }
    static const bool isSync = false;

    AbortOpenAndUpgradeNeeded(uint64_t databaseConnectionIdentifier, const WebCore::IDBResourceIdentifier& transactionIdentifier)
        : m_arguments(databaseConnectionIdentifier, transactionIdentifier)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidFireVersionChangeEvent {
public:
    typedef std::tuple<uint64_t, const WebCore::IDBResourceIdentifier&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidFireVersionChangeEvent"); }
    static const bool isSync = false;

    DidFireVersionChangeEvent(uint64_t databaseConnectionIdentifier, const WebCore::IDBResourceIdentifier& requestIdentifier)
        : m_arguments(databaseConnectionIdentifier, requestIdentifier)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class OpenDBRequestCancelled {
public:
    typedef std::tuple<const WebCore::IDBRequestData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("OpenDBRequestCancelled"); }
    static const bool isSync = false;

    explicit OpenDBRequestCancelled(const WebCore::IDBRequestData& requestData)
        : m_arguments(requestData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ConfirmDidCloseFromServer {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ConfirmDidCloseFromServer"); }
    static const bool isSync = false;

    explicit ConfirmDidCloseFromServer(uint64_t databaseConnectionIdentifier)
        : m_arguments(databaseConnectionIdentifier)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class GetAllDatabaseNames {
public:
    typedef std::tuple<uint64_t, const WebCore::SecurityOriginData&, const WebCore::SecurityOriginData&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetAllDatabaseNames"); }
    static const bool isSync = false;

    GetAllDatabaseNames(uint64_t serverConnectionIdentifier, const WebCore::SecurityOriginData& topOrigin, const WebCore::SecurityOriginData& openingOrigin, uint64_t callbackID)
        : m_arguments(serverConnectionIdentifier, topOrigin, openingOrigin, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

} // namespace WebIDBConnectionToClient
} // namespace Messages

#endif // ENABLE(INDEXED_DATABASE) && ENABLE(DATABASE_PROCESS)
