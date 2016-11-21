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

#ifndef WebProcessConnectionMessages_h
#define WebProcessConnectionMessages_h

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "Arguments.h"
#include "MessageEncoder.h"
#include "StringReference.h"
#include <wtf/ThreadSafeRefCounted.h>

namespace IPC {
    class Connection;
}

namespace WebKit {
    struct PluginCreationParameters;
}

namespace Messages {
namespace WebProcessConnection {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("WebProcessConnection");
}

class CreatePlugin {
public:
    typedef std::tuple<WebKit::PluginCreationParameters> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CreatePlugin"); }
    static const bool isSync = true;

    struct DelayedReply : public ThreadSafeRefCounted<DelayedReply> {
        DelayedReply(PassRefPtr<IPC::Connection>, std::unique_ptr<IPC::MessageEncoder>);
        ~DelayedReply();

        bool send(bool creationResult, bool wantsWheelEvents, uint32_t remoteLayerClientID);

    private:
        RefPtr<IPC::Connection> m_connection;
        std::unique_ptr<IPC::MessageEncoder> m_encoder;
    };

    typedef IPC::Arguments<bool&, bool&, uint32_t&> Reply;
    explicit CreatePlugin(const WebKit::PluginCreationParameters& pluginCreationParameters)
        : m_arguments(pluginCreationParameters)
    {
    }

    const std::tuple<const WebKit::PluginCreationParameters&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebKit::PluginCreationParameters&> m_arguments;
};

class CreatePluginAsynchronously {
public:
    typedef std::tuple<WebKit::PluginCreationParameters> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CreatePluginAsynchronously"); }
    static const bool isSync = false;

    explicit CreatePluginAsynchronously(const WebKit::PluginCreationParameters& pluginCreationParameters)
        : m_arguments(pluginCreationParameters)
    {
    }

    const std::tuple<const WebKit::PluginCreationParameters&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebKit::PluginCreationParameters&> m_arguments;
};

class DestroyPlugin {
public:
    typedef std::tuple<uint64_t, bool> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DestroyPlugin"); }
    static const bool isSync = true;

    struct DelayedReply : public ThreadSafeRefCounted<DelayedReply> {
        DelayedReply(PassRefPtr<IPC::Connection>, std::unique_ptr<IPC::MessageEncoder>);
        ~DelayedReply();

        bool send();

    private:
        RefPtr<IPC::Connection> m_connection;
        std::unique_ptr<IPC::MessageEncoder> m_encoder;
    };

    typedef IPC::Arguments<> Reply;
    DestroyPlugin(uint64_t pluginInstanceID, bool asynchronousCreationIncomplete)
        : m_arguments(pluginInstanceID, asynchronousCreationIncomplete)
    {
    }

    const std::tuple<uint64_t, bool>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t, bool> m_arguments;
};

} // namespace WebProcessConnection
} // namespace Messages

#endif // ENABLE(NETSCAPE_PLUGIN_API)

#endif // WebProcessConnectionMessages_h
