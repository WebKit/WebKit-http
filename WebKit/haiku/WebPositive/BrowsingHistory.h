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

#ifndef BrowsingHistory_h
#define BrowsingHistory_h

#include "DateTime.h"
#include <List.h>
#include <Locker.h>

class BFile;
class BString;

class BrowsingHistoryItem {
public:
    BrowsingHistoryItem(const BString& url);
    BrowsingHistoryItem(const BrowsingHistoryItem& other);
    BrowsingHistoryItem(const BMessage* archive);
    ~BrowsingHistoryItem();

    status_t archive(BMessage* archive) const;

    BrowsingHistoryItem& operator=(const BrowsingHistoryItem& other);

    bool operator==(const BrowsingHistoryItem& other) const;
    bool operator!=(const BrowsingHistoryItem& other) const;
    bool operator<(const BrowsingHistoryItem& other) const;
    bool operator<=(const BrowsingHistoryItem& other) const;
    bool operator>(const BrowsingHistoryItem& other) const;
    bool operator>=(const BrowsingHistoryItem& other) const;

    const BString& url() const { return m_url; }
    const BDateTime& dateTime() const { return m_dateTime; }
    uint32 invokationCount() const { return m_invokationCount; }
    void invoked();

private:
    BString m_url;
    BDateTime m_dateTime;
    uint32 m_invokationCount;
};

class BrowsingHistory : public BLocker {
public:
    static BrowsingHistory* defaultInstance();

    bool addItem(const BrowsingHistoryItem& item);

    // Should Lock() the object when using these in some loop or so:
    int32 countItems() const;
    BrowsingHistoryItem historyItemAt(int32 index) const;
    void clear();

private:
    BrowsingHistory();
    virtual ~BrowsingHistory();
    void privateClear();
    bool privateAddItem(const BrowsingHistoryItem& item, bool invoke);

    void saveSettings();
	bool openSettingsFile(BFile& file, uint32 mode);

private:
    BList m_historyItems;
};

#endif // BrowsingHistory_h

