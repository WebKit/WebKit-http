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
#include "CookieJar.h"
#include <UrlContext.h>

#include "Cookie.h"
#include "KURL.h"
#include "NotImplemented.h"
#include "PlatformString.h"
#include <wtf/HashMap.h>
#include <wtf/text/CString.h>
#include <stdio.h>

#define TRACE_COOKIE_JAR 0


namespace WebCore {

static BUrlContext gDefaultContext;
static BUrlContext* gNetworkContext = &gDefaultContext;

void setNetworkContext(BUrlContext* context)
{
#if TRACE_COOKIE_JAR
    printf("CookieJar: Set context %p (was %p)\n", context, gNetworkContext);
#endif

    // TODO should delete previous context ?
    if (context == NULL) context = &gDefaultContext;
    gNetworkContext = context;
}

BUrlContext& networkContext()
{
#if TRACE_COOKIE_JAR
    printf("CookieJar: Get context %p\n", gNetworkContext);
#endif
    return *gNetworkContext;
}

void setCookies(Document* document, const KURL& url, const String& value)
{
	BNetworkCookie* heapCookie
		= new BNetworkCookie(value, BUrl(url.string().utf8().data()));

#if TRACE_COOKIE_JAR
	printf("CookieJar: Add %s for %s\n", heapCookie->RawCookie(true).String(),
        url.string().utf8().data());
	printf("  from %s\n", value.utf8().data());
#endif
    gNetworkContext->GetCookieJar().AddCookie(heapCookie);
}

String cookies(const Document* document, const KURL& url)
{
#if TRACE_COOKIE_JAR
	printf("CookieJar: Request for %s\n", url.string().utf8().data());
#endif
	return cookieRequestHeaderFieldValue(document, url);
}

String cookieRequestHeaderFieldValue(const Document* document, const KURL& url)
{
	BString result;
	BUrl hUrl(url.string().utf8().data());

#if TRACE_COOKIE_JAR
	printf("CookieJar: RequestHeaderField for %s\n", hUrl.UrlString().String());
#endif

	BNetworkCookie* c;
	for (BNetworkCookieJar::UrlIterator it(
            gNetworkContext->GetCookieJar().GetUrlIterator(hUrl));
		    (c = it.Next()); ) {
        // filter out httpOnly cookies,as this method is used to get cookies
        // from JS code and these shouldn't be visible there.
        if(!c->HttpOnly())
		    result << "; " << c->RawCookie(false);
	}
	result.Remove(0, 2);

    return result;
}

bool cookiesEnabled(const Document* document)
{
    return true;
}

bool getRawCookies(const Document* document, const KURL& url, Vector<Cookie>& rawCookies)
{
#if TRACE_COOKIE_JAR
	printf("CookieJar: get raw cookies for %s (NOT IMPLEMENTED)\n", url.string().utf8().data());
#endif
	notImplemented();

    rawCookies.clear();
    return false; // return true when implemented
}

void deleteCookie(const Document* document, const KURL& url, const String& name)
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

} // namespace WebCore

