/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "cc/CCLayerTreeHost.h"

#include "LayerChromium.h"
#include "LayerPainterChromium.h"
#include "LayerRendererChromium.h"
#include "TraceEvent.h"
#include "TreeSynchronizer.h"
#include "cc/CCLayerTreeHostCommon.h"
#include "cc/CCLayerTreeHostImpl.h"
#include "cc/CCSingleThreadProxy.h"
#include "cc/CCThread.h"
#include "cc/CCThreadProxy.h"

namespace WebCore {

PassRefPtr<CCLayerTreeHost> CCLayerTreeHost::create(CCLayerTreeHostClient* client, PassRefPtr<LayerChromium> rootLayer, const CCSettings& settings)
{
    RefPtr<CCLayerTreeHost> layerTreeHost = adoptRef(new CCLayerTreeHost(client, rootLayer, settings));
    if (!layerTreeHost->initialize())
        return 0;
    return layerTreeHost;
}

CCLayerTreeHost::CCLayerTreeHost(CCLayerTreeHostClient* client, PassRefPtr<LayerChromium> rootLayer, const CCSettings& settings)
    : m_animating(false)
    , m_client(client)
    , m_frameNumber(0)
    , m_rootLayer(rootLayer)
    , m_settings(settings)
    , m_zoomAnimatorScale(1)
    , m_visible(true)
{
}

bool CCLayerTreeHost::initialize()
{
    if (m_settings.enableCompositorThread) {
        // Accelerated Painting is not supported in threaded mode. Turn it off.
        m_settings.acceleratePainting = false;
        m_proxy = CCThreadProxy::create(this);
    } else
        m_proxy = CCSingleThreadProxy::create(this);
    m_proxy->start();

    if (!m_proxy->initializeLayerRenderer())
        return false;

    // Update m_settings based on capabilities that we got back from the renderer.
    m_settings.acceleratePainting = m_proxy->layerRendererCapabilities().usingAcceleratedPainting;

    // We changed the root layer. Tell the proxy a commit is needed.
    m_proxy->setNeedsCommitAndRedraw();

    m_contentsTextureManager = TextureManager::create(TextureManager::highLimitBytes(), m_proxy->layerRendererCapabilities().maxTextureSize);
    return true;
}

CCLayerTreeHost::~CCLayerTreeHost()
{
    ASSERT(CCProxy::isMainThread());
    TRACE_EVENT("CCLayerTreeHost::~CCLayerTreeHost", this, 0);
    m_proxy->stop();
    m_proxy.clear();
    clearPendingUpdate();
    ASSERT(!m_contentsTextureManager || !m_contentsTextureManager->currentMemoryUseBytes());
    m_contentsTextureManager.clear();
}

void CCLayerTreeHost::deleteContentsTextures(GraphicsContext3D* context)
{
    ASSERT(CCProxy::isImplThread());
    if (m_contentsTextureManager)
        m_contentsTextureManager->evictAndDeleteAllTextures(context);
}

void CCLayerTreeHost::animateAndLayout(double frameBeginTime)
{
    m_animating = true;
    m_client->animateAndLayout(frameBeginTime);
    m_animating = false;
}

void CCLayerTreeHost::commitTo(CCLayerTreeHostImpl* hostImpl)
{
    ASSERT(CCProxy::isImplThread());
    TRACE_EVENT("CCLayerTreeHost::commitTo", this, 0);
    hostImpl->setSourceFrameNumber(frameNumber());

    // FIXME: Temporary diagnostic for crbug 96719. This shouldn't happen.
    if (!contentsTextureManager())
        CRASH();

    contentsTextureManager()->reduceMemoryToLimit(TextureManager::reclaimLimitBytes());
    contentsTextureManager()->deleteEvictedTextures(hostImpl->context());

    updateCompositorResources(m_updateList, hostImpl->context());
    clearPendingUpdate();

    hostImpl->setVisible(m_visible);
    hostImpl->setZoomAnimatorScale(m_zoomAnimatorScale);
    hostImpl->setViewport(viewportSize());

    hostImpl->layerRenderer()->setContentsTextureMemoryUseBytes(m_contentsTextureManager->currentMemoryUseBytes());
    m_contentsTextureManager->unprotectAllTextures();

    // Synchronize trees, if one exists at all...
    if (rootLayer())
        hostImpl->setRootLayer(TreeSynchronizer::synchronizeTrees(rootLayer(), hostImpl->rootLayer()));
    else
        hostImpl->setRootLayer(0);

    m_frameNumber++;
}

PassOwnPtr<CCThread> CCLayerTreeHost::createCompositorThread()
{
    return m_client->createCompositorThread();
}

PassRefPtr<GraphicsContext3D> CCLayerTreeHost::createLayerTreeHostContext3D()
{
    return m_client->createLayerTreeHostContext3D();
}

PassOwnPtr<CCLayerTreeHostImpl> CCLayerTreeHost::createLayerTreeHostImpl()
{
    return CCLayerTreeHostImpl::create(m_settings);
}

void CCLayerTreeHost::didRecreateGraphicsContext(bool success)
{
    m_contentsTextureManager->evictAndDeleteAllTextures(0);

    if (rootLayer())
        rootLayer()->cleanupResourcesRecursive();
    m_client->didRecreateGraphicsContext(success);
}

#if !USE(THREADED_COMPOSITING)
void CCLayerTreeHost::scheduleComposite()
{
    m_client->scheduleComposite();
}
#endif

// Temporary hack until WebViewImpl context creation gets simplified
GraphicsContext3D* CCLayerTreeHost::context()
{
    ASSERT(!m_settings.enableCompositorThread);
    return m_proxy->context();
}

bool CCLayerTreeHost::compositeAndReadback(void *pixels, const IntRect& rect)
{
    return m_proxy->compositeAndReadback(pixels, rect);
}

void CCLayerTreeHost::finishAllRendering()
{
    m_proxy->finishAllRendering();
}

const LayerRendererCapabilities& CCLayerTreeHost::layerRendererCapabilities() const
{
    return m_proxy->layerRendererCapabilities();
}

void CCLayerTreeHost::setZoomAnimatorScale(double zoom)
{
    bool zoomChanged = m_zoomAnimatorScale != zoom;

    m_zoomAnimatorScale = zoom;

    if (zoomChanged)
        setNeedsCommitAndRedraw();
}

void CCLayerTreeHost::setNeedsCommitAndRedraw()
{
#if USE(THREADED_COMPOSITING)
    TRACE_EVENT("CCLayerTreeHost::setNeedsRedraw", this, 0);
    m_proxy->setNeedsCommitAndRedraw();
#else
    m_client->scheduleComposite();
#endif
}

void CCLayerTreeHost::setNeedsRedraw()
{
#if USE(THREADED_COMPOSITING)
    TRACE_EVENT("CCLayerTreeHost::setNeedsRedraw", this, 0);
    m_proxy->setNeedsRedraw();
#else
    m_client->scheduleComposite();
#endif
}

void CCLayerTreeHost::setViewport(const IntSize& viewportSize)
{
    m_viewportSize = viewportSize;
    setNeedsCommitAndRedraw();
}

void CCLayerTreeHost::setVisible(bool visible)
{
    m_visible = visible;
    if (visible)
        m_proxy->setNeedsCommitAndRedraw();
    else {
        m_contentsTextureManager->reduceMemoryToLimit(TextureManager::lowLimitBytes());
        m_contentsTextureManager->unprotectAllTextures();
        m_proxy->setNeedsCommit();
    }
}

void CCLayerTreeHost::loseCompositorContext(int numTimes)
{
    m_proxy->loseCompositorContext(numTimes);
}

TextureManager* CCLayerTreeHost::contentsTextureManager() const
{
    return m_contentsTextureManager.get();
}

#if !USE(THREADED_COMPOSITING)
void CCLayerTreeHost::composite()
{
    ASSERT(!m_settings.enableCompositorThread);
    static_cast<CCSingleThreadProxy*>(m_proxy.get())->compositeImmediately();
}
#endif // !USE(THREADED_COMPOSITING)

void CCLayerTreeHost::updateLayers()
{
    if (!rootLayer())
        return;

    if (viewportSize().isEmpty())
        return;

    updateLayers(rootLayer());
}

void CCLayerTreeHost::updateLayers(LayerChromium* rootLayer)
{
    TRACE_EVENT("CCLayerTreeHost::updateLayers", this, 0);

    if (!rootLayer->renderSurface())
        rootLayer->createRenderSurface();
    rootLayer->renderSurface()->setContentRect(IntRect(IntPoint(0, 0), viewportSize()));

    IntRect rootScissorRect(IntPoint(), viewportSize());
    rootLayer->setScissorRect(rootScissorRect);

    // This assert fires if updateCompositorResources wasn't called after
    // updateLayers. Only one update can be pending at any given time.
    ASSERT(!m_updateList.size());
    m_updateList.append(rootLayer);

    RenderSurfaceChromium* rootRenderSurface = rootLayer->renderSurface();
    rootRenderSurface->clearLayerList();

    TransformationMatrix zoomMatrix;
    zoomMatrix.scale3d(m_zoomAnimatorScale, m_zoomAnimatorScale, 1);
    {
        TRACE_EVENT("CCLayerTreeHost::updateLayers::calcDrawEtc", this, 0);
        CCLayerTreeHostCommon::calculateDrawTransformsAndVisibility(rootLayer, rootLayer, zoomMatrix, zoomMatrix, m_updateList, rootRenderSurface->layerList(), layerRendererCapabilities().maxTextureSize);
    }

    paintLayerContents(m_updateList);
}

static void paintContentsIfDirty(LayerChromium* layer, const IntRect& visibleLayerRect)
{
    if (layer->drawsContent()) {
        layer->setVisibleLayerRect(visibleLayerRect);
        layer->paintContentsIfDirty();
    }
}

void CCLayerTreeHost::paintLayerContents(const LayerList& renderSurfaceLayerList)
{
    for (int surfaceIndex = renderSurfaceLayerList.size() - 1; surfaceIndex >= 0 ; --surfaceIndex) {
        LayerChromium* renderSurfaceLayer = renderSurfaceLayerList[surfaceIndex].get();
        RenderSurfaceChromium* renderSurface = renderSurfaceLayer->renderSurface();
        ASSERT(renderSurface);

        renderSurfaceLayer->setLayerTreeHost(this);

        // Render surfaces whose drawable area has zero width or height
        // will have no layers associated with them and should be skipped.
        if (!renderSurface->layerList().size())
            continue;

        if (!renderSurface->drawOpacity())
            continue;

        const LayerList& layerList = renderSurface->layerList();
        ASSERT(layerList.size());
        for (unsigned layerIndex = 0; layerIndex < layerList.size(); ++layerIndex) {
            LayerChromium* layer = layerList[layerIndex].get();

            // Layers that start a new render surface will be painted when the render
            // surface's list is processed.
            if (layer->renderSurface() && layer->renderSurface() != renderSurface)
                continue;

            layer->setLayerTreeHost(this);

            if (!layer->opacity())
                continue;

            if (layer->maskLayer())
                layer->maskLayer()->setLayerTreeHost(this);
            if (layer->replicaLayer()) {
                layer->replicaLayer()->setLayerTreeHost(this);
                if (layer->replicaLayer()->maskLayer())
                    layer->replicaLayer()->maskLayer()->setLayerTreeHost(this);
            }

            if (layer->bounds().isEmpty())
                continue;

            IntRect defaultContentRect = IntRect(rootLayer()->scrollPosition(), viewportSize());

            IntRect targetSurfaceRect = layer->targetRenderSurface() ? layer->targetRenderSurface()->contentRect() : defaultContentRect;
            if (layer->usesLayerScissor())
                targetSurfaceRect.intersect(layer->scissorRect());
            IntRect visibleLayerRect = CCLayerTreeHostCommon::calculateVisibleLayerRect(targetSurfaceRect, layer->bounds(), layer->contentBounds(), layer->drawTransform());

            visibleLayerRect.move(toSize(layer->scrollPosition()));
            paintContentsIfDirty(layer, visibleLayerRect);

            if (LayerChromium* maskLayer = layer->maskLayer())
                paintContentsIfDirty(maskLayer, IntRect(IntPoint(), maskLayer->contentBounds()));

            if (LayerChromium* replicaLayer = layer->replicaLayer()) {
                paintContentsIfDirty(replicaLayer, visibleLayerRect);

                if (LayerChromium* replicaMaskLayer = replicaLayer->maskLayer())
                    paintContentsIfDirty(replicaMaskLayer, IntRect(IntPoint(), replicaMaskLayer->contentBounds()));
            }
        }
    }
}

void CCLayerTreeHost::updateCompositorResources(const LayerList& renderSurfaceLayerList, GraphicsContext3D* context)
{
    for (int surfaceIndex = renderSurfaceLayerList.size() - 1; surfaceIndex >= 0 ; --surfaceIndex) {
        LayerChromium* renderSurfaceLayer = renderSurfaceLayerList[surfaceIndex].get();
        RenderSurfaceChromium* renderSurface = renderSurfaceLayer->renderSurface();
        ASSERT(renderSurface);

        if (!renderSurface->layerList().size() || !renderSurface->drawOpacity())
            continue;

        const LayerList& layerList = renderSurface->layerList();
        ASSERT(layerList.size());
        for (unsigned layerIndex = 0; layerIndex < layerList.size(); ++layerIndex) {
            LayerChromium* layer = layerList[layerIndex].get();
            if (layer->renderSurface() && layer->renderSurface() != renderSurface)
                continue;

            updateCompositorResources(layer, context);
        }
    }
}

void CCLayerTreeHost::updateCompositorResources(LayerChromium* layer, GraphicsContext3D* context)
{
    if (layer->bounds().isEmpty())
        return;

    if (!layer->opacity())
        return;

    if (layer->maskLayer())
        updateCompositorResources(layer->maskLayer(), context);
    if (layer->replicaLayer())
        updateCompositorResources(layer->replicaLayer(), context);

    if (layer->drawsContent())
        layer->updateCompositorResources(context);
}

void CCLayerTreeHost::clearPendingUpdate()
{
    for (size_t surfaceIndex = 0; surfaceIndex < m_updateList.size(); ++surfaceIndex) {
        LayerChromium* layer = m_updateList[surfaceIndex].get();
        ASSERT(layer->renderSurface());
        layer->clearRenderSurface();
    }
    m_updateList.clear();
}

}
