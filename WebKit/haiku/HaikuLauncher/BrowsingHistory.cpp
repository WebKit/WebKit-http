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
#include "BrowsingHistory.h"

#include <Autolock.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Path.h>
#include <stdio.h>


BrowsingHistoryItem::BrowsingHistoryItem(const BString& url)
    : m_url(url)
    , m_dateTime(BDateTime::CurrentDateTime(B_LOCAL_TIME))
    , m_invokationCount(0)
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
	    m_dateTime = BDateTime(&dateTimeArchive);
	archive->FindString("url", &m_url);
	archive->FindUInt32("invokations", &m_invokationCount);
}

BrowsingHistoryItem::~BrowsingHistoryItem()
{
}

status_t BrowsingHistoryItem::archive(BMessage* archive) const
{
	if (!archive)
	    return B_BAD_VALUE;
	BMessage dateTimeArchive;
	status_t status = m_dateTime.Archive(&dateTimeArchive);
	if (status == B_OK)
	    status = archive->AddMessage("date time", &dateTimeArchive);
	if (status == B_OK)
	    status = archive->AddString("url", m_url.String());
	if (status == B_OK)
	    status = archive->AddUInt32("invokations", m_invokationCount);
    return status;
}

BrowsingHistoryItem& BrowsingHistoryItem::operator=(const BrowsingHistoryItem& other)
{
    if (this == &other)
        return *this;

    m_url = other.m_url;
    m_dateTime = other.m_dateTime;
    m_invokationCount = other.m_invokationCount;

    return *this;
}

bool BrowsingHistoryItem::operator==(const BrowsingHistoryItem& other) const
{
	if (this == &other)
	    return true;

	return m_url == other.m_url && m_dateTime == other.m_dateTime
	    && m_invokationCount == other.m_invokationCount;
}

bool BrowsingHistoryItem::operator!=(const BrowsingHistoryItem& other) const
{
	return !(*this == other);
}

bool BrowsingHistoryItem::operator<(const BrowsingHistoryItem& other) const
{
	if (this == &other)
	    return false;

	return m_dateTime < other.m_dateTime || m_url < other.m_url;
}

bool BrowsingHistoryItem::operator<=(const BrowsingHistoryItem& other) const
{
	return (*this == other) || (*this < other);
}

bool BrowsingHistoryItem::operator>(const BrowsingHistoryItem& other) const
{
	if (this == &other)
	    return false;

	return m_dateTime > other.m_dateTime || m_url > other.m_url;
}

bool BrowsingHistoryItem::operator>=(const BrowsingHistoryItem& other) const
{
	return (*this == other) || (*this > other);
}

void BrowsingHistoryItem::increaseInvokationCount()
{
	// Eventually, we may overflow...
	uint32 count = m_invokationCount + 1;
	if (count > m_invokationCount)
	    m_invokationCount = count;
}

// #pragma mark - BrowsingHistory

BrowsingHistory BrowsingHistory::s_defaultInstance;

BrowsingHistory::BrowsingHistory()
    : BLocker("browsing history")
{
	BFile settingsFile;
	if (openSettingsFile(settingsFile, B_READ_ONLY)) {
		BMessage settingsArchive;
		settingsArchive.Unflatten(&settingsFile);
		// TODO: ...
	}
}

BrowsingHistory::~BrowsingHistory()
{
	saveSettings();
}

/*static*/ BrowsingHistory* BrowsingHistory::defaultInstance()
{
	return &s_defaultInstance;
}

// #pragma mark - private

void BrowsingHistory::saveSettings()
{
	BFile settingsFile;
	if (openSettingsFile(settingsFile, B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY)) {
		BMessage settingsArchive;
		// TODO: ...
		settingsArchive.Flatten(&settingsFile);
	}
}

bool BrowsingHistory::openSettingsFile(BFile& file, uint32 mode)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK
		|| path.Append("HaikuLauncher_BrowsingHistory") != B_OK) {
		return false;
	}
	return file.SetTo(path.Path(), mode) == B_OK;
}

