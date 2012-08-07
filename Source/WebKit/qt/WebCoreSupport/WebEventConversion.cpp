/*
    Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2006 Zack Rusin <zack@kde.org>
    Copyright (C) 2011 Research In Motion Limited.

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

#include "config.h"
#include "WebEventConversion.h"

#include "PlatformMouseEvent.h"
#include "PlatformTouchEvent.h"
#include "PlatformTouchPoint.h"
#include "PlatformWheelEvent.h"
#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QTouchEvent>
#include <QWheelEvent>
#include <wtf/CurrentTime.h>

namespace WebCore {

static void mouseEventModifiersFromQtKeyboardModifiers(Qt::KeyboardModifiers keyboardModifiers, unsigned& modifiers)
{
    modifiers = 0;
    if (keyboardModifiers & Qt::ShiftModifier)
        modifiers |= PlatformEvent::ShiftKey;
    if (keyboardModifiers & Qt::ControlModifier)
        modifiers |= PlatformEvent::CtrlKey;
    if (keyboardModifiers & Qt::AltModifier)
        modifiers |= PlatformEvent::AltKey;
    if (keyboardModifiers & Qt::MetaModifier)
        modifiers |= PlatformEvent::MetaKey;
}

static void mouseEventTypeAndMouseButtonFromQEvent(const QEvent* event, PlatformEvent::Type& mouseEventType, MouseButton& mouseButton)
{
    enum { MouseEvent, GraphicsSceneMouseEvent } frameworkMouseEventType;
    switch (event->type()) {
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonPress:
        frameworkMouseEventType = MouseEvent;
        mouseEventType = PlatformEvent::MousePressed;
        break;
    case QEvent::MouseButtonRelease:
        frameworkMouseEventType = MouseEvent;
        mouseEventType = PlatformEvent::MouseReleased;
        break;
    case QEvent::MouseMove:
        frameworkMouseEventType = MouseEvent;
        mouseEventType = PlatformEvent::MouseMoved;
        break;
    case QEvent::GraphicsSceneMouseDoubleClick:
    case QEvent::GraphicsSceneMousePress:
        frameworkMouseEventType = GraphicsSceneMouseEvent;
        mouseEventType = PlatformEvent::MousePressed;
        break;
    case QEvent::GraphicsSceneMouseRelease:
        frameworkMouseEventType = GraphicsSceneMouseEvent;
        mouseEventType = PlatformEvent::MouseReleased;
        break;
    case QEvent::GraphicsSceneMouseMove:
        frameworkMouseEventType = GraphicsSceneMouseEvent;
        mouseEventType = PlatformEvent::MouseMoved;
        break;
    default:
        ASSERT_NOT_REACHED();
        frameworkMouseEventType = MouseEvent;
        mouseEventType = PlatformEvent::MouseMoved;
        break;
    }

    Qt::MouseButtons mouseButtons;
    switch (frameworkMouseEventType) {
    case MouseEvent: {
        const QMouseEvent* mouseEvent = static_cast<const QMouseEvent*>(event);
        mouseButtons = mouseEventType == PlatformEvent::MouseMoved ? mouseEvent->buttons() : mouseEvent->button();
        break;
    }
    case GraphicsSceneMouseEvent: {
        const QGraphicsSceneMouseEvent* mouseEvent = static_cast<const QGraphicsSceneMouseEvent*>(event);
        mouseButtons = mouseEventType == PlatformEvent::MouseMoved ? mouseEvent->buttons() : mouseEvent->button();
        break;
    }
    }

    if (mouseButtons & Qt::LeftButton)
        mouseButton = LeftButton;
    else if (mouseButtons & Qt::RightButton)
        mouseButton = RightButton;
    else if (mouseButtons & Qt::MidButton)
        mouseButton = MiddleButton;
    else
        mouseButton = NoButton;
}

class WebKitPlatformMouseEvent : public PlatformMouseEvent {
public:
    WebKitPlatformMouseEvent(QGraphicsSceneMouseEvent*, int clickCount);
    WebKitPlatformMouseEvent(QInputEvent*, int clickCount);
};

WebKitPlatformMouseEvent::WebKitPlatformMouseEvent(QGraphicsSceneMouseEvent* event, int clickCount)
{
    m_timestamp = WTF::currentTime();

    // FIXME: Why don't we handle a context menu event here as we do in PlatformMouseEvent(QInputEvent*, int)?
    // See <https://bugs.webkit.org/show_bug.cgi?id=60728>.
    PlatformEvent::Type type;
    mouseEventTypeAndMouseButtonFromQEvent(event, type, m_button);

    m_type = type;
    m_position = IntPoint(event->pos().toPoint());
    m_globalPosition = IntPoint(event->screenPos());

    m_clickCount = clickCount;
    mouseEventModifiersFromQtKeyboardModifiers(event->modifiers(), m_modifiers);
}

WebKitPlatformMouseEvent::WebKitPlatformMouseEvent(QInputEvent* event, int clickCount)
{
    m_timestamp = WTF::currentTime();

    bool isContextMenuEvent = false;
#ifndef QT_NO_CONTEXTMENU
    if (event->type() == QEvent::ContextMenu) {
        isContextMenuEvent = true;
        m_type = PlatformEvent::MousePressed;
        QContextMenuEvent* ce = static_cast<QContextMenuEvent*>(event);
        m_position = IntPoint(ce->pos());
        m_globalPosition = IntPoint(ce->globalPos());
        m_button = RightButton;
    }
#endif
    if (!isContextMenuEvent) {
        PlatformEvent::Type type;
        mouseEventTypeAndMouseButtonFromQEvent(event, type, m_button);
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

        m_type = type;
        m_position = IntPoint(mouseEvent->pos());
        m_globalPosition = IntPoint(mouseEvent->globalPos());
    }

    m_clickCount = clickCount;
    mouseEventModifiersFromQtKeyboardModifiers(event->modifiers(), m_modifiers);
}

PlatformMouseEvent convertMouseEvent(QInputEvent* event, int clickCount)
{
    return WebKitPlatformMouseEvent(event, clickCount);
}

PlatformMouseEvent convertMouseEvent(QGraphicsSceneMouseEvent* event, int clickCount)
{
    return WebKitPlatformMouseEvent(event, clickCount);
}

class WebKitPlatformWheelEvent : public PlatformWheelEvent {
public:
    WebKitPlatformWheelEvent(QGraphicsSceneWheelEvent*);
    WebKitPlatformWheelEvent(QWheelEvent*);

private:
    void applyDelta(int delta, Qt::Orientation);
};

void WebKitPlatformWheelEvent::applyDelta(int delta, Qt::Orientation orientation)
{
    if (orientation == Qt::Horizontal) {
        m_deltaX = delta;
        m_deltaY = 0;
    } else {
        m_deltaX = 0;
        m_deltaY = delta;
    }
    m_wheelTicksX = m_deltaX / 120.0f;
    m_wheelTicksY = m_deltaY / 120.0f;

    // Since we request the scroll delta by the pixel, convert the wheel delta to pixel delta using the standard scroll step.
    // Use the same single scroll step as QTextEdit (in QTextEditPrivate::init [h,v]bar->setSingleStep)
    static const float cDefaultQtScrollStep = 20.f;
    m_deltaX = m_wheelTicksX * QApplication::wheelScrollLines() * cDefaultQtScrollStep;
    m_deltaY = m_wheelTicksY * QApplication::wheelScrollLines() * cDefaultQtScrollStep;
}

WebKitPlatformWheelEvent::WebKitPlatformWheelEvent(QGraphicsSceneWheelEvent* e)
{
    m_timestamp = WTF::currentTime();
    mouseEventModifiersFromQtKeyboardModifiers(e->modifiers(), m_modifiers);
    m_position = e->pos().toPoint();
    m_globalPosition = e->screenPos();
    m_granularity = ScrollByPixelWheelEvent;
    m_directionInvertedFromDevice = false;
    applyDelta(e->delta(), e->orientation());
}

WebKitPlatformWheelEvent::WebKitPlatformWheelEvent(QWheelEvent* e)
{
    m_timestamp = WTF::currentTime();
    mouseEventModifiersFromQtKeyboardModifiers(e->modifiers(), m_modifiers);
    m_position = e->pos();
    m_globalPosition = e->globalPos();
    m_granularity = ScrollByPixelWheelEvent;
    m_directionInvertedFromDevice = false;
    applyDelta(e->delta(), e->orientation());
}

#if ENABLE(TOUCH_EVENTS)
class WebKitPlatformTouchEvent : public PlatformTouchEvent {
public:
    WebKitPlatformTouchEvent(QTouchEvent*);
};

class WebKitPlatformTouchPoint : public PlatformTouchPoint {
public:
    WebKitPlatformTouchPoint(const QTouchEvent::TouchPoint&, State);
};

WebKitPlatformTouchEvent::WebKitPlatformTouchEvent(QTouchEvent* event)
{
    switch (event->type()) {
    case QEvent::TouchBegin:
        m_type = PlatformEvent::TouchStart;
        break;
    case QEvent::TouchUpdate:
        m_type = PlatformEvent::TouchMove;
        break;
    case QEvent::TouchEnd:
        m_type = PlatformEvent::TouchEnd;
        break;
    case QEvent::TouchCancel:
        m_type = PlatformEvent::TouchCancel;
        break;
    }

    const QList<QTouchEvent::TouchPoint>& points = event->touchPoints();
    for (int i = 0; i < points.count(); ++i) {
        PlatformTouchPoint::State state = PlatformTouchPoint::TouchStateEnd;

        switch (points.at(i).state()) {
        case Qt::TouchPointReleased:
            state = PlatformTouchPoint::TouchReleased;
            break;
        case Qt::TouchPointMoved:
            state = PlatformTouchPoint::TouchMoved;
            break;
        case Qt::TouchPointPressed:
            state = PlatformTouchPoint::TouchPressed;
            break;
        case Qt::TouchPointStationary:
            state = PlatformTouchPoint::TouchStationary;
            break;
        }

        // Qt does not have a Qt::TouchPointCancelled point state, so if we receive a touch cancel event,
        // simply cancel all touch points here.
        if (m_type == PlatformEvent::TouchCancel)
            state = PlatformTouchPoint::TouchCancelled;

        m_touchPoints.append(WebKitPlatformTouchPoint(points.at(i), state));
    }

    mouseEventModifiersFromQtKeyboardModifiers(event->modifiers(), m_modifiers);

    m_timestamp = WTF::currentTime();
}

WebKitPlatformTouchPoint::WebKitPlatformTouchPoint(const QTouchEvent::TouchPoint& point, State state)
{
    // The QTouchEvent::TouchPoint API states that ids will be >= 0.
    m_id = point.id();
    m_state = state;
    m_screenPos = point.screenPos().toPoint();
    m_pos = point.pos().toPoint();
    // Qt reports touch point size as rectangles, but we will pretend it is an oval.
    QRect touchRect = point.rect().toAlignedRect();
    if (touchRect.isValid()) {
        m_radiusX = point.rect().width() / 2;
        m_radiusY = point.rect().height() / 2;
    } else {
        // http://www.w3.org/TR/2011/WD-touch-events-20110505: 1 if no value is known.
        m_radiusX = 1;
        m_radiusY = 1;
    }
    m_force = point.pressure();
    // FIXME: Support m_rotationAngle if QTouchEvent at some point supports it.
}
#endif

PlatformWheelEvent convertWheelEvent(QWheelEvent* event)
{
    return WebKitPlatformWheelEvent(event);
}

PlatformWheelEvent convertWheelEvent(QGraphicsSceneWheelEvent* event)
{
    return WebKitPlatformWheelEvent(event);
}

#if ENABLE(TOUCH_EVENTS)
PlatformTouchEvent convertTouchEvent(QTouchEvent* event)
{
    return WebKitPlatformTouchEvent(event);
}
#endif

}
