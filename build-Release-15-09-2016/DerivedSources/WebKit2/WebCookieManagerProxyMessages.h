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

#ifndef WebCookieManagerProxyMessages_h
#define WebCookieManagerProxyMessages_h

#include "Arguments.h"
#include "MessageEncoder.h"
#include "StringReference.h"
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>


namespace Messages {
namespace WebCookieManagerProxy {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("WebCookieManagerProxy");
}

class DidGetHostnamesWithCookies {
public:
    typedef std::tuple<Vector<String>, uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidGetHostnamesWithCookies"); }
    static const bool isSync = false;

    DidGetHostnamesWithCookies(const Vector<String>& hostnames, uint64_t callbackID)
        : m_arguments(hostnames, callbackID)
    {
    }

    const std::tuple<const Vector<String>&, uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const Vector<String>&, uint64_t> m_arguments;
};

class DidGetHTTPCookieAcceptPolicy {
public:
    typedef std::tuple<uint32_t, uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidGetHTTPCookieAcceptPolicy"); }
    static const bool isSync = false;

    DidGetHTTPCookieAcceptPolicy(uint32_t policy, uint64_t callbackID)
        : m_arguments(policy, callbackID)
    {
    }

    const std::tuple<uint32_t, uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint32_t, uint64_t> m_arguments;
};

class DidGetCookies {
public:
    typedef std::tuple<Vector<String>, uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidGetCookies"); }
    static const bool isSync = false;

    DidGetCookies(const Vector<String>& cookies, uint64_t callbackID)
        : m_arguments(cookies, callbackID)
    {
    }

    const std::tuple<const Vector<String>&, uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const Vector<String>&, uint64_t> m_arguments;
};

class CookiesDidChange {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CookiesDidChange"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

} // namespace WebCookieManagerProxy
} // namespace Messages

#endif // WebCookieManagerProxyMessages_h
