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
#include "DownloadWindow.h"

#include "WebDownload.h"
#include "WebProcess.h"
#include <GridLayoutBuilder.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <SeparatorView.h>
#include <SpaceLayoutItem.h>
#include <StatusBar.h>
#include <stdio.h>

DownloadWindow::DownloadWindow(BRect frame, bool visible)
    : BWindow(frame, "Downloads",
        B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
        B_AUTO_UPDATE_SIZE_LIMITS | B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE)
{
	SetLayout(new BGroupLayout(B_VERTICAL));
	if (!visible)
	    Minimize(true);
	Show();
}

DownloadWindow::~DownloadWindow()
{
}

void DownloadWindow::MessageReceived(BMessage* message)
{
    switch (message->what) {
    case DOWNLOAD_ADDED: {
        WebDownload* download;
        if (message->FindPointer("download", reinterpret_cast<void**>(&download)) == B_OK)
            downloadStarted(download);
        break;
    }
    case DOWNLOAD_REMOVED: {
        WebDownload* download;
        if (message->FindPointer("download", reinterpret_cast<void**>(&download)) == B_OK)
            downloadFinished(download);
        break;
    }
    default:
        BWindow::MessageReceived(message);
        break;
    }
}

bool DownloadWindow::QuitRequested()
{
	if (!IsMinimized())
        Minimize(true);
    return false;
}

class DownloadProgressView : public BGroupView {
public:
    DownloadProgressView(WebDownload* download)
        : BGroupView(B_HORIZONTAL)
        , m_download(download)
        , m_expectedSize(download->expectedSize())
    {
    	GroupLayout()->SetInsets(5, 5, 5, 5);
    	m_statusBar = new BStatusBar("download progress", download->filename().String());
    	m_statusBar->SetMaxValue(100);
    	AddChild(m_statusBar);
    }

    virtual void AttachedToWindow()
    {
        m_download->setProgressListener(BMessenger(this));
    }

    virtual void MessageReceived(BMessage* message)
    {
        switch (message->what) {
        case WebDownload::DOWNLOAD_PROGRESS: {
        	float progress;
        	if (message->FindFloat("progress", &progress) == B_OK)
        	    m_statusBar->SetTo(progress);
            break;
        }
        default:
            BGroupView::MessageReceived(message);
        }
    }

    WebDownload* download() const
    {
    	return m_download;
    }

private:
    BStatusBar* m_statusBar;
    WebDownload* m_download;
    off_t m_expectedSize;
};

void DownloadWindow::downloadStarted(WebDownload* download)
{
	AddChild(new DownloadProgressView(download));
}

void DownloadWindow::downloadFinished(WebDownload* download)
{
}
