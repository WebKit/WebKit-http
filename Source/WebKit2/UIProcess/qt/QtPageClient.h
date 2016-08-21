/*
 * Copyright (C) 2010, 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef QtPageClient_h
#define QtPageClient_h

#include "FindIndicator.h"
#include "PageClient.h"

class QQuickWebView;

namespace WebKit {

class DrawingAreaProxy;
class LayerTreeContext;
class QtWebPageEventHandler;
class DefaultUndoController;
class ShareableBitmap;

class QtPageClient : public PageClient {
public:
    QtPageClient();
    ~QtPageClient();

    void initialize(QQuickWebView*, QtWebPageEventHandler*, WebKit::DefaultUndoController*);

    // QQuickWebView.
    void setViewNeedsDisplay(const WebCore::IntRect&) override;
    void didRenderFrame(const WebCore::IntSize& contentsSize, const WebCore::IntRect& coveredRect) override;
    WebCore::IntSize viewSize() override;
    bool isViewFocused() override;
    bool isViewVisible() override;
    void pageDidRequestScroll(const WebCore::IntPoint&) override;
    void didChangeContentsSize(const WebCore::IntSize&) override;
    void didChangeViewportProperties(const WebCore::ViewportAttributes&) override;
    void processDidCrash() override;
    void didRelaunchProcess() override;
    std::unique_ptr<DrawingAreaProxy> createDrawingAreaProxy() override;
    void handleDownloadRequest(DownloadProxy*) override;
    void handleApplicationSchemeRequest(PassRefPtr<QtRefCountedNetworkRequestData>) override;
    void handleAuthenticationRequiredRequest(const String& hostname, const String& realm, const String& prefilledUsername, String& username, String& password) override;
    void handleCertificateVerificationRequest(const String& hostname, bool& ignoreErrors) override;
    void handleProxyAuthenticationRequiredRequest(const String& hostname, uint16_t port, const String& prefilledUsername, String& username, String& password) override;

    void displayView() override;
    bool canScrollView() override { return false; }
    void scrollView(const WebCore::IntRect& scrollRect, const WebCore::IntSize& scrollOffset) override;
    bool isViewWindowActive() override;
    bool isViewInWindow() override;
    void enterAcceleratedCompositingMode(const LayerTreeContext&) override;
    void exitAcceleratedCompositingMode() override;
    void updateAcceleratedCompositingMode(const LayerTreeContext&) override;
    void pageClosed() override { }
    void preferencesDidChange() override { }
    void startDrag(const WebCore::DragData&, PassRefPtr<ShareableBitmap> dragImage) override;
    void setCursor(const WebCore::Cursor&) override;
    void setCursorHiddenUntilMouseMoves(bool) override;
    void toolTipChanged(const String&, const String&) override;

    // DefaultUndoController
    void registerEditCommand(PassRefPtr<WebEditCommandProxy>, WebPageProxy::UndoOrRedo) override;
    void clearAllEditCommands() override;
    bool canUndoRedo(WebPageProxy::UndoOrRedo) override;
    void executeUndoRedo(WebPageProxy::UndoOrRedo) override;

    WebCore::FloatRect convertToDeviceSpace(const WebCore::FloatRect&) override;
    WebCore::FloatRect convertToUserSpace(const WebCore::FloatRect&) override;
    WebCore::IntPoint screenToWindow(const WebCore::IntPoint&) override;
    WebCore::IntRect windowToScreen(const WebCore::IntRect&) override;
    void doneWithKeyEvent(const NativeWebKeyboardEvent&, bool wasEventHandled) override { }
    RefPtr<WebPopupMenuProxy> createPopupMenuProxy(WebPageProxy&) override;
    std::unique_ptr<WebContextMenuProxy> createContextMenuProxy(WebPageProxy&, const ContextMenuContextData&, const UserData&) override;
#if ENABLE(INPUT_TYPE_COLOR)
    virtual PassRefPtr<WebColorPicker> createColorPicker(WebPageProxy*, const WebCore::Color& initialColor, const WebCore::IntRect&);
#endif
    void pageTransitionViewportReady() override;
    void didFindZoomableArea(const WebCore::IntPoint&, const WebCore::IntRect&) override;
    void updateTextInputState() override;
    void handleWillSetInputMethodState() override;
#if ENABLE(GESTURE_EVENTS)
    void doneWithGestureEvent(const WebGestureEvent&, bool wasEventHandled) override;
#endif
#if ENABLE(TOUCH_EVENTS)
    void doneWithTouchEvent(const NativeWebTouchEvent&, bool wasEventHandled) override;
#endif

private:
    QQuickWebView* m_webView;
    QtWebPageEventHandler* m_eventHandler;
    DefaultUndoController* m_undoController;
};

} // namespace WebKit

#endif /* QtPageClient_h */
