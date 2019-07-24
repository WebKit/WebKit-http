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

#include "qwebfullscreenrequest.h"

#include "QWebPageAdapter.h"

#include <QPointer>
#include <WebCore/Element.h>

class QWebFullScreenRequestPrivate {
public:
    QWebFullScreenRequestPrivate(QWebPageAdapter* page, const QWebElement& element, bool toggleOn)
        : element(element)
        , toggleOn(toggleOn)
        , accepted(false)
        , m_handle(page->handle())
        , m_page(page)
    {
    }

    QWebPageAdapter* page() const
    {
        return m_handle.isNull() ? nullptr : m_page;
    }

    QWebElement element;
    const bool toggleOn;
    bool accepted;

private:
    QPointer<QObject> m_handle;
    QWebPageAdapter* m_page;
};

QWebFullScreenRequest::QWebFullScreenRequest()
{
}

QWebFullScreenRequest::QWebFullScreenRequest(QWebPageAdapter* page, const QWebElement& element, bool toggleOn)
    : d(new QWebFullScreenRequestPrivate(page, element, toggleOn))
{
    if (element.isNull())
        d->element = page->fullScreenElement();
}

QWebFullScreenRequest::QWebFullScreenRequest(const QWebFullScreenRequest& other)
    : d(new QWebFullScreenRequestPrivate(*other.d))
{
}

QWebFullScreenRequest::~QWebFullScreenRequest()
{
    if (d->accepted && d->page()) {
        if (d->toggleOn) {
            d->element.endEnterFullScreen();
        } else {
            d->element.endExitFullScreen();
            d->page()->setFullScreenElement(QWebElement());
        }
    }
}

void QWebFullScreenRequest::accept()
{
    if (!d->page()) {
        qWarning("Cannot accept QWebFullScreenRequest: Originating page is already deleted");
        return;
    }

    d->accepted = true;

    if (d->toggleOn) {
        d->page()->setFullScreenElement(d->element);
        d->element.beginEnterFullScreen();
    } else {
        d->element.beginExitFullScreen();
    }
}

void QWebFullScreenRequest::reject()
{
}

bool QWebFullScreenRequest::toggleOn() const
{
    return d->toggleOn;
}

QUrl QWebFullScreenRequest::origin() const
{
    if (!d->element.isNull())
        return d->element.m_element->document().url();

    return QUrl();
}

const QWebElement& QWebFullScreenRequest::element() const
{
    return d->element;
}
