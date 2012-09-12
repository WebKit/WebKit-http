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

#include "CCLayerTreeHostImpl.h"

#include "CCAppendQuadsData.h"
#include "CCDamageTracker.h"
#include "CCDebugRectHistory.h"
#include "CCDelayBasedTimeSource.h"
#include "CCFontAtlas.h"
#include "CCFrameRateCounter.h"
#include "CCHeadsUpDisplayLayerImpl.h"
#include "CCLayerIterator.h"
#include "CCLayerTreeHost.h"
#include "CCLayerTreeHostCommon.h"
#include "CCMathUtil.h"
#include "CCOverdrawMetrics.h"
#include "CCPageScaleAnimation.h"
#include "CCPrioritizedTextureManager.h"
#include "CCRenderPassDrawQuad.h"
#include "CCRendererGL.h"
#include "CCRenderingStats.h"
#include "CCScrollbarAnimationController.h"
#include "CCScrollbarLayerImpl.h"
#include "CCSettings.h"
#include "CCSingleThreadProxy.h"
#include "TextStream.h"
#include "TraceEvent.h"
#include <wtf/CurrentTime.h>

using WebKit::WebTransformationMatrix;

namespace {

void didVisibilityChange(WebCore::CCLayerTreeHostImpl* id, bool visible)
{
    if (visible) {
        TRACE_EVENT_ASYNC_BEGIN1("webkit", "CCLayerTreeHostImpl::setVisible", id, "CCLayerTreeHostImpl", id);
        return;
    }

    TRACE_EVENT_ASYNC_END0("webkit", "CCLayerTreeHostImpl::setVisible", id);
}

} // namespace

namespace WebCore {

class CCLayerTreeHostImplTimeSourceAdapter : public CCTimeSourceClient {
    WTF_MAKE_NONCOPYABLE(CCLayerTreeHostImplTimeSourceAdapter);
public:
    static PassOwnPtr<CCLayerTreeHostImplTimeSourceAdapter> create(CCLayerTreeHostImpl* layerTreeHostImpl, PassRefPtr<CCDelayBasedTimeSource> timeSource)
    {
        return adoptPtr(new CCLayerTreeHostImplTimeSourceAdapter(layerTreeHostImpl, timeSource));
    }
    virtual ~CCLayerTreeHostImplTimeSourceAdapter()
    {
        m_timeSource->setClient(0);
        m_timeSource->setActive(false);
    }

    virtual void onTimerTick() OVERRIDE
    {
        // FIXME: We require that animate be called on the impl thread. This
        // avoids asserts in single threaded mode. Ideally background ticking
        // would be handled by the proxy/scheduler and this could be removed.
        DebugScopedSetImplThread impl;

        m_layerTreeHostImpl->animate(monotonicallyIncreasingTime(), currentTime());
    }

    void setActive(bool active)
    {
        if (active != m_timeSource->active())
            m_timeSource->setActive(active);
    }

private:
    CCLayerTreeHostImplTimeSourceAdapter(CCLayerTreeHostImpl* layerTreeHostImpl, PassRefPtr<CCDelayBasedTimeSource> timeSource)
        : m_layerTreeHostImpl(layerTreeHostImpl)
        , m_timeSource(timeSource)
    {
        m_timeSource->setClient(this);
    }

    CCLayerTreeHostImpl* m_layerTreeHostImpl;
    RefPtr<CCDelayBasedTimeSource> m_timeSource;
};

PassOwnPtr<CCLayerTreeHostImpl> CCLayerTreeHostImpl::create(const CCLayerTreeSettings& settings, CCLayerTreeHostImplClient* client)
{
    return adoptPtr(new CCLayerTreeHostImpl(settings, client));
}

CCLayerTreeHostImpl::CCLayerTreeHostImpl(const CCLayerTreeSettings& settings, CCLayerTreeHostImplClient* client)
    : m_client(client)
    , m_sourceFrameNumber(-1)
    , m_rootScrollLayerImpl(0)
    , m_currentlyScrollingLayerImpl(0)
    , m_hudLayerImpl(0)
    , m_scrollingLayerIdFromPreviousTree(-1)
    , m_scrollDeltaIsInScreenSpace(false)
    , m_settings(settings)
    , m_deviceScaleFactor(1)
    , m_visible(true)
    , m_contentsTexturesPurged(false)
    , m_memoryAllocationLimitBytes(CCPrioritizedTextureManager::defaultMemoryAllocationLimit())
    , m_pageScale(1)
    , m_pageScaleDelta(1)
    , m_sentPageScaleDelta(1)
    , m_minPageScale(0)
    , m_maxPageScale(0)
    , m_backgroundColor(0)
    , m_hasTransparentBackground(false)
    , m_needsAnimateLayers(false)
    , m_pinchGestureActive(false)
    , m_fpsCounter(CCFrameRateCounter::create())
    , m_debugRectHistory(CCDebugRectHistory::create())
{
    ASSERT(CCProxy::isImplThread());
    didVisibilityChange(this, m_visible);
}

CCLayerTreeHostImpl::~CCLayerTreeHostImpl()
{
    ASSERT(CCProxy::isImplThread());
    TRACE_EVENT0("cc", "CCLayerTreeHostImpl::~CCLayerTreeHostImpl()");

    if (m_rootLayerImpl)
        clearRenderSurfaces();
}

void CCLayerTreeHostImpl::beginCommit()
{
}

void CCLayerTreeHostImpl::commitComplete()
{
    TRACE_EVENT0("cc", "CCLayerTreeHostImpl::commitComplete");
    // Recompute max scroll position; must be after layer content bounds are
    // updated.
    updateMaxScrollPosition();
}

bool CCLayerTreeHostImpl::canDraw()
{
    // Note: If you are changing this function or any other function that might
    // affect the result of canDraw, make sure to call m_client->onCanDrawStateChanged
    // in the proper places and update the notifyIfCanDrawChanged test.

    if (!m_rootLayerImpl) {
        TRACE_EVENT_INSTANT0("cc", "CCLayerTreeHostImpl::canDraw no root layer");
        return false;
    }
    if (deviceViewportSize().isEmpty()) {
        TRACE_EVENT_INSTANT0("cc", "CCLayerTreeHostImpl::canDraw empty viewport");
        return false;
    }
    if (!m_renderer) {
        TRACE_EVENT_INSTANT0("cc", "CCLayerTreeHostImpl::canDraw no renderer");
        return false;
    }
    if (m_contentsTexturesPurged) {
        TRACE_EVENT_INSTANT0("cc", "CCLayerTreeHostImpl::canDraw contents textures purged");
        return false;
    }
    return true;
}

CCGraphicsContext* CCLayerTreeHostImpl::context() const
{
    return m_context.get();
}

void CCLayerTreeHostImpl::animate(double monotonicTime, double wallClockTime)
{
    animatePageScale(monotonicTime);
    animateLayers(monotonicTime, wallClockTime);
    animateScrollbars(monotonicTime);
}

void CCLayerTreeHostImpl::startPageScaleAnimation(const IntSize& targetPosition, bool anchorPoint, float pageScale, double startTime, double duration)
{
    if (!m_rootScrollLayerImpl)
        return;

    IntSize scrollTotal = flooredIntSize(m_rootScrollLayerImpl->scrollPosition() + m_rootScrollLayerImpl->scrollDelta());
    scrollTotal.scale(m_pageScaleDelta);
    float scaleTotal = m_pageScale * m_pageScaleDelta;
    IntSize scaledContentSize = contentSize();
    scaledContentSize.scale(m_pageScaleDelta);

    m_pageScaleAnimation = CCPageScaleAnimation::create(scrollTotal, scaleTotal, m_deviceViewportSize, scaledContentSize, startTime);

    if (anchorPoint) {
        IntSize windowAnchor(targetPosition);
        windowAnchor.scale(scaleTotal / pageScale);
        windowAnchor -= scrollTotal;
        m_pageScaleAnimation->zoomWithAnchor(windowAnchor, pageScale, duration);
    } else
        m_pageScaleAnimation->zoomTo(targetPosition, pageScale, duration);

    m_client->setNeedsRedrawOnImplThread();
    m_client->setNeedsCommitOnImplThread();
}

void CCLayerTreeHostImpl::scheduleAnimation()
{
    m_client->setNeedsRedrawOnImplThread();
}

void CCLayerTreeHostImpl::trackDamageForAllSurfaces(CCLayerImpl* rootDrawLayer, const CCLayerList& renderSurfaceLayerList)
{
    // For now, we use damage tracking to compute a global scissor. To do this, we must
    // compute all damage tracking before drawing anything, so that we know the root
    // damage rect. The root damage rect is then used to scissor each surface.

    for (int surfaceIndex = renderSurfaceLayerList.size() - 1; surfaceIndex >= 0 ; --surfaceIndex) {
        CCLayerImpl* renderSurfaceLayer = renderSurfaceLayerList[surfaceIndex];
        CCRenderSurface* renderSurface = renderSurfaceLayer->renderSurface();
        ASSERT(renderSurface);
        renderSurface->damageTracker()->updateDamageTrackingState(renderSurface->layerList(), renderSurfaceLayer->id(), renderSurface->surfacePropertyChangedOnlyFromDescendant(), renderSurface->contentRect(), renderSurfaceLayer->maskLayer(), renderSurfaceLayer->filters());
    }
}

void CCLayerTreeHostImpl::calculateRenderSurfaceLayerList(CCLayerList& renderSurfaceLayerList)
{
    ASSERT(renderSurfaceLayerList.isEmpty());
    ASSERT(m_rootLayerImpl);
    ASSERT(m_renderer); // For maxTextureSize.

    {
        TRACE_EVENT0("cc", "CCLayerTreeHostImpl::calcDrawEtc");
        CCLayerTreeHostCommon::calculateDrawTransforms(m_rootLayerImpl.get(), deviceViewportSize(), m_deviceScaleFactor, &m_layerSorter, rendererCapabilities().maxTextureSize, renderSurfaceLayerList);
        CCLayerTreeHostCommon::calculateVisibleRects(renderSurfaceLayerList);

        trackDamageForAllSurfaces(m_rootLayerImpl.get(), renderSurfaceLayerList);
    }
}

void CCLayerTreeHostImpl::FrameData::appendRenderPass(PassOwnPtr<CCRenderPass> renderPass)
{
    CCRenderPass* pass = renderPass.get();
    renderPasses.append(pass);
    renderPassesById.set(pass->id(), renderPass);
}

bool CCLayerTreeHostImpl::calculateRenderPasses(FrameData& frame)
{
    ASSERT(frame.renderPasses.isEmpty());

    calculateRenderSurfaceLayerList(*frame.renderSurfaceLayerList);

    TRACE_EVENT1("cc", "CCLayerTreeHostImpl::calculateRenderPasses", "renderSurfaceLayerList.size()", static_cast<long long unsigned>(frame.renderSurfaceLayerList->size()));

    // Create the render passes in dependency order.
    for (int surfaceIndex = frame.renderSurfaceLayerList->size() - 1; surfaceIndex >= 0 ; --surfaceIndex) {
        CCLayerImpl* renderSurfaceLayer = (*frame.renderSurfaceLayerList)[surfaceIndex];
        renderSurfaceLayer->renderSurface()->appendRenderPasses(frame);
    }

    bool recordMetricsForFrame = true; // FIXME: In the future, disable this when about:tracing is off.
    CCOcclusionTrackerImpl occlusionTracker(m_rootLayerImpl->renderSurface()->contentRect(), recordMetricsForFrame);
    occlusionTracker.setMinimumTrackingSize(m_settings.minimumOcclusionTrackingSize);

    if (settings().showOccludingRects)
        occlusionTracker.setOccludingScreenSpaceRectsContainer(&frame.occludingScreenSpaceRects);

    // Add quads to the Render passes in FrontToBack order to allow for testing occlusion and performing culling during the tree walk.
    typedef CCLayerIterator<CCLayerImpl, Vector<CCLayerImpl*>, CCRenderSurface, CCLayerIteratorActions::FrontToBack> CCLayerIteratorType;

    // Typically when we are missing a texture and use a checkerboard quad, we still draw the frame. However when the layer being
    // checkerboarded is moving due to an impl-animation, we drop the frame to avoid flashing due to the texture suddenly appearing
    // in the future.
    bool drawFrame = true;

    CCLayerIteratorType end = CCLayerIteratorType::end(frame.renderSurfaceLayerList);
    for (CCLayerIteratorType it = CCLayerIteratorType::begin(frame.renderSurfaceLayerList); it != end; ++it) {
        CCRenderPass::Id targetRenderPassId = it.targetRenderSurfaceLayer()->renderSurface()->renderPassId();
        CCRenderPass* targetRenderPass = frame.renderPassesById.get(targetRenderPassId);

        occlusionTracker.enterLayer(it);

        CCAppendQuadsData appendQuadsData;

        if (it.representsContributingRenderSurface()) {
            CCRenderPass::Id contributingRenderPassId = it->renderSurface()->renderPassId();
            CCRenderPass* contributingRenderPass = frame.renderPassesById.get(contributingRenderPassId);
            targetRenderPass->appendQuadsForRenderSurfaceLayer(*it, contributingRenderPass, &occlusionTracker, appendQuadsData);
        } else if (it.representsItself() && !it->visibleContentRect().isEmpty()) {
            bool hasOcclusionFromOutsideTargetSurface;
            if (occlusionTracker.occluded(*it, it->visibleContentRect(), &hasOcclusionFromOutsideTargetSurface))
                appendQuadsData.hadOcclusionFromOutsideTargetSurface |= hasOcclusionFromOutsideTargetSurface;
            else {
                it->willDraw(m_resourceProvider.get());
                frame.willDrawLayers.append(*it);
                targetRenderPass->appendQuadsForLayer(*it, &occlusionTracker, appendQuadsData);
            }
        }

        if (appendQuadsData.hadOcclusionFromOutsideTargetSurface)
            targetRenderPass->setHasOcclusionFromOutsideTargetSurface(true);

        if (appendQuadsData.hadMissingTiles) {
            bool layerHasAnimatingTransform = it->screenSpaceTransformIsAnimating() || it->drawTransformIsAnimating();
            if (layerHasAnimatingTransform)
                drawFrame = false;
        }

        occlusionTracker.leaveLayer(it);
    }

#if !ASSERT_DISABLED
    for (size_t i = 0; i < frame.renderPasses.size(); ++i) {
        for (size_t j = 0; j < frame.renderPasses[i]->quadList().size(); ++j)
            ASSERT(frame.renderPasses[i]->quadList()[j]->sharedQuadStateId() >= 0);
        ASSERT(frame.renderPassesById.contains(frame.renderPasses[i]->id()));
    }
#endif

    if (!m_hasTransparentBackground) {
        frame.renderPasses.last()->setHasTransparentBackground(false);
        frame.renderPasses.last()->appendQuadsToFillScreen(m_rootLayerImpl.get(), m_backgroundColor, occlusionTracker);
    }

    if (drawFrame)
        occlusionTracker.overdrawMetrics().recordMetrics(this);

    removeRenderPasses(CullRenderPassesWithNoQuads(), frame);
    m_renderer->decideRenderPassAllocationsForFrame(frame.renderPasses);
    removeRenderPasses(CullRenderPassesWithCachedTextures(*m_renderer), frame);

    return drawFrame;
}

void CCLayerTreeHostImpl::animateLayersRecursive(CCLayerImpl* current, double monotonicTime, double wallClockTime, CCAnimationEventsVector* events, bool& didAnimate, bool& needsAnimateLayers)
{
    bool subtreeNeedsAnimateLayers = false;

    CCLayerAnimationController* currentController = current->layerAnimationController();

    bool hadActiveAnimation = currentController->hasActiveAnimation();
    currentController->animate(monotonicTime, events);
    bool startedAnimation = events->size() > 0;

    // We animated if we either ticked a running animation, or started a new animation.
    if (hadActiveAnimation || startedAnimation)
        didAnimate = true;

    // If the current controller still has an active animation, we must continue animating layers.
    if (currentController->hasActiveAnimation())
         subtreeNeedsAnimateLayers = true;

    for (size_t i = 0; i < current->children().size(); ++i) {
        bool childNeedsAnimateLayers = false;
        animateLayersRecursive(current->children()[i].get(), monotonicTime, wallClockTime, events, didAnimate, childNeedsAnimateLayers);
        if (childNeedsAnimateLayers)
            subtreeNeedsAnimateLayers = true;
    }

    needsAnimateLayers = subtreeNeedsAnimateLayers;
}

void CCLayerTreeHostImpl::setBackgroundTickingEnabled(bool enabled)
{
    // Lazily create the timeSource adapter so that we can vary the interval for testing.
    if (!m_timeSourceClientAdapter)
        m_timeSourceClientAdapter = CCLayerTreeHostImplTimeSourceAdapter::create(this, CCDelayBasedTimeSource::create(lowFrequencyAnimationInterval(), CCProxy::currentThread()));

    m_timeSourceClientAdapter->setActive(enabled);
}

IntSize CCLayerTreeHostImpl::contentSize() const
{
    // TODO(aelias): Hardcoding the first child here is weird. Think of
    // a cleaner way to get the contentBounds on the Impl side.
    if (!m_rootScrollLayerImpl || m_rootScrollLayerImpl->children().isEmpty())
        return IntSize();
    return m_rootScrollLayerImpl->children()[0]->contentBounds();
}

static inline CCRenderPass* findRenderPassById(CCRenderPass::Id renderPassId, const CCLayerTreeHostImpl::FrameData& frame)
{
    CCRenderPassIdHashMap::const_iterator it = frame.renderPassesById.find(renderPassId);
    ASSERT(it != frame.renderPassesById.end());
    return it->second.get();
}

static void removeRenderPassesRecursive(CCRenderPass::Id removeRenderPassId, CCLayerTreeHostImpl::FrameData& frame)
{
    CCRenderPass* removeRenderPass = findRenderPassById(removeRenderPassId, frame);
    size_t removeIndex = frame.renderPasses.find(removeRenderPass);

    // The pass was already removed by another quad - probably the original, and we are the replica.
    if (removeIndex == notFound)
        return;

    const CCRenderPass* removedPass = frame.renderPasses[removeIndex];
    frame.renderPasses.remove(removeIndex);

    // Now follow up for all RenderPass quads and remove their RenderPasses recursively.
    const CCQuadList& quadList = removedPass->quadList();
    CCQuadList::constBackToFrontIterator quadListIterator = quadList.backToFrontBegin();
    for (; quadListIterator != quadList.backToFrontEnd(); ++quadListIterator) {
        CCDrawQuad* currentQuad = (*quadListIterator).get();
        if (currentQuad->material() != CCDrawQuad::RenderPass)
            continue;

        CCRenderPass::Id nextRemoveRenderPassId = CCRenderPassDrawQuad::materialCast(currentQuad)->renderPassId();
        removeRenderPassesRecursive(nextRemoveRenderPassId, frame);
    }
}

bool CCLayerTreeHostImpl::CullRenderPassesWithCachedTextures::shouldRemoveRenderPass(const CCRenderPassDrawQuad& quad, const FrameData&) const
{
    return quad.contentsChangedSinceLastFrame().isEmpty() && m_renderer.haveCachedResourcesForRenderPassId(quad.renderPassId());
}

bool CCLayerTreeHostImpl::CullRenderPassesWithNoQuads::shouldRemoveRenderPass(const CCRenderPassDrawQuad& quad, const FrameData& frame) const
{
    const CCRenderPass* renderPass = findRenderPassById(quad.renderPassId(), frame);
    size_t passIndex = frame.renderPasses.find(renderPass);

    bool renderPassAlreadyRemoved = passIndex == notFound;
    if (renderPassAlreadyRemoved)
        return false;

    // If any quad or RenderPass draws into this RenderPass, then keep it.
    const CCQuadList& quadList = frame.renderPasses[passIndex]->quadList();
    for (CCQuadList::constBackToFrontIterator quadListIterator = quadList.backToFrontBegin(); quadListIterator != quadList.backToFrontEnd(); ++quadListIterator) {
        CCDrawQuad* currentQuad = quadListIterator->get();

        if (currentQuad->material() != CCDrawQuad::RenderPass)
            return false;

        const CCRenderPass* contributingPass = findRenderPassById(CCRenderPassDrawQuad::materialCast(currentQuad)->renderPassId(), frame);
        if (frame.renderPasses.contains(contributingPass))
            return false;
    }
    return true;
}

// Defined for linking tests.
template void CCLayerTreeHostImpl::removeRenderPasses<CCLayerTreeHostImpl::CullRenderPassesWithCachedTextures>(CullRenderPassesWithCachedTextures, FrameData&);
template void CCLayerTreeHostImpl::removeRenderPasses<CCLayerTreeHostImpl::CullRenderPassesWithNoQuads>(CullRenderPassesWithNoQuads, FrameData&);

// static
template<typename RenderPassCuller>
void CCLayerTreeHostImpl::removeRenderPasses(RenderPassCuller culler, FrameData& frame)
{
    for (size_t it = culler.renderPassListBegin(frame.renderPasses); it != culler.renderPassListEnd(frame.renderPasses); it = culler.renderPassListNext(it)) {
        const CCRenderPass* currentPass = frame.renderPasses[it];
        const CCQuadList& quadList = currentPass->quadList();
        CCQuadList::constBackToFrontIterator quadListIterator = quadList.backToFrontBegin();

        for (; quadListIterator != quadList.backToFrontEnd(); ++quadListIterator) {
            CCDrawQuad* currentQuad = quadListIterator->get();

            if (currentQuad->material() != CCDrawQuad::RenderPass)
                continue;

            CCRenderPassDrawQuad* renderPassQuad = static_cast<CCRenderPassDrawQuad*>(currentQuad);
            if (!culler.shouldRemoveRenderPass(*renderPassQuad, frame))
                continue;

            // We are changing the vector in the middle of iteration. Because we
            // delete render passes that draw into the current pass, we are
            // guaranteed that any data from the iterator to the end will not
            // change. So, capture the iterator position from the end of the
            // list, and restore it after the change.
            int positionFromEnd = frame.renderPasses.size() - it;
            removeRenderPassesRecursive(renderPassQuad->renderPassId(), frame);
            it = frame.renderPasses.size() - positionFromEnd;
            ASSERT(it >= 0);
        }
    }
}

bool CCLayerTreeHostImpl::prepareToDraw(FrameData& frame)
{
    TRACE_EVENT0("cc", "CCLayerTreeHostImpl::prepareToDraw");
    ASSERT(canDraw());

    frame.renderSurfaceLayerList = &m_renderSurfaceLayerList;
    frame.renderPasses.clear();
    frame.renderPassesById.clear();
    frame.renderSurfaceLayerList->clear();
    frame.willDrawLayers.clear();

    if (!calculateRenderPasses(frame))
        return false;

    // If we return true, then we expect drawLayers() to be called before this function is called again.
    return true;
}

void CCLayerTreeHostImpl::releaseContentsTextures()
{
    if (m_contentsTexturesPurged)
        return;
    m_client->releaseContentsTexturesOnImplThread();
    setContentsTexturesPurged();
    m_client->setNeedsCommitOnImplThread();
    m_client->onCanDrawStateChanged(canDraw());
}

void CCLayerTreeHostImpl::setMemoryAllocationLimitBytes(size_t bytes)
{
    if (m_memoryAllocationLimitBytes == bytes)
        return;
    m_memoryAllocationLimitBytes = bytes;

    ASSERT(bytes);
    m_client->setNeedsCommitOnImplThread();
}

void CCLayerTreeHostImpl::onVSyncParametersChanged(double monotonicTimebase, double intervalInSeconds)
{
    m_client->onVSyncParametersChanged(monotonicTimebase, intervalInSeconds);
}

void CCLayerTreeHostImpl::drawLayers(const FrameData& frame)
{
    TRACE_EVENT0("cc", "CCLayerTreeHostImpl::drawLayers");
    ASSERT(canDraw());
    ASSERT(!frame.renderPasses.isEmpty());

    // FIXME: use the frame begin time from the overall compositor scheduler.
    // This value is currently inaccessible because it is up in Chromium's
    // RenderWidget.
    m_fpsCounter->markBeginningOfFrame(currentTime());

    if (m_settings.showDebugRects())
        m_debugRectHistory->saveDebugRectsForCurrentFrame(m_rootLayerImpl.get(), *frame.renderSurfaceLayerList, frame.occludingScreenSpaceRects, settings());

    // Because the contents of the HUD depend on everything else in the frame, the contents
    // of its texture are updated as the last thing before the frame is drawn.
    if (m_hudLayerImpl)
        m_hudLayerImpl->updateHudTexture(m_resourceProvider.get());

    m_renderer->drawFrame(frame.renderPasses, frame.renderPassesById);

    // Once a RenderPass has been drawn, its damage should be cleared in
    // case the RenderPass will be reused next frame.
    for (unsigned int i = 0; i < frame.renderPasses.size(); i++)
        frame.renderPasses[i]->setDamageRect(FloatRect());

    // The next frame should start by assuming nothing has changed, and changes are noted as they occur.
    for (unsigned int i = 0; i < frame.renderSurfaceLayerList->size(); i++)
        (*frame.renderSurfaceLayerList)[i]->renderSurface()->damageTracker()->didDrawDamagedArea();
    m_rootLayerImpl->resetAllChangeTrackingForSubtree();
}

void CCLayerTreeHostImpl::didDrawAllLayers(const FrameData& frame)
{
    for (size_t i = 0; i < frame.willDrawLayers.size(); ++i)
        frame.willDrawLayers[i]->didDraw(m_resourceProvider.get());
}

void CCLayerTreeHostImpl::finishAllRendering()
{
    if (m_renderer)
        m_renderer->finish();
}

bool CCLayerTreeHostImpl::isContextLost()
{
    return m_renderer && m_renderer->isContextLost();
}

const RendererCapabilities& CCLayerTreeHostImpl::rendererCapabilities() const
{
    return m_renderer->capabilities();
}

bool CCLayerTreeHostImpl::swapBuffers()
{
    ASSERT(m_renderer);

    m_fpsCounter->markEndOfFrame();
    return m_renderer->swapBuffers();
}

void CCLayerTreeHostImpl::didLoseContext()
{
    m_client->didLoseContextOnImplThread();
}

void CCLayerTreeHostImpl::onSwapBuffersComplete()
{
    m_client->onSwapBuffersCompleteOnImplThread();
}

void CCLayerTreeHostImpl::readback(void* pixels, const IntRect& rect)
{
    ASSERT(m_renderer);
    m_renderer->getFramebufferPixels(pixels, rect);
}

static CCLayerImpl* findRootScrollLayer(CCLayerImpl* layer)
{
    if (!layer)
        return 0;

    if (layer->scrollable())
        return layer;

    for (size_t i = 0; i < layer->children().size(); ++i) {
        CCLayerImpl* found = findRootScrollLayer(layer->children()[i].get());
        if (found)
            return found;
    }

    return 0;
}

// Content layers can be either directly scrollable or contained in an outer
// scrolling layer which applies the scroll transform. Given a content layer,
// this function returns the associated scroll layer if any.
static CCLayerImpl* findScrollLayerForContentLayer(CCLayerImpl* layerImpl)
{
    if (!layerImpl)
        return 0;

    if (layerImpl->scrollable())
        return layerImpl;

    if (layerImpl->drawsContent() && layerImpl->parent() && layerImpl->parent()->scrollable())
        return layerImpl->parent();

    return 0;
}

void CCLayerTreeHostImpl::setRootLayer(PassOwnPtr<CCLayerImpl> layer)
{
    m_rootLayerImpl = layer;
    m_rootScrollLayerImpl = findRootScrollLayer(m_rootLayerImpl.get());
    m_currentlyScrollingLayerImpl = 0;

    if (m_rootLayerImpl && m_scrollingLayerIdFromPreviousTree != -1)
        m_currentlyScrollingLayerImpl = CCLayerTreeHostCommon::findLayerInSubtree(m_rootLayerImpl.get(), m_scrollingLayerIdFromPreviousTree);

    m_scrollingLayerIdFromPreviousTree = -1;

    m_client->onCanDrawStateChanged(canDraw());
}

PassOwnPtr<CCLayerImpl> CCLayerTreeHostImpl::detachLayerTree()
{
    // Clear all data structures that have direct references to the layer tree.
    m_scrollingLayerIdFromPreviousTree = m_currentlyScrollingLayerImpl ? m_currentlyScrollingLayerImpl->id() : -1;
    m_currentlyScrollingLayerImpl = 0;
    m_renderSurfaceLayerList.clear();

    return m_rootLayerImpl.release();
}

void CCLayerTreeHostImpl::setVisible(bool visible)
{
    ASSERT(CCProxy::isImplThread());

    if (m_visible == visible)
        return;
    m_visible = visible;
    didVisibilityChange(this, m_visible);

    if (!m_renderer)
        return;

    m_renderer->setVisible(visible);

    setBackgroundTickingEnabled(!m_visible && m_needsAnimateLayers);
}

bool CCLayerTreeHostImpl::initializeRenderer(PassOwnPtr<CCGraphicsContext> context, TextureUploaderOption textureUploader)
{
    if (!context->bindToClient(this))
        return false;

    WebKit::WebGraphicsContext3D* context3d = context->context3D();

    if (!context3d) {
        // FIXME: Implement this path for software compositing.
        return false;
    }

    OwnPtr<CCGraphicsContext> contextRef(context);
    OwnPtr<CCResourceProvider> resourceProvider = CCResourceProvider::create(contextRef.get());
    OwnPtr<CCRendererGL> renderer;
    if (resourceProvider.get())
        renderer = CCRendererGL::create(this, resourceProvider.get(), textureUploader);

    // Since we now have a new context/renderer, we cannot continue to use the old
    // resources (i.e. renderSurfaces and texture IDs).
    if (m_rootLayerImpl) {
        clearRenderSurfaces();
        sendDidLoseContextRecursive(m_rootLayerImpl.get());
    }

    m_renderer = renderer.release();
    m_resourceProvider = resourceProvider.release();
    if (m_renderer)
        m_context = contextRef.release();

    if (!m_visible && m_renderer)
         m_renderer->setVisible(m_visible);

    m_client->onCanDrawStateChanged(canDraw());

    return m_renderer;
}

void CCLayerTreeHostImpl::setContentsTexturesPurged()
{
    m_contentsTexturesPurged = true;
    m_client->onCanDrawStateChanged(canDraw());
}

void CCLayerTreeHostImpl::resetContentsTexturesPurged()
{
    m_contentsTexturesPurged = false;
    m_client->onCanDrawStateChanged(canDraw());
}

void CCLayerTreeHostImpl::setViewportSize(const IntSize& layoutViewportSize, const IntSize& deviceViewportSize)
{
    if (layoutViewportSize == m_layoutViewportSize && deviceViewportSize == m_deviceViewportSize)
        return;

    m_layoutViewportSize = layoutViewportSize;
    m_deviceViewportSize = deviceViewportSize;

    updateMaxScrollPosition();

    if (m_renderer)
        m_renderer->viewportChanged();

    m_client->onCanDrawStateChanged(canDraw());
}

static void adjustScrollsForPageScaleChange(CCLayerImpl* layerImpl, float pageScaleChange)
{
    if (!layerImpl)
        return;

    if (layerImpl->scrollable()) {
        // We need to convert impl-side scroll deltas to pageScale space.
        FloatSize scrollDelta = layerImpl->scrollDelta();
        scrollDelta.scale(pageScaleChange);
        layerImpl->setScrollDelta(scrollDelta);
    }

    for (size_t i = 0; i < layerImpl->children().size(); ++i)
        adjustScrollsForPageScaleChange(layerImpl->children()[i].get(), pageScaleChange);
}

void CCLayerTreeHostImpl::setDeviceScaleFactor(float deviceScaleFactor)
{
    if (deviceScaleFactor == m_deviceScaleFactor)
        return;
    m_deviceScaleFactor = deviceScaleFactor;

    updateMaxScrollPosition();
}


void CCLayerTreeHostImpl::setPageScaleFactorAndLimits(float pageScale, float minPageScale, float maxPageScale)
{
    if (!pageScale)
        return;

    if (m_sentPageScaleDelta == 1 && pageScale == m_pageScale && minPageScale == m_minPageScale && maxPageScale == m_maxPageScale)
        return;

    m_minPageScale = minPageScale;
    m_maxPageScale = maxPageScale;

    float pageScaleChange = pageScale / m_pageScale;
    m_pageScale = pageScale;

    if (pageScaleChange != 1)
        adjustScrollsForPageScaleChange(m_rootScrollLayerImpl, pageScaleChange);

    // Clamp delta to limits and refresh display matrix.
    setPageScaleDelta(m_pageScaleDelta / m_sentPageScaleDelta);
    m_sentPageScaleDelta = 1;
    if (m_rootScrollLayerImpl)
        m_rootScrollLayerImpl->setPageScaleDelta(m_pageScaleDelta);
}

void CCLayerTreeHostImpl::setPageScaleDelta(float delta)
{
    // Clamp to the current min/max limits.
    float finalMagnifyScale = m_pageScale * delta;
    if (m_minPageScale && finalMagnifyScale < m_minPageScale)
        delta = m_minPageScale / m_pageScale;
    else if (m_maxPageScale && finalMagnifyScale > m_maxPageScale)
        delta = m_maxPageScale / m_pageScale;

    if (delta == m_pageScaleDelta)
        return;

    m_pageScaleDelta = delta;

    updateMaxScrollPosition();
    if (m_rootScrollLayerImpl)
        m_rootScrollLayerImpl->setPageScaleDelta(m_pageScaleDelta);
}

void CCLayerTreeHostImpl::updateMaxScrollPosition()
{
    if (!m_rootScrollLayerImpl || !m_rootScrollLayerImpl->children().size())
        return;

    FloatSize viewBounds = m_deviceViewportSize;
    if (CCLayerImpl* clipLayer = m_rootScrollLayerImpl->parent()) {
        // Compensate for non-overlay scrollbars.
        if (clipLayer->masksToBounds()) {
            viewBounds = clipLayer->bounds();
            viewBounds.scale(m_deviceScaleFactor);
        }
    }
    viewBounds.scale(1 / m_pageScaleDelta);

    // maxScroll is computed in physical pixels, but scroll positions are in layout pixels.
    IntSize maxScroll = contentSize() - expandedIntSize(viewBounds);
    maxScroll.scale(1 / m_deviceScaleFactor);
    // The viewport may be larger than the contents in some cases, such as
    // having a vertical scrollbar but no horizontal overflow.
    maxScroll.clampNegativeToZero();

    m_rootScrollLayerImpl->setMaxScrollPosition(maxScroll);
}

void CCLayerTreeHostImpl::setNeedsRedraw()
{
    m_client->setNeedsRedrawOnImplThread();
}

bool CCLayerTreeHostImpl::ensureRenderSurfaceLayerList()
{
    if (!m_rootLayerImpl)
        return false;
    if (!m_renderer)
        return false;

    // We need both a non-empty render surface layer list and a root render
    // surface to be able to iterate over the visible layers.
    if (m_renderSurfaceLayerList.size() && m_rootLayerImpl->renderSurface())
        return true;

    // If we are called after setRootLayer() but before prepareToDraw(), we need
    // to recalculate the visible layers. This prevents being unable to scroll
    // during part of a commit.
    m_renderSurfaceLayerList.clear();
    calculateRenderSurfaceLayerList(m_renderSurfaceLayerList);

    return m_renderSurfaceLayerList.size();
}

CCInputHandlerClient::ScrollStatus CCLayerTreeHostImpl::scrollBegin(const IntPoint& viewportPoint, CCInputHandlerClient::ScrollInputType type)
{
    TRACE_EVENT0("cc", "CCLayerTreeHostImpl::scrollBegin");

    ASSERT(!m_currentlyScrollingLayerImpl);
    clearCurrentlyScrollingLayer();

    if (!ensureRenderSurfaceLayerList())
        return ScrollIgnored;

    IntPoint deviceViewportPoint = viewportPoint;
    deviceViewportPoint.scale(m_deviceScaleFactor, m_deviceScaleFactor);

    // First find out which layer was hit from the saved list of visible layers
    // in the most recent frame.
    CCLayerImpl* layerImpl = CCLayerTreeHostCommon::findLayerThatIsHitByPoint(viewportPoint, m_renderSurfaceLayerList);

    // Walk up the hierarchy and look for a scrollable layer.
    CCLayerImpl* potentiallyScrollingLayerImpl = 0;
    for (; layerImpl; layerImpl = layerImpl->parent()) {
        // The content layer can also block attempts to scroll outside the main thread.
        if (layerImpl->tryScroll(deviceViewportPoint, type) == ScrollOnMainThread)
            return ScrollOnMainThread;

        CCLayerImpl* scrollLayerImpl = findScrollLayerForContentLayer(layerImpl);
        if (!scrollLayerImpl)
            continue;

        ScrollStatus status = scrollLayerImpl->tryScroll(viewportPoint, type);

        // If any layer wants to divert the scroll event to the main thread, abort.
        if (status == ScrollOnMainThread)
            return ScrollOnMainThread;

        if (status == ScrollStarted && !potentiallyScrollingLayerImpl)
            potentiallyScrollingLayerImpl = scrollLayerImpl;
    }

    if (potentiallyScrollingLayerImpl) {
        m_currentlyScrollingLayerImpl = potentiallyScrollingLayerImpl;
        // Gesture events need to be transformed from screen coordinates to local layer coordinates
        // so that the scrolling contents exactly follow the user's finger. In contrast, wheel
        // events are already in local layer coordinates so we can just apply them directly.
        m_scrollDeltaIsInScreenSpace = (type == Gesture);
        return ScrollStarted;
    }
    return ScrollIgnored;
}

static FloatSize scrollLayerWithScreenSpaceDelta(CCLayerImpl& layerImpl, const FloatPoint& screenSpacePoint, const FloatSize& screenSpaceDelta)
{
    // Layers with non-invertible screen space transforms should not have passed the scroll hit
    // test in the first place.
    ASSERT(layerImpl.screenSpaceTransform().isInvertible());
    WebTransformationMatrix inverseScreenSpaceTransform = layerImpl.screenSpaceTransform().inverse();

    // First project the scroll start and end points to local layer space to find the scroll delta
    // in layer coordinates.
    bool startClipped, endClipped;
    FloatPoint screenSpaceEndPoint = screenSpacePoint + screenSpaceDelta;
    FloatPoint localStartPoint = CCMathUtil::projectPoint(inverseScreenSpaceTransform, screenSpacePoint, startClipped);
    FloatPoint localEndPoint = CCMathUtil::projectPoint(inverseScreenSpaceTransform, screenSpaceEndPoint, endClipped);

    // In general scroll point coordinates should not get clipped.
    ASSERT(!startClipped);
    ASSERT(!endClipped);
    if (startClipped || endClipped)
        return FloatSize();

    // Apply the scroll delta.
    FloatSize previousDelta(layerImpl.scrollDelta());
    layerImpl.scrollBy(localEndPoint - localStartPoint);

    // Calculate the applied scroll delta in screen space coordinates.
    FloatPoint actualLocalEndPoint = localStartPoint + layerImpl.scrollDelta() - previousDelta;
    FloatPoint actualScreenSpaceEndPoint = CCMathUtil::mapPoint(layerImpl.screenSpaceTransform(), actualLocalEndPoint, endClipped);
    ASSERT(!endClipped);
    if (endClipped)
        return FloatSize();
    return actualScreenSpaceEndPoint - screenSpacePoint;
}

static FloatSize scrollLayerWithLocalDelta(CCLayerImpl& layerImpl, const FloatSize& localDelta)
{
    FloatSize previousDelta(layerImpl.scrollDelta());
    layerImpl.scrollBy(localDelta);
    return layerImpl.scrollDelta() - previousDelta;
}

void CCLayerTreeHostImpl::scrollBy(const IntPoint& viewportPoint, const IntSize& scrollDelta)
{
    TRACE_EVENT0("cc", "CCLayerTreeHostImpl::scrollBy");
    if (!m_currentlyScrollingLayerImpl)
        return;

    FloatSize pendingDelta(scrollDelta);

    pendingDelta.scale(m_deviceScaleFactor);

    for (CCLayerImpl* layerImpl = m_currentlyScrollingLayerImpl; layerImpl; layerImpl = layerImpl->parent()) {
        if (!layerImpl->scrollable())
            continue;

        FloatSize appliedDelta;
        if (m_scrollDeltaIsInScreenSpace)
            appliedDelta = scrollLayerWithScreenSpaceDelta(*layerImpl, viewportPoint, pendingDelta);
        else
            appliedDelta = scrollLayerWithLocalDelta(*layerImpl, pendingDelta);

        // If the layer wasn't able to move, try the next one in the hierarchy.
        float moveThresholdSquared = 0.1f * 0.1f;
        if (appliedDelta.diagonalLengthSquared() < moveThresholdSquared)
            continue;

        // If the applied delta is within 45 degrees of the input delta, bail out to make it easier
        // to scroll just one layer in one direction without affecting any of its parents.
        float angleThreshold = 45;
        if (CCMathUtil::smallestAngleBetweenVectors(appliedDelta, pendingDelta) < angleThreshold) {
            pendingDelta = FloatSize();
            break;
        }

        // Allow further movement only on an axis perpendicular to the direction in which the layer
        // moved.
        FloatSize perpendicularAxis(-appliedDelta.height(), appliedDelta.width());
        pendingDelta = CCMathUtil::projectVector(pendingDelta, perpendicularAxis);

        if (flooredIntSize(pendingDelta).isZero())
            break;
    }

    if (!scrollDelta.isZero() && flooredIntSize(pendingDelta).isEmpty()) {
        m_client->setNeedsCommitOnImplThread();
        m_client->setNeedsRedrawOnImplThread();
    }
}

void CCLayerTreeHostImpl::clearCurrentlyScrollingLayer()
{
    m_currentlyScrollingLayerImpl = 0;
    m_scrollingLayerIdFromPreviousTree = -1;
}

void CCLayerTreeHostImpl::scrollEnd()
{
    clearCurrentlyScrollingLayer();
}

void CCLayerTreeHostImpl::pinchGestureBegin()
{
    m_pinchGestureActive = true;
    m_previousPinchAnchor = IntPoint();

    if (m_rootScrollLayerImpl && m_rootScrollLayerImpl->scrollbarAnimationController())
        m_rootScrollLayerImpl->scrollbarAnimationController()->didPinchGestureBegin();
}

void CCLayerTreeHostImpl::pinchGestureUpdate(float magnifyDelta,
                                             const IntPoint& anchor)
{
    TRACE_EVENT0("cc", "CCLayerTreeHostImpl::pinchGestureUpdate");

    if (!m_rootScrollLayerImpl)
        return;

    if (m_previousPinchAnchor == IntPoint::zero())
        m_previousPinchAnchor = anchor;

    // Keep the center-of-pinch anchor specified by (x, y) in a stable
    // position over the course of the magnify.
    FloatPoint previousScaleAnchor(m_previousPinchAnchor.x() / m_pageScaleDelta, m_previousPinchAnchor.y() / m_pageScaleDelta);
    setPageScaleDelta(m_pageScaleDelta * magnifyDelta);
    FloatPoint newScaleAnchor(anchor.x() / m_pageScaleDelta, anchor.y() / m_pageScaleDelta);
    FloatSize move = previousScaleAnchor - newScaleAnchor;

    m_previousPinchAnchor = anchor;

    m_rootScrollLayerImpl->scrollBy(roundedIntSize(move));

    if (m_rootScrollLayerImpl->scrollbarAnimationController())
        m_rootScrollLayerImpl->scrollbarAnimationController()->didPinchGestureUpdate();

    m_client->setNeedsCommitOnImplThread();
    m_client->setNeedsRedrawOnImplThread();
}

void CCLayerTreeHostImpl::pinchGestureEnd()
{
    m_pinchGestureActive = false;

    if (m_rootScrollLayerImpl && m_rootScrollLayerImpl->scrollbarAnimationController())
        m_rootScrollLayerImpl->scrollbarAnimationController()->didPinchGestureEnd();

    m_client->setNeedsCommitOnImplThread();
}

void CCLayerTreeHostImpl::computeDoubleTapZoomDeltas(CCScrollAndScaleSet* scrollInfo)
{
    float pageScale = m_pageScaleAnimation->finalPageScale();
    IntSize scrollOffset = m_pageScaleAnimation->finalScrollOffset();
    scrollOffset.scale(m_pageScale / pageScale);
    makeScrollAndScaleSet(scrollInfo, scrollOffset, pageScale);
}

void CCLayerTreeHostImpl::computePinchZoomDeltas(CCScrollAndScaleSet* scrollInfo)
{
    if (!m_rootScrollLayerImpl)
        return;

    // Only send fake scroll/zoom deltas if we're pinch zooming out by a
    // significant amount. This also ensures only one fake delta set will be
    // sent.
    const float pinchZoomOutSensitivity = 0.95f;
    if (m_pageScaleDelta > pinchZoomOutSensitivity)
        return;

    // Compute where the scroll offset/page scale would be if fully pinch-zoomed
    // out from the anchor point.
    IntSize scrollBegin = flooredIntSize(m_rootScrollLayerImpl->scrollPosition() + m_rootScrollLayerImpl->scrollDelta());
    scrollBegin.scale(m_pageScaleDelta);
    float scaleBegin = m_pageScale * m_pageScaleDelta;
    float pageScaleDeltaToSend = m_minPageScale / m_pageScale;
    FloatSize scaledContentsSize = contentSize();
    scaledContentsSize.scale(pageScaleDeltaToSend);

    FloatSize anchor = toSize(m_previousPinchAnchor);
    FloatSize scrollEnd = scrollBegin + anchor;
    scrollEnd.scale(m_minPageScale / scaleBegin);
    scrollEnd -= anchor;
    scrollEnd = scrollEnd.shrunkTo(roundedIntSize(scaledContentsSize - m_deviceViewportSize)).expandedTo(FloatSize(0, 0));
    scrollEnd.scale(1 / pageScaleDeltaToSend);
    scrollEnd.scale(m_deviceScaleFactor);

    makeScrollAndScaleSet(scrollInfo, roundedIntSize(scrollEnd), m_minPageScale);
}

void CCLayerTreeHostImpl::makeScrollAndScaleSet(CCScrollAndScaleSet* scrollInfo, const IntSize& scrollOffset, float pageScale)
{
    if (!m_rootScrollLayerImpl)
        return;

    CCLayerTreeHostCommon::ScrollUpdateInfo scroll;
    scroll.layerId = m_rootScrollLayerImpl->id();
    scroll.scrollDelta = scrollOffset - toSize(m_rootScrollLayerImpl->scrollPosition());
    scrollInfo->scrolls.append(scroll);
    m_rootScrollLayerImpl->setSentScrollDelta(scroll.scrollDelta);
    m_sentPageScaleDelta = scrollInfo->pageScaleDelta = pageScale / m_pageScale;
}

static void collectScrollDeltas(CCScrollAndScaleSet* scrollInfo, CCLayerImpl* layerImpl)
{
    if (!layerImpl)
        return;

    if (!layerImpl->scrollDelta().isZero()) {
        IntSize scrollDelta = flooredIntSize(layerImpl->scrollDelta());
        CCLayerTreeHostCommon::ScrollUpdateInfo scroll;
        scroll.layerId = layerImpl->id();
        scroll.scrollDelta = scrollDelta;
        scrollInfo->scrolls.append(scroll);
        layerImpl->setSentScrollDelta(scrollDelta);
    }

    for (size_t i = 0; i < layerImpl->children().size(); ++i)
        collectScrollDeltas(scrollInfo, layerImpl->children()[i].get());
}

PassOwnPtr<CCScrollAndScaleSet> CCLayerTreeHostImpl::processScrollDeltas()
{
    OwnPtr<CCScrollAndScaleSet> scrollInfo = adoptPtr(new CCScrollAndScaleSet());

    if (m_pinchGestureActive || m_pageScaleAnimation) {
        m_sentPageScaleDelta = scrollInfo->pageScaleDelta = 1;
        if (m_pinchGestureActive)
            computePinchZoomDeltas(scrollInfo.get());
        else if (m_pageScaleAnimation.get())
            computeDoubleTapZoomDeltas(scrollInfo.get());
        return scrollInfo.release();
    }

    collectScrollDeltas(scrollInfo.get(), m_rootLayerImpl.get());
    m_sentPageScaleDelta = scrollInfo->pageScaleDelta = m_pageScaleDelta;

    return scrollInfo.release();
}

void CCLayerTreeHostImpl::setFullRootLayerDamage()
{
    if (m_rootLayerImpl) {
        CCRenderSurface* renderSurface = m_rootLayerImpl->renderSurface();
        if (renderSurface)
            renderSurface->damageTracker()->forceFullDamageNextUpdate();
    }
}

void CCLayerTreeHostImpl::animatePageScale(double monotonicTime)
{
    if (!m_pageScaleAnimation || !m_rootScrollLayerImpl)
        return;

    IntSize scrollTotal = flooredIntSize(m_rootScrollLayerImpl->scrollPosition() + m_rootScrollLayerImpl->scrollDelta());

    setPageScaleDelta(m_pageScaleAnimation->pageScaleAtTime(monotonicTime) / m_pageScale);
    IntSize nextScroll = m_pageScaleAnimation->scrollOffsetAtTime(monotonicTime);
    nextScroll.scale(1 / m_pageScaleDelta);
    m_rootScrollLayerImpl->scrollBy(nextScroll - scrollTotal);
    m_client->setNeedsRedrawOnImplThread();

    if (m_pageScaleAnimation->isAnimationCompleteAtTime(monotonicTime)) {
        m_pageScaleAnimation.clear();
        m_client->setNeedsCommitOnImplThread();
    }
}

void CCLayerTreeHostImpl::animateLayers(double monotonicTime, double wallClockTime)
{
    if (!CCSettings::acceleratedAnimationEnabled() || !m_needsAnimateLayers || !m_rootLayerImpl)
        return;

    TRACE_EVENT0("cc", "CCLayerTreeHostImpl::animateLayers");

    OwnPtr<CCAnimationEventsVector> events(adoptPtr(new CCAnimationEventsVector));

    bool didAnimate = false;
    animateLayersRecursive(m_rootLayerImpl.get(), monotonicTime, wallClockTime, events.get(), didAnimate, m_needsAnimateLayers);

    if (!events->isEmpty())
        m_client->postAnimationEventsToMainThreadOnImplThread(events.release(), wallClockTime);

    if (didAnimate)
        m_client->setNeedsRedrawOnImplThread();

    setBackgroundTickingEnabled(!m_visible && m_needsAnimateLayers);
}

double CCLayerTreeHostImpl::lowFrequencyAnimationInterval() const
{
    return 1;
}

void CCLayerTreeHostImpl::sendDidLoseContextRecursive(CCLayerImpl* current)
{
    ASSERT(current);
    current->didLoseContext();
    if (current->maskLayer())
        sendDidLoseContextRecursive(current->maskLayer());
    if (current->replicaLayer())
        sendDidLoseContextRecursive(current->replicaLayer());
    for (size_t i = 0; i < current->children().size(); ++i)
        sendDidLoseContextRecursive(current->children()[i].get());
}

static void clearRenderSurfacesOnCCLayerImplRecursive(CCLayerImpl* current)
{
    ASSERT(current);
    for (size_t i = 0; i < current->children().size(); ++i)
        clearRenderSurfacesOnCCLayerImplRecursive(current->children()[i].get());
    current->clearRenderSurface();
}

void CCLayerTreeHostImpl::clearRenderSurfaces()
{
    clearRenderSurfacesOnCCLayerImplRecursive(m_rootLayerImpl.get());
    m_renderSurfaceLayerList.clear();
}

String CCLayerTreeHostImpl::layerTreeAsText() const
{
    TextStream ts;
    if (m_rootLayerImpl) {
        ts << m_rootLayerImpl->layerTreeAsText();
        ts << "RenderSurfaces:\n";
        dumpRenderSurfaces(ts, 1, m_rootLayerImpl.get());
    }
    return ts.release();
}

void CCLayerTreeHostImpl::dumpRenderSurfaces(TextStream& ts, int indent, const CCLayerImpl* layer) const
{
    if (layer->renderSurface())
        layer->renderSurface()->dumpSurface(ts, indent);

    for (size_t i = 0; i < layer->children().size(); ++i)
        dumpRenderSurfaces(ts, indent, layer->children()[i].get());
}

int CCLayerTreeHostImpl::sourceAnimationFrameNumber() const
{
    return fpsCounter()->currentFrameNumber();
}

void CCLayerTreeHostImpl::renderingStats(CCRenderingStats& stats) const
{
    stats.numFramesSentToScreen = fpsCounter()->currentFrameNumber();
    stats.droppedFrameCount = fpsCounter()->droppedFrameCount();
}

void CCLayerTreeHostImpl::animateScrollbars(double monotonicTime)
{
    animateScrollbarsRecursive(m_rootLayerImpl.get(), monotonicTime);
}

void CCLayerTreeHostImpl::animateScrollbarsRecursive(CCLayerImpl* layer, double monotonicTime)
{
    if (!layer)
        return;

    CCScrollbarAnimationController* scrollbarController = layer->scrollbarAnimationController();
    if (scrollbarController && scrollbarController->animate(monotonicTime))
        m_client->setNeedsRedrawOnImplThread();

    for (size_t i = 0; i < layer->children().size(); ++i)
        animateScrollbarsRecursive(layer->children()[i].get(), monotonicTime);
}

} // namespace WebCore
