/*
 * Copyright (C) 2015 Igalia S.L.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Handling.h"

#include "KeyboardEventHandler.h"
#include "KeyboardEventRepeating.h"
#include <glib.h>

namespace WPE {

namespace Input {

Server& Server::singleton()
{
    static Server singletonObject;
    return singletonObject;
}

Server::Server()
    : m_keyboardEventHandler(KeyboardEventHandler::create())
    , m_keyboardEventRepeating(std::unique_ptr<KeyboardEventRepeating>(new KeyboardEventRepeating))
{
}

void Server::setTarget(Client* target)
{
    m_target = target;
}

void Server::serveKeyboardEvent(KeyboardEvent::Raw&& event)
{
    if (!m_target) {
        m_keyboardEventRepeating->cancel();
        return;
    }

    KeyboardEventHandler::Result handlingResult = m_keyboardEventHandler->handleKeyboardEvent(event);
    m_target->handleKeyboardEvent({
        event.time,
        std::get<0>(handlingResult),
        std::get<1>(handlingResult),
        !!event.state,
        std::get<2>(handlingResult)
    });

    if (!!event.state)
        m_keyboardEventRepeating->schedule(event);
    else
        m_keyboardEventRepeating->cancel();
}

void Server::servePointerEvent(PointerEvent::Raw&& event)
{
    if (event.type == PointerEvent::Motion) {
        // FIXME: PointerEvent should store x/y coordinates in int32_t.
        m_pointer = {
            static_cast<uint32_t>(std::max(0, event.x)),
            static_cast<uint32_t>(std::max(0, event.y))
        };
    }

    if (m_target)
        m_target->handlePointerEvent({
            event.type,
            event.time,
            static_cast<int>(m_pointer.x),
            static_cast<int>(m_pointer.y),
            event.button,
            event.state
        });
}

void Server::serveAxisEvent(AxisEvent::Raw&& event)
{
    if (m_target)
        m_target->handleAxisEvent({
            event.type,
            event.time,
            static_cast<int>(m_pointer.x),
            static_cast<int>(m_pointer.y),
            event.axis,
            event.value
        });
}

void Server::serveTouchEvent(TouchEvent::Raw&& event)
{
    auto& targetPoint = m_touchEvents[event.id];

    switch (event.type) {
    case TouchEvent::Null:
        return;
    case TouchEvent::Down:
        targetPoint = TouchEvent::Raw{ TouchEvent::Down, event.time, event.id, event.x, event.y };
        break;
    case TouchEvent::Motion:
        targetPoint = TouchEvent::Raw{ TouchEvent::Motion, event.time, event.id, event.x, event.y };
        break;
    case TouchEvent::Up:
        targetPoint = TouchEvent::Raw{ TouchEvent::Up, event.time, event.id, targetPoint.x, targetPoint.y };
        break;
    }

    if (m_target)
        m_target->handleTouchEvent({
            m_touchEvents,
            targetPoint.type,
            targetPoint.id,
            targetPoint.time
        });

    if (event.type == TouchEvent::Up)
        targetPoint = TouchEvent::Raw{ TouchEvent::Null, 0, -1, -1, -1 };
}

} // namespace Input

} // namespace WPE
