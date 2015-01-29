/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef LayerTreeHostWPE_h
#define LayerTreeHostWPE_h

#include "LayerTreeContext.h"
#include "LayerTreeHost.h"
#include "TextureMapperLayer.h"
#include <WebCore/DisplayRefreshMonitor.h>
#include <WebCore/GLContext.h>
#include <WebCore/GraphicsLayerClient.h>
#include <WebCore/Timer.h>
#include <WebCore/WaylandSurfaceWPE.h>
#include <wayland-client.h>
#include <wtf/HashMap.h>
#include <wtf/gobject/GSourceWrap.h>

namespace WebKit {

class LayerTreeHostWPE : public LayerTreeHost, WebCore::GraphicsLayerClient {
public:
    static PassRefPtr<LayerTreeHostWPE> create(WebPage*);
    virtual ~LayerTreeHostWPE();

protected:
    explicit LayerTreeHostWPE(WebPage*);

    WebCore::GraphicsLayer* rootLayer() const { return m_rootLayer.get(); }

    void initialize();

    // LayerTreeHost.
    virtual void invalidate();
    virtual void sizeDidChange(const WebCore::IntSize& newSize);
    virtual void deviceOrPageScaleFactorChanged();
    virtual void forceRepaint();
    virtual void setRootCompositingLayer(WebCore::GraphicsLayer*);
    virtual void scheduleLayerFlush();
    virtual void setLayerFlushSchedulingEnabled(bool layerFlushingEnabled);
    virtual void pageBackgroundTransparencyChanged() override;

private:
    // LayerTreeHost.
    virtual const LayerTreeContext& layerTreeContext();
    virtual void setShouldNotifyAfterNextScheduledLayerFlush(bool);

    virtual void setNonCompositedContentsNeedDisplay() override;
    virtual void setNonCompositedContentsNeedDisplayInRect(const WebCore::IntRect&) override;
    virtual void scrollNonCompositedContents(const WebCore::IntRect& scrollRect);

    virtual bool flushPendingLayerChanges();

    virtual PassRefPtr<WebCore::DisplayRefreshMonitor> createDisplayRefreshMonitor(PlatformDisplayID) override;

    virtual void setViewOverlayRootLayer(WebCore::GraphicsLayer*) override { };

    // GraphicsLayerClient
    virtual void paintContents(const WebCore::GraphicsLayer*, WebCore::GraphicsContext&, WebCore::GraphicsLayerPaintingPhase, const WebCore::FloatRect& clipRect);

    enum CompositePurpose { ForResize, NotForResize };
    void compositeLayersToContext(CompositePurpose = NotForResize);

    void flushAndRenderLayers();
    void cancelPendingLayerFlush();

    static constexpr const double m_targetFlushFrequency = 60;
    void layerFlushTimerFired();

    WebCore::GLContext* glContext();

    LayerTreeContext m_layerTreeContext;
    bool m_isValid;
    bool m_notifyAfterScheduledLayerFlush;
    std::unique_ptr<WebCore::GraphicsLayer> m_rootLayer;
    std::unique_ptr<WebCore::GraphicsLayer> m_nonCompositedContentLayer;
    std::unique_ptr<WebCore::TextureMapper> m_textureMapper;
    std::unique_ptr<WebCore::WaylandSurface> m_waylandSurface;
    std::unique_ptr<WebCore::GLContext> m_context;
    bool m_layerFlushSchedulingEnabled;
    GSourceWrap::Static m_layerFlushTimer;

    static const struct wl_callback_listener m_frameListener;

    class DisplayRefreshMonitorWPE : public WebCore::DisplayRefreshMonitor {
    public:
        DisplayRefreshMonitorWPE();

        virtual bool requestRefreshCallback() override;
        void dispatchRefreshCallback();
    };
    RefPtr<DisplayRefreshMonitorWPE> m_displayRefreshMonitor;
};

} // namespace WebKit

#endif // LayerTreeHostWPE_h
