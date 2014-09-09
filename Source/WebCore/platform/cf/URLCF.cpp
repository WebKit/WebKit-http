/*
 * Copyright (C) 2004, 2008, 2013 Apple Inc. All rights reserved.
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

#include "config.h"
#include "URL.h"

#include "CFURLExtras.h"
#include <CoreFoundation/CFURL.h>
#include <wtf/text/CString.h>

#if PLATFORM(IOS)
#include "RuntimeApplicationChecksIOS.h"
#endif

namespace WebCore {

URL::URL(CFURLRef url)
{
    if (!url) {
        invalidate();
        return;
    }

    // FIXME: Why is it OK to ignore base URL here?
    CString urlBytes;
    getURLBytes(url, urlBytes);
    parse(urlBytes.data());
}

#if !USE(FOUNDATION)
RetainPtr<CFURLRef> URL::createCFURL() const
{
    // FIXME: What should this return for invalid URLs?
    // Currently it throws away the high bytes of the characters in the string in that case,
    // which is clearly wrong.
    URLCharBuffer buffer;
    copyToBuffer(buffer);
    return createCFURLFromBuffer(buffer.data(), buffer.size());
}
#endif

String URL::fileSystemPath() const
{
    RetainPtr<CFURLRef> cfURL = createCFURL();
    if (!cfURL)
        return String();

#if PLATFORM(WIN)
    CFURLPathStyle pathStyle = kCFURLWindowsPathStyle;
#else
    CFURLPathStyle pathStyle = kCFURLPOSIXPathStyle;
#endif
    return adoptCF(CFURLCopyFileSystemPath(cfURL.get(), pathStyle)).get();
}

}
