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

#include "config.h"
#include "WebDownload.h"

#include "CString.h"
#include "NotImplemented.h"
#include "ResourceHandleInternal.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include <Entry.h>


WebDownload::WebDownload()
    : m_suggestedFileName()
    , m_currentSize(0)
    , m_file()
    , m_lastProgressReportTime(0)
{
}

void WebDownload::didReceiveResponse(ResourceHandle*, const ResourceResponse& response)
{
    if (!response.isNull() && !response.suggestedFilename().isEmpty())
        m_suggestedFileName = response.suggestedFilename(); //.utf8().data();
}

void WebDownload::didReceiveData(ResourceHandle*, const char* data, int length, int lengthReceived)
{
    ssize_t bytesWritten = m_file.Write(data, length);
    if (bytesWritten != (ssize_t)length) {
        // FIXME: Report error
        return;
    }
    m_currentSize += length;

    // FIXME: Report total size update, if m_currentSize greater than previous total size
    
}

void WebDownload::didFinishLoading(ResourceHandle*)
{
    // FIXME: Send notification
}

void WebDownload::didFail(ResourceHandle*, const ResourceError& error)
{
    // FIXME: Send notification
}

void WebDownload::wasBlocked(ResourceHandle*)
{
    // FIXME: Implement this when we have the new frame loader signals
    // and error handling.
    notImplemented();
}

void WebDownload::cannotShowURL(ResourceHandle*)
{
    // FIXME: Implement this when we have the new frame loader signals
    // and error handling.
    notImplemented();
}

void WebDownload::cancel()
{
    // FIXME: m_resourceHandle->cancel();
}

