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

#include "cc/CCLayerTreeHostImpl.h"

#include "Extensions3D.h"
#include "LayerRendererChromium.h"
#include "TraceEvent.h"
#include "cc/CCActiveGestureAnimation.h"
#include "cc/CCDamageTracker.h"
#include "cc/CCDebugRectHistory.h"
#include "cc/CCDelayBasedTimeSource.h"
#include "cc/CCFontAtlas.h"
#include "cc/CCFrameRateCounter.h"
#include "cc/CCGestureCurve.h"
#include "cc/CCHeadsUpDisplay.h"
#include "cc/CCLayerIterator.h"
#include "cc/CCLayerTreeHost.h"
#include "cc/CCLayerTreeHostCommon.h"
#include "cc/CCMathUtil.h"
#include "cc/CCPageScaleAnimation.h"
#include "cc/CCSettings.h"
#include "cc/CCSingleThreadProxy.h"
#include "cc/CCThreadTask.h"
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
    , m_frameNumber(0)
    , m_scrollLayerImpl(0)
    , m_settings(settings)
    , m_visible(true)
    , m_sourceFrameCanBeDrawn(true)
    , m_headsUpDisplay(CCHeadsUpDisplay::create())
    , m_pageScale(1)
    , m_pageScaleDelta(1)
    , m_sentPageScaleDelta(1)
    , m_minPageScale(0)
    , m_maxPageScale(0)
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
    TRACE_EVENT("CCLayerTreeHostImpl::~CCLayerTreeHostImpl()", this, 0);

    if (m_rootLayerImpl)
        clearRenderSurfaces();
}

void CCLayerTreeHostImpl::beginCommit()
{
}

void CCLayerTreeHostImpl::commitComplete()
{
    // Recompute max scroll position; must be after layer content bounds are
    // updated.
    updateMaxScrollPosition();
}

bool CCLayerTreeHostImpl::canDraw()
{
    if (!m_rootLayerImpl)
        return false;
    if (viewportSize().isEmpty())
        return false;
    if (!m_layerRenderer)
        return false;
    if (!m_sourceFrameCanBeDrawn)
        return false;
    return true;
}

CCGraphicsContext* CCLayerTreeHostImpl::context()
{
    return m_context.get();
}

void CCLayerTreeHostImpl::animate(double monotonicTime, double wallClockTime)
{
    animatePageScale(monotonicTime);
    animateLayers(monotonicTime, wallClockTime);
    animateGestures(monotonicTime);
}

void CCLayerTreeHostImpl::startPageScaleAnimation(const IntSize& targetPosition, bool anchorPoint, float pageScale, double startTime, double duration)
{
    if (!m_scrollLayerImpl)
        return;

    IntSize scrollTotal = flooredIntSize(m_scrollLayerImpl->scrollPosition() + m_scrollLayerImpl->scrollDelta());
    scrollTotal.scale(m_pageScaleDelta);
    float scaleTotal = m_pageScale * m_pageScaleDelta;
    IntSize scaledContentSize = contentSize();
    scaledContentSize.scale(m_pageScaleDelta);

    m_pageScaleAnimation = CCPageScaleAnimation::create(scrollTotal, scaleTotal, m_viewportSize, scaledContentSize, startTime);

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

void CCLayerTreeHostImpl::setActiveGestureAnimation(PassOwnPtr<CCActiveGestureAnimation> gestureAnimation)
{
    m_activeGestureAnimation = gestureAnimation;

    if (m_activeGestureAnimation)
        m_client->setNeedsRedrawOnImplThread();
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

    renderSurfaceLayerList.append(m_rootLayerImpl.get());

    if (!m_rootLayerImpl->renderSurface())
        m_rootLayerImpl->createRenderSurface();
    m_rootLayerImpl->renderSurface()->clearLayerList();
    m_rootLayerImpl->renderSurface()->setContentRect(IntRect(IntPoint(), deviceViewportSize()));

    m_rootLayerImpl->setClipRect(IntRect(IntPoint(), deviceViewportSize()));

    {
        TRACE_EVENT("CCLayerTreeHostImpl::calcDrawEtc", this, 0);
        WebTransformationMatrix identityMatrix;
        WebTransformationMatrix deviceScaleTransform;
        deviceScaleTransform.scale(m_settings.deviceScaleFactor);
        CCLayerTreeHostCommon::calculateDrawTransforms(m_rootLayerImpl.get(), m_rootLayerImpl.get(), deviceScaleTransform, identityMatrix, renderSurfaceLayerList, m_rootLayerImpl->renderSurface()->layerList(), &m_layerSorter, layerRendererCapabilities().maxTextureSize);

        if (layerRendererCapabilities().usingPartialSwap || settings().showSurfaceDamageRects)
            trackDamageForAllSurfaces(m_rootLayerImpl.get(), renderSurfaceLayerList);
        m_rootScissorRect = m_rootLayerImpl->renderSurface()->damageTracker()->currentDamageRect();

        if (!layerRendererCapabilities().usingPartialSwap)
            m_rootScissorRect = FloatRect(FloatPoint(0, 0), deviceViewportSize());

        CCLayerTreeHostCommon::calculateVisibleAndScissorRects(renderSurfaceLayerList, m_rootScissorRect);
    }
}

bool CCLayerTreeHostImpl::calculateRenderPasses(CCRenderPassList& passes, CCLayerList& renderSurfaceLayerList, CCLayerList& willDrawLayers)
{
    ASSERT(passes.isEmpty());

    calculateRenderSurfaceLayerList(renderSurfaceLayerList);

    TRACE_EVENT1("webkit", "CCLayerTreeHostImpl::calculateRenderPasses", "renderSurfaceLayerList.size()", static_cast<long long unsigned>(renderSurfaceLayerList.size()));

    m_rootLayerImpl->setScissorRect(enclosingIntRect(m_rootScissorRect));

    // Create the render passes in dependency order.
    HashMap<CCRenderSurface*, CCRenderPass*> surfacePassMap;
    for (int surfaceIndex = renderSurfaceLayerList.size() - 1; surfaceIndex >= 0 ; --surfaceIndex) {
        CCLayerImpl* renderSurfaceLayer = renderSurfaceLayerList[surfaceIndex];
        CCRenderSurface* renderSurface = renderSurfaceLayer->renderSurface();

        OwnPtr<CCRenderPass> pass = CCRenderPass::create(renderSurface);
        surfacePassMap.add(renderSurface, pass.get());
        passes.append(pass.release());
    }

    bool recordMetricsForFrame = true; // FIXME: In the future, disable this when about:tracing is off.
    CCOcclusionTrackerImpl occlusionTracker(enclosingIntRect(m_rootScissorRect), recordMetricsForFrame);
    occlusionTracker.setMinimumTrackingSize(CCOcclusionTrackerImpl::preferredMinimumTrackingSize());

    // Add quads to the Render passes in FrontToBack order to allow for testing occlusion and performing culling during the tree walk.
    typedef CCLayerIterator<CCLayerImpl, Vector<CCLayerImpl*>, CCRenderSurface, CCLayerIteratorActions::FrontToBack> CCLayerIteratorType;

    // Typically when we are missing a texture and use a checkerboard quad, we still draw the frame. However when the layer being
    // checkerboarded is moving due to an impl-animation, we drop the frame to avoid flashing due to the texture suddenly appearing
    // in the future.
    bool drawFrame = true;

    CCLayerIteratorType end = CCLayerIteratorType::end(&renderSurfaceLayerList);
    for (CCLayerIteratorType it = CCLayerIteratorType::begin(&renderSurfaceLayerList); it != end; ++it) {
        CCRenderSurface* renderSurface = it.targetRenderSurfaceLayer()->renderSurface();
        CCRenderPass* pass = surfacePassMap.get(renderSurface);
        bool hadMissingTiles = false;

        occlusionTracker.enterLayer(it);

        if (it.representsContributingRenderSurface() && !it->renderSurface()->scissorRect().isEmpty()) {
            CCRenderPass* contributingRenderPass = surfacePassMap.get(it->renderSurface());
            pass->appendQuadsForRenderSurfaceLayer(*it, contributingRenderPass, &occlusionTracker);
        } else if (it.representsItself() && !occlusionTracker.occluded(*it, it->visibleLayerRect()) && !it->visibleLayerRect().isEmpty() && !it->scissorRect().isEmpty()) {
            it->willDraw(m_layerRenderer.get(), context());
            willDrawLayers.append(*it);

            pass->appendQuadsForLayer(*it, &occlusionTracker, hadMissingTiles);
        }

        if (hadMissingTiles) {
            bool layerHasAnimatingTransform = it->screenSpaceTransformIsAnimating() || it->drawTransformIsAnimating();
            if (layerHasAnimatingTransform)
                drawFrame = false;
        }

        occlusionTracker.leaveLayer(it);
    }

    if (!m_hasTransparentBackground) {
        passes.last()->setHasTransparentBackground(false);
        passes.last()->appendQuadsToFillScreen(m_rootLayerImpl.get(), m_backgroundColor, occlusionTracker);
    }

    if (drawFrame)
        occlusionTracker.overdrawMetrics().recordMetrics(this);
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
    if (!m_scrollLayerImpl || m_scrollLayerImpl->children().isEmpty())
        return IntSize();
    return m_scrollLayerImpl->children()[0]->contentBounds();
}

bool CCLayerTreeHostImpl::prepareToDraw(FrameData& frame)
{
    TRACE_EVENT("CCLayerTreeHostImpl::prepareToDraw", this, 0);
    ASSERT(canDraw());

    frame.renderPasses.clear();
    frame.renderSurfaceLayerList.clear();
    frame.willDrawLayers.clear();

    if (!calculateRenderPasses(frame.renderPasses, frame.renderSurfaceLayerList, frame.willDrawLayers))
        return false;

    // If we return true, then we expect drawLayers() to be called before this function is called again.
    return true;
}

void CCLayerTreeHostImpl::setContentsMemoryAllocationLimitBytes(size_t bytes)
{
    m_client->postSetContentsMemoryAllocationLimitBytesToMainThreadOnImplThread(bytes);
}

void CCLayerTreeHostImpl::drawLayers(const FrameData& frame)
{
    TRACE_EVENT("CCLayerTreeHostImpl::drawLayers", this, 0);
    ASSERT(canDraw());
    ASSERT(!frame.renderPasses.isEmpty());

    // FIXME: use the frame begin time from the overall compositor scheduler.
    // This value is currently inaccessible because it is up in Chromium's
    // RenderWidget.

    // The root RenderPass is the last one to be drawn.
    CCRenderPass* rootRenderPass = frame.renderPasses.last().get();

    m_fpsCounter->markBeginningOfFrame(currentTime());
    m_layerRenderer->beginDrawingFrame(rootRenderPass);

    for (size_t i = 0; i < frame.renderPasses.size(); ++i) {
        CCRenderPass* renderPass = frame.renderPasses[i].get();

        FloatRect rootScissorRectInCurrentSurface = renderPass->targetSurface()->computeRootScissorRectInCurrentSurface(m_rootScissorRect);
        m_layerRenderer->drawRenderPass(renderPass, rootScissorRectInCurrentSurface);

        renderPass->targetSurface()->damageTracker()->didDrawDamagedArea();
    }

    if (m_debugRectHistory->enabled(settings()))
        m_debugRectHistory->saveDebugRectsForCurrentFrame(m_rootLayerImpl.get(), frame.renderSurfaceLayerList, settings());

    if (m_headsUpDisplay->enabled(settings()))
        m_headsUpDisplay->draw(this);

    m_layerRenderer->finishDrawingFrame();

    ++m_frameNumber;

    // The next frame should start by assuming nothing has changed, and changes are noted as they occur.
    m_rootLayerImpl->resetAllChangeTrackingForSubtree();
}

void CCLayerTreeHostImpl::didDrawAllLayers(const FrameData& frame)
{
    for (size_t i = 0; i < frame.willDrawLayers.size(); ++i)
        frame.willDrawLayers[i]->didDraw();
}

void CCLayerTreeHostImpl::finishAllRendering()
{
    if (m_layerRenderer)
        m_layerRenderer->finish();
}

bool CCLayerTreeHostImpl::isContextLost()
{
    return m_layerRenderer && m_layerRenderer->isContextLost();
}

const LayerRendererCapabilities& CCLayerTreeHostImpl::layerRendererCapabilities() const
{
    return m_layerRenderer->capabilities();
}

TextureAllocator* CCLayerTreeHostImpl::contentsTextureAllocator() const
{
    return m_layerRenderer ? m_layerRenderer->contentsTextureAllocator() : 0;
}

bool CCLayerTreeHostImpl::swapBuffers()
{
    ASSERT(m_layerRenderer);

    m_fpsCounter->markEndOfFrame();
    return m_layerRenderer->swapBuffers(enclosingIntRect(m_rootScissorRect));
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
    ASSERT(m_layerRenderer);
    m_layerRenderer->getFramebufferPixels(pixels, rect);
}

static CCLayerImpl* findScrollLayer(CCLayerImpl* layer)
{
    if (!layer)
        return 0;

    if (layer->scrollable())
        return layer;

    for (size_t i = 0; i < layer->children().size(); ++i) {
        CCLayerImpl* found = findScrollLayer(layer->children()[i].get());
        if (found)
            return found;
    }

    return 0;
}

void CCLayerTreeHostImpl::setRootLayer(PassOwnPtr<CCLayerImpl> layer)
{
    m_rootLayerImpl = layer;

    // FIXME: Currently, this only finds the first scrollable layer.
    m_scrollLayerImpl = findScrollLayer(m_rootLayerImpl.get());
}

void CCLayerTreeHostImpl::setVisible(bool visible)
{
    ASSERT(CCProxy::isImplThread());

    if (m_visible == visible)
        return;
    m_visible = visible;
    didVisibilityChange(this, m_visible);

    if (!m_layerRenderer)
        return;

    m_layerRenderer->setVisible(visible);

    setBackgroundTickingEnabled(!m_visible && m_needsAnimateLayers);
}

bool CCLayerTreeHostImpl::initializeLayerRenderer(PassRefPtr<CCGraphicsContext> context, TextureUploaderOption textureUploader)
{
    GraphicsContext3D* context3d = context->context3D();
    if (!context3d) {
        // FIXME: Implement this path for software compositing.
        return false;
    }

    OwnPtr<LayerRendererChromium> layerRenderer;
    layerRenderer = LayerRendererChromium::create(this, context3d, textureUploader);

    // Since we now have a new context/layerRenderer, we cannot continue to use the old
    // resources (i.e. renderSurfaces and texture IDs).
    if (m_rootLayerImpl) {
        clearRenderSurfaces();
        sendDidLoseContextRecursive(m_rootLayerImpl.get());
    }

    m_layerRenderer = layerRenderer.release();
    if (m_layerRenderer)
        m_context = context;

    if (!m_visible && m_layerRenderer)
         m_layerRenderer->setVisible(m_visible);

    return m_layerRenderer;
}

void CCLayerTreeHostImpl::setViewportSize(const IntSize& viewportSize)
{
    if (viewportSize == m_viewportSize)
        return;

    m_viewportSize = viewportSize;

    m_deviceViewportSize = viewportSize;
    m_deviceViewportSize.scale(m_settings.deviceScaleFactor);

    updateMaxScrollPosition();

    if (m_layerRenderer)
        m_layerRenderer->viewportChanged();
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

    adjustScrollsForPageScaleChange(pageScaleChange);

    // Clamp delta to limits and refresh display matrix.
    setPageScaleDelta(m_pageScaleDelta / m_sentPageScaleDelta);
    m_sentPageScaleDelta = 1;
    applyPageScaleDeltaToScrollLayer();
}

void CCLayerTreeHostImpl::adjustScrollsForPageScaleChange(float pageScaleChange)
{
    if (pageScaleChange == 1)
        return;

    // We also need to convert impl-side scroll deltas to pageScale space.
    if (m_scrollLayerImpl) {
        FloatSize scrollDelta = m_scrollLayerImpl->scrollDelta();
        scrollDelta.scale(pageScaleChange);
        m_scrollLayerImpl->setScrollDelta(scrollDelta);
    }
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
    applyPageScaleDeltaToScrollLayer();
}

void CCLayerTreeHostImpl::applyPageScaleDeltaToScrollLayer()
{
    if (m_scrollLayerImpl)
        m_scrollLayerImpl->setPageScaleDelta(m_pageScaleDelta);
}

void CCLayerTreeHostImpl::updateMaxScrollPosition()
{
    if (!m_scrollLayerImpl || !m_scrollLayerImpl->children().size())
        return;

    FloatSize viewBounds = m_viewportSize;
    if (CCLayerImpl* clipLayer = m_scrollLayerImpl->parent()) {
        if (clipLayer->masksToBounds())
            viewBounds = clipLayer->bounds();
    }
    viewBounds.scale(1 / m_pageScaleDelta);
    viewBounds.scale(m_settings.deviceScaleFactor);

    // maxScroll is computed in physical pixels, but scroll positions are in layout pixels.
    IntSize maxScroll = contentSize() - expandedIntSize(viewBounds);
    maxScroll.scale(1 / m_settings.deviceScaleFactor);
    // The viewport may be larger than the contents in some cases, such as
    // having a vertical scrollbar but no horizontal overflow.
    maxScroll.clampNegativeToZero();

    m_scrollLayerImpl->setMaxScrollPosition(maxScroll);

    // TODO(aelias): Also update sublayers.
}

void CCLayerTreeHostImpl::setNeedsRedraw()
{
    m_client->setNeedsRedrawOnImplThread();
}

CCInputHandlerClient::ScrollStatus CCLayerTreeHostImpl::scrollBegin(const IntPoint& viewportPoint, CCInputHandlerClient::ScrollInputType type)
{
    // TODO: Check for scrollable sublayers.
    if (!m_scrollLayerImpl || !m_scrollLayerImpl->scrollable()) {
        TRACE_EVENT("scrollBegin Ignored no scrollable", this, 0);
        return ScrollIgnored;
    }

    if (m_scrollLayerImpl->shouldScrollOnMainThread()) {
        TRACE_EVENT("scrollBegin Failed shouldScrollOnMainThread", this, 0);
        return ScrollFailed;
    }

    IntPoint deviceViewportPoint = viewportPoint;
    deviceViewportPoint.scale(m_settings.deviceScaleFactor, m_settings.deviceScaleFactor);

    // The inverse of the screen space transform takes us from physical pixels to layout pixels.
    IntPoint scrollLayerPoint(m_scrollLayerImpl->screenSpaceTransform().inverse().mapPoint(deviceViewportPoint));

    if (m_scrollLayerImpl->nonFastScrollableRegion().contains(scrollLayerPoint)) {
        TRACE_EVENT("scrollBegin Failed nonFastScrollableRegion", this, 0);
        return ScrollFailed;
    }

    if (type == CCInputHandlerClient::Wheel && m_scrollLayerImpl->haveWheelEventHandlers()) {
        TRACE_EVENT("scrollBegin Failed wheelEventHandlers", this, 0);
        return ScrollFailed;
    }

    return ScrollStarted;
}

void CCLayerTreeHostImpl::scrollBy(const IntSize& scrollDelta)
{
    TRACE_EVENT("CCLayerTreeHostImpl::scrollBy", this, 0);
    if (!m_scrollLayerImpl)
        return;

    m_scrollLayerImpl->scrollBy(scrollDelta);
    m_client->setNeedsCommitOnImplThread();
    m_client->setNeedsRedrawOnImplThread();
}

void CCLayerTreeHostImpl::scrollEnd()
{
}

void CCLayerTreeHostImpl::pinchGestureBegin()
{
    m_pinchGestureActive = true;
    m_previousPinchAnchor = IntPoint();
}

void CCLayerTreeHostImpl::pinchGestureUpdate(float magnifyDelta,
                                             const IntPoint& anchor)
{
    TRACE_EVENT("CCLayerTreeHostImpl::pinchGestureUpdate", this, 0);

    if (!m_scrollLayerImpl)
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

    m_scrollLayerImpl->scrollBy(roundedIntSize(move));
    m_client->setNeedsCommitOnImplThread();
    m_client->setNeedsRedrawOnImplThread();
}

void CCLayerTreeHostImpl::pinchGestureEnd()
{
    m_pinchGestureActive = false;

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
    if (!m_scrollLayerImpl)
        return;

    // Only send fake scroll/zoom deltas if we're pinch zooming out by a
    // significant amount. This also ensures only one fake delta set will be
    // sent.
    const float pinchZoomOutSensitivity = 0.95;
    if (m_pageScaleDelta > pinchZoomOutSensitivity)
        return;

    // Compute where the scroll offset/page scale would be if fully pinch-zoomed
    // out from the anchor point.
    IntSize scrollBegin = flooredIntSize(m_scrollLayerImpl->scrollPosition() + m_scrollLayerImpl->scrollDelta());
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
    scrollEnd.scale(m_settings.deviceScaleFactor);

    makeScrollAndScaleSet(scrollInfo, roundedIntSize(scrollEnd), m_minPageScale);
}

void CCLayerTreeHostImpl::makeScrollAndScaleSet(CCScrollAndScaleSet* scrollInfo, const IntSize& scrollOffset, float pageScale)
{
    if (!m_scrollLayerImpl)
        return;

    CCLayerTreeHostCommon::ScrollUpdateInfo scroll;
    scroll.layerId = m_scrollLayerImpl->id();
    scroll.scrollDelta = scrollOffset - toSize(m_scrollLayerImpl->scrollPosition());
    scrollInfo->scrolls.append(scroll);
    m_scrollLayerImpl->setSentScrollDelta(scroll.scrollDelta);
    m_sentPageScaleDelta = scrollInfo->pageScaleDelta = pageScale / m_pageScale;
}

PassOwnPtr<CCScrollAndScaleSet> CCLayerTreeHostImpl::processScrollDeltas()
{
    OwnPtr<CCScrollAndScaleSet> scrollInfo = adoptPtr(new CCScrollAndScaleSet());
    bool didMove = m_scrollLayerImpl && (!m_scrollLayerImpl->scrollDelta().isZero() || m_pageScaleDelta != 1.0f);
    if (!didMove || m_pinchGestureActive || m_pageScaleAnimation) {
        m_sentPageScaleDelta = scrollInfo->pageScaleDelta = 1;
        if (m_pinchGestureActive)
            computePinchZoomDeltas(scrollInfo.get());
        else if (m_pageScaleAnimation.get())
            computeDoubleTapZoomDeltas(scrollInfo.get());
        return scrollInfo.release();
    }

    m_sentPageScaleDelta = scrollInfo->pageScaleDelta = m_pageScaleDelta;

    // FIXME: track scrolls from layers other than the root
    CCLayerTreeHostCommon::ScrollUpdateInfo scroll;
    scroll.layerId = m_scrollLayerImpl->id();
    scroll.scrollDelta = flooredIntSize(m_scrollLayerImpl->scrollDelta());
    scrollInfo->scrolls.append(scroll);

    m_scrollLayerImpl->setSentScrollDelta(scroll.scrollDelta);

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
    if (!m_pageScaleAnimation || !m_scrollLayerImpl)
        return;

    IntSize scrollTotal = flooredIntSize(m_scrollLayerImpl->scrollPosition() + m_scrollLayerImpl->scrollDelta());

    setPageScaleDelta(m_pageScaleAnimation->pageScaleAtTime(monotonicTime) / m_pageScale);
    IntSize nextScroll = m_pageScaleAnimation->scrollOffsetAtTime(monotonicTime);
    nextScroll.scale(1 / m_pageScaleDelta);
    m_scrollLayerImpl->scrollBy(nextScroll - scrollTotal);
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

    TRACE_EVENT("CCLayerTreeHostImpl::animateLayers", this, 0);

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
    // FIXME: If we have any RenderSurfaceLayerList saved, then make the list empty here.
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

void CCLayerTreeHostImpl::setFontAtlas(PassOwnPtr<CCFontAtlas> fontAtlas)
{
    m_headsUpDisplay->setFontAtlas(fontAtlas);
}

void CCLayerTreeHostImpl::dumpRenderSurfaces(TextStream& ts, int indent, const CCLayerImpl* layer) const
{
    if (layer->renderSurface())
        layer->renderSurface()->dumpSurface(ts, indent);

    for (size_t i = 0; i < layer->children().size(); ++i)
        dumpRenderSurfaces(ts, indent, layer->children()[i].get());
}


void CCLayerTreeHostImpl::animateGestures(double monotonicTime)
{
    if (!m_activeGestureAnimation)
        return;

    bool isContinuing = m_activeGestureAnimation->animate(monotonicTime);
    if (isContinuing)
        m_client->setNeedsRedrawOnImplThread();
    else
        m_activeGestureAnimation.clear();
}

} // namespace WebCore
