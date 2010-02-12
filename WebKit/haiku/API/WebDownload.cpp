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
#include "ResourceHandle.h"
#include "ResourceHandleInternal.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "WebProcess.h"
#include <Entry.h>
#include <Message.h>
#include <Messenger.h>
#include <NodeInfo.h>
#include <Path.h>

WebDownload::WebDownload(WebProcess* webProcess, const ResourceRequest& request)
    : m_webPocess(webProcess)
    , m_resourceHandle(ResourceHandle::create(request, this, 0, false, false, false))
    , m_suggestedFileName("Download")
    , m_currentSize(0)
    , m_expectedSize(0)
    , m_file()
    , m_lastProgressReportTime(0)
{
printf("WebDownload::WebDownload()\n");
}

WebDownload::WebDownload(WebProcess* webProcess, ResourceHandle* handle,
        const ResourceRequest& request, const ResourceResponse& response)
    : m_webPocess(webProcess)
    , m_resourceHandle(handle)
    , m_suggestedFileName("Download")
    , m_currentSize(0)
    , m_expectedSize(0)
    , m_file()
    , m_lastProgressReportTime(0)
{
	m_resourceHandle->setClient(this);
	// Call the hook manually to figure out the details of the request
	didReceiveResponse(handle, response);
}

void WebDownload::didReceiveResponse(ResourceHandle*, const ResourceResponse& response)
{
	BString mimeType("application/octet-stream");
    if (!response.isNull()) {
    	if (!response.suggestedFilename().isEmpty())
            m_suggestedFileName = response.suggestedFilename();
        else {
        	WebCore::KURL url(response.url());
        	url.setQuery(WebCore::String());
        	url.removeFragmentIdentifier();
            m_suggestedFileName = decodeURLEscapeSequences(url.lastPathComponent()).utf8().data();
        }
        if (response.mimeType().length())
            mimeType = response.mimeType();
        m_expectedSize = response.expectedContentLength();
    }
    BPath path("/boot/home/Desktop/");
    path.Append(m_suggestedFileName.String());
	if (m_file.SetTo(path.Path(), B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY) == B_OK) {
		BNodeInfo info(&m_file);
		info.SetType(mimeType.String());
	}
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
    BMessage message(DOWNLOAD_PROGRESS);
    message.AddFloat("progress", m_currentSize * 100.0 / m_expectedSize);
    m_progressListener.SendMessage(&message);
}

void WebDownload::didFinishLoading(ResourceHandle* handle)
{
printf("WebDownload::didFinishLoading()\n");
    m_webPocess->downloadFinished(handle, this, DOWNLOAD_FINISHED);
}

void WebDownload::didFail(ResourceHandle* handle, const ResourceError& error)
{
printf("WebDownload::didFail()\n");
    m_webPocess->downloadFinished(handle, this, DOWNLOAD_FAILED);
}

void WebDownload::wasBlocked(ResourceHandle* handle)
{
printf("WebDownload::wasBlocked()\n");
    // FIXME: Implement this when we have the new frame loader signals
    // and error handling.
    m_webPocess->downloadFinished(handle, this, DOWNLOAD_BLOCKED);
}

void WebDownload::cannotShowURL(ResourceHandle* handle)
{
printf("WebDownload::cannotShowURL()\n");
    // FIXME: Implement this when we have the new frame loader signals
    // and error handling.
    m_webPocess->downloadFinished(handle, this, DOWNLOAD_CANNOT_SHOW_URL);
}

void WebDownload::start()
{
	// FIXME, the download is already running, and we cannot begin it paused...
}

void WebDownload::cancel()
{
    m_resourceHandle->cancel();
}

void WebDownload::setProgressListener(const BMessenger& listener)
{
	m_progressListener = listener;
}
