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
#include "WebDownloadPrivate.h"

#include "CString.h"
#include "NotImplemented.h"
#include "ResourceHandle.h"
#include "ResourceHandleInternal.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "WebDownload.h"
#include "WebPage.h"
#include <Entry.h>
#include <Message.h>
#include <Messenger.h>
#include <NodeInfo.h>

namespace BPrivate {

WebDownloadPrivate::WebDownloadPrivate(const ResourceRequest& request)
    : m_webDownload(0)
    , m_resourceHandle(ResourceHandle::create(request, this, 0, false, false, false))
    , m_currentSize(0)
    , m_expectedSize(0)
    , m_url(request.url().string())
    , m_path("/boot/home/Desktop/")
    , m_filename("Download")
    , m_mimeType("application/octet-stream")
    , m_file()
    , m_lastProgressReportTime(0)
{
}

WebDownloadPrivate::WebDownloadPrivate(ResourceHandle* handle,
        const ResourceRequest& request, const ResourceResponse& response)
    : m_webDownload(0)
    , m_resourceHandle(handle)
    , m_currentSize(0)
    , m_expectedSize(0)
    , m_url()
    , m_path("/boot/home/Desktop/")
    , m_filename("Download")
    , m_mimeType("application/octet-stream")
    , m_file()
    , m_lastProgressReportTime(0)
{
	m_resourceHandle->setClient(this);
	// Call the hook manually to figure out the details of the request
	didReceiveResponse(handle, response);
}

void WebDownloadPrivate::didReceiveResponse(ResourceHandle*, const ResourceResponse& response)
{
    if (!response.isNull()) {
    	if (!response.suggestedFilename().isEmpty())
            m_filename = response.suggestedFilename();
        else {
        	WebCore::KURL url(response.url());
        	url.setQuery(WebCore::String());
        	url.removeFragmentIdentifier();
            m_filename = decodeURLEscapeSequences(url.lastPathComponent()).utf8().data();
        }
        if (response.mimeType().length())
            m_mimeType = response.mimeType();
        m_expectedSize = response.expectedContentLength();
    }

    m_url = response.url().string();
}

void WebDownloadPrivate::didReceiveData(ResourceHandle*, const char* data, int length, int lengthReceived)
{
	if (m_file.InitCheck() != B_OK)
		createFile();

    ssize_t bytesWritten = m_file.Write(data, length);
    if (bytesWritten != (ssize_t)length) {
        // FIXME: Report error
        return;
    }
    m_currentSize += length;

    // FIXME: Report total size update, if m_currentSize greater than previous total size
    BMessage message(B_DOWNLOAD_PROGRESS);
    message.AddFloat("progress", m_currentSize * 100.0 / m_expectedSize);
    message.AddInt64("current size", m_currentSize);
    message.AddInt64("expected size", m_expectedSize);
    m_progressListener.SendMessage(&message);
}

void WebDownloadPrivate::didFinishLoading(ResourceHandle* handle)
{
    handleFinished(handle, B_DOWNLOAD_FINISHED);
}

void WebDownloadPrivate::didFail(ResourceHandle* handle, const ResourceError& error)
{
    handleFinished(handle, B_DOWNLOAD_FAILED);
}

void WebDownloadPrivate::wasBlocked(ResourceHandle* handle)
{
    // FIXME: Implement this when we have the new frame loader signals
    // and error handling.
    handleFinished(handle, B_DOWNLOAD_BLOCKED);
}

void WebDownloadPrivate::cannotShowURL(ResourceHandle* handle)
{
    // FIXME: Implement this when we have the new frame loader signals
    // and error handling.
    handleFinished(handle, B_DOWNLOAD_CANNOT_SHOW_URL);
}

void WebDownloadPrivate::setDownload(BWebDownload* download)
{
    m_webDownload = download;
}

void WebDownloadPrivate::start(const BPath& path)
{
	if (path.InitCheck() == B_OK)
		m_path = path;
}

void WebDownloadPrivate::cancel()
{
    m_resourceHandle->cancel();
}

void WebDownloadPrivate::setProgressListener(const BMessenger& listener)
{
	m_progressListener = listener;
}

// #pragma mark - private

void WebDownloadPrivate::handleFinished(WebCore::ResourceHandle* handle, uint32 status)
{
	handle->setClient(0);
	if (m_progressListener.IsValid()) {
        BMessage message(B_DOWNLOAD_REMOVED);
        message.AddPointer("download", m_webDownload);
        // Block until the listener has released the object on it's side...
        BMessage reply;
        m_progressListener.SendMessage(&message, &reply);
	}
	delete m_webDownload;
}

void WebDownloadPrivate::createFile()
{
    m_path.Append(m_filename.String());
	if (m_file.SetTo(m_path.Path(), B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY) == B_OK) {
		BNodeInfo info(&m_file);
		info.SetType(m_mimeType.String());
		m_file.WriteAttrString("META:url", &m_url);
	}

    if (m_progressListener.IsValid()) {
        BMessage message(B_DOWNLOAD_STARTED);
        message.AddString("path", m_path.Path());
        m_progressListener.SendMessage(&message);
    }
}

} // namespace BPrivate

