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

#include "config.h"
#include "DownloadManager.h"

#include "Download.h"
#include "WebProcess.h"
#include <wtf/StdLibExtras.h>

using namespace WebCore;

namespace WebKit {

DownloadManager& DownloadManager::shared()
{
    DEFINE_STATIC_LOCAL(DownloadManager, downloadManager, ());
    return downloadManager;
}

DownloadManager::DownloadManager()
{
}

void DownloadManager::startDownload(uint64_t downloadID, WebPage* initiatingPage, const ResourceRequest& request)
{
    OwnPtr<Download> download = Download::create(downloadID, request);
    download->start(initiatingPage);

    ASSERT(!m_downloads.contains(downloadID));
    m_downloads.set(downloadID, download.leakPtr());
}

void DownloadManager::convertHandleToDownload(uint64_t downloadID, WebPage* initiatingPage, ResourceHandle* handle, const ResourceRequest& request, const ResourceResponse& response)
{
    OwnPtr<Download> download = Download::create(downloadID, request);

    download->startWithHandle(initiatingPage, handle, response);
    ASSERT(!m_downloads.contains(downloadID));
    m_downloads.set(downloadID, download.leakPtr());
}

void DownloadManager::cancelDownload(uint64_t downloadID)
{
    Download* download = m_downloads.get(downloadID);
    if (!download)
        return;

    download->cancel();
}

void DownloadManager::downloadFinished(Download* download)
{
    ASSERT(m_downloads.contains(download->downloadID()));
    m_downloads.remove(download->downloadID());

    delete download;
}

#if PLATFORM(QT)
void DownloadManager::startTransfer(uint64_t downloadID, const String& destination)
{
    ASSERT(m_downloads.contains(downloadID));
    Download* download = m_downloads.get(downloadID);
    download->startTransfer(destination);
}
#endif

} // namespace WebKit
