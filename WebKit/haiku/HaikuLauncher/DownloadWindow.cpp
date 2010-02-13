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
#include <Button.h>
#include <GridLayoutBuilder.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <SpaceLayoutItem.h>
#include <StatusBar.h>
#include <stdio.h>

enum {
    REMOVE_FINISHED_DOWNLOADS = 'rmfd'
};

DownloadWindow::DownloadWindow(BRect frame, bool visible)
    : BWindow(frame, "Downloads",
        B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
        B_AUTO_UPDATE_SIZE_LIMITS | B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE)
{
	SetLayout(new BGroupLayout(B_VERTICAL));

	BGroupView* downloadsGroupView = new BGroupView(B_VERTICAL);
	downloadsGroupView->SetViewColor(245, 245, 245);
	m_downloadViewsLayout = downloadsGroupView->GroupLayout();

    BMenuBar* menuBar = new BMenuBar("Menu bar");
    BMenu* menu = new BMenu("Window");
    menu->AddItem(new BMenuItem("Minimize", new BMessage(B_QUIT_REQUESTED), 'M'));
    menuBar->AddItem(menu);

    BScrollView* scrollView = new BScrollView("Downloads scroll view",
        downloadsGroupView, 0, false, true, B_NO_BORDER);

    m_removeFinishedButton = new BButton("Remove finished",
        new BMessage(REMOVE_FINISHED_DOWNLOADS));
	m_removeFinishedButton->SetEnabled(false);

    AddChild(BGroupLayoutBuilder(B_VERTICAL)
        .Add(menuBar)
        .Add(scrollView)
        .Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
        .Add(BGroupLayoutBuilder(B_HORIZONTAL)
            .AddGlue()
            .Add(m_removeFinishedButton)
            .SetInsets(5, 5, 5, 5)
        )
    );

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
    case REMOVE_FINISHED_DOWNLOADS:
        removeFinishedDownloads();
        break;
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
    	SetViewColor(245, 245, 245);
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

    void downloadFinished()
    {
    	m_download = NULL;
    }

private:
    BStatusBar* m_statusBar;
    WebDownload* m_download;
    off_t m_expectedSize;
};

void DownloadWindow::downloadStarted(WebDownload* download)
{
	m_downloadViewsLayout->AddView(new DownloadProgressView(download));
}

void DownloadWindow::downloadFinished(WebDownload* download)
{
	int32 finishedCount = 0;
	for (int32 i = 0; BLayoutItem* item = m_downloadViewsLayout->ItemAt(i); i++) {
        DownloadProgressView* view = dynamic_cast<DownloadProgressView*>(item->View());
        if (!view)
            continue;
        if (view->download() == download) {
            view->downloadFinished();
            finishedCount++;
        } else if (!view->download())
            finishedCount++;
	}
	m_removeFinishedButton->SetEnabled(finishedCount > 0);
}

void DownloadWindow::removeFinishedDownloads()
{
	for (int32 i = m_downloadViewsLayout->CountItems() - 1;
	        BLayoutItem* item = m_downloadViewsLayout->ItemAt(i); i--) {
        DownloadProgressView* view = dynamic_cast<DownloadProgressView*>(item->View());
        if (!view)
            continue;
        if (!view->download()) {
            view->RemoveSelf();
            delete view;
        }
	}
	m_removeFinishedButton->SetEnabled(false);
}
