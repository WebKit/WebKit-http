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
#include <Button.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <Roster.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <SpaceLayoutItem.h>

#include "BrowserApp.h"
#include "DownloadProgressView.h"
#include "SettingsMessage.h"
#include "WebDownload.h"
#include "WebPage.h"


enum {
	INIT = 'init',
	OPEN_DOWNLOADS_FOLDER = 'odnf',
	REMOVE_FINISHED_DOWNLOADS = 'rmfd',
	REMOVE_MISSING_DOWNLOADS = 'rmmd'
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
		// Set the scroll steps
		if (BView* item = Target()->ChildAt(0)) {
			scrollBar->SetSteps(item->MinSize().height + 1,
				item->MinSize().height + 1);
		}
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

	fRemoveMissingButton = new BButton("Remove missing",
		new BMessage(REMOVE_MISSING_DOWNLOADS));
	fRemoveMissingButton->SetEnabled(false);

	AddChild(BGroupLayoutBuilder(B_VERTICAL)
		.Add(menuBar)
		.Add(scrollView)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
		.Add(BGroupLayoutBuilder(B_HORIZONTAL)
			.AddGlue()
			.Add(fRemoveMissingButton)
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
		case REMOVE_MISSING_DOWNLOADS:
			_RemoveMissingDownloads();
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
	int32 missingCount = 0;
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
			continue;
		}
		if (view->IsFinished())
			finishedCount++;
		if (view->IsMissing())
			missingCount++;
	}
	fRemoveFinishedButton->SetEnabled(finishedCount > 0);
	fRemoveMissingButton->SetEnabled(missingCount > 0);
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
	int32 missingCount = 0;
	for (int32 i = 0;
			BLayoutItem* item = fDownloadViewsLayout->ItemAt(i); i++) {
		DownloadProgressView* view = dynamic_cast<DownloadProgressView*>(
			item->View());
		if (!view)
			continue;
		if (download && view->Download() == download) {
			view->DownloadFinished();
			finishedCount++;
			continue;
		}
		if (view->IsFinished())
			finishedCount++;
		if (view->IsMissing())
			missingCount++;
	}
	fRemoveFinishedButton->SetEnabled(finishedCount > 0);
	fRemoveMissingButton->SetEnabled(missingCount > 0);
	if (download)
		_SaveSettings();
}


void
DownloadWindow::_RemoveFinishedDownloads()
{
	int32 missingCount = 0;
	for (int32 i = fDownloadViewsLayout->CountItems() - 1;
			BLayoutItem* item = fDownloadViewsLayout->ItemAt(i); i--) {
		DownloadProgressView* view = dynamic_cast<DownloadProgressView*>(
			item->View());
		if (!view)
			continue;
		if (view->IsFinished()) {
			view->RemoveSelf();
			delete view;
		} else if (view->IsMissing())
			missingCount++;
	}
	fRemoveFinishedButton->SetEnabled(false);
	fRemoveMissingButton->SetEnabled(missingCount > 0);
	_SaveSettings();
}


void
DownloadWindow::_RemoveMissingDownloads()
{
	int32 finishedCount = 0;
	for (int32 i = fDownloadViewsLayout->CountItems() - 1;
			BLayoutItem* item = fDownloadViewsLayout->ItemAt(i); i--) {
		DownloadProgressView* view = dynamic_cast<DownloadProgressView*>(
			item->View());
		if (!view)
			continue;
		if (view->IsMissing()) {
			view->RemoveSelf();
			delete view;
		} else if (view->IsFinished())
			finishedCount++;
	}
	fRemoveMissingButton->SetEnabled(false);
	fRemoveFinishedButton->SetEnabled(finishedCount > 0);
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


