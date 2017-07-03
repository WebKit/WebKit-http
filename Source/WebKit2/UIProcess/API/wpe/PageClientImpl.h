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

#ifndef PageClientImpl_h
#define PageClientImpl_h

#include "PageClient.h"
#include "WebFullScreenManagerProxy.h"

namespace WKWPE {
class View;
}

namespace WebKit {

class PageClientImpl final : public PageClient 
#if ENABLE(FULLSCREEN_API)
    , public WebFullScreenManagerProxyClient
#endif

{
public:
    PageClientImpl(WKWPE::View&);

private:
    // PageClient
    virtual std::unique_ptr<DrawingAreaProxy> createDrawingAreaProxy() override;
    virtual void setViewNeedsDisplay(const WebCore::Region&) override;
    virtual void requestScroll(const WebCore::FloatPoint&, const WebCore::IntPoint&, bool) override;
    virtual WebCore::IntSize viewSize() override;
    virtual bool isViewWindowActive() override;
    virtual bool isViewFocused() override;
    virtual bool isViewVisible() override;
    virtual bool isViewInWindow() override;

    virtual void processDidExit() override;
    virtual void didRelaunchProcess() override;
    virtual void pageClosed() override;
    virtual void preferencesDidChange() override;
    virtual void toolTipChanged(const String&, const String&) override;

    virtual void didCommitLoadForMainFrame(const String&, bool) override;
    virtual void handleDownloadRequest(DownloadProxy*) override;

    virtual void didChangeContentSize(const WebCore::IntSize&) override;

    virtual void setCursor(const WebCore::Cursor&) override;
    virtual void setCursorHiddenUntilMouseMoves(bool) override;
    virtual void didChangeViewportProperties(const WebCore::ViewportAttributes&) override;

    virtual void registerEditCommand(PassRefPtr<WebEditCommandProxy>, WebPageProxy::UndoOrRedo) override;
    virtual void clearAllEditCommands() override;
    virtual bool canUndoRedo(WebPageProxy::UndoOrRedo) override;
    virtual void executeUndoRedo(WebPageProxy::UndoOrRedo) override;

    virtual WebCore::FloatRect convertToDeviceSpace(const WebCore::FloatRect&) override;
    virtual WebCore::FloatRect convertToUserSpace(const WebCore::FloatRect&) override;
    virtual WebCore::IntPoint screenToRootView(const WebCore::IntPoint&) override;
    virtual WebCore::IntRect rootViewToScreen(const WebCore::IntRect&) override;

    virtual void doneWithKeyEvent(const NativeWebKeyboardEvent&, bool) override;
#if ENABLE(TOUCH_EVENTS)
    virtual void doneWithTouchEvent(const NativeWebTouchEvent&, bool) override;
#endif
    virtual void wheelEventWasNotHandledByWebCore(const NativeWebWheelEvent&) override;

    virtual RefPtr<WebPopupMenuProxy> createPopupMenuProxy(WebPageProxy&) override;
#if ENABLE(CONTEXT_MENUS)
    virtual std::unique_ptr<WebContextMenuProxy> createContextMenuProxy(WebPageProxy&, const ContextMenuContextData&, const UserData&) override;
#endif

    virtual void enterAcceleratedCompositingMode(const LayerTreeContext&) override;
    virtual void exitAcceleratedCompositingMode() override;
    virtual void updateAcceleratedCompositingMode(const LayerTreeContext&) override;

    virtual void didFinishLoadingDataForCustomContentProvider(const String&, const IPC::DataReference&) override;

    virtual void navigationGestureDidBegin() override;
    virtual void navigationGestureWillEnd(bool, WebBackForwardListItem&) override;
    virtual void navigationGestureDidEnd(bool, WebBackForwardListItem&) override;
    virtual void navigationGestureDidEnd() override;
    virtual void willRecordNavigationSnapshot(WebBackForwardListItem&) override;
    virtual void didRemoveNavigationGestureSnapshot() override;

    virtual void didFirstVisuallyNonEmptyLayoutForMainFrame() override;
    virtual void didFinishLoadForMainFrame() override;
    virtual void didFailLoadForMainFrame() override;
    virtual void didSameDocumentNavigationForMainFrame(SameDocumentNavigationType) override;

    virtual void didChangeBackgroundColor() override;

    virtual void refView() override;
    virtual void derefView() override;

#if ENABLE(VIDEO) && USE(GSTREAMER)
    virtual bool decidePolicyForInstallMissingMediaPluginsPermissionRequest(InstallMissingMediaPluginsPermissionRequest&) override;
#endif

    virtual void didRestoreScrollPosition() override;

    virtual WebCore::UserInterfaceLayoutDirection userInterfaceLayoutDirection() override;

#if ENABLE(FULLSCREEN_API)
    virtual WebFullScreenManagerProxyClient& fullScreenManagerProxyClient() final;
    virtual void closeFullScreenManager() override;
    virtual bool isFullScreen() override;
    virtual void enterFullScreen() override;
    virtual void exitFullScreen() override;
    virtual void beganEnterFullScreen(const WebCore::IntRect& initialFrame, const WebCore::IntRect& finalFrame) override;
    virtual void beganExitFullScreen(const WebCore::IntRect& initialFrame, const WebCore::IntRect& finalFrame) override;
#endif

    WKWPE::View& m_view;
};

} // namespace WebKit

#endif // PageClientImpl_h
