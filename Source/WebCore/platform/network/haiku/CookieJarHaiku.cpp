/*
 * Copyright (C) 2006 George Staikos <staikos@kde.org>
 * Copyright (C) 2007 Ryan Leavengood <leavengood@gmail.com>
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

#include "Cookie.h"
#include "CookieStorage.h"

#include "URL.h"
#include "NetworkingContext.h"
#include "NotImplemented.h"
#include "PlatformCookieJar.h"

#include <UrlContext.h>
#include <stdio.h>

#include <wtf/HashMap.h>
#include <wtf/text/CString.h>

#define TRACE_COOKIE_JAR 0


namespace WebCore {

void setCookiesFromDOM(const NetworkStorageSession& session, const URL&, const URL& url, const String& value)
{
	BNetworkCookie* heapCookie
		= new BNetworkCookie(value, BUrl(url));

#if TRACE_COOKIE_JAR
	printf("CookieJar: Add %s for %s\n", heapCookie->RawCookie(true).String(),
        url.string().utf8().data());
	printf("  from %s\n", value.utf8().data());
#endif
    session.platformSession().GetCookieJar().AddCookie(heapCookie);
}

String cookiesForDOM(const NetworkStorageSession& session, const URL& firstParty,
    const URL& url)
{
#if TRACE_COOKIE_JAR
	printf("CookieJar: Request for %s\n", url.string().utf8().data());
#endif

	BString result;
	BUrl hUrl(url);

	const BNetworkCookie* c;
	for (BNetworkCookieJar::UrlIterator it(
            session.platformSession().GetCookieJar().GetUrlIterator(hUrl));
		    (c = it.Next()); ) {
        // filter out httpOnly cookies,as this method is used to get cookies
        // from JS code and these shouldn't be visible there.
        if(!c->HttpOnly())
		    result << "; " << c->RawCookie(false);
	}
	result.Remove(0, 2);

    return result;
}

String cookieRequestHeaderFieldValue(const NetworkStorageSession& session, const URL&, const URL& url)
{
#if TRACE_COOKIE_JAR
	printf("CookieJar: RequestHeaderField for %s\n", url.string().utf8().data());
#endif

	BString result;
	BUrl hUrl(url);

	const BNetworkCookie* c;
	for (BNetworkCookieJar::UrlIterator it(
            session.platformSession().GetCookieJar().GetUrlIterator(hUrl));
		    (c = it.Next()); ) {
		result << "; " << c->RawCookie(false);
	}
	result.Remove(0, 2);

    return result;
}

bool cookiesEnabled(const NetworkStorageSession&, const URL&, const URL&)
{
    return true;
}

bool getRawCookies(const NetworkStorageSession&, const URL&, const URL& url, Vector<Cookie>& rawCookies)
{
#if TRACE_COOKIE_JAR
	printf("CookieJar: get raw cookies for %s (NOT IMPLEMENTED)\n", url.string().utf8().data());
#endif
	notImplemented();

    rawCookies.clear();
    return false; // return true when implemented
}

void deleteCookie(const NetworkStorageSession&, const URL& url, const String& name)
{
#if TRACE_COOKIE_JAR
	printf("CookieJar: delete cookie for %s (NOT IMPLEMENTED)\n", url.string().utf8().data());
#endif
	notImplemented();
}

void setCookieStoragePrivateBrowsingEnabled(bool)
{
#if TRACE_COOKIE_JAR
	printf("CookieJar: private browsing (NOT IMPLEMENTED)\n");
#endif
    notImplemented();
}

void getHostnamesWithCookies(const NetworkStorageSession& session, HashSet<String>& hostnames)
{
    notImplemented();
}

void deleteCookiesForHostname(const NetworkStorageSession& session, const String& hostname)
{
    notImplemented();
}

void deleteAllCookies(const NetworkStorageSession& session)
{
    notImplemented();
}

void deleteAllCookiesModifiedAfterDate(const NetworkStorageSession&, double)
{
    notImplemented();
}

void startObservingCookieChanges(CookieChangeCallbackPtr callback)
{
    notImplemented();
}

void stopObservingCookieChanges()
{
}

} // namespace WebCore

