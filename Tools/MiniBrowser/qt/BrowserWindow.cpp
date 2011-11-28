/*
 * Copyright (C) 2010, 2011 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2010 University of Szeged
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

#include "BrowserWindow.h"

#include "qquickwebpage_p.h"
#include "qquickwebview_p.h"
#include "utils.h"

#include <QDeclarativeEngine>
#include <QDir>

BrowserWindow::BrowserWindow(WindowOptions* options)
{
    setWindowTitle("MiniBrowser");
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    setResizeMode(QQuickView::SizeRootObjectToView);

    // This allows starting MiniBrowser from the build directory without previously defining QML_IMPORT_PATH.
    QDir qmlImportDir = QDir(QCoreApplication::applicationDirPath());
    qmlImportDir.cd("../imports");
    engine()->addImportPath(qmlImportDir.canonicalPath());

    Utils* utils = new Utils(this);
    engine()->rootContext()->setContextProperty("utils", utils);
    engine()->rootContext()->setContextProperty("options", options);
    setSource(QUrl("qrc:/qml/BrowserWindow.qml"));
    connect(rootObject(), SIGNAL(pageTitleChanged(QString)), this, SLOT(setWindowTitle(QString)));
    if (!options->useTouchWebView())
        webView()->experimental()->setUseTraditionalDesktopBehaviour(true);
    if (options->startMaximized())
        setWindowState(Qt::WindowMaximized);
    else
        resize(options->requestedWindowSize());
    show();
}

QQuickWebView* BrowserWindow::webView() const
{
    return rootObject()->property("webview").value<QQuickWebView*>();
}

void BrowserWindow::load(const QString& url)
{
    QUrl completedUrl = Utils::urlFromUserInput(url);
    QMetaObject::invokeMethod(rootObject(), "load", Qt::DirectConnection, Q_ARG(QVariant, completedUrl));
}

BrowserWindow* BrowserWindow::newWindow(const QString& url)
{
    BrowserWindow* window = new BrowserWindow();
    window->load(url);
    return window;
}

void BrowserWindow::screenshot()
{
}

void BrowserWindow::updateUserAgentList()
{
}

BrowserWindow::~BrowserWindow()
{
}
