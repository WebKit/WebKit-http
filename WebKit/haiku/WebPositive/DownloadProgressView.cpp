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
#include <Clipboard.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <GroupLayoutBuilder.h>
#include <MenuItem.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <SpaceLayoutItem.h>
#include <StatusBar.h>
#include <StringView.h>

#include "WebDownload.h"
#include "WebPage.h"
#include "StringForSize.h"


enum {
	OPEN_DOWNLOAD			= 'opdn',
	RESTART_DOWNLOAD		= 'rsdn',
	CANCEL_DOWNLOAD			= 'cndn',
	REMOVE_DOWNLOAD			= 'rmdn',
	COPY_URL_TO_CLIPBOARD	= 'curl',
	OPEN_CONTAINING_FOLDER	= 'opfd',
};

const bigtime_t kMaxUpdateInterval = 100000LL;
const bigtime_t kSpeedReferenceInterval = 500000LL;
const bigtime_t kShowSpeedInterval = 8000000LL;
const bigtime_t kShowEstimatedFinishInterval = 4000000LL;

bigtime_t DownloadProgressView::sLastEstimatedFinishSpeedToggleTime = -1;
bool DownloadProgressView::sShowSpeed = true;


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
	BGroupView(B_HORIZONTAL, 8),
	fDownload(download),
	fURL(download->URL()),
	fPath(download->Path())
{
}


DownloadProgressView::DownloadProgressView(const BMessage* archive)
	:
	BGroupView(B_HORIZONTAL, 8),
	fDownload(NULL),
	fURL(),
	fPath()
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
	fCurrentSize = 0;
	fExpectedSize = 0;
	fLastUpdateTime = 0;
	fBytesPerSecond = 0.0;
	for (size_t i = 0; i < kBytesPerSecondSlots; i++)
		fBytesPerSecondSlot[i] = 0.0;
	fCurrentBytesPerSecondSlot = 0;
	fLastSpeedReferenceSize = 0;
	fEstimatedFinishReferenceSize = 0;

	fProcessStartTime = fLastSpeedReferenceTime = fEstimatedFinishReferenceTime
		= system_time();

	SetViewColor(245, 245, 245);
	SetFlags(Flags() | B_FULL_UPDATE_ON_RESIZE | B_WILL_DRAW);

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

	fInfoView = new BStringView("info view", "");

	BGroupLayout* layout = GroupLayout();
	layout->SetInsets(8, 5, 5, 6);
	layout->AddView(fIconView);
	BView* verticalGroup = BGroupLayoutBuilder(B_VERTICAL, 3)
		.Add(fStatusBar)
		.Add(fInfoView)
	;
	verticalGroup->SetViewColor(ViewColor());
	layout->AddView(verticalGroup);
	verticalGroup = BGroupLayoutBuilder(B_VERTICAL, 3)
		.Add(fTopButton)
		.Add(fBottomButton)
	;
	verticalGroup->SetViewColor(ViewColor());
	layout->AddView(verticalGroup);

	BFont font;
	fInfoView->GetFont(&font);
	float fontSize = font.Size() * 0.8f;
	font.SetSize(max_c(8.0f, fontSize));
	fInfoView->SetFont(&font, B_FONT_SIZE);
	fInfoView->SetHighColor(tint_color(fInfoView->LowColor(),
		B_DARKEN_4_TINT));
	fInfoView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

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
	if (fDownload) {
		fDownload->SetProgressListener(BMessenger(this));
		// Will start node monitor upon receiving the B_DOWNLOAD_STARTED
		// message.
	} else {
		BEntry entry(fPath.Path());
		if (entry.Exists())
			_StartNodeMonitor(entry);
	}

	fTopButton->SetTarget(this);
	fBottomButton->SetTarget(this);
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

			// Immediately switch to speed display whenever a new download
			// starts.
			sShowSpeed = true;
			sLastEstimatedFinishSpeedToggleTime
				= fProcessStartTime = fLastSpeedReferenceTime
				= fEstimatedFinishReferenceTime = system_time();
			break;
		}
		case B_DOWNLOAD_PROGRESS:
		{
			int64 currentSize;
			int64 expectedSize;
			if (message->FindInt64("current size", &currentSize) == B_OK
				&& message->FindInt64("expected size", &expectedSize) == B_OK) {
				_UpdateStatus(currentSize, expectedSize);
			}
			break;
		}
		case B_DOWNLOAD_REMOVED:
			// TODO: This is a bit asymetric. The removed notification
			// arrives here, but it would be nicer if it arrived
			// at the window...
			Window()->PostMessage(message);
			break;
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

					float value = fStatusBar->CurrentValue();
					fStatusBar->Reset(name);
					fStatusBar->SetTo(value);
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

		// Context menu messages
		case COPY_URL_TO_CLIPBOARD:
			if (be_clipboard->Lock()) {
				BMessage* data = be_clipboard->Data();
				if (data != NULL) {
					be_clipboard->Clear();
					data->AddData("text/plain", B_MIME_TYPE, fURL.String(),
						fURL.Length());
				}
				be_clipboard->Commit();
				be_clipboard->Unlock();
			}
			break;
		case OPEN_CONTAINING_FOLDER:
			if (fPath.InitCheck() == B_OK) {
				BPath containingFolder;
				if (fPath.GetParent(&containingFolder) != B_OK)
					break;
				BEntry entry(containingFolder.Path());
				if (!entry.Exists())
					break;
				entry_ref ref;
				if (entry.GetRef(&ref) != B_OK)
					break;
				be_roster->Launch(&ref);

				// Use Tracker scripting and select the download pose
				// in the window.
				// TODO: We should somehow get the window that just openend.
				// Using the name like this is broken when there are multiple
				// windows open with this name. Also Tracker does not scroll
				// to this entry.
				BString windowName = ref.name;
				BString fullWindowName = containingFolder.Path();

				BMessenger trackerMessenger("application/x-vnd.Be-TRAK");
				if (trackerMessenger.IsValid()
					&& get_ref_for_path(fPath.Path(), &ref) == B_OK) {
					// We need to wait a bit until the folder is open.
					// TODO: This is also too fragile... we should be able
					// to wait for the roster message.
					snooze(250000);
					int32 tries = 2;
					while (tries > 0) {
						BMessage selectionCommand(B_SET_PROPERTY);
						selectionCommand.AddSpecifier("Selection");
						selectionCommand.AddSpecifier("Poses");
						selectionCommand.AddSpecifier("Window",
							windowName.String());
						selectionCommand.AddRef("data", &ref);
						BMessage reply;
						trackerMessenger.SendMessage(&selectionCommand, &reply);
						int32 error;
						if (reply.FindInt32("error", &error) != B_OK
							|| error == B_OK) {
							break;
						}
						windowName = fullWindowName;
						tries--;
					}
				}
			}
			break;

		default:
			BGroupView::MessageReceived(message);
	}
}


void
DownloadProgressView::ShowContextMenu(BPoint screenWhere)
{
	screenWhere += BPoint(2, 2);

	BPopUpMenu* contextMenu = new BPopUpMenu("download context");
	BMenuItem* copyURL = new BMenuItem("Copy URL to clipboard",
		new BMessage(COPY_URL_TO_CLIPBOARD));
	copyURL->SetEnabled(fURL.Length() > 0);
	contextMenu->AddItem(copyURL);
	BMenuItem* openFolder = new BMenuItem("Open containing folder",
		new BMessage(OPEN_CONTAINING_FOLDER));
	contextMenu->AddItem(openFolder);

	contextMenu->SetTargetForItems(this);
	contextMenu->Go(screenWhere, true, true, true);
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
	if (fExpectedSize == -1) {
		fStatusBar->SetTo(100.0);
		fExpectedSize = fCurrentSize;
	}
	fTopButton->SetEnabled(true);
	fBottomButton->SetLabel("Remove");
	fBottomButton->SetMessage(new BMessage(REMOVE_DOWNLOAD));
	fBottomButton->SetEnabled(true);
	fInfoView->SetText("");
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
	fInfoView->SetText("");
	fPath.Unset();
}


/*static*/ void
DownloadProgressView::SpeedVersusEstimatedFinishTogglePulse()
{
	bigtime_t now = system_time();
	if (sShowSpeed
		&& sLastEstimatedFinishSpeedToggleTime + kShowSpeedInterval
			<= now) {
		sShowSpeed = false;
		sLastEstimatedFinishSpeedToggleTime = now;
	} else if (!sShowSpeed
		&& sLastEstimatedFinishSpeedToggleTime
			+ kShowEstimatedFinishInterval <= now) {
		sShowSpeed = true;
		sLastEstimatedFinishSpeedToggleTime = now;
	}
}


// #pragma mark - private


void
DownloadProgressView::_UpdateStatus(off_t currentSize, off_t expectedSize)
{
	fCurrentSize = currentSize;
	fExpectedSize = expectedSize;

	fStatusBar->SetTo(100.0 * currentSize / expectedSize);

	bigtime_t currentTime = system_time();
	if ((currentTime - fLastUpdateTime) > kMaxUpdateInterval) {
		fLastUpdateTime = currentTime;

		if (currentTime >= fLastSpeedReferenceTime + kSpeedReferenceInterval) {
			// update current speed every kSpeedReferenceInterval
			fCurrentBytesPerSecondSlot
				= (fCurrentBytesPerSecondSlot + 1) % kBytesPerSecondSlots;
			fBytesPerSecondSlot[fCurrentBytesPerSecondSlot]
				= (double)(currentSize - fLastSpeedReferenceSize)
					* 1000000LL / (currentTime - fLastSpeedReferenceTime);
			fLastSpeedReferenceSize = currentSize;
			fLastSpeedReferenceTime = currentTime;
			fBytesPerSecond = 0.0;
			size_t count = 0;
			for (size_t i = 0; i < kBytesPerSecondSlots; i++) {
				if (fBytesPerSecondSlot[i] != 0.0) {
					fBytesPerSecond += fBytesPerSecondSlot[i];
					count++;
				}
			}
			if (count > 0)
				fBytesPerSecond /= count;
		}
		_UpdateStatusText();
	}
}


void
DownloadProgressView::_UpdateStatusText()
{
	fInfoView->SetText("");
	BString buffer;
	if (sShowSpeed && fBytesPerSecond != 0.0) {
		// Draw speed info
		char sizeBuffer[128];
		buffer = "(";
		// Get strings for current and expected size and remove the unit
		// from the current size string if it's the same as the expected
		// size unit.
		BString currentSize = string_for_size((double)fCurrentSize, sizeBuffer,
			sizeof(sizeBuffer));
		BString expectedSize = string_for_size((double)fExpectedSize, sizeBuffer,
			sizeof(sizeBuffer));
		int currentSizeUnitPos = currentSize.FindLast(' ');
		int expectedSizeUnitPos = expectedSize.FindLast(' ');
		if (currentSizeUnitPos >= 0 && expectedSizeUnitPos >= 0
			&& strcmp(currentSize.String() + currentSizeUnitPos,
				expectedSize.String() + expectedSizeUnitPos) == 0) {
			currentSize.Truncate(currentSizeUnitPos);
		}
		buffer << currentSize;
		buffer << " of ";
		buffer << expectedSize;
		buffer << ", ";
		buffer << string_for_size(fBytesPerSecond, sizeBuffer,
			sizeof(sizeBuffer));
		buffer << "/s)";
		float stringWidth = fInfoView->StringWidth(buffer.String());
		if (stringWidth < fInfoView->Bounds().Width())
			fInfoView->SetText(buffer.String());
		else {
			// complete string too wide, try with shorter version
			buffer << string_for_size(fBytesPerSecond, sizeBuffer,
				sizeof(sizeBuffer));
			buffer << "/s";
			stringWidth = fInfoView->StringWidth(buffer.String());
			if (stringWidth < fInfoView->Bounds().Width())
				fInfoView->SetText(buffer.String());
		}
	} else if (!sShowSpeed && fCurrentSize < fExpectedSize) {
		double totalBytesPerSecond = (double)(fCurrentSize
				- fEstimatedFinishReferenceSize)
			* 1000000LL / (system_time() - fEstimatedFinishReferenceTime);
		double secondsRemaining = (fExpectedSize - fCurrentSize)
			/ totalBytesPerSecond;
		time_t now = (time_t)real_time_clock();
		time_t finishTime = (time_t)(now + secondsRemaining);

		tm _time;
		tm* time = localtime_r(&finishTime, &_time);
		int32 year = time->tm_year + 1900;

		char timeText[32];
		time_t secondsPerDay = 24 * 60 * 60;
		// TODO: Localization of time string...
		if (now < finishTime - secondsPerDay) {
			// process is going to take more than a day!
			sprintf(timeText, "%0*d:%0*d %0*d/%0*d/%ld",
				2, time->tm_hour, 2, time->tm_min,
				2, time->tm_mon + 1, 2, time->tm_mday, year);
		} else {
			sprintf(timeText, "%0*d:%0*d",
				2, time->tm_hour, 2, time->tm_min);
		}

		BString buffer1("Finish: ");
		buffer1 << timeText;
		finishTime -= now;
		time = gmtime(&finishTime);

		BString buffer2;
		if (finishTime > secondsPerDay) {
			int64 days = finishTime / secondsPerDay;
			if (days == 1)
				buffer2 << "Over 1 day";
			else
				buffer2 << "Over " << days << " days";
		} else if (finishTime > 60 * 60) {
			int64 hours = finishTime / (60 * 60);
			if (hours == 1)
				buffer2 << "Over 1 hour";
			else
				buffer2 << "Over " << hours << " hours";
		} else if (finishTime > 60) {
			int64 minutes = finishTime / 60;
			if (minutes == 1)
				buffer2 << "Over 1 minute";
			else
				buffer2 << minutes << " minutes";
		} else {
			if (finishTime == 1)
				buffer2 << "1 second";
			else
				buffer2 << finishTime << " seconds";
		}

		buffer2 << " left";

		buffer = "(";
		buffer << buffer1 << " - " << buffer2 << ")";

		float stringWidth = fInfoView->StringWidth(buffer.String());
		if (stringWidth < fInfoView->Bounds().Width())
			fInfoView->SetText(buffer.String());
		else {
			// complete string too wide, try with shorter version
			buffer = "(";
			buffer << buffer1 << ")";
			stringWidth = fInfoView->StringWidth(buffer.String());
			if (stringWidth < fInfoView->Bounds().Width())
				fInfoView->SetText(buffer.String());
		}
	}
}


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

