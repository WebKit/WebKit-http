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

#include "ArgumentCoders.h"
#include <wtf/HashMap.h>
#include <wtf/text/WTFString.h>

namespace WTF {
    class String;
}

namespace WebCore {
    struct SecurityOriginData;
}

namespace Messages {
namespace StorageManager {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("StorageManager");
}

class CreateLocalStorageMap {
public:
    typedef std::tuple<uint64_t, uint64_t, const WebCore::SecurityOriginData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CreateLocalStorageMap"); }
    static const bool isSync = false;

    CreateLocalStorageMap(uint64_t storageMapID, uint64_t storageNamespaceID, const WebCore::SecurityOriginData& securityOriginData)
        : m_arguments(storageMapID, storageNamespaceID, securityOriginData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class CreateTransientLocalStorageMap {
public:
    typedef std::tuple<uint64_t, uint64_t, const WebCore::SecurityOriginData&, const WebCore::SecurityOriginData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CreateTransientLocalStorageMap"); }
    static const bool isSync = false;

    CreateTransientLocalStorageMap(uint64_t storageMapID, uint64_t storageNamespaceID, const WebCore::SecurityOriginData& topLevelSecurityOriginData, const WebCore::SecurityOriginData& securityOriginData)
        : m_arguments(storageMapID, storageNamespaceID, topLevelSecurityOriginData, securityOriginData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class CreateSessionStorageMap {
public:
    typedef std::tuple<uint64_t, uint64_t, const WebCore::SecurityOriginData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CreateSessionStorageMap"); }
    static const bool isSync = false;

    CreateSessionStorageMap(uint64_t storageMapID, uint64_t storageNamespaceID, const WebCore::SecurityOriginData& securityOriginData)
        : m_arguments(storageMapID, storageNamespaceID, securityOriginData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DestroyStorageMap {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DestroyStorageMap"); }
    static const bool isSync = false;

    explicit DestroyStorageMap(uint64_t storageMapID)
        : m_arguments(storageMapID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class GetValues {
public:
    typedef std::tuple<uint64_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetValues"); }
    static const bool isSync = true;

    typedef std::tuple<HashMap<String, String>&> Reply;
    GetValues(uint64_t storageMapID, uint64_t storageMapSeed)
        : m_arguments(storageMapID, storageMapSeed)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetItem {
public:
    typedef std::tuple<uint64_t, uint64_t, uint64_t, const String&, const String&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetItem"); }
    static const bool isSync = false;

    SetItem(uint64_t storageMapID, uint64_t sourceStorageAreaID, uint64_t storageMapSeed, const String& key, const String& value, const String& urlString)
        : m_arguments(storageMapID, sourceStorageAreaID, storageMapSeed, key, value, urlString)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class RemoveItem {
public:
    typedef std::tuple<uint64_t, uint64_t, uint64_t, const String&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RemoveItem"); }
    static const bool isSync = false;

    RemoveItem(uint64_t storageMapID, uint64_t sourceStorageAreaID, uint64_t storageMapSeed, const String& key, const String& urlString)
        : m_arguments(storageMapID, sourceStorageAreaID, storageMapSeed, key, urlString)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class Clear {
public:
    typedef std::tuple<uint64_t, uint64_t, uint64_t, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("Clear"); }
    static const bool isSync = false;

    Clear(uint64_t storageMapID, uint64_t sourceStorageAreaID, uint64_t storageMapSeed, const String& urlString)
        : m_arguments(storageMapID, sourceStorageAreaID, storageMapSeed, urlString)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

} // namespace StorageManager
} // namespace Messages
