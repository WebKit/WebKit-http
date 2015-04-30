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

namespace WKWPE {
class View;
}

namespace WebKit {

class PageClientImpl final : public PageClient {
public:
    PageClientImpl(WKWPE::View&);

private:
    // PageClient
    virtual std::unique_ptr<DrawingAreaProxy> createDrawingAreaProxy() override;
    virtual void setViewNeedsDisplay(const WebCore::IntRect&) override;
    virtual void displayView() override;
    virtual bool canScrollView() override { return false; }
    virtual void scrollView(const WebCore::IntRect&, const WebCore::IntSize&) override;
    virtual void requestScroll(const WebCore::FloatPoint&, bool) override;
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
    virtual void doneWithTouchEvent(const NativeWebTouchEvent&, bool) override;

    virtual PassRefPtr<WebPopupMenuProxy> createPopupMenuProxy(WebPageProxy*) override;
    virtual PassRefPtr<WebContextMenuProxy> createContextMenuProxy(WebPageProxy*) override;

    virtual void setTextIndicator(Ref<WebCore::TextIndicator>, WebCore::TextIndicatorLifetime) override;
    virtual void clearTextIndicator(WebCore::TextIndicatorDismissalAnimation) override;
    virtual void setTextIndicatorAnimationProgress(float) override;

    virtual void enterAcceleratedCompositingMode(const LayerTreeContext&) override;
    virtual void exitAcceleratedCompositingMode() override;
    virtual void updateAcceleratedCompositingMode(const LayerTreeContext&) override;

    virtual void didFinishLoadingDataForCustomContentProvider(const String&, const IPC::DataReference&) override;

    virtual void navigationGestureDidBegin() override;
    virtual void navigationGestureWillEnd(bool, WebBackForwardListItem&) override;
    virtual void navigationGestureDidEnd(bool, WebBackForwardListItem&) override;
    virtual void willRecordNavigationSnapshot(WebBackForwardListItem&) override;

    virtual void didFirstVisuallyNonEmptyLayoutForMainFrame() override;
    virtual void didFinishLoadForMainFrame() override;
    virtual void didSameDocumentNavigationForMainFrame(SameDocumentNavigationType) override;

    virtual void didChangeBackgroundColor() override;

    WKWPE::View& m_view;
};

} // namespace WebKit

#endif // PageClientImpl_h
