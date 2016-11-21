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

#ifndef AuthenticationManagerMessages_h
#define AuthenticationManagerMessages_h

#include "Arguments.h"
#include "MessageEncoder.h"
#include "StringReference.h"

namespace WebCore {
    class CertificateInfo;
    class Credential;
}

namespace Messages {
namespace AuthenticationManager {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("AuthenticationManager");
}

class UseCredentialForChallenge {
public:
    typedef std::tuple<uint64_t, WebCore::Credential, WebCore::CertificateInfo> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("UseCredentialForChallenge"); }
    static const bool isSync = false;

    UseCredentialForChallenge(uint64_t challengeID, const WebCore::Credential& credential, const WebCore::CertificateInfo& certificate)
        : m_arguments(challengeID, credential, certificate)
    {
    }

    const std::tuple<uint64_t, const WebCore::Credential&, const WebCore::CertificateInfo&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t, const WebCore::Credential&, const WebCore::CertificateInfo&> m_arguments;
};

class ContinueWithoutCredentialForChallenge {
public:
    typedef std::tuple<uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ContinueWithoutCredentialForChallenge"); }
    static const bool isSync = false;

    explicit ContinueWithoutCredentialForChallenge(uint64_t challengeID)
        : m_arguments(challengeID)
    {
    }

    const std::tuple<uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t> m_arguments;
};

class CancelChallenge {
public:
    typedef std::tuple<uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CancelChallenge"); }
    static const bool isSync = false;

    explicit CancelChallenge(uint64_t challengeID)
        : m_arguments(challengeID)
    {
    }

    const std::tuple<uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t> m_arguments;
};

class PerformDefaultHandling {
public:
    typedef std::tuple<uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PerformDefaultHandling"); }
    static const bool isSync = false;

    explicit PerformDefaultHandling(uint64_t challengeID)
        : m_arguments(challengeID)
    {
    }

    const std::tuple<uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t> m_arguments;
};

class RejectProtectionSpaceAndContinue {
public:
    typedef std::tuple<uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RejectProtectionSpaceAndContinue"); }
    static const bool isSync = false;

    explicit RejectProtectionSpaceAndContinue(uint64_t challengeID)
        : m_arguments(challengeID)
    {
    }

    const std::tuple<uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t> m_arguments;
};

} // namespace AuthenticationManager
} // namespace Messages

#endif // AuthenticationManagerMessages_h
