/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebPopupMenuImpl.h"

#include "Cursor.h"
#include "FramelessScrollView.h"
#include "FrameView.h"
#include "IntRect.h"
#include "NotImplemented.h"
#include "painting/GraphicsContextBuilder.h"
#include "PlatformKeyboardEvent.h"
#include "PlatformMouseEvent.h"
#include "PlatformWheelEvent.h"
#include "PopupContainer.h"
#include "PopupMenuChromium.h"
#include "SkiaUtils.h"

#include "WebInputEvent.h"
#include "WebInputEventConversion.h"
#include "WebRange.h"
#include "WebRect.h"
#include "WebViewClient.h"
#include "WebWidgetClient.h"

#include <skia/ext/platform_canvas.h>

#if ENABLE(GESTURE_RECOGNIZER)
#include "PlatformGestureEvent.h"
#include "PlatformGestureRecognizer.h"
#endif

using namespace WebCore;

namespace WebKit {

// WebPopupMenu ---------------------------------------------------------------

WebPopupMenu* WebPopupMenu::create(WebWidgetClient* client)
{
    // Pass the WebPopupMenuImpl's self-reference to the caller.
    return adoptRef(new WebPopupMenuImpl(client)).leakRef();
}

// WebWidget ------------------------------------------------------------------

WebPopupMenuImpl::WebPopupMenuImpl(WebWidgetClient* client)
    : m_client(client)
    , m_widget(0)
#if ENABLE(GESTURE_RECOGNIZER)
    , m_gestureRecognizer(WebCore::PlatformGestureRecognizer::create())
#endif
{
    // set to impossible point so we always get the first mouse pos
    m_lastMousePosition = WebPoint(-1, -1);
}

WebPopupMenuImpl::~WebPopupMenuImpl()
{
    if (m_widget)
        m_widget->setClient(0);
}

void WebPopupMenuImpl::Init(FramelessScrollView* widget, const WebRect& bounds)
{
    m_widget = widget;
    m_widget->setClient(this);

    if (m_client) {
        m_client->setWindowRect(bounds);
        m_client->show(WebNavigationPolicy());  // Policy is ignored
    }
}

void WebPopupMenuImpl::MouseMove(const WebMouseEvent& event)
{
    // don't send mouse move messages if the mouse hasn't moved.
    if (event.x != m_lastMousePosition.x || event.y != m_lastMousePosition.y) {
        m_lastMousePosition = WebPoint(event.x, event.y);
        m_widget->handleMouseMoveEvent(PlatformMouseEventBuilder(m_widget, event));

        // We cannot call setToolTipText() in PopupContainer, because PopupContainer is in WebCore, and we cannot refer to WebKit from Webcore.
        WebCore::PopupContainer* container = static_cast<WebCore::PopupContainer*>(m_widget);
        client()->setToolTipText(container->getSelectedItemToolTip(), container->menuStyle().textDirection() == WebCore::RTL ? WebTextDirectionRightToLeft : WebTextDirectionLeftToRight);
    }
}

void WebPopupMenuImpl::MouseLeave(const WebMouseEvent& event)
{
    m_widget->handleMouseMoveEvent(PlatformMouseEventBuilder(m_widget, event));
}

void WebPopupMenuImpl::MouseDown(const WebMouseEvent& event)
{
    m_widget->handleMouseDownEvent(PlatformMouseEventBuilder(m_widget, event));
}

void WebPopupMenuImpl::MouseUp(const WebMouseEvent& event)
{
    mouseCaptureLost();
    m_widget->handleMouseReleaseEvent(PlatformMouseEventBuilder(m_widget, event));
}

void WebPopupMenuImpl::MouseWheel(const WebMouseWheelEvent& event)
{
    m_widget->handleWheelEvent(PlatformWheelEventBuilder(m_widget, event));
}

bool WebPopupMenuImpl::GestureEvent(const WebGestureEvent& event)
{
    return m_widget->handleGestureEvent(PlatformGestureEventBuilder(m_widget, event));
}

#if ENABLE(TOUCH_EVENTS)
bool WebPopupMenuImpl::TouchEvent(const WebTouchEvent& event)
{

    PlatformTouchEventBuilder touchEventBuilder(m_widget, event);
    bool defaultPrevented(m_widget->handleTouchEvent(touchEventBuilder));
#if ENABLE(GESTURE_RECOGNIZER)
    OwnPtr<Vector<WebCore::PlatformGestureEvent> > gestureEvents(m_gestureRecognizer->processTouchEventForGestures(touchEventBuilder, defaultPrevented));
    for (unsigned int  i = 0; i < gestureEvents->size(); i++)
        m_widget->handleGestureEvent((*gestureEvents)[i]);
#endif
    return defaultPrevented;
}
#endif

bool WebPopupMenuImpl::KeyEvent(const WebKeyboardEvent& event)
{
    return m_widget->handleKeyEvent(PlatformKeyboardEventBuilder(event));
}

// WebWidget -------------------------------------------------------------------

void WebPopupMenuImpl::close()
{
    if (m_widget)
        m_widget->hide();

    m_client = 0;

    deref();  // Balances ref() from WebWidget::Create
}

void WebPopupMenuImpl::willStartLiveResize()
{
}

void WebPopupMenuImpl::resize(const WebSize& newSize)
{
    if (m_size == newSize)
        return;
    m_size = newSize;

    if (m_widget) {
        IntRect newGeometry(0, 0, m_size.width, m_size.height);
        m_widget->setFrameRect(newGeometry);
    }

    if (m_client) {
        WebRect damagedRect(0, 0, m_size.width, m_size.height);
        m_client->didInvalidateRect(damagedRect);
    }
}

void WebPopupMenuImpl::willEndLiveResize()
{
}

void WebPopupMenuImpl::animate(double)
{
}

void WebPopupMenuImpl::layout()
{
}

void WebPopupMenuImpl::paint(WebCanvas* canvas, const WebRect& rect)
{
    if (!m_widget)
        return;

    if (!rect.isEmpty())
        m_widget->paint(&GraphicsContextBuilder(canvas).context(), rect);
}

void WebPopupMenuImpl::themeChanged()
{
    notImplemented();
}

void WebPopupMenuImpl::composite(bool finish)
{
    notImplemented();
}

bool WebPopupMenuImpl::handleInputEvent(const WebInputEvent& inputEvent)
{
    if (!m_widget)
        return false;

    // TODO (jcampan): WebKit seems to always return false on mouse events
    // methods. For now we'll assume it has processed them (as we are only
    // interested in whether keyboard events are processed).
    switch (inputEvent.type) {
    case WebInputEvent::MouseMove:
        MouseMove(*static_cast<const WebMouseEvent*>(&inputEvent));
        return true;

    case WebInputEvent::MouseLeave:
        MouseLeave(*static_cast<const WebMouseEvent*>(&inputEvent));
        return true;

    case WebInputEvent::MouseWheel:
        MouseWheel(*static_cast<const WebMouseWheelEvent*>(&inputEvent));
        return true;

    case WebInputEvent::MouseDown:
        MouseDown(*static_cast<const WebMouseEvent*>(&inputEvent));
        return true;

    case WebInputEvent::MouseUp:
        MouseUp(*static_cast<const WebMouseEvent*>(&inputEvent));
        return true;

    // In Windows, RawKeyDown only has information about the physical key, but
    // for "selection", we need the information about the character the key
    // translated into. For English, the physical key value and the character
    // value are the same, hence, "selection" works for English. But for other
    // languages, such as Hebrew, the character value is different from the
    // physical key value. Thus, without accepting Char event type which
    // contains the key's character value, the "selection" won't work for
    // non-English languages, such as Hebrew.
    case WebInputEvent::RawKeyDown:
    case WebInputEvent::KeyDown:
    case WebInputEvent::KeyUp:
    case WebInputEvent::Char:
        return KeyEvent(*static_cast<const WebKeyboardEvent*>(&inputEvent));

    case WebInputEvent::TouchStart:
    case WebInputEvent::TouchMove:
    case WebInputEvent::TouchEnd:
    case WebInputEvent::TouchCancel:
        return TouchEvent(*static_cast<const WebTouchEvent*>(&inputEvent));

    case WebInputEvent::GestureScrollBegin:
    case WebInputEvent::GestureScrollEnd:
    case WebInputEvent::GestureTap:
        return GestureEvent(*static_cast<const WebGestureEvent*>(&inputEvent));

    case WebInputEvent::Undefined:
    case WebInputEvent::MouseEnter:
    case WebInputEvent::ContextMenu:
        return false;
    }
    return false;
}

void WebPopupMenuImpl::mouseCaptureLost()
{
}

void WebPopupMenuImpl::setFocus(bool enable)
{
}

void WebPopupMenu::setMinimumRowHeight(int minimumRowHeight)
{
    PopupMenuChromium::setMinimumRowHeight(minimumRowHeight);
}

bool WebPopupMenuImpl::setComposition(
    const WebString& text, const WebVector<WebCompositionUnderline>& underlines,
    int selectionStart, int selectionEnd)
{
    return false;
}

bool WebPopupMenuImpl::confirmComposition()
{
    return false;
}

bool WebPopupMenuImpl::confirmComposition(const WebString& text)
{
    return false;
}

bool WebPopupMenuImpl::compositionRange(size_t* location, size_t* length)
{
    *location = 0;
    *length = 0;
    return false;
}

WebTextInputType WebPopupMenuImpl::textInputType()
{
    return WebTextInputTypeNone;
}

WebRect WebPopupMenuImpl::caretOrSelectionBounds()
{
    return WebRect();
}

bool WebPopupMenuImpl::caretOrSelectionRange(size_t* location, size_t* length)
{
    *location = 0;
    *length = 0;
    return false;
}

void WebPopupMenuImpl::setTextDirection(WebTextDirection direction)
{
}


//-----------------------------------------------------------------------------
// WebCore::HostWindow

void WebPopupMenuImpl::invalidateContents(const IntRect&, bool)
{
    notImplemented();
}

void WebPopupMenuImpl::invalidateWindow(const IntRect&, bool)
{
    notImplemented();
}

void WebPopupMenuImpl::invalidateContentsAndWindow(const IntRect& paintRect, bool /*immediate*/)
{
    if (paintRect.isEmpty())
        return;
    if (m_client)
        m_client->didInvalidateRect(paintRect);
}

void WebPopupMenuImpl::invalidateContentsForSlowScroll(const IntRect& updateRect, bool immediate)
{
    invalidateContentsAndWindow(updateRect, immediate);
}

void WebPopupMenuImpl::scheduleAnimation()
{
}

void WebPopupMenuImpl::scroll(const IntSize& scrollDelta,
                              const IntRect& scrollRect,
                              const IntRect& clipRect)
{
    if (m_client) {
        int dx = scrollDelta.width();
        int dy = scrollDelta.height();
        m_client->didScrollRect(dx, dy, clipRect);
    }
}

IntPoint WebPopupMenuImpl::screenToWindow(const IntPoint& point) const
{
    notImplemented();
    return IntPoint();
}

IntRect WebPopupMenuImpl::windowToScreen(const IntRect& rect) const
{
    notImplemented();
    return IntRect();
}

void WebPopupMenuImpl::scrollRectIntoView(const IntRect&, const ScrollView*) const
{
    // Nothing to be done here since we do not have the concept of a container
    // that implements its own scrolling.
}

void WebPopupMenuImpl::scrollbarsModeDidChange() const
{
    // Nothing to be done since we have no concept of different scrollbar modes.
}

void WebPopupMenuImpl::setCursor(const WebCore::Cursor&)
{
}

void WebPopupMenuImpl::setCursorHiddenUntilMouseMoves(bool)
{
}

//-----------------------------------------------------------------------------
// WebCore::FramelessScrollViewClient

void WebPopupMenuImpl::popupClosed(FramelessScrollView* widget)
{
    ASSERT(widget == m_widget);
    if (m_widget) {
        m_widget->setClient(0);
        m_widget = 0;
    }
    if (m_client)
        m_client->closeWidgetSoon();
}

} // namespace WebKit
