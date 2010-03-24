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

#include "DownloadProgressView.h"

#include <stdio.h>

#include <Alert.h>
#include <Bitmap.h>
#include <Button.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <GridLayoutBuilder.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <Roster.h>
#include <SpaceLayoutItem.h>
#include <StatusBar.h>

#include "WebDownload.h"
#include "WebPage.h"


enum {
	OPEN_DOWNLOAD = 'opdn',
	RESTART_DOWNLOAD = 'rsdn',
	CANCEL_DOWNLOAD = 'cndn',
	REMOVE_DOWNLOAD = 'rmdn',
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
	
	bool IsIconDimmed() const
	{
		return fDimmedIcon;
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


// #pragma mark - DownloadProgressView


DownloadProgressView::DownloadProgressView(BWebDownload* download)
	:
	BGridView(8, 3),
	fDownload(download),
	fURL(download->URL()),
	fPath(download->Path()),
	fExpectedSize(download->ExpectedSize())
{
}


DownloadProgressView::DownloadProgressView(const BMessage* archive)
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


bool
DownloadProgressView::Init(BMessage* archive)
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


status_t
DownloadProgressView::SaveSettings(BMessage* archive)
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


void
DownloadProgressView::AttachedToWindow()
{
	if (fDownload)
		fDownload->SetProgressListener(BMessenger(this));
	fTopButton->SetTarget(this);
	fBottomButton->SetTarget(this);
	
	BEntry entry(fPath.Path());
	if (entry.Exists())
		_StartNodeMonitor(entry);
}


void
DownloadProgressView::DetachedFromWindow()
{
	_StopNodeMonitor();
}


void
DownloadProgressView::AllAttached()
{
	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(245, 245, 245);
	SetHighColor(tint_color(LowColor(), B_DARKEN_1_TINT));
}


void
DownloadProgressView::Draw(BRect updateRect)
{
	BRect bounds(Bounds());
	bounds.bottom--;
	FillRect(bounds, B_SOLID_LOW);
	bounds.bottom++;
	StrokeLine(bounds.LeftBottom(), bounds.RightBottom());
}


void
DownloadProgressView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_DOWNLOAD_STARTED:
		{
			BString path;
			if (message->FindString("path", &path) != B_OK)
				break;
			fPath.SetTo(path);
			BEntry entry(fPath.Path());
			fIconView->SetTo(entry);
			fStatusBar->Reset(fPath.Leaf());
			_StartNodeMonitor(entry);
			break;
		};
		case B_DOWNLOAD_PROGRESS:
		{
			float progress;
			if (message->FindFloat("progress", &progress) == B_OK)
				fStatusBar->SetTo(progress);
			break;
		}
		case OPEN_DOWNLOAD:
		{
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

		case REMOVE_DOWNLOAD:
		{
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
		case B_NODE_MONITOR:
		{
			int32 opCode;
			if (message->FindInt32("opcode", &opCode) != B_OK)
				break;
			switch (opCode) {
				case B_ENTRY_REMOVED:
					fIconView->SetIconDimmed(true);
					DownloadCanceled();
					break;
				case B_ENTRY_MOVED:
				{
					// Follow the entry to the new location
					dev_t device;
					ino_t directory;
					const char* name;
					if (message->FindInt32("device",
							reinterpret_cast<int32*>(&device)) != B_OK
						|| message->FindInt64("to directory",
							reinterpret_cast<int64*>(&directory)) != B_OK
						|| message->FindString("name", &name) != B_OK
						|| strlen(name) == 0) {
						break;
					}
					// Construct the BEntry and update fPath
					entry_ref ref(device, directory, name);
					BEntry entry(&ref);
					if (entry.GetPath(&fPath) != B_OK)
						break;
					// Find out if the directory is the Trash for this
					// volume
					char trashPath[B_PATH_NAME_LENGTH];
					if (find_directory(B_TRASH_DIRECTORY, device, false,
							trashPath, B_PATH_NAME_LENGTH) == B_OK) {
						BPath trashDirectory(trashPath);
						BPath parentDirectory;
						fPath.GetParent(&parentDirectory);
						if (parentDirectory == trashDirectory) {
							// The entry was moved into the Trash.
							// If the download is still in progress,
							// cancel it.
							if (fDownload)
								fDownload->Cancel();
							fIconView->SetIconDimmed(true);
							DownloadCanceled();
							break;
						} else if (fIconView->IsIconDimmed()) {
							// Maybe it was moved out of the trash.
							fIconView->SetIconDimmed(false);
						}
					}

					fStatusBar->SetText(name);
					Window()->PostMessage(SAVE_SETTINGS);
					break;
				}
				case B_ATTR_CHANGED:
				{
					BEntry entry(fPath.Path());
					fIconView->SetIconDimmed(false);
					fIconView->SetTo(entry);
					break;
				}
			}
			break;
		}
		default:
			BGridView::MessageReceived(message);
	}
}


BWebDownload*
DownloadProgressView::Download() const
{
	return fDownload;
}


const BString&
DownloadProgressView::URL() const
{
	return fURL;
}


bool
DownloadProgressView::IsMissing() const
{
	return fIconView->IsIconDimmed();
}


bool
DownloadProgressView::IsFinished() const
{
	return !fDownload && fStatusBar->CurrentValue() == 100;
}


void
DownloadProgressView::DownloadFinished()
{
	fDownload = NULL;
	fTopButton->SetEnabled(true);
	fBottomButton->SetLabel("Remove");
	fBottomButton->SetMessage(new BMessage(REMOVE_DOWNLOAD));
	fBottomButton->SetEnabled(true);
}


void
DownloadProgressView::DownloadCanceled()
{
	fDownload = NULL;
	fTopButton->SetLabel("Restart");
	fTopButton->SetMessage(new BMessage(RESTART_DOWNLOAD));
	fTopButton->SetEnabled(true);
	fBottomButton->SetLabel("Remove");
	fBottomButton->SetMessage(new BMessage(REMOVE_DOWNLOAD));
	fBottomButton->SetEnabled(true);
}


// #pragma mark - private


void
DownloadProgressView::_StartNodeMonitor(const BEntry& entry)
{
	node_ref nref;
	if (entry.GetNodeRef(&nref) == B_OK)
		watch_node(&nref, B_WATCH_ALL, this);
}


void
DownloadProgressView::_StopNodeMonitor()
{
	stop_watching(this);
}

