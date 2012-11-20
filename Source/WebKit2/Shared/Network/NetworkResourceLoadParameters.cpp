/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#include "config.h"
#include "NetworkResourceLoadParameters.h"

#include "ArgumentCoders.h"
#include "WebCoreArgumentCoders.h"

#if ENABLE(NETWORK_PROCESS)

using namespace WebCore;

namespace WebKit {
NetworkResourceLoadParameters::NetworkResourceLoadParameters()
    : m_priority(ResourceLoadPriorityVeryLow)
    , m_contentSniffingPolicy(SniffContent)
    , m_allowStoredCredentials(DoNotAllowStoredCredentials)
{
}

NetworkResourceLoadParameters::NetworkResourceLoadParameters(const ResourceRequest& request, ResourceLoadPriority priority, ContentSniffingPolicy contentSniffingPolicy, StoredCredentials allowStoredCredentials)
    : m_request(request)
    , m_priority(priority)
    , m_contentSniffingPolicy(contentSniffingPolicy)
    , m_allowStoredCredentials(allowStoredCredentials)
{
}

void NetworkResourceLoadParameters::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder.encode(m_request);
    encoder.encodeEnum(m_priority);
    encoder.encodeEnum(m_contentSniffingPolicy);
    encoder.encodeEnum(m_allowStoredCredentials);
}

bool NetworkResourceLoadParameters::decode(CoreIPC::ArgumentDecoder* decoder, NetworkResourceLoadParameters& result)
{
    if (!decoder->decode(result.m_request))
        return false;
    if (!decoder->decodeEnum(result.m_priority))
        return false;
    if (!decoder->decodeEnum(result.m_contentSniffingPolicy))
        return false;
    if (!decoder->decodeEnum(result.m_allowStoredCredentials))
        return false;

    return true;
}
    
} // namespace WebKit

#endif // ENABLE(NETWORK_PROCESS)
