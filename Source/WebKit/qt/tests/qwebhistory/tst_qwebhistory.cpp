/*
    Copyright (C) 2008 Holger Hans Peter Freyther

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

#include <QtTest/QtTest>
#include <QAction>

#include "qwebpage.h"
#include "qwebview.h"
#include "qwebframe.h"
#include "qwebhistory.h"
#include "qdebug.h"

class tst_QWebHistory : public QObject
{
    Q_OBJECT

public:
    tst_QWebHistory();
    virtual ~tst_QWebHistory();

protected :
    void loadPage(int nr)
    {
        frame->load(QUrl("qrc:/resources/page" + QString::number(nr) + ".html"));
        waitForLoadFinished.exec();
    }

public Q_SLOTS:
    void init();
    void cleanup();

private Q_SLOTS:
    void title();
    void count();
    void back();
    void forward();
    void itemAt();
    void goToItem();
    void items();
    void serialize_1(); //QWebHistory countity
    void serialize_2(); //QWebHistory index
    void serialize_3(); //QWebHistoryItem
    void saveAndRestore_crash_1();
    void saveAndRestore_crash_2();
    void saveAndRestore_crash_3();
    void popPushState_data();
    void popPushState();
    void clear();


private:
    QWebPage* page;
    QWebFrame* frame;
    QWebHistory* hist;
    QEventLoop waitForLoadFinished;  //operation on history are asynchronous!
    int histsize;
};

tst_QWebHistory::tst_QWebHistory()
{
}

tst_QWebHistory::~tst_QWebHistory()
{
}

void tst_QWebHistory::init()
{
    page = new QWebPage(this);
    frame = page->mainFrame();
    connect(page, SIGNAL(loadFinished(bool)), &waitForLoadFinished, SLOT(quit()), Qt::QueuedConnection);

    for (int i = 1;i < 6;i++) {
        loadPage(i);
    }
    hist = page->history();
    histsize = 5;
}

void tst_QWebHistory::cleanup()
{
    delete page;
}

/**
  * Check QWebHistoryItem::title() method
  */
void tst_QWebHistory::title()
{
    QCOMPARE(hist->currentItem().title(), QString("page5"));
}

/**
  * Check QWebHistory::count() method
  */
void tst_QWebHistory::count()
{
    QCOMPARE(hist->count(), histsize);
}

/**
  * Check QWebHistory::back() method
  */
void tst_QWebHistory::back()
{
    for (int i = histsize;i > 1;i--) {
        QCOMPARE(page->mainFrame()->toPlainText(), QString("page") + QString::number(i));
        hist->back();
        waitForLoadFinished.exec();
    }
    //try one more time (too many). crash test
    hist->back();
    QCOMPARE(page->mainFrame()->toPlainText(), QString("page1"));
}

/**
  * Check QWebHistory::forward() method
  */
void tst_QWebHistory::forward()
{
    //rewind history :-)
    while (hist->canGoBack()) {
        hist->back();
        waitForLoadFinished.exec();
    }

    for (int i = 1;i < histsize;i++) {
        QCOMPARE(page->mainFrame()->toPlainText(), QString("page") + QString::number(i));
        hist->forward();
        waitForLoadFinished.exec();
    }
    //try one more time (too many). crash test
    hist->forward();
    QCOMPARE(page->mainFrame()->toPlainText(), QString("page") + QString::number(histsize));
}

/**
  * Check QWebHistory::itemAt() method
  */
void tst_QWebHistory::itemAt()
{
    for (int i = 1;i < histsize;i++) {
        QCOMPARE(hist->itemAt(i - 1).title(), QString("page") + QString::number(i));
        QVERIFY(hist->itemAt(i - 1).isValid());
    }
    //check out of range values
    QVERIFY(!hist->itemAt(-1).isValid());
    QVERIFY(!hist->itemAt(histsize).isValid());
}

/**
  * Check QWebHistory::goToItem() method
  */
void tst_QWebHistory::goToItem()
{
    QWebHistoryItem current = hist->currentItem();
    hist->back();
    waitForLoadFinished.exec();
    hist->back();
    waitForLoadFinished.exec();
    QVERIFY(hist->currentItem().title() != current.title());
    hist->goToItem(current);
    waitForLoadFinished.exec();
    QCOMPARE(hist->currentItem().title(), current.title());
}

/**
  * Check QWebHistory::items() method
  */
void tst_QWebHistory::items() 
{
    QList<QWebHistoryItem> items = hist->items();
    //check count
    QCOMPARE(histsize, items.count());

    //check order
    for (int i = 1;i <= histsize;i++) {
        QCOMPARE(items.at(i - 1).title(), QString("page") + QString::number(i));
    }
}

/**
  * Check history state after serialization (pickle, persistent..) method
  * Checks history size, history order
  */
void tst_QWebHistory::serialize_1() 
{
    QByteArray tmp;  //buffer
    QDataStream save(&tmp, QIODevice::WriteOnly); //here data will be saved
    QDataStream load(&tmp, QIODevice::ReadOnly); //from here data will be loaded

    save << *hist;
    QVERIFY(save.status() == QDataStream::Ok);
    QCOMPARE(hist->count(), histsize);

    //check size of history
    //load next page to find differences
    loadPage(6);
    QCOMPARE(hist->count(), histsize + 1);
    load >> *hist;
    QVERIFY(load.status() == QDataStream::Ok);
    QCOMPARE(hist->count(), histsize);

    //check order of historyItems
    QList<QWebHistoryItem> items = hist->items();
    for (int i = 1;i <= histsize;i++) {
        QCOMPARE(items.at(i - 1).title(), QString("page") + QString::number(i));
    }
}

/**
  * Check history state after serialization (pickle, persistent..) method
  * Checks history currentIndex value
  */
void tst_QWebHistory::serialize_2() 
{
    QByteArray tmp;  //buffer
    QDataStream save(&tmp, QIODevice::WriteOnly); //here data will be saved
    QDataStream load(&tmp, QIODevice::ReadOnly); //from here data will be loaded

    int oldCurrentIndex = hist->currentItemIndex();

    hist->back();
    waitForLoadFinished.exec();
    hist->back();
    waitForLoadFinished.exec();
    //check if current index was changed (make sure that it is not last item)
    QVERIFY(hist->currentItemIndex() != oldCurrentIndex);
    //save current index
    oldCurrentIndex = hist->currentItemIndex();

    save << *hist;
    QVERIFY(save.status() == QDataStream::Ok);
    load >> *hist;
    QVERIFY(load.status() == QDataStream::Ok);

    //check current index
    QCOMPARE(hist->currentItemIndex(), oldCurrentIndex);
}

/**
  * Check history state after serialization (pickle, persistent..) method
  * Checks QWebHistoryItem public property after serialization
  */
void tst_QWebHistory::serialize_3() 
{
    QByteArray tmp;  //buffer
    QDataStream save(&tmp, QIODevice::WriteOnly); //here data will be saved
    QDataStream load(&tmp, QIODevice::ReadOnly); //from here data will be loaded

    //prepare two different history items
    QWebHistoryItem a = hist->currentItem();
    a.setUserData("A - user data");

    //check properties BEFORE serialization
    QString title(a.title());
    QDateTime lastVisited(a.lastVisited());
    QUrl originalUrl(a.originalUrl());
    QUrl url(a.url());
    QVariant userData(a.userData());

    save << *hist;
    QVERIFY(save.status() == QDataStream::Ok);
    QVERIFY(!load.atEnd());
    hist->clear();
    QVERIFY(hist->count() == 1);
    load >> *hist;
    QVERIFY(load.status() == QDataStream::Ok);
    QWebHistoryItem b = hist->currentItem();

    //check properties AFTER serialization
    QCOMPARE(b.title(), title);
    QCOMPARE(b.lastVisited(), lastVisited);
    QCOMPARE(b.originalUrl(), originalUrl);
    QCOMPARE(b.url(), url);
    QCOMPARE(b.userData(), userData);

    //Check if all data was read
    QVERIFY(load.atEnd());
}

static void saveHistory(QWebHistory* history, QByteArray* in)
{
    in->clear();
    QDataStream save(in, QIODevice::WriteOnly);
    save << *history;
}

static void restoreHistory(QWebHistory* history, QByteArray* out)
{
    QDataStream load(out, QIODevice::ReadOnly);
    load >> *history;
}

/** The test shouldn't crash */
void tst_QWebHistory::saveAndRestore_crash_1()
{
    QByteArray buffer;
    saveHistory(hist, &buffer);
    for (unsigned i = 0; i < 5; i++) {
        restoreHistory(hist, &buffer);
        saveHistory(hist, &buffer);
    }
}

/** The test shouldn't crash */
void tst_QWebHistory::saveAndRestore_crash_2()
{
    QByteArray buffer;
    saveHistory(hist, &buffer);
    QWebPage* page2 = new QWebPage(this);
    QWebHistory* hist2 = page2->history();
    for (unsigned i = 0; i < 5; i++) {
        restoreHistory(hist2, &buffer);
        saveHistory(hist2, &buffer);
    }
    delete page2;
}

/** The test shouldn't crash */
void tst_QWebHistory::saveAndRestore_crash_3()
{
    QByteArray buffer;
    saveHistory(hist, &buffer);
    QWebPage* page2 = new QWebPage(this);
    QWebHistory* hist1 = hist;
    QWebHistory* hist2 = page2->history();
    for (unsigned i = 0; i < 5; i++) {
        restoreHistory(hist1, &buffer);
        restoreHistory(hist2, &buffer);
        QVERIFY(hist1->count() == hist2->count());
        QVERIFY(hist1->count() == histsize);
        hist2->back();
        saveHistory(hist2, &buffer);
        hist2->clear();
    }
    delete page2;
}

void tst_QWebHistory::popPushState_data()
{
    QTest::addColumn<QString>("script");
    QTest::newRow("pushState") << "history.pushState(123, \"foo\");";
    QTest::newRow("replaceState") << "history.replaceState(\"a\", \"b\");";
    QTest::newRow("back") << "history.back();";
    QTest::newRow("forward") << "history.forward();";
    QTest::newRow("clearState") << "history.clearState();";
}

/** Crash test, WebKit bug 38840 (https://bugs.webkit.org/show_bug.cgi?id=38840) */
void tst_QWebHistory::popPushState()
{
    QFETCH(QString, script);
    QWebPage page;
    page.mainFrame()->setHtml("<html><body>long live Qt!</body></html>");
    page.mainFrame()->evaluateJavaScript(script);
}

/** ::clear */
void tst_QWebHistory::clear()
{
    QByteArray buffer;

    QAction* actionBack = page->action(QWebPage::Back);
    QVERIFY(actionBack->isEnabled());
    saveHistory(hist, &buffer);
    QVERIFY(hist->count() > 1);
    hist->clear();
    QVERIFY(hist->count() == 1);  // Leave current item.
    QVERIFY(!actionBack->isEnabled());

    QWebPage* page2 = new QWebPage(this);
    QWebHistory* hist2 = page2->history();
    QVERIFY(hist2->count() == 0);
    hist2->clear();
    QVERIFY(hist2->count() == 0); // Do not change anything.
    delete page2;
}

QTEST_MAIN(tst_QWebHistory)
#include "tst_qwebhistory.moc"
