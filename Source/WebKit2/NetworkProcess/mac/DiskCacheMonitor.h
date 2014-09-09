/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DiskCacheMonitor_h
#define DiskCacheMonitor_h

#include "MessageSender.h"
#include <WebCore/ResourceRequest.h>
#include <WebCore/SessionID.h>
#include <wtf/RunLoop.h>

typedef const struct _CFCachedURLResponse* CFCachedURLResponseRef;

namespace WebKit {

class NetworkConnectionToWebProcess;
class NetworkResourceLoader;

class DiskCacheMonitor : public IPC::MessageSender {
public:
    static void monitorFileBackingStoreCreation(CFCachedURLResponseRef, NetworkResourceLoader*);

    const WebCore::ResourceRequest& resourceRequest() const { return m_resourceRequest; }

private:
    DiskCacheMonitor(CFCachedURLResponseRef, NetworkResourceLoader*);

    // IPC::MessageSender
    virtual IPC::Connection* messageSenderConnection() override;
    virtual uint64_t messageSenderDestinationID() override;

    RefPtr<NetworkConnectionToWebProcess> m_connectionToWebProcess;
    WebCore::ResourceRequest m_resourceRequest;
    WebCore::SessionID m_sessionID;
};


} // namespace WebKit

#endif // DiskCacheMonitor_h
