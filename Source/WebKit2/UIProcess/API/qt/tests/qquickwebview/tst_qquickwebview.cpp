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

#include "../testwindow.h"
#include "../util.h"

#include <QDeclarativeEngine>
#include <QScopedPointer>
#include <QtTest/QtTest>
#include <qquickwebpage_p.h>
#include <qquickwebview_p.h>
#include <qwebloadrequest_p.h>

class tst_QQuickWebView : public QObject {
    Q_OBJECT
public:
    tst_QQuickWebView();

private slots:
    void init();
    void cleanup();

    void accessPage();
    void navigationStatusAtStartup();
    void stopEnabledAfterLoadStarted();
    void baseUrl();
    void loadEmptyUrl();
    void loadEmptyPageViewVisible();
    void loadEmptyPageViewHidden();
    void loadNonexistentFileUrl();
    void backAndForward();
    void reload();
    void stop();
    void loadProgress();
    void scrollRequest();

    void show();
    void showWebView();
    void removeFromCanvas();
    void multipleWebViewWindows();
    void multipleWebViews();

private:
    void prepareWebViewComponent();
    inline QQuickWebView* newWebView();
    inline QQuickWebView* webView() const;
    QScopedPointer<TestWindow> m_window;
    QScopedPointer<QDeclarativeComponent> m_component;
};

tst_QQuickWebView::tst_QQuickWebView()
{
    addQtWebProcessToPath();
    prepareWebViewComponent();
}

void tst_QQuickWebView::prepareWebViewComponent()
{
    static QDeclarativeEngine* engine = new QDeclarativeEngine(this);
    engine->addImportPath(QString::fromUtf8(IMPORT_DIR));

    m_component.reset(new QDeclarativeComponent(engine, this));

    m_component->setData(QByteArrayLiteral("import QtQuick 2.0\n"
                                           "import QtWebKit 3.0\n"
                                           "WebView {}")
                         , QUrl());
}

QQuickWebView* tst_QQuickWebView::newWebView()
{
    QObject* viewInstance = m_component->create();

    return qobject_cast<QQuickWebView*>(viewInstance);
}

void tst_QQuickWebView::init()
{
    m_window.reset(new TestWindow(newWebView()));
}

void tst_QQuickWebView::cleanup()
{
    m_window.reset();
}

inline QQuickWebView* tst_QQuickWebView::webView() const
{
    return static_cast<QQuickWebView*>(m_window->webView.data());
}

void tst_QQuickWebView::accessPage()
{
    QQuickWebPage* const pageDirectAccess = webView()->page();

    QVariant pagePropertyValue = webView()->experimental()->property("page");
    QQuickWebPage* const pagePropertyAccess = pagePropertyValue.value<QQuickWebPage*>();
    QCOMPARE(pagePropertyAccess, pageDirectAccess);
}

void tst_QQuickWebView::navigationStatusAtStartup()
{
    QCOMPARE(webView()->canGoBack(), false);

    QCOMPARE(webView()->canGoForward(), false);

    QCOMPARE(webView()->loading(), false);
}

class LoadStartedCatcher : public QObject {
    Q_OBJECT
public:
    LoadStartedCatcher(QQuickWebView* webView)
        : m_webView(webView)
    {
        connect(m_webView, SIGNAL(loadingChanged(QWebLoadRequest*)), this, SLOT(onLoadingChanged(QWebLoadRequest*)));
    }

public slots:
    void onLoadingChanged(QWebLoadRequest* loadRequest)
    {
        if (loadRequest->status() == QQuickWebView::LoadStartedStatus) {
            QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);

            QCOMPARE(m_webView->loading(), true);
        }
    }

signals:
    void finished();

private:
    QQuickWebView* m_webView;
};

void tst_QQuickWebView::stopEnabledAfterLoadStarted()
{
    QCOMPARE(webView()->loading(), false);

    LoadStartedCatcher catcher(webView());
    webView()->setUrl(QUrl::fromLocalFile(QLatin1String(TESTS_SOURCE_DIR "/html/basic_page.html")));
    waitForSignal(&catcher, SIGNAL(finished()));

    QCOMPARE(webView()->loading(), true);

    QVERIFY(waitForLoadSucceeded(webView()));
}

void tst_QQuickWebView::baseUrl()
{
    // Test the url is in a well defined state when instanciating the view, but before loading anything.
    QVERIFY(webView()->url().isEmpty());
}

void tst_QQuickWebView::loadEmptyUrl()
{
    webView()->setUrl(QUrl());
    webView()->setUrl(QUrl(QLatin1String("")));
}

void tst_QQuickWebView::loadEmptyPageViewVisible()
{
    m_window->show();
    loadEmptyPageViewHidden();
}

void tst_QQuickWebView::loadEmptyPageViewHidden()
{
    QSignalSpy loadSpy(webView(), SIGNAL(loadingChanged(QWebLoadRequest*)));

    webView()->setUrl(QUrl::fromLocalFile(QLatin1String(TESTS_SOURCE_DIR "/html/basic_page.html")));
    QVERIFY(waitForLoadSucceeded(webView()));

    QCOMPARE(loadSpy.size(), 2);
}

void tst_QQuickWebView::loadNonexistentFileUrl()
{
    QSignalSpy loadSpy(webView(), SIGNAL(loadingChanged(QWebLoadRequest*)));

    webView()->setUrl(QUrl::fromLocalFile(QLatin1String(TESTS_SOURCE_DIR "/html/file_that_does_not_exist.html")));
    QVERIFY(waitForLoadFailed(webView()));

    QCOMPARE(loadSpy.size(), 2);
}

void tst_QQuickWebView::backAndForward()
{
    webView()->setUrl(QUrl::fromLocalFile(QLatin1String(TESTS_SOURCE_DIR "/html/basic_page.html")));
    QVERIFY(waitForLoadSucceeded(webView()));

    QCOMPARE(webView()->url().path(), QLatin1String(TESTS_SOURCE_DIR "/html/basic_page.html"));

    webView()->setUrl(QUrl::fromLocalFile(QLatin1String(TESTS_SOURCE_DIR "/html/basic_page2.html")));
    QVERIFY(waitForLoadSucceeded(webView()));

    QCOMPARE(webView()->url().path(), QLatin1String(TESTS_SOURCE_DIR "/html/basic_page2.html"));

    webView()->goBack();
    QVERIFY(waitForLoadSucceeded(webView()));

    QCOMPARE(webView()->url().path(), QLatin1String(TESTS_SOURCE_DIR "/html/basic_page.html"));

    webView()->goForward();
    QVERIFY(waitForLoadSucceeded(webView()));

    QCOMPARE(webView()->url().path(), QLatin1String(TESTS_SOURCE_DIR "/html/basic_page2.html"));
}

void tst_QQuickWebView::reload()
{
    webView()->setUrl(QUrl::fromLocalFile(QLatin1String(TESTS_SOURCE_DIR "/html/basic_page.html")));
    QVERIFY(waitForLoadSucceeded(webView()));

    QCOMPARE(webView()->url().path(), QLatin1String(TESTS_SOURCE_DIR "/html/basic_page.html"));

    webView()->reload();
    QVERIFY(waitForLoadSucceeded(webView()));

    QCOMPARE(webView()->url().path(), QLatin1String(TESTS_SOURCE_DIR "/html/basic_page.html"));
}

void tst_QQuickWebView::stop()
{
    webView()->setUrl(QUrl::fromLocalFile(QLatin1String(TESTS_SOURCE_DIR "/html/basic_page.html")));
    QVERIFY(waitForLoadSucceeded(webView()));

    QCOMPARE(webView()->url().path(), QLatin1String(TESTS_SOURCE_DIR "/html/basic_page.html"));

    // FIXME: This test should be fleshed out. Right now it's just here to make sure we don't crash.
    webView()->stop();
}

void tst_QQuickWebView::loadProgress()
{
    QCOMPARE(webView()->loadProgress(), 0);

    webView()->setUrl(QUrl::fromLocalFile(QLatin1String(TESTS_SOURCE_DIR "/html/basic_page.html")));
    QSignalSpy loadProgressChangedSpy(webView(), SIGNAL(loadProgressChanged()));
    QVERIFY(waitForLoadSucceeded(webView()));

    QVERIFY(loadProgressChangedSpy.count() >= 1);

    QCOMPARE(webView()->loadProgress(), 100);
}

void tst_QQuickWebView::show()
{
    // This should not crash.
    m_window->show();
    QTest::qWait(200);
    m_window->hide();
}

void tst_QQuickWebView::showWebView()
{
    webView()->setSize(QSizeF(300, 400));

    webView()->setUrl(QUrl::fromLocalFile(QLatin1String(TESTS_SOURCE_DIR "/html/direct-image-compositing.html")));
    QVERIFY(waitForLoadSucceeded(webView()));

    m_window->show();
    // This should not crash.
    webView()->setVisible(true);
    QTest::qWait(200);
    webView()->setVisible(false);
    QTest::qWait(200);
}

void tst_QQuickWebView::removeFromCanvas()
{
    showWebView();

    // This should not crash.
    QQuickItem* parent = webView()->parentItem();
    QQuickItem noCanvasItem;
    webView()->setParentItem(&noCanvasItem);
    QTest::qWait(200);
    webView()->setParentItem(parent);
    webView()->setVisible(true);
    QTest::qWait(200);
}

void tst_QQuickWebView::multipleWebViewWindows()
{
    showWebView();

    // This should not crash.
    QQuickWebView* webView1 = newWebView();
    QScopedPointer<TestWindow> window1(new TestWindow(webView1));
    QQuickWebView* webView2 = newWebView();
    QScopedPointer<TestWindow> window2(new TestWindow(webView2));

    webView1->setSize(QSizeF(300, 400));
    webView1->setUrl(QUrl::fromLocalFile(QLatin1String(TESTS_SOURCE_DIR "/html/scroll.html")));
    QVERIFY(waitForLoadSucceeded(webView1));
    window1->show();
    webView1->setVisible(true);

    webView2->setSize(QSizeF(300, 400));
    webView2->setUrl(QUrl::fromLocalFile(QLatin1String(TESTS_SOURCE_DIR "/html/basic_page.html")));
    QVERIFY(waitForLoadSucceeded(webView2));
    window2->show();
    webView2->setVisible(true);
    QTest::qWait(200);
}

void tst_QQuickWebView::multipleWebViews()
{
    showWebView();

    // This should not crash.
    QScopedPointer<QQuickWebView> webView1(newWebView());
    webView1->setParentItem(m_window->rootItem());
    QScopedPointer<QQuickWebView> webView2(newWebView());
    webView2->setParentItem(m_window->rootItem());

    webView1->setSize(QSizeF(300, 400));
    webView1->setUrl(QUrl::fromLocalFile(QLatin1String(TESTS_SOURCE_DIR "/html/scroll.html")));
    QVERIFY(waitForLoadSucceeded(webView1.data()));
    webView1->setVisible(true);

    webView2->setSize(QSizeF(300, 400));
    webView2->setUrl(QUrl::fromLocalFile(QLatin1String(TESTS_SOURCE_DIR "/html/basic_page.html")));
    QVERIFY(waitForLoadSucceeded(webView2.data()));
    webView2->setVisible(true);
    QTest::qWait(200);
}

void tst_QQuickWebView::scrollRequest()
{
    webView()->setSize(QSizeF(300, 400));

    webView()->setUrl(QUrl::fromLocalFile(QLatin1String(TESTS_SOURCE_DIR "/html/scroll.html")));
    QVERIFY(waitForLoadSucceeded(webView()));

    // COMPARE with the position requested in the html
    // Use qRound as that is also used when calculating the position
    // in WebKit.
    int y = qRound(50 * webView()->page()->contentsScale());
    QVERIFY(webView()->experimental()->contentY() == y);
}

QTEST_MAIN(tst_QQuickWebView)

#include "tst_qquickwebview.moc"

