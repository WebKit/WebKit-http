/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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

#ifndef ewk_cookie_manager_private_h
#define ewk_cookie_manager_private_h

#include "SoupCookiePersistentStorageType.h"
#include "WKCookieManager.h"
#include "WKRetainPtr.h"
#include "ewk_cookie_manager.h"
#include <WebKit2/WKBase.h>
#include <wtf/PassOwnPtr.h>

struct Cookie_Change_Handler {
    Ewk_Cookie_Manager_Changes_Watch_Cb callback;
    void* userData;

    Cookie_Change_Handler()
        : callback(0)
        , userData(0)
    { }

    Cookie_Change_Handler(Ewk_Cookie_Manager_Changes_Watch_Cb _callback, void* _userData)
        : callback(_callback)
        , userData(_userData)
    { }
};

class EwkCookieManager {
public:
    static PassOwnPtr<EwkCookieManager> create(WKCookieManagerRef cookieManagerRef)
    {
        return adoptPtr(new Ewk_Cookie_Manager(cookieManagerRef));
    }

    ~EwkCookieManager();

    void setPersistentStorage(const String& filename, WebKit::SoupCookiePersistentStorageType storage);

    void getHTTPAcceptPolicy(WKCookieManagerGetHTTPCookieAcceptPolicyFunction callback, void* userData) const;
    void setHTTPAcceptPolicy(WKHTTPCookieAcceptPolicy policy);

    void clearHostnameCookies(const String& hostname);
    void clearAllCookies();

    void getHostNamesWithCookies(WKCookieManagerGetCookieHostnamesFunction callback, void* userData) const;
    void watchChanges(const Cookie_Change_Handler& changeHandler);

private:
    explicit EwkCookieManager(WKCookieManagerRef cookieManagerRef);

    bool isWatchingForChanges() const;

    static void cookiesDidChange(WKCookieManagerRef, const void* clientInfo);

    WKRetainPtr<WKCookieManagerRef> m_wkCookieManager;
    Cookie_Change_Handler m_changeHandler;
};

#endif // ewk_cookie_manager_private_h
