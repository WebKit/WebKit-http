/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef qwebkittest_p_h
#define qwebkittest_p_h

#include "qwebkitglobal.h"
#include "qquickwebview_p.h"

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSize>
#include <QtCore/QVariant>
#include <QtQml/QtQml>
#include <QtQuick/qquickitem.h>

class QQuickWebViewPrivate;

class QWEBKIT_EXPORT QWebKitTest : public QObject {
    Q_OBJECT

    Q_PROPERTY(QSize contentsSize READ contentsSize NOTIFY contentsSizeChanged)

    Q_PROPERTY(QVariant contentsScale READ contentsScale NOTIFY contentsScaleChanged)

    Q_PROPERTY(QVariant devicePixelRatio READ devicePixelRatio NOTIFY viewportChanged)
    Q_PROPERTY(QVariant initialScale READ initialScale NOTIFY viewportChanged)
    Q_PROPERTY(QVariant isScalable READ isScalable NOTIFY viewportChanged)
    Q_PROPERTY(QVariant maximumScale READ maximumScale NOTIFY viewportChanged)
    Q_PROPERTY(QVariant minimumScale READ minimumScale NOTIFY viewportChanged)
    Q_PROPERTY(QVariant layoutSize READ layoutSize NOTIFY viewportChanged)

signals:
    void contentsSizeChanged();
    void contentsScaleChanged();
    void contentsScaleCommitted();
    void viewportChanged();

public slots:
    bool touchTap(QObject* item, qreal x, qreal y, int delay = -1);
    bool touchDoubleTap(QObject* item, qreal x, qreal y, int delay = -1);
    bool wheelEvent(QObject* item, qreal x, qreal y, int delta, Qt::Orientation orient = Qt::Vertical);

public:
    QWebKitTest(QQuickWebViewPrivate* webviewPrivate, QObject* parent = 0);
    virtual ~QWebKitTest();

    bool sendTouchEvent(QQuickWebView* window, QEvent::Type type, const QList<QTouchEvent::TouchPoint>& points, ulong timestamp);

    QSize contentsSize() const;
    QVariant contentsScale() const;

    QVariant devicePixelRatio() const;
    QVariant initialScale() const;
    QVariant isScalable() const;
    QVariant layoutSize() const;
    QVariant maximumScale() const;
    QVariant minimumScale() const;

private:
    QQuickWebViewPrivate* m_webViewPrivate;
};

#endif // qwebkittest_p
