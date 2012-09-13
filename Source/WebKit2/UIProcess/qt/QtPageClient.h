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
    virtual void setViewNeedsDisplay(const WebCore::IntRect&);
    virtual WebCore::IntSize viewSize();
    virtual bool isViewFocused();
    virtual bool isViewVisible();
    virtual void didReceiveMessageFromNavigatorQtObject(const String&);
    virtual void pageDidRequestScroll(const WebCore::IntPoint&);
    virtual void didChangeContentsSize(const WebCore::IntSize&);
    virtual void didChangeViewportProperties(const WebCore::ViewportAttributes&);
    virtual void processDidCrash();
    virtual void didRelaunchProcess();
    virtual PassOwnPtr<DrawingAreaProxy> createDrawingAreaProxy();
    virtual void handleDownloadRequest(DownloadProxy*);
    virtual void handleApplicationSchemeRequest(PassRefPtr<QtRefCountedNetworkRequestData>);
    virtual void handleAuthenticationRequiredRequest(const String& hostname, const String& realm, const String& prefilledUsername, String& username, String& password);
    virtual void handleCertificateVerificationRequest(const String& hostname, bool& ignoreErrors);
    virtual void handleProxyAuthenticationRequiredRequest(const String& hostname, uint16_t port, const String& prefilledUsername, String& username, String& password);

    virtual void displayView();
    virtual void scrollView(const WebCore::IntRect& scrollRect, const WebCore::IntSize& scrollOffset);
    virtual bool isViewWindowActive();
    virtual bool isViewInWindow();
#if USE(ACCELERATED_COMPOSITING)
    virtual void enterAcceleratedCompositingMode(const LayerTreeContext&);
    virtual void exitAcceleratedCompositingMode();
    virtual void updateAcceleratedCompositingMode(const LayerTreeContext&);
#endif // USE(ACCELERATED_COMPOSITING)
    virtual void pageClosed() { }
    virtual void startDrag(const WebCore::DragData&, PassRefPtr<ShareableBitmap> dragImage);
    virtual void setCursor(const WebCore::Cursor&);
    virtual void setCursorHiddenUntilMouseMoves(bool);
    virtual void toolTipChanged(const String&, const String&);

    // DefaultUndoController
    virtual void registerEditCommand(PassRefPtr<WebEditCommandProxy>, WebPageProxy::UndoOrRedo);
    virtual void clearAllEditCommands();
    virtual bool canUndoRedo(WebPageProxy::UndoOrRedo);
    virtual void executeUndoRedo(WebPageProxy::UndoOrRedo);

    virtual WebCore::FloatRect convertToDeviceSpace(const WebCore::FloatRect&);
    virtual WebCore::FloatRect convertToUserSpace(const WebCore::FloatRect&);
    virtual WebCore::IntPoint screenToWindow(const WebCore::IntPoint&);
    virtual WebCore::IntRect windowToScreen(const WebCore::IntRect&);
    virtual void doneWithKeyEvent(const NativeWebKeyboardEvent&, bool wasEventHandled) { }
    virtual PassRefPtr<WebPopupMenuProxy> createPopupMenuProxy(WebPageProxy*);
    virtual PassRefPtr<WebContextMenuProxy> createContextMenuProxy(WebPageProxy*);
#if ENABLE(INPUT_TYPE_COLOR)
    virtual PassRefPtr<WebColorChooserProxy> createColorChooserProxy(WebPageProxy*, const WebCore::Color& initialColor, const WebCore::IntRect&);
#endif
    virtual void setFindIndicator(PassRefPtr<FindIndicator>, bool fadeOut, bool animate) { }
    virtual void didCommitLoadForMainFrame(bool useCustomRepresentation) { }
    virtual void didFinishLoadingDataForCustomRepresentation(const String& suggestedFilename, const CoreIPC::DataReference&) { }
    virtual double customRepresentationZoomFactor() { return 1; }
    virtual void setCustomRepresentationZoomFactor(double) { }
    virtual void didChangeScrollbarsForMainFrame() const { }
    virtual void flashBackingStoreUpdates(const Vector<WebCore::IntRect>& updateRects);
    virtual void findStringInCustomRepresentation(const String&, WebKit::FindOptions, unsigned maxMatchCount) { }
    virtual void countStringMatchesInCustomRepresentation(const String&, WebKit::FindOptions, unsigned maxMatchCount) { }
    virtual void didFindZoomableArea(const WebCore::IntPoint&, const WebCore::IntRect&);
    virtual void updateTextInputState();
    virtual void doneWithGestureEvent(const WebGestureEvent&, bool wasEventHandled);
#if ENABLE(TOUCH_EVENTS)
    virtual void doneWithTouchEvent(const NativeWebTouchEvent&, bool wasEventHandled);
#endif

private:
    QQuickWebView* m_webView;
    QtWebPageEventHandler* m_eventHandler;
    DefaultUndoController* m_undoController;
};

} // namespace WebKit

#endif /* QtPageClient_h */
