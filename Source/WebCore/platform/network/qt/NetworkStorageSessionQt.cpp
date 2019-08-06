/*
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
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
#include "NetworkStorageSession.h"

#include "Cookie.h"
#include "CookieRequestHeaderFieldProxy.h"
#include "NetworkingContext.h"
#include "SharedCookieJarQt.h"
#include "ThirdPartyCookiesQt.h"
#include <QNetworkAccessManager>
#include <QNetworkCookie>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

static void appendCookie(StringBuilder& builder, const QNetworkCookie& cookie)
{
    if (!builder.isEmpty())
        builder.append("; ");
    QByteArray rawData = cookie.toRawForm(QNetworkCookie::NameAndValueOnly);
    builder.append(rawData.constData(), rawData.length());
}

NetworkStorageSession::NetworkStorageSession(PAL::SessionID sessionID, NetworkingContext*)
    : m_sessionID(sessionID)
{
}

NetworkStorageSession::~NetworkStorageSession()
{
}

NetworkingContext* NetworkStorageSession::context() const
{
    return m_context.get();
}

void NetworkStorageSession::setCookiesFromDOM(const URL& firstParty, const SameSiteInfo& sameSiteInfo, const URL& url, Optional<uint64_t> frameID, Optional<PageIdentifier> pageID, const String& value) const
{
    QNetworkCookieJar* jar = context() ? context()->networkAccessManager()->cookieJar() : SharedCookieJarQt::shared();
    if (!jar)
        return;

    QUrl urlForCookies(url);
    QUrl firstPartyUrl(firstParty);
    if (!thirdPartyCookiePolicyPermits(context(), urlForCookies, firstPartyUrl))
        return;

    CString cookieString = value.latin1();
    QList<QNetworkCookie> cookies = QNetworkCookie::parseCookies(QByteArray::fromRawData(cookieString.data(), cookieString.length()));
    QList<QNetworkCookie>::Iterator it = cookies.begin();
    while (it != cookies.end()) {
        if (it->isHttpOnly())
            it = cookies.erase(it);
        else
            ++it;
    }

    jar->setCookiesFromUrl(cookies, urlForCookies);
}

bool NetworkStorageSession::cookiesEnabled() const
{
    return context() ? context()->networkAccessManager()->cookieJar() : SharedCookieJarQt::shared();
}

std::pair<String, bool> NetworkStorageSession::cookiesForDOM(const URL& firstParty, const SameSiteInfo& sameSiteInfo, const URL& url, Optional<uint64_t> frameID, Optional<PageIdentifier> pageID, IncludeSecureCookies includeSecureCookies) const
{
    QNetworkCookieJar* jar = context() ? context()->networkAccessManager()->cookieJar() : SharedCookieJarQt::shared();
    if (!jar)
        return { };

    QUrl urlForCookies(url);
    QUrl firstPartyUrl(firstParty);
    if (!thirdPartyCookiePolicyPermits(context(), urlForCookies, firstPartyUrl))
        return { };

    // QTFIXME: Filter out secure cookies if needed
    QList<QNetworkCookie> cookies = jar->cookiesForUrl(urlForCookies);
    if (cookies.isEmpty())
        return { };

    StringBuilder builder;
    for (const auto& cookie : cookies) {
        if (cookie.isHttpOnly())
            continue;
        appendCookie(builder, cookie);
    }
    return { builder.toString(), false }; // QTFIXME: secure cookies
}

void NetworkStorageSession::setCookies(const Vector<Cookie>&, const URL&, const URL&)
{
    // FIXME: Implement for WebKit to use.
}

void NetworkStorageSession::setCookie(const Cookie&)
{
    // FIXME: Implement for WebKit to use.
}

void NetworkStorageSession::deleteCookie(const Cookie&)
{
    // FIXME: Implement for WebKit to use.
}

void NetworkStorageSession::deleteCookie(const URL& url, const String& cookie) const
{
    //cookieStorage().deleteCookie(*this, url, cookie);
}

void NetworkStorageSession::deleteAllCookies()
{
    ASSERT(!context()); // Not yet implemented for cookie jars other than the shared one.
    SharedCookieJarQt* jar = SharedCookieJarQt::shared();
    if (jar)
        jar->deleteAllCookies();
}

void NetworkStorageSession::deleteAllCookiesModifiedSince(WallTime time)
{
    ASSERT(!context()); // Not yet implemented for cookie jars other than the shared one.
    SharedCookieJarQt* jar = SharedCookieJarQt::shared();
    if (jar)
        jar->deleteAllCookiesModifiedSince(time);
}

void NetworkStorageSession::deleteCookiesForHostnames(const Vector<String>& hostnames, IncludeHttpOnlyCookies includeHttpOnlyCookies)
{
    // QTFIXME: Not yet implemented.
    UNUSED_PARAM(includeHttpOnlyCookies);
    deleteCookiesForHostnames(hostnames);
}

void NetworkStorageSession::deleteCookiesForHostnames(const Vector<String>& hostNames)
{
    ASSERT(!context()); // Not yet implemented for cookie jars other than the shared one.
    SharedCookieJarQt* jar = SharedCookieJarQt::shared();
    if (jar)
        jar->deleteCookiesForHostnames(hostNames);
}

Vector<Cookie> NetworkStorageSession::getAllCookies()
{
    // FIXME: Implement for WebKit to use.
    return { };
}

void NetworkStorageSession::getHostnamesWithCookies(HashSet<String>& hostnames)
{
    ASSERT(!context()); // Not yet implemented for cookie jars other than the shared one.
    SharedCookieJarQt* jar = SharedCookieJarQt::shared();
    if (jar)
        jar->getHostnamesWithCookies(hostnames);
}

Vector<Cookie> NetworkStorageSession::getCookies(const URL&)
{
    // FIXME: Implement for WebKit to use.
    return { };
}

bool NetworkStorageSession::getRawCookies(const URL& firstParty, const SameSiteInfo& sameSiteInfo, const URL& url, Optional<uint64_t> frameID, Optional<PageIdentifier> pageID, Vector<Cookie>& rawCookies) const
{
    // QTFIXME: Not yet implemented
    rawCookies.clear();
    return false; // return true when implemented
}

void NetworkStorageSession::flushCookieStore()
{
    // FIXME: Implement for WebKit to use.
}

std::pair<String, bool> NetworkStorageSession::cookieRequestHeaderFieldValue(const URL& firstParty, const SameSiteInfo& sameSiteInfo, const URL& url, Optional<uint64_t> frameID, Optional<PageIdentifier> pageID, IncludeSecureCookies includeSecureCookies) const
{
    QNetworkCookieJar* jar = context() ? context()->networkAccessManager()->cookieJar() : SharedCookieJarQt::shared();
    if (!jar)
        return { };

    QList<QNetworkCookie> cookies = jar->cookiesForUrl(QUrl(url));
    if (cookies.isEmpty())
        return { };

    StringBuilder builder;
    for (const auto& cookie : cookies)
        appendCookie(builder, cookie);
    return { builder.toString(), false };
}

std::pair<String, bool> NetworkStorageSession::cookieRequestHeaderFieldValue(const CookieRequestHeaderFieldProxy& headerFieldProxy) const
{
    return cookieRequestHeaderFieldValue(headerFieldProxy.firstParty, headerFieldProxy.sameSiteInfo, headerFieldProxy.url, headerFieldProxy.frameID, headerFieldProxy.pageID, headerFieldProxy.includeSecureCookies);
}

} // namespace WebCore
