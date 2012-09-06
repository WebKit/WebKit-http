/*
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
#include "CookieJar.h"

#include "Cookie.h"
#include "Document.h"
#include "KURL.h"
#include <wtf/HashMap.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

static HashMap<String, String> cookieJar;

void setCookies(Document* /*document*/, const KURL& url, const String& value)
{
    cookieJar.set(url.string(), value);
}

String cookies(const Document* /*document*/, const KURL& url)
{
    return cookieJar.get(url.string());
}

String cookieRequestHeaderFieldValue(const Document* /*document*/, const KURL& url)
{
    // FIXME: include HttpOnly cookie.
    return cookieJar.get(url.string());
}

bool cookiesEnabled(const Document* /*document*/)
{
    return true;
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

#if !PLATFORM(EFL)
void setCookieStoragePrivateBrowsingEnabled(bool enabled)
{
    // FIXME: Not yet implemented
}
#endif

void getHostnamesWithCookies(HashSet<String>& hostnames)
{
    // FIXME: Not yet implemented
}

void deleteCookiesForHostname(const String& hostname)
{
    // FIXME: Not yet implemented
}

void deleteAllCookies()
{
    // FIXME: Not yet implemented
}

}
