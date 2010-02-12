/*
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * All rights reserved.
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

#ifndef WebDownload_h
#define WebDownload_h

#include "Noncopyable.h"
#include "ResourceHandleClient.h"
#include <File.h>
#include <String.h>

namespace WebCore {
class ResourceError;
class ResourceHandle;
class ResourceResponse;
}

class WebDownload : public Noncopyable, public ResourceHandleClient {
public:
    WebDownload();

    virtual void didReceiveResponse(WebCore::ResourceHandle*, const WebCore::ResourceResponse&);
    virtual void didReceiveData(WebCore::ResourceHandle*, const char*, int, int);
    virtual void didFinishLoading(WebCore::ResourceHandle*);
    virtual void didFail(WebCore::ResourceHandle*, const WebCore::ResourceError&);
    virtual void wasBlocked(WebCore::ResourceHandle*);
    virtual void cannotShowURL(WebCore::ResourceHandle*);

    void cancel();

private:
    BString m_suggestedFileName;
    off_t m_currentSize;
    BFile m_file;
    bigtime_t m_lastProgressReportTime;
};

#endif // WebDownload_h
