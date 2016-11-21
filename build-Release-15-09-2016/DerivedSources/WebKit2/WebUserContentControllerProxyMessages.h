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

#ifndef WebUserContentControllerProxyMessages_h
#define WebUserContentControllerProxyMessages_h

#include "Arguments.h"
#include "MessageEncoder.h"
#include "StringReference.h"

namespace IPC {
    class DataReference;
}

namespace WebCore {
    struct SecurityOriginData;
}

namespace Messages {
namespace WebUserContentControllerProxy {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("WebUserContentControllerProxy");
}

class DidPostMessage {
public:
    typedef std::tuple<uint64_t, uint64_t, WebCore::SecurityOriginData, uint64_t, IPC::DataReference> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidPostMessage"); }
    static const bool isSync = false;

    DidPostMessage(uint64_t pageID, uint64_t frameID, const WebCore::SecurityOriginData& frameSecurityOrigin, uint64_t messageHandlerID, const IPC::DataReference& message)
        : m_arguments(pageID, frameID, frameSecurityOrigin, messageHandlerID, message)
    {
    }

    const std::tuple<uint64_t, uint64_t, const WebCore::SecurityOriginData&, uint64_t, const IPC::DataReference&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t, uint64_t, const WebCore::SecurityOriginData&, uint64_t, const IPC::DataReference&> m_arguments;
};

} // namespace WebUserContentControllerProxy
} // namespace Messages

#endif // WebUserContentControllerProxyMessages_h
