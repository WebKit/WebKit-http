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

namespace WPE {

InputHandler::InputHandler(View& view)
    : m_view(view)
    , m_keyInputHandler(KeyInputHandler::create())
{
    m_pointer.x = m_pointer.y = 0;
}

void InputHandler::handleKeyboardKey(KeyboardEvent::Raw event)
{
    KeyInputHandler::HandlingResult handledEvent = m_keyInputHandler->handleKeyInputEvent(event);
    m_view.page().handleKeyboardEvent(WebKit::NativeWebKeyboardEvent({
        event.time,
        std::get<0>(handledEvent),
        std::get<1>(handledEvent),
        !!event.state,
        std::get<2>(handledEvent)
    }));
}

void InputHandler::handlePointerEvent(PointerEvent::Raw event)
{
    if (event.type == PointerEvent::Motion) {
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

void InputHandler::handleAxisEvent(AxisEvent::Raw event)
{
    ASSERT(event.type == AxisEvent::Motion);
    m_view.page().handleWheelEvent(WebKit::NativeWebWheelEvent({
        event.type,
        event.time,
        m_pointer.x,
        m_pointer.y,
        event.axis,
        event.value
    }));
}

void InputHandler::handleTouchDown(TouchEvent::Raw event)
{
    ASSERT(m_touchEvents[event.id].type == TouchEvent::Null);
    m_touchEvents[event.id] = TouchEvent::Raw{ TouchEvent::Down, event.time, event.id, event.x, event.y };
    dispatchTouchEvent(event.id);
}

void InputHandler::handleTouchUp(TouchEvent::Raw event)
{
    ASSERT(m_touchEvents[event.id].type == TouchEvent::Down || m_touchEvents[event.id].type == TouchEvent::Motion);
    auto& previousEvent = m_touchEvents[event.id];
    int x = previousEvent.x;
    int y = previousEvent.y;
    m_touchEvents[event.id] = TouchEvent::Raw{ TouchEvent::Up, event.time, event.id, x, y };
    dispatchTouchEvent(event.id);
}

void InputHandler::handleTouchMotion(TouchEvent::Raw event)
{
    ASSERT(m_touchEvents[event.id].type == TouchEvent::Down || m_touchEvents[event.id].type == TouchEvent::Motion);
    m_touchEvents[event.id] = TouchEvent::Raw{ TouchEvent::Motion, event.time, event.id, event.x, event.y };
    dispatchTouchEvent(event.id);
}

static WebKit::WebPlatformTouchPoint::TouchPointState stateForTouchEvent(int mainEventId, const TouchEvent::Raw& event)
{
    if (event.id != mainEventId)
        return WebKit::WebPlatformTouchPoint::TouchStationary;

    switch (event.type) {
    case TouchEvent::Down:
        return WebKit::WebPlatformTouchPoint::TouchPressed;
    case TouchEvent::Motion:
        return WebKit::WebPlatformTouchPoint::TouchMoved;
    case TouchEvent::Up:
        return WebKit::WebPlatformTouchPoint::TouchReleased;
    case TouchEvent::Null:
        ASSERT_NOT_REACHED();
        return WebKit::WebPlatformTouchPoint::TouchStationary;
    };
}

void InputHandler::dispatchTouchEvent(int id)
{
    Vector<WebKit::WebPlatformTouchPoint> touchPoints;
    touchPoints.reserveCapacity(m_touchEvents.size());

    for (auto& event : m_touchEvents) {
        if (event.type == TouchEvent::Null)
            continue;

        touchPoints.uncheckedAppend(WebKit::WebPlatformTouchPoint(event.id, stateForTouchEvent(id, event),
            WebCore::IntPoint(event.x, event.y), WebCore::IntPoint(event.x, event.y)));
    }

    auto& event = m_touchEvents[id];
    m_view.page().handleTouchEvent(WebKit::NativeWebTouchEvent(WPE::TouchEvent{
        WTF::move(touchPoints),
        event.type,
        event.time
    }));

    if (event.type == TouchEvent::Up)
        event = TouchEvent::Raw{ TouchEvent::Null, 0, -1, -1, -1 };
}

} // namespace WPE
