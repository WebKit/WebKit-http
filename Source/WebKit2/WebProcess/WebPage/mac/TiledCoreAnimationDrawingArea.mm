/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#import "config.h"
#import "TiledCoreAnimationDrawingArea.h"

#import "DrawingAreaProxyMessages.h"
#import "EventDispatcher.h"
#import "LayerTreeContext.h"
#import "RemoteLayerClient.h"
#import "WebPage.h"
#import "WebProcess.h"
#import <QuartzCore/QuartzCore.h>
#import <WebCore/Frame.h>
#import <WebCore/FrameView.h>
#import <WebCore/GraphicsContext.h>
#import <WebCore/Page.h>
#import <WebCore/ScrollingCoordinator.h>
#import <WebCore/Settings.h>

@interface CATransaction (Details)
+ (void)synchronize;
@end

using namespace WebCore;

namespace WebKit {

PassOwnPtr<TiledCoreAnimationDrawingArea> TiledCoreAnimationDrawingArea::create(WebPage* webPage, const WebPageCreationParameters& parameters)
{
    return adoptPtr(new TiledCoreAnimationDrawingArea(webPage, parameters));
}

TiledCoreAnimationDrawingArea::TiledCoreAnimationDrawingArea(WebPage* webPage, const WebPageCreationParameters& parameters)
    : DrawingArea(DrawingAreaTypeTiledCoreAnimation, webPage)
    , m_layerTreeStateIsFrozen(false)
    , m_layerFlushScheduler(this)
{
    Page* page = webPage->corePage();

    // FIXME: It's weird that we're mucking around with the settings here.
    page->settings()->setForceCompositingMode(true);

#if ENABLE(THREADED_SCROLLING)
    page->settings()->setScrollingCoordinatorEnabled(true);

    WebProcess::shared().eventDispatcher().addScrollingTreeForPage(webPage);
#endif

    m_rootLayer = [CALayer layer];

    CGRect rootLayerFrame = m_webPage->bounds();
    m_rootLayer.get().frame = rootLayerFrame;
    m_rootLayer.get().opaque = YES;
    m_rootLayer.get().geometryFlipped = YES;

    m_remoteLayerClient = RemoteLayerClient::create(WebProcess::shared().compositingRenderServerPort(), m_rootLayer.get());

    LayerTreeContext layerTreeContext;
    layerTreeContext.contextID = m_remoteLayerClient->clientID();
    m_webPage->send(Messages::DrawingAreaProxy::EnterAcceleratedCompositingMode(0, layerTreeContext));
}

TiledCoreAnimationDrawingArea::~TiledCoreAnimationDrawingArea()
{
#if ENABLE(THREADED_SCROLLING)
    WebProcess::shared().eventDispatcher().removeScrollingTreeForPage(m_webPage);
#endif

    m_layerFlushScheduler.invalidate();
}

void TiledCoreAnimationDrawingArea::setNeedsDisplay(const IntRect& rect)
{
}

void TiledCoreAnimationDrawingArea::scroll(const IntRect& scrollRect, const IntSize& scrollOffset)
{
}

void TiledCoreAnimationDrawingArea::setRootCompositingLayer(GraphicsLayer* graphicsLayer)
{
    CALayer *rootCompositingLayer = graphicsLayer ? graphicsLayer->platformLayer() : nil;

    if (m_layerTreeStateIsFrozen) {
        m_pendingRootCompositingLayer = rootCompositingLayer;
        return;
    }

    setRootCompositingLayer(rootCompositingLayer);
}

void TiledCoreAnimationDrawingArea::forceRepaint()
{
    if (m_layerTreeStateIsFrozen)
        return;

    flushLayers();
    [CATransaction flush];
    [CATransaction synchronize];
}

void TiledCoreAnimationDrawingArea::setLayerTreeStateIsFrozen(bool layerTreeStateIsFrozen)
{
    if (m_layerTreeStateIsFrozen == layerTreeStateIsFrozen)
        return;

    m_layerTreeStateIsFrozen = layerTreeStateIsFrozen;
    if (m_layerTreeStateIsFrozen)
        m_layerFlushScheduler.suspend();
    else
        m_layerFlushScheduler.resume();
}

bool TiledCoreAnimationDrawingArea::layerTreeStateIsFrozen() const
{
    return m_layerTreeStateIsFrozen;
}

void TiledCoreAnimationDrawingArea::scheduleCompositingLayerSync()
{
    m_layerFlushScheduler.schedule();
}

void TiledCoreAnimationDrawingArea::didInstallPageOverlay()
{
    createPageOverlayLayer();
    scheduleCompositingLayerSync();
}

void TiledCoreAnimationDrawingArea::didUninstallPageOverlay()
{
    destroyPageOverlayLayer();
    scheduleCompositingLayerSync();
}

void TiledCoreAnimationDrawingArea::setPageOverlayNeedsDisplay(const IntRect& rect)
{
    ASSERT(m_pageOverlayLayer);
    m_pageOverlayLayer->setNeedsDisplayInRect(rect);
    scheduleCompositingLayerSync();
}

void TiledCoreAnimationDrawingArea::notifyAnimationStarted(const GraphicsLayer*, double)
{
}

void TiledCoreAnimationDrawingArea::notifySyncRequired(const GraphicsLayer*)
{
}

void TiledCoreAnimationDrawingArea::paintContents(const GraphicsLayer* graphicsLayer, GraphicsContext& graphicsContext, GraphicsLayerPaintingPhase, const IntRect& clipRect)
{
    ASSERT_UNUSED(graphicsLayer, graphicsLayer == m_pageOverlayLayer);

    m_webPage->drawPageOverlay(graphicsContext, clipRect);
}

bool TiledCoreAnimationDrawingArea::showDebugBorders(const GraphicsLayer*) const
{
    return m_webPage->corePage()->settings()->showDebugBorders();
}

bool TiledCoreAnimationDrawingArea::showRepaintCounter(const GraphicsLayer*) const
{
    return m_webPage->corePage()->settings()->showRepaintCounter();
}

float TiledCoreAnimationDrawingArea::deviceScaleFactor() const
{
    return m_webPage->corePage()->deviceScaleFactor();
}

bool TiledCoreAnimationDrawingArea::flushLayers()
{
    ASSERT(!m_layerTreeStateIsFrozen);

    // This gets called outside of the normal event loop so wrap in an autorelease pool
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    m_webPage->layoutIfNeeded();

    if (m_pendingRootCompositingLayer) {
        setRootCompositingLayer(m_pendingRootCompositingLayer.get());
        m_pendingRootCompositingLayer = nullptr;
    }

    if (m_pageOverlayLayer)
        m_pageOverlayLayer->syncCompositingStateForThisLayerOnly();

    bool returnValue = m_webPage->corePage()->mainFrame()->view()->syncCompositingStateIncludingSubframes();

    [pool drain];
    return returnValue;
}

void TiledCoreAnimationDrawingArea::updateGeometry(const IntSize& viewSize)
{
    m_webPage->setSize(viewSize);
    m_webPage->layoutIfNeeded();

    if (m_pageOverlayLayer)
        m_pageOverlayLayer->setSize(viewSize);

    if (!m_layerTreeStateIsFrozen)
        flushLayers();

    [CATransaction begin];
    [CATransaction setDisableActions:YES];

    m_rootLayer.get().frame = CGRectMake(0, 0, viewSize.width(), viewSize.height());

    [CATransaction commit];
    
    [CATransaction flush];
    [CATransaction synchronize];

    m_webPage->send(Messages::DrawingAreaProxy::DidUpdateGeometry());
}

void TiledCoreAnimationDrawingArea::setDeviceScaleFactor(float deviceScaleFactor)
{
    m_webPage->setDeviceScaleFactor(deviceScaleFactor);
}

void TiledCoreAnimationDrawingArea::setRootCompositingLayer(CALayer *layer)
{
    ASSERT(!m_layerTreeStateIsFrozen);

    [CATransaction begin];
    [CATransaction setDisableActions:YES];

    if (!layer)
        m_rootLayer.get().sublayers = nil;
    else {
        m_rootLayer.get().sublayers = [NSArray arrayWithObject:layer];

        if (m_pageOverlayLayer)
            [m_rootLayer.get() addSublayer:m_pageOverlayLayer->platformLayer()];
    }

    [CATransaction commit];
}

void TiledCoreAnimationDrawingArea::createPageOverlayLayer()
{
    ASSERT(!m_pageOverlayLayer);

    m_pageOverlayLayer = GraphicsLayer::create(this);
#ifndef NDEBUG
    m_pageOverlayLayer->setName("page overlay content");
#endif

    m_pageOverlayLayer->setDrawsContent(true);
    m_pageOverlayLayer->setSize(m_webPage->size());

    [m_rootLayer.get() addSublayer:m_pageOverlayLayer->platformLayer()];
}

void TiledCoreAnimationDrawingArea::destroyPageOverlayLayer()
{
    ASSERT(m_pageOverlayLayer);

    [m_pageOverlayLayer->platformLayer() removeFromSuperlayer];
    m_pageOverlayLayer = nullptr;
}

} // namespace WebKit
