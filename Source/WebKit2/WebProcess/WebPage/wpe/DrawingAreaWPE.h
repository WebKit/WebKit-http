/*
 * Copyright (C) 2015 Igalia S.L.
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

#ifndef DrawingAreaWPE_h
#define DrawingAreaWPE_h

#include "DrawingArea.h"

#include "LayerTreeHost.h"

namespace WebKit {

class DrawingAreaWPE : public DrawingArea {
public:
    DrawingAreaWPE(WebPage&, const WebPageCreationParameters&);
    virtual ~DrawingAreaWPE();

    void layerHostDidFlushLayers();

private:
    // DrawingArea
    virtual void setNeedsDisplay() override;
    virtual void setNeedsDisplayInRect(const WebCore::IntRect&) override;
    virtual void scroll(const WebCore::IntRect& scrollRect, const WebCore::IntSize& scrollDelta) override;

    virtual void pageBackgroundTransparencyChanged() override;
    virtual void forceRepaint() override;
    virtual bool forceRepaintAsync(uint64_t callbackID) override;
    virtual void setLayerTreeStateIsFrozen(bool) override;
    virtual bool layerTreeStateIsFrozen() const override { return m_layerTreeStateIsFrozen; }
    virtual LayerTreeHost* layerTreeHost() const override { return m_layerTreeHost.get(); }

    virtual void setPaintingEnabled(bool) override;
    virtual void updatePreferences(const WebPreferencesStore&) override;
    virtual void mainFrameContentSizeChanged(const WebCore::IntSize&) override;

    virtual WebCore::GraphicsLayerFactory* graphicsLayerFactory() override;
    virtual void setRootCompositingLayer(WebCore::GraphicsLayer*) override;
    virtual void scheduleCompositingLayerFlush() override;
    virtual void scheduleCompositingLayerFlushImmediately() override;

#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    virtual RefPtr<WebCore::DisplayRefreshMonitor> createDisplayRefreshMonitor(WebCore::PlatformDisplayID) override;
#endif

    virtual void attachViewOverlayGraphicsLayer(WebCore::Frame*, WebCore::GraphicsLayer*) override;

    virtual void updateBackingStoreState(uint64_t, bool, float, const WebCore::IntSize&, const WebCore::IntSize&) override;
    virtual void didUpdate() override;

    void enterAcceleratedCompositingMode(WebCore::GraphicsLayer*);

    // When true, we maintain the layer tree in its current state by not leaving accelerated compositing mode
    // and not scheduling layer flushes.
    bool m_layerTreeStateIsFrozen { false };

    // The layer tree host that handles accelerated compositing.
    RefPtr<LayerTreeHost> m_layerTreeHost;
};

} // namespace WebKit

#endif // DrawingAreaWPE_h
