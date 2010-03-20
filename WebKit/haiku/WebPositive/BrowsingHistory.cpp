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

#include "BrowsingHistory.h"

#include <new>
#include <stdio.h>

#include <Autolock.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Path.h>

#include "BrowserApp.h"


BrowsingHistoryItem::BrowsingHistoryItem(const BString& url)
	:
	fURL(url),
	fDateTime(BDateTime::CurrentDateTime(B_LOCAL_TIME)),
	fInvokationCount(0)
{
}


BrowsingHistoryItem::BrowsingHistoryItem(const BrowsingHistoryItem& other)
{
	*this = other;
}


BrowsingHistoryItem::BrowsingHistoryItem(const BMessage* archive)
{
	if (!archive)
		return;
	BMessage dateTimeArchive;
	if (archive->FindMessage("date time", &dateTimeArchive) == B_OK)
		fDateTime = BDateTime(&dateTimeArchive);
	archive->FindString("url", &fURL);
	archive->FindUInt32("invokations", &fInvokationCount);
}


BrowsingHistoryItem::~BrowsingHistoryItem()
{
}


status_t
BrowsingHistoryItem::Archive(BMessage* archive) const
{
	if (!archive)
		return B_BAD_VALUE;
	BMessage dateTimeArchive;
	status_t status = fDateTime.Archive(&dateTimeArchive);
	if (status == B_OK)
		status = archive->AddMessage("date time", &dateTimeArchive);
	if (status == B_OK)
		status = archive->AddString("url", fURL.String());
	if (status == B_OK)
		status = archive->AddUInt32("invokations", fInvokationCount);
	return status;
}


BrowsingHistoryItem&
BrowsingHistoryItem::operator=(const BrowsingHistoryItem& other)
{
	if (this == &other)
		return *this;

	fURL = other.fURL;
	fDateTime = other.fDateTime;
	fInvokationCount = other.fInvokationCount;

	return *this;
}


bool
BrowsingHistoryItem::operator==(const BrowsingHistoryItem& other) const
{
	if (this == &other)
		return true;

	return fURL == other.fURL && fDateTime == other.fDateTime
		&& fInvokationCount == other.fInvokationCount;
}


bool
BrowsingHistoryItem::operator!=(const BrowsingHistoryItem& other) const
{
	return !(*this == other);
}


bool
BrowsingHistoryItem::operator<(const BrowsingHistoryItem& other) const
{
	if (this == &other)
		return false;

	return fDateTime < other.fDateTime || fURL < other.fURL;
}


bool
BrowsingHistoryItem::operator<=(const BrowsingHistoryItem& other) const
{
	return (*this == other) || (*this < other);
}


bool
BrowsingHistoryItem::operator>(const BrowsingHistoryItem& other) const
{
	if (this == &other)
		return false;

	return fDateTime > other.fDateTime || fURL > other.fURL;
}


bool
BrowsingHistoryItem::operator>=(const BrowsingHistoryItem& other) const
{
	return (*this == other) || (*this > other);
}


void
BrowsingHistoryItem::Invoked()
{
	// Eventually, we may overflow...
	uint32 count = fInvokationCount + 1;
	if (count > fInvokationCount)
		fInvokationCount = count;
	fDateTime = BDateTime::CurrentDateTime(B_LOCAL_TIME);
}


// #pragma mark - BrowsingHistory


BrowsingHistory
BrowsingHistory::sDefaultInstance;


BrowsingHistory::BrowsingHistory()
	:
	BLocker("browsing history"),
	fHistoryItems(64),
	fSettingsLoaded(false)
{
}


BrowsingHistory::~BrowsingHistory()
{
	_SaveSettings();
	_Clear();
}


/*static*/ BrowsingHistory*
BrowsingHistory::DefaultInstance()
{
	if (sDefaultInstance.Lock()) {
		sDefaultInstance._LoadSettings();
		sDefaultInstance.Unlock();
	}
	return &sDefaultInstance;
}


bool
BrowsingHistory::AddItem(const BrowsingHistoryItem& item)
{
	BAutolock _(this);

	return _AddItem(item, false);
}


int32
BrowsingHistory::BrowsingHistory::CountItems() const
{
	BAutolock _(const_cast<BrowsingHistory*>(this));

	return fHistoryItems.CountItems();
}


BrowsingHistoryItem
BrowsingHistory::HistoryItemAt(int32 index) const
{
	BAutolock _(const_cast<BrowsingHistory*>(this));

	BrowsingHistoryItem* existingItem = reinterpret_cast<BrowsingHistoryItem*>(
		fHistoryItems.ItemAt(index));
	if (!existingItem)
		return BrowsingHistoryItem(BString());

	return BrowsingHistoryItem(*existingItem);
}


void
BrowsingHistory::Clear()
{
	BAutolock _(this);
	_Clear();
	_SaveSettings();
}	


// #pragma mark - private


void
BrowsingHistory::_Clear()
{
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		BrowsingHistoryItem* item = reinterpret_cast<BrowsingHistoryItem*>(
			fHistoryItems.ItemAtFast(i));
		delete item;
	}
	fHistoryItems.MakeEmpty();
}


bool
BrowsingHistory::_AddItem(const BrowsingHistoryItem& item, bool internal)
{
	int32 count = CountItems();
	int32 insertionIndex = count;
	for (int32 i = 0; i < count; i++) {
		BrowsingHistoryItem* existingItem
			= reinterpret_cast<BrowsingHistoryItem*>(
			fHistoryItems.ItemAtFast(i));
		if (item.URL() == existingItem->URL()) {
			if (!internal) {
				existingItem->Invoked();
				_SaveSettings();
			}
			return true;
		}
		if (item < *existingItem)
			insertionIndex = i;
	}
	BrowsingHistoryItem* newItem = new(std::nothrow) BrowsingHistoryItem(item);
	if (!newItem || !fHistoryItems.AddItem(newItem, insertionIndex)) {
		delete newItem;
		return false;
	}

	if (!internal) {
		newItem->Invoked();
		_SaveSettings();
	}

	return true;
}


void
BrowsingHistory::_LoadSettings()
{
	if (fSettingsLoaded)
		return;

	fSettingsLoaded = true;

	BFile settingsFile;
	if (_OpenSettingsFile(settingsFile, B_READ_ONLY)) {
		BMessage settingsArchive;
		settingsArchive.Unflatten(&settingsFile);

		BMessage historyItemArchive;
		for (int32 i = 0; settingsArchive.FindMessage("history item", i,
				&historyItemArchive) == B_OK; i++) {
			_AddItem(BrowsingHistoryItem(&historyItemArchive), true);
			historyItemArchive.MakeEmpty();
		}
	}
}


void
BrowsingHistory::_SaveSettings()
{
	BFile settingsFile;
	if (_OpenSettingsFile(settingsFile,
			B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY)) {
		BMessage settingsArchive;
		BMessage historyItemArchive;
		int32 count = CountItems();
		for (int32 i = 0; i < count; i++) {
			BrowsingHistoryItem item = HistoryItemAt(i);
			if (item.Archive(&historyItemArchive) != B_OK)
				break;
			if (settingsArchive.AddMessage("history item",
					&historyItemArchive) != B_OK) {
				break;
			}
			historyItemArchive.MakeEmpty();
		}
		settingsArchive.Flatten(&settingsFile);
	}
}


bool
BrowsingHistory::_OpenSettingsFile(BFile& file, uint32 mode)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK
		|| path.Append(kApplicationName) != B_OK
		|| path.Append("BrowsingHistory") != B_OK) {
		return false;
	}
	return file.SetTo(path.Path(), mode) == B_OK;
}

