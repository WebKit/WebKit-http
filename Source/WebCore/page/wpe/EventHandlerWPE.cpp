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
#include "EventHandler.h"

#include "NotImplemented.h"

namespace WebCore {

bool EventHandler::tabsToAllFormControls(KeyboardEvent*) const
{
    notImplemented();
    return false;
}

void EventHandler::focusDocumentView()
{
    notImplemented();
}

bool EventHandler::passWidgetMouseDownEventToWidget(const MouseEventWithHitTestResults&)
{
    notImplemented();
    return false;
}

bool EventHandler::passWidgetMouseDownEventToWidget(RenderWidget*)
{
    notImplemented();
    return false;
}

bool EventHandler::passMouseDownEventToWidget(Widget*)
{
    notImplemented();
    return false;
}

bool EventHandler::eventActivatedView(const PlatformMouseEvent&) const
{
    notImplemented();
    return false;
}

bool EventHandler::passWheelEventToWidget(const PlatformWheelEvent&, Widget&)
{
    notImplemented();
    return false;
}

bool EventHandler::passMousePressEventToSubframe(MouseEventWithHitTestResults&, Frame*)
{
    notImplemented();
    return false;
}

bool EventHandler::passMouseMoveEventToSubframe(MouseEventWithHitTestResults&, Frame*, HitTestResult*)
{
    notImplemented();
    return false;
}

bool EventHandler::passMouseReleaseEventToSubframe(MouseEventWithHitTestResults&, Frame*)
{
    notImplemented();
    return false;
}

unsigned EventHandler::accessKeyModifiers()
{
    notImplemented();
    return PlatformEvent::AltKey;
}

// GTK+ must scroll horizontally if the mouse pointer is on top of the
// horizontal scrollbar while scrolling with the wheel; we need to
// add the deltas and ticks here so that this behavior is consistent
// for styled scrollbars.
bool EventHandler::shouldTurnVerticalTicksIntoHorizontal(const HitTestResult&, const PlatformWheelEvent&) const
{
    notImplemented();
    return false;
}

}
