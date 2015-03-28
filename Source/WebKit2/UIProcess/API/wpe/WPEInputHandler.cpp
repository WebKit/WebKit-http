/*
 * Copyright (C) 2014 Igalia S.L.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WPEInputHandler.h"

#include "NativeWebKeyboardEvent.h"
#include "NativeWebMouseEvent.h"
#include "NativeWebWheelEvent.h"
#include "WPEView.h"

namespace WKWPE {

InputHandler::InputHandler(View& view)
    : m_view(view)
    , m_keyInputHandler(WPE::KeyInputHandler::create())
{
    m_pointer.x = m_pointer.y = 0;
}

void InputHandler::handleKeyboardKey(WPE::Input::KeyboardEvent::Raw event)
{
    WPE::KeyInputHandler::HandlingResult handledEvent = m_keyInputHandler->handleKeyInputEvent(event);
    m_view.page().handleKeyboardEvent(WebKit::NativeWebKeyboardEvent({
        event.time,
        std::get<0>(handledEvent),
        std::get<1>(handledEvent),
        !!event.state,
        std::get<2>(handledEvent)
    }));
}

void InputHandler::handlePointerEvent(WPE::Input::PointerEvent::Raw event)
{
    if (event.type == WPE::Input::PointerEvent::Motion) {
        const WebCore::IntSize& viewSize = m_view.size();
        m_pointer.x = std::max(0, std::min(event.x, viewSize.width() - 1));
        m_pointer.y = std::max(0, std::min(event.y, viewSize.height() - 1));
    }

    m_view.page().handleMouseEvent(WebKit::NativeWebMouseEvent({
        event.type,
        event.time,
        m_pointer.x,
        m_pointer.y,
        event.button,
        event.state
    }));
}

void InputHandler::handleAxisEvent(WPE::Input::AxisEvent::Raw event)
{
    ASSERT(event.type == WPE::Input::AxisEvent::Motion);
    m_view.page().handleWheelEvent(WebKit::NativeWebWheelEvent({
        event.type,
        event.time,
        m_pointer.x,
        m_pointer.y,
        event.axis,
        event.value
    }));
}

void InputHandler::handleTouchDown(WPE::Input::TouchEvent::Raw event)
{
    ASSERT(m_touchEvents[event.id].type == WPE::Input::TouchEvent::Null);
    m_touchEvents[event.id] = WPE::Input::TouchEvent::Raw{ WPE::Input::TouchEvent::Down, event.time, event.id, event.x, event.y };
    dispatchTouchEvent(event.id);
}

void InputHandler::handleTouchUp(WPE::Input::TouchEvent::Raw event)
{
    ASSERT(m_touchEvents[event.id].type == WPE::Input::TouchEvent::Down || m_touchEvents[event.id].type == WPE::Input::TouchEvent::Motion);
    auto& previousEvent = m_touchEvents[event.id];
    int x = previousEvent.x;
    int y = previousEvent.y;
    m_touchEvents[event.id] = WPE::Input::TouchEvent::Raw{ WPE::Input::TouchEvent::Up, event.time, event.id, x, y };
    dispatchTouchEvent(event.id);
}

void InputHandler::handleTouchMotion(WPE::Input::TouchEvent::Raw event)
{
    ASSERT(m_touchEvents[event.id].type == WPE::Input::TouchEvent::Down || m_touchEvents[event.id].type == WPE::Input::TouchEvent::Motion);
    m_touchEvents[event.id] = WPE::Input::TouchEvent::Raw{ WPE::Input::TouchEvent::Motion, event.time, event.id, event.x, event.y };
    dispatchTouchEvent(event.id);
}

void InputHandler::dispatchTouchEvent(int id)
{
    auto& event = m_touchEvents[id];
    m_view.page().handleTouchEvent(WebKit::NativeWebTouchEvent(WPE::Input::TouchEvent{
        m_touchEvents,
        event.type,
        id,
        event.time
    }));

    if (event.type == WPE::Input::TouchEvent::Up)
        event = WPE::Input::TouchEvent::Raw{ WPE::Input::TouchEvent::Null, 0, -1, -1, -1 };
}

} // namespace WPE
