/*
 * Copyright (C) 2019 Konstantin Tokarev <annulen@yandex.ru>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "QWebPageAdapter.h"
#include "qwebsettings.h"
#include <WebCore/NetworkStorageSession.h>
#include <WebCore/StorageSessionProvider.h>

namespace WebKit {

inline WebCore::ThirdPartyCookiePolicy thirdPartyCookiePolicy(const QWebSettings& settings)
{
    switch (settings.thirdPartyCookiePolicy()) {
    case QWebSettings::AlwaysAllowThirdPartyCookies:
        return WebCore::ThirdPartyCookiePolicy::Allow;
    case QWebSettings::AlwaysBlockThirdPartyCookies:
        return WebCore::ThirdPartyCookiePolicy::Block;
    case QWebSettings::AllowThirdPartyWithExistingCookies:
        return WebCore::ThirdPartyCookiePolicy::AllowWithExistingCookies;
    }
    ASSERT_NOT_REACHED();
    return WebCore::ThirdPartyCookiePolicy::Allow;
}

class QWebPageStorageSessionProvider final : public WebCore::StorageSessionProvider {
public:
    static Ref<QWebPageStorageSessionProvider> create(QWebPageAdapter& page)
    {
        return adoptRef(*new QWebPageStorageSessionProvider(page));
    }
private:
    QWebPageStorageSessionProvider(QWebPageAdapter& page)
        : m_page(page)
        , m_session(PAL::SessionID::defaultSessionID())
    { }

    WebCore::NetworkStorageSession* storageSession() const
    {
        m_session.setCookieJar(m_page.networkAccessManager()->cookieJar());
        m_session.setThirdPartyCookiePolicy(thirdPartyCookiePolicy(*QWebSettings::globalSettings())); // TODO: Support per-page setting?
        return &m_session;
    }

    QWebPageAdapter& m_page;
    mutable WebCore::NetworkStorageSession m_session;
};

}
