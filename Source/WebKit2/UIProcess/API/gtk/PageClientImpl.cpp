/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
 * Copyright (C) 2011 Igalia S.L.
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
#include "PageClientImpl.h"

#include "DrawingAreaProxyImpl.h"
#include "NativeWebKeyboardEvent.h"
#include "NativeWebMouseEvent.h"
#include "NotImplemented.h"
#include "WebContext.h"
#include "WebContextMenuProxyGtk.h"
#include "WebEventFactory.h"
#include "WebKitWebViewBasePrivate.h"
#include "WebPageProxy.h"
#include "WebPopupMenuProxyGtk.h"
#include <WebCore/Cursor.h>
#include <WebCore/EventNames.h>
#include <WebCore/GtkUtilities.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

using namespace WebCore;

namespace WebKit {

PageClientImpl::PageClientImpl(GtkWidget* viewWidget)
    : m_viewWidget(viewWidget)
{
}

void PageClientImpl::getEditorCommandsForKeyEvent(const NativeWebKeyboardEvent& event, const AtomicString& eventType, Vector<WTF::String>& commandList)
{
    ASSERT(eventType == eventNames().keydownEvent || eventType == eventNames().keypressEvent);

    KeyBindingTranslator::EventType type = eventType == eventNames().keydownEvent ?
        KeyBindingTranslator::KeyDown : KeyBindingTranslator::KeyPress;
    m_keyBindingTranslator.getEditorCommandsForKeyEvent(const_cast<GdkEventKey*>(&event.nativeEvent()->key), type, commandList);
}

// PageClient's pure virtual functions
std::unique_ptr<DrawingAreaProxy> PageClientImpl::createDrawingAreaProxy()
{
    return std::make_unique<DrawingAreaProxyImpl>(webkitWebViewBaseGetPage(WEBKIT_WEB_VIEW_BASE(m_viewWidget)));
}

void PageClientImpl::setViewNeedsDisplay(const WebCore::IntRect& rect)
{
    gtk_widget_queue_draw_area(m_viewWidget, rect.x(), rect.y(), rect.width(), rect.height());
}

void PageClientImpl::displayView()
{
    notImplemented();
}

void PageClientImpl::scrollView(const WebCore::IntRect& scrollRect, const WebCore::IntSize& /* scrollOffset */)
{
    setViewNeedsDisplay(scrollRect);
}

void PageClientImpl::requestScroll(const WebCore::FloatPoint&, bool)
{
    notImplemented();
}

WebCore::IntSize PageClientImpl::viewSize()
{
    if (!gtk_widget_get_realized(m_viewWidget))
        return IntSize();
    GtkAllocation allocation;
    gtk_widget_get_allocation(m_viewWidget, &allocation);
    return IntSize(allocation.width, allocation.height);
}

bool PageClientImpl::isViewWindowActive()
{
    return webkitWebViewBaseIsInWindowActive(WEBKIT_WEB_VIEW_BASE(m_viewWidget));
}

bool PageClientImpl::isViewFocused()
{
    return webkitWebViewBaseIsFocused(WEBKIT_WEB_VIEW_BASE(m_viewWidget));
}

bool PageClientImpl::isViewVisible()
{
    return webkitWebViewBaseIsVisible(WEBKIT_WEB_VIEW_BASE(m_viewWidget));
}

bool PageClientImpl::isViewInWindow()
{
    return webkitWebViewBaseIsInWindow(WEBKIT_WEB_VIEW_BASE(m_viewWidget));
}

void PageClientImpl::PageClientImpl::processDidExit()
{
    notImplemented();
}

void PageClientImpl::didRelaunchProcess()
{
    notImplemented();
}

void PageClientImpl::toolTipChanged(const String&, const String& newToolTip)
{
    webkitWebViewBaseSetTooltipText(WEBKIT_WEB_VIEW_BASE(m_viewWidget), newToolTip.utf8().data());
}

void PageClientImpl::setCursor(const Cursor& cursor)
{
    if (!gtk_widget_get_realized(m_viewWidget))
        return;

    // [GTK] Widget::setCursor() gets called frequently
    // http://bugs.webkit.org/show_bug.cgi?id=16388
    // Setting the cursor may be an expensive operation in some backends,
    // so don't re-set the cursor if it's already set to the target value.
    GdkWindow* window = gtk_widget_get_window(m_viewWidget);
    GdkCursor* currentCursor = gdk_window_get_cursor(window);
    GdkCursor* newCursor = cursor.platformCursor().get();
    if (currentCursor != newCursor)
        gdk_window_set_cursor(window, newCursor);
}

void PageClientImpl::setCursorHiddenUntilMouseMoves(bool /* hiddenUntilMouseMoves */)
{
    notImplemented();
}

void PageClientImpl::didChangeViewportProperties(const WebCore::ViewportAttributes&)
{
    notImplemented();
}

void PageClientImpl::registerEditCommand(PassRefPtr<WebEditCommandProxy> command, WebPageProxy::UndoOrRedo undoOrRedo)
{
    m_undoController.registerEditCommand(command, undoOrRedo);
}

void PageClientImpl::clearAllEditCommands()
{
    m_undoController.clearAllEditCommands();
}

bool PageClientImpl::canUndoRedo(WebPageProxy::UndoOrRedo undoOrRedo)
{
    return m_undoController.canUndoRedo(undoOrRedo);
}

void PageClientImpl::executeUndoRedo(WebPageProxy::UndoOrRedo undoOrRedo)
{
    m_undoController.executeUndoRedo(undoOrRedo);
}

FloatRect PageClientImpl::convertToDeviceSpace(const FloatRect& viewRect)
{
    notImplemented();
    return viewRect;
}

FloatRect PageClientImpl::convertToUserSpace(const FloatRect& viewRect)
{
    notImplemented();
    return viewRect;
}

IntPoint PageClientImpl::screenToRootView(const IntPoint& point)
{
    IntPoint widgetPositionOnScreen = convertWidgetPointToScreenPoint(m_viewWidget, IntPoint());
    IntPoint result(point);
    result.move(-widgetPositionOnScreen.x(), -widgetPositionOnScreen.y());
    return result;
}

IntRect PageClientImpl::rootViewToScreen(const IntRect& rect)
{
    return IntRect(convertWidgetPointToScreenPoint(m_viewWidget, rect.location()), rect.size());
}

void PageClientImpl::doneWithKeyEvent(const NativeWebKeyboardEvent& event, bool wasEventHandled)
{
    if (wasEventHandled)
        return;
    if (event.isFakeEventForComposition())
        return;

    WebKitWebViewBase* webkitWebViewBase = WEBKIT_WEB_VIEW_BASE(m_viewWidget);
    webkitWebViewBaseForwardNextKeyEvent(webkitWebViewBase);
    gtk_main_do_event(event.nativeEvent());
}

PassRefPtr<WebPopupMenuProxy> PageClientImpl::createPopupMenuProxy(WebPageProxy* page)
{
    return WebPopupMenuProxyGtk::create(m_viewWidget, page);
}

PassRefPtr<WebContextMenuProxy> PageClientImpl::createContextMenuProxy(WebPageProxy* page)
{
    return WebContextMenuProxyGtk::create(m_viewWidget, page);
}

#if ENABLE(INPUT_TYPE_COLOR)
PassRefPtr<WebColorPicker> PageClientImpl::createColorPicker(WebPageProxy*, const WebCore::Color&, const WebCore::IntRect&)
{
    notImplemented();
    return 0;
}
#endif

void PageClientImpl::setFindIndicator(PassRefPtr<FindIndicator>, bool /* fadeOut */, bool /* animate */)
{
    notImplemented();
}

void PageClientImpl::enterAcceleratedCompositingMode(const LayerTreeContext&)
{
    notImplemented();
}

void PageClientImpl::exitAcceleratedCompositingMode()
{
    notImplemented();
}

void PageClientImpl::updateAcceleratedCompositingMode(const LayerTreeContext&)
{
    notImplemented();
}

void PageClientImpl::pageClosed()
{
    notImplemented();
}

void PageClientImpl::preferencesDidChange()
{
    notImplemented();
}

void PageClientImpl::updateTextInputState()
{
    webkitWebViewBaseUpdateTextInputState(WEBKIT_WEB_VIEW_BASE(m_viewWidget));
}

void PageClientImpl::startDrag(const WebCore::DragData& dragData, PassRefPtr<ShareableBitmap> dragImage)
{
    webkitWebViewBaseStartDrag(WEBKIT_WEB_VIEW_BASE(m_viewWidget), dragData, dragImage);
}

void PageClientImpl::handleDownloadRequest(DownloadProxy* download)
{
    webkitWebViewBaseHandleDownloadRequest(WEBKIT_WEB_VIEW_BASE(m_viewWidget), download);
}

void PageClientImpl::didCommitLoadForMainFrame(const String& /* mimeType */, bool /* useCustomContentProvider */ )
{
    webkitWebViewBaseResetClickCounter(WEBKIT_WEB_VIEW_BASE(m_viewWidget));
}

#if ENABLE(FULLSCREEN_API)
WebFullScreenManagerProxyClient& PageClientImpl::fullScreenManagerProxyClient()
{
    return *this;
}

void PageClientImpl::closeFullScreenManager()
{
    notImplemented();
}

bool PageClientImpl::isFullScreen()
{
    notImplemented();
    return false;
}

void PageClientImpl::enterFullScreen()
{
    if (!m_viewWidget)
        return;

    webkitWebViewBaseEnterFullScreen(WEBKIT_WEB_VIEW_BASE(m_viewWidget));
}

void PageClientImpl::exitFullScreen()
{
    if (!m_viewWidget)
        return;

    webkitWebViewBaseExitFullScreen(WEBKIT_WEB_VIEW_BASE(m_viewWidget));
}

void PageClientImpl::beganEnterFullScreen(const IntRect& /* initialFrame */, const IntRect& /* finalFrame */)
{
    notImplemented();
}

void PageClientImpl::beganExitFullScreen(const IntRect& /* initialFrame */, const IntRect& /* finalFrame */)
{
    notImplemented();
}

#endif // ENABLE(FULLSCREEN_API)

void PageClientImpl::doneWithTouchEvent(const NativeWebTouchEvent& event, bool wasEventHandled)
{
    if (wasEventHandled)
        return;

    // Emulate pointer events if unhandled.
    const GdkEvent* touchEvent = event.nativeEvent();

    if (!touchEvent->touch.emulating_pointer)
        return;

    GUniquePtr<GdkEvent> pointerEvent;

    if (touchEvent->type == GDK_TOUCH_UPDATE) {
        pointerEvent.reset(gdk_event_new(GDK_MOTION_NOTIFY));
        pointerEvent->motion.time = touchEvent->touch.time;
        pointerEvent->motion.x = touchEvent->touch.x;
        pointerEvent->motion.y = touchEvent->touch.y;
        pointerEvent->motion.x_root = touchEvent->touch.x_root;
        pointerEvent->motion.y_root = touchEvent->touch.y_root;
        pointerEvent->motion.state = touchEvent->touch.state | GDK_BUTTON1_MASK;
    } else {
        switch (touchEvent->type) {
        case GDK_TOUCH_END:
            pointerEvent.reset(gdk_event_new(GDK_BUTTON_RELEASE));
            pointerEvent->button.state = touchEvent->touch.state | GDK_BUTTON1_MASK;
            break;
        case GDK_TOUCH_BEGIN:
            pointerEvent.reset(gdk_event_new(GDK_BUTTON_PRESS));
            break;
        default:
            ASSERT_NOT_REACHED();
        }

        pointerEvent->button.button = 1;
        pointerEvent->button.time = touchEvent->touch.time;
        pointerEvent->button.x = touchEvent->touch.x;
        pointerEvent->button.y = touchEvent->touch.y;
        pointerEvent->button.x_root = touchEvent->touch.x_root;
        pointerEvent->button.y_root = touchEvent->touch.y_root;
    }

    gdk_event_set_device(pointerEvent.get(), gdk_event_get_device(touchEvent));
    gdk_event_set_source_device(pointerEvent.get(), gdk_event_get_source_device(touchEvent));
    pointerEvent->any.window = GDK_WINDOW(g_object_ref(touchEvent->any.window));
    pointerEvent->any.send_event = TRUE;

    gtk_widget_event(m_viewWidget, pointerEvent.get());
}

void PageClientImpl::didFinishLoadingDataForCustomContentProvider(const String&, const IPC::DataReference&)
{
}

void PageClientImpl::navigationGestureDidBegin()
{
}

void PageClientImpl::navigationGestureWillEnd(bool, WebBackForwardListItem&)
{
}

void PageClientImpl::navigationGestureDidEnd(bool, WebBackForwardListItem&)
{
}

void PageClientImpl::willRecordNavigationSnapshot(WebBackForwardListItem&)
{
}

} // namespace WebKit
