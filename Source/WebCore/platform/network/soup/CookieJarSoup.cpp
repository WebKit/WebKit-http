/*
 *  Copyright (C) 2008 Xan Lopez <xan@gnome.org>
 *  Copyright (C) 2009 Igalia S.L.
 *  Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "CookieJarSoup.h"

#include "Cookie.h"
#include "Document.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "GOwnPtrSoup.h"
#include "KURL.h"
#include "NetworkingContext.h"
#include "ResourceHandle.h"
#include <wtf/text/CString.h>

namespace WebCore {

static bool cookiesInitialized;
static SoupCookieJar* cookieJar;

static SoupCookieJar* cookieJarForDocument(const Document* document)
{
    if (!document)
        return 0;
    const Frame* frame = document->frame();
    if (!frame)
        return 0;
    const FrameLoader* loader = frame->loader();
    if (!loader)
        return 0;
    const NetworkingContext* context = loader->networkingContext();
    if (!context)
        return 0;
    return SOUP_COOKIE_JAR(soup_session_get_feature(context->soupSession(), SOUP_TYPE_COOKIE_JAR));
}

SoupCookieJar* defaultCookieJar()
{
    if (!cookiesInitialized) {
        cookiesInitialized = true;

        cookieJar = soup_cookie_jar_new();
        soup_cookie_jar_set_accept_policy(cookieJar, SOUP_COOKIE_JAR_ACCEPT_NO_THIRD_PARTY);
    }

    return cookieJar;
}

void setDefaultCookieJar(SoupCookieJar* jar)
{
    cookiesInitialized = true;

    if (cookieJar)
        g_object_unref(cookieJar);

    cookieJar = jar;

    if (cookieJar)
        g_object_ref(cookieJar);
}

void setCookies(Document* document, const KURL& url, const String& value)
{
    SoupCookieJar* jar = cookieJarForDocument(document);
    if (!jar)
        return;

    GOwnPtr<SoupURI> origin(soup_uri_new(url.string().utf8().data()));

    GOwnPtr<SoupURI> firstParty(soup_uri_new(document->firstPartyForCookies().string().utf8().data()));

    soup_cookie_jar_set_cookie_with_first_party(jar,
                                                origin.get(),
                                                firstParty.get(),
                                                value.utf8().data());
}

String cookies(const Document* document, const KURL& url)
{
    SoupCookieJar* jar = cookieJarForDocument(document);
    if (!jar)
        return String();

    SoupURI* uri = soup_uri_new(url.string().utf8().data());
    char* cookies = soup_cookie_jar_get_cookies(jar, uri, FALSE);
    soup_uri_free(uri);

    String result(String::fromUTF8(cookies));
    g_free(cookies);

    return result;
}

String cookieRequestHeaderFieldValue(const Document* document, const KURL& url)
{
    SoupCookieJar* jar = cookieJarForDocument(document);
    if (!jar)
        return String();

    SoupURI* uri = soup_uri_new(url.string().utf8().data());
    char* cookies = soup_cookie_jar_get_cookies(jar, uri, TRUE);
    soup_uri_free(uri);

    String result(String::fromUTF8(cookies));
    g_free(cookies);

    return result;
}

bool cookiesEnabled(const Document* document)
{
    return !!cookieJarForDocument(document);
}

bool getRawCookies(const Document*, const KURL&, Vector<Cookie>& rawCookies)
{
    // FIXME: Not yet implemented
    rawCookies.clear();
    return false; // return true when implemented
}

void deleteCookie(const Document*, const KURL&, const String&)
{
    // FIXME: Not yet implemented
}

void getHostnamesWithCookies(HashSet<String>& hostnames)
{
    SoupCookieJar* cookieJar = WebCore::defaultCookieJar();
    GSList* cookies = soup_cookie_jar_all_cookies(cookieJar);
    for (GSList* item = cookies; item; item = item->next) {
        SoupCookie* soupCookie = static_cast<SoupCookie*>(item->data);
        if (char* domain = const_cast<char*>(soup_cookie_get_domain(soupCookie)))
            hostnames.add(String::fromUTF8(domain));
    }

    soup_cookies_free(cookies);
}

void deleteCookiesForHostname(const String& hostname)
{
    CString hostNameString = hostname.utf8();

    SoupCookieJar* cookieJar = WebCore::defaultCookieJar();
    GSList* cookies = soup_cookie_jar_all_cookies(cookieJar);
    for (GSList* item = cookies; item; item = item->next) {
        SoupCookie* soupCookie = static_cast<SoupCookie*>(item->data);
        if (hostNameString == soup_cookie_get_domain(soupCookie))
            soup_cookie_jar_delete_cookie(cookieJar, soupCookie);
    }

    soup_cookies_free(cookies);
}

void deleteAllCookies()
{
    SoupCookieJar* cookieJar = WebCore::defaultCookieJar();
    GSList* cookies = soup_cookie_jar_all_cookies(cookieJar);
    for (GSList* item = cookies; item; item = item->next)
        soup_cookie_jar_delete_cookie(cookieJar, static_cast<SoupCookie*>(item->data));

    soup_cookies_free(cookies);
}

}
