/*
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2009 Girish Ramakrishnan <girish@forwardbias.in>
 * Copyright (C) 2006 George Staikos <staikos@kde.org>
 * Copyright (C) 2006 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Simon Hausmann <hausmann@kde.org>
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

#ifndef webpage_h
#define webpage_h

#include <qwebframe.h>
#include <qwebpage.h>

class WebPage : public QWebPage {
    Q_OBJECT

public:
    WebPage(QObject* parent = 0);

    virtual QWebPage* createWindow(QWebPage::WebWindowType);
    virtual QObject* createPlugin(const QString&, const QUrl&, const QStringList&, const QStringList&);
    virtual bool supportsExtension(QWebPage::Extension extension) const;
    virtual bool extension(Extension extension, const ExtensionOption* option, ExtensionReturn* output);

    virtual bool acceptNavigationRequest(QWebFrame* frame, const QNetworkRequest& request, NavigationType type);

    QString userAgentForUrl(const QUrl& url) const;
    void setInterruptingJavaScriptEnabled(bool enabled) { m_interruptingJavaScriptEnabled = enabled; }

public Q_SLOTS:
    void openUrlInDefaultBrowser(const QUrl& url = QUrl());
    void setUserAgent(const QString& ua) { m_userAgent = ua; }
    bool shouldInterruptJavaScript();
    void authenticationRequired(QNetworkReply*, QAuthenticator*);
    void requestPermission(QWebFrame* frame, QWebPage::Feature feature);
    void featurePermissionRequestCanceled(QWebFrame* frame, QWebPage::Feature feature);

private:
    void applyProxy();
    QString m_userAgent;
    bool m_interruptingJavaScriptEnabled;
};

#endif
