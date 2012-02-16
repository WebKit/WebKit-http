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
#include "PlatformWheelEvent.h"
#include <QApplication>
#include <QGraphicsSceneMouseEvent>
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
    // A delta that is not mod 120 indicates a device that is sending
    // fine-resolution scroll events, so use the delta as number of wheel ticks
    // and number of pixels to scroll.See also webkit.org/b/29601
    bool fullTick = !(delta % 120);

    if (orientation == Qt::Horizontal) {
        m_deltaX = (fullTick) ? delta / 120.0f : delta;
        m_deltaY = 0;
    } else {
        m_deltaX = 0;
        m_deltaY = (fullTick) ? delta / 120.0f : delta;
    }

    m_wheelTicksX = m_deltaX;
    m_wheelTicksY = m_deltaY;

    // Use the same single scroll step as QTextEdit
    // (in QTextEditPrivate::init [h,v]bar->setSingleStep)
    static const float cDefaultQtScrollStep = 20.f;
    m_deltaX *= (fullTick) ? QApplication::wheelScrollLines() * cDefaultQtScrollStep : 1;
    m_deltaY *= (fullTick) ? QApplication::wheelScrollLines() * cDefaultQtScrollStep : 1;
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


PlatformWheelEvent convertWheelEvent(QWheelEvent* event)
{
    return WebKitPlatformWheelEvent(event);
}

PlatformWheelEvent convertWheelEvent(QGraphicsSceneWheelEvent* event)
{
    return WebKitPlatformWheelEvent(event);
}

}
