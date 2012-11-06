/*
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
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
#include <NetworkCookieJar.h>
#include <Url.h>

#include "CookieJarClientHaiku.h"

#include "Cookie.h"
#include "KURL.h"
#include "NotImplemented.h"
#include "PlatformString.h"

#include <stdio.h>

namespace WebCore {

CookieJarClientHaiku::CookieJarClientHaiku(BNetworkCookieJar* cookieJar)
    : m_cookieJar(cookieJar)
{
	printf("CookieJar construction!\n");
}

String CookieJarClientHaiku::cookies(const Document* doc, const KURL& url)
{
	printf("CookieJar: Request for %s\n", url.string().utf8().data());

	return cookieRequestHeaderFieldValue(doc, url);
}

String CookieJarClientHaiku::cookieRequestHeaderFieldValue(const Document*,
    const KURL& url)
{
	BString result;
	BUrl hUrl(url.string().utf8().data());

	printf("CookieJar: RequestHeaderField for %s\n", hUrl.UrlString().String());

	BNetworkCookie* c;
	for (BNetworkCookieJar::UrlIterator it(m_cookieJar->GetUrlIterator(hUrl));
		(c = it.Next()); ) {
		result << "; " << c->RawCookie(false);
	}
	result.Remove(0, 2);

    return result;
}

void CookieJarClientHaiku::setCookies(Document*, const KURL& url,
    const String& value)
{
	BNetworkCookie* heapCookie
		= new BNetworkCookie(value, BUrl(url.string().utf8().data()));

	printf("CookieJar: Add %s for %s\n", heapCookie->RawCookie(true).String(), url.string().utf8().data());
	printf("  from %s\n", value.utf8().data());
    m_cookieJar->AddCookie(heapCookie);
}

bool CookieJarClientHaiku::cookiesEnabled(const Document*)
{
    return true;
}

bool CookieJarClientHaiku::getRawCookies(const Document*, const KURL& url,
    Vector<Cookie>& rawCookies)
{
	notImplemented();

    rawCookies.clear();
    return false;
}

void CookieJarClientHaiku::deleteCookie(const Document*, const KURL& url,
    const String& name)
{
	notImplemented();
}

} // namespace WebCore

