/*
 * Copyright (C) 2006 Apple Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef ResourceResponse_h
#define ResourceResponse_h

#include "ResourceResponseBase.h"
#include <wtf/RetainPtr.h>

#if USE(CFNETWORK)
typedef struct _CFURLResponse* CFURLResponseRef;
#endif

OBJC_CLASS NSURLResponse;

namespace WebCore {

class ResourceResponse : public ResourceResponseBase {
public:
    ResourceResponse()
        : m_initLevel(CommonAndUncommonFields)
        , m_platformResponseIsUpToDate(true)
    {
    }

#if USE(CFNETWORK)
    ResourceResponse(CFURLResponseRef cfResponse)
        : m_initLevel(Uninitialized)
        , m_platformResponseIsUpToDate(true)
        , m_cfResponse(cfResponse)
    {
        m_isNull = !cfResponse;
    }
#if PLATFORM(COCOA)
    ResourceResponse(NSURLResponse *);
#endif
#else
    ResourceResponse(NSURLResponse *nsResponse)
        : m_initLevel(Uninitialized)
        , m_platformResponseIsUpToDate(true)
        , m_nsResponse(nsResponse)
    {
        m_isNull = !nsResponse;
    }
#endif

    ResourceResponse(const URL& url, const String& mimeType, long long expectedLength, const String& textEncodingName, const String& filename)
        : ResourceResponseBase(url, mimeType, expectedLength, textEncodingName, filename)
        , m_initLevel(CommonAndUncommonFields)
        , m_platformResponseIsUpToDate(false)
    {
    }

    unsigned memoryUsage() const
    {
        // FIXME: Find some programmatic lighweight way to calculate ResourceResponse and associated classes.
        // This is a rough estimate of resource overhead based on stats collected from memory usage tests.
        return 3800;
        /*  1280 * 2 +                // average size of ResourceResponse. Doubled to account for the WebCore copy and the CF copy.
                                      // Mostly due to the size of the hash maps, the Header Map strings and the URL.
            256 * 2                   // Overhead from ResourceRequest, doubled to account for WebCore copy and CF copy.
                                      // Mostly due to the URL and Header Map.
         */
    }

#if USE(CFNETWORK)
    CFURLResponseRef cfURLResponse() const;
#endif
#if PLATFORM(COCOA)
    NSURLResponse *nsURLResponse() const;
#endif

#if PLATFORM(COCOA) || USE(CFNETWORK)
    void setCertificateChain(CFArrayRef);
    RetainPtr<CFArrayRef> certificateChain() const;
#endif

    bool platformResponseIsUpToDate() const { return m_platformResponseIsUpToDate; }

private:
    friend class ResourceResponseBase;

    void platformLazyInit(InitLevel);
    PassOwnPtr<CrossThreadResourceResponseData> doPlatformCopyData(PassOwnPtr<CrossThreadResourceResponseData> data) const { return data; }
    void doPlatformAdopt(PassOwnPtr<CrossThreadResourceResponseData>) { }
#if PLATFORM(COCOA)
    void initNSURLResponse() const;
#endif

    static bool platformCompare(const ResourceResponse& a, const ResourceResponse& b);

    unsigned m_initLevel : 3;
    bool m_platformResponseIsUpToDate : 1;

#if USE(CFNETWORK)
    mutable RetainPtr<CFURLResponseRef> m_cfResponse;
#endif
#if PLATFORM(COCOA)
    mutable RetainPtr<NSURLResponse> m_nsResponse;
#endif
#if PLATFORM(COCOA) || USE(CFNETWORK)
    // Certificate chain is normally part of NS/CFURLResponse, but there is no way to re-add it to a deserialized response after IPC.
    RetainPtr<CFArrayRef> m_externalCertificateChain;
#endif
};

struct CrossThreadResourceResponseData : public CrossThreadResourceResponseDataBase {
};

} // namespace WebCore

#endif // ResourceResponse_h
