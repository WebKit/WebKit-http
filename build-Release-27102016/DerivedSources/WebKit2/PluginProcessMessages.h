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

#ifndef PluginProcessMessages_h
#define PluginProcessMessages_h

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "Arguments.h"
#include "MessageEncoder.h"
#include "StringReference.h"
#include <chrono>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebKit {
    struct PluginProcessCreationParameters;
}

namespace Messages {
namespace PluginProcess {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("PluginProcess");
}

class InitializePluginProcess {
public:
    typedef std::tuple<WebKit::PluginProcessCreationParameters> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("InitializePluginProcess"); }
    static const bool isSync = false;

    explicit InitializePluginProcess(const WebKit::PluginProcessCreationParameters& processCreationParameters)
        : m_arguments(processCreationParameters)
    {
    }

    const std::tuple<const WebKit::PluginProcessCreationParameters&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebKit::PluginProcessCreationParameters&> m_arguments;
};

class CreateWebProcessConnection {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CreateWebProcessConnection"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

class GetSitesWithData {
public:
    typedef std::tuple<uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetSitesWithData"); }
    static const bool isSync = false;

    explicit GetSitesWithData(uint64_t callbackID)
        : m_arguments(callbackID)
    {
    }

    const std::tuple<uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t> m_arguments;
};

class DeleteWebsiteData {
public:
    typedef std::tuple<std::chrono::system_clock::time_point, uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DeleteWebsiteData"); }
    static const bool isSync = false;

    DeleteWebsiteData(const std::chrono::system_clock::time_point& modifiedSince, uint64_t callbackID)
        : m_arguments(modifiedSince, callbackID)
    {
    }

    const std::tuple<const std::chrono::system_clock::time_point&, uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const std::chrono::system_clock::time_point&, uint64_t> m_arguments;
};

class DeleteWebsiteDataForHostNames {
public:
    typedef std::tuple<Vector<String>, uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DeleteWebsiteDataForHostNames"); }
    static const bool isSync = false;

    DeleteWebsiteDataForHostNames(const Vector<String>& hostNames, uint64_t callbackID)
        : m_arguments(hostNames, callbackID)
    {
    }

    const std::tuple<const Vector<String>&, uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const Vector<String>&, uint64_t> m_arguments;
};

class SetProcessSuppressionEnabled {
public:
    typedef std::tuple<bool> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetProcessSuppressionEnabled"); }
    static const bool isSync = false;

    explicit SetProcessSuppressionEnabled(bool flag)
        : m_arguments(flag)
    {
    }

    const std::tuple<bool>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<bool> m_arguments;
};

#if PLATFORM(COCOA)
class SetQOS {
public:
    typedef std::tuple<int, int> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetQOS"); }
    static const bool isSync = false;

    SetQOS(const int& latencyQOS, const int& throughputQOS)
        : m_arguments(latencyQOS, throughputQOS)
    {
    }

    const std::tuple<const int&, const int&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const int&, const int&> m_arguments;
};
#endif

} // namespace PluginProcess
} // namespace Messages

#endif // ENABLE(NETSCAPE_PLUGIN_API)

#endif // PluginProcessMessages_h
