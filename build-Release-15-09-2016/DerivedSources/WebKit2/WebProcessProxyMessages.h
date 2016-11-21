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

#ifndef WebProcessProxyMessages_h
#define WebProcessProxyMessages_h

#include "Arguments.h"
#include "MessageEncoder.h"
#include "StringReference.h"
#include <WebCore/PluginData.h>
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/Vector.h>

namespace IPC {
    class Connection;
    class Attachment;
}

namespace WTF {
    class String;
}

namespace WebKit {
    struct WebsiteData;
    struct PageState;
}

namespace Messages {
namespace WebProcessProxy {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("WebProcessProxy");
}

class AddBackForwardItem {
public:
    typedef std::tuple<uint64_t, uint64_t, WebKit::PageState> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("AddBackForwardItem"); }
    static const bool isSync = false;

    AddBackForwardItem(uint64_t itemID, uint64_t pageID, const WebKit::PageState& pageState)
        : m_arguments(itemID, pageID, pageState)
    {
    }

    const std::tuple<uint64_t, uint64_t, const WebKit::PageState&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t, uint64_t, const WebKit::PageState&> m_arguments;
};

class DidDestroyFrame {
public:
    typedef std::tuple<uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidDestroyFrame"); }
    static const bool isSync = false;

    explicit DidDestroyFrame(uint64_t frameID)
        : m_arguments(frameID)
    {
    }

    const std::tuple<uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t> m_arguments;
};

class ShouldTerminate {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ShouldTerminate"); }
    static const bool isSync = true;

    typedef IPC::Arguments<bool&> Reply;
    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

class EnableSuddenTermination {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("EnableSuddenTermination"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

class DisableSuddenTermination {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DisableSuddenTermination"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

class DidFetchWebsiteData {
public:
    typedef std::tuple<uint64_t, WebKit::WebsiteData> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidFetchWebsiteData"); }
    static const bool isSync = false;

    DidFetchWebsiteData(uint64_t callbackID, const WebKit::WebsiteData& websiteData)
        : m_arguments(callbackID, websiteData)
    {
    }

    const std::tuple<uint64_t, const WebKit::WebsiteData&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t, const WebKit::WebsiteData&> m_arguments;
};

class DidDeleteWebsiteData {
public:
    typedef std::tuple<uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidDeleteWebsiteData"); }
    static const bool isSync = false;

    explicit DidDeleteWebsiteData(uint64_t callbackID)
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

class DidDeleteWebsiteDataForOrigins {
public:
    typedef std::tuple<uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidDeleteWebsiteDataForOrigins"); }
    static const bool isSync = false;

    explicit DidDeleteWebsiteDataForOrigins(uint64_t callbackID)
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

#if ENABLE(NETSCAPE_PLUGIN_API)
class GetPlugins {
public:
    typedef std::tuple<bool> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetPlugins"); }
    static const bool isSync = true;

    typedef IPC::Arguments<Vector<WebCore::PluginInfo>&, Vector<WebCore::PluginInfo>&> Reply;
    explicit GetPlugins(bool refresh)
        : m_arguments(refresh)
    {
    }

    const std::tuple<bool>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<bool> m_arguments;
};
#endif

#if ENABLE(NETSCAPE_PLUGIN_API)
class GetPluginProcessConnection {
public:
    typedef std::tuple<uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetPluginProcessConnection"); }
    static const bool isSync = true;

    struct DelayedReply : public ThreadSafeRefCounted<DelayedReply> {
        DelayedReply(PassRefPtr<IPC::Connection>, std::unique_ptr<IPC::MessageEncoder>);
        ~DelayedReply();

        bool send(const IPC::Attachment& connectionHandle, bool supportsAsynchronousInitialization);

    private:
        RefPtr<IPC::Connection> m_connection;
        std::unique_ptr<IPC::MessageEncoder> m_encoder;
    };

    typedef IPC::Arguments<IPC::Attachment&, bool&> Reply;
    explicit GetPluginProcessConnection(uint64_t pluginProcessToken)
        : m_arguments(pluginProcessToken)
    {
    }

    const std::tuple<uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t> m_arguments;
};
#endif

class GetNetworkProcessConnection {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetNetworkProcessConnection"); }
    static const bool isSync = true;

    struct DelayedReply : public ThreadSafeRefCounted<DelayedReply> {
        DelayedReply(PassRefPtr<IPC::Connection>, std::unique_ptr<IPC::MessageEncoder>);
        ~DelayedReply();

        bool send(const IPC::Attachment& connectionHandle);

    private:
        RefPtr<IPC::Connection> m_connection;
        std::unique_ptr<IPC::MessageEncoder> m_encoder;
    };

    typedef IPC::Arguments<IPC::Attachment&> Reply;
    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

#if ENABLE(DATABASE_PROCESS)
class GetDatabaseProcessConnection {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetDatabaseProcessConnection"); }
    static const bool isSync = true;

    struct DelayedReply : public ThreadSafeRefCounted<DelayedReply> {
        DelayedReply(PassRefPtr<IPC::Connection>, std::unique_ptr<IPC::MessageEncoder>);
        ~DelayedReply();

        bool send(const IPC::Attachment& connectionHandle);

    private:
        RefPtr<IPC::Connection> m_connection;
        std::unique_ptr<IPC::MessageEncoder> m_encoder;
    };

    typedef IPC::Arguments<IPC::Attachment&> Reply;
    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};
#endif

class ProcessReadyToSuspend {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ProcessReadyToSuspend"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

class DidCancelProcessSuspension {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidCancelProcessSuspension"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

class SetIsHoldingLockedFiles {
public:
    typedef std::tuple<bool> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetIsHoldingLockedFiles"); }
    static const bool isSync = false;

    explicit SetIsHoldingLockedFiles(bool isHoldingLockedFiles)
        : m_arguments(isHoldingLockedFiles)
    {
    }

    const std::tuple<bool>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<bool> m_arguments;
};

class RetainIconForPageURL {
public:
    typedef std::tuple<String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RetainIconForPageURL"); }
    static const bool isSync = false;

    explicit RetainIconForPageURL(const String& pageURL)
        : m_arguments(pageURL)
    {
    }

    const std::tuple<const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&> m_arguments;
};

class ReleaseIconForPageURL {
public:
    typedef std::tuple<String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ReleaseIconForPageURL"); }
    static const bool isSync = false;

    explicit ReleaseIconForPageURL(const String& pageURL)
        : m_arguments(pageURL)
    {
    }

    const std::tuple<const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&> m_arguments;
};

class DidReceiveMainThreadPing {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidReceiveMainThreadPing"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

} // namespace WebProcessProxy
} // namespace Messages

#endif // WebProcessProxyMessages_h
