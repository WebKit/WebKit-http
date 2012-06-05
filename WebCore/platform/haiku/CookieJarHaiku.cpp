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

#include "Cookie.h"
#include "CString.h"
#include "KURL.h"
#include "PlatformString.h"
#include "StringHash.h"
#include <wtf/HashMap.h>
#include <stdio.h>

#define TRACE_COOKIE_JAR 0


namespace WebCore {

// This temporary cookie jar is used when the client has not provided one.
static HashMap<String, String> cookieJar;

// This global CookieJarClient will be used, if set.
static CookieJarClient* gCookieJarClient;

void setCookieJarClient(CookieJarClient* client)
{
    gCookieJarClient = client;
}

void setCookies(Document* document, const KURL& url, const String& value)
{
#if TRACE_COOKIE_JAR
printf("setCookies(%s): %s\n\n", url.string().utf8().data(), value.utf8().data());
#endif
    if (gCookieJarClient)
        gCookieJarClient->setCookies(document, url, value);
    else
        cookieJar.set(url.string(), value);
}

String cookies(const Document* document, const KURL& url)
{
    if (gCookieJarClient)
#if TRACE_COOKIE_JAR
{
String result = gCookieJarClient->cookies(document, url);
printf("cookies(%s): %s\n\n", url.string().utf8().data(), result.utf8().data());
return result;
}
#else
        return gCookieJarClient->cookies(document, url);
#endif
    return cookieJar.get(url.string());
}

String cookieRequestHeaderFieldValue(const Document* document, const KURL& url)
{
    if (gCookieJarClient)
#if TRACE_COOKIE_JAR
{
String result = gCookieJarClient->cookieRequestHeaderFieldValue(document, url);
printf("cookies(%s): %s\n\n", url.string().utf8().data(), result.utf8().data());
return result;
}
#else
        return gCookieJarClient->cookieRequestHeaderFieldValue(document, url);
#endif
    return cookieJar.get(url.string());
}

bool cookiesEnabled(const Document* document)
{
	if (gCookieJarClient)
	    return gCookieJarClient->cookiesEnabled(document);
    return true;
}

bool getRawCookies(const Document* document, const KURL& url, Vector<Cookie>& rawCookies)
{
	if (gCookieJarClient)
	    return gCookieJarClient->getRawCookies(document, url, rawCookies);
    // FIXME: Not yet implemented
    rawCookies.clear();
    return false; // return true when implemented
}

void deleteCookie(const Document* document, const KURL& url, const String& name)
{
	if (gCookieJarClient)
	    gCookieJarClient->deleteCookie(document, url, name);
	else {
        // FIXME: Not yet implemented
	}
}

} // namespace WebCore

