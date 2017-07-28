/*
* Copyright (c) 2016, Comcast
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
*  * Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR OR; PROFITS BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
* ANY OF THEORY LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "config.h"
#include "APIWebCookie.h"
#include "WKCookie.h"
#include "WKAPICast.h"
#include "WKString.h"

using namespace WebKit;

WKTypeID WKCookieGetTypeID()
{
    return toAPI(WebCookie::APIType);
}

WKCookieRef WKCookieCreate(WKStringRef name,
                           WKStringRef value,
                           WKStringRef domain,
                           WKStringRef path,
                           double expires,
                           bool httpOnly,
                           bool secure,
                           bool session)
{
    return toAPI(&WebCookie::create(WebCore::Cookie(
                                        toWTFString(name),
                                        toWTFString(value),
                                        toWTFString(domain),
                                        toWTFString(path),
                                        expires,
                                        httpOnly,
                                        secure,
                                        session,
                                        String(), WebCore::URL(), Vector<uint16_t>{ })).leakRef());
}

WKStringRef WKCookieGetName(WKCookieRef cookie)
{
    return WKStringCreateWithUTF8CString(
        toImpl(cookie)->cookie().name.utf8().data());
}

WKStringRef WKCookieGetValue(WKCookieRef cookie)
{
    return WKStringCreateWithUTF8CString(
        toImpl(cookie)->cookie().value.utf8().data());
}

WKStringRef WKCookieGetDomain(WKCookieRef cookie)
{
    return WKStringCreateWithUTF8CString(
        toImpl(cookie)->cookie().domain.utf8().data());
}

WKStringRef WKCookieGetPath(WKCookieRef cookie)
{
    return WKStringCreateWithUTF8CString(
        toImpl(cookie)->cookie().path.utf8().data());
}

double WKCookieGetExpires(WKCookieRef cookie)
{
    return toImpl(cookie)->cookie().expires;
}

bool WKCookieGetHttpOnly(WKCookieRef cookie)
{
    return toImpl(cookie)->cookie().httpOnly;
}

bool WKCookieGetSecure(WKCookieRef cookie)
{
    return toImpl(cookie)->cookie().secure;
}

bool WKCookieGetSession(WKCookieRef cookie)
{
    return toImpl(cookie)->cookie().session;
}
