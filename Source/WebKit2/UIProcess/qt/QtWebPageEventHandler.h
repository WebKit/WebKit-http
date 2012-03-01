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
#include <QInputMethodEvent>
#include <QTouchEvent>
#include <WKPage.h>

class QQuickWebPage;

using namespace WebKit;

class QtWebPageEventHandler : public QObject {
    Q_OBJECT

public:
    QtWebPageEventHandler(WKPageRef, QQuickWebPage*, QQuickWebView*);
    ~QtWebPageEventHandler();

    void handleKeyPressEvent(QKeyEvent*);
    void handleKeyReleaseEvent(QKeyEvent*);
    void handleFocusInEvent(QFocusEvent*);
    void handleFocusOutEvent(QFocusEvent*);
    void handleMouseMoveEvent(QMouseEvent*);
    void handleMousePressEvent(QMouseEvent*);
    void handleMouseReleaseEvent(QMouseEvent*);
    void handleWheelEvent(QWheelEvent*);
    void handleHoverLeaveEvent(QHoverEvent*);
    void handleHoverMoveEvent(QHoverEvent*);
    void handleDragEnterEvent(QDragEnterEvent*);
    void handleDragLeaveEvent(QDragLeaveEvent*);
    void handleDragMoveEvent(QDragMoveEvent*);
    void handleDropEvent(QDropEvent*);
    void handleInputMethodEvent(QInputMethodEvent*);
    void handleTouchEvent(QTouchEvent*);

    void setViewportInteractionEngine(QtViewportInteractionEngine*);

    void handlePotentialSingleTapEvent(const QTouchEvent::TouchPoint&);
    void handleSingleTapEvent(const QTouchEvent::TouchPoint&);
    void handleDoubleTapEvent(const QTouchEvent::TouchPoint&);

    void didFindZoomableArea(const WebCore::IntPoint& target, const WebCore::IntRect& area);
    void updateTextInputState();
    void doneWithGestureEvent(const WebGestureEvent& event, bool wasEventHandled);
    void doneWithTouchEvent(const NativeWebTouchEvent&, bool wasEventHandled);
    void resetGestureRecognizers();

    QtViewportInteractionEngine* interactionEngine() { return m_interactionEngine; }

    void startDrag(const WebCore::DragData&, PassRefPtr<ShareableBitmap> dragImage);

protected:
    WebPageProxy* m_webPageProxy;
    QtViewportInteractionEngine* m_interactionEngine;
    QtPanGestureRecognizer m_panGestureRecognizer;
    QtPinchGestureRecognizer m_pinchGestureRecognizer;
    QtTapGestureRecognizer m_tapGestureRecognizer;
    QQuickWebPage* m_webPage;
    QQuickWebView* m_webView;

private slots:
    void inputPanelVisibleChanged();

private:
    void timerEvent(QTimerEvent*);

    QPointF m_lastClick;
    QBasicTimer m_clickTimer;
    Qt::MouseButton m_previousClickButton;
    int m_clickCount;
    bool m_postponeTextInputStateChanged;
};

#endif /* QtWebPageEventHandler_h */
