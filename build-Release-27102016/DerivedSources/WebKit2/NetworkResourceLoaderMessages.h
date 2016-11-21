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

#ifndef NetworkResourceLoaderMessages_h
#define NetworkResourceLoaderMessages_h

#include "Arguments.h"
#include "MessageEncoder.h"
#include "StringReference.h"

namespace WebCore {
    class ResourceRequest;
}

namespace Messages {
namespace NetworkResourceLoader {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("NetworkResourceLoader");
}

class ContinueWillSendRequest {
public:
    typedef std::tuple<WebCore::ResourceRequest> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ContinueWillSendRequest"); }
    static const bool isSync = false;

    explicit ContinueWillSendRequest(const WebCore::ResourceRequest& request)
        : m_arguments(request)
    {
    }

    const std::tuple<const WebCore::ResourceRequest&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::ResourceRequest&> m_arguments;
};

class ContinueDidReceiveResponse {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ContinueDidReceiveResponse"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
class ContinueCanAuthenticateAgainstProtectionSpace {
public:
    typedef std::tuple<bool> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ContinueCanAuthenticateAgainstProtectionSpace"); }
    static const bool isSync = false;

    explicit ContinueCanAuthenticateAgainstProtectionSpace(bool canAuthenticate)
        : m_arguments(canAuthenticate)
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

} // namespace NetworkResourceLoader
} // namespace Messages

#endif // NetworkResourceLoaderMessages_h
