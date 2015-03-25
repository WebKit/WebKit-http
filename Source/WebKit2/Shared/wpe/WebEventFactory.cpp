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
#include "WebEventFactory.h"

#include "KeyMapping.h"
#include "WPEInputEvents.h"

namespace WebKit {

static WebEvent::Modifiers modifiersForEvent(const WPE::KeyboardEvent& event)
{
    unsigned modifiers = 0;

    if (event.modifiers & WPE::KeyboardEvent::Control)
        modifiers |= WebEvent::ControlKey;
    if (event.modifiers & WPE::KeyboardEvent::Shift)
        modifiers |= WebEvent::ShiftKey;
    if (event.modifiers & WPE::KeyboardEvent::Alt)
        modifiers |= WebEvent::AltKey;
    if (event.modifiers & WPE::KeyboardEvent::Meta)
        modifiers |= WebEvent::MetaKey;

    return static_cast<WebEvent::Modifiers>(modifiers);
}

WebKeyboardEvent WebEventFactory::createWebKeyboardEvent(WPE::KeyboardEvent&& event)
{
    String singleCharacterString = KeyMapping::singleCharacterStringForKeyEvent(event);
    return WebKeyboardEvent(event.pressed ? WebEvent::KeyDown : WebEvent::KeyUp,
        singleCharacterString, singleCharacterString,
        KeyMapping::identifierForKeyEvent(event),
        KeyMapping::windowsKeyCodeForKeyEvent(event),
        event.keyCode, 0, false, false, false,
        modifiersForEvent(event), event.time);
}

WebMouseEvent WebEventFactory::createWebMouseEvent(WPE::PointerEvent&& event)
{
    WebEvent::Type type = WebEvent::NoType;
    switch (event.type) {
    case WPE::PointerEvent::Motion:
        type = WebEvent::MouseMove;
        break;
    case WPE::PointerEvent::Button:
        type = event.state ? WebEvent::MouseDown : WebEvent::MouseUp;
        break;
    case WPE::PointerEvent::Null:
        ASSERT_NOT_REACHED();
    }

    // FIXME: Proper button support. Modifiers. deltaX/Y/Z. Click count.
    WebCore::IntPoint position(event.x, event.y);
    return WebMouseEvent(type, WebMouseEvent::LeftButton /* FIXME: HAH! */,
        position, position,
        0, 0, 0, 1, static_cast<WebEvent::Modifiers>(0), event.time);
}

WebWheelEvent WebEventFactory::createWebWheelEvent(WPE::AxisEvent&& event)
{
    // FIXME: We shouldn't hard-code this.
    enum Axis {
        Vertical,
        Horizontal
    };

    WebCore::FloatSize wheelTicks;
    switch (event.axis) {
    case Vertical:
        wheelTicks = WebCore::FloatSize(0, 1);
        break;
    case Horizontal:
        wheelTicks = WebCore::FloatSize(1, 0);
        break;
    default:
        ASSERT_NOT_REACHED();
    };

    WebCore::FloatSize delta = wheelTicks;
    delta.scale(event.value);

    WebCore::IntPoint position(event.x, event.y);
    return WebWheelEvent(WebEvent::Wheel, position, position,
        delta, wheelTicks, WebWheelEvent::ScrollByPixelWheelEvent, static_cast<WebEvent::Modifiers>(0), event.time);
}

WebTouchEvent WebEventFactory::createWebTouchEvent(WPE::TouchEvent&& event)
{
    WebEvent::Type type = WebEvent::NoType;
    switch (event.type) {
    case WPE::TouchEvent::Down:
        type = WebEvent::TouchStart;
        break;
    case WPE::TouchEvent::Motion:
        type = WebEvent::TouchMove;
        break;
    case WPE::TouchEvent::Up:
        type = WebEvent::TouchEnd;
        break;
    case WPE::TouchEvent::Null:
        ASSERT_NOT_REACHED();
    }

    return WebTouchEvent(type, WTF::move(event.touchPoints), WebEvent::Modifiers(0), event.time);
}

} // namespace WebKit
