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
#include "APIHTTPCookieStorage.h"

#include "WebCookieManagerProxy.h"
#include "WebCoreArgumentCoders.h"
#include "WebPageProxy.h"
#include "WebProcessPool.h"
#include <WebCore/Cookie.h>

namespace API {

HTTPCookieStorage::HTTPCookieStorage(WebKit::WebPageProxy& webPage)
    : m_webPage(webPage)
{
}

HTTPCookieStorage::~HTTPCookieStorage()
{
}

void HTTPCookieStorage::deleteAllCookies()
{
    m_webPage.process().processPool().supplement<WebKit::WebCookieManagerProxy>()->deleteAllCookies(m_webPage.sessionID());
}

void HTTPCookieStorage::startObservingCookieChanges()
{
    m_webPage.process().processPool().supplement<WebKit::WebCookieManagerProxy>()->setCookieObserverCallback(m_webPage.sessionID(), [this] {
        m_webPage.cookiesDidChange();
    });
}

void HTTPCookieStorage::stopObservingCookieChanges()
{
    m_webPage.process().processPool().supplement<WebKit::WebCookieManagerProxy>()->setCookieObserverCallback(m_webPage.sessionID(), nullptr);
}

void HTTPCookieStorage::setCookies(const Vector<WebCore::Cookie>& cookies)
{
    m_webPage.process().processPool().supplement<WebKit::WebCookieManagerProxy>()->setCookies2(m_webPage.sessionID(), cookies);
}

void HTTPCookieStorage::getCookies(Function<void (API::Array*, WebKit::CallbackBase::Error)>&& callbackFunction)
{
    m_webPage.process().processPool().supplement<WebKit::WebCookieManagerProxy>()->getCookies2(m_webPage.sessionID(), WTFMove(callbackFunction));
}

} // namespace API
