/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#include "WebCoreArgumentCoders.h"

#include <WebCore/ResourceError.h>
#include <WebCore/ResourceRequest.h>
#include <WebCore/ResourceResponse.h>

using namespace WebCore;
 
namespace IPC {

void ArgumentCoder<ResourceRequest>::encodePlatformData(ArgumentEncoder& encoder, const ResourceRequest& resourceRequest)
{
    // QTFIXME: Copied from WebCoreArgumentCodersSoup.cpp, partially duplicates ResourceRequestBase::encodeWithoutPlatformData
    // See r207407

    encoder << resourceRequest.url().string();
    encoder << resourceRequest.httpMethod();
    encoder << resourceRequest.httpHeaderFields();
    encoder << resourceRequest.timeoutInterval();

    // FIXME: Do not encode HTTP message body.
    // 1. It can be large and thus costly to send across.
    // 2. It is misleading to provide a body with some requests, while others use body streams, which cannot be serialized at all.
    FormData* httpBody = resourceRequest.httpBody();
    encoder << static_cast<bool>(httpBody);
    if (httpBody)
        encoder << httpBody->flattenToString();

    encoder << resourceRequest.firstPartyForCookies().string();
    encoder << resourceRequest.allowCookies();
    encoder.encodeEnum(resourceRequest.priority());
    encoder.encodeEnum(resourceRequest.cachePolicy());
    encoder.encodeEnum(resourceRequest.requester());
}

bool ArgumentCoder<ResourceRequest>::decodePlatformData(ArgumentDecoder& decoder, ResourceRequest& resourceRequest)
{
    // QTFIXME: ditto

    String url;
    if (!decoder.decode(url))
        return false;
    resourceRequest.setURL(URL(URL(), url));

    String httpMethod;
    if (!decoder.decode(httpMethod))
        return false;
    resourceRequest.setHTTPMethod(httpMethod);

    HTTPHeaderMap headers;
    if (!decoder.decode(headers))
        return false;
    resourceRequest.setHTTPHeaderFields(WTFMove(headers));

    double timeoutInterval;
    if (!decoder.decode(timeoutInterval))
        return false;
    resourceRequest.setTimeoutInterval(timeoutInterval);

    bool hasHTTPBody;
    if (!decoder.decode(hasHTTPBody))
        return false;
    if (hasHTTPBody) {
        String httpBody;
        if (!decoder.decode(httpBody))
            return false;
        resourceRequest.setHTTPBody(FormData::create(httpBody.utf8()));
    }

    String firstPartyForCookies;
    if (!decoder.decode(firstPartyForCookies))
        return false;
    resourceRequest.setFirstPartyForCookies(URL(URL(), firstPartyForCookies));

    bool allowCookies;
    if (!decoder.decode(allowCookies))
        return false;
    resourceRequest.setAllowCookies(allowCookies);

    ResourceLoadPriority priority;
    if (!decoder.decodeEnum(priority))
        return false;
    resourceRequest.setPriority(priority);

    ResourceRequestCachePolicy cachePolicy;
    if (!decoder.decodeEnum(cachePolicy))
        return false;
    resourceRequest.setCachePolicy(cachePolicy);

    ResourceRequest::Requester requester;
    if (!decoder.decodeEnum(requester))
        return false;
    resourceRequest.setRequester(requester);

    return true;
}

void ArgumentCoder<ResourceError>::encodePlatformData(ArgumentEncoder& encoder, const ResourceError& resourceError)
{
    // QTFIXME: ditto

    bool errorIsNull = resourceError.isNull();
    encoder << errorIsNull;
    if (errorIsNull)
        return;

    encoder << resourceError.domain();
    encoder << resourceError.errorCode();
    encoder << resourceError.failingURL().string();
    encoder << resourceError.localizedDescription();
    encoder << resourceError.isCancellation();
    encoder << resourceError.isTimeout();
}

bool ArgumentCoder<ResourceError>::decodePlatformData(ArgumentDecoder& decoder, ResourceError& resourceError)
{
    // QTFIXME: ditto

    bool errorIsNull;
    if (!decoder.decode(errorIsNull))
        return false;
    if (errorIsNull) {
        resourceError = ResourceError();
        return true;
    }

    String domain;
    if (!decoder.decode(domain))
        return false;

    int errorCode;
    if (!decoder.decode(errorCode))
        return false;

    String failingURL;
    if (!decoder.decode(failingURL))
        return false;

    String localizedDescription;
    if (!decoder.decode(localizedDescription))
        return false;

    bool isCancellation;
    if (!decoder.decode(isCancellation))
        return false;

    bool isTimeout;
    if (!decoder.decode(isTimeout))
        return false;

    resourceError = ResourceError(domain, errorCode, URL(URL(), failingURL), localizedDescription);
    resourceError.setIsCancellation(isCancellation);
    resourceError.setIsTimeout(isTimeout);

    return true;
}

void ArgumentCoder<CertificateInfo>::encode(ArgumentEncoder& encoder, const CertificateInfo& certificateInfo)
{
    // QTFIXME: Our CertificateInfo is a stub
    notImplemented();
}

bool ArgumentCoder<CertificateInfo>::decode(ArgumentDecoder& decoder, CertificateInfo& certificateInfo)
{
    // QTFIXME: Our CertificateInfo is a stub
    notImplemented();
    return true;
}

void ArgumentCoder<ProtectionSpace>::encodePlatformData(ArgumentEncoder&, const ProtectionSpace&)
{
    ASSERT_NOT_REACHED();
}

bool ArgumentCoder<ProtectionSpace>::decodePlatformData(ArgumentDecoder&, ProtectionSpace&)
{
    ASSERT_NOT_REACHED();
    return false;
}

void ArgumentCoder<Credential>::encodePlatformData(ArgumentEncoder&, const Credential&)
{
    ASSERT_NOT_REACHED();
}

bool ArgumentCoder<Credential>::decodePlatformData(ArgumentDecoder&, Credential&)
{
    ASSERT_NOT_REACHED();
    return false;
}


} // namespace IPC
