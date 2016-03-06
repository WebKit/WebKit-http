/*
 * Copyright (C) 2016 Konstantin Tokavev <annulen@yandex.ru>
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

#include "VisitedLinkStoreQt.h"

#include "qwebhistoryinterface.h"

VisitedLinkStoreQt& VisitedLinkStoreQt::singleton()
{
    static VisitedLinkStoreQt& visitedLinkStore = adoptRef(*new VisitedLinkStoreQt).leakRef();
    return visitedLinkStore;
}

void VisitedLinkStoreQt::removeAllVisitedLinks()
{
    m_visitedLinkHashes.clear();
}

bool VisitedLinkStoreQt::isLinkVisited(WebCore::Page&, WebCore::LinkHash linkHash, const WebCore::URL& baseURL, const WTF::AtomicString& attributeURL)
{
    // If the Qt4.4 interface for the history is used, we will have to fallback
    // to the old global history.
    // See https://bugs.webkit.org/show_bug.cgi?id=20952
    QWebHistoryInterface* iface = QWebHistoryInterface::defaultInterface();
    if (iface) {
        Vector<UChar, 512> url;
        visitedURL(baseURL, attributeURL, url);
        return iface->historyContains(QString(reinterpret_cast<QChar*>(url.data()), url.size()));
    }

    return m_visitedLinkHashes.contains(linkHash);
}

void VisitedLinkStoreQt::addVisitedLink(WebCore::Page&, WebCore::LinkHash linkHash)
{
    m_visitedLinkHashes.add(linkHash);
    invalidateStylesForLink(linkHash);
}

VisitedLinkStoreQt::VisitedLinkStoreQt()
{
}
