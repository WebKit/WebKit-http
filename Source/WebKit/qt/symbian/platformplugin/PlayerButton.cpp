/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
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

#include "PlayerButton.h"

#include <QGesture>
#include <QGestureEvent>

PlayerButton::PlayerButton(QWidget *parent)
    : QPushButton(parent)
{
    grabGesture(Qt::TapAndHoldGesture);
    setFixedSize(50, 50);
    setIconSize(QSize(50, 50));
    setStyleSheet("border: none");
}

bool PlayerButton::event(QEvent *event)
{
    if (event->type() == QEvent::Gesture) {
        QGestureEvent * gestureEvent = (QGestureEvent*)static_cast<QGestureEvent*>(event);
        QGesture * gesture = gestureEvent->gesture(Qt::TapAndHoldGesture);
        if (gesture && gesture->state() == Qt::GestureFinished) {
            emit tapAndHold();
            return true;
        }
    }
    return QPushButton::event(event);
}
