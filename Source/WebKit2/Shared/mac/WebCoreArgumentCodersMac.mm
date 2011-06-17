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

#import "config.h"
#import "WebCoreArgumentCoders.h"

#import "ArgumentCodersCF.h"
#import "PlatformCertificateInfo.h"
#import "WebKitSystemInterface.h"
#import <WebCore/KeyboardEvent.h>
#import <WebCore/ResourceError.h>
#import <WebCore/ResourceRequest.h>

using namespace WebCore;
using namespace WebKit;

namespace CoreIPC {

void ArgumentCoder<ResourceRequest>::encode(ArgumentEncoder* encoder, const ResourceRequest& resourceRequest)
{
    bool requestIsPresent = resourceRequest.nsURLRequest();
    encoder->encode(requestIsPresent);

    if (!requestIsPresent)
        return;

    RetainPtr<CFDictionaryRef> dictionary(AdoptCF, WKNSURLRequestCreateSerializableRepresentation(resourceRequest.nsURLRequest(), CoreIPC::tokenNullTypeRef()));
    CoreIPC::encode(encoder, dictionary.get());
}

bool ArgumentCoder<ResourceRequest>::decode(ArgumentDecoder* decoder, ResourceRequest& resourceRequest)
{
    bool requestIsPresent;
    if (!decoder->decode(requestIsPresent))
        return false;

    if (!requestIsPresent) {
        resourceRequest = ResourceRequest();
        return true;
    }

    RetainPtr<CFDictionaryRef> dictionary;
    if (!CoreIPC::decode(decoder, dictionary))
        return false;

    NSURLRequest *nsURLRequest = WKNSURLRequestFromSerializableRepresentation(dictionary.get(), CoreIPC::tokenNullTypeRef());
    if (!nsURLRequest)
        return false;

    resourceRequest = ResourceRequest(nsURLRequest);
    return true;
}

void ArgumentCoder<ResourceResponse>::encode(ArgumentEncoder* encoder, const ResourceResponse& resourceResponse)
{
    bool responseIsPresent = resourceResponse.nsURLResponse();
    encoder->encode(responseIsPresent);

    if (!responseIsPresent)
        return;

    RetainPtr<CFDictionaryRef> dictionary(AdoptCF, WKNSURLResponseCreateSerializableRepresentation(resourceResponse.nsURLResponse(), CoreIPC::tokenNullTypeRef()));
    CoreIPC::encode(encoder, dictionary.get());
}

bool ArgumentCoder<ResourceResponse>::decode(ArgumentDecoder* decoder, ResourceResponse& resourceResponse)
{
    bool responseIsPresent;
    if (!decoder->decode(responseIsPresent))
        return false;

    if (!responseIsPresent) {
        resourceResponse = ResourceResponse();
        return true;
    }

    RetainPtr<CFDictionaryRef> dictionary;
    if (!CoreIPC::decode(decoder, dictionary))
        return false;

    NSURLResponse* nsURLResponse = WKNSURLResponseFromSerializableRepresentation(dictionary.get(), CoreIPC::tokenNullTypeRef());
    if (!nsURLResponse)
        return false;

    resourceResponse = ResourceResponse(nsURLResponse);
    return true;
}

static NSString* nsString(const String& string)
{
    return string.impl() ? [NSString stringWithCharacters:reinterpret_cast<const UniChar*>(string.characters()) length:string.length()] : @"";
}

void ArgumentCoder<ResourceError>::encode(ArgumentEncoder* encoder, const ResourceError& resourceError)
{
    bool errorIsNull = resourceError.isNull();
    encoder->encode(errorIsNull);

    if (errorIsNull)
        return;

    NSError *nsError = resourceError.nsError();

    String domain = [nsError domain];
    encoder->encode(domain);
    
    int64_t code = [nsError code];
    encoder->encode(code);

    HashMap<String, String> stringUserInfoMap;

    NSDictionary* userInfo = [nsError userInfo];
    for (NSString *key in userInfo) {
        id value = [userInfo objectForKey:key];
        if (![value isKindOfClass:[NSString class]])
            continue;

        stringUserInfoMap.set(key, (NSString *)value);
        continue;
    }
    encoder->encode(stringUserInfoMap);

    id peerCertificateChain = [userInfo objectForKey:@"NSErrorPeerCertificateChainKey"];
    ASSERT(!peerCertificateChain || [peerCertificateChain isKindOfClass:[NSArray class]]);
    encoder->encode(PlatformCertificateInfo((CFArrayRef)peerCertificateChain));
}

bool ArgumentCoder<ResourceError>::decode(ArgumentDecoder* decoder, ResourceError& resourceError)
{
    bool errorIsNull;
    if (!decoder->decode(errorIsNull))
        return false;
    
    if (errorIsNull) {
        resourceError = ResourceError();
        return true;
    }

    String domain;
    if (!decoder->decode(domain))
        return false;

    int64_t code;
    if (!decoder->decode(code))
        return false;

    HashMap<String, String> stringUserInfoMap;
    if (!decoder->decode(stringUserInfoMap))
        return false;

    PlatformCertificateInfo certificate;
    if (!decoder->decode(certificate))
        return false;

    NSUInteger userInfoSize = stringUserInfoMap.size();
    if (certificate.certificateChain())
        userInfoSize++;

    NSMutableDictionary* userInfo = [NSMutableDictionary dictionaryWithCapacity:userInfoSize];
    
    HashMap<String, String>::const_iterator it = stringUserInfoMap.begin();
    HashMap<String, String>::const_iterator end = stringUserInfoMap.end();
    for (; it != end; ++it)
        [userInfo setObject:nsString(it->second) forKey:nsString(it->first)];

    if (certificate.certificateChain())
        [userInfo setObject:(NSArray *)certificate.certificateChain() forKey:@"NSErrorPeerCertificateChainKey"];

    RetainPtr<NSError> nsError(AdoptNS, [[NSError alloc] initWithDomain:nsString(domain) code:code userInfo:userInfo]);

    resourceError = ResourceError(nsError.get());
    return true;
}

void ArgumentCoder<KeypressCommand>::encode(ArgumentEncoder* encoder, const KeypressCommand& keypressCommand)
{
    encoder->encode(keypressCommand.commandName);
    encoder->encode(keypressCommand.text);
}
    
bool ArgumentCoder<KeypressCommand>::decode(ArgumentDecoder* decoder, KeypressCommand& keypressCommand)
{
    if (!decoder->decode(keypressCommand.commandName))
        return false;

    if (!decoder->decode(keypressCommand.text))
        return false;

    return true;
}

} // namespace CoreIPC
