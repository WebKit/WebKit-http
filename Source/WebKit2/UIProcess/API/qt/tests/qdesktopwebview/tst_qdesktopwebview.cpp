/*
    Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <QAction>
#include <QScopedPointer>
#include <QtTest/QtTest>
#include <qdesktopwebview.h>
#include <qwebnavigationcontroller.h>
#include "../testwindow.h"
#include "../util.h"

class tst_QDesktopWebView : public QObject {
    Q_OBJECT
public:
    tst_QDesktopWebView();

private slots:
    void init();
    void cleanup();

    void navigationActionsStatusAtStartup();
    void stopActionEnabledAfterLoadStarted();

private:
    inline QDesktopWebView* webView() const;
    QScopedPointer<TestWindow> m_window;
};

tst_QDesktopWebView::tst_QDesktopWebView()
{
    addQtWebProcessToPath();
}

void tst_QDesktopWebView::init()
{
    m_window.reset(new TestWindow(new QDesktopWebView()));
}

void tst_QDesktopWebView::cleanup()
{
    m_window.reset();
}

inline QDesktopWebView* tst_QDesktopWebView::webView() const
{
    return static_cast<QDesktopWebView*>(m_window->webView.data());
}

void tst_QDesktopWebView::navigationActionsStatusAtStartup()
{
    QAction* backAction = webView()->navigationController()->backAction();
    QVERIFY(backAction);
    QCOMPARE(backAction->isEnabled(), false);

    QAction* forwardAction = webView()->navigationController()->forwardAction();
    QVERIFY(forwardAction);
    QCOMPARE(forwardAction->isEnabled(), false);

    QAction* stopAction = webView()->navigationController()->stopAction();
    QVERIFY(stopAction);
    QCOMPARE(stopAction->isEnabled(), false);

    QAction* reloadAction = webView()->navigationController()->reloadAction();
    QVERIFY(reloadAction);
    QCOMPARE(reloadAction->isEnabled(), false);
}

class LoadStartedCatcher : public QObject {
    Q_OBJECT
public:
    LoadStartedCatcher(QDesktopWebView* webView)
        : m_webView(webView)
    {
        connect(m_webView, SIGNAL(loadStarted()), this, SLOT(onLoadStarted()));
    }

public slots:
    void onLoadStarted()
    {
        QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);

        QAction* stopAction = m_webView->navigationController()->stopAction();
        QVERIFY(stopAction);
        QCOMPARE(stopAction->isEnabled(), true);
    }

signals:
    void finished();

private:
    QDesktopWebView* m_webView;
};

void tst_QDesktopWebView::stopActionEnabledAfterLoadStarted()
{
    QAction* stopAction = webView()->navigationController()->stopAction();
    QVERIFY(stopAction);
    QCOMPARE(stopAction->isEnabled(), false);

    LoadStartedCatcher catcher(webView());
    webView()->load(QUrl::fromLocalFile(QLatin1String(TESTS_SOURCE_DIR "/html/basic_page.html")));
    waitForSignal(&catcher, SIGNAL(finished()));

    QCOMPARE(stopAction->isEnabled(), true);

    waitForSignal(webView(), SIGNAL(loadSucceeded()));
}

QTEST_MAIN(tst_QDesktopWebView)

#include "tst_qdesktopwebview.moc"

