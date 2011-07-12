/*
 * Copyright (C) 2006, 2007, 2008 Apple, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#import "config.h"
#import "ResourceRequest.h"

#import "FormDataStreamMac.h"
#import "ResourceRequestCFNet.h"
#import "RuntimeApplicationChecks.h"
#import "WebCoreSystemInterface.h"

#import <Foundation/Foundation.h>


@interface NSURLRequest (WebNSURLRequestDetails)
- (NSArray *)contentDispositionEncodingFallbackArray;
+ (void)setDefaultTimeoutInterval:(NSTimeInterval)seconds;
- (CFURLRequestRef)_CFURLRequest;
- (id)_initWithCFURLRequest:(CFURLRequestRef)request;
@end

@interface NSMutableURLRequest (WebMutableNSURLRequestDetails)
- (void)setContentDispositionEncodingFallbackArray:(NSArray *)theEncodingFallbackArray;
@end

namespace WebCore {

NSURLRequest *ResourceRequest::nsURLRequest() const
{ 
    updatePlatformRequest();
    
    return [[m_nsRequest.get() retain] autorelease]; 
}

#if USE(CFNETWORK)

ResourceRequest::ResourceRequest(NSURLRequest *nsRequest)
    : ResourceRequestBase()
    , m_cfRequest([nsRequest _CFURLRequest])
    , m_nsRequest(nsRequest)
{
}

void ResourceRequest::updateNSURLRequest()
{
    if (m_cfRequest)
        m_nsRequest.adoptNS([[NSURLRequest alloc] _initWithCFURLRequest:m_cfRequest.get()]);
}

#else

void ResourceRequest::doUpdateResourceRequest()
{
    m_url = [m_nsRequest.get() URL];
    m_cachePolicy = (ResourceRequestCachePolicy)[m_nsRequest.get() cachePolicy];
    m_timeoutInterval = [m_nsRequest.get() timeoutInterval];
    m_firstPartyForCookies = [m_nsRequest.get() mainDocumentURL];
    
    if (NSString* method = [m_nsRequest.get() HTTPMethod])
        m_httpMethod = method;
    m_allowCookies = [m_nsRequest.get() HTTPShouldHandleCookies];

#if !defined(BUILDING_ON_LEOPARD) && !defined(BUILDING_ON_SNOW_LEOPARD)
    if (ResourceRequest::httpPipeliningEnabled())
        m_priority = toResourceLoadPriority(wkGetHTTPPipeliningPriority([m_nsRequest.get() _CFURLRequest]));
#endif

    NSDictionary *headers = [m_nsRequest.get() allHTTPHeaderFields];
    NSEnumerator *e = [headers keyEnumerator];
    NSString *name;
    m_httpHeaderFields.clear();
    while ((name = [e nextObject]))
        m_httpHeaderFields.set(name, [headers objectForKey:name]);

    // The below check can be removed once we require a version of Foundation with -[NSURLRequest contentDispositionEncodingFallbackArray] method.
    static bool supportsContentDispositionEncodingFallbackArray = [NSURLRequest instancesRespondToSelector:@selector(contentDispositionEncodingFallbackArray)];
    if (supportsContentDispositionEncodingFallbackArray) {
        m_responseContentDispositionEncodingFallbackArray.clear();
        NSArray *encodingFallbacks = [m_nsRequest.get() contentDispositionEncodingFallbackArray];
        NSUInteger count = [encodingFallbacks count];
        for (NSUInteger i = 0; i < count; ++i) {
            CFStringEncoding encoding = CFStringConvertNSStringEncodingToEncoding([(NSNumber *)[encodingFallbacks objectAtIndex:i] unsignedLongValue]);
            if (encoding != kCFStringEncodingInvalidId)
                m_responseContentDispositionEncodingFallbackArray.append(CFStringConvertEncodingToIANACharSetName(encoding));
        }
    }

    if (NSData* bodyData = [m_nsRequest.get() HTTPBody])
        m_httpBody = FormData::create([bodyData bytes], [bodyData length]);
    else if (NSInputStream* bodyStream = [m_nsRequest.get() HTTPBodyStream])
        if (FormData* formData = httpBodyFromStream(bodyStream))
            m_httpBody = formData;
}

void ResourceRequest::doUpdatePlatformRequest()
{
    if (isNull()) {
        m_nsRequest = nil;
        return;
    }
    
    NSMutableURLRequest* nsRequest = [m_nsRequest.get() mutableCopy];

    if (nsRequest)
        [nsRequest setURL:url()];
    else
        nsRequest = [[NSMutableURLRequest alloc] initWithURL:url()];


#if !defined(BUILDING_ON_LEOPARD) && !defined(BUILDING_ON_SNOW_LEOPARD)
    if (ResourceRequest::httpPipeliningEnabled())
        wkSetHTTPPipeliningPriority([nsRequest _CFURLRequest], toHTTPPipeliningPriority(m_priority));
#endif

    [nsRequest setCachePolicy:(NSURLRequestCachePolicy)cachePolicy()];

    double timeoutInterval = ResourceRequestBase::timeoutInterval();
    if (timeoutInterval)
        [nsRequest setTimeoutInterval:timeoutInterval];
    // Otherwise, respect NSURLRequest default timeout.

    [nsRequest setMainDocumentURL:firstPartyForCookies()];
    if (!httpMethod().isEmpty())
        [nsRequest setHTTPMethod:httpMethod()];
    [nsRequest setHTTPShouldHandleCookies:allowCookies()];

    // Cannot just use setAllHTTPHeaderFields here, because it does not remove headers.
    NSArray *oldHeaderFieldNames = [[nsRequest allHTTPHeaderFields] allKeys];
    for (unsigned i = [oldHeaderFieldNames count]; i != 0; --i)
        [nsRequest setValue:nil forHTTPHeaderField:[oldHeaderFieldNames objectAtIndex:i - 1]];
    HTTPHeaderMap::const_iterator end = httpHeaderFields().end();
    for (HTTPHeaderMap::const_iterator it = httpHeaderFields().begin(); it != end; ++it)
        [nsRequest setValue:it->second forHTTPHeaderField:it->first];

    // The below check can be removed once we require a version of Foundation with -[NSMutableURLRequest setContentDispositionEncodingFallbackArray:] method.
    static bool supportsContentDispositionEncodingFallbackArray = [NSMutableURLRequest instancesRespondToSelector:@selector(setContentDispositionEncodingFallbackArray:)];
    if (supportsContentDispositionEncodingFallbackArray) {
        NSMutableArray *encodingFallbacks = [NSMutableArray array];
        unsigned count = m_responseContentDispositionEncodingFallbackArray.size();
        for (unsigned i = 0; i != count; ++i) {
            CFStringRef encodingName = m_responseContentDispositionEncodingFallbackArray[i].createCFString();
            unsigned long nsEncoding = CFStringConvertEncodingToNSStringEncoding(CFStringConvertIANACharSetNameToEncoding(encodingName));
            CFRelease(encodingName);
            if (nsEncoding != kCFStringEncodingInvalidId)
                [encodingFallbacks addObject:[NSNumber numberWithUnsignedLong:nsEncoding]];
        }
        [nsRequest setContentDispositionEncodingFallbackArray:encodingFallbacks];
    }

    RefPtr<FormData> formData = httpBody();
    if (formData && !formData->isEmpty())
        WebCore::setHTTPBody(nsRequest, formData);
    
    m_nsRequest.adoptNS(nsRequest);
}

void ResourceRequest::applyWebArchiveHackForMail()
{
    // Hack because Mail checks for this property to detect data / archive loads
    [NSURLProtocol setProperty:@"" forKey:@"WebDataRequest" inRequest:(NSMutableURLRequest *)nsURLRequest()];
}

#if USE(CFURLSTORAGESESSIONS)

void ResourceRequest::setStorageSession(CFURLStorageSessionRef storageSession)
{
    m_nsRequest = wkCopyRequestWithStorageSession(storageSession, m_nsRequest.get());
}

#endif
    
#endif // USE(CFNETWORK)

static bool initQuickLookResourceCachingQuirks()
{
    if (applicationIsSafari())
        return false;
    
    NSArray* frameworks = [NSBundle allFrameworks];
    
    if (!frameworks)
        return false;
    
    for (unsigned int i = 0; i < [frameworks count]; i++) {
        NSBundle* bundle = [frameworks objectAtIndex: i];
        const char* bundleID = [[bundle bundleIdentifier] UTF8String];
        if (bundleID && !strcasecmp(bundleID, "com.apple.QuickLookUIFramework"))
            return true;
    }
    return false;
}

bool ResourceRequest::useQuickLookResourceCachingQuirks()
{
    static bool flag = initQuickLookResourceCachingQuirks();
    return flag;
}

} // namespace WebCore

