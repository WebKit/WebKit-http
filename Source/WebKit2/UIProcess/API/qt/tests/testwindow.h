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

#ifndef testwindow_h
#define testwindow_h

#include <QResizeEvent>
#include <QScopedPointer>
#include <QtDeclarative/qsgview.h>
#include <QtDeclarative/qsgitem.h>

// TestWindow: Utility class to ignore QGraphicsView details.
class TestWindow : public QSGView {
public:
    inline TestWindow(QSGItem* webView);
    QScopedPointer<QSGItem> webView;

protected:
    inline void resizeEvent(QResizeEvent*);
};

inline TestWindow::TestWindow(QSGItem* webView)
    : webView(webView)
{
    Q_ASSERT(webView);
    webView->setParentItem(rootItem());
}

inline void TestWindow::resizeEvent(QResizeEvent* event)
{
    QSGCanvas::resizeEvent(event);
    webView->setX(0);
    webView->setY(0);
    webView->setWidth(event->size().width());
    webView->setHeight(event->size().height());
}

#endif /* testwindow_h */
