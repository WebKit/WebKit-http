/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PlatformInstrumentation_h
#define PlatformInstrumentation_h

#include <wtf/text/WTFString.h>

#if PLATFORM(CHROMIUM)
#include "TraceEvent.h"
#endif

namespace WebCore {

class PlatformInstrumentationClient {
public:
    virtual ~PlatformInstrumentationClient();

    virtual void willDecodeImage(const String& imageType) = 0;
    virtual void didDecodeImage() = 0;
    virtual void willResizeImage(bool shouldCache) = 0;
    virtual void didResizeImage() = 0;
};

class PlatformInstrumentation {
public:
    static void setClient(PlatformInstrumentationClient*);
    static bool hasClient() { return m_client; }

    static void willDecodeImage(const String& imageType);
    static void didDecodeImage();
    static void willResizeImage(bool shouldCache);
    static void didResizeImage();

private:
    static PlatformInstrumentationClient* m_client;
};

#define FAST_RETURN_IF_NO_CLIENT() if (!m_client) return;

inline void PlatformInstrumentation::willDecodeImage(const String& imageType)
{
#if PLATFORM(CHROMIUM)
    TRACE_EVENT_BEGIN1("webkit", "Image Decode", "imageType", TRACE_STR_COPY(imageType.ascii().data()));
#endif
    FAST_RETURN_IF_NO_CLIENT();
    m_client->willDecodeImage(imageType);
}

inline void PlatformInstrumentation::didDecodeImage()
{
#if PLATFORM(CHROMIUM)
    TRACE_EVENT_END0("webkit", "Image Decode");
#endif
    FAST_RETURN_IF_NO_CLIENT();
    m_client->didDecodeImage();
}

inline void PlatformInstrumentation::willResizeImage(bool shouldCache)
{
#if PLATFORM(CHROMIUM)
    TRACE_EVENT_BEGIN1("webkit", "Image Resize", "cached", shouldCache);
#endif
    FAST_RETURN_IF_NO_CLIENT();
    m_client->willResizeImage(shouldCache);
}

inline void PlatformInstrumentation::didResizeImage()
{
#if PLATFORM(CHROMIUM)
    TRACE_EVENT_END0("webkit", "Image Resize");
#endif
    FAST_RETURN_IF_NO_CLIENT();
    m_client->didResizeImage();
}

} // namespace WebCore

#endif // PlatformInstrumentation_h
