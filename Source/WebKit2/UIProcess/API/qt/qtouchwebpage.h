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

#ifndef qtouchwebpage_h
#define qtouchwebpage_h

#include "qwebkitglobal.h"

#include <QGraphicsWidget>
#include <QSharedPointer>

class QTouchWebPagePrivate;
class QTouchWebPageProxy;

namespace WebKit {
    class TouchViewInterface;
}

class QWEBKIT_EXPORT QTouchWebPage : public QGraphicsWidget {
    Q_OBJECT
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QUrl url READ url NOTIFY urlChanged)

public:
    QTouchWebPage(QGraphicsItem* parent = 0);

    virtual ~QTouchWebPage();

    void load(const QUrl&);
    Q_INVOKABLE QUrl url() const;

    Q_INVOKABLE QString title() const;

    virtual void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*);
    virtual bool event(QEvent*);

    // FIXME: should not be public
    virtual QRectF visibleRect() const;

Q_SIGNALS:
    void urlChanged(const QUrl&);
    void titleChanged(const QString&);
    void loadStarted();
    void loadSucceeded();
    void loadProgress(int progress);

protected:
    virtual void timerEvent(QTimerEvent*);
    virtual void resizeEvent(QGraphicsSceneResizeEvent*);

    Q_SLOT void focusNextPrevChildCallback(bool next);

    virtual bool focusNextPrevChild(bool next);

private:
    Q_PRIVATE_SLOT(d, void onScaleChanged())

    QTouchWebPagePrivate* d;
    friend class QTouchWebPagePrivate;
    friend class WebKit::TouchViewInterface;
    friend class TiledDrawingAreaProxy;
};

#endif /* qtouchwebpage_h */
