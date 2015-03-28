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
#include "WKInputHandler.h"

#include "WKAPICast.h"
#include "WPEInputHandler.h"
#include "WPEView.h"
#include <WPE/Input/Events.h>

using namespace WebKit;

WKInputHandlerRef WKInputHandlerCreate(WKViewRef view)
{
    return toAPI(WKWPE::InputHandler::create(*toImpl(view)));
}

void WKInputHandlerNotifyKeyboardKey(WKInputHandlerRef handler, WKKeyboardKey event)
{
    toImpl(handler)->handleKeyboardKey(WPE::Input::KeyboardEvent::Raw{
        event.time,
        event.key,
        event.state
    });
}

void WKInputHandlerNotifyPointerMotion(WKInputHandlerRef handler, WKPointerMotion event)
{
    toImpl(handler)->handlePointerEvent(WPE::Input::PointerEvent::Raw{
        WPE::Input::PointerEvent::Motion,
        event.time,
        event.x,
        event.y,
        0, 0
    });
}

void WKInputHandlerNotifyPointerButton(WKInputHandlerRef handler, WKPointerButton event)
{
    toImpl(handler)->handlePointerEvent(WPE::Input::PointerEvent::Raw{
        WPE::Input::PointerEvent::Button,
        event.time,
        0, 0,
        event.button,
        event.state
    });
}

void WKInputHandlerNotifyAxisMotion(WKInputHandlerRef handler, WKAxisMotion event)
{
    toImpl(handler)->handleAxisEvent(WPE::Input::AxisEvent::Raw{
        WPE::Input::AxisEvent::Motion,
        event.time,
        event.axis,
        event.value
    });
}

void WKInputHandlerNotifyTouchDown(WKInputHandlerRef handler, WKTouchDown event)
{
    toImpl(handler)->handleTouchDown(WPE::Input::TouchEvent::Raw{
        WPE::Input::TouchEvent::Down,
        event.time,
        event.id,
        event.x,
        event.y
    });
}

void WKInputHandlerNotifyTouchUp(WKInputHandlerRef handler, WKTouchUp event)
{
    toImpl(handler)->handleTouchUp(WPE::Input::TouchEvent::Raw{
        WPE::Input::TouchEvent::Up,
        event.time,
        event.id,
        -1, -1
    });
}

void WKInputHandlerNotifyTouchMotion(WKInputHandlerRef handler, WKTouchMotion event)
{
    toImpl(handler)->handleTouchMotion(WPE::Input::TouchEvent::Raw{
        WPE::Input::TouchEvent::Motion,
        event.time,
        event.id,
        event.x,
        event.y
    });
}
