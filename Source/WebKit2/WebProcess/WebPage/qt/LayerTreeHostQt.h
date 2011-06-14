/*
    Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef LayerTreeHostQt_h
#define LayerTreeHostQt_h

#include "LayerTreeContext.h"
#include "LayerTreeHost.h"
#include "Timer.h"
#include <WebCore/GraphicsLayerClient.h>
#include <wtf/OwnPtr.h>

namespace WebKit {

class UpdateInfo;
class WebPage;

class LayerTreeHostQt : public LayerTreeHost, WebCore::GraphicsLayerClient {
public:
    static PassRefPtr<LayerTreeHostQt> create(WebPage*);
    virtual ~LayerTreeHostQt();

    static bool supportsAcceleratedCompositing();

    virtual const LayerTreeContext& layerTreeContext() { return m_layerTreeContext; }
    virtual void setLayerFlushSchedulingEnabled(bool);
    virtual void scheduleLayerFlush();
    virtual void setShouldNotifyAfterNextScheduledLayerFlush(bool);
    virtual void setRootCompositingLayer(WebCore::GraphicsLayer*);
    virtual void invalidate();

    virtual void setNonCompositedContentsNeedDisplay(const WebCore::IntRect&);
    virtual void scrollNonCompositedContents(const WebCore::IntRect& scrollRect, const WebCore::IntSize& scrollOffset);
    virtual void forceRepaint();
    virtual void sizeDidChange(const WebCore::IntSize& newSize);

    virtual void didInstallPageOverlay();
    virtual void didUninstallPageOverlay();
    virtual void setPageOverlayNeedsDisplay(const WebCore::IntRect&);

    virtual void pauseRendering() { }
    virtual void resumeRendering() { }

protected:
    explicit LayerTreeHostQt(WebPage*);

private:
    // GraphicsLayerClient
    virtual void notifyAnimationStarted(const WebCore::GraphicsLayer*, double time);
    virtual void notifySyncRequired(const WebCore::GraphicsLayer*);
    virtual void paintContents(const WebCore::GraphicsLayer*, WebCore::GraphicsContext&, WebCore::GraphicsLayerPaintingPhase, const WebCore::IntRect& clipRect);
    virtual bool showDebugBorders() const;
    virtual bool showRepaintCounter() const;

    // LayerTreeHostQt
    void createPageOverlayLayer();
    void destroyPageOverlayLayer();
    bool flushPendingLayerChanges();
    void cancelPendingLayerFlush();
    void performScheduledLayerFlush();
    void sendLayersToUI();

    OwnPtr<WebCore::GraphicsLayer> m_rootLayer;

    // The layer which contains all non-composited content.
    OwnPtr<WebCore::GraphicsLayer> m_nonCompositedContentLayer;

    // The page overlay layer. Will be null if there's no page overlay.
    OwnPtr<WebCore::GraphicsLayer> m_pageOverlayLayer;

    bool m_notifyAfterScheduledLayerFlush;
    bool m_isValid;
    LayerTreeContext m_layerTreeContext;
    void layerFlushTimerFired(WebCore::Timer<LayerTreeHostQt>*);
    WebCore::Timer<LayerTreeHostQt> m_layerFlushTimer;
    bool m_layerFlushSchedulingEnabled;
};

}

#endif // LayerTreeHostQt_h
