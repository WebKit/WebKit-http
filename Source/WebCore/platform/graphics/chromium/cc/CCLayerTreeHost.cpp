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
#include "Region.h"
#include "TraceEvent.h"
#include "TreeSynchronizer.h"
#include "cc/CCLayerIterator.h"
#include "cc/CCLayerTreeHostCommon.h"
#include "cc/CCLayerTreeHostImpl.h"
#include "cc/CCSingleThreadProxy.h"
#include "cc/CCThread.h"
#include "cc/CCThreadProxy.h"

using namespace std;

namespace {
static int numLayerTreeInstances;
}

namespace WebCore {

bool CCLayerTreeHost::anyLayerTreeHostInstanceExists()
{
    return numLayerTreeInstances > 0;
}

PassRefPtr<CCLayerTreeHost> CCLayerTreeHost::create(CCLayerTreeHostClient* client, const CCSettings& settings)
{
    RefPtr<CCLayerTreeHost> layerTreeHost = adoptRef(new CCLayerTreeHost(client, settings));
    if (!layerTreeHost->initialize())
        return 0;
    return layerTreeHost;
}

CCLayerTreeHost::CCLayerTreeHost(CCLayerTreeHostClient* client, const CCSettings& settings)
    : m_compositorIdentifier(-1)
    , m_animating(false)
    , m_client(client)
    , m_frameNumber(0)
    , m_layerRendererInitialized(false)
    , m_contextLost(false)
    , m_numTimesRecreateShouldFail(0)
    , m_numFailedRecreateAttempts(0)
    , m_settings(settings)
    , m_visible(true)
    , m_pageScaleFactor(1)
    , m_minPageScaleFactor(1)
    , m_maxPageScaleFactor(1)
    , m_triggerIdlePaints(true)
    , m_partialTextureUpdateRequests(0)
{
    ASSERT(CCProxy::isMainThread());
    numLayerTreeInstances++;
}

bool CCLayerTreeHost::initialize()
{
    TRACE_EVENT("CCLayerTreeHost::initialize", this, 0);
    if (CCProxy::hasImplThread()) {
        // The HUD does not work in threaded mode. Turn it off.
        m_settings.showFPSCounter = false;
        m_settings.showPlatformLayerTree = false;

        m_proxy = CCThreadProxy::create(this);
    } else
        m_proxy = CCSingleThreadProxy::create(this);
    m_proxy->start();

    if (!m_proxy->initializeContext())
        return false;

    m_compositorIdentifier = m_proxy->compositorIdentifier();
    return true;
}

CCLayerTreeHost::~CCLayerTreeHost()
{
    ASSERT(CCProxy::isMainThread());
    TRACE_EVENT("CCLayerTreeHost::~CCLayerTreeHost", this, 0);
    ASSERT(m_proxy);
    m_proxy->stop();
    m_proxy.clear();
    clearPendingUpdate();
    numLayerTreeInstances--;
}

void CCLayerTreeHost::initializeLayerRenderer()
{
    TRACE_EVENT("CCLayerTreeHost::initializeLayerRenderer", this, 0);
    if (!m_proxy->initializeLayerRenderer()) {
        // Uh oh, better tell the client that we can't do anything with this context.
        m_client->didRecreateContext(false);
        return;
    }

    // Update m_settings based on capabilities that we got back from the renderer.
    m_settings.acceleratePainting = m_proxy->layerRendererCapabilities().usingAcceleratedPainting;

    // Update m_settings based on partial update capability.
    m_settings.maxPartialTextureUpdates = min(m_settings.maxPartialTextureUpdates, m_proxy->maxPartialTextureUpdates());

    m_contentsTextureManager = TextureManager::create(TextureManager::highLimitBytes(viewportSize()),
                                                      TextureManager::reclaimLimitBytes(viewportSize()),
                                                      m_proxy->layerRendererCapabilities().maxTextureSize);

    m_layerRendererInitialized = true;
}

void CCLayerTreeHost::recreateContext()
{
    TRACE_EVENT0("cc", "CCLayerTreeHost::recreateContext");
    ASSERT(m_contextLost);

    bool recreated = false;
    if (!m_numTimesRecreateShouldFail)
        recreated = m_proxy->recreateContext();
    else
        m_numTimesRecreateShouldFail--;

    if (recreated) {
        m_client->didRecreateContext(true);
        m_contextLost = false;
        return;
    }

    // Tolerate a certain number of recreation failures to work around races
    // in the context-lost machinery.
    m_numFailedRecreateAttempts++;
    if (m_numFailedRecreateAttempts < 5) {
        setNeedsCommit();
        return;
    }

    // We have tried too many times to recreate the context. Tell the host to fall
    // back to software rendering.
    m_client->didRecreateContext(false);
}

void CCLayerTreeHost::deleteContentsTexturesOnImplThread(TextureAllocator* allocator)
{
    ASSERT(CCProxy::isImplThread());
    if (m_contentsTextureManager)
        m_contentsTextureManager->evictAndDeleteAllTextures(allocator);
}

void CCLayerTreeHost::updateAnimations(double frameBeginTime)
{
    m_animating = true;
    m_client->updateAnimations(frameBeginTime);
    m_animating = false;
}

void CCLayerTreeHost::layout()
{
    m_client->layout();
}

void CCLayerTreeHost::beginCommitOnImplThread(CCLayerTreeHostImpl* hostImpl)
{
    ASSERT(CCProxy::isImplThread());
    TRACE_EVENT("CCLayerTreeHost::commitTo", this, 0);

    m_contentsTextureManager->reduceMemoryToLimit(TextureManager::reclaimLimitBytes(viewportSize()));
    m_contentsTextureManager->deleteEvictedTextures(hostImpl->contentsTextureAllocator());
}

// This function commits the CCLayerTreeHost to an impl tree. When modifying
// this function, keep in mind that the function *runs* on the impl thread! Any
// code that is logically a main thread operation, e.g. deletion of a LayerChromium,
// should be delayed until the CCLayerTreeHost::commitComplete, which will run
// after the commit, but on the main thread.
void CCLayerTreeHost::finishCommitOnImplThread(CCLayerTreeHostImpl* hostImpl)
{
    ASSERT(CCProxy::isImplThread());

    hostImpl->setRootLayer(TreeSynchronizer::synchronizeTrees(rootLayer(), hostImpl->releaseRootLayer()));

    // We may have added an animation during the tree sync. This will cause hostImpl to visit its controllers.
    if (rootLayer())
        hostImpl->setNeedsAnimateLayers();

    hostImpl->setSourceFrameNumber(frameNumber());
    hostImpl->setViewportSize(viewportSize());
    hostImpl->setPageScaleFactorAndLimits(m_pageScaleFactor, m_minPageScaleFactor, m_maxPageScaleFactor);

    m_frameNumber++;
}

void CCLayerTreeHost::commitComplete()
{
    m_deleteTextureAfterCommitList.clear();
    clearPendingUpdate();
    m_contentsTextureManager->unprotectAllTextures();
}

PassRefPtr<GraphicsContext3D> CCLayerTreeHost::createContext()
{
    return m_client->createContext();
}

PassOwnPtr<CCLayerTreeHostImpl> CCLayerTreeHost::createLayerTreeHostImpl(CCLayerTreeHostImplClient* client)
{
    return CCLayerTreeHostImpl::create(m_settings, client);
}

void CCLayerTreeHost::didLoseContext()
{
    TRACE_EVENT("CCLayerTreeHost::didLoseContext", 0, this);
    ASSERT(CCProxy::isMainThread());
    m_contextLost = true;
    m_numFailedRecreateAttempts = 0;
    setNeedsCommit();
}

// Temporary hack until WebViewImpl context creation gets simplified
GraphicsContext3D* CCLayerTreeHost::context()
{
    ASSERT(!CCProxy::hasImplThread());
    return m_proxy->context();
}

bool CCLayerTreeHost::compositeAndReadback(void *pixels, const IntRect& rect)
{
    if (!m_layerRendererInitialized) {
        initializeLayerRenderer();
        if (!m_layerRendererInitialized)
            return false;
    }
    if (m_contextLost) {
        recreateContext();
        if (m_contextLost)
            return false;
    }
    m_triggerIdlePaints = false;
    bool ret = m_proxy->compositeAndReadback(pixels, rect);
    m_triggerIdlePaints = true;
    return ret;
}

void CCLayerTreeHost::finishAllRendering()
{
    if (!m_layerRendererInitialized)
        return;
    m_proxy->finishAllRendering();
}

const LayerRendererCapabilities& CCLayerTreeHost::layerRendererCapabilities() const
{
    return m_proxy->layerRendererCapabilities();
}

void CCLayerTreeHost::setNeedsAnimate()
{
    ASSERT(CCProxy::hasImplThread());
    m_proxy->setNeedsAnimate();
}

void CCLayerTreeHost::setNeedsCommit()
{
    if (CCThreadProxy::implThread())
        m_proxy->setNeedsCommit();
    else
        m_client->scheduleComposite();
}

void CCLayerTreeHost::setNeedsRedraw()
{
    m_proxy->setNeedsRedraw();
    if (!CCThreadProxy::implThread())
        m_client->scheduleComposite();
}

void CCLayerTreeHost::setAnimationEvents(PassOwnPtr<CCAnimationEventsVector> events)
{
    ASSERT(CCThreadProxy::isMainThread());
    setAnimationEventsRecursive(*events, m_rootLayer.get());
}

void CCLayerTreeHost::setRootLayer(PassRefPtr<LayerChromium> rootLayer)
{
    if (m_rootLayer == rootLayer)
        return;

    if (m_rootLayer)
        m_rootLayer->setLayerTreeHost(0);
    m_rootLayer = rootLayer;
    if (m_rootLayer)
        m_rootLayer->setLayerTreeHost(this);
    setNeedsCommit();
}

void CCLayerTreeHost::setViewportSize(const IntSize& viewportSize)
{
    if (viewportSize == m_viewportSize)
        return;

    if (m_contentsTextureManager) {
        m_contentsTextureManager->setMaxMemoryLimitBytes(TextureManager::highLimitBytes(viewportSize));
        m_contentsTextureManager->setPreferredMemoryLimitBytes(TextureManager::reclaimLimitBytes(viewportSize));
    }
    m_viewportSize = viewportSize;
    setNeedsCommit();
}

void CCLayerTreeHost::setPageScaleFactorAndLimits(float pageScaleFactor, float minPageScaleFactor, float maxPageScaleFactor)
{
    if (pageScaleFactor == m_pageScaleFactor && minPageScaleFactor == m_minPageScaleFactor && maxPageScaleFactor == m_maxPageScaleFactor)
        return;

    m_pageScaleFactor = pageScaleFactor;
    m_minPageScaleFactor = minPageScaleFactor;
    m_maxPageScaleFactor = maxPageScaleFactor;
    setNeedsCommit();
}

void CCLayerTreeHost::setVisible(bool visible)
{
    if (m_visible == visible)
        return;

    m_visible = visible;

    if (!m_layerRendererInitialized)
        return;

    if (!visible) {
        m_contentsTextureManager->reduceMemoryToLimit(TextureManager::lowLimitBytes(viewportSize()));
        m_contentsTextureManager->unprotectAllTextures();
    }

    // Tells the proxy that visibility state has changed. This will in turn call
    // CCLayerTreeHost::didBecomeInvisibleOnImplThread on the appropriate thread, for
    // the case where !visible.
    m_proxy->setVisible(visible);
}

void CCLayerTreeHost::didBecomeInvisibleOnImplThread(CCLayerTreeHostImpl* hostImpl)
{
    ASSERT(CCProxy::isImplThread());
    if (m_proxy->layerRendererCapabilities().contextHasCachedFrontBuffer)
        contentsTextureManager()->evictAndDeleteAllTextures(hostImpl->contentsTextureAllocator());
    else {
        contentsTextureManager()->reduceMemoryToLimit(TextureManager::reclaimLimitBytes(viewportSize()));
        contentsTextureManager()->deleteEvictedTextures(hostImpl->contentsTextureAllocator());
    }

    // Ensure that the dropped tiles are propagated to the impl tree.
    // If the frontbuffer is cached, then clobber the impl tree. Otherwise,
    // push over the tree changes.
    if (m_proxy->layerRendererCapabilities().contextHasCachedFrontBuffer) {
        hostImpl->setRootLayer(nullptr);
        return;
    }

    hostImpl->setRootLayer(TreeSynchronizer::synchronizeTrees(rootLayer(), hostImpl->releaseRootLayer()));

    // We may have added an animation during the tree sync. This will cause hostImpl to visit its controllers.
    if (rootLayer())
        hostImpl->setNeedsAnimateLayers();
}

void CCLayerTreeHost::startPageScaleAnimation(const IntSize& targetPosition, bool useAnchor, float scale, double durationSec)
{
    m_proxy->startPageScaleAnimation(targetPosition, useAnchor, scale, durationSec);
}

void CCLayerTreeHost::loseContext(int numTimes)
{
    TRACE_EVENT1("cc", "CCLayerTreeHost::loseCompositorContext", "numTimes", numTimes);
    m_numTimesRecreateShouldFail = numTimes - 1;
    m_proxy->loseContext();
}

TextureManager* CCLayerTreeHost::contentsTextureManager() const
{
    return m_contentsTextureManager.get();
}

void CCLayerTreeHost::composite()
{
    ASSERT(!CCThreadProxy::implThread());
    static_cast<CCSingleThreadProxy*>(m_proxy.get())->compositeImmediately();
}

bool CCLayerTreeHost::updateLayers()
{
    if (!m_layerRendererInitialized) {
        initializeLayerRenderer();
        // If we couldn't initialize, then bail since we're returning to software mode.
        if (!m_layerRendererInitialized)
            return false;
    }
    if (m_contextLost) {
        recreateContext();
        if (m_contextLost)
            return false;
    }

    if (!rootLayer())
        return true;

    if (viewportSize().isEmpty())
        return true;

    updateLayers(rootLayer());
    return true;
}

void CCLayerTreeHost::updateLayers(LayerChromium* rootLayer)
{
    TRACE_EVENT("CCLayerTreeHost::updateLayers", this, 0);

    if (!rootLayer->renderSurface())
        rootLayer->createRenderSurface();
    rootLayer->renderSurface()->setContentRect(IntRect(IntPoint(0, 0), viewportSize()));

    IntRect rootClipRect(IntPoint(), viewportSize());
    rootLayer->setClipRect(rootClipRect);

    // This assert fires if updateCompositorResources wasn't called after
    // updateLayers. Only one update can be pending at any given time.
    ASSERT(!m_updateList.size());
    m_updateList.append(rootLayer);

    RenderSurfaceChromium* rootRenderSurface = rootLayer->renderSurface();
    rootRenderSurface->clearLayerList();

    TransformationMatrix identityMatrix;
    {
        TRACE_EVENT("CCLayerTreeHost::updateLayers::calcDrawEtc", this, 0);
        CCLayerTreeHostCommon::calculateDrawTransformsAndVisibility(rootLayer, rootLayer, identityMatrix, identityMatrix, m_updateList, rootRenderSurface->layerList(), layerRendererCapabilities().maxTextureSize);
    }

    // Reset partial texture update requests.
    m_partialTextureUpdateRequests = 0;

    reserveTextures();

    paintLayerContents(m_updateList, PaintVisible);
    if (!m_triggerIdlePaints)
        return;

    size_t preferredLimitBytes = TextureManager::reclaimLimitBytes(m_viewportSize);
    size_t maxLimitBytes = TextureManager::highLimitBytes(m_viewportSize);
    m_contentsTextureManager->reduceMemoryToLimit(preferredLimitBytes);
    if (m_contentsTextureManager->currentMemoryUseBytes() >= preferredLimitBytes)
        return;

    // Idle painting should fail when we hit the preferred memory limit,
    // otherwise it will always push us towards the maximum limit.
    m_contentsTextureManager->setMaxMemoryLimitBytes(preferredLimitBytes);
    // The second (idle) paint will be a no-op in layers where painting already occured above.
    paintLayerContents(m_updateList, PaintIdle);
    m_contentsTextureManager->setMaxMemoryLimitBytes(maxLimitBytes);
}

void CCLayerTreeHost::reserveTextures()
{
    // Use BackToFront since it's cheap and this isn't order-dependent.
    typedef CCLayerIterator<LayerChromium, Vector<RefPtr<LayerChromium> >, RenderSurfaceChromium, CCLayerIteratorActions::BackToFront> CCLayerIteratorType;

    CCLayerIteratorType end = CCLayerIteratorType::end(&m_updateList);
    for (CCLayerIteratorType it = CCLayerIteratorType::begin(&m_updateList); it != end; ++it) {
        if (!it.representsItself() || !it->alwaysReserveTextures())
            continue;
        it->reserveTextures();
    }
}

// static
void CCLayerTreeHost::paintContentsIfDirty(LayerChromium* layer, PaintType paintType, const Region& occludedScreenSpace)
{
    ASSERT(layer);
    ASSERT(PaintVisible == paintType || PaintIdle == paintType);
    if (PaintVisible == paintType)
        layer->paintContentsIfDirty(occludedScreenSpace);
    else
        layer->idlePaintContentsIfDirty();
}

void CCLayerTreeHost::paintMaskAndReplicaForRenderSurface(LayerChromium* renderSurfaceLayer, PaintType paintType)
{
    // Note: Masks and replicas only exist for layers that own render surfaces. If we reach this point
    // in code, we already know that at least something will be drawn into this render surface, so the
    // mask and replica should be painted.

    // FIXME: If the surface has a replica, it should be painted with occlusion that excludes the current target surface subtree.
    Region noOcclusion;

    if (renderSurfaceLayer->maskLayer()) {
        renderSurfaceLayer->maskLayer()->setVisibleLayerRect(IntRect(IntPoint(), renderSurfaceLayer->contentBounds()));
        paintContentsIfDirty(renderSurfaceLayer->maskLayer(), paintType, noOcclusion);
    }

    LayerChromium* replicaLayer = renderSurfaceLayer->replicaLayer();
    if (replicaLayer) {
        paintContentsIfDirty(replicaLayer, paintType, noOcclusion);

        if (replicaLayer->maskLayer()) {
            replicaLayer->maskLayer()->setVisibleLayerRect(IntRect(IntPoint(), replicaLayer->maskLayer()->contentBounds()));
            paintContentsIfDirty(replicaLayer->maskLayer(), paintType, noOcclusion);
        }
    }
}

struct RenderSurfaceRegion {
    RenderSurfaceChromium* surface;
    Region occludedInScreen;
};

// Add the surface to the top of the stack and copy the occlusion from the old top of the stack to the new.
static void enterTargetRenderSurface(Vector<RenderSurfaceRegion>& stack, RenderSurfaceChromium* newTarget)
{
    if (stack.isEmpty()) {
        stack.append(RenderSurfaceRegion());
        stack.last().surface = newTarget;
    } else if (stack.last().surface != newTarget) {
        // If we are entering a subtree that is going to move pixels around, then the occlusion we've computed
        // so far won't apply to the pixels we're drawing here in the same way. We discard the occlusion thus
        // far to be safe, and ensure we don't cull any pixels that are moved such that they become visible.
        const RenderSurfaceChromium* oldAncestorThatMovesPixels = stack.last().surface->nearestAncestorThatMovesPixels();
        const RenderSurfaceChromium* newAncestorThatMovesPixels = newTarget->nearestAncestorThatMovesPixels();
        bool enteringSubtreeThatMovesPixels = newAncestorThatMovesPixels && newAncestorThatMovesPixels != oldAncestorThatMovesPixels;

        stack.append(RenderSurfaceRegion());
        stack.last().surface = newTarget;
        int lastIndex = stack.size() - 1;
        if (!enteringSubtreeThatMovesPixels)
            stack[lastIndex].occludedInScreen = stack[lastIndex - 1].occludedInScreen;
    }
}

// Pop the top of the stack off, push on the new surface, and merge the old top's occlusion into the new top surface.
static void leaveTargetRenderSurface(Vector<RenderSurfaceRegion>& stack, RenderSurfaceChromium* newTarget)
{
    int lastIndex = stack.size() - 1;
    bool surfaceWillBeAtTopAfterPop = stack.size() > 1 && stack[lastIndex - 1].surface == newTarget;

    if (surfaceWillBeAtTopAfterPop) {
        // Merge the top of the stack down.
        stack[lastIndex - 1].occludedInScreen.unite(stack[lastIndex].occludedInScreen);
        stack.removeLast();
    } else {
        // Replace the top of the stack with the new pushed surface. Copy the occluded region to the top.
        stack.last().surface = newTarget;
    }
}

void CCLayerTreeHost::paintLayerContents(const LayerList& renderSurfaceLayerList, PaintType paintType)
{
    // Use FrontToBack to allow for testing occlusion and performing culling during the tree walk.
    typedef CCLayerIterator<LayerChromium, Vector<RefPtr<LayerChromium> >, RenderSurfaceChromium, CCLayerIteratorActions::FrontToBack> CCLayerIteratorType;

    // The stack holds occluded regions for subtrees in the RenderSurface-Layer tree, so that when we leave a subtree we may
    // apply a mask to it, but not to the parts outside the subtree.
    // - The first time we see a new subtree under a target, we add that target to the top of the stack. This can happen as a layer representing itself, or as a target surface.
    // - When we visit a target surface, we apply its mask to its subtree, which is at the top of the stack.
    // - When we visit a layer representing itself, we add its occlusion to the current subtree, which is at the top of the stack.
    // - When we visit a layer representing a contributing surface, the current target will never be the top of the stack since we just came from the contributing surface.
    // We merge the occlusion at the top of the stack with the new current subtree. This new target is pushed onto the stack if not already there.
    Vector<RenderSurfaceRegion> targetSurfaceStack;

    CCLayerIteratorType end = CCLayerIteratorType::end(&renderSurfaceLayerList);
    for (CCLayerIteratorType it = CCLayerIteratorType::begin(&renderSurfaceLayerList); it != end; ++it) {
        if (it.representsTargetRenderSurface()) {
            ASSERT(it->renderSurface()->drawOpacity());

            enterTargetRenderSurface(targetSurfaceStack, it->renderSurface());
            paintMaskAndReplicaForRenderSurface(*it, paintType);
            // FIXME: add the replica layer to the current occlusion

            if (it->maskLayer() || it->renderSurface()->drawOpacity() < 1 || it->renderSurface()->filters().hasFilterThatAffectsOpacity())
                targetSurfaceStack.last().occludedInScreen = Region();
        } else if (it.representsItself()) {
            ASSERT(!it->bounds().isEmpty());

            enterTargetRenderSurface(targetSurfaceStack, it->targetRenderSurface());
            paintContentsIfDirty(*it, paintType, targetSurfaceStack.last().occludedInScreen);
            it->addSelfToOccludedScreenSpace(targetSurfaceStack.last().occludedInScreen);
        } else {
            leaveTargetRenderSurface(targetSurfaceStack, it.targetRenderSurfaceLayer()->renderSurface());
        }
    }
}

void CCLayerTreeHost::updateCompositorResources(GraphicsContext3D* context, CCTextureUpdater& updater)
{
    // Use BackToFront since it's cheap and this isn't order-dependent.
    typedef CCLayerIterator<LayerChromium, Vector<RefPtr<LayerChromium> >, RenderSurfaceChromium, CCLayerIteratorActions::BackToFront> CCLayerIteratorType;

    CCLayerIteratorType end = CCLayerIteratorType::end(&m_updateList);
    for (CCLayerIteratorType it = CCLayerIteratorType::begin(&m_updateList); it != end; ++it) {
        if (it.representsTargetRenderSurface()) {
            ASSERT(it->renderSurface()->drawOpacity());
            if (it->maskLayer())
                it->maskLayer()->updateCompositorResources(context, updater);

            if (it->replicaLayer()) {
                it->replicaLayer()->updateCompositorResources(context, updater);
                if (it->replicaLayer()->maskLayer())
                    it->replicaLayer()->maskLayer()->updateCompositorResources(context, updater);
            }
        } else if (it.representsItself())
            it->updateCompositorResources(context, updater);
    }
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

void CCLayerTreeHost::applyScrollAndScale(const CCScrollAndScaleSet& info)
{
    // FIXME: pushing scroll offsets to non-root layers not implemented
    if (!info.scrolls.size())
        return;

    ASSERT(info.scrolls.size() == 1);
    IntSize scrollDelta = info.scrolls[0].scrollDelta;
    m_client->applyScrollAndScale(scrollDelta, info.pageScaleDelta);
}

void CCLayerTreeHost::startRateLimiter(GraphicsContext3D* context)
{
    if (m_animating)
        return;
    ASSERT(context);
    RateLimiterMap::iterator it = m_rateLimiters.find(context);
    if (it != m_rateLimiters.end())
        it->second->start();
    else {
        RefPtr<RateLimiter> rateLimiter = RateLimiter::create(context);
        m_rateLimiters.set(context, rateLimiter);
        rateLimiter->start();
    }
}

void CCLayerTreeHost::stopRateLimiter(GraphicsContext3D* context)
{
    RateLimiterMap::iterator it = m_rateLimiters.find(context);
    if (it != m_rateLimiters.end()) {
        it->second->stop();
        m_rateLimiters.remove(it);
    }
}

bool CCLayerTreeHost::requestPartialTextureUpdate()
{
    if (m_partialTextureUpdateRequests >= m_settings.maxPartialTextureUpdates)
        return false;

    m_partialTextureUpdateRequests++;
    return true;
}

void CCLayerTreeHost::deleteTextureAfterCommit(PassOwnPtr<ManagedTexture> texture)
{
    m_deleteTextureAfterCommitList.append(texture);
}

void CCLayerTreeHost::setAnimationEventsRecursive(const CCAnimationEventsVector& events, LayerChromium* layer)
{
    for (size_t eventIndex = 0; eventIndex < events.size(); ++eventIndex) {
        if (layer->id() == events[eventIndex]->layerId())
            layer->setAnimationEvent(*events[eventIndex]);
    }

    for (size_t childIndex = 0; childIndex < layer->children().size(); ++childIndex)
        setAnimationEventsRecursive(events, layer->children()[childIndex].get());
}

} // namespace WebCore
