/*
 * Copyright (C) 2004, 2008 Apple Inc. All rights reserved.
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
#import "KURL.h"

#import "FoundationExtras.h"
#import <CoreFoundation/CFURL.h>

using namespace WTF;

namespace WebCore {

typedef Vector<char, 512> CharBuffer;
extern CFURLRef createCFURLFromBuffer(const CharBuffer& buffer);

KURL::KURL(NSURL *url)
{
    if (!url) {
        invalidate();
        return;
    }

    CFIndex bytesLength = CFURLGetBytes(reinterpret_cast<CFURLRef>(url), 0, 0);
    Vector<char, 512> buffer(bytesLength + 1);
    char* bytes = &buffer[0];
    CFURLGetBytes(reinterpret_cast<CFURLRef>(url), reinterpret_cast<UInt8*>(bytes), bytesLength);
    bytes[bytesLength] = '\0';
#if !USE(WTFURL)
    parse(bytes);
#else
    m_urlImpl = adoptRef(new KURLWTFURLImpl());
    String urlString(bytes, bytesLength);
    m_urlImpl->m_parsedURL = ParsedURL(urlString);
    if (!m_urlImpl->m_parsedURL.isValid())
        m_urlImpl->m_invalidUrlString = urlString;
#endif // USE(WTFURL)
}

KURL::operator NSURL *() const
{
    return HardAutorelease(createCFURL());
}

// We use the toll-free bridge between NSURL and CFURL to
// create a CFURLRef supporting both empty and null values.
CFURLRef KURL::createCFURL() const
{
    if (isNull())
        return 0;

    if (isEmpty())
        return reinterpret_cast<CFURLRef>([[NSURL alloc] initWithString:@""]);

    CharBuffer buffer;
#if !USE(WTFURL)
    copyToBuffer(buffer);
#else
    String urlString = string();
    buffer.resize(urlString.length());
    size_t length = urlString.length();
    for (size_t i = 0; i < length; i++)
        buffer[i] = static_cast<char>(urlString[i]);
#endif
    return createCFURLFromBuffer(buffer);
}



}
