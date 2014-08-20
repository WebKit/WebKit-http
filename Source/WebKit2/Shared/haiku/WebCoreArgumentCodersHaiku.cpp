/*
 * Copyright (C) 2014 Haiku, inc.
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
#include "DataReference.h"
#include "WebCoreArgumentCoders.h"

#include "NotImplemented.h"
#include <WebCore/CertificateInfo.h>
#include <WebCore/ResourceError.h>
#include <WebCore/ResourceRequest.h>
#include <WebCore/ResourceResponse.h>
#include <wtf/text/CString.h>

using namespace WebCore;

namespace IPC {

void ArgumentCoder<ResourceRequest>::encodePlatformData(ArgumentEncoder& encoder, const ResourceRequest& resourceRequest)
{
    notImplemented();
}

bool ArgumentCoder<ResourceRequest>::decodePlatformData(ArgumentDecoder& decoder, ResourceRequest& resourceRequest)
{
    notImplemented();
    return false;
}

void ArgumentCoder<ResourceResponse>::encodePlatformData(ArgumentEncoder& encoder, const ResourceResponse& resourceResponse)
{
    //encoder << static_cast<uint32_t>(resourceResponse.soupMessageFlags());
}

bool ArgumentCoder<ResourceResponse>::decodePlatformData(ArgumentDecoder& decoder, ResourceResponse& resourceResponse)
{
    /*
    uint32_t soupMessageFlags;
    if (!decoder.decode(soupMessageFlags))
        return false;
    resourceResponse.setSoupMessageFlags(static_cast<SoupMessageFlags>(soupMessageFlags));
    return true;
    */

    return false;
}

void ArgumentCoder<CertificateInfo>::encode(ArgumentEncoder& encoder, const CertificateInfo& certificateInfo)
{
    notImplemented();
}

bool ArgumentCoder<CertificateInfo>::decode(ArgumentDecoder& decoder, CertificateInfo& certificateInfo)
{
    notImplemented();
    return false;
}

void ArgumentCoder<ResourceError>::encodePlatformData(ArgumentEncoder& encoder, const ResourceError& resourceError)
{
    bool errorIsNull = resourceError.isNull();
    encoder << errorIsNull;
    if (errorIsNull)
        return;

    encoder << resourceError.domain();
    encoder << resourceError.errorCode();
    encoder << resourceError.failingURL();
    encoder << resourceError.localizedDescription();
    encoder << resourceError.isCancellation();
    encoder << resourceError.isTimeout();

    encoder << CertificateInfo(resourceError);
}

bool ArgumentCoder<ResourceError>::decodePlatformData(ArgumentDecoder& decoder, ResourceError& resourceError)
{
    notImplemented();
    return false;
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

}


