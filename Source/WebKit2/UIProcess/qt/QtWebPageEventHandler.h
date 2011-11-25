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

#ifndef QtWebPageEventHandler_h
#define QtWebPageEventHandler_h

#include "QtPanGestureRecognizer.h"
#include "QtPinchGestureRecognizer.h"
#include "QtTapGestureRecognizer.h"
#include "QtViewportInteractionEngine.h"
#include "WebPageProxy.h"
#include <QBasicTimer>
#include <QKeyEvent>
#include <QTouchEvent>
#include <WKPage.h>

using namespace WebKit;

class QtWebPageEventHandler : public QObject {
    Q_OBJECT

public:
    QtWebPageEventHandler(WKPageRef, WebKit::QtViewportInteractionEngine* = 0);
    ~QtWebPageEventHandler();

    static Qt::DropAction dragOperationToDropAction(unsigned dragOperation);
    static Qt::DropActions dragOperationToDropActions(unsigned dragOperations);
    static WebCore::DragOperation dropActionToDragOperation(Qt::DropActions);

    bool handleEvent(QEvent*);

    void setViewportInteractionEngine(QtViewportInteractionEngine*);

    void handleSingleTapEvent(const QTouchEvent::TouchPoint&);
    void handleDoubleTapEvent(const QTouchEvent::TouchPoint&);

    void didFindZoomableArea(const WebCore::IntPoint& target, const WebCore::IntRect& area);
    void doneWithTouchEvent(const NativeWebTouchEvent&, bool wasEventHandled);
    void resetGestureRecognizers();

    QtViewportInteractionEngine* interactionEngine() { return m_interactionEngine; }

protected:
    WebPageProxy* m_webPageProxy;
    QtViewportInteractionEngine* m_interactionEngine;
    QtPanGestureRecognizer m_panGestureRecognizer;
    QtPinchGestureRecognizer m_pinchGestureRecognizer;
    QtTapGestureRecognizer m_tapGestureRecognizer;

private:
    bool handleKeyPressEvent(QKeyEvent*);
    bool handleKeyReleaseEvent(QKeyEvent*);
    bool handleFocusInEvent(QFocusEvent*);
    bool handleFocusOutEvent(QFocusEvent*);
    bool handleMouseMoveEvent(QMouseEvent*);
    bool handleMousePressEvent(QMouseEvent*);
    bool handleMouseReleaseEvent(QMouseEvent*);
    bool handleMouseDoubleClickEvent(QMouseEvent*);
    bool handleWheelEvent(QWheelEvent*);
    bool handleHoverLeaveEvent(QHoverEvent*);
    bool handleHoverMoveEvent(QHoverEvent*);
    bool handleDragEnterEvent(QDragEnterEvent*);
    bool handleDragLeaveEvent(QDragLeaveEvent*);
    bool handleDragMoveEvent(QDragMoveEvent*);
    bool handleDropEvent(QDropEvent*);

    void timerEvent(QTimerEvent*);

    void touchEvent(QTouchEvent*);

    QPoint m_tripleClick;
    QBasicTimer m_tripleClickTimer;
};

#endif /* QtWebPageEventHandler_h */
