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

#ifndef NetworkProcessConnectionMessages_h
#define NetworkProcessConnectionMessages_h

#include "Arguments.h"
#include "MessageEncoder.h"
#include "ShareableResource.h"
#include "StringReference.h"
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {
    class SessionID;
    class ResourceRequest;
}

namespace Messages {
namespace NetworkProcessConnection {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("NetworkProcessConnection");
}

#if ENABLE(SHAREABLE_RESOURCE)
class DidCacheResource {
public:
    typedef std::tuple<WebCore::ResourceRequest, WebKit::ShareableResource::Handle, WebCore::SessionID> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidCacheResource"); }
    static const bool isSync = false;

    DidCacheResource(const WebCore::ResourceRequest& request, const WebKit::ShareableResource::Handle& resource, const WebCore::SessionID& sessionID)
        : m_arguments(request, resource, sessionID)
    {
    }

    const std::tuple<const WebCore::ResourceRequest&, const WebKit::ShareableResource::Handle&, const WebCore::SessionID&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::ResourceRequest&, const WebKit::ShareableResource::Handle&, const WebCore::SessionID&> m_arguments;
};
#endif

class DidWriteBlobsToTemporaryFiles {
public:
    typedef std::tuple<uint64_t, Vector<String>> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidWriteBlobsToTemporaryFiles"); }
    static const bool isSync = false;

    DidWriteBlobsToTemporaryFiles(uint64_t requestIdentifier, const Vector<String>& filenames)
        : m_arguments(requestIdentifier, filenames)
    {
    }

    const std::tuple<uint64_t, const Vector<String>&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t, const Vector<String>&> m_arguments;
};

} // namespace NetworkProcessConnection
} // namespace Messages

#endif // NetworkProcessConnectionMessages_h
