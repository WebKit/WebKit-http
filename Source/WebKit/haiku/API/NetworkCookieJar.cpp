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
#include "NetworkCookieJar.h"

#include "NetworkCookie.h"
#include "PlatformString.h"
#include <Message.h>
#include <wtf/HashMap.h>
#include <wtf/text/StringHash.h>
#include <stdio.h>

using namespace WebCore;

class BNetworkCookieJar::Private {
public:
	typedef HashMap<String, String> URLCookiesMap;
	URLCookiesMap cookies;
};

BNetworkCookieJar::BNetworkCookieJar()
	: fData(new Private())
	, fCookiesEnabled(true)
{
}

BNetworkCookieJar::BNetworkCookieJar(const BMessage& archive)
	: fData(new Private())
{
	BMessage cookiesArchive;
	for (int32 i = 0; archive.FindMessage("cookies", i, &cookiesArchive) == B_OK; i++) {
		BString url;
		if (cookiesArchive.FindString("url", &url) != B_OK)
			continue;
		BString cookies;
		if (cookiesArchive.FindString("cookies", &cookies) != B_OK)
			continue;
		fData->cookies.set(url, cookies);
	}
	if (archive.FindBool("cookies enabled", &fCookiesEnabled) != B_OK)
		fCookiesEnabled = true;
}

BNetworkCookieJar::~BNetworkCookieJar()
{
	delete fData;
}

status_t BNetworkCookieJar::Archive(BMessage* into, bool deep) const
{
	if (!into)
		return B_BAD_VALUE;

	Private::URLCookiesMap::iterator it = fData->cookies.begin();

	while (it != fData->cookies.end()) {
		BMessage cookiesArchive;
		status_t ret = cookiesArchive.AddString("url", it.get()->first);
		if (ret != B_OK)
			return ret;
		ret = cookiesArchive.AddString("cookies", it.get()->second);
		if (ret != B_OK)
			return ret;
		ret = into->AddMessage("cookies", &cookiesArchive);
		if (ret != B_OK)
			return ret;

		++it;
	}

	return into->AddBool("cookies enabled", fCookiesEnabled);
}

void BNetworkCookieJar::SetCookiesFor(const BString& url, const BString& value)
{
	// TODO: This replaces the previous cookie value, which means it's totally broken.
	// What really needs to happen is that there needs to be a list of cookies, and
	// cookies need to be appended.
	fData->cookies.set(url, value);
}

BString BNetworkCookieJar::CookiesFor(const BString& url) const
{
	return fData->cookies.get(url);
}

BString BNetworkCookieJar::CookieRequestHeaderFieldValue(const BString& url) const
{
	// TODO: Return just HTMLOnly cookies here.
	return CookiesFor(url);
}

void BNetworkCookieJar::SetCookiesEnabled(bool enabled)
{
	fCookiesEnabled = enabled;
}

bool BNetworkCookieJar::CookiesEnabled() const
{
	return fCookiesEnabled;
}
