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
#include "DataReference.h"
#include "WebCoreArgumentCoders.h"

#if ENABLE(NETWORK_PROCESS)

using namespace WebCore;

namespace WebKit {

NetworkResourceLoadParameters::NetworkResourceLoadParameters()
    : identifier(0)
    , webPageID(0)
    , webFrameID(0)
    , sessionID(SessionID::emptySessionID())
    , priority(ResourceLoadPriorityVeryLow)
    , contentSniffingPolicy(SniffContent)
    , allowStoredCredentials(DoNotAllowStoredCredentials)
    , clientCredentialPolicy(DoNotAskClientForAnyCredentials)
    , shouldClearReferrerOnHTTPSToHTTPRedirect(true)
    , isMainResource(false)
    , defersLoading(false)
    , shouldBufferResource(false)
{
}

void NetworkResourceLoadParameters::encode(IPC::ArgumentEncoder& encoder) const
{
    encoder << identifier;
    encoder << webPageID;
    encoder << webFrameID;
    encoder << sessionID;
    encoder << request;

    encoder << static_cast<bool>(request.httpBody());
    if (request.httpBody()) {
        request.httpBody()->encode(encoder);

        const Vector<FormDataElement>& elements = request.httpBody()->elements();
        size_t fileCount = 0;
        for (size_t i = 0, count = elements.size(); i < count; ++i) {
            if (elements[i].m_type == FormDataElement::Type::EncodedFile)
                ++fileCount;
        }

        SandboxExtension::HandleArray requestBodySandboxExtensions;
        requestBodySandboxExtensions.allocate(fileCount);
        size_t extensionIndex = 0;
        for (size_t i = 0, count = elements.size(); i < count; ++i) {
            const FormDataElement& element = elements[i];
            if (element.m_type == FormDataElement::Type::EncodedFile) {
                const String& path = element.m_shouldGenerateFile ? element.m_generatedFilename : element.m_filename;
                SandboxExtension::createHandle(path, SandboxExtension::ReadOnly, requestBodySandboxExtensions[extensionIndex++]);
            }
        }
        encoder << requestBodySandboxExtensions;
    }

    if (request.url().isLocalFile()) {
        SandboxExtension::Handle requestSandboxExtension;
        SandboxExtension::createHandle(request.url().fileSystemPath(), SandboxExtension::ReadOnly, requestSandboxExtension);
        encoder << requestSandboxExtension;
    }

    encoder.encodeEnum(priority);
    encoder.encodeEnum(contentSniffingPolicy);
    encoder.encodeEnum(allowStoredCredentials);
    encoder.encodeEnum(clientCredentialPolicy);
    encoder << shouldClearReferrerOnHTTPSToHTTPRedirect;
    encoder << isMainResource;
    encoder << defersLoading;
    encoder << shouldBufferResource;
}

bool NetworkResourceLoadParameters::decode(IPC::ArgumentDecoder& decoder, NetworkResourceLoadParameters& result)
{
    if (!decoder.decode(result.identifier))
        return false;

    if (!decoder.decode(result.webPageID))
        return false;

    if (!decoder.decode(result.webFrameID))
        return false;

    if (!decoder.decode(result.sessionID))
        return false;

    if (!decoder.decode(result.request))
        return false;

    bool hasHTTPBody;
    if (!decoder.decode(hasHTTPBody))
        return false;

    if (hasHTTPBody) {
        RefPtr<FormData> formData = FormData::decode(decoder);
        if (!formData)
            return false;
        result.request.setHTTPBody(formData.release());

        if (!decoder.decode(result.requestBodySandboxExtensions))
            return false;
    }

    if (result.request.url().isLocalFile()) {
        if (!decoder.decode(result.resourceSandboxExtension))
            return false;
    }

    if (!decoder.decodeEnum(result.priority))
        return false;
    if (!decoder.decodeEnum(result.contentSniffingPolicy))
        return false;
    if (!decoder.decodeEnum(result.allowStoredCredentials))
        return false;
    if (!decoder.decodeEnum(result.clientCredentialPolicy))
        return false;
    if (!decoder.decode(result.shouldClearReferrerOnHTTPSToHTTPRedirect))
        return false;
    if (!decoder.decode(result.isMainResource))
        return false;
    if (!decoder.decode(result.defersLoading))
        return false;
    if (!decoder.decode(result.shouldBufferResource))
        return false;

    return true;
}
    
} // namespace WebKit

#endif // ENABLE(NETWORK_PROCESS)
