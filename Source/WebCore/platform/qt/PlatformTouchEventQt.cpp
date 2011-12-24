/*
 * This file is part of the WebKit project.
 *
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)
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

#include "config.h"
#include "PlatformTouchEvent.h"
#include <wtf/CurrentTime.h>

#if ENABLE(TOUCH_EVENTS)

namespace WebCore {

PlatformTouchEvent::PlatformTouchEvent(QTouchEvent* event)
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
    }

    const QList<QTouchEvent::TouchPoint>& points = event->touchPoints();
    for (int i = 0; i < points.count(); ++i)
        m_touchPoints.append(PlatformTouchPoint(points.at(i)));

    m_modifiers = 0;
    if (event->modifiers()  & Qt::ShiftModifier)
        m_modifiers |= ShiftKey;
    if (event->modifiers()  & Qt::ControlModifier)
        m_modifiers |= CtrlKey;
    if (event->modifiers()  & Qt::AltModifier)
        m_modifiers |= AltKey;
    if (event->modifiers()  & Qt::MetaModifier)
        m_modifiers |= MetaKey;

    m_timestamp = WTF::currentTime();
}

}

#endif
