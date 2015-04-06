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

#include "config.h"
#include "LayerTreeHostWPE.h"

#include "DrawingAreaImpl.h"
#include "TextureMapperGL.h"
#include "WebPage.h"
#include "WebProcess.h"
#include <GLES2/gl2.h>
#include <WebCore/FrameView.h>
#include <WebCore/GLContextEGL.h>
#include <WebCore/GraphicsLayerTextureMapper.h>
#include <WebCore/MainFrame.h>
#include <WebCore/Page.h>
#include <WebCore/Settings.h>
#include <WebCore/WaylandDisplayWPE.h>
#include <cstdlib>
#include <wtf/CurrentTime.h>

#include <stdio.h>

using namespace WebCore;

namespace WebKit {

PassRefPtr<LayerTreeHostWPE> LayerTreeHostWPE::create(WebPage* webPage)
{
    RefPtr<LayerTreeHostWPE> host = adoptRef(new LayerTreeHostWPE(webPage));
    host->initialize();
    return host.release();
}

LayerTreeHostWPE::LayerTreeHostWPE(WebPage* webPage)
    : LayerTreeHost(webPage)
    , m_isValid(true)
    , m_notifyAfterScheduledLayerFlush(false)
    , m_layerFlushSchedulingEnabled(true)
    , m_layerFlushTimer("[WebKit] layerFlushTimer", std::bind(&LayerTreeHostWPE::layerFlushTimerFired, this), G_PRIORITY_HIGH_IDLE + 20)
    , m_viewOverlayRootLayer(nullptr)
    , m_frameRequestState(FrameRequestState::Completed)
    , m_displayRefreshMonitor(adoptRef(new DisplayRefreshMonitorWPE))
{
}

GLContext* LayerTreeHostWPE::glContext()
{
    if (m_context)
        return m_context.get();

    if (!m_waylandSurface)
        return nullptr;

    m_context = m_waylandSurface->createGLContext();
    return m_context.get();
}

void LayerTreeHostWPE::initialize()
{
    m_rootLayer = GraphicsLayer::create(graphicsLayerFactory(), *this);
    m_rootLayer->setDrawsContent(false);
    m_rootLayer->setSize(m_webPage->size());

    // The non-composited contents are a child of the root layer.
    m_nonCompositedContentLayer = GraphicsLayer::create(graphicsLayerFactory(), *this);
    m_nonCompositedContentLayer->setDrawsContent(true);
    m_nonCompositedContentLayer->setContentsOpaque(m_webPage->drawsBackground() && !m_webPage->drawsTransparentBackground());
    m_nonCompositedContentLayer->setSize(m_webPage->size());
    if (m_webPage->corePage()->settings().acceleratedDrawingEnabled())
        m_nonCompositedContentLayer->setAcceleratesDrawing(true);

#if ENABLE(TREE_DEBUGGING)
    m_rootLayer->setName("LayerTreeHost root layer");
    m_nonCompositedContentLayer->setName("LayerTreeHost non-composited content");
#endif

    m_rootLayer->addChild(m_nonCompositedContentLayer.get());
    m_nonCompositedContentLayer->setNeedsDisplay();

    RELEASE_ASSERT(WaylandDisplay::instance());
    m_waylandSurface = WaylandDisplay::instance()->createSurface(m_webPage->size());
    if (!m_waylandSurface)
        return;
    WaylandDisplay::instance()->registerSurface(m_waylandSurface->surface());

    m_layerTreeContext.contextID = m_webPage->pageID();

    GLContext* context = glContext();
    if (!context)
        return;

    // The creation of the TextureMapper needs an active OpenGL context.
    context->makeContextCurrent();

    m_textureMapper = TextureMapper::create(TextureMapper::OpenGLMode);
    static_cast<TextureMapperGL*>(m_textureMapper.get())->setEnableEdgeDistanceAntialiasing(true);
    downcast<GraphicsLayerTextureMapper>(*m_rootLayer).layer().setTextureMapper(m_textureMapper.get());

    // FIXME: Cretae page olverlay layers. https://bugs.webkit.org/show_bug.cgi?id=131433.

    scheduleLayerFlush();
}

LayerTreeHostWPE::~LayerTreeHostWPE()
{
    ASSERT(!m_isValid);
    ASSERT(!m_rootLayer);
    cancelPendingLayerFlush();
}

const LayerTreeContext& LayerTreeHostWPE::layerTreeContext()
{
    return m_layerTreeContext;
}

void LayerTreeHostWPE::setShouldNotifyAfterNextScheduledLayerFlush(bool notifyAfterScheduledLayerFlush)
{
    m_notifyAfterScheduledLayerFlush = notifyAfterScheduledLayerFlush;
}

void LayerTreeHostWPE::setRootCompositingLayer(GraphicsLayer* graphicsLayer)
{
    m_nonCompositedContentLayer->removeAllChildren();

    // Add the accelerated layer tree hierarchy.
    if (graphicsLayer)
        m_nonCompositedContentLayer->addChild(graphicsLayer);

    scheduleLayerFlush();
}

void LayerTreeHostWPE::invalidate()
{
    ASSERT(m_isValid);

    // This can trigger destruction of GL objects so let's make sure that
    // we have the right active context
    if (m_context)
        m_context->makeContextCurrent();

    cancelPendingLayerFlush();
    m_rootLayer = nullptr;
    m_nonCompositedContentLayer = nullptr;
    m_textureMapper = nullptr;

    m_context = nullptr;
    m_isValid = false;
}

void LayerTreeHostWPE::setNonCompositedContentsNeedDisplay()
{
    m_nonCompositedContentLayer->setNeedsDisplay();
    scheduleLayerFlush();
}

void LayerTreeHostWPE::setNonCompositedContentsNeedDisplayInRect(const IntRect& rect)
{
    m_nonCompositedContentLayer->setNeedsDisplayInRect(rect);
    scheduleLayerFlush();
}

void LayerTreeHostWPE::scrollNonCompositedContents(const IntRect& scrollRect)
{
    setNonCompositedContentsNeedDisplayInRect(scrollRect);
}

void LayerTreeHostWPE::sizeDidChange(const IntSize& newSize)
{
    if (m_rootLayer->size() == newSize)
        return;
    m_rootLayer->setSize(newSize);
    m_waylandSurface->resize(newSize);

    // If the newSize exposes new areas of the non-composited content a setNeedsDisplay is needed
    // for those newly exposed areas.
    FloatSize oldSize = m_nonCompositedContentLayer->size();
    m_nonCompositedContentLayer->setSize(newSize);

    if (newSize.width() > oldSize.width()) {
        float height = std::min(static_cast<float>(newSize.height()), oldSize.height());
        m_nonCompositedContentLayer->setNeedsDisplayInRect(FloatRect(oldSize.width(), 0, newSize.width() - oldSize.width(), height));
    }

    if (newSize.height() > oldSize.height())
        m_nonCompositedContentLayer->setNeedsDisplayInRect(FloatRect(0, oldSize.height(), newSize.width(), newSize.height() - oldSize.height()));
    m_nonCompositedContentLayer->setNeedsDisplay();

    compositeLayersToContext(ForResize);
}

void LayerTreeHostWPE::deviceOrPageScaleFactorChanged()
{
    // Other layers learn of the scale factor change via WebPage::setDeviceScaleFactor.
    m_nonCompositedContentLayer->deviceOrPageScaleFactorChanged();
}

void LayerTreeHostWPE::forceRepaint()
{
    scheduleLayerFlush();
}

void LayerTreeHostWPE::paintContents(const GraphicsLayer* graphicsLayer, GraphicsContext& graphicsContext, GraphicsLayerPaintingPhase, const FloatRect& clipRect)
{
    if (graphicsLayer == m_nonCompositedContentLayer.get()) {
        m_webPage->drawRect(graphicsContext, enclosingIntRect(clipRect));
        return;
    }

    // FIXME: Draw page overlays. https://bugs.webkit.org/show_bug.cgi?id=131433.
}

void LayerTreeHostWPE::layerFlushTimerFired()
{
    if (m_frameRequestState != FrameRequestState::Completed) {
        m_frameRequestState = FrameRequestState::ScheduleLayerFlushOnCompletion;
        return;
    }

    flushAndRenderLayers();
}

bool LayerTreeHostWPE::flushPendingLayerChanges()
{
    m_rootLayer->flushCompositingStateForThisLayerOnly();
    m_nonCompositedContentLayer->flushCompositingStateForThisLayerOnly();

    if (!m_webPage->corePage()->mainFrame().view()->flushCompositingStateIncludingSubframes())
        return false;

    if (m_viewOverlayRootLayer)
        m_viewOverlayRootLayer->flushCompositingState(FloatRect(FloatPoint(), m_rootLayer->size()));

    downcast<GraphicsLayerTextureMapper>(*m_rootLayer).updateBackingStoreIncludingSubLayers();
    return true;
}

void LayerTreeHostWPE::compositeLayersToContext(CompositePurpose purpose)
{
    GLContext* context = glContext();
    if (!context || !context->makeContextCurrent())
        return;

    // The window size may be out of sync with the page size at this point, and getting
    // the viewport parameters incorrect, means that the content will be misplaced. Thus
    // we set the viewport parameters directly from the window size.
    IntSize contextSize = context->defaultFrameBufferSize();
    glViewport(0, 0, contextSize.width(), contextSize.height());

    if (purpose == ForResize) {
        glClearColor(1, 1, 1, 0);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    m_textureMapper->beginPainting();
    downcast<GraphicsLayerTextureMapper>(*m_rootLayer).layer().paint();
    m_textureMapper->endPainting();

    ASSERT(m_frameRequestState == FrameRequestState::Completed);
    m_frameRequestState = FrameRequestState::InProgress;
    wl_callback_add_listener(wl_surface_frame(m_waylandSurface->surface()), &m_frameListener, this);

    context->swapBuffers();
}

void LayerTreeHostWPE::flushAndRenderLayers()
{
    {
        RefPtr<LayerTreeHostWPE> protect(this);
        m_webPage->layoutIfNeeded();

        if (!m_isValid)
            return;
    }

    GLContext* context = glContext();
    if (!context || !context->makeContextCurrent())
        return;

    if (!flushPendingLayerChanges())
        return;

    // Our model is very simple. We always composite and render the tree immediately after updating it.
    compositeLayersToContext();

    if (m_notifyAfterScheduledLayerFlush) {
        // Let the drawing area know that we've done a flush of the layer changes.
        static_cast<DrawingAreaImpl*>(m_webPage->drawingArea())->layerHostDidFlushLayers();
        m_notifyAfterScheduledLayerFlush = false;
    }
}

void LayerTreeHostWPE::scheduleLayerFlush()
{
    if (!m_layerFlushSchedulingEnabled || m_layerFlushTimer.isScheduled())
        return;

    if (m_frameRequestState != FrameRequestState::Completed) {
        m_frameRequestState = FrameRequestState::ScheduleLayerFlushOnCompletion;
        return;
    }

    m_layerFlushTimer.schedule();
}

void LayerTreeHostWPE::setLayerFlushSchedulingEnabled(bool layerFlushingEnabled)
{
    if (m_layerFlushSchedulingEnabled == layerFlushingEnabled)
        return;

    m_layerFlushSchedulingEnabled = layerFlushingEnabled;

    if (m_layerFlushSchedulingEnabled) {
        scheduleLayerFlush();
        return;
    }

    cancelPendingLayerFlush();
}

void LayerTreeHostWPE::pageBackgroundTransparencyChanged()
{
    m_nonCompositedContentLayer->setContentsOpaque(m_webPage->drawsBackground() && !m_webPage->drawsTransparentBackground());
}

void LayerTreeHostWPE::cancelPendingLayerFlush()
{
    m_layerFlushTimer.cancel();
}

PassRefPtr<WebCore::DisplayRefreshMonitor> LayerTreeHostWPE::createDisplayRefreshMonitor(PlatformDisplayID)
{
    // FIXME: See LayerTreeHostWPE::DisplayRefreshMonitorWPE implementation.
    return m_displayRefreshMonitor;
}

void LayerTreeHostWPE::setViewOverlayRootLayer(WebCore::GraphicsLayer* viewOverlayRootLayer)
{
    m_viewOverlayRootLayer = viewOverlayRootLayer;
    if (m_viewOverlayRootLayer)
        m_rootLayer->addChild(m_viewOverlayRootLayer);
}

static void debugLayerTreeHostFPS()
{
    static double lastTime = currentTime();
    static unsigned frameCount = 0;

    double ct = currentTime();
    frameCount++;

    if (ct - lastTime >= 5.0) {
        fprintf(stderr, "LayerTreeHostWPE: frame callbacks %.2f FPS\n", frameCount / (ct - lastTime));
        lastTime = ct;
        frameCount = 0;
    }
}

const struct wl_callback_listener LayerTreeHostWPE::m_frameListener = {
    // frame
    [](void* data, struct wl_callback* callback, uint32_t) {
        static bool reportFPS = !!std::getenv("WPE_LAYER_TREE_HOST_FPS");
        if (reportFPS)
            debugLayerTreeHostFPS();

        wl_callback_destroy(callback);

        auto& layerTreeHost = *static_cast<LayerTreeHostWPE*>(data);
        layerTreeHost.m_displayRefreshMonitor->dispatchRefreshCallback();

        switch (layerTreeHost.m_frameRequestState) {
        case FrameRequestState::InProgress:
            layerTreeHost.m_frameRequestState = FrameRequestState::Completed;
            break;
        case FrameRequestState::ScheduleLayerFlushOnCompletion:
            ASSERT(!layerTreeHost.m_layerFlushTimer.isActive());
            layerTreeHost.m_frameRequestState = FrameRequestState::Completed;
            layerTreeHost.flushAndRenderLayers();
            break;
        case FrameRequestState::Completed:
            ASSERT_NOT_REACHED();
            break;
        }
    }
};

// FIXME: This is a dumb implementation of the DisplayRefreshMonitor class.
// It is specific to one LayerTreeHost object and doesn't properly use
// display IDs or perform cross-thread safety. But it works well enough
// at the moment, though it really needs additional work.
LayerTreeHostWPE::DisplayRefreshMonitorWPE::DisplayRefreshMonitorWPE()
    : DisplayRefreshMonitor(0)
{
}

bool LayerTreeHostWPE::DisplayRefreshMonitorWPE::requestRefreshCallback()
{
    setIsScheduled(true);
    return true;
}

void LayerTreeHostWPE::DisplayRefreshMonitorWPE::dispatchRefreshCallback()
{
    // We're currently dispatching this callback on main thread, so let's
    // go straight ahead to handling the refresh notifications.
    ASSERT(isMainThread());
    if (isScheduled())
        handleDisplayRefreshedNotificationOnMainThread(this);
}

} // namespace WebKit
