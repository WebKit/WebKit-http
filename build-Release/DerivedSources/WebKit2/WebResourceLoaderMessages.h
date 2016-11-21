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
#include "ShareableResource.h"

namespace IPC {
    class DataReference;
}

namespace WebCore {
    class ResourceResponse;
    class ResourceRequest;
    class ResourceError;
}

namespace Messages {
namespace WebResourceLoader {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("WebResourceLoader");
}

class WillSendRequest {
public:
    typedef std::tuple<const WebCore::ResourceRequest&, const WebCore::ResourceResponse&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("WillSendRequest"); }
    static const bool isSync = false;

    WillSendRequest(const WebCore::ResourceRequest& request, const WebCore::ResourceResponse& redirectResponse)
        : m_arguments(request, redirectResponse)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidSendData {
public:
    typedef std::tuple<uint64_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidSendData"); }
    static const bool isSync = false;

    DidSendData(uint64_t bytesSent, uint64_t totalBytesToBeSent)
        : m_arguments(bytesSent, totalBytesToBeSent)
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
    typedef std::tuple<const WebCore::ResourceResponse&, bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidReceiveResponse"); }
    static const bool isSync = false;

    DidReceiveResponse(const WebCore::ResourceResponse& response, bool needsContinueDidReceiveResponseMessage)
        : m_arguments(response, needsContinueDidReceiveResponseMessage)
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
    typedef std::tuple<const IPC::DataReference&, int64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidReceiveData"); }
    static const bool isSync = false;

    DidReceiveData(const IPC::DataReference& data, int64_t encodedDataLength)
        : m_arguments(data, encodedDataLength)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidFinishResourceLoad {
public:
    typedef std::tuple<double> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidFinishResourceLoad"); }
    static const bool isSync = false;

    explicit DidFinishResourceLoad(double finishTime)
        : m_arguments(finishTime)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidFailResourceLoad {
public:
    typedef std::tuple<const WebCore::ResourceError&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidFailResourceLoad"); }
    static const bool isSync = false;

    explicit DidFailResourceLoad(const WebCore::ResourceError& error)
        : m_arguments(error)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

#if ENABLE(SHAREABLE_RESOURCE)
class DidReceiveResource {
public:
    typedef std::tuple<const WebKit::ShareableResource::Handle&, double> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidReceiveResource"); }
    static const bool isSync = false;

    DidReceiveResource(const WebKit::ShareableResource::Handle& resource, double finishTime)
        : m_arguments(resource, finishTime)
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

} // namespace WebResourceLoader
} // namespace Messages
