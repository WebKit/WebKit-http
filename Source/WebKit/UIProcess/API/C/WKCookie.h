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

#ifndef WKCookie_h
#define WKCookie_h

#include <WebKit/WKBase.h>

#ifdef __cplusplus
extern "C" {
#endif

WK_EXPORT WKTypeID WKCookieGetTypeID();

WK_EXPORT WKCookieRef WKCookieCreate(WKStringRef name,
                                     WKStringRef value,
                                     WKStringRef domain,
                                     WKStringRef path,
                                     double expires,
                                     bool httpOnly,
                                     bool secure,
                                     bool session);

WK_EXPORT WKStringRef WKCookieGetName(WKCookieRef cookie);
WK_EXPORT WKStringRef WKCookieGetValue(WKCookieRef cookie);
WK_EXPORT WKStringRef WKCookieGetDomain(WKCookieRef cookie);
WK_EXPORT WKStringRef WKCookieGetPath(WKCookieRef cookie);
WK_EXPORT double WKCookieGetExpires(WKCookieRef cookie);
WK_EXPORT bool WKCookieGetHttpOnly(WKCookieRef cookie);
WK_EXPORT bool WKCookieGetSecure(WKCookieRef cookie);
WK_EXPORT bool WKCookieGetSession(WKCookieRef cookie);

#ifdef __cplusplus
}
#endif

#endif
