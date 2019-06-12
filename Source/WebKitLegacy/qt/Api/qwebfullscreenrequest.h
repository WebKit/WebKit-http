/*
 * Copyright (C) 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef QWEBFULLSCREENREQUEST_H
#define QWEBFULLSCREENREQUEST_H

#include <QtWebKit/qwebkitglobal.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qurl.h>

namespace WebCore {
class ChromeClientQt;
}

class QWebElement;
class QWebFullScreenRequestPrivate;
class QWebPageAdapter;

class QWEBKIT_EXPORT QWebFullScreenRequest {
public:
    QWebFullScreenRequest();
    QWebFullScreenRequest(const QWebFullScreenRequest&);
    ~QWebFullScreenRequest();

    void accept();
    void reject();
    bool toggleOn() const;
    QUrl origin() const;
    const QWebElement &element() const;

private:
    friend class WebCore::ChromeClientQt;

    static QWebFullScreenRequest createEnterRequest(QWebPageAdapter* page, const QWebElement& element)
    {
        return QWebFullScreenRequest(page, element, true);
    }

    static QWebFullScreenRequest createExitRequest(QWebPageAdapter* page, const QWebElement& element)
    {
        return QWebFullScreenRequest(page, element, false);
    }

    QWebFullScreenRequest(QWebPageAdapter* page, const QWebElement& element, bool toggleOn);
    QScopedPointer<QWebFullScreenRequestPrivate> d;
};

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(QWebFullScreenRequest, Q_MOVABLE_TYPE);
QT_END_NAMESPACE

Q_DECLARE_METATYPE(QWebFullScreenRequest)

#endif
