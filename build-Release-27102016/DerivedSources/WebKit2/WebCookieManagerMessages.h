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

#ifndef WebCookieManagerMessages_h
#define WebCookieManagerMessages_h

#include "Arguments.h"
#include "MessageEncoder.h"
#include "StringReference.h"
#include <WebCore/Cookie.h>
#include <chrono>
#include <wtf/Vector.h>

namespace WTF {
    class String;
}

namespace WebCore {
    struct Cookie;
}

namespace Messages {
namespace WebCookieManager {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("WebCookieManager");
}

class GetHostnamesWithCookies {
public:
    typedef std::tuple<uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetHostnamesWithCookies"); }
    static const bool isSync = false;

    explicit GetHostnamesWithCookies(uint64_t callbackID)
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

class DeleteCookiesForHostname {
public:
    typedef std::tuple<String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DeleteCookiesForHostname"); }
    static const bool isSync = false;

    explicit DeleteCookiesForHostname(const String& hostname)
        : m_arguments(hostname)
    {
    }

    const std::tuple<const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&> m_arguments;
};

class DeleteAllCookies {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DeleteAllCookies"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

class DeleteAllCookiesModifiedSince {
public:
    typedef std::tuple<std::chrono::system_clock::time_point> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DeleteAllCookiesModifiedSince"); }
    static const bool isSync = false;

    explicit DeleteAllCookiesModifiedSince(const std::chrono::system_clock::time_point& time)
        : m_arguments(time)
    {
    }

    const std::tuple<const std::chrono::system_clock::time_point&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const std::chrono::system_clock::time_point&> m_arguments;
};

class AddCookie {
public:
    typedef std::tuple<WebCore::Cookie, String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("AddCookie"); }
    static const bool isSync = false;

    AddCookie(const WebCore::Cookie& cookie, const String& hostname)
        : m_arguments(cookie, hostname)
    {
    }

    const std::tuple<const WebCore::Cookie&, const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::Cookie&, const String&> m_arguments;
};

class SetHTTPCookieAcceptPolicy {
public:
    typedef std::tuple<uint32_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetHTTPCookieAcceptPolicy"); }
    static const bool isSync = false;

    explicit SetHTTPCookieAcceptPolicy(uint32_t policy)
        : m_arguments(policy)
    {
    }

    const std::tuple<uint32_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint32_t> m_arguments;
};

class GetHTTPCookieAcceptPolicy {
public:
    typedef std::tuple<uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetHTTPCookieAcceptPolicy"); }
    static const bool isSync = false;

    explicit GetHTTPCookieAcceptPolicy(uint64_t callbackID)
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

class SetCookies {
public:
    typedef std::tuple<Vector<WebCore::Cookie>> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetCookies"); }
    static const bool isSync = false;

    explicit SetCookies(const Vector<WebCore::Cookie>& cookies)
        : m_arguments(cookies)
    {
    }

    const std::tuple<const Vector<WebCore::Cookie>&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const Vector<WebCore::Cookie>&> m_arguments;
};

class GetCookies {
public:
    typedef std::tuple<uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetCookies"); }
    static const bool isSync = false;

    explicit GetCookies(uint64_t callbackID)
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

class StartObservingCookieChanges {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("StartObservingCookieChanges"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

class StopObservingCookieChanges {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("StopObservingCookieChanges"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

#if USE(SOUP)
class SetCookiePersistentStorage {
public:
    typedef std::tuple<String, uint32_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetCookiePersistentStorage"); }
    static const bool isSync = false;

    SetCookiePersistentStorage(const String& storagePath, uint32_t storageType)
        : m_arguments(storagePath, storageType)
    {
    }

    const std::tuple<const String&, uint32_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&, uint32_t> m_arguments;
};
#endif

} // namespace WebCookieManager
} // namespace Messages

#endif // WebCookieManagerMessages_h
