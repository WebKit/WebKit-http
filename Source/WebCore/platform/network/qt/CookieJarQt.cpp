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
#include "CookieJarQt.h"

#include "Cookie.h"
#include "URL.h"
#include "NetworkingContext.h"
#include "NotImplemented.h"
#include "PlatformCookieJar.h"
#include "SQLiteStatement.h"
#include "SQLiteTransaction.h"
#include "ThirdPartyCookiesQt.h"
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkCookie>
#include <QStringList>
#include <QVariant>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

static SharedCookieJarQt* s_sharedCookieJarQt = 0;

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
    return true;
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

SharedCookieJarQt* SharedCookieJarQt::shared()
{
    return s_sharedCookieJarQt;
}

SharedCookieJarQt* SharedCookieJarQt::create(const String& cookieStorageDirectory)
{
    if (!s_sharedCookieJarQt)
        s_sharedCookieJarQt = new SharedCookieJarQt(cookieStorageDirectory);

    return s_sharedCookieJarQt;
}

void SharedCookieJarQt::destroy()
{
    delete s_sharedCookieJarQt;
    s_sharedCookieJarQt = 0;
}

void SharedCookieJarQt::getHostnamesWithCookies(HashSet<String>& hostnames)
{
    QList<QNetworkCookie> cookies = allCookies();
    foreach (const QNetworkCookie& networkCookie, cookies)
        hostnames.add(networkCookie.domain());
}

bool SharedCookieJarQt::deleteCookie(const QNetworkCookie& cookie)
{
    if (!QNetworkCookieJar::deleteCookie(cookie))
        return false;

    if (!m_database.isOpen())
        return false;

    SQLiteStatement sqlQuery(m_database, ASCIILiteral("DELETE FROM cookies WHERE cookieId=?"));
    if (sqlQuery.prepare() != SQLITE_OK) {
        qWarning("Failed to prepare delete statement - cannot write to cookie database");
        return false;
    }

    sqlQuery.bindText(1, cookie.domain().append(QLatin1String(cookie.name())));
    int result = sqlQuery.step();
    if (result != SQLITE_DONE) {
        qWarning("Failed to delete cookie from database - %i", result);
        return false;
    }
    return true;
}

void SharedCookieJarQt::deleteCookiesForHostnames(const Vector<WTF::String>& hostNames)
{
    // QTFIXME: Implement as one statement or transaction
    for (auto& hostname : hostNames)
        deleteCookiesForHostname(hostname);
}

void SharedCookieJarQt::deleteCookiesForHostname(const String& hostname)
{
    if (!m_database.isOpen())
        return;

    QList<QNetworkCookie> cookies = allCookies();
    QList<QNetworkCookie>::Iterator it = cookies.begin();
    QList<QNetworkCookie>::Iterator end = cookies.end();
    SQLiteStatement sqlQuery(m_database, ASCIILiteral("DELETE FROM cookies WHERE cookieId=?"));
    if (sqlQuery.prepare() != SQLITE_OK) {
        qWarning("Failed to prepare delete statement - cannot write to cookie database");
        return;
    }

    SQLiteTransaction transaction(m_database);
    transaction.begin();
    while (it != end) {
        if (it->domain() == QString(hostname)) {
            sqlQuery.bindText(1, it->domain().append(QLatin1String(it->name())));
            int result = sqlQuery.step();
            if (result != SQLITE_DONE)
                qWarning("Failed to remove cookie from database - %i", result);
            sqlQuery.reset();
            it = cookies.erase(it);
        } else
            it++;
    }
    transaction.commit();

    setAllCookies(cookies);
}

void SharedCookieJarQt::deleteAllCookies()
{
    if (!m_database.isOpen())
        return;

    if (!m_database.executeCommand(ASCIILiteral("DELETE FROM cookies")))
        qWarning("Failed to clear cookies database");
    setAllCookies(QList<QNetworkCookie>());
}

void SharedCookieJarQt::deleteAllCookiesModifiedSince(std::chrono::system_clock::time_point)
{
    // QTFIXME
    notImplemented();
}

SharedCookieJarQt::SharedCookieJarQt(const String& cookieStorageDirectory)
{
    if (!m_database.open(cookieStorageDirectory + ASCIILiteral("/cookies.db"))) {
        qWarning("Can't open cookie database");
        return;
    }

    m_database.setSynchronous(SQLiteDatabase::SyncOff);
    m_database.executeCommand(ASCIILiteral("PRAGMA secure_delete = 1;"));

    if (ensureDatabaseTable())
        loadCookies();
    else
        m_database.close();
}

SharedCookieJarQt::~SharedCookieJarQt()
{
    m_database.close();
}

bool SharedCookieJarQt::setCookiesFromUrl(const QList<QNetworkCookie>& cookieList, const QUrl& url)
{
    if (!QNetworkCookieJar::setCookiesFromUrl(cookieList, url))
        return false;

    if (!m_database.isOpen())
        return false;

    SQLiteStatement sqlQuery(m_database, ASCIILiteral("INSERT OR REPLACE INTO cookies (cookieId, cookie) VALUES (?, ?)"));
    if (sqlQuery.prepare() != SQLITE_OK)
        return false;

    SQLiteTransaction transaction(m_database);
    transaction.begin();
    foreach (const QNetworkCookie &cookie, cookiesForUrl(url)) {
        if (cookie.isSessionCookie())
            continue;
        sqlQuery.bindText(1, cookie.domain().append(QLatin1String(cookie.name())));
        QByteArray rawCookie = cookie.toRawForm();
        sqlQuery.bindBlob(2, rawCookie.constData(), rawCookie.size());
        int result = sqlQuery.step();
        if (result != SQLITE_DONE)
            qWarning("Failed to insert cookie into database - %i", result);
        sqlQuery.reset();
    }
    transaction.commit();

    return true;
}

bool SharedCookieJarQt::ensureDatabaseTable()
{
    if (!m_database.executeCommand(ASCIILiteral("CREATE TABLE IF NOT EXISTS cookies (cookieId VARCHAR PRIMARY KEY, cookie BLOB);"))) {
        qWarning("Failed to create cookie table");
        return false;
    }
    return true;
}

void SharedCookieJarQt::loadCookies()
{
    if (!m_database.isOpen())
        return;

    QList<QNetworkCookie> cookies;
    SQLiteStatement sqlQuery(m_database, ASCIILiteral("SELECT cookie FROM cookies"));
    if (sqlQuery.prepare() != SQLITE_OK)
        return;

    int result = sqlQuery.step();
    while (result == SQLITE_ROW) {
        Vector<char> blob;
        sqlQuery.getColumnBlobAsVector(0, blob);
        cookies.append(QNetworkCookie::parseCookies(QByteArray::fromRawData(blob.data(), blob.size())));
        result = sqlQuery.step();
    }

    if (result != SQLITE_DONE) {
        LOG_ERROR("Error reading cookies from database");
        return;
    }

    setAllCookies(cookies);
}

#include "moc_CookieJarQt.cpp"

}

// vim: ts=4 sw=4 et
