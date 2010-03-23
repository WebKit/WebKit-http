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

#include "DownloadWindow.h"

#include <stdio.h>

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

#include "BrowserApp.h"
#include "SettingsMessage.h"
#include "WebDownload.h"
#include "WebPage.h"


enum {
	INIT = 'init',
	OPEN_DOWNLOADS_FOLDER = 'odnf',
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
		:
		BView("Download icon", B_WILL_DRAW),
		fIconBitmap(BRect(0, 0, 31, 31), 0, B_RGBA32),
		fDimmedIcon(false)
	{
		SetDrawingMode(B_OP_OVER);
		SetTo(entry);
	}

	IconView()
		:
		BView("Download icon", B_WILL_DRAW),
		fIconBitmap(BRect(0, 0, 31, 31), 0, B_RGBA32),
		fDimmedIcon(false)
	{
		SetDrawingMode(B_OP_OVER);
		memset(fIconBitmap.Bits(), 0, fIconBitmap.BitsLength());
	}

	IconView(BMessage* archive)
		:
		BView("Download icon", B_WILL_DRAW),
		fIconBitmap(archive),
		fDimmedIcon(true)
	{
		SetDrawingMode(B_OP_OVER);
	}

	void SetTo(const BEntry& entry)
	{
		BNode node(&entry);
		BNodeInfo info(&node);
		info.GetTrackerIcon(&fIconBitmap, B_LARGE_ICON);
		Invalidate();
	}

	void SetIconDimmed(bool iconDimmed)
	{
		if (fDimmedIcon != iconDimmed) {
			fDimmedIcon = iconDimmed;
			Invalidate();
		}
	}

	status_t SaveSettings(BMessage* archive)
	{
		return fIconBitmap.Archive(archive);
	}

	virtual void AttachedToWindow()
	{
		SetViewColor(Parent()->ViewColor());
	}

	virtual void Draw(BRect updateRect)
	{
		if (fDimmedIcon) {
			SetDrawingMode(B_OP_ALPHA);
			SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
			SetHighColor(0, 0, 0, 100);
		}
		DrawBitmapAsync(&fIconBitmap);
	}

	virtual BSize MinSize()
	{
		return BSize(fIconBitmap.Bounds().Width(), fIconBitmap.Bounds().Height());
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
	BBitmap	fIconBitmap;
	bool	fDimmedIcon;
};


class SmallButton : public BButton {
public:
	SmallButton(const char* label, BMessage* message = NULL)
		:
		BButton(label, message)
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
		:
		BGridView(8, 3),
		fDownload(download),
		fURL(download->URL()),
		fPath(download->Path()),
		fExpectedSize(download->ExpectedSize())
	{
	}

	DownloadProgressView(const BMessage* archive)
		:
		BGridView(8, 3),
		fDownload(NULL),
		fURL(),
		fPath(),
		fExpectedSize(0)
	{
		const char* string;
		if (archive->FindString("path", &string) == B_OK)
			fPath.SetTo(string);
		if (archive->FindString("url", &string) == B_OK)
			fURL = string;
	}

	bool Init(BMessage* archive = NULL)
	{
		SetViewColor(245, 245, 245);
		SetFlags(Flags() | B_FULL_UPDATE_ON_RESIZE | B_WILL_DRAW);

		BGridLayout* layout = GridLayout();
		if (archive) {
			fStatusBar = new BStatusBar("download progress", fPath.Leaf());
			float value;
			if (archive->FindFloat("value", &value) == B_OK)
				fStatusBar->SetTo(value);
		} else
			fStatusBar = new BStatusBar("download progress", "Download");
		fStatusBar->SetMaxValue(100);
		fStatusBar->SetBarHeight(12);

		// fPath is only valid when constructed from archive (fDownload == NULL)
		BEntry entry(fPath.Path());

		if (archive) {
			if (!entry.Exists())
				fIconView = new IconView(archive);
			else
				fIconView = new IconView(entry);
		} else
			fIconView = new IconView();

		if (!fDownload && (fStatusBar->CurrentValue() < 100 || !entry.Exists()))
			fTopButton = new SmallButton("Restart", new BMessage(RESTART_DOWNLOAD));
		else {
			fTopButton = new SmallButton("Open", new BMessage(OPEN_DOWNLOAD));
			fTopButton->SetEnabled(fDownload == NULL);
		}
		if (fDownload)
			fBottomButton = new SmallButton("Cancel", new BMessage(CANCEL_DOWNLOAD));
		else {
			fBottomButton = new SmallButton("Remove", new BMessage(REMOVE_DOWNLOAD));
			fBottomButton->SetEnabled(fDownload == NULL);
		}

		layout->SetInsets(8, 5, 5, 6);
		layout->AddView(fIconView, 0, 0, 1, 2);
		layout->AddView(fStatusBar, 1, 0, 1, 2);
		layout->AddView(fTopButton, 2, 0);
		layout->AddView(fBottomButton, 2, 1);

		return true;
	}

	status_t SaveSettings(BMessage* archive)
	{
		if (!archive)
			return B_BAD_VALUE;
		status_t ret = archive->AddString("path", fPath.Path());
		if (ret == B_OK)
			ret = archive->AddString("url", fURL.String());
		if (ret == B_OK)
			ret = archive->AddFloat("value", fStatusBar->CurrentValue());
		if (ret == B_OK)
			ret = fIconView->SaveSettings(archive);
		return ret;
	}

	virtual void AttachedToWindow()
	{
		if (fDownload)
			fDownload->SetProgressListener(BMessenger(this));
		fTopButton->SetTarget(this);
		fBottomButton->SetTarget(this);
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
			case B_DOWNLOAD_STARTED: {
				BString path;
				if (message->FindString("path", &path) != B_OK)
					break;
				fPath.SetTo(path);
				BEntry entry(fPath.Path());
				fIconView->SetTo(entry);
				fStatusBar->Reset(fPath.Leaf());
				break;
			};
			case B_DOWNLOAD_PROGRESS: {
				float progress;
				if (message->FindFloat("progress", &progress) == B_OK)
					fStatusBar->SetTo(progress);
				break;
			}
			case OPEN_DOWNLOAD: {
				// TODO: In case of executable files, ask the user first!
				entry_ref ref;
				status_t status = get_ref_for_path(fPath.Path(), &ref);
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
				BWebPage::RequestDownload(fURL);
				break;
	
			case CANCEL_DOWNLOAD:
				fDownload->Cancel();
				DownloadCanceled();
				break;
	
			case REMOVE_DOWNLOAD: {
				Window()->PostMessage(SAVE_SETTINGS);
				RemoveSelf();
				delete this;
				// TOAST!
				return;
			}
			case B_DOWNLOAD_REMOVED:
				// TODO: This is a bit asymetric. The removed notification
				// arrives here, but it would be nicer if it arrived
				// at the window...
				Window()->PostMessage(message);
				break;
			default:
				BGridView::MessageReceived(message);
		}
	}

	BWebDownload* Download() const
	{
		return fDownload;
	}

	const BString& URL() const
	{
		return fURL;
	}

	void DownloadFinished()
	{
		fDownload = NULL;
		fTopButton->SetEnabled(true);
		fBottomButton->SetLabel("Remove");
		fBottomButton->SetMessage(new BMessage(REMOVE_DOWNLOAD));
		fBottomButton->SetEnabled(true);
	}

	void DownloadCanceled()
	{
		fDownload = NULL;
		fTopButton->SetLabel("Restart");
		fTopButton->SetMessage(new BMessage(RESTART_DOWNLOAD));
		fTopButton->SetEnabled(true);
		fBottomButton->SetLabel("Remove");
		fBottomButton->SetMessage(new BMessage(REMOVE_DOWNLOAD));
		fBottomButton->SetEnabled(true);
	}

private:
	IconView*		fIconView;
	BStatusBar*		fStatusBar;
	SmallButton*	fTopButton;
	SmallButton*	fBottomButton;
	BWebDownload*	fDownload;
	BString			fURL;
	BPath			fPath;
	off_t			fExpectedSize;
};


class DownloadsContainerView : public BGroupView {
public:
	DownloadsContainerView()
		:
		BGroupView(B_VERTICAL)
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
		:
		BScrollView("Downloads scroll view", target, 0, false, true,
			B_NO_BORDER)
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


// #pragma mark -


DownloadWindow::DownloadWindow(BRect frame, bool visible,
		SettingsMessage* settings)
	: BWindow(frame, "Downloads",
		B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_AUTO_UPDATE_SIZE_LIMITS | B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE)
{
	settings->AddListener(BMessenger(this));
	BPath downloadPath;
	if (find_directory(B_DESKTOP_DIRECTORY, &downloadPath) != B_OK)
		downloadPath.SetTo("/boot/home/Desktop");
	fDownloadPath = settings->GetValue("download path", downloadPath.Path());
	settings->SetValue("download path", fDownloadPath);

	SetLayout(new BGroupLayout(B_VERTICAL));

	DownloadsContainerView* downloadsGroupView = new DownloadsContainerView();
	fDownloadViewsLayout = downloadsGroupView->GroupLayout();

	BMenuBar* menuBar = new BMenuBar("Menu bar");
	BMenu* menu = new BMenu("Downloads");
	menu->AddItem(new BMenuItem("Open downloads folder",
		new BMessage(OPEN_DOWNLOADS_FOLDER)));
	menu->AddItem(new BMenuItem("Hide", new BMessage(B_QUIT_REQUESTED), 'H'));
	menuBar->AddItem(menu);

	BScrollView* scrollView = new DownloadContainerScrollView(
		downloadsGroupView);

	fRemoveFinishedButton = new BButton("Remove finished",
		new BMessage(REMOVE_FINISHED_DOWNLOADS));
	fRemoveFinishedButton->SetEnabled(false);

	AddChild(BGroupLayoutBuilder(B_VERTICAL)
		.Add(menuBar)
		.Add(scrollView)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
		.Add(BGroupLayoutBuilder(B_HORIZONTAL)
			.AddGlue()
			.Add(fRemoveFinishedButton)
			.SetInsets(5, 5, 5, 5)
		)
	);

	PostMessage(INIT);

	if (!visible)
		Hide();
	Show();
}


DownloadWindow::~DownloadWindow()
{
	// Only necessary to save the current progress of unfinished downloads:
	_SaveSettings();
}


void
DownloadWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case INIT:
		{
			_LoadSettings();
			// Small trick to get the correct enabled status of the Remove
			// finished button
			_DownloadFinished(NULL);
			break;
		}
		case B_DOWNLOAD_ADDED:
		{
			BWebDownload* download;
			if (message->FindPointer("download", reinterpret_cast<void**>(
					&download)) == B_OK) {
				_DownloadStarted(download);
			}
			break;
		}
		case B_DOWNLOAD_REMOVED:
		{
			BWebDownload* download;
			if (message->FindPointer("download", reinterpret_cast<void**>(
					&download)) == B_OK) {
				_DownloadFinished(download);
			}
			break;
		}
		case OPEN_DOWNLOADS_FOLDER:
		{
			entry_ref ref;
			status_t status = get_ref_for_path(fDownloadPath.String(), &ref);
			if (status == B_OK)
				status = be_roster->Launch(&ref);
			if (status != B_OK && status != B_ALREADY_RUNNING) {
				BString errorString("The downloads folder could not be "
					"opened.\n\n");
				errorString << "Error: " << strerror(status);
				BAlert* alert = new BAlert("Error opening downloads folder",
					errorString.String(), "OK");
				alert->Go(NULL);
			}
			break;
		}
		case REMOVE_FINISHED_DOWNLOADS:
			_RemoveFinishedDownloads();
			break;
		case SAVE_SETTINGS:
			_SaveSettings();
			break;

		case SETTINGS_VALUE_CHANGED:
		{
			BString string;
			if (message->FindString("name", &string) == B_OK 
				&& string == "download path"
				&& message->FindString("value", &string) == B_OK) {
				fDownloadPath = string;
			}
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
DownloadWindow::QuitRequested()
{
	if (!IsHidden())
		Hide();
	return false;
}


void
DownloadWindow::_DownloadStarted(BWebDownload* download)
{
	download->Start(BPath(fDownloadPath.String()));

	int32 finishedCount = 0;
	int32 index = 0;
	for (int32 i = fDownloadViewsLayout->CountItems() - 1;
			BLayoutItem* item = fDownloadViewsLayout->ItemAt(i); i--) {
		DownloadProgressView* view = dynamic_cast<DownloadProgressView*>(
			item->View());
		if (!view)
			continue;
		if (view->URL() == download->URL()) {
			index = i;
			view->RemoveSelf();
			delete view;
		} else if (!view->Download())
			finishedCount++;
	}
	fRemoveFinishedButton->SetEnabled(finishedCount > 0);
	DownloadProgressView* view = new DownloadProgressView(download);
	if (!view->Init())
		return;
	fDownloadViewsLayout->AddView(index, view);
	_SaveSettings();

	SetWorkspaces(B_CURRENT_WORKSPACE);
	if (IsHidden())
		Show();
}


void
DownloadWindow::_DownloadFinished(BWebDownload* download)
{
	int32 finishedCount = 0;
	for (int32 i = 0;
			BLayoutItem* item = fDownloadViewsLayout->ItemAt(i); i++) {
		DownloadProgressView* view = dynamic_cast<DownloadProgressView*>(
			item->View());
		if (!view)
			continue;
		if (view->Download() == download) {
			view->DownloadFinished();
			finishedCount++;
		} else if (!view->Download())
			finishedCount++;
	}
	fRemoveFinishedButton->SetEnabled(finishedCount > 0);
	if (download)
		_SaveSettings();
}


void
DownloadWindow::_RemoveFinishedDownloads()
{
	for (int32 i = fDownloadViewsLayout->CountItems() - 1;
			BLayoutItem* item = fDownloadViewsLayout->ItemAt(i); i--) {
		DownloadProgressView* view = dynamic_cast<DownloadProgressView*>(
			item->View());
		if (!view)
			continue;
		if (!view->Download()) {
			view->RemoveSelf();
			delete view;
		}
	}
	fRemoveFinishedButton->SetEnabled(false);
	_SaveSettings();
}


void
DownloadWindow::_SaveSettings()
{
	BFile file;
	if (!_OpenSettingsFile(file, B_ERASE_FILE | B_CREATE_FILE | B_WRITE_ONLY))
		return;
	BMessage message;
	for (int32 i = fDownloadViewsLayout->CountItems() - 1;
			BLayoutItem* item = fDownloadViewsLayout->ItemAt(i); i--) {
		DownloadProgressView* view = dynamic_cast<DownloadProgressView*>(
			item->View());
		if (!view)
			continue;
	   BMessage downloadArchive;
	   if (view->SaveSettings(&downloadArchive) == B_OK)
		   message.AddMessage("download", &downloadArchive);
	}
	message.Flatten(&file);
}


void
DownloadWindow::_LoadSettings()
{
	BFile file;
	if (!_OpenSettingsFile(file, B_READ_ONLY))
		return;
	BMessage message;
	if (message.Unflatten(&file) != B_OK)
		return;
	BMessage downloadArchive;
	for (int32 i = 0;
			message.FindMessage("download", i, &downloadArchive) == B_OK;
			i++) {
		DownloadProgressView* view = new DownloadProgressView(
			&downloadArchive);
		if (!view->Init(&downloadArchive))
			continue;
		fDownloadViewsLayout->AddView(0, view);
	}
}


bool
DownloadWindow::_OpenSettingsFile(BFile& file, uint32 mode)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK
		|| path.Append(kApplicationName) != B_OK
		|| path.Append("Downloads") != B_OK) {
		return false;
	}
	return file.SetTo(path.Path(), mode) == B_OK;
}


