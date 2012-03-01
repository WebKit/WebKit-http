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
#include "GraphicsContext3D.h"
#include "LayerRendererChromium.h"
#include "TraceEvent.h"
#include "cc/CCDamageTracker.h"
#include "cc/CCLayerIterator.h"
#include "cc/CCLayerTreeHost.h"
#include "cc/CCLayerTreeHostCommon.h"
#include "cc/CCPageScaleAnimation.h"
#include "cc/CCRenderSurfaceDrawQuad.h"
#include "cc/CCThreadTask.h"
#include <wtf/CurrentTime.h>

namespace WebCore {

PassOwnPtr<CCLayerTreeHostImpl> CCLayerTreeHostImpl::create(const CCSettings& settings, CCLayerTreeHostImplClient* client)
{
    return adoptPtr(new CCLayerTreeHostImpl(settings, client));
}

CCLayerTreeHostImpl::CCLayerTreeHostImpl(const CCSettings& settings, CCLayerTreeHostImplClient* client)
    : m_client(client)
    , m_sourceFrameNumber(-1)
    , m_frameNumber(0)
    , m_scrollLayerImpl(0)
    , m_settings(settings)
    , m_visible(true)
    , m_pageScale(1)
    , m_pageScaleDelta(1)
    , m_sentPageScaleDelta(1)
    , m_minPageScale(0)
    , m_maxPageScale(0)
    , m_needsAnimateLayers(false)
    , m_pinchGestureActive(false)
{
    ASSERT(CCProxy::isImplThread());
}

CCLayerTreeHostImpl::~CCLayerTreeHostImpl()
{
    ASSERT(CCProxy::isImplThread());
    TRACE_EVENT("CCLayerTreeHostImpl::~CCLayerTreeHostImpl()", this, 0);
    if (m_layerRenderer)
        m_layerRenderer->close();
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
    if (!rootLayer())
        return false;
    if (viewportSize().isEmpty())
        return false;
    return true;
}

GraphicsContext3D* CCLayerTreeHostImpl::context()
{
    return m_layerRenderer ? m_layerRenderer->context() : 0;
}

void CCLayerTreeHostImpl::animate(double frameBeginTimeMs)
{
    animatePageScale(frameBeginTimeMs);
    animateLayers(frameBeginTimeMs);
}

void CCLayerTreeHostImpl::startPageScaleAnimation(const IntSize& targetPosition, bool anchorPoint, float pageScale, double startTimeMs, double durationMs)
{
    if (!m_scrollLayerImpl)
        return;

    IntSize scrollTotal = toSize(m_scrollLayerImpl->scrollPosition() + m_scrollLayerImpl->scrollDelta());
    scrollTotal.scale(m_pageScaleDelta);
    float scaleTotal = m_pageScale * m_pageScaleDelta;
    IntSize scaledContentSize = contentSize();
    scaledContentSize.scale(m_pageScaleDelta);

    m_pageScaleAnimation = CCPageScaleAnimation::create(scrollTotal, scaleTotal, m_viewportSize, scaledContentSize, startTimeMs);

    if (anchorPoint) {
        IntSize windowAnchor(targetPosition);
        windowAnchor.scale(scaleTotal / pageScale);
        windowAnchor -= scrollTotal;
        m_pageScaleAnimation->zoomWithAnchor(windowAnchor, pageScale, durationMs);
    } else
        m_pageScaleAnimation->zoomTo(targetPosition, pageScale, durationMs);

    m_client->setNeedsRedrawOnImplThread();
    m_client->setNeedsCommitOnImplThread();
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
        renderSurface->damageTracker()->updateDamageTrackingState(renderSurface->layerList(), renderSurfaceLayer->id(), renderSurfaceLayer->maskLayer());
    }
}

static TransformationMatrix computeScreenSpaceTransformForSurface(CCLayerImpl* renderSurfaceLayer)
{
    // The layer's screen space transform can be written as:
    //   layerScreenSpaceTransform = surfaceScreenSpaceTransform * layerOriginTransform
    // So, to compute the surface screen space, we can do:
    //   surfaceScreenSpaceTransform = layerScreenSpaceTransform * inverse(layerOriginTransform)

    TransformationMatrix layerOriginTransform = renderSurfaceLayer->drawTransform();
    layerOriginTransform.translate(-0.5 * renderSurfaceLayer->bounds().width(), -0.5 * renderSurfaceLayer->bounds().height());
    TransformationMatrix surfaceScreenSpaceTransform = renderSurfaceLayer->screenSpaceTransform();
    surfaceScreenSpaceTransform.multiply(layerOriginTransform.inverse());

    return surfaceScreenSpaceTransform;
}

static FloatRect damageInSurfaceSpace(CCLayerImpl* renderSurfaceLayer, const FloatRect& rootDamageRect)
{
    FloatRect surfaceDamageRect;
    // For now, we conservatively use the root damage as the damage for
    // all surfaces, except perspective transforms.
    TransformationMatrix screenSpaceTransform = computeScreenSpaceTransformForSurface(renderSurfaceLayer);
    if (screenSpaceTransform.hasPerspective()) {
        // Perspective projections do not play nice with mapRect of
        // inverse transforms. In this uncommon case, its simpler to
        // just redraw the entire surface.
        // FIXME: use calculateVisibleRect to handle projections.
        CCRenderSurface* renderSurface = renderSurfaceLayer->renderSurface();
        surfaceDamageRect = renderSurface->contentRect();
    } else {
        TransformationMatrix inverseScreenSpaceTransform = screenSpaceTransform.inverse();
        surfaceDamageRect = inverseScreenSpaceTransform.mapRect(rootDamageRect);
    }
    return surfaceDamageRect;
}

void CCLayerTreeHostImpl::calculateRenderPasses(CCRenderPassList& passes, CCLayerList& renderSurfaceLayerList)
{
    renderSurfaceLayerList.append(rootLayer());

    if (!rootLayer()->renderSurface())
        rootLayer()->createRenderSurface();
    rootLayer()->renderSurface()->clearLayerList();
    rootLayer()->renderSurface()->setContentRect(IntRect(IntPoint(), viewportSize()));

    rootLayer()->setClipRect(IntRect(IntPoint(), viewportSize()));

    {
        TransformationMatrix identityMatrix;
        TRACE_EVENT("CCLayerTreeHostImpl::calcDrawEtc", this, 0);
        CCLayerTreeHostCommon::calculateDrawTransformsAndVisibility(rootLayer(), rootLayer(), identityMatrix, identityMatrix, renderSurfaceLayerList, rootLayer()->renderSurface()->layerList(), &m_layerSorter, layerRendererCapabilities().maxTextureSize);
    }

    if (layerRendererCapabilities().usingPartialSwap)
        trackDamageForAllSurfaces(rootLayer(), renderSurfaceLayerList);
    m_rootDamageRect = rootLayer()->renderSurface()->damageTracker()->currentDamageRect();

    for (int surfaceIndex = renderSurfaceLayerList.size() - 1; surfaceIndex >= 0 ; --surfaceIndex) {
        CCLayerImpl* renderSurfaceLayer = renderSurfaceLayerList[surfaceIndex];
        CCRenderSurface* renderSurface = renderSurfaceLayer->renderSurface();

        OwnPtr<CCRenderPass> pass = CCRenderPass::create(renderSurface);

        FloatRect surfaceDamageRect;
        if (layerRendererCapabilities().usingPartialSwap)
            surfaceDamageRect = damageInSurfaceSpace(renderSurfaceLayer, m_rootDamageRect);
        pass->setSurfaceDamageRect(surfaceDamageRect);

        const CCLayerList& layerList = renderSurface->layerList();
        for (unsigned layerIndex = 0; layerIndex < layerList.size(); ++layerIndex) {
            CCLayerImpl* layer = layerList[layerIndex];
            if (layer->visibleLayerRect().isEmpty())
                continue;

            if (CCLayerTreeHostCommon::renderSurfaceContributesToTarget(layer, renderSurfaceLayer->id())) {
                pass->appendQuadsForRenderSurfaceLayer(layer);
                continue;
            }

            layer->willDraw(m_layerRenderer.get());
            pass->appendQuadsForLayer(layer);
        }

        passes.append(pass.release());
    }
}

void CCLayerTreeHostImpl::optimizeRenderPasses(CCRenderPassList& passes)
{
    TRACE_EVENT1("webkit", "CCLayerTreeHostImpl::optimizeRenderPasses", "passes.size()", static_cast<long long unsigned>(passes.size()));

    bool haveDamageRect = layerRendererCapabilities().usingPartialSwap;

    // FIXME: compute overdraw metrics only occasionally, not every frame.
    CCOverdrawCounts overdrawCounts;
    for (unsigned i = 0; i < passes.size(); ++i) {
        FloatRect damageRect = passes[i]->targetSurface()->damageTracker()->currentDamageRect();
        passes[i]->optimizeQuads(haveDamageRect, damageRect, &overdrawCounts);
    }

    float normalization = 1000.f / (m_layerRenderer->viewportWidth() * m_layerRenderer->viewportHeight());
    PlatformSupport::histogramCustomCounts("Renderer4.pixelOverdrawOpaque", static_cast<int>(normalization * overdrawCounts.m_pixelsDrawnOpaque), 100, 1000000, 50);
    PlatformSupport::histogramCustomCounts("Renderer4.pixelOverdrawTransparent", static_cast<int>(normalization * overdrawCounts.m_pixelsDrawnTransparent), 100, 1000000, 50);
    PlatformSupport::histogramCustomCounts("Renderer4.pixelOverdrawCulled", static_cast<int>(normalization * overdrawCounts.m_pixelsCulled), 100, 1000000, 50);
}

void CCLayerTreeHostImpl::animateLayersRecursive(CCLayerImpl* current, double frameBeginTimeSecs, CCAnimationEventsVector& events, bool& didAnimate, bool& needsAnimateLayers)
{
    bool subtreeNeedsAnimateLayers = false;

    CCLayerAnimationControllerImpl* currentController = current->layerAnimationController();

    bool hadActiveAnimation = currentController->hasActiveAnimation();
    currentController->animate(frameBeginTimeSecs, events);
    bool startedAnimation = events.size() > 0;

    // We animated if we either ticked a running animation, or started a new animation.
    if (hadActiveAnimation || startedAnimation)
        didAnimate = true;

    // If the current controller still has an active animation, we must continue animating layers.
    if (currentController->hasActiveAnimation())
         subtreeNeedsAnimateLayers = true;

    for (size_t i = 0; i < current->children().size(); ++i) {
        bool childNeedsAnimateLayers = false;
        animateLayersRecursive(current->children()[i].get(), frameBeginTimeSecs, events, didAnimate, childNeedsAnimateLayers);
        if (childNeedsAnimateLayers)
            subtreeNeedsAnimateLayers = true;
    }

    needsAnimateLayers = subtreeNeedsAnimateLayers;
}

IntSize CCLayerTreeHostImpl::contentSize() const
{
    // TODO(aelias): Hardcoding the first child here is weird. Think of
    // a cleaner way to get the contentBounds on the Impl side.
    if (!m_scrollLayerImpl || m_scrollLayerImpl->children().isEmpty())
        return IntSize();
    return m_scrollLayerImpl->children()[0]->contentBounds();
}

void CCLayerTreeHostImpl::drawLayers()
{
    TRACE_EVENT("CCLayerTreeHostImpl::drawLayers", this, 0);
    ASSERT(m_layerRenderer);

    if (!rootLayer())
        return;

    CCRenderPassList passes;
    CCLayerList renderSurfaceLayerList;
    calculateRenderPasses(passes, renderSurfaceLayerList);

    optimizeRenderPasses(passes);

    m_layerRenderer->beginDrawingFrame();
    for (size_t i = 0; i < passes.size(); ++i)
        m_layerRenderer->drawRenderPass(passes[i].get());

    typedef CCLayerIterator<CCLayerImpl, Vector<CCLayerImpl*>, CCRenderSurface, CCLayerIteratorActions::BackToFront> CCLayerIteratorType;

    CCLayerIteratorType end = CCLayerIteratorType::end(&renderSurfaceLayerList);
    for (CCLayerIteratorType it = CCLayerIteratorType::begin(&renderSurfaceLayerList); it != end; ++it) {
        if (it.representsItself() && !it->visibleLayerRect().isEmpty())
            it->didDraw();
    }
    m_layerRenderer->finishDrawingFrame();

    ++m_frameNumber;

    // The next frame should start by assuming nothing has changed, and changes are noted as they occur.
    rootLayer()->resetAllChangeTrackingForSubtree();
}

void CCLayerTreeHostImpl::finishAllRendering()
{
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

void CCLayerTreeHostImpl::swapBuffers()
{
    ASSERT(m_layerRenderer && !isContextLost());
    m_layerRenderer->swapBuffers(enclosingIntRect(m_rootDamageRect));
}

void CCLayerTreeHostImpl::onSwapBuffersComplete()
{
    m_client->onSwapBuffersCompleteOnImplThread();
}

void CCLayerTreeHostImpl::readback(void* pixels, const IntRect& rect)
{
    ASSERT(m_layerRenderer && !isContextLost());
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
    if (m_visible == visible)
        return;
    m_visible = visible;

    if (m_layerRenderer)
        m_layerRenderer->setVisible(visible);
}

bool CCLayerTreeHostImpl::initializeLayerRenderer(PassRefPtr<GraphicsContext3D> context)
{
    OwnPtr<LayerRendererChromium> layerRenderer;
    layerRenderer = LayerRendererChromium::create(this, context);

    if (m_layerRenderer)
        m_layerRenderer->close();

    m_layerRenderer = layerRenderer.release();
    return m_layerRenderer;
}

void CCLayerTreeHostImpl::setViewportSize(const IntSize& viewportSize)
{
    if (viewportSize == m_viewportSize)
        return;

    m_viewportSize = viewportSize;
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
        IntSize scrollDelta = m_scrollLayerImpl->scrollDelta();
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
    viewBounds.scale(1 / m_pageScaleDelta);

    IntSize maxScroll = contentSize() - expandedIntSize(viewBounds);
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

    IntPoint scrollLayerContentPoint(m_scrollLayerImpl->screenSpaceTransform().inverse().mapPoint(viewportPoint));
    if (m_scrollLayerImpl->nonFastScrollableRegion().contains(scrollLayerContentPoint)) {
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
    FloatSize scrollBegin = toSize(m_scrollLayerImpl->scrollPosition() + m_scrollLayerImpl->scrollDelta());
    scrollBegin.scale(m_pageScaleDelta);
    float scaleBegin = m_pageScale * m_pageScaleDelta;
    float pageScaleDeltaToSend = m_minPageScale / m_pageScale;
    FloatSize scaledContentsSize = contentSize();
    scaledContentsSize.scale(pageScaleDeltaToSend);

    FloatSize anchor = toSize(m_previousPinchAnchor);
    FloatSize scrollEnd = scrollBegin + anchor;
    scrollEnd.scale(m_minPageScale / scaleBegin);
    scrollEnd -= anchor;
    scrollEnd = scrollEnd.shrunkTo(roundedIntSize(scaledContentsSize - m_viewportSize)).expandedTo(FloatSize(0, 0));
    scrollEnd.scale(1 / pageScaleDeltaToSend);

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
    scroll.scrollDelta = m_scrollLayerImpl->scrollDelta();
    scrollInfo->scrolls.append(scroll);

    m_scrollLayerImpl->setSentScrollDelta(scroll.scrollDelta);

    return scrollInfo.release();
}

void CCLayerTreeHostImpl::setFullRootLayerDamage()
{
    if (rootLayer()) {
        CCRenderSurface* renderSurface = rootLayer()->renderSurface();
        if (renderSurface)
            renderSurface->damageTracker()->forceFullDamageNextUpdate();
    }
}

void CCLayerTreeHostImpl::animatePageScale(double frameBeginTimeMs)
{
    if (!m_pageScaleAnimation)
        return;

    IntSize scrollTotal = toSize(m_scrollLayerImpl->scrollPosition() + m_scrollLayerImpl->scrollDelta());

    setPageScaleDelta(m_pageScaleAnimation->pageScaleAtTime(frameBeginTimeMs) / m_pageScale);
    IntSize nextScroll = m_pageScaleAnimation->scrollOffsetAtTime(frameBeginTimeMs);
    nextScroll.scale(1 / m_pageScaleDelta);
    m_scrollLayerImpl->scrollBy(nextScroll - scrollTotal);
    m_client->setNeedsRedrawOnImplThread();

    if (m_pageScaleAnimation->isAnimationCompleteAtTime(frameBeginTimeMs)) {
        m_pageScaleAnimation.clear();
        m_client->setNeedsCommitOnImplThread();
    }
}

void CCLayerTreeHostImpl::animateLayers(double frameBeginTimeMs)
{
    if (!m_settings.threadedAnimationEnabled || !m_needsAnimateLayers || !m_rootLayerImpl)
        return;

    TRACE_EVENT("CCLayerTreeHostImpl::animateLayers", this, 0);

    OwnPtr<CCAnimationEventsVector> events(adoptPtr(new CCAnimationEventsVector));

    bool didAnimate = false;
    animateLayersRecursive(m_rootLayerImpl.get(), frameBeginTimeMs / 1000, *events, didAnimate, m_needsAnimateLayers);

    if (!events->isEmpty())
        m_client->postAnimationEventsToMainThreadOnImplThread(events.release());

    if (didAnimate)
        m_client->setNeedsRedrawOnImplThread();
}

} // namespace WebCore
