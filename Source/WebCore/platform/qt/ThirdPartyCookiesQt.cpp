/*
 * Copyright (C) 2011 Robert Hogan <robert@roberthogan.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "ThirdPartyCookiesQt.h"

#include "Cookie.h"
#include "CookieJar.h"
#include "Document.h"
#include "qwebframe.h"
#include "qwebpage.h"
#include "qwebsettings.h"

#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkCookieJar>

namespace WebCore {

#if QT_VERSION >= QT_VERSION_CHECK(4, 8, 0)
static QNetworkCookieJar* cookieJar(QObject* originatingFrame)
{
    if (!originatingFrame)
        return 0;
    QWebFrame* frame = qobject_cast<QWebFrame*>(originatingFrame);
    if (!frame)
        return 0;
    QNetworkAccessManager* manager = frame->page()->networkAccessManager();
    QNetworkCookieJar* jar = manager->cookieJar();
    return jar;
}

inline void removeTopLevelDomain(QString* domain, const QString& topLevelDomain)
{
    domain->remove(domain->length() - topLevelDomain.length(), topLevelDomain.length());
    domain->prepend(QLatin1Char('.'));
}

static bool urlsShareSameDomain(const QUrl& url, const QUrl& firstPartyUrl)
{
    QString firstPartyTLD = firstPartyUrl.topLevelDomain();
    QString requestTLD = url.topLevelDomain();

    if (firstPartyTLD != requestTLD)
        return false;

    QString firstPartyDomain = QString(firstPartyUrl.host().toLower());
    QString requestDomain = QString(url.host().toLower());

    removeTopLevelDomain(&firstPartyDomain, firstPartyTLD);
    removeTopLevelDomain(&requestDomain, requestTLD);

    if (firstPartyDomain.section(QLatin1Char('.'), -1) == requestDomain.section(QLatin1Char('.'), -1))
        return true;

    return false;
}
#endif

bool thirdPartyCookiePolicyPermits(QNetworkCookieJar* jar, const QUrl& url, const QUrl& firstPartyUrl)
{
#if QT_VERSION >= QT_VERSION_CHECK(4, 8, 0)
    if (firstPartyUrl.isEmpty())
        return true;

    if (urlsShareSameDomain(url, firstPartyUrl))
        return true;

    switch (QWebSettings::globalSettings()->thirdPartyCookiePolicy()) {
    case QWebSettings::AlwaysAllowThirdPartyCookies:
        return true;
    case QWebSettings::AlwaysBlockThirdPartyCookies:
        return false;
    case QWebSettings::AllowThirdPartyWithExistingCookies: {
        QList<QNetworkCookie> cookies = jar->cookiesForUrl(url);
        return !cookies.isEmpty();
    }
    default:
        return false;
    }
#else
    return true;
#endif
}

bool thirdPartyCookiePolicyPermitsForFrame(QObject* originatingFrame, const QUrl& url, const QUrl& firstPartyUrl)
{
#if QT_VERSION >= QT_VERSION_CHECK(4, 8, 0)
    QNetworkCookieJar* jar = cookieJar(originatingFrame);
    if (!jar)
        return true;
    return thirdPartyCookiePolicyPermits(jar, url, firstPartyUrl);
#else
    return true;
#endif
}

}
// vim: ts=4 sw=4 et
