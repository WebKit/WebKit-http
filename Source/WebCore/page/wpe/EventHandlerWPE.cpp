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

#include "FocusController.h"
#include "Frame.h"
#include "FrameView.h"
#include "KeyboardEvent.h"
#include "MouseEventWithHitTestResults.h"
#include "NotImplemented.h"
#include "Page.h"
#include "PlatformKeyboardEvent.h"
#include "PlatformWheelEvent.h"
#include "RenderWidget.h"

namespace WebCore {

bool EventHandler::tabsToAllFormControls(KeyboardEvent*) const
{
    // We always allow tabs to all controls
    return true;
}

void EventHandler::focusDocumentView()
{
    if (Page* page = m_frame.page())
        page->focusController().setFocusedFrame(&m_frame);
}

bool EventHandler::passWidgetMouseDownEventToWidget(const MouseEventWithHitTestResults& event)
{
    // Figure out which view to send the event to.
    RenderObject* target = event.targetNode() ? event.targetNode()->renderer() : nullptr;
    if (!is<RenderWidget>(target))
        return false;
    return passMouseDownEventToWidget(downcast<RenderWidget>(*target).widget());
}

bool EventHandler::passWidgetMouseDownEventToWidget(RenderWidget* renderWidget)
{
    return passMouseDownEventToWidget(renderWidget->widget());
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

bool EventHandler::widgetDidHandleWheelEvent(const PlatformWheelEvent& event, Widget& widget)
{
    if (!is<FrameView>(widget))
        return false;

    return downcast<FrameView>(widget).frame().eventHandler().handleWheelEvent(event);
}

bool EventHandler::passMousePressEventToSubframe(MouseEventWithHitTestResults& mev, Frame* subframe)
{
    subframe->eventHandler().handleMousePressEvent(mev.event());
    return true;
}

bool EventHandler::passMouseMoveEventToSubframe(MouseEventWithHitTestResults& mev, Frame* subframe, HitTestResult* hoveredNode)
{
    subframe->eventHandler().handleMouseMoveEvent(mev.event(), hoveredNode);
    return true;
}

bool EventHandler::passMouseReleaseEventToSubframe(MouseEventWithHitTestResults& mev, Frame* subframe)
{
    subframe->eventHandler().handleMouseReleaseEvent(mev.event());
    return true;
}

OptionSet<PlatformEvent::Modifier> EventHandler::accessKeyModifiers()
{
    return PlatformEvent::Modifier::AltKey;
}

bool EventHandler::shouldTurnVerticalTicksIntoHorizontal(const HitTestResult&, const PlatformWheelEvent&) const
{
    notImplemented();
    return false;
}

}
