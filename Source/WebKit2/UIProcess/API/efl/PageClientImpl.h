/*
 * Copyright (C) 2011 Samsung Electronics
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
#include <Evas.h>

namespace WebKit {

class PageClientImpl : public PageClient {
public:
    static PassOwnPtr<PageClientImpl> create(WebContext* context, WebPageGroup* pageGroup, Evas_Object* viewObject)
    {
        return adoptPtr(new PageClientImpl(context, pageGroup, viewObject));
    }
    ~PageClientImpl();

    Evas_Object* viewObject() const { return m_viewObject; }

    WebPageProxy* page() const { return m_page.get(); }

private:
    PageClientImpl(WebContext*, WebPageGroup*, Evas_Object*);

    // PageClient
    virtual PassOwnPtr<DrawingAreaProxy> createDrawingAreaProxy();
    virtual void setViewNeedsDisplay(const WebCore::IntRect&);
    virtual void displayView();
    virtual void scrollView(const WebCore::IntRect&, const WebCore::IntSize&);
    virtual WebCore::IntSize viewSize();
    virtual bool isViewWindowActive();
    virtual bool isViewFocused();
    virtual bool isViewVisible();
    virtual bool isViewInWindow();

    virtual void processDidCrash();
    virtual void didRelaunchProcess();
    virtual void pageClosed();

    virtual void toolTipChanged(const String&, const String&);

    virtual void setCursor(const WebCore::Cursor&);
    virtual void setCursorHiddenUntilMouseMoves(bool);
    virtual void setViewportArguments(const WebCore::ViewportArguments&);

    virtual void registerEditCommand(PassRefPtr<WebEditCommandProxy>, WebPageProxy::UndoOrRedo);
    virtual void clearAllEditCommands();
    virtual bool canUndoRedo(WebPageProxy::UndoOrRedo);
    virtual void executeUndoRedo(WebPageProxy::UndoOrRedo);
    virtual WebCore::FloatRect convertToDeviceSpace(const WebCore::FloatRect&);
    virtual WebCore::FloatRect convertToUserSpace(const WebCore::FloatRect&);
    virtual WebCore::IntPoint screenToWindow(const WebCore::IntPoint&);
    virtual WebCore::IntRect windowToScreen(const WebCore::IntRect&);

    virtual void doneWithKeyEvent(const NativeWebKeyboardEvent&, bool);

    virtual PassRefPtr<WebPopupMenuProxy> createPopupMenuProxy(WebPageProxy*);
    virtual PassRefPtr<WebContextMenuProxy> createContextMenuProxy(WebPageProxy*);

    virtual void setFindIndicator(PassRefPtr<FindIndicator>, bool);
#if USE(ACCELERATED_COMPOSITING)
    virtual void enterAcceleratedCompositingMode(const LayerTreeContext&);
    virtual void exitAcceleratedCompositingMode();
#endif

    virtual void didChangeScrollbarsForMainFrame() const;

    virtual void didCommitLoadForMainFrame(bool);
    virtual void didFinishLoadingDataForCustomRepresentation(const String&, const CoreIPC::DataReference&);
    virtual double customRepresentationZoomFactor();
    virtual void setCustomRepresentationZoomFactor(double);

    virtual void flashBackingStoreUpdates(const Vector<WebCore::IntRect>&);
    virtual void findStringInCustomRepresentation(const String&, FindOptions, unsigned);
    virtual void countStringMatchesInCustomRepresentation(const String&, FindOptions, unsigned);

    virtual float userSpaceScaleFactor() const;

private:
    RefPtr<WebPageProxy> m_page;
    Evas_Object* m_viewObject;
};

} // namespace WebKit

#endif // PageClientImpl_h
