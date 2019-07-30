/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef PageClientQt_h
#define PageClientQt_h

#include "qgraphicswebview.h"
#include "qwebframe.h"
#include "qwebframe_p.h"
#include "qwebpage.h"
#include "qwebpage_p.h"
#include <qgraphicsscene.h>
#include <qgraphicsview.h>
#include <qgraphicswidget.h>
#include <qmetaobject.h>
#include <qscrollbar.h>
#include <qstyleoption.h>
#include <qtimer.h>
#include <qwidget.h>
#include <WebCore/QWebPageClient.h>

namespace WebCore {

class PageClientQWidget final : public QWebPageClient {
public:
    PageClientQWidget(QWidget* newView, QWebPage* newPage)
        : view(newView)
        , page(newPage)
    {
        Q_ASSERT(view);
    }
    ~PageClientQWidget() final;

    bool isQWidgetClient() const final { return true; }

    void scroll(int dx, int dy, const QRect&) final;
    void update(const QRect& dirtyRect) final;
    void repaintViewport() final;
    void setInputMethodEnabled(bool) final;
    bool inputMethodEnabled() const final;
    void setInputMethodHints(Qt::InputMethodHints) final;

#ifndef QT_NO_CURSOR
    QCursor cursor() const final;
    void updateCursor(const QCursor&) final;
#endif

    QPalette palette() const final;
    int screenNumber() const final;
    QObject* ownerWidget() const final;
    QRect geometryRelativeToOwnerWidget() const final;
    QPoint mapToOwnerWindow(const QPoint&) const final;

    QObject* pluginParent() const final;

    QStyle* style() const final;

    bool viewResizesToContentsEnabled() const final { return false; }

    QRectF windowRect() const final;

    void setWidgetVisible(Widget*, bool visible) final;
    bool isViewVisible() final;

    QWidget* view;
    QWebPage* page;
};

#if !defined(QT_NO_GRAPHICSVIEW)
// the overlay is here for one reason only: to have the scroll-bars and other
// extra UI elements appear on top of any QGraphicsItems created by CSS compositing layers
class QGraphicsItemOverlay final : public QGraphicsObject {
    public:
    QGraphicsItemOverlay(QGraphicsWidget* view, QWebPage* p)
        : QGraphicsObject(view)
        , q(view)
        , page(p)
    {
        setPos(0, 0);
        setFlag(QGraphicsItem::ItemUsesExtendedStyleOption, true);
        setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    }

    QRectF boundingRect() const final
    {
        return q->boundingRect();
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* options, QWidget*) final
    {
        page->mainFrame()->render(painter, static_cast<QWebFrame::RenderLayer>(QWebFrame::AllLayers&(~QWebFrame::ContentsLayer)), options->exposedRect.toRect());
    }

    void prepareGraphicsItemGeometryChange()
    {
        prepareGeometryChange();
    }

    QGraphicsWidget* q;
    QWebPage* page;
};


class PageClientQGraphicsWidget final : public QWebPageClient {
public:
    PageClientQGraphicsWidget(QGraphicsWebView* newView, QWebPage* newPage)
        : view(newView)
        , page(newPage)
        , viewResizesToContents(false)
        , overlay(0)
    {
        Q_ASSERT(view);
        // the overlay and stays alive for the lifetime of
        // this QGraphicsWebView as the scrollbars are needed when there's no compositing
        view->setFlag(QGraphicsItem::ItemUsesExtendedStyleOption);
    }

    ~PageClientQGraphicsWidget() final;

    bool isQWidgetClient() const final { return false; }

    void scroll(int dx, int dy, const QRect&) final;
    void update(const QRect& dirtyRect) final;
    void repaintViewport() final;
    void setInputMethodEnabled(bool) final;
    bool inputMethodEnabled() const final;
    void setInputMethodHints(Qt::InputMethodHints) final;

#ifndef QT_NO_CURSOR
    QCursor cursor() const final;
    void updateCursor(const QCursor&) final;
#endif

    QPalette palette() const final;
    int screenNumber() const final;
    QObject* ownerWidget() const final;
    QRect geometryRelativeToOwnerWidget() const final;
    QPoint mapToOwnerWindow(const QPoint&) const final;

    QObject* pluginParent() const final;

    QStyle* style() const final;

    bool viewResizesToContentsEnabled() const final { return viewResizesToContents; }

    void setWidgetVisible(Widget*, bool) final;
    bool isViewVisible() final;

#if USE(TILED_BACKING_STORE)
    virtual QRectF graphicsItemVisibleRect() const;
#endif

    bool makeOpenGLContextCurrentIfAvailable() final;
    QOpenGLContext* openGLContextIfAvailable() final;

    QRectF windowRect() const final;

    QGraphicsView* firstGraphicsView() const;

    QGraphicsWebView* view;
    QWebPage* page;
    bool viewResizesToContents;

    // the overlay gets instantiated when the root layer is attached, and get deleted when it's detached
    QGraphicsItemOverlay* overlay;

    // we need to put the root graphics layer behind the overlay (which contains the scrollbar)
    enum { RootGraphicsLayerZValue, OverlayZValue };
};
#endif // QT_NO_GRAPHICSVIEW

}
#endif // PageClientQt
