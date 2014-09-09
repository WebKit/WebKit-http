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

#ifndef SynchronousNetworkLoaderClient_h
#define SynchronousNetworkLoaderClient_h

#include "NetworkConnectionToWebProcessMessages.h"
#include "NetworkLoaderClient.h"
#include <WebCore/ResourceError.h>
#include <WebCore/ResourceRequest.h>
#include <WebCore/ResourceResponse.h>
#include <memory>
#include <wtf/RefCounted.h>

#if ENABLE(NETWORK_PROCESS)

namespace WebCore {
class SharedBuffer;
}

namespace WebKit {

class SynchronousNetworkLoaderClient : public NetworkLoaderClient {
public:
    SynchronousNetworkLoaderClient(const WebCore::ResourceRequest& originalRequest, PassRefPtr<Messages::NetworkConnectionToWebProcess::PerformSynchronousLoad::DelayedReply>);
    virtual ~SynchronousNetworkLoaderClient() override;

    virtual bool isSynchronous() override { return true; }

private:
    virtual void willSendRequest(NetworkResourceLoader*, WebCore::ResourceRequest& proposedRequest, const WebCore::ResourceResponse& redirectResponse) override;
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    virtual void canAuthenticateAgainstProtectionSpace(NetworkResourceLoader*, const WebCore::ProtectionSpace&) override;
#endif
    virtual void didReceiveResponse(NetworkResourceLoader*, const WebCore::ResourceResponse&) override;
    virtual void didReceiveBuffer(NetworkResourceLoader*, WebCore::SharedBuffer*, int encodedDataLength) override;
    virtual void didSendData(NetworkResourceLoader*, unsigned long long /* bytesSent */, unsigned long long /* totalBytesToBeSent */) override { }
    virtual void didFinishLoading(NetworkResourceLoader*, double finishTime) override;
    virtual void didFail(NetworkResourceLoader*, const WebCore::ResourceError&) override;

    void sendDelayedReply(NetworkResourceLoader&);

    WebCore::ResourceRequest m_originalRequest;
    WebCore::ResourceRequest m_currentRequest;
    RefPtr<Messages::NetworkConnectionToWebProcess::PerformSynchronousLoad::DelayedReply> m_delayedReply;
    WebCore::ResourceResponse m_response;
    WebCore::ResourceError m_error;
};

} // namespace WebKit

#endif // ENABLE(NETWORK_PROCESS)

#endif // SynchronousNetworkLoaderClient_h
