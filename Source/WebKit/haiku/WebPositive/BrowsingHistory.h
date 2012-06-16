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
#ifndef BROWSING_HISTORY_H
#define BROWSING_HISTORY_H

#include "DateTime.h"
#include <List.h>
#include <Locker.h>

class BFile;
class BString;


class BrowsingHistoryItem {
public:
								BrowsingHistoryItem(const BString& url);
								BrowsingHistoryItem(
									const BrowsingHistoryItem& other);
								BrowsingHistoryItem(const BMessage* archive);
								~BrowsingHistoryItem();

			status_t			Archive(BMessage* archive) const;

			BrowsingHistoryItem& operator=(const BrowsingHistoryItem& other);

			bool				operator==(
									const BrowsingHistoryItem& other) const;
			bool				operator!=(
									const BrowsingHistoryItem& other) const;
			bool				operator<(
									const BrowsingHistoryItem& other) const;
			bool				operator<=(
									const BrowsingHistoryItem& other) const;
			bool				operator>(
									const BrowsingHistoryItem& other) const;
			bool				operator>=(
									const BrowsingHistoryItem& other) const;

			const BString&		URL() const { return fURL; }
			const BDateTime&	DateTime() const { return fDateTime; }
			uint32				InvokationCount() const {
									return fInvokationCount; }
			void				Invoked();

private:
			BString				fURL;
			BDateTime			fDateTime;
			uint32				fInvokationCount;
};


class BrowsingHistory : public BLocker {
public:
	static	BrowsingHistory*	DefaultInstance();

			bool				AddItem(const BrowsingHistoryItem& item);

	// Should Lock() the object when using these in some loop or so:
			int32				CountItems() const;
			BrowsingHistoryItem	HistoryItemAt(int32 index) const;
			void				Clear();

			void				SetMaxHistoryItemAge(int32 days);
			int32				MaxHistoryItemAge() const;

private:
								BrowsingHistory();
	virtual						~BrowsingHistory();

			void				_Clear();
			bool				_AddItem(const BrowsingHistoryItem& item,
									bool invoke);

			void				_LoadSettings();
			void				_SaveSettings();
			bool				_OpenSettingsFile(BFile& file, uint32 mode);

private:
			BList				fHistoryItems;
			int32				fMaxHistoryItemAge;

	static	BrowsingHistory		sDefaultInstance;
			bool				fSettingsLoaded;
};


#endif // BROWSING_HISTORY_H

