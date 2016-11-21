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
#include "SandboxExtension.h"

namespace IPC {
    class DataReference;
}

namespace WTF {
    class String;
}

namespace WebCore {
    class ResourceResponse;
    class ProtectionSpace;
    class AuthenticationChallenge;
    class ResourceRequest;
    class ResourceError;
}

namespace WebKit {
    class DownloadID;
}

namespace Messages {
namespace DownloadProxy {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("DownloadProxy");
}

class DidStart {
public:
    typedef std::tuple<const WebCore::ResourceRequest&, const AtomicString&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidStart"); }
    static const bool isSync = false;

    DidStart(const WebCore::ResourceRequest& request, const AtomicString& suggestedFilename)
        : m_arguments(request, suggestedFilename)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidReceiveAuthenticationChallenge {
public:
    typedef std::tuple<const WebCore::AuthenticationChallenge&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidReceiveAuthenticationChallenge"); }
    static const bool isSync = false;

    DidReceiveAuthenticationChallenge(const WebCore::AuthenticationChallenge& challenge, uint64_t challengeID)
        : m_arguments(challenge, challengeID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

#if USE(NETWORK_SESSION)
class WillSendRequest {
public:
    typedef std::tuple<const WebCore::ResourceRequest&, const WebCore::ResourceResponse&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("WillSendRequest"); }
    static const bool isSync = false;

    WillSendRequest(const WebCore::ResourceRequest& redirectRequest, const WebCore::ResourceResponse& redirectResponse)
        : m_arguments(redirectRequest, redirectResponse)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if USE(NETWORK_SESSION)
class CanAuthenticateAgainstProtectionSpace {
public:
    typedef std::tuple<const WebCore::ProtectionSpace&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CanAuthenticateAgainstProtectionSpace"); }
    static const bool isSync = false;

    explicit CanAuthenticateAgainstProtectionSpace(const WebCore::ProtectionSpace& protectionSpace)
        : m_arguments(protectionSpace)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if USE(NETWORK_SESSION)
class DecideDestinationWithSuggestedFilenameAsync {
public:
    typedef std::tuple<const WebKit::DownloadID&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DecideDestinationWithSuggestedFilenameAsync"); }
    static const bool isSync = false;

    DecideDestinationWithSuggestedFilenameAsync(const WebKit::DownloadID& downloadID, const String& suggestedFilename)
        : m_arguments(downloadID, suggestedFilename)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

class DidReceiveResponse {
public:
    typedef std::tuple<const WebCore::ResourceResponse&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidReceiveResponse"); }
    static const bool isSync = false;

    explicit DidReceiveResponse(const WebCore::ResourceResponse& response)
        : m_arguments(response)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidReceiveData {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidReceiveData"); }
    static const bool isSync = false;

    explicit DidReceiveData(uint64_t length)
        : m_arguments(length)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ShouldDecodeSourceDataOfMIMEType {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ShouldDecodeSourceDataOfMIMEType"); }
    static const bool isSync = true;

    typedef std::tuple<bool&> Reply;
    explicit ShouldDecodeSourceDataOfMIMEType(const String& mimeType)
        : m_arguments(mimeType)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

#if !USE(NETWORK_SESSION)
class DecideDestinationWithSuggestedFilename {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DecideDestinationWithSuggestedFilename"); }
    static const bool isSync = true;

    typedef std::tuple<String&, bool&, WebKit::SandboxExtension::Handle&> Reply;
    explicit DecideDestinationWithSuggestedFilename(const String& filename)
        : m_arguments(filename)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

class DidCreateDestination {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidCreateDestination"); }
    static const bool isSync = false;

    explicit DidCreateDestination(const String& path)
        : m_arguments(path)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidFinish {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidFinish"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidFail {
public:
    typedef std::tuple<const WebCore::ResourceError&, const IPC::DataReference&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidFail"); }
    static const bool isSync = false;

    DidFail(const WebCore::ResourceError& error, const IPC::DataReference& resumeData)
        : m_arguments(error, resumeData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidCancel {
public:
    typedef std::tuple<const IPC::DataReference&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidCancel"); }
    static const bool isSync = false;

    explicit DidCancel(const IPC::DataReference& resumeData)
        : m_arguments(resumeData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

} // namespace DownloadProxy
} // namespace Messages
