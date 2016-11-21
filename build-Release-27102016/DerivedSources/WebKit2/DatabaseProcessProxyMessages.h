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

#ifndef DatabaseProcessProxyMessages_h
#define DatabaseProcessProxyMessages_h

#if ENABLE(DATABASE_PROCESS)

#include "Arguments.h"
#include "MessageEncoder.h"
#include "StringReference.h"
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace IPC {
    class Attachment;
}

namespace WebKit {
    struct WebsiteData;
}

namespace Messages {
namespace DatabaseProcessProxy {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("DatabaseProcessProxy");
}

class DidCreateDatabaseToWebProcessConnection {
public:
    typedef std::tuple<IPC::Attachment> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidCreateDatabaseToWebProcessConnection"); }
    static const bool isSync = false;

    explicit DidCreateDatabaseToWebProcessConnection(const IPC::Attachment& connectionIdentifier)
        : m_arguments(connectionIdentifier)
    {
    }

    const std::tuple<const IPC::Attachment&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const IPC::Attachment&> m_arguments;
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

class GetSandboxExtensionsForBlobFiles {
public:
    typedef std::tuple<uint64_t, Vector<String>> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetSandboxExtensionsForBlobFiles"); }
    static const bool isSync = false;

    GetSandboxExtensionsForBlobFiles(uint64_t requestID, const Vector<String>& paths)
        : m_arguments(requestID, paths)
    {
    }

    const std::tuple<uint64_t, const Vector<String>&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t, const Vector<String>&> m_arguments;
};

} // namespace DatabaseProcessProxy
} // namespace Messages

#endif // ENABLE(DATABASE_PROCESS)

#endif // DatabaseProcessProxyMessages_h
