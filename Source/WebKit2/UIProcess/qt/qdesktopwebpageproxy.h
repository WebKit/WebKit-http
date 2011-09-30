/*
 * Copyright (C) 2010, 2011 Nokia Corporation and/or its subsidiary(-ies)
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

#ifndef qdesktopwebpageproxy_h
#define qdesktopwebpageproxy_h

#include "QtWebPageProxy.h"
#include "DrawingAreaProxy.h"
#include <wtf/PassOwnPtr.h>

class QDesktopWebViewPrivate;

using namespace WebKit;

class QDesktopWebPageProxy : public QtWebPageProxy
{
public:
    QDesktopWebPageProxy(QDesktopWebViewPrivate*, WKContextRef, WKPageGroupRef = 0);

    virtual bool handleEvent(QEvent*);

protected:
    void paintContent(QPainter* painter, const QRect& area);

private:
    /* PageClient overrides. */
    virtual PassOwnPtr<DrawingAreaProxy> createDrawingAreaProxy();
    virtual void setViewportArguments(const WebCore::ViewportArguments&);
#if ENABLE(TOUCH_EVENTS)
    virtual void doneWithTouchEvent(const NativeWebTouchEvent&, bool wasEventHandled);
#endif

    virtual PassRefPtr<WebKit::WebPopupMenuProxy> createPopupMenuProxy(WebKit::WebPageProxy*);

    virtual void timerEvent(QTimerEvent*);

    bool handleMouseMoveEvent(QMouseEvent*);
    bool handleMousePressEvent(QMouseEvent*);
    bool handleMouseReleaseEvent(QMouseEvent*);
    bool handleMouseDoubleClickEvent(QMouseEvent*);
    bool handleWheelEvent(QWheelEvent*);
    bool handleHoverMoveEvent(QHoverEvent*);
    bool handleDragEnterEvent(QGraphicsSceneDragDropEvent*);
    bool handleDragLeaveEvent(QGraphicsSceneDragDropEvent*);
    bool handleDragMoveEvent(QGraphicsSceneDragDropEvent*);
    bool handleDropEvent(QGraphicsSceneDragDropEvent*);

    QPoint m_tripleClick;
    QBasicTimer m_tripleClickTimer;
};

#endif /* qdesktopwebpageproxy_h */
