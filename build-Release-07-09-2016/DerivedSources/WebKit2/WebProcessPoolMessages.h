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

#ifndef WebProcessPoolMessages_h
#define WebProcessPoolMessages_h

#include "Arguments.h"
#include "MessageEncoder.h"
#include "StringReference.h"

namespace WTF {
    class String;
}

namespace WebCore {
    class SessionID;
}

namespace WebKit {
    class UserData;
    struct StatisticsData;
}

namespace Messages {
namespace WebProcessPool {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("WebProcessPool");
}

class HandleMessage {
public:
    typedef std::tuple<String, WebKit::UserData> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("HandleMessage"); }
    static const bool isSync = false;

    HandleMessage(const String& messageName, const WebKit::UserData& messageBody)
        : m_arguments(messageName, messageBody)
    {
    }

    const std::tuple<const String&, const WebKit::UserData&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&, const WebKit::UserData&> m_arguments;
};

class HandleSynchronousMessage {
public:
    typedef std::tuple<String, WebKit::UserData> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("HandleSynchronousMessage"); }
    static const bool isSync = true;

    typedef IPC::Arguments<WebKit::UserData&> Reply;
    HandleSynchronousMessage(const String& messageName, const WebKit::UserData& messageBody)
        : m_arguments(messageName, messageBody)
    {
    }

    const std::tuple<const String&, const WebKit::UserData&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&, const WebKit::UserData&> m_arguments;
};

class DidGetStatistics {
public:
    typedef std::tuple<WebKit::StatisticsData, uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidGetStatistics"); }
    static const bool isSync = false;

    DidGetStatistics(const WebKit::StatisticsData& statisticsData, uint64_t callbackID)
        : m_arguments(statisticsData, callbackID)
    {
    }

    const std::tuple<const WebKit::StatisticsData&, uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebKit::StatisticsData&, uint64_t> m_arguments;
};

class AddPlugInAutoStartOriginHash {
public:
    typedef std::tuple<String, uint32_t, WebCore::SessionID> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("AddPlugInAutoStartOriginHash"); }
    static const bool isSync = false;

    AddPlugInAutoStartOriginHash(const String& pageOrigin, uint32_t hash, const WebCore::SessionID& sessionID)
        : m_arguments(pageOrigin, hash, sessionID)
    {
    }

    const std::tuple<const String&, uint32_t, const WebCore::SessionID&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&, uint32_t, const WebCore::SessionID&> m_arguments;
};

class PlugInDidReceiveUserInteraction {
public:
    typedef std::tuple<uint32_t, WebCore::SessionID> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PlugInDidReceiveUserInteraction"); }
    static const bool isSync = false;

    PlugInDidReceiveUserInteraction(uint32_t hash, const WebCore::SessionID& sessionID)
        : m_arguments(hash, sessionID)
    {
    }

    const std::tuple<uint32_t, const WebCore::SessionID&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint32_t, const WebCore::SessionID&> m_arguments;
};

} // namespace WebProcessPool
} // namespace Messages

#endif // WebProcessPoolMessages_h
