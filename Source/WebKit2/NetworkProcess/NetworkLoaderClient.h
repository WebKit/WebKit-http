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

#ifndef NetworkLoaderClient_h
#define NetworkLoaderClient_h

#include <wtf/RefCounted.h>

#if ENABLE(NETWORK_PROCESS)

namespace WebCore {
class ProtectionSpace;
class ResourceError;
class ResourceRequest;
class ResourceResponse;
class SharedBuffer;
}

namespace WebKit {

class NetworkResourceLoader;

class NetworkLoaderClient {
public:
    virtual ~NetworkLoaderClient() { }

    virtual void willSendRequest(NetworkResourceLoader*, WebCore::ResourceRequest& newRequest, const WebCore::ResourceResponse& redirectResponse) = 0;
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    virtual void canAuthenticateAgainstProtectionSpace(NetworkResourceLoader*, const WebCore::ProtectionSpace&) = 0;
#endif
    virtual void didReceiveResponse(NetworkResourceLoader*, const WebCore::ResourceResponse&) = 0;
    virtual void didReceiveBuffer(NetworkResourceLoader*, WebCore::SharedBuffer*, int encodedDataLength) = 0;
    virtual void didSendData(NetworkResourceLoader*, unsigned long long bytesSent, unsigned long long totalBytesToBeSent) = 0;
    virtual void didFinishLoading(NetworkResourceLoader*, double finishTime) = 0;
    virtual void didFail(NetworkResourceLoader*, const WebCore::ResourceError&) = 0;

    virtual bool isSynchronous() { return false; }

protected:
    NetworkLoaderClient() { }
};

} // namespace WebKit

#endif // ENABLE(NETWORK_PROCESS)

#endif // NetworkLoaderClient_h
