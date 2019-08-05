/*
 * Copyright (C) 2006 George Staikos <staikos@kde.org>
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * All rights reserved.
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

#include "Cookie.h"
#include "NetworkingContext.h"
#include "NetworkStorageSession.h"
#include "SharedCookieJarQt.h"
#include "ThirdPartyCookiesQt.h"
#include <QNetworkAccessManager>
#include <QNetworkCookie>
#include <wtf/URL.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

static void appendCookie(StringBuilder& builder, const QNetworkCookie& cookie)
{
    if (!builder.isEmpty())
        builder.append("; ");
    QByteArray rawData = cookie.toRawForm(QNetworkCookie::NameAndValueOnly);
    builder.append(rawData.constData(), rawData.length());
}

void setCookiesFromDOM(const NetworkStorageSession& session, const URL& firstParty, const URL& url, const String& value)
{
    QNetworkCookieJar* jar = session.context() ? session.context()->networkAccessManager()->cookieJar() : SharedCookieJarQt::shared();
    if (!jar)
        return;

    QUrl urlForCookies(url);
    QUrl firstPartyUrl(firstParty);
    if (!thirdPartyCookiePolicyPermits(session.context(), urlForCookies, firstPartyUrl))
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

String cookiesForDOM(const NetworkStorageSession& session, const URL& firstParty, const URL& url)
{
    QNetworkCookieJar* jar = session.context() ? session.context()->networkAccessManager()->cookieJar() : SharedCookieJarQt::shared();
    if (!jar)
        return String();

    QUrl urlForCookies(url);
    QUrl firstPartyUrl(firstParty);
    if (!thirdPartyCookiePolicyPermits(session.context(), urlForCookies, firstPartyUrl))
        return String();

    QList<QNetworkCookie> cookies = jar->cookiesForUrl(urlForCookies);
    if (cookies.isEmpty())
        return String();

    StringBuilder builder;
    for (const auto& cookie : cookies) {
        if (cookie.isHttpOnly())
            continue;
        appendCookie(builder, cookie);
    }
    return builder.toString();
}

String cookieRequestHeaderFieldValue(const NetworkStorageSession& session, const URL& /*firstParty*/, const URL& url)
{
    QNetworkCookieJar* jar = session.context() ? session.context()->networkAccessManager()->cookieJar() : SharedCookieJarQt::shared();
    if (!jar)
        return String();

    QList<QNetworkCookie> cookies = jar->cookiesForUrl(QUrl(url));
    if (cookies.isEmpty())
        return String();

    StringBuilder builder;
    for (const auto& cookie : cookies)
        appendCookie(builder, cookie);
    return builder.toString();
}

bool cookiesEnabled(const NetworkStorageSession& session, const URL& /*firstParty*/, const URL& /*url*/)
{
    return session.context() ? session.context()->networkAccessManager()->cookieJar() : SharedCookieJarQt::shared();
}

bool getRawCookies(const NetworkStorageSession& session, const URL& /*firstParty*/, const URL& /*url*/, Vector<Cookie>& rawCookies)
{
    // FIXME: Not yet implemented
    rawCookies.clear();
    return false; // return true when implemented
}

void deleteCookie(const NetworkStorageSession&, const URL&, const String&)
{
    // FIXME: Not yet implemented
}

void getHostnamesWithCookies(const NetworkStorageSession& session, HashSet<String>& hostnames)
{
    ASSERT_UNUSED(session, !session.context()); // Not yet implemented for cookie jars other than the shared one.
    SharedCookieJarQt* jar = SharedCookieJarQt::shared();
    if (jar)
        jar->getHostnamesWithCookies(hostnames);
}

void deleteCookiesForHostnames(const NetworkStorageSession& session, const Vector<String>& hostNames)
{
    ASSERT_UNUSED(session, !session.context()); // Not yet implemented for cookie jars other than the shared one.
    SharedCookieJarQt* jar = SharedCookieJarQt::shared();
    if (jar)
        jar->deleteCookiesForHostnames(hostNames);
}

void deleteAllCookies(const NetworkStorageSession& session)
{
    ASSERT_UNUSED(session, !session.context()); // Not yet implemented for cookie jars other than the shared one.
    SharedCookieJarQt* jar = SharedCookieJarQt::shared();
    if (jar)
        jar->deleteAllCookies();
}

void deleteAllCookiesModifiedSince(const NetworkStorageSession& session, std::chrono::system_clock::time_point time)
{
    ASSERT_UNUSED(session, !session.context()); // Not yet implemented for cookie jars other than the shared one.
    SharedCookieJarQt* jar = SharedCookieJarQt::shared();
    if (jar)
        jar->deleteAllCookiesModifiedSince(time);
}

}

// vim: ts=4 sw=4 et
