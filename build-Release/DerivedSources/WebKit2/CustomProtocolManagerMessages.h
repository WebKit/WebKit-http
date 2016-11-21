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

namespace IPC {
    class DataReference;
}

namespace WTF {
    class String;
}

namespace WebCore {
    class ResourceResponse;
    class ResourceRequest;
    class ResourceError;
}

namespace Messages {
namespace CustomProtocolManager {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("CustomProtocolManager");
}

class DidFailWithError {
public:
    typedef std::tuple<uint64_t, const WebCore::ResourceError&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidFailWithError"); }
    static const bool isSync = false;

    DidFailWithError(uint64_t customProtocolID, const WebCore::ResourceError& error)
        : m_arguments(customProtocolID, error)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidLoadData {
public:
    typedef std::tuple<uint64_t, const IPC::DataReference&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidLoadData"); }
    static const bool isSync = false;

    DidLoadData(uint64_t customProtocolID, const IPC::DataReference& data)
        : m_arguments(customProtocolID, data)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidReceiveResponse {
public:
    typedef std::tuple<uint64_t, const WebCore::ResourceResponse&, uint32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidReceiveResponse"); }
    static const bool isSync = false;

    DidReceiveResponse(uint64_t customProtocolID, const WebCore::ResourceResponse& response, uint32_t cacheStoragePolicy)
        : m_arguments(customProtocolID, response, cacheStoragePolicy)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidFinishLoading {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidFinishLoading"); }
    static const bool isSync = false;

    explicit DidFinishLoading(uint64_t customProtocolID)
        : m_arguments(customProtocolID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class WasRedirectedToRequest {
public:
    typedef std::tuple<uint64_t, const WebCore::ResourceRequest&, const WebCore::ResourceResponse&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("WasRedirectedToRequest"); }
    static const bool isSync = false;

    WasRedirectedToRequest(uint64_t customProtocolID, const WebCore::ResourceRequest& request, const WebCore::ResourceResponse& redirectResponse)
        : m_arguments(customProtocolID, request, redirectResponse)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class RegisterScheme {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RegisterScheme"); }
    static const bool isSync = false;

    explicit RegisterScheme(const String& name)
        : m_arguments(name)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class UnregisterScheme {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("UnregisterScheme"); }
    static const bool isSync = false;

    explicit UnregisterScheme(const String& name)
        : m_arguments(name)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

} // namespace CustomProtocolManager
} // namespace Messages
