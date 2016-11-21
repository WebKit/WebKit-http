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

#ifndef StorageAreaMapMessages_h
#define StorageAreaMapMessages_h

#include "Arguments.h"
#include "MessageEncoder.h"
#include "StringReference.h"

namespace WTF {
    class String;
}

namespace Messages {
namespace StorageAreaMap {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("StorageAreaMap");
}

class DidGetValues {
public:
    typedef std::tuple<uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidGetValues"); }
    static const bool isSync = false;

    explicit DidGetValues(uint64_t storageMapSeed)
        : m_arguments(storageMapSeed)
    {
    }

    const std::tuple<uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t> m_arguments;
};

class DidSetItem {
public:
    typedef std::tuple<uint64_t, String, bool> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidSetItem"); }
    static const bool isSync = false;

    DidSetItem(uint64_t storageMapSeed, const String& key, bool quotaException)
        : m_arguments(storageMapSeed, key, quotaException)
    {
    }

    const std::tuple<uint64_t, const String&, bool>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t, const String&, bool> m_arguments;
};

class DidRemoveItem {
public:
    typedef std::tuple<uint64_t, String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidRemoveItem"); }
    static const bool isSync = false;

    DidRemoveItem(uint64_t storageMapSeed, const String& key)
        : m_arguments(storageMapSeed, key)
    {
    }

    const std::tuple<uint64_t, const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t, const String&> m_arguments;
};

class DidClear {
public:
    typedef std::tuple<uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidClear"); }
    static const bool isSync = false;

    explicit DidClear(uint64_t storageMapSeed)
        : m_arguments(storageMapSeed)
    {
    }

    const std::tuple<uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t> m_arguments;
};

class DispatchStorageEvent {
public:
    typedef std::tuple<uint64_t, String, String, String, String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DispatchStorageEvent"); }
    static const bool isSync = false;

    DispatchStorageEvent(uint64_t sourceStorageAreaID, const String& key, const String& oldValue, const String& newValue, const String& urlString)
        : m_arguments(sourceStorageAreaID, key, oldValue, newValue, urlString)
    {
    }

    const std::tuple<uint64_t, const String&, const String&, const String&, const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t, const String&, const String&, const String&, const String&> m_arguments;
};

class ClearCache {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ClearCache"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

} // namespace StorageAreaMap
} // namespace Messages

#endif // StorageAreaMapMessages_h
