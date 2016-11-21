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
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace IPC {
    class Attachment;
}

namespace WTF {
    class String;
}

namespace WebCore {
    class ProtectionSpace;
    class AuthenticationChallenge;
}

namespace WebKit {
    struct WebsiteData;
}

namespace Messages {
namespace NetworkProcessProxy {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("NetworkProcessProxy");
}

class DidCreateNetworkConnectionToWebProcess {
public:
    typedef std::tuple<const IPC::Attachment&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidCreateNetworkConnectionToWebProcess"); }
    static const bool isSync = false;

    explicit DidCreateNetworkConnectionToWebProcess(const IPC::Attachment& connectionIdentifier)
        : m_arguments(connectionIdentifier)
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
    typedef std::tuple<uint64_t, uint64_t, const WebCore::AuthenticationChallenge&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidReceiveAuthenticationChallenge"); }
    static const bool isSync = false;

    DidReceiveAuthenticationChallenge(uint64_t pageID, uint64_t frameID, const WebCore::AuthenticationChallenge& challenge, uint64_t challengeID)
        : m_arguments(pageID, frameID, challenge, challengeID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidFetchWebsiteData {
public:
    typedef std::tuple<uint64_t, const WebKit::WebsiteData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidFetchWebsiteData"); }
    static const bool isSync = false;

    DidFetchWebsiteData(uint64_t callbackID, const WebKit::WebsiteData& websiteData)
        : m_arguments(callbackID, websiteData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidDeleteWebsiteData {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidDeleteWebsiteData"); }
    static const bool isSync = false;

    explicit DidDeleteWebsiteData(uint64_t callbackID)
        : m_arguments(callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidDeleteWebsiteDataForOrigins {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidDeleteWebsiteDataForOrigins"); }
    static const bool isSync = false;

    explicit DidDeleteWebsiteDataForOrigins(uint64_t callbackID)
        : m_arguments(callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class GrantSandboxExtensionsToDatabaseProcessForBlobs {
public:
    typedef std::tuple<uint64_t, const Vector<String>&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GrantSandboxExtensionsToDatabaseProcessForBlobs"); }
    static const bool isSync = false;

    GrantSandboxExtensionsToDatabaseProcessForBlobs(uint64_t requestID, const Vector<String>& paths)
        : m_arguments(requestID, paths)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ProcessReadyToSuspend {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ProcessReadyToSuspend"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetIsHoldingLockedFiles {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetIsHoldingLockedFiles"); }
    static const bool isSync = false;

    explicit SetIsHoldingLockedFiles(bool isHoldingLockedFiles)
        : m_arguments(isHoldingLockedFiles)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class LogSampledDiagnosticMessage {
public:
    typedef std::tuple<uint64_t, const String&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("LogSampledDiagnosticMessage"); }
    static const bool isSync = false;

    LogSampledDiagnosticMessage(uint64_t pageID, const String& message, const String& description)
        : m_arguments(pageID, message, description)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class LogSampledDiagnosticMessageWithResult {
public:
    typedef std::tuple<uint64_t, const String&, const String&, uint32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("LogSampledDiagnosticMessageWithResult"); }
    static const bool isSync = false;

    LogSampledDiagnosticMessageWithResult(uint64_t pageID, const String& message, const String& description, uint32_t result)
        : m_arguments(pageID, message, description, result)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class LogSampledDiagnosticMessageWithValue {
public:
    typedef std::tuple<uint64_t, const String&, const String&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("LogSampledDiagnosticMessageWithValue"); }
    static const bool isSync = false;

    LogSampledDiagnosticMessageWithValue(uint64_t pageID, const String& message, const String& description, const String& value)
        : m_arguments(pageID, message, description, value)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
class CanAuthenticateAgainstProtectionSpace {
public:
    typedef std::tuple<uint64_t, uint64_t, uint64_t, const WebCore::ProtectionSpace&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CanAuthenticateAgainstProtectionSpace"); }
    static const bool isSync = false;

    CanAuthenticateAgainstProtectionSpace(uint64_t loaderID, uint64_t pageID, uint64_t frameID, const WebCore::ProtectionSpace& protectionSpace)
        : m_arguments(loaderID, pageID, frameID, protectionSpace)
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

} // namespace NetworkProcessProxy
} // namespace Messages
