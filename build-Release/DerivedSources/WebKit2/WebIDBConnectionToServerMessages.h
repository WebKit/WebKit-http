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
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {
    class IDBResultData;
    class IDBResourceIdentifier;
    class IDBError;
}

namespace WebKit {
    class WebIDBResult;
}

namespace Messages {
namespace WebIDBConnectionToServer {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("WebIDBConnectionToServer");
}

class DidDeleteDatabase {
public:
    typedef std::tuple<const WebCore::IDBResultData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidDeleteDatabase"); }
    static const bool isSync = false;

    explicit DidDeleteDatabase(const WebCore::IDBResultData& result)
        : m_arguments(result)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidOpenDatabase {
public:
    typedef std::tuple<const WebCore::IDBResultData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidOpenDatabase"); }
    static const bool isSync = false;

    explicit DidOpenDatabase(const WebCore::IDBResultData& result)
        : m_arguments(result)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidAbortTransaction {
public:
    typedef std::tuple<const WebCore::IDBResourceIdentifier&, const WebCore::IDBError&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidAbortTransaction"); }
    static const bool isSync = false;

    DidAbortTransaction(const WebCore::IDBResourceIdentifier& transactionIdentifier, const WebCore::IDBError& error)
        : m_arguments(transactionIdentifier, error)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidCommitTransaction {
public:
    typedef std::tuple<const WebCore::IDBResourceIdentifier&, const WebCore::IDBError&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidCommitTransaction"); }
    static const bool isSync = false;

    DidCommitTransaction(const WebCore::IDBResourceIdentifier& transactionIdentifier, const WebCore::IDBError& error)
        : m_arguments(transactionIdentifier, error)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidCreateObjectStore {
public:
    typedef std::tuple<const WebCore::IDBResultData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidCreateObjectStore"); }
    static const bool isSync = false;

    explicit DidCreateObjectStore(const WebCore::IDBResultData& result)
        : m_arguments(result)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidDeleteObjectStore {
public:
    typedef std::tuple<const WebCore::IDBResultData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidDeleteObjectStore"); }
    static const bool isSync = false;

    explicit DidDeleteObjectStore(const WebCore::IDBResultData& result)
        : m_arguments(result)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidClearObjectStore {
public:
    typedef std::tuple<const WebCore::IDBResultData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidClearObjectStore"); }
    static const bool isSync = false;

    explicit DidClearObjectStore(const WebCore::IDBResultData& result)
        : m_arguments(result)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidCreateIndex {
public:
    typedef std::tuple<const WebCore::IDBResultData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidCreateIndex"); }
    static const bool isSync = false;

    explicit DidCreateIndex(const WebCore::IDBResultData& result)
        : m_arguments(result)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidDeleteIndex {
public:
    typedef std::tuple<const WebCore::IDBResultData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidDeleteIndex"); }
    static const bool isSync = false;

    explicit DidDeleteIndex(const WebCore::IDBResultData& result)
        : m_arguments(result)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidPutOrAdd {
public:
    typedef std::tuple<const WebCore::IDBResultData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidPutOrAdd"); }
    static const bool isSync = false;

    explicit DidPutOrAdd(const WebCore::IDBResultData& result)
        : m_arguments(result)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidGetRecord {
public:
    typedef std::tuple<const WebKit::WebIDBResult&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidGetRecord"); }
    static const bool isSync = false;

    explicit DidGetRecord(const WebKit::WebIDBResult& result)
        : m_arguments(result)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidGetCount {
public:
    typedef std::tuple<const WebCore::IDBResultData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidGetCount"); }
    static const bool isSync = false;

    explicit DidGetCount(const WebCore::IDBResultData& result)
        : m_arguments(result)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidDeleteRecord {
public:
    typedef std::tuple<const WebCore::IDBResultData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidDeleteRecord"); }
    static const bool isSync = false;

    explicit DidDeleteRecord(const WebCore::IDBResultData& result)
        : m_arguments(result)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidOpenCursor {
public:
    typedef std::tuple<const WebKit::WebIDBResult&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidOpenCursor"); }
    static const bool isSync = false;

    explicit DidOpenCursor(const WebKit::WebIDBResult& result)
        : m_arguments(result)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidIterateCursor {
public:
    typedef std::tuple<const WebKit::WebIDBResult&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidIterateCursor"); }
    static const bool isSync = false;

    explicit DidIterateCursor(const WebKit::WebIDBResult& result)
        : m_arguments(result)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class FireVersionChangeEvent {
public:
    typedef std::tuple<uint64_t, const WebCore::IDBResourceIdentifier&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("FireVersionChangeEvent"); }
    static const bool isSync = false;

    FireVersionChangeEvent(uint64_t databaseConnectionIdentifier, const WebCore::IDBResourceIdentifier& requestIdentifier, uint64_t requestedVersion)
        : m_arguments(databaseConnectionIdentifier, requestIdentifier, requestedVersion)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidStartTransaction {
public:
    typedef std::tuple<const WebCore::IDBResourceIdentifier&, const WebCore::IDBError&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidStartTransaction"); }
    static const bool isSync = false;

    DidStartTransaction(const WebCore::IDBResourceIdentifier& transactionIdentifier, const WebCore::IDBError& error)
        : m_arguments(transactionIdentifier, error)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidCloseFromServer {
public:
    typedef std::tuple<uint64_t, const WebCore::IDBError&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidCloseFromServer"); }
    static const bool isSync = false;

    DidCloseFromServer(uint64_t databaseConnectionIdentifier, const WebCore::IDBError& error)
        : m_arguments(databaseConnectionIdentifier, error)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class NotifyOpenDBRequestBlocked {
public:
    typedef std::tuple<const WebCore::IDBResourceIdentifier&, uint64_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("NotifyOpenDBRequestBlocked"); }
    static const bool isSync = false;

    NotifyOpenDBRequestBlocked(const WebCore::IDBResourceIdentifier& requestIdentifier, uint64_t oldVersion, uint64_t newVersion)
        : m_arguments(requestIdentifier, oldVersion, newVersion)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidGetAllDatabaseNames {
public:
    typedef std::tuple<uint64_t, const Vector<String>&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidGetAllDatabaseNames"); }
    static const bool isSync = false;

    DidGetAllDatabaseNames(uint64_t callbackID, const Vector<String>& databaseNames)
        : m_arguments(callbackID, databaseNames)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

} // namespace WebIDBConnectionToServer
} // namespace Messages

#endif // ENABLE(INDEXED_DATABASE) && ENABLE(DATABASE_PROCESS)
