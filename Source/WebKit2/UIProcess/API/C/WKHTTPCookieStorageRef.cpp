/*
 * Copyright (C) 2017 Metrological Group B.V.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WKHTTPCookieStorageRef.h"

#include "APIHTTPCookieStorage.h"
#include "APIWebCookie.h"
#include "WKArray.h"
#include "WKCookie.h"
#include <WebCore/Cookie.h>

WKTypeID WKHTTPCookieStorageGetTypeID()
{
    return WebKit::toAPI(API::HTTPCookieStorage::APIType);
}

void WKHTTPCookieStorageDeleteAllCookies(WKHTTPCookieStorageRef cookieStorage)
{
    WebKit::toImpl(cookieStorage)->deleteAllCookies();
}

void WKHTTPCookieStorageSetCookies(WKHTTPCookieStorageRef cookieStorage, WKArrayRef cookies)
{
    size_t size = cookies ? WKArrayGetSize(cookies) : 0;

    Vector<WebCore::Cookie> passCookies(size);

    for (size_t i = 0; i < size; ++i)
        passCookies[i] = WebKit::toImpl(static_cast<WKCookieRef>(WKArrayGetItemAtIndex(cookies, i)))->cookie();

    WebKit::toImpl(cookieStorage)->setCookies(passCookies);
}

void WKHTTPCookieStorageGetCookies(WKHTTPCookieStorageRef cookieStorage, void* context, WKHTTPCookieStorageGetCookiesFunction callback)
{
    WebKit::toImpl(cookieStorage)->getCookies(WebKit::toGenericCallbackFunction(context, callback));
}

void WKHTTPCookieStorageStartObservingCookieChanges(WKHTTPCookieStorageRef cookieStorage)
{
    WebKit::toImpl(cookieStorage)->startObservingCookieChanges();
}

void WKHTTPCookieStorageStopObservingCookieChanges(WKHTTPCookieStorageRef cookieStorage)
{
    WebKit::toImpl(cookieStorage)->stopObservingCookieChanges();
}
