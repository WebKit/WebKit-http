/*
 * Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
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

#ifndef NetworkResourceLoadParameters_h
#define NetworkResourceLoadParameters_h

#include "SandboxExtension.h"
#include <WebCore/ResourceHandle.h>
#include <WebCore/ResourceLoaderOptions.h>
#include <WebCore/ResourceRequest.h>
#include <WebCore/SessionID.h>

#if ENABLE(NETWORK_PROCESS)

namespace IPC {
    class ArgumentDecoder;
    class ArgumentEncoder;
}

namespace WebKit {

typedef uint64_t ResourceLoadIdentifier;

class NetworkResourceLoadParameters {
public:
    NetworkResourceLoadParameters();

    void encode(IPC::ArgumentEncoder&) const;
    static bool decode(IPC::ArgumentDecoder&, NetworkResourceLoadParameters&);

    ResourceLoadIdentifier identifier;
    uint64_t webPageID;
    uint64_t webFrameID;
    WebCore::SessionID sessionID;
    WebCore::ResourceRequest request;
    SandboxExtension::HandleArray requestBodySandboxExtensions; // Created automatically for the sender.
    SandboxExtension::Handle resourceSandboxExtension; // Created automatically for the sender.
    WebCore::ResourceLoadPriority priority;
    WebCore::ContentSniffingPolicy contentSniffingPolicy;
    WebCore::StoredCredentials allowStoredCredentials;
    WebCore::ClientCredentialPolicy clientCredentialPolicy;
    bool shouldClearReferrerOnHTTPSToHTTPRedirect;
    bool isMainResource;
    bool defersLoading;
    bool shouldBufferResource;
};

} // namespace WebKit

#endif // ENABLE(NETWORK_PROCESS)

#endif // NetworkResourceLoadParameters_h
