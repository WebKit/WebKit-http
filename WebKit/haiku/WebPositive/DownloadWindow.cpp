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

#include "BrowserApp.h"
#include "WebDownload.h"
#include "WebPage.h"
#include <Alert.h>
#include <Bitmap.h>
#include <Button.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <GridLayoutBuilder.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Roster.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <SpaceLayoutItem.h>
#include <StatusBar.h>
#include <stdio.h>

enum {
    REMOVE_FINISHED_DOWNLOADS = 'rmfd',
    OPEN_DOWNLOAD = 'opdn',
    RESTART_DOWNLOAD = 'rsdn',
    CANCEL_DOWNLOAD = 'cndn',
    REMOVE_DOWNLOAD = 'rmdn',
    SAVE_SETTINGS = 'svst'
};

class IconView : public BView {
public:
    IconView(const BEntry& entry)
        : BView("Download icon", B_WILL_DRAW)
        , m_iconBitmap(BRect(0, 0, 31, 31), 0, B_RGBA32)
    {
    	BNode node(&entry);
    	BNodeInfo info(&node);
    	info.GetTrackerIcon(&m_iconBitmap, B_LARGE_ICON);
    	SetDrawingMode(B_OP_OVER);
    }

    IconView(BMessage* archive)
        : BView("Download icon", B_WILL_DRAW)
        , m_iconBitmap(archive)
    {
    	SetDrawingMode(B_OP_OVER);
    }

    status_t saveSettings(BMessage* archive)
    {
    	return m_iconBitmap.Archive(archive);
    }

    virtual void AttachedToWindow()
    {
    	SetViewColor(Parent()->ViewColor());
    }

    virtual void Draw(BRect updateRect)
    {
    	DrawBitmapAsync(&m_iconBitmap);
    }

    virtual BSize MinSize()
    {
    	return BSize(m_iconBitmap.Bounds().Width(), m_iconBitmap.Bounds().Height());
    }

    virtual BSize PreferredSize()
    {
    	return MinSize();
    }

    virtual BSize MaxSize()
    {
    	return MinSize();
    }

private:
    BBitmap m_iconBitmap;
};

class SmallButton : public BButton {
public:
    SmallButton(const char* label, BMessage* message = NULL)
        : BButton(label, message)
    {
    	BFont font;
    	GetFont(&font);
    	float size = ceilf(font.Size() * 0.8);
    	font.SetSize(max_c(8, size));
    	SetFont(&font, B_FONT_SIZE);
    }
};

class DownloadProgressView : public BGridView {
public:
    DownloadProgressView(BWebDownload* download)
        : BGridView(8, 3)
        , m_download(download)
        , m_url(download->URL())
        , m_path(download->Path())
        , m_expectedSize(download->ExpectedSize())
    {
    }

    DownloadProgressView(const BMessage* archive)
        : BGridView(8, 3)
        , m_download(NULL)
        , m_url()
        , m_path()
        , m_expectedSize(0)
    {
    	const char* string;
    	if (archive->FindString("path", &string) == B_OK)
    	    m_path.SetTo(string);
    	if (archive->FindString("url", &string) == B_OK)
    	    m_url = string;
    }

    bool init(BMessage* archive = NULL)
    {
        BEntry entry(m_path.Path());
        if (!entry.Exists() && !archive)
        	return false;

    	SetViewColor(245, 245, 245);
    	SetFlags(Flags() | B_FULL_UPDATE_ON_RESIZE | B_WILL_DRAW);

    	BGridLayout* layout = GridLayout();
    	m_statusBar = new BStatusBar("download progress", m_path.Leaf());
    	m_statusBar->SetMaxValue(100);
    	if (archive) {
    		float value;
    		if (archive->FindFloat("value", &value) == B_OK)
    	        m_statusBar->SetTo(value);
    	}
    	m_statusBar->SetBarHeight(12);

        if (entry.Exists())
            m_iconView = new IconView(entry);
        else
            m_iconView = new IconView(archive);

        if (!m_download && m_statusBar->CurrentValue() < 100)
            m_button1 = new SmallButton("Restart", new BMessage(RESTART_DOWNLOAD));
        else {
            m_button1 = new SmallButton("Open", new BMessage(OPEN_DOWNLOAD));
            m_button1->SetEnabled(m_download == NULL);
        }
        if (m_download)
            m_button2 = new SmallButton("Cancel", new BMessage(CANCEL_DOWNLOAD));
        else {
            m_button2 = new SmallButton("Remove", new BMessage(REMOVE_DOWNLOAD));
            m_button2->SetEnabled(m_download == NULL);
        }

		layout->SetInsets(8, 5, 5, 6);
    	layout->AddView(m_iconView, 0, 0, 1, 2);
    	layout->AddView(m_statusBar, 1, 0, 1, 2);
    	layout->AddView(m_button1, 2, 0);
    	layout->AddView(m_button2, 2, 1);

    	return true;
    }

    status_t saveSettings(BMessage* archive)
    {
    	if (!archive)
    	    return B_BAD_VALUE;
    	status_t ret = archive->AddString("path", m_path.Path());
    	if (ret == B_OK)
    	    ret = archive->AddString("url", m_url.String());
    	if (ret == B_OK)
    	    ret = archive->AddFloat("value", m_statusBar->CurrentValue());
    	if (ret == B_OK)
    	    ret = m_iconView->saveSettings(archive);
    	return ret;
    }

    virtual void AttachedToWindow()
    {
    	if (m_download)
            m_download->SetProgressListener(BMessenger(this));
        m_button1->SetTarget(this);
        m_button2->SetTarget(this);
    }

    virtual void AllAttached()
    {
    	SetViewColor(B_TRANSPARENT_COLOR);
    	SetLowColor(245, 245, 245);
    	SetHighColor(tint_color(LowColor(), B_DARKEN_1_TINT));
    }

    virtual void Draw(BRect updateRect)
    {
    	BRect bounds(Bounds());
    	bounds.bottom--;
    	FillRect(bounds, B_SOLID_LOW);
    	bounds.bottom++;
    	StrokeLine(bounds.LeftBottom(), bounds.RightBottom());
    }

    virtual void MessageReceived(BMessage* message)
    {
        switch (message->what) {
        case B_DOWNLOAD_PROGRESS: {
        	float progress;
        	if (message->FindFloat("progress", &progress) == B_OK)
        	    m_statusBar->SetTo(progress);
            break;
        }
        case OPEN_DOWNLOAD: {
        	// TODO: In case of executable files, ask the user first!
        	entry_ref ref;
        	status_t status = get_ref_for_path(m_path.Path(), &ref);
        	if (status == B_OK)
        		status = be_roster->Launch(&ref);
        	if (status != B_OK && status != B_ALREADY_RUNNING) {
        		BAlert* alert = new BAlert("Open download error",
        		    "The download could not be opened.", "OK");
        		alert->Go(NULL);
        	}
            break;
        }
        case RESTART_DOWNLOAD:
            // TODO: 
            break;

        case CANCEL_DOWNLOAD:
        	m_download->Cancel();
        	downloadCanceled();
            break;

        case REMOVE_DOWNLOAD: {
        	Window()->PostMessage(SAVE_SETTINGS);
        	RemoveSelf();
        	delete this;
        	// TOAST!
            return;
        }
        default:
            BGridView::MessageReceived(message);
        }
    }

    BWebDownload* download() const
    {
    	return m_download;
    }

    const BString& url() const
    {
    	return m_url;
    }

    void downloadFinished()
    {
    	m_download = NULL;
    	m_button1->SetEnabled(true);
    	m_button2->SetLabel("Remove");
    	m_button2->SetMessage(new BMessage(REMOVE_DOWNLOAD));
    	m_button2->SetEnabled(true);
    }

    void downloadCanceled()
    {
    	m_download = NULL;
    	m_button1->SetLabel("Restart");
    	m_button1->SetMessage(new BMessage(RESTART_DOWNLOAD));
    	m_button1->SetEnabled(true);
    	m_button2->SetLabel("Remove");
    	m_button2->SetMessage(new BMessage(REMOVE_DOWNLOAD));
    	m_button2->SetEnabled(true);
    }

private:
    IconView* m_iconView;
    BStatusBar* m_statusBar;
    SmallButton* m_button1;
    SmallButton* m_button2;
    BWebDownload* m_download;
    BString m_url;
    BPath m_path;
    off_t m_expectedSize;
};

class DownloadsContainerView : public BGroupView {
public:
	DownloadsContainerView()
		: BGroupView(B_VERTICAL)
	{
	    SetViewColor(245, 245, 245);
	    AddChild(BSpaceLayoutItem::CreateGlue());
	}

    virtual BSize MinSize()
    {
    	BSize minSize = BGroupView::MinSize();
    	return BSize(minSize.width, 80);
    }

protected:
	virtual void DoLayout()
	{
		BGroupView::DoLayout();
		if (BScrollBar* scrollBar = ScrollBar(B_VERTICAL)) {
			BSize minSize = BGroupView::MinSize();
			float height = Bounds().Height();
			float max = minSize.height - height;
			scrollBar->SetRange(0, max);
			if (minSize.height > 0)
			    scrollBar->SetProportion(height / minSize.height);
			else
			    scrollBar->SetProportion(1);
		}
	}
};

class DownloadContainerScrollView : public BScrollView {
public:
    DownloadContainerScrollView(BView* target)
        : BScrollView("Downloads scroll view", target, 0, false, true, B_NO_BORDER)
    {
    }

protected:
    virtual void DoLayout()
    {
    	BScrollView::DoLayout();
        // Tweak scroll bar layout to hide part of the frame for better looks.
        BScrollBar* scrollBar = ScrollBar(B_VERTICAL);
        scrollBar->MoveBy(1, -1);
        scrollBar->ResizeBy(0, 2);
        Target()->ResizeBy(1, 0);
    }
};

DownloadWindow::DownloadWindow(BRect frame, bool visible)
    : BWindow(frame, "Downloads",
        B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
        B_AUTO_UPDATE_SIZE_LIMITS | B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE)
{
	SetLayout(new BGroupLayout(B_VERTICAL));

	DownloadsContainerView* downloadsGroupView = new DownloadsContainerView();
	m_downloadViewsLayout = downloadsGroupView->GroupLayout();

    BMenuBar* menuBar = new BMenuBar("Menu bar");
    BMenu* menu = new BMenu("Window");
    menu->AddItem(new BMenuItem("Hide", new BMessage(B_QUIT_REQUESTED), 'H'));
    menuBar->AddItem(menu);

    BScrollView* scrollView = new DownloadContainerScrollView(downloadsGroupView);

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

    loadSettings();
    // Small trick to get the correct enabled status of the Remove finished button
    downloadFinished(NULL);

	if (!visible)
	    Hide();
	Show();
}

DownloadWindow::~DownloadWindow()
{
	// Only necessary to save the current progress of unfinished downloads:
	saveSettings();
}

void DownloadWindow::MessageReceived(BMessage* message)
{
    switch (message->what) {
    case B_DOWNLOAD_ADDED: {
        BWebDownload* download;
        if (message->FindPointer("download", reinterpret_cast<void**>(&download)) == B_OK)
            downloadStarted(download);
        break;
    }
    case B_DOWNLOAD_REMOVED: {
        BWebDownload* download;
        if (message->FindPointer("download", reinterpret_cast<void**>(&download)) == B_OK)
            downloadFinished(download);
        break;
    }
    case REMOVE_FINISHED_DOWNLOADS:
        removeFinishedDownloads();
        break;
    case SAVE_SETTINGS:
        saveSettings();
        break;
    default:
        BWindow::MessageReceived(message);
        break;
    }
}

bool DownloadWindow::QuitRequested()
{
	if (!IsHidden())
        Hide();
    return false;
}

void DownloadWindow::downloadStarted(BWebDownload* download)
{
	int32 finishedCount = 0;
	int32 index = 0;
	for (int32 i = m_downloadViewsLayout->CountItems() - 1;
	        BLayoutItem* item = m_downloadViewsLayout->ItemAt(i); i--) {
        DownloadProgressView* view = dynamic_cast<DownloadProgressView*>(item->View());
        if (!view)
            continue;
        if (view->url() == download->URL()) {
        	index = i;
            view->RemoveSelf();
            delete view;
        } else if (!view->download())
            finishedCount++;
	}
	m_removeFinishedButton->SetEnabled(finishedCount > 0);
	DownloadProgressView* view = new DownloadProgressView(download);
	if (!view->init())
		return;
	m_downloadViewsLayout->AddView(index, view);
	saveSettings();

	SetWorkspaces(B_CURRENT_WORKSPACE);
	if (IsHidden())
		Show();
}

void DownloadWindow::downloadFinished(BWebDownload* download)
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
	if (download)
        saveSettings();
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
	saveSettings();
}

void DownloadWindow::saveSettings()
{
	BFile file;
	if (!openSettingsFile(file, B_ERASE_FILE | B_CREATE_FILE | B_WRITE_ONLY))
	    return;
	BMessage message;
	for (int32 i = m_downloadViewsLayout->CountItems() - 1;
	        BLayoutItem* item = m_downloadViewsLayout->ItemAt(i); i--) {
        DownloadProgressView* view = dynamic_cast<DownloadProgressView*>(item->View());
        if (!view)
            continue;
       BMessage downloadArchive;
       if (view->saveSettings(&downloadArchive) == B_OK)
           message.AddMessage("download", &downloadArchive);
	}
	message.Flatten(&file);
}

void DownloadWindow::loadSettings()
{
	BFile file;
	if (!openSettingsFile(file, B_READ_ONLY))
	    return;
	BMessage message;
	if (message.Unflatten(&file) != B_OK)
        return;
    BMessage downloadArchive;
	for (int32 i = 0; message.FindMessage("download", i, &downloadArchive) == B_OK; i++) {
        DownloadProgressView* view = new DownloadProgressView(&downloadArchive);
		if (!view->init(&downloadArchive))
			continue;
        m_downloadViewsLayout->AddView(0, view);
	}
}

bool DownloadWindow::openSettingsFile(BFile& file, uint32 mode)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK
		|| path.Append(kApplicationName) != B_OK
		|| path.Append("Downloads") != B_OK) {
		return false;
	}
	return file.SetTo(path.Path(), mode) == B_OK;
}


