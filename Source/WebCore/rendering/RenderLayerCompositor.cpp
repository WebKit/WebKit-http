/*
 * Copyright (C) 2009, 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#include "RenderLayerCompositor.h"

#include "AnimationController.h"
#include "CanvasRenderingContext.h"
#include "CSSPropertyNames.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "FlowThreadController.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsLayer.h"
#include "HTMLCanvasElement.h"
#include "HTMLIFrameElement.h"
#include "HTMLNames.h"
#include "HitTestResult.h"
#include "InspectorInstrumentation.h"
#include "Logging.h"
#include "MainFrame.h"
#include "NodeList.h"
#include "Page.h"
#include "RenderEmbeddedObject.h"
#include "RenderFlowThread.h"
#include "RenderFullScreen.h"
#include "RenderGeometryMap.h"
#include "RenderIFrame.h"
#include "RenderLayerBacking.h"
#include "RenderNamedFlowFragment.h"
#include "RenderReplica.h"
#include "RenderVideo.h"
#include "RenderView.h"
#include "ScrollingConstraints.h"
#include "ScrollingCoordinator.h"
#include "Settings.h"
#include "TiledBacking.h"
#include "TransformState.h"
#include <wtf/CurrentTime.h>
#include <wtf/TemporaryChange.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>

#if PLATFORM(IOS)
#include "LegacyTileCache.h"
#include "MainFrame.h"
#include "Region.h"
#include "RenderScrollbar.h"
#endif

#ifndef NDEBUG
#include "RenderTreeAsText.h"
#endif

#if ENABLE(3D_RENDERING)
// This symbol is used to determine from a script whether 3D rendering is enabled (via 'nm').
bool WebCoreHas3DRendering = true;
#endif

#if !PLATFORM(MAC) && !PLATFORM(IOS)
#define WTF_USE_COMPOSITING_FOR_SMALL_CANVASES 1
#endif

namespace WebCore {

static const int canvasAreaThresholdRequiringCompositing = 50 * 100;
// During page loading delay layer flushes up to this many seconds to allow them coalesce, reducing workload.
#if PLATFORM(IOS)
static const double throttledLayerFlushInitialDelay = .5;
static const double throttledLayerFlushDelay = 1.5;
#else
static const double throttledLayerFlushInitialDelay = .5;
static const double throttledLayerFlushDelay = .5;
#endif

using namespace HTMLNames;

class OverlapMapContainer {
public:
    void add(const IntRect& bounds)
    {
        m_layerRects.append(bounds);
        m_boundingBox.unite(bounds);
    }

    bool overlapsLayers(const IntRect& bounds) const
    {
        // Checking with the bounding box will quickly reject cases when
        // layers are created for lists of items going in one direction and
        // never overlap with each other.
        if (!bounds.intersects(m_boundingBox))
            return false;
        for (unsigned i = 0; i < m_layerRects.size(); i++) {
            if (m_layerRects[i].intersects(bounds))
                return true;
        }
        return false;
    }

    void unite(const OverlapMapContainer& otherContainer)
    {
        m_layerRects.appendVector(otherContainer.m_layerRects);
        m_boundingBox.unite(otherContainer.m_boundingBox);
    }
private:
    Vector<IntRect> m_layerRects;
    IntRect m_boundingBox;
};

class RenderLayerCompositor::OverlapMap {
    WTF_MAKE_NONCOPYABLE(OverlapMap);
public:
    OverlapMap()
        : m_geometryMap(UseTransforms)
    {
        // Begin assuming the root layer will be composited so that there is
        // something on the stack. The root layer should also never get an
        // popCompositingContainer call.
        pushCompositingContainer();
    }

    void add(const RenderLayer* layer, const IntRect& bounds)
    {
        // Layers do not contribute to overlap immediately--instead, they will
        // contribute to overlap as soon as their composited ancestor has been
        // recursively processed and popped off the stack.
        ASSERT(m_overlapStack.size() >= 2);
        m_overlapStack[m_overlapStack.size() - 2].add(bounds);
        m_layers.add(layer);
    }

    bool contains(const RenderLayer* layer)
    {
        return m_layers.contains(layer);
    }

    bool overlapsLayers(const IntRect& bounds) const
    {
        return m_overlapStack.last().overlapsLayers(bounds);
    }

    bool isEmpty()
    {
        return m_layers.isEmpty();
    }

    void pushCompositingContainer()
    {
        m_overlapStack.append(OverlapMapContainer());
    }

    void popCompositingContainer()
    {
        m_overlapStack[m_overlapStack.size() - 2].unite(m_overlapStack.last());
        m_overlapStack.removeLast();
    }

    RenderGeometryMap& geometryMap() { return m_geometryMap; }

private:
    struct RectList {
        Vector<IntRect> rects;
        IntRect boundingRect;
        
        void append(const IntRect& rect)
        {
            rects.append(rect);
            boundingRect.unite(rect);
        }

        void append(const RectList& rectList)
        {
            rects.appendVector(rectList.rects);
            boundingRect.unite(rectList.boundingRect);
        }
        
        bool intersects(const IntRect& rect) const
        {
            if (!rects.size() || !boundingRect.intersects(rect))
                return false;

            for (unsigned i = 0; i < rects.size(); i++) {
                if (rects[i].intersects(rect))
                    return true;
            }
            return false;
        }
    };

    Vector<OverlapMapContainer> m_overlapStack;
    HashSet<const RenderLayer*> m_layers;
    RenderGeometryMap m_geometryMap;
};

struct CompositingState {
    CompositingState(RenderLayer* compAncestor, bool testOverlap = true)
        : m_compositingAncestor(compAncestor)
        , m_subtreeIsCompositing(false)
        , m_testingOverlap(testOverlap)
#if ENABLE(CSS_COMPOSITING)
        , m_hasNotIsolatedCompositedBlendingDescendants(false)
#endif
#ifndef NDEBUG
        , m_depth(0)
#endif
    {
    }
    
    CompositingState(const CompositingState& other)
        : m_compositingAncestor(other.m_compositingAncestor)
        , m_subtreeIsCompositing(other.m_subtreeIsCompositing)
        , m_testingOverlap(other.m_testingOverlap)
#if ENABLE(CSS_COMPOSITING)
        , m_hasNotIsolatedCompositedBlendingDescendants(other.m_hasNotIsolatedCompositedBlendingDescendants)
#endif
#ifndef NDEBUG
        , m_depth(other.m_depth + 1)
#endif
    {
    }
    
    RenderLayer* m_compositingAncestor;
    bool m_subtreeIsCompositing;
    bool m_testingOverlap;
#if ENABLE(CSS_COMPOSITING)
    bool m_hasNotIsolatedCompositedBlendingDescendants;
#endif
#ifndef NDEBUG
    int m_depth;
#endif
};


#if !LOG_DISABLED
static inline bool compositingLogEnabled()
{
    return LogCompositing.state == WTFLogChannelOn;
}
#endif

RenderLayerCompositor::RenderLayerCompositor(RenderView& renderView)
    : m_renderView(renderView)
    , m_updateCompositingLayersTimer(this, &RenderLayerCompositor::updateCompositingLayersTimerFired)
    , m_hasAcceleratedCompositing(true)
    , m_compositingTriggers(static_cast<ChromeClient::CompositingTriggerFlags>(ChromeClient::AllTriggers))
    , m_compositedLayerCount(0)
    , m_showDebugBorders(false)
    , m_showRepaintCounter(false)
    , m_acceleratedDrawingEnabled(false)
    , m_reevaluateCompositingAfterLayout(false)
    , m_compositing(false)
    , m_compositingLayersNeedRebuild(false)
    , m_flushingLayers(false)
    , m_shouldFlushOnReattach(false)
    , m_forceCompositingMode(false)
    , m_inPostLayoutUpdate(false)
    , m_subframeScrollLayersNeedReattach(false)
    , m_isTrackingRepaints(false)
    , m_layersWithTiledBackingCount(0)
    , m_rootLayerAttachment(RootLayerUnattached)
    , m_layerFlushTimer(this, &RenderLayerCompositor::layerFlushTimerFired)
    , m_layerFlushThrottlingEnabled(false)
    , m_layerFlushThrottlingTemporarilyDisabledForInteraction(false)
    , m_hasPendingLayerFlush(false)
    , m_paintRelatedMilestonesTimer(this, &RenderLayerCompositor::paintRelatedMilestonesTimerFired)
#if !LOG_DISABLED
    , m_rootLayerUpdateCount(0)
    , m_obligateCompositedLayerCount(0)
    , m_secondaryCompositedLayerCount(0)
    , m_obligatoryBackingStoreBytes(0)
    , m_secondaryBackingStoreBytes(0)
#endif
{
}

RenderLayerCompositor::~RenderLayerCompositor()
{
    // Take care that the owned GraphicsLayers are deleted first as their destructors may call back here.
    m_clipLayer = nullptr;
    m_scrollLayer = nullptr;
    ASSERT(m_rootLayerAttachment == RootLayerUnattached);
}

void RenderLayerCompositor::enableCompositingMode(bool enable /* = true */)
{
    if (enable != m_compositing) {
        m_compositing = enable;
        
        if (m_compositing) {
            ensureRootLayer();
            notifyIFramesOfCompositingChange();
        } else
            destroyRootLayer();
    }
}

void RenderLayerCompositor::cacheAcceleratedCompositingFlags()
{
    bool hasAcceleratedCompositing = false;
    bool showDebugBorders = false;
    bool showRepaintCounter = false;
    bool forceCompositingMode = false;
    bool acceleratedDrawingEnabled = false;

    const Settings& settings = m_renderView.frameView().frame().settings();
    hasAcceleratedCompositing = settings.acceleratedCompositingEnabled();

    // We allow the chrome to override the settings, in case the page is rendered
    // on a chrome that doesn't allow accelerated compositing.
    if (hasAcceleratedCompositing) {
        if (Page* page = this->page()) {
            m_compositingTriggers = page->chrome().client().allowedCompositingTriggers();
            hasAcceleratedCompositing = m_compositingTriggers;
        }
    }

    showDebugBorders = settings.showDebugBorders();
    showRepaintCounter = settings.showRepaintCounter();
    forceCompositingMode = settings.forceCompositingMode() && hasAcceleratedCompositing;

    if (forceCompositingMode && m_renderView.document().ownerElement())
        forceCompositingMode = requiresCompositingForScrollableFrame();

    acceleratedDrawingEnabled = settings.acceleratedDrawingEnabled();

    if (hasAcceleratedCompositing != m_hasAcceleratedCompositing || showDebugBorders != m_showDebugBorders || showRepaintCounter != m_showRepaintCounter || forceCompositingMode != m_forceCompositingMode)
        setCompositingLayersNeedRebuild();

    bool debugBordersChanged = m_showDebugBorders != showDebugBorders;
    m_hasAcceleratedCompositing = hasAcceleratedCompositing;
    m_showDebugBorders = showDebugBorders;
    m_showRepaintCounter = showRepaintCounter;
    m_forceCompositingMode = forceCompositingMode;
    m_acceleratedDrawingEnabled = acceleratedDrawingEnabled;
    
    if (debugBordersChanged) {
        if (m_layerForHorizontalScrollbar)
            m_layerForHorizontalScrollbar->setShowDebugBorder(m_showDebugBorders);

        if (m_layerForVerticalScrollbar)
            m_layerForVerticalScrollbar->setShowDebugBorder(m_showDebugBorders);

        if (m_layerForScrollCorner)
            m_layerForScrollCorner->setShowDebugBorder(m_showDebugBorders);
    }
}

bool RenderLayerCompositor::canRender3DTransforms() const
{
    return hasAcceleratedCompositing() && (m_compositingTriggers & ChromeClient::ThreeDTransformTrigger);
}

void RenderLayerCompositor::setCompositingLayersNeedRebuild(bool needRebuild)
{
    if (inCompositingMode())
        m_compositingLayersNeedRebuild = needRebuild;
}

void RenderLayerCompositor::customPositionForVisibleRectComputation(const GraphicsLayer* graphicsLayer, FloatPoint& position) const
{
    if (graphicsLayer != m_scrollLayer.get())
        return;

    FloatPoint scrollPosition = -position;

    if (m_renderView.frameView().scrollBehaviorForFixedElements() == StickToDocumentBounds)
        scrollPosition = m_renderView.frameView().constrainScrollPositionForOverhang(roundedIntPoint(scrollPosition));

    position = -scrollPosition;
}

void RenderLayerCompositor::notifyFlushRequired(const GraphicsLayer* layer)
{
    scheduleLayerFlush(layer->canThrottleLayerFlush());
}

void RenderLayerCompositor::scheduleLayerFlushNow()
{
    m_hasPendingLayerFlush = false;
    if (Page* page = this->page())
        page->chrome().client().scheduleCompositingLayerFlush();
}

void RenderLayerCompositor::scheduleLayerFlush(bool canThrottle)
{
    ASSERT(!m_flushingLayers);

    if (canThrottle)
        startInitialLayerFlushTimerIfNeeded();

    if (canThrottle && isThrottlingLayerFlushes()) {
        m_hasPendingLayerFlush = true;
        return;
    }
    scheduleLayerFlushNow();
}

#if PLATFORM(IOS)
ChromeClient* RenderLayerCompositor::chromeClient() const
{
    Page* page = m_renderView.frameView().frame().page();
    if (!page)
        return 0;
    return &page->chrome().client();
}
#endif

void RenderLayerCompositor::flushPendingLayerChanges(bool isFlushRoot)
{
    // FrameView::flushCompositingStateIncludingSubframes() flushes each subframe,
    // but GraphicsLayer::flushCompositingState() will cross frame boundaries
    // if the GraphicsLayers are connected (the RootLayerAttachedViaEnclosingFrame case).
    // As long as we're not the root of the flush, we can bail.
    if (!isFlushRoot && rootLayerAttachment() == RootLayerAttachedViaEnclosingFrame)
        return;
    
    if (rootLayerAttachment() == RootLayerUnattached) {
#if PLATFORM(IOS)
        startLayerFlushTimerIfNeeded();
#endif
        m_shouldFlushOnReattach = true;
        return;
    }

    FrameView& frameView = m_renderView.frameView();
    AnimationUpdateBlock animationUpdateBlock(&frameView.frame().animation());

    ASSERT(!m_flushingLayers);
    m_flushingLayers = true;

    if (GraphicsLayer* rootLayer = rootGraphicsLayer()) {
#if PLATFORM(IOS)
        double horizontalMargin = defaultTileWidth / pageScaleFactor();
        double verticalMargin = defaultTileHeight / pageScaleFactor();
        FloatRect visibleRect = frameView.computeCoverageRect(horizontalMargin, verticalMargin);
        rootLayer->flushCompositingState(visibleRect);
#else
        // Having a m_clipLayer indicates that we're doing scrolling via GraphicsLayers.
        IntRect visibleRect = m_clipLayer ? IntRect(IntPoint(), frameView.unscaledVisibleContentSizeIncludingObscuredArea()) : frameView.visibleContentRect();
        if (!frameView.exposedRect().isInfinite())
            visibleRect.intersect(IntRect(frameView.exposedRect()));
        rootLayer->flushCompositingState(visibleRect);
#endif
    }
    
    ASSERT(m_flushingLayers);
    m_flushingLayers = false;

    updateScrollCoordinatedLayersAfterFlushIncludingSubframes();

#if PLATFORM(IOS)
    ChromeClient* client = this->chromeClient();
    if (client && isFlushRoot)
        client->didFlushCompositingLayers();
#endif

    startLayerFlushTimerIfNeeded();
}

void RenderLayerCompositor::updateScrollCoordinatedLayersAfterFlushIncludingSubframes()
{
    updateScrollCoordinatedLayersAfterFlush();

    Frame& frame = m_renderView.frameView().frame();
    for (Frame* subframe = frame.tree().firstChild(); subframe; subframe = subframe->tree().traverseNext(&frame)) {
        RenderView* view = subframe->contentRenderer();
        if (!view)
            continue;

        view->compositor().updateScrollCoordinatedLayersAfterFlush();
    }
}

void RenderLayerCompositor::updateScrollCoordinatedLayersAfterFlush()
{
#if PLATFORM(IOS)
    updateCustomLayersAfterFlush();
#endif

    for (auto it = m_scrollCoordinatedLayersNeedingUpdate.begin(), end = m_scrollCoordinatedLayersNeedingUpdate.end(); it != end; ++it)
        updateScrollCoordinatedStatus(**it);

    m_scrollCoordinatedLayersNeedingUpdate.clear();
}

#if PLATFORM(IOS)
static bool scrollbarHasDisplayNone(Scrollbar* scrollbar)
{
    if (!scrollbar || !scrollbar->isCustomScrollbar())
        return false;

    RefPtr<RenderStyle> scrollbarStyle = static_cast<RenderScrollbar*>(scrollbar)->getScrollbarPseudoStyle(ScrollbarBGPart, SCROLLBAR);
    return scrollbarStyle && scrollbarStyle->display() == NONE;
}

// FIXME: Can we make |layer| const RenderLayer&?
static void updateScrollingLayerWithClient(RenderLayer& layer, ChromeClient* client)
{
    if (!client)
        return;

    RenderLayerBacking* backing = layer.backing();
    ASSERT(backing);

    bool allowHorizontalScrollbar = !scrollbarHasDisplayNone(layer.horizontalScrollbar());
    bool allowVerticalScrollbar = !scrollbarHasDisplayNone(layer.verticalScrollbar());
    client->addOrUpdateScrollingLayer(layer.renderer().element(), backing->scrollingLayer()->platformLayer(), backing->scrollingContentsLayer()->platformLayer(),
        layer.scrollableContentsSize(), allowHorizontalScrollbar, allowVerticalScrollbar);
}

void RenderLayerCompositor::updateCustomLayersAfterFlush()
{
    registerAllViewportConstrainedLayers();

    if (!m_scrollingLayersNeedingUpdate.isEmpty()) {
        ChromeClient* chromeClient = this->chromeClient();

        for (auto it = m_scrollingLayersNeedingUpdate.begin(), end = m_scrollingLayersNeedingUpdate.end(); it != end; ++it)
            updateScrollingLayerWithClient(**it, chromeClient);
        m_scrollingLayersNeedingUpdate.clear();
    }
    m_scrollingLayersNeedingUpdate.clear();
}
#endif

void RenderLayerCompositor::didFlushChangesForLayer(RenderLayer& layer, const GraphicsLayer* graphicsLayer)
{
    if (m_scrollCoordinatedLayers.contains(&layer))
        m_scrollCoordinatedLayersNeedingUpdate.add(&layer);

#if PLATFORM(IOS)
    if (m_scrollingLayers.contains(&layer))
        m_scrollingLayersNeedingUpdate.add(&layer);
#endif

    RenderLayerBacking* backing = layer.backing();
    if (backing->backgroundLayerPaintsFixedRootBackground() && graphicsLayer == backing->backgroundLayer())
        fixedRootBackgroundLayerChanged();
}

void RenderLayerCompositor::didPaintBacking(RenderLayerBacking*)
{
    FrameView& frameView = m_renderView.frameView();
    frameView.setLastPaintTime(monotonicallyIncreasingTime());
    if (frameView.milestonesPendingPaint() && !m_paintRelatedMilestonesTimer.isActive())
        m_paintRelatedMilestonesTimer.startOneShot(0);
}

void RenderLayerCompositor::didChangeVisibleRect()
{
    GraphicsLayer* rootLayer = rootGraphicsLayer();
    if (!rootLayer)
        return;

    const FrameView& frameView = m_renderView.frameView();

#if PLATFORM(IOS)
    IntRect visibleRect = enclosingIntRect(frameView.exposedContentRect());
#else
    IntRect visibleRect = m_clipLayer ? IntRect(IntPoint(), frameView.contentsSize()) : frameView.visibleContentRect();
#endif
    if (!rootLayer->visibleRectChangeRequiresFlush(visibleRect))
        return;
    scheduleLayerFlushNow();
}

void RenderLayerCompositor::notifyFlushBeforeDisplayRefresh(const GraphicsLayer*)
{
    if (!m_layerUpdater) {
        PlatformDisplayID displayID = 0;
        if (Page* page = this->page())
            displayID = page->chrome().displayID();

        m_layerUpdater = std::make_unique<GraphicsLayerUpdater>(this, displayID);
    }
    
    m_layerUpdater->scheduleUpdate();
}

void RenderLayerCompositor::flushLayersSoon(GraphicsLayerUpdater*)
{
    scheduleLayerFlush(true);
}

void RenderLayerCompositor::layerTiledBackingUsageChanged(const GraphicsLayer* graphicsLayer, bool usingTiledBacking)
{
    if (usingTiledBacking) {
        ++m_layersWithTiledBackingCount;

        if (Page* page = this->page())
            graphicsLayer->tiledBacking()->setIsInWindow(page->isInWindow());
    } else {
        ASSERT(m_layersWithTiledBackingCount > 0);
        --m_layersWithTiledBackingCount;
    }
}

RenderLayerCompositor* RenderLayerCompositor::enclosingCompositorFlushingLayers() const
{
    for (Frame* frame = &m_renderView.frameView().frame(); frame; frame = frame->tree().parent()) {
        RenderLayerCompositor* compositor = frame->contentRenderer() ? &frame->contentRenderer()->compositor() : nullptr;
        if (compositor->isFlushingLayers())
            return compositor;
    }
    
    return 0;
}

void RenderLayerCompositor::scheduleCompositingLayerUpdate()
{
    if (!m_updateCompositingLayersTimer.isActive())
        m_updateCompositingLayersTimer.startOneShot(0);
}

void RenderLayerCompositor::updateCompositingLayersTimerFired(Timer<RenderLayerCompositor>&)
{
    updateCompositingLayers(CompositingUpdateAfterLayout);
}

bool RenderLayerCompositor::hasAnyAdditionalCompositedLayers(const RenderLayer& rootLayer) const
{
    return m_compositedLayerCount > (rootLayer.isComposited() ? 1 : 0);
}

void RenderLayerCompositor::updateCompositingLayers(CompositingUpdateType updateType, RenderLayer* updateRoot)
{
    m_updateCompositingLayersTimer.stop();

    ASSERT(!m_renderView.document().inPageCache());
    
    // Compositing layers will be updated in Document::implicitClose() if suppressed here.
    if (!m_renderView.document().visualUpdatesAllowed())
        return;

    // Avoid updating the layers with old values. Compositing layers will be updated after the layout is finished.
    if (m_renderView.needsLayout())
        return;

    if (m_forceCompositingMode && !m_compositing)
        enableCompositingMode(true);

    if (!m_reevaluateCompositingAfterLayout && !m_compositing)
        return;

    AnimationUpdateBlock animationUpdateBlock(&m_renderView.frameView().frame().animation());

    TemporaryChange<bool> postLayoutChange(m_inPostLayoutUpdate, true);
    
    bool checkForHierarchyUpdate = m_reevaluateCompositingAfterLayout;
    bool needGeometryUpdate = false;

    switch (updateType) {
    case CompositingUpdateAfterStyleChange:
    case CompositingUpdateAfterLayout:
    case CompositingUpdateOnHitTest:
        checkForHierarchyUpdate = true;
        break;
    case CompositingUpdateOnScroll:
        checkForHierarchyUpdate = true; // Overlap can change with scrolling, so need to check for hierarchy updates.

        needGeometryUpdate = true;
        break;
    case CompositingUpdateOnCompositedScroll:
        needGeometryUpdate = true;
        break;
    }

    if (!checkForHierarchyUpdate && !needGeometryUpdate)
        return;

    bool needHierarchyUpdate = m_compositingLayersNeedRebuild;
    bool isFullUpdate = !updateRoot;

    // Only clear the flag if we're updating the entire hierarchy.
    m_compositingLayersNeedRebuild = false;
    updateRoot = &rootRenderLayer();

    if (isFullUpdate && updateType == CompositingUpdateAfterLayout)
        m_reevaluateCompositingAfterLayout = false;

#if !LOG_DISABLED
    double startTime = 0;
    if (compositingLogEnabled()) {
        ++m_rootLayerUpdateCount;
        startTime = monotonicallyIncreasingTime();
    }
#endif

    if (checkForHierarchyUpdate) {
        if (m_renderView.hasRenderNamedFlowThreads() && isFullUpdate)
            m_renderView.flowThreadController().updateFlowThreadsLayerToRegionMappingsIfNeeded();
        // Go through the layers in presentation order, so that we can compute which RenderLayers need compositing layers.
        // FIXME: we could maybe do this and the hierarchy udpate in one pass, but the parenting logic would be more complex.
        CompositingState compState(updateRoot);
        bool layersChanged = false;
        bool saw3DTransform = false;
        OverlapMap overlapTestRequestMap;
        computeCompositingRequirements(nullptr, *updateRoot, &overlapTestRequestMap, compState, layersChanged, saw3DTransform);
        needHierarchyUpdate |= layersChanged;
    }

#if !LOG_DISABLED
    if (compositingLogEnabled() && isFullUpdate && (needHierarchyUpdate || needGeometryUpdate)) {
        m_obligateCompositedLayerCount = 0;
        m_secondaryCompositedLayerCount = 0;
        m_obligatoryBackingStoreBytes = 0;
        m_secondaryBackingStoreBytes = 0;

        Frame& frame = m_renderView.frameView().frame();
        bool isMainFrame = !m_renderView.document().ownerElement();
        LOG(Compositing, "\nUpdate %d of %s.\n", m_rootLayerUpdateCount, isMainFrame ? "main frame" : frame.tree().uniqueName().string().utf8().data());
    }
#endif

    if (needHierarchyUpdate) {
        // Update the hierarchy of the compositing layers.
        Vector<GraphicsLayer*> childList;
        rebuildCompositingLayerTree(*updateRoot, childList, 0);

        // Host the document layer in the RenderView's root layer.
        if (isFullUpdate) {
            appendOverlayLayers(childList);
            // Even when childList is empty, don't drop out of compositing mode if there are
            // composited layers that we didn't hit in our traversal (e.g. because of visibility:hidden).
            if (childList.isEmpty() && !hasAnyAdditionalCompositedLayers(*updateRoot))
                destroyRootLayer();
            else if (m_rootContentLayer)
                m_rootContentLayer->setChildren(childList);
        }
        
        reattachSubframeScrollLayers();
    } else if (needGeometryUpdate) {
        // We just need to do a geometry update. This is only used for position:fixed scrolling;
        // most of the time, geometry is updated via RenderLayer::styleChanged().
        updateLayerTreeGeometry(*updateRoot, 0);
        ASSERT(!isFullUpdate || !m_subframeScrollLayersNeedReattach);
    }
    
#if !LOG_DISABLED
    if (compositingLogEnabled() && isFullUpdate && (needHierarchyUpdate || needGeometryUpdate)) {
        double endTime = monotonicallyIncreasingTime();
        LOG(Compositing, "Total layers   primary   secondary   obligatory backing (KB)   secondary backing(KB)   total backing (KB)  update time (ms)\n");

        LOG(Compositing, "%8d %11d %9d %20.2f %22.2f %22.2f %18.2f\n",
            m_obligateCompositedLayerCount + m_secondaryCompositedLayerCount, m_obligateCompositedLayerCount,
            m_secondaryCompositedLayerCount, m_obligatoryBackingStoreBytes / 1024, m_secondaryBackingStoreBytes / 1024, (m_obligatoryBackingStoreBytes + m_secondaryBackingStoreBytes) / 1024, 1000.0 * (endTime - startTime));
    }
#endif
    ASSERT(updateRoot || !m_compositingLayersNeedRebuild);

    if (!hasAcceleratedCompositing())
        enableCompositingMode(false);

    // Inform the inspector that the layer tree has changed.
    InspectorInstrumentation::layerTreeDidChange(page());
}

void RenderLayerCompositor::appendOverlayLayers(Vector<GraphicsLayer*>& childList)
{
    Frame& frame = m_renderView.frameView().frame();
    Page* page = frame.page();
    if (!page)
        return;

    if (GraphicsLayer* overlayLayer = page->chrome().client().documentOverlayLayerForFrame(frame))
        childList.append(overlayLayer);
}

void RenderLayerCompositor::layerBecameNonComposited(const RenderLayer& layer)
{
    // Inform the inspector that the given RenderLayer was destroyed.
    InspectorInstrumentation::renderLayerDestroyed(page(), &layer);

    ASSERT(m_compositedLayerCount > 0);
    --m_compositedLayerCount;
}

#if !LOG_DISABLED
void RenderLayerCompositor::logLayerInfo(const RenderLayer& layer, int depth)
{
    if (!compositingLogEnabled())
        return;

    RenderLayerBacking* backing = layer.backing();
    if (requiresCompositingLayer(layer) || layer.isRootLayer()) {
        ++m_obligateCompositedLayerCount;
        m_obligatoryBackingStoreBytes += backing->backingStoreMemoryEstimate();
    } else {
        ++m_secondaryCompositedLayerCount;
        m_secondaryBackingStoreBytes += backing->backingStoreMemoryEstimate();
    }

    StringBuilder logString;
    logString.append(String::format("%*p %dx%d %.2fKB", 12 + depth * 2, &layer,
        backing->compositedBounds().width().round(), backing->compositedBounds().height().round(),
        backing->backingStoreMemoryEstimate() / 1024));
    
    logString.append(" (");
    logString.append(logReasonsForCompositing(layer));
    logString.append(") ");

    if (backing->graphicsLayer()->contentsOpaque() || backing->paintsIntoCompositedAncestor()) {
        logString.append('[');
        if (backing->graphicsLayer()->contentsOpaque())
            logString.append("opaque");
        if (backing->paintsIntoCompositedAncestor())
            logString.append("paints into ancestor");
        logString.append("] ");
    }

    logString.append(layer.name());

    LOG(Compositing, "%s", logString.toString().utf8().data());
}
#endif

bool RenderLayerCompositor::updateBacking(RenderLayer& layer, CompositingChangeRepaint shouldRepaint)
{
    bool layerChanged = false;
    RenderLayer::ViewportConstrainedNotCompositedReason viewportConstrainedNotCompositedReason = RenderLayer::NoNotCompositedReason;

    if (needsToBeComposited(layer, &viewportConstrainedNotCompositedReason)) {
        enableCompositingMode();
        
        if (!layer.backing()) {
            // If we need to repaint, do so before making backing
            if (shouldRepaint == CompositingChangeRepaintNow)
                repaintOnCompositingChange(layer);

            layer.ensureBacking();

            // At this time, the ScrollingCoordinator only supports the top-level frame.
            if (layer.isRootLayer() && !m_renderView.document().ownerElement()) {
                updateScrollCoordinatedStatus(layer);
                if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
                    scrollingCoordinator->frameViewRootLayerDidChange(&m_renderView.frameView());
#if ENABLE(RUBBER_BANDING)
                if (Page* page = this->page()) {
                    updateLayerForHeader(page->headerHeight());
                    updateLayerForFooter(page->footerHeight());
                }
#endif
                if (m_renderView.frameView().frame().settings().backgroundShouldExtendBeyondPage())
                    m_rootContentLayer->setMasksToBounds(false);

                if (TiledBacking* tiledBacking = layer.backing()->tiledBacking())
                    tiledBacking->setTopContentInset(m_renderView.frameView().topContentInset());
            }

            // This layer and all of its descendants have cached repaints rects that are relative to
            // the repaint container, so change when compositing changes; we need to update them here.
            if (layer.parent())
                layer.computeRepaintRectsIncludingDescendants();

            layerChanged = true;
        }
    } else {
        if (layer.backing()) {
            // If we're removing backing on a reflection, clear the source GraphicsLayer's pointer to
            // its replica GraphicsLayer. In practice this should never happen because reflectee and reflection 
            // are both either composited, or not composited.
            if (layer.isReflection()) {
                RenderLayer* sourceLayer = toRenderLayerModelObject(layer.renderer().parent())->layer();
                if (RenderLayerBacking* backing = sourceLayer->backing()) {
                    ASSERT(backing->graphicsLayer()->replicaLayer() == layer.backing()->graphicsLayer());
                    backing->graphicsLayer()->setReplicatedByLayer(0);
                }
            }

            removeFromScrollCoordinatedLayers(layer);

            layer.clearBacking();
            layerChanged = true;

            // This layer and all of its descendants have cached repaints rects that are relative to
            // the repaint container, so change when compositing changes; we need to update them here.
            layer.computeRepaintRectsIncludingDescendants();

            // If we need to repaint, do so now that we've removed the backing
            if (shouldRepaint == CompositingChangeRepaintNow)
                repaintOnCompositingChange(layer);
        }
    }
    
#if ENABLE(VIDEO)
    if (layerChanged && layer.renderer().isVideo()) {
        // If it's a video, give the media player a chance to hook up to the layer.
        toRenderVideo(layer.renderer()).acceleratedRenderingStateChanged();
    }
#endif

    if (layerChanged && layer.renderer().isWidget()) {
        RenderLayerCompositor* innerCompositor = frameContentsCompositor(toRenderWidget(&layer.renderer()));
        if (innerCompositor && innerCompositor->inCompositingMode())
            innerCompositor->updateRootLayerAttachment();
    }
    
    if (layerChanged)
        layer.clearClipRectsIncludingDescendants(PaintingClipRects);

    // If a fixed position layer gained/lost a backing or the reason not compositing it changed,
    // the scrolling coordinator needs to recalculate whether it can do fast scrolling.
    if (layer.renderer().style().position() == FixedPosition) {
        if (layer.viewportConstrainedNotCompositedReason() != viewportConstrainedNotCompositedReason) {
            layer.setViewportConstrainedNotCompositedReason(viewportConstrainedNotCompositedReason);
            layerChanged = true;
        }
        if (layerChanged) {
            if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
                scrollingCoordinator->frameViewFixedObjectsDidChange(&m_renderView.frameView());
        }
    } else
        layer.setViewportConstrainedNotCompositedReason(RenderLayer::NoNotCompositedReason);
    
    if (layer.backing())
        layer.backing()->updateDebugIndicators(m_showDebugBorders, m_showRepaintCounter);

    return layerChanged;
}

bool RenderLayerCompositor::updateLayerCompositingState(RenderLayer& layer, CompositingChangeRepaint shouldRepaint)
{
    bool layerChanged = updateBacking(layer, shouldRepaint);

    // See if we need content or clipping layers. Methods called here should assume
    // that the compositing state of descendant layers has not been updated yet.
    if (layer.backing() && layer.backing()->updateConfiguration())
        layerChanged = true;

    return layerChanged;
}

void RenderLayerCompositor::repaintOnCompositingChange(RenderLayer& layer)
{
    // If the renderer is not attached yet, no need to repaint.
    if (&layer.renderer() != &m_renderView && !layer.renderer().parent())
        return;

    RenderLayerModelObject* repaintContainer = layer.renderer().containerForRepaint();
    if (!repaintContainer)
        repaintContainer = &m_renderView;

    layer.repaintIncludingNonCompositingDescendants(repaintContainer);
    if (repaintContainer == &m_renderView) {
        // The contents of this layer may be moving between the window
        // and a GraphicsLayer, so we need to make sure the window system
        // synchronizes those changes on the screen.
        m_renderView.frameView().setNeedsOneShotDrawingSynchronization();
    }
}

// This method assumes that layout is up-to-date, unlike repaintOnCompositingChange().
void RenderLayerCompositor::repaintInCompositedAncestor(RenderLayer& layer, const LayoutRect& rect)
{
    RenderLayer* compositedAncestor = layer.enclosingCompositingLayerForRepaint(ExcludeSelf);
    if (compositedAncestor) {
        ASSERT(compositedAncestor->backing());
        LayoutRect repaintRect = rect;
        repaintRect.move(layer.offsetFromAncestor(compositedAncestor));
        compositedAncestor->setBackingNeedsRepaintInRect(repaintRect);
    }

    // The contents of this layer may be moving from a GraphicsLayer to the window,
    // so we need to make sure the window system synchronizes those changes on the screen.
    if (compositedAncestor == m_renderView.layer())
        m_renderView.frameView().setNeedsOneShotDrawingSynchronization();
}

void RenderLayerCompositor::layerWasAdded(RenderLayer&, RenderLayer&)
{
    setCompositingLayersNeedRebuild();
}

void RenderLayerCompositor::layerWillBeRemoved(RenderLayer& parent, RenderLayer& child)
{
    if (!child.isComposited() || parent.renderer().documentBeingDestroyed())
        return;

    removeFromScrollCoordinatedLayers(child);
    repaintInCompositedAncestor(child, child.backing()->compositedBounds());

    setCompositingParent(child, nullptr);
    setCompositingLayersNeedRebuild();
}

RenderLayer* RenderLayerCompositor::enclosingNonStackingClippingLayer(const RenderLayer& layer) const
{
    for (RenderLayer* parent = layer.parent(); parent; parent = parent->parent()) {
        if (parent->isStackingContainer())
            return nullptr;
        if (parent->renderer().hasClipOrOverflowClip())
            return parent;
    }
    return nullptr;
}

void RenderLayerCompositor::addToOverlapMap(OverlapMap& overlapMap, RenderLayer& layer, IntRect& layerBounds, bool& boundsComputed)
{
    if (layer.isRootLayer())
        return;

    if (!boundsComputed) {
        // FIXME: If this layer's overlap bounds include its children, we don't need to add its
        // children's bounds to the overlap map.
        layerBounds = enclosingIntRect(overlapMap.geometryMap().absoluteRect(layer.overlapBounds()));
        // Empty rects never intersect, but we need them to for the purposes of overlap testing.
        if (layerBounds.isEmpty())
            layerBounds.setSize(IntSize(1, 1));
        boundsComputed = true;
    }

    IntRect clipRect = pixelSnappedIntRect(layer.backgroundClipRect(RenderLayer::ClipRectsContext(&rootRenderLayer(), AbsoluteClipRects)).rect()); // FIXME: Incorrect for CSS regions.

    // On iOS, pageScaleFactor() is not applied by RenderView, so we should not scale here.
    // FIXME: Set Settings::delegatesPageScaling to true for iOS.
#if !PLATFORM(IOS)
    const Settings& settings = m_renderView.frameView().frame().settings();
    if (!settings.delegatesPageScaling())
        clipRect.scale(pageScaleFactor());
#endif
    clipRect.intersect(layerBounds);
    overlapMap.add(&layer, clipRect);
}

void RenderLayerCompositor::addToOverlapMapRecursive(OverlapMap& overlapMap, RenderLayer& layer, RenderLayer* ancestorLayer)
{
    if (!canBeComposited(layer) || overlapMap.contains(&layer))
        return;

    // A null ancestorLayer is an indication that 'layer' has already been pushed.
    if (ancestorLayer)
        overlapMap.geometryMap().pushMappingsToAncestor(&layer, ancestorLayer);
    
    IntRect bounds;
    bool haveComputedBounds = false;
    addToOverlapMap(overlapMap, layer, bounds, haveComputedBounds);

#if !ASSERT_DISABLED
    LayerListMutationDetector mutationChecker(&layer);
#endif

    if (layer.isStackingContainer()) {
        if (Vector<RenderLayer*>* negZOrderList = layer.negZOrderList()) {
            for (size_t i = 0, size = negZOrderList->size(); i < size; ++i)
                addToOverlapMapRecursive(overlapMap, *negZOrderList->at(i), &layer);
        }
    }

    if (Vector<RenderLayer*>* normalFlowList = layer.normalFlowList()) {
        for (size_t i = 0, size = normalFlowList->size(); i < size; ++i)
            addToOverlapMapRecursive(overlapMap, *normalFlowList->at(i), &layer);
    }

    if (layer.isStackingContainer()) {
        if (Vector<RenderLayer*>* posZOrderList = layer.posZOrderList()) {
            for (size_t i = 0, size = posZOrderList->size(); i < size; ++i)
                addToOverlapMapRecursive(overlapMap, *posZOrderList->at(i), &layer);
        }
    }
    
    if (ancestorLayer)
        overlapMap.geometryMap().popMappingsToAncestor(ancestorLayer);
}

void RenderLayerCompositor::computeCompositingRequirementsForNamedFlowFixed(RenderLayer& layer, OverlapMap* overlapMap, CompositingState& childState, bool& layersChanged, bool& anyDescendantHas3DTransform)
{
    if (!layer.isRootLayer())
        return;

    if (!layer.renderer().view().hasRenderNamedFlowThreads())
        return;

    Vector<RenderLayer*> fixedLayers;
    layer.renderer().view().flowThreadController().collectFixedPositionedLayers(fixedLayers);

    for (size_t i = 0; i < fixedLayers.size(); ++i) {
        RenderLayer* fixedLayer = fixedLayers.at(i);
        computeCompositingRequirements(&layer, *fixedLayer, overlapMap, childState, layersChanged, anyDescendantHas3DTransform);
    }
}

//  Recurse through the layers in z-index and overflow order (which is equivalent to painting order)
//  For the z-order children of a compositing layer:
//      If a child layers has a compositing layer, then all subsequent layers must
//      be compositing in order to render above that layer.
//
//      If a child in the negative z-order list is compositing, then the layer itself
//      must be compositing so that its contents render over that child.
//      This implies that its positive z-index children must also be compositing.
//
void RenderLayerCompositor::computeCompositingRequirements(RenderLayer* ancestorLayer, RenderLayer& layer, OverlapMap* overlapMap, CompositingState& compositingState, bool& layersChanged, bool& descendantHas3DTransform)
{
    layer.updateDescendantDependentFlags();
    layer.updateLayerListsIfNeeded();

    if (layer.isFlowThreadCollectingGraphicsLayersUnderRegions()) {
        RenderFlowThread& flowThread = toRenderFlowThread(layer.renderer());
        layer.setHasCompositingDescendant(flowThread.hasCompositingRegionDescendant());

        // Before returning, we need to update the lists of all child layers. This is required because,
        // if this flow thread will not be painted (for instance because of having no regions, or only invalid regions),
        // the child layers will never have their lists updated (which would normally happen during painting).
        layer.updateDescendantsLayerListsIfNeeded(true);

        return;
    }

    if (overlapMap)
        overlapMap->geometryMap().pushMappingsToAncestor(&layer, ancestorLayer);
    
    // Clear the flag
    layer.setHasCompositingDescendant(false);
    layer.setIndirectCompositingReason(RenderLayer::IndirectCompositingReason::None);

    // Check if the layer needs to be composited for non-indirect reasons (ex. 3D transform).
    // We use this value to avoid checking the overlap-map, if we know for sure the layer
    // is already going to be composited for other reasons.
    bool willBeComposited = needsToBeComposited(layer);

    RenderLayer::IndirectCompositingReason compositingReason = compositingState.m_subtreeIsCompositing ? RenderLayer::IndirectCompositingReason::Stacking : RenderLayer::IndirectCompositingReason::None;
    bool haveComputedBounds = false;
    IntRect absBounds;

    // If we know for sure the layer is going to be composited, don't bother looking it up in the overlap map
    if (!willBeComposited && overlapMap && !overlapMap->isEmpty() && compositingState.m_testingOverlap) {
        // If we're testing for overlap, we only need to composite if we overlap something that is already composited.
        absBounds = enclosingIntRect(overlapMap->geometryMap().absoluteRect(layer.overlapBounds()));

        // Empty rects never intersect, but we need them to for the purposes of overlap testing.
        if (absBounds.isEmpty())
            absBounds.setSize(IntSize(1, 1));
        haveComputedBounds = true;
        compositingReason = overlapMap->overlapsLayers(absBounds) ? RenderLayer::IndirectCompositingReason::Overlap : RenderLayer::IndirectCompositingReason::None;
    }

#if ENABLE(VIDEO)
    // Video is special. It's the only RenderLayer type that can both have
    // RenderLayer children and whose children can't use its backing to render
    // into. These children (the controls) always need to be promoted into their
    // own layers to draw on top of the accelerated video.
    if (compositingState.m_compositingAncestor && compositingState.m_compositingAncestor->renderer().isVideo())
        compositingReason = RenderLayer::IndirectCompositingReason::Overlap;
#endif

    layer.setIndirectCompositingReason(compositingReason);

    // Check if the computed indirect reason will force the layer to become composited.
    if (!willBeComposited && layer.mustCompositeForIndirectReasons() && canBeComposited(layer))
        willBeComposited = true;
    ASSERT(willBeComposited == needsToBeComposited(layer));

    // The children of this layer don't need to composite, unless there is
    // a compositing layer among them, so start by inheriting the compositing
    // ancestor with m_subtreeIsCompositing set to false.
    CompositingState childState(compositingState);
    childState.m_subtreeIsCompositing = false;
#if ENABLE(CSS_COMPOSITING)
    childState.m_hasNotIsolatedCompositedBlendingDescendants = false;
#endif

    if (willBeComposited) {
        // Tell the parent it has compositing descendants.
        compositingState.m_subtreeIsCompositing = true;
        // This layer now acts as the ancestor for kids.
        childState.m_compositingAncestor = &layer;

        if (overlapMap)
            overlapMap->pushCompositingContainer();
        // This layer is going to be composited, so children can safely ignore the fact that there's an 
        // animation running behind this layer, meaning they can rely on the overlap map testing again.
        childState.m_testingOverlap = true;
    }

#if !ASSERT_DISABLED
    LayerListMutationDetector mutationChecker(&layer);
#endif

    bool anyDescendantHas3DTransform = false;

    if (layer.isStackingContainer()) {
        if (Vector<RenderLayer*>* negZOrderList = layer.negZOrderList()) {
            for (size_t i = 0, size = negZOrderList->size(); i < size; ++i) {
                computeCompositingRequirements(&layer, *negZOrderList->at(i), overlapMap, childState, layersChanged, anyDescendantHas3DTransform);

                // If we have to make a layer for this child, make one now so we can have a contents layer
                // (since we need to ensure that the -ve z-order child renders underneath our contents).
                if (!willBeComposited && childState.m_subtreeIsCompositing) {
                    // make layer compositing
                    layer.setIndirectCompositingReason(RenderLayer::IndirectCompositingReason::BackgroundLayer);
                    childState.m_compositingAncestor = &layer;
                    if (overlapMap)
                        overlapMap->pushCompositingContainer();
                    // This layer is going to be composited, so children can safely ignore the fact that there's an 
                    // animation running behind this layer, meaning they can rely on the overlap map testing again
                    childState.m_testingOverlap = true;
                    willBeComposited = true;
                }
            }
        }
    }

    if (layer.renderer().isRenderNamedFlowFragmentContainer()) {
        // We are going to collect layers from the RenderFlowThread into the GraphicsLayer of the parent of the
        // anonymous RenderRegion, but first we need to make sure that the parent itself of the region is going to
        // have a composited layer. We only want to make regions composited when there's an actual layer that we
        // need to move to that region.
        computeRegionCompositingRequirements(toRenderBlockFlow(layer.renderer()).renderNamedFlowFragment(), overlapMap, childState, layersChanged, anyDescendantHas3DTransform);
    }

    
    if (Vector<RenderLayer*>* normalFlowList = layer.normalFlowList()) {
        for (size_t i = 0, size = normalFlowList->size(); i < size; ++i)
            computeCompositingRequirements(&layer, *normalFlowList->at(i), overlapMap, childState, layersChanged, anyDescendantHas3DTransform);
    }

    if (layer.isStackingContainer()) {
        if (Vector<RenderLayer*>* posZOrderList = layer.posZOrderList()) {
            for (size_t i = 0, size = posZOrderList->size(); i < size; ++i)
                computeCompositingRequirements(&layer, *posZOrderList->at(i), overlapMap, childState, layersChanged, anyDescendantHas3DTransform);
        }
    }

    if (layer.isRootLayer())
        computeCompositingRequirementsForNamedFlowFixed(layer, overlapMap, childState, layersChanged, anyDescendantHas3DTransform);

    // If we just entered compositing mode, the root will have become composited (as long as accelerated compositing is enabled).
    if (layer.isRootLayer()) {
        if (inCompositingMode() && m_hasAcceleratedCompositing)
            willBeComposited = true;
    }
    
    ASSERT(willBeComposited == needsToBeComposited(layer));

    // All layers (even ones that aren't being composited) need to get added to
    // the overlap map. Layers that do not composite will draw into their
    // compositing ancestor's backing, and so are still considered for overlap.
    if (overlapMap && childState.m_compositingAncestor && !childState.m_compositingAncestor->isRootLayer())
        addToOverlapMap(*overlapMap, layer, absBounds, haveComputedBounds);

#if ENABLE(CSS_COMPOSITING)
    layer.setHasNotIsolatedCompositedBlendingDescendants(childState.m_hasNotIsolatedCompositedBlendingDescendants);
    ASSERT(!layer.hasNotIsolatedCompositedBlendingDescendants() || layer.hasNotIsolatedBlendingDescendants());
#endif
    // Now check for reasons to become composited that depend on the state of descendant layers.
    RenderLayer::IndirectCompositingReason indirectCompositingReason;
    if (!willBeComposited && canBeComposited(layer)
        && requiresCompositingForIndirectReason(layer.renderer(), childState.m_subtreeIsCompositing, anyDescendantHas3DTransform, indirectCompositingReason)) {
        layer.setIndirectCompositingReason(indirectCompositingReason);
        childState.m_compositingAncestor = &layer;
        if (overlapMap) {
            overlapMap->pushCompositingContainer();
            addToOverlapMapRecursive(*overlapMap, layer);
        }
        willBeComposited = true;
    }
    
    ASSERT(willBeComposited == needsToBeComposited(layer));
    if (layer.reflectionLayer()) {
        // FIXME: Shouldn't we call computeCompositingRequirements to handle a reflection overlapping with another renderer?
        layer.reflectionLayer()->setIndirectCompositingReason(willBeComposited ? RenderLayer::IndirectCompositingReason::Stacking : RenderLayer::IndirectCompositingReason::None);
    }

    // Subsequent layers in the parent stacking context also need to composite.
    if (childState.m_subtreeIsCompositing)
        compositingState.m_subtreeIsCompositing = true;

    // Set the flag to say that this SC has compositing children.
    layer.setHasCompositingDescendant(childState.m_subtreeIsCompositing);

    // setHasCompositingDescendant() may have changed the answer to needsToBeComposited() when clipping,
    // so test that again.
    bool isCompositedClippingLayer = canBeComposited(layer) && clipsCompositingDescendants(layer);

    // Turn overlap testing off for later layers if it's already off, or if we have an animating transform.
    // Note that if the layer clips its descendants, there's no reason to propagate the child animation to the parent layers. That's because
    // we know for sure the animation is contained inside the clipping rectangle, which is already added to the overlap map.
    if ((!childState.m_testingOverlap && !isCompositedClippingLayer) || isRunningAcceleratedTransformAnimation(layer.renderer()))
        compositingState.m_testingOverlap = false;
    
    if (isCompositedClippingLayer) {
        if (!willBeComposited) {
            childState.m_compositingAncestor = &layer;
            if (overlapMap) {
                overlapMap->pushCompositingContainer();
                addToOverlapMapRecursive(*overlapMap, layer);
            }
            willBeComposited = true;
         }
    }

#if ENABLE(CSS_COMPOSITING)
    if ((willBeComposited && layer.hasBlendMode())
        || (layer.hasNotIsolatedCompositedBlendingDescendants() && !layer.isolatesCompositedBlending()))
        compositingState.m_hasNotIsolatedCompositedBlendingDescendants = true;
#endif

    if (overlapMap && childState.m_compositingAncestor == &layer && !layer.isRootLayer())
        overlapMap->popCompositingContainer();

    // If we're back at the root, and no other layers need to be composited, and the root layer itself doesn't need
    // to be composited, then we can drop out of compositing mode altogether. However, don't drop out of compositing mode
    // if there are composited layers that we didn't hit in our traversal (e.g. because of visibility:hidden).
    if (layer.isRootLayer() && !childState.m_subtreeIsCompositing && !requiresCompositingLayer(layer) && !m_forceCompositingMode && !hasAnyAdditionalCompositedLayers(layer)) {
        // Don't drop out of compositing on iOS, because we may flash. See <rdar://problem/8348337>.
#if !PLATFORM(IOS)
        enableCompositingMode(false);
        willBeComposited = false;
#endif
    }
    
    // If the layer is going into compositing mode, repaint its old location.
    ASSERT(willBeComposited == needsToBeComposited(layer));
    if (!layer.isComposited() && willBeComposited)
        repaintOnCompositingChange(layer);

    // Update backing now, so that we can use isComposited() reliably during tree traversal in rebuildCompositingLayerTree().
    if (updateBacking(layer, CompositingChangeRepaintNow))
        layersChanged = true;

    if (layer.reflectionLayer() && updateLayerCompositingState(*layer.reflectionLayer(), CompositingChangeRepaintNow))
        layersChanged = true;

    descendantHas3DTransform |= anyDescendantHas3DTransform || layer.has3DTransform();

    if (overlapMap)
        overlapMap->geometryMap().popMappingsToAncestor(ancestorLayer);
}

void RenderLayerCompositor::computeRegionCompositingRequirements(RenderNamedFlowFragment* region, OverlapMap* overlapMap, CompositingState& childState, bool& layersChanged, bool& anyDescendantHas3DTransform)
{
    if (!region->isValid())
        return;

    RenderFlowThread* flowThread = region->flowThread();
    
    if (overlapMap)
        overlapMap->geometryMap().pushRenderFlowThread(flowThread);

    if (const RenderLayerList* layerList = flowThread->getLayerListForRegion(region)) {
        for (size_t i = 0, listSize = layerList->size(); i < listSize; ++i) {
            RenderLayer& curLayer = *layerList->at(i);
            ASSERT(flowThread->regionForCompositedLayer(curLayer) == region);
            computeCompositingRequirements(flowThread->layer(), curLayer, overlapMap, childState, layersChanged, anyDescendantHas3DTransform);
        }
    }

    if (overlapMap)
        overlapMap->geometryMap().popMappingsToAncestor(&region->layerOwner());
}

void RenderLayerCompositor::setCompositingParent(RenderLayer& childLayer, RenderLayer* parentLayer)
{
    ASSERT(!parentLayer || childLayer.ancestorCompositingLayer() == parentLayer);
    ASSERT(childLayer.isComposited());

    // It's possible to be called with a parent that isn't yet composited when we're doing
    // partial updates as required by painting or hit testing. Just bail in that case;
    // we'll do a full layer update soon.
    if (!parentLayer || !parentLayer->isComposited())
        return;

    if (parentLayer) {
        GraphicsLayer* hostingLayer = parentLayer->backing()->parentForSublayers();
        GraphicsLayer* hostedLayer = childLayer.backing()->childForSuperlayers();
        
        hostingLayer->addChild(hostedLayer);
    } else
        childLayer.backing()->childForSuperlayers()->removeFromParent();
}

void RenderLayerCompositor::removeCompositedChildren(RenderLayer& layer)
{
    ASSERT(layer.isComposited());

    layer.backing()->parentForSublayers()->removeAllChildren();
}

#if ENABLE(VIDEO)
bool RenderLayerCompositor::canAccelerateVideoRendering(RenderVideo& video) const
{
    if (!m_hasAcceleratedCompositing)
        return false;

    return video.supportsAcceleratedRendering();
}
#endif

void RenderLayerCompositor::rebuildCompositingLayerTreeForNamedFlowFixed(RenderLayer& layer, Vector<GraphicsLayer*>& childGraphicsLayersOfEnclosingLayer, int depth)
{
    if (!layer.isRootLayer())
        return;

    if (!layer.renderer().view().hasRenderNamedFlowThreads())
        return;

    Vector<RenderLayer*> fixedLayers;
    layer.renderer().view().flowThreadController().collectFixedPositionedLayers(fixedLayers);

    for (size_t i = 0; i < fixedLayers.size(); ++i) {
        RenderLayer* fixedLayer = fixedLayers.at(i);
        rebuildCompositingLayerTree(*fixedLayer, childGraphicsLayersOfEnclosingLayer, depth);
    }
}

void RenderLayerCompositor::rebuildCompositingLayerTree(RenderLayer& layer, Vector<GraphicsLayer*>& childLayersOfEnclosingLayer, int depth)
{
    // Make the layer compositing if necessary, and set up clipping and content layers.
    // Note that we can only do work here that is independent of whether the descendant layers
    // have been processed. computeCompositingRequirements() will already have done the repaint if necessary.

    // Do not iterate the RenderFlowThread directly. We are going to collect composited layers as part of regions.
    if (layer.isFlowThreadCollectingGraphicsLayersUnderRegions())
        return;

    RenderLayerBacking* layerBacking = layer.backing();
    if (layerBacking) {
        // The compositing state of all our children has been updated already, so now
        // we can compute and cache the composited bounds for this layer.
        layerBacking->updateCompositedBounds();

        if (RenderLayer* reflection = layer.reflectionLayer()) {
            if (reflection->backing())
                reflection->backing()->updateCompositedBounds();
        }

        if (layerBacking->updateConfiguration())
            layerBacking->updateDebugIndicators(m_showDebugBorders, m_showRepaintCounter);
        
        layerBacking->updateGeometry();

        if (!layer.parent())
            updateRootLayerPosition();

#if !LOG_DISABLED
        logLayerInfo(layer, depth);
#else
        UNUSED_PARAM(depth);
#endif
        if (layerBacking->hasUnpositionedOverflowControlsLayers())
            layer.positionNewlyCreatedOverflowControls();
    }

    // If this layer has backing, then we are collecting its children, otherwise appending
    // to the compositing child list of an enclosing layer.
    Vector<GraphicsLayer*> layerChildren;
    Vector<GraphicsLayer*>& childList = layerBacking ? layerChildren : childLayersOfEnclosingLayer;

#if !ASSERT_DISABLED
    LayerListMutationDetector mutationChecker(&layer);
#endif

    if (layer.isStackingContainer()) {
        if (Vector<RenderLayer*>* negZOrderList = layer.negZOrderList()) {
            for (size_t i = 0, size = negZOrderList->size(); i < size; ++i)
                rebuildCompositingLayerTree(*negZOrderList->at(i), childList, depth + 1);
        }

        // If a negative z-order child is compositing, we get a foreground layer which needs to get parented.
        if (layerBacking && layerBacking->foregroundLayer())
            childList.append(layerBacking->foregroundLayer());
    }

    if (layer.renderer().isRenderNamedFlowFragmentContainer())
        rebuildRegionCompositingLayerTree(toRenderBlockFlow(layer.renderer()).renderNamedFlowFragment(), layerChildren, depth + 1);

    if (Vector<RenderLayer*>* normalFlowList = layer.normalFlowList()) {
        for (size_t i = 0, size = normalFlowList->size(); i < size; ++i)
            rebuildCompositingLayerTree(*normalFlowList->at(i), childList, depth + 1);
    }
    
    if (layer.isStackingContainer()) {
        if (Vector<RenderLayer*>* posZOrderList = layer.posZOrderList()) {
            for (size_t i = 0, size = posZOrderList->size(); i < size; ++i)
                rebuildCompositingLayerTree(*posZOrderList->at(i), childList, depth + 1);
        }
    }

    if (layer.isRootLayer())
        rebuildCompositingLayerTreeForNamedFlowFixed(layer, childList, depth + 1);

    if (layerBacking) {
        bool parented = false;
        if (layer.renderer().isWidget())
            parented = parentFrameContentLayers(toRenderWidget(&layer.renderer()));

        if (!parented)
            layerBacking->parentForSublayers()->setChildren(layerChildren);

        // If the layer has a clipping layer the overflow controls layers will be siblings of the clipping layer.
        // Otherwise, the overflow control layers are normal children.
        if (!layerBacking->hasClippingLayer() && !layerBacking->hasScrollingLayer()) {
            if (GraphicsLayer* overflowControlLayer = layerBacking->layerForHorizontalScrollbar()) {
                overflowControlLayer->removeFromParent();
                layerBacking->parentForSublayers()->addChild(overflowControlLayer);
            }

            if (GraphicsLayer* overflowControlLayer = layerBacking->layerForVerticalScrollbar()) {
                overflowControlLayer->removeFromParent();
                layerBacking->parentForSublayers()->addChild(overflowControlLayer);
            }

            if (GraphicsLayer* overflowControlLayer = layerBacking->layerForScrollCorner()) {
                overflowControlLayer->removeFromParent();
                layerBacking->parentForSublayers()->addChild(overflowControlLayer);
            }
        }

        childLayersOfEnclosingLayer.append(layerBacking->childForSuperlayers());
    }
    
    if (RenderLayerBacking* layerBacking = layer.backing())
        layerBacking->updateAfterDescendents();
}

void RenderLayerCompositor::rebuildRegionCompositingLayerTree(RenderNamedFlowFragment* region, Vector<GraphicsLayer*>& childList, int depth)
{
    if (!region->isValid())
        return;

    RenderFlowThread* flowThread = region->flowThread();
    ASSERT(flowThread->collectsGraphicsLayersUnderRegions());
    if (const RenderLayerList* layerList = flowThread->getLayerListForRegion(region)) {
        for (size_t i = 0, listSize = layerList->size(); i < listSize; ++i) {
            RenderLayer& curLayer = *layerList->at(i);
            ASSERT(flowThread->regionForCompositedLayer(curLayer) == region);
            rebuildCompositingLayerTree(curLayer, childList, depth + 1);
        }
    }
}

void RenderLayerCompositor::frameViewDidChangeLocation(const IntPoint& contentsOffset)
{
    if (m_overflowControlsHostLayer)
        m_overflowControlsHostLayer->setPosition(contentsOffset);
}

void RenderLayerCompositor::frameViewDidChangeSize()
{
    if (m_clipLayer) {
        const FrameView& frameView = m_renderView.frameView();
        m_clipLayer->setSize(frameView.unscaledVisibleContentSizeIncludingObscuredArea());
        m_clipLayer->setPosition(positionForClipLayer());

        frameViewDidScroll();
        updateOverflowControlsLayers();

#if ENABLE(RUBBER_BANDING)
        if (m_layerForOverhangAreas) {
            m_layerForOverhangAreas->setSize(frameView.frameRect().size());
            m_layerForOverhangAreas->setPosition(FloatPoint(0, m_renderView.frameView().topContentInset()));
        }
#endif
    }
}

bool RenderLayerCompositor::hasCoordinatedScrolling() const
{
    ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator();
    return scrollingCoordinator && scrollingCoordinator->coordinatesScrollingForFrameView(&m_renderView.frameView());
}

void RenderLayerCompositor::updateScrollLayerPosition()
{
    ASSERT(m_scrollLayer);

    FrameView& frameView = m_renderView.frameView();
    IntPoint scrollPosition = frameView.scrollPosition();

    m_scrollLayer->setPosition(FloatPoint(-scrollPosition.x(), -scrollPosition.y()));

    if (GraphicsLayer* fixedBackgroundLayer = fixedRootBackgroundLayer())
        fixedBackgroundLayer->setPosition(toLayoutPoint(frameView.scrollOffsetForFixedPosition()));
}

FloatPoint RenderLayerCompositor::positionForClipLayer() const
{
    return FloatPoint(0, FrameView::yPositionForInsetClipLayer(m_renderView.frameView().scrollPosition(), m_renderView.frameView().topContentInset()));
}

void RenderLayerCompositor::frameViewDidScroll()
{
    if (!m_scrollLayer)
        return;

    // If there's a scrolling coordinator that manages scrolling for this frame view,
    // it will also manage updating the scroll layer position.
    if (hasCoordinatedScrolling()) {
        // We have to schedule a flush in order for the main TiledBacking to update its tile coverage.
        scheduleLayerFlushNow();
        return;
    }

    updateScrollLayerPosition();
}

void RenderLayerCompositor::frameViewDidAddOrRemoveScrollbars()
{
    updateOverflowControlsLayers();
}

void RenderLayerCompositor::frameViewDidLayout()
{
    RenderLayerBacking* renderViewBacking = m_renderView.layer()->backing();
    if (renderViewBacking)
        renderViewBacking->adjustTiledBackingCoverage();
}

void RenderLayerCompositor::rootFixedBackgroundsChanged()
{
    RenderLayerBacking* renderViewBacking = m_renderView.layer()->backing();
    if (renderViewBacking && renderViewBacking->usingTiledBacking())
        setCompositingLayersNeedRebuild();
}

void RenderLayerCompositor::scrollingLayerDidChange(RenderLayer& layer)
{
    if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
        scrollingCoordinator->scrollableAreaScrollLayerDidChange(&layer);
}

void RenderLayerCompositor::fixedRootBackgroundLayerChanged()
{
    if (m_renderView.documentBeingDestroyed())
        return;

    if (m_renderView.layer()->isComposited())
        updateScrollCoordinatedStatus(*m_renderView.layer());
}

String RenderLayerCompositor::layerTreeAsText(LayerTreeFlags flags)
{
    updateCompositingLayers(CompositingUpdateAfterLayout);

    if (!m_rootContentLayer)
        return String();

    flushPendingLayerChanges(true);

    LayerTreeAsTextBehavior layerTreeBehavior = LayerTreeAsTextBehaviorNormal;
    if (flags & LayerTreeFlagsIncludeDebugInfo)
        layerTreeBehavior |= LayerTreeAsTextDebug;
    if (flags & LayerTreeFlagsIncludeVisibleRects)
        layerTreeBehavior |= LayerTreeAsTextIncludeVisibleRects;
    if (flags & LayerTreeFlagsIncludeTileCaches)
        layerTreeBehavior |= LayerTreeAsTextIncludeTileCaches;
    if (flags & LayerTreeFlagsIncludeRepaintRects)
        layerTreeBehavior |= LayerTreeAsTextIncludeRepaintRects;
    if (flags & LayerTreeFlagsIncludePaintingPhases)
        layerTreeBehavior |= LayerTreeAsTextIncludePaintingPhases;
    if (flags & LayerTreeFlagsIncludeContentLayers)
        layerTreeBehavior |= LayerTreeAsTextIncludeContentLayers;

    // We skip dumping the scroll and clip layers to keep layerTreeAsText output
    // similar between platforms.
    String layerTreeText = m_rootContentLayer->layerTreeAsText(layerTreeBehavior);

    // Dump an empty layer tree only if the only composited layer is the main frame's tiled backing,
    // so that tests expecting us to drop out of accelerated compositing when there are no layers succeed.
    if (!hasAnyAdditionalCompositedLayers(rootRenderLayer()) && mainFrameBackingIsTiled() && !(layerTreeBehavior & LayerTreeAsTextIncludeTileCaches))
        layerTreeText = "";

    // The true root layer is not included in the dump, so if we want to report
    // its repaint rects, they must be included here.
    if (flags & LayerTreeFlagsIncludeRepaintRects)
        return m_renderView.frameView().trackedRepaintRectsAsText() + layerTreeText;

    return layerTreeText;
}

RenderLayerCompositor* RenderLayerCompositor::frameContentsCompositor(RenderWidget* renderer)
{
    if (Document* contentDocument = renderer->frameOwnerElement().contentDocument()) {
        if (RenderView* view = contentDocument->renderView())
            return &view->compositor();
    }
    return 0;
}

bool RenderLayerCompositor::parentFrameContentLayers(RenderWidget* renderer)
{
    RenderLayerCompositor* innerCompositor = frameContentsCompositor(renderer);
    if (!innerCompositor || !innerCompositor->inCompositingMode() || innerCompositor->rootLayerAttachment() != RootLayerAttachedViaEnclosingFrame)
        return false;
    
    RenderLayer* layer = renderer->layer();
    if (!layer->isComposited())
        return false;

    RenderLayerBacking* backing = layer->backing();
    GraphicsLayer* hostingLayer = backing->parentForSublayers();
    GraphicsLayer* rootLayer = innerCompositor->rootGraphicsLayer();
    if (hostingLayer->children().size() != 1 || hostingLayer->children()[0] != rootLayer) {
        hostingLayer->removeAllChildren();
        hostingLayer->addChild(rootLayer);
    }
    return true;
}

// This just updates layer geometry without changing the hierarchy.
void RenderLayerCompositor::updateLayerTreeGeometry(RenderLayer& layer, int depth)
{
    if (RenderLayerBacking* layerBacking = layer.backing()) {
        // The compositing state of all our children has been updated already, so now
        // we can compute and cache the composited bounds for this layer.
        layerBacking->updateCompositedBounds();

        if (RenderLayer* reflection = layer.reflectionLayer()) {
            if (reflection->backing())
                reflection->backing()->updateCompositedBounds();
        }

        layerBacking->updateConfiguration();
        layerBacking->updateGeometry();

        if (!layer.parent())
            updateRootLayerPosition();

#if !LOG_DISABLED
        logLayerInfo(layer, depth);
#else
        UNUSED_PARAM(depth);
#endif
    }

#if !ASSERT_DISABLED
    LayerListMutationDetector mutationChecker(&layer);
#endif

    if (layer.isStackingContainer()) {
        if (Vector<RenderLayer*>* negZOrderList = layer.negZOrderList()) {
            for (size_t i = 0, size = negZOrderList->size(); i < size; ++i)
                updateLayerTreeGeometry(*negZOrderList->at(i), depth + 1);
        }
    }

    if (Vector<RenderLayer*>* normalFlowList = layer.normalFlowList()) {
        for (size_t i = 0, size = normalFlowList->size(); i < size; ++i)
            updateLayerTreeGeometry(*normalFlowList->at(i), depth + 1);
    }
    
    if (layer.isStackingContainer()) {
        if (Vector<RenderLayer*>* posZOrderList = layer.posZOrderList()) {
            for (size_t i = 0, size = posZOrderList->size(); i < size; ++i)
                updateLayerTreeGeometry(*posZOrderList->at(i), depth + 1);
        }
    }

    if (RenderLayerBacking* layerBacking = layer.backing())
        layerBacking->updateAfterDescendents();
}

// Recurs down the RenderLayer tree until its finds the compositing descendants of compositingAncestor and updates their geometry.
void RenderLayerCompositor::updateCompositingDescendantGeometry(RenderLayer& compositingAncestor, RenderLayer& layer, bool compositedChildrenOnly)
{
    if (&layer != &compositingAncestor) {
        if (RenderLayerBacking* layerBacking = layer.backing()) {
            layerBacking->updateCompositedBounds();

            if (RenderLayer* reflection = layer.reflectionLayer()) {
                if (reflection->backing())
                    reflection->backing()->updateCompositedBounds();
            }

            layerBacking->updateGeometry();
            if (compositedChildrenOnly) {
                layerBacking->updateAfterDescendents();
                return;
            }
        }
    }

    if (layer.reflectionLayer())
        updateCompositingDescendantGeometry(compositingAncestor, *layer.reflectionLayer(), compositedChildrenOnly);

    if (!layer.hasCompositingDescendant())
        return;

#if !ASSERT_DISABLED
    LayerListMutationDetector mutationChecker(&layer);
#endif
    
    if (layer.isStackingContainer()) {
        if (Vector<RenderLayer*>* negZOrderList = layer.negZOrderList()) {
            size_t listSize = negZOrderList->size();
            for (size_t i = 0; i < listSize; ++i)
                updateCompositingDescendantGeometry(compositingAncestor, *negZOrderList->at(i), compositedChildrenOnly);
        }
    }

    if (Vector<RenderLayer*>* normalFlowList = layer.normalFlowList()) {
        size_t listSize = normalFlowList->size();
        for (size_t i = 0; i < listSize; ++i)
            updateCompositingDescendantGeometry(compositingAncestor, *normalFlowList->at(i), compositedChildrenOnly);
    }
    
    if (layer.isStackingContainer()) {
        if (Vector<RenderLayer*>* posZOrderList = layer.posZOrderList()) {
            size_t listSize = posZOrderList->size();
            for (size_t i = 0; i < listSize; ++i)
                updateCompositingDescendantGeometry(compositingAncestor, *posZOrderList->at(i), compositedChildrenOnly);
        }
    }
    
    if (&layer != &compositingAncestor) {
        if (RenderLayerBacking* layerBacking = layer.backing())
            layerBacking->updateAfterDescendents();
    }
}

void RenderLayerCompositor::repaintCompositedLayers()
{
    recursiveRepaintLayer(rootRenderLayer());
}

void RenderLayerCompositor::recursiveRepaintLayer(RenderLayer& layer)
{
    // FIXME: This method does not work correctly with transforms.
    if (layer.isComposited() && !layer.backing()->paintsIntoCompositedAncestor())
        layer.setBackingNeedsRepaint();

#if !ASSERT_DISABLED
    LayerListMutationDetector mutationChecker(&layer);
#endif

    if (layer.hasCompositingDescendant()) {
        if (Vector<RenderLayer*>* negZOrderList = layer.negZOrderList()) {
            for (size_t i = 0, size = negZOrderList->size(); i < size; ++i)
                recursiveRepaintLayer(*negZOrderList->at(i));
        }

        if (Vector<RenderLayer*>* posZOrderList = layer.posZOrderList()) {
            for (size_t i = 0, size = posZOrderList->size(); i < size; ++i)
                recursiveRepaintLayer(*posZOrderList->at(i));
        }
    }
    if (Vector<RenderLayer*>* normalFlowList = layer.normalFlowList()) {
        for (size_t i = 0, size = normalFlowList->size(); i < size; ++i)
            recursiveRepaintLayer(*normalFlowList->at(i));
    }
}

RenderLayer& RenderLayerCompositor::rootRenderLayer() const
{
    return *m_renderView.layer();
}

GraphicsLayer* RenderLayerCompositor::rootGraphicsLayer() const
{
    if (m_overflowControlsHostLayer)
        return m_overflowControlsHostLayer.get();
    return m_rootContentLayer.get();
}

GraphicsLayer* RenderLayerCompositor::scrollLayer() const
{
    return m_scrollLayer.get();
}

GraphicsLayer* RenderLayerCompositor::clipLayer() const
{
    return m_clipLayer.get();
}

GraphicsLayer* RenderLayerCompositor::rootContentLayer() const
{
    return m_rootContentLayer.get();
}

#if ENABLE(RUBBER_BANDING)
GraphicsLayer* RenderLayerCompositor::headerLayer() const
{
    return m_layerForHeader.get();
}

GraphicsLayer* RenderLayerCompositor::footerLayer() const
{
    return m_layerForFooter.get();
}
#endif

void RenderLayerCompositor::setIsInWindowForLayerIncludingDescendants(RenderLayer& layer, bool isInWindow)
{
    if (layer.isComposited() && layer.backing()->usingTiledBacking())
        layer.backing()->tiledBacking()->setIsInWindow(isInWindow);

    // No need to recurse if we don't have any other tiled layers.
    if (hasNonMainLayersWithTiledBacking())
        return;

    for (RenderLayer* childLayer = layer.firstChild(); childLayer; childLayer = childLayer->nextSibling())
        setIsInWindowForLayerIncludingDescendants(*childLayer, isInWindow);
}

void RenderLayerCompositor::setIsInWindow(bool isInWindow)
{
    setIsInWindowForLayerIncludingDescendants(*m_renderView.layer(), isInWindow);
    
    if (!inCompositingMode())
        return;

    if (isInWindow) {
        if (m_rootLayerAttachment != RootLayerUnattached)
            return;

        RootLayerAttachment attachment = shouldPropagateCompositingToEnclosingFrame() ? RootLayerAttachedViaEnclosingFrame : RootLayerAttachedViaChromeClient;
        attachRootLayer(attachment);
#if PLATFORM(IOS)
        registerAllViewportConstrainedLayers();
        registerAllScrollingLayers();
#endif
    } else {
        if (m_rootLayerAttachment == RootLayerUnattached)
            return;

        detachRootLayer();
#if PLATFORM(IOS)
        unregisterAllViewportConstrainedLayers();
        unregisterAllScrollingLayers();
#endif
    }
}

void RenderLayerCompositor::clearBackingForLayerIncludingDescendants(RenderLayer& layer)
{
    if (layer.isComposited()) {
        removeFromScrollCoordinatedLayers(layer);
        layer.clearBacking();
    }

    for (RenderLayer* childLayer = layer.firstChild(); childLayer; childLayer = childLayer->nextSibling())
        clearBackingForLayerIncludingDescendants(*childLayer);
}

void RenderLayerCompositor::clearBackingForAllLayers()
{
    clearBackingForLayerIncludingDescendants(*m_renderView.layer());
}

void RenderLayerCompositor::updateRootLayerPosition()
{
    if (m_rootContentLayer) {
        const IntRect& documentRect = m_renderView.documentRect();
        m_rootContentLayer->setSize(documentRect.size());        
        m_rootContentLayer->setPosition(FloatPoint(documentRect.x(), documentRect.y()
            + FrameView::yPositionForRootContentLayer(m_renderView.frameView().scrollPosition(), m_renderView.frameView().topContentInset(), m_renderView.frameView().headerHeight())));
        m_rootContentLayer->setAnchorPoint(FloatPoint3D());
    }
    if (m_clipLayer) {
        m_clipLayer->setSize(m_renderView.frameView().unscaledVisibleContentSizeIncludingObscuredArea());
        m_clipLayer->setPosition(positionForClipLayer());
    }

#if ENABLE(RUBBER_BANDING)
    if (m_contentShadowLayer) {
        m_contentShadowLayer->setPosition(m_rootContentLayer->position());
        m_contentShadowLayer->setSize(m_rootContentLayer->size());
    }

    updateLayerForTopOverhangArea(m_layerForTopOverhangArea != nullptr);
    updateLayerForBottomOverhangArea(m_layerForBottomOverhangArea != nullptr);
    updateLayerForHeader(m_layerForHeader != nullptr);
    updateLayerForFooter(m_layerForFooter != nullptr);
#endif
}

bool RenderLayerCompositor::has3DContent() const
{
    return layerHas3DContent(rootRenderLayer());
}

bool RenderLayerCompositor::allowsIndependentlyCompositedFrames(const FrameView* view)
{
#if PLATFORM(MAC)
    // frames are only independently composited in Mac pre-WebKit2.
    return view->platformWidget();
#else
    UNUSED_PARAM(view);
#endif
    return false;
}

bool RenderLayerCompositor::shouldPropagateCompositingToEnclosingFrame() const
{
    // Parent document content needs to be able to render on top of a composited frame, so correct behavior
    // is to have the parent document become composited too. However, this can cause problems on platforms that
    // use native views for frames (like Mac), so disable that behavior on those platforms for now.
    HTMLFrameOwnerElement* ownerElement = m_renderView.document().ownerElement();
    RenderElement* renderer = ownerElement ? ownerElement->renderer() : 0;

    // If we are the top-level frame, don't propagate.
    if (!ownerElement)
        return false;

    if (!allowsIndependentlyCompositedFrames(&m_renderView.frameView()))
        return true;

    if (!renderer || !renderer->isWidget())
        return false;

    // On Mac, only propagate compositing if the frame is overlapped in the parent
    // document, or the parent is already compositing, or the main frame is scaled.
    Page* page = this->page();
    if (page && page->pageScaleFactor() != 1)
        return true;
    
    RenderWidget* frameRenderer = toRenderWidget(renderer);
    if (frameRenderer->widget()) {
        ASSERT(frameRenderer->widget()->isFrameView());
        FrameView* view = toFrameView(frameRenderer->widget());
        if (view->isOverlappedIncludingAncestors() || view->hasCompositingAncestor())
            return true;
    }

    return false;
}

bool RenderLayerCompositor::needsToBeComposited(const RenderLayer& layer, RenderLayer::ViewportConstrainedNotCompositedReason* viewportConstrainedNotCompositedReason) const
{
    if (!canBeComposited(layer))
        return false;

    return requiresCompositingLayer(layer, viewportConstrainedNotCompositedReason) || layer.mustCompositeForIndirectReasons() || (inCompositingMode() && layer.isRootLayer());
}

// Note: this specifies whether the RL needs a compositing layer for intrinsic reasons.
// Use needsToBeComposited() to determine if a RL actually needs a compositing layer.
// static
bool RenderLayerCompositor::requiresCompositingLayer(const RenderLayer& layer, RenderLayer::ViewportConstrainedNotCompositedReason* viewportConstrainedNotCompositedReason) const
{
    auto renderer = &layer.renderer();

    // The compositing state of a reflection should match that of its reflected layer.
    if (layer.isReflection())
        renderer = toRenderLayerModelObject(renderer->parent()); // The RenderReplica's parent is the object being reflected.

    // The root layer always has a compositing layer, but it may not have backing.
    return requiresCompositingForTransform(*renderer)
        || requiresCompositingForVideo(*renderer)
        || requiresCompositingForCanvas(*renderer)
        || requiresCompositingForPlugin(*renderer)
        || requiresCompositingForFrame(*renderer)
        || requiresCompositingForBackfaceVisibility(*renderer)
        || clipsCompositingDescendants(*renderer->layer())
        || requiresCompositingForAnimation(*renderer)
        || requiresCompositingForFilters(*renderer)
        || requiresCompositingForPosition(*renderer, *renderer->layer(), viewportConstrainedNotCompositedReason)
#if PLATFORM(IOS)
        || requiresCompositingForScrolling(*renderer->layer())
#endif
        || requiresCompositingForOverflowScrolling(*renderer->layer());
}

bool RenderLayerCompositor::canBeComposited(const RenderLayer& layer) const
{
    if (m_hasAcceleratedCompositing && layer.isSelfPaintingLayer()) {
        if (!layer.isInsideFlowThread())
            return true;

        // CSS Regions flow threads do not need to be composited as we use composited RenderRegions
        // to render the background of the RenderFlowThread.
        if (layer.isRenderFlowThread())
            return false;

        return true;
    }
    return false;
}

bool RenderLayerCompositor::requiresOwnBackingStore(const RenderLayer& layer, const RenderLayer* compositingAncestorLayer, const LayoutRect& layerCompositedBoundsInAncestor, const LayoutRect& ancestorCompositedBounds) const
{
    auto& renderer = layer.renderer();

    if (compositingAncestorLayer
        && !(compositingAncestorLayer->backing()->graphicsLayer()->drawsContent()
            || compositingAncestorLayer->backing()->paintsIntoWindow()
            || compositingAncestorLayer->backing()->paintsIntoCompositedAncestor()))
        return true;

    if (layer.isRootLayer()
        || layer.transform() // note: excludes perspective and transformStyle3D.
        || requiresCompositingForVideo(renderer)
        || requiresCompositingForCanvas(renderer)
        || requiresCompositingForPlugin(renderer)
        || requiresCompositingForFrame(renderer)
        || requiresCompositingForBackfaceVisibility(renderer)
        || requiresCompositingForAnimation(renderer)
        || requiresCompositingForFilters(renderer)
        || requiresCompositingForPosition(renderer, layer)
        || requiresCompositingForOverflowScrolling(layer)
        || renderer.isTransparent()
        || renderer.hasMask()
        || renderer.hasReflection()
        || renderer.hasFilter()
#if PLATFORM(IOS)
        || requiresCompositingForScrolling(layer)
#endif
        )
        return true;
        
    
    if (layer.mustCompositeForIndirectReasons()) {
        RenderLayer::IndirectCompositingReason reason = layer.indirectCompositingReason();
        return reason == RenderLayer::IndirectCompositingReason::Overlap
            || reason == RenderLayer::IndirectCompositingReason::Stacking
            || reason == RenderLayer::IndirectCompositingReason::BackgroundLayer
            || reason == RenderLayer::IndirectCompositingReason::GraphicalEffect
            || reason == RenderLayer::IndirectCompositingReason::Preserve3D; // preserve-3d has to create backing store to ensure that 3d-transformed elements intersect.
    }

    if (!ancestorCompositedBounds.contains(layerCompositedBoundsInAncestor))
        return true;

    return false;
}

CompositingReasons RenderLayerCompositor::reasonsForCompositing(const RenderLayer& layer) const
{
    CompositingReasons reasons = CompositingReasonNone;

    if (!layer.isComposited())
        return reasons;

    auto renderer = &layer.renderer();
    if (layer.isReflection())
        renderer = toRenderLayerModelObject(renderer->parent());

    if (requiresCompositingForTransform(*renderer))
        reasons |= CompositingReason3DTransform;

    if (requiresCompositingForVideo(*renderer))
        reasons |= CompositingReasonVideo;
    else if (requiresCompositingForCanvas(*renderer))
        reasons |= CompositingReasonCanvas;
    else if (requiresCompositingForPlugin(*renderer))
        reasons |= CompositingReasonPlugin;
    else if (requiresCompositingForFrame(*renderer))
        reasons |= CompositingReasonIFrame;
    
    if ((canRender3DTransforms() && renderer->style().backfaceVisibility() == BackfaceVisibilityHidden))
        reasons |= CompositingReasonBackfaceVisibilityHidden;

    if (clipsCompositingDescendants(*renderer->layer()))
        reasons |= CompositingReasonClipsCompositingDescendants;

    if (requiresCompositingForAnimation(*renderer))
        reasons |= CompositingReasonAnimation;

    if (requiresCompositingForFilters(*renderer))
        reasons |= CompositingReasonFilters;

    if (requiresCompositingForPosition(*renderer, *renderer->layer()))
        reasons |= renderer->style().position() == FixedPosition ? CompositingReasonPositionFixed : CompositingReasonPositionSticky;

#if PLATFORM(IOS)
    if (requiresCompositingForScrolling(*renderer->layer()))
        reasons |= CompositingReasonOverflowScrollingTouch;
#endif

    if (requiresCompositingForOverflowScrolling(*renderer->layer()))
        reasons |= CompositingReasonOverflowScrollingTouch;

    switch (renderer->layer()->indirectCompositingReason()) {
    case RenderLayer::IndirectCompositingReason::None:
        break;
    case RenderLayer::IndirectCompositingReason::Stacking:
        reasons |= CompositingReasonStacking;
        break;
    case RenderLayer::IndirectCompositingReason::Overlap:
        reasons |= CompositingReasonOverlap;
        break;
    case RenderLayer::IndirectCompositingReason::BackgroundLayer:
        reasons |= CompositingReasonNegativeZIndexChildren;
        break;
    case RenderLayer::IndirectCompositingReason::GraphicalEffect:
        if (renderer->layer()->transform())
            reasons |= CompositingReasonTransformWithCompositedDescendants;

        if (renderer->isTransparent())
            reasons |= CompositingReasonOpacityWithCompositedDescendants;

        if (renderer->hasMask())
            reasons |= CompositingReasonMaskWithCompositedDescendants;

        if (renderer->hasReflection())
            reasons |= CompositingReasonReflectionWithCompositedDescendants;

        if (renderer->hasFilter())
            reasons |= CompositingReasonFilterWithCompositedDescendants;

#if ENABLE(CSS_COMPOSITING)
        if (layer.isolatesCompositedBlending())
            reasons |= CompositingReasonIsolatesCompositedBlendingDescendants;

        if (layer.hasBlendMode())
            reasons |= CompositingReasonBlendingWithCompositedDescendants;
#endif
        break;
    case RenderLayer::IndirectCompositingReason::Perspective:
        reasons |= CompositingReasonPerspective;
        break;
    case RenderLayer::IndirectCompositingReason::Preserve3D:
        reasons |= CompositingReasonPreserve3D;
        break;
    }

    if (inCompositingMode() && renderer->layer()->isRootLayer())
        reasons |= CompositingReasonRoot;

    return reasons;
}

#if !LOG_DISABLED
const char* RenderLayerCompositor::logReasonsForCompositing(const RenderLayer& layer)
{
    CompositingReasons reasons = reasonsForCompositing(layer);

    if (reasons & CompositingReason3DTransform)
        return "3D transform";

    if (reasons & CompositingReasonVideo)
        return "video";
    else if (reasons & CompositingReasonCanvas)
        return "canvas";
    else if (reasons & CompositingReasonPlugin)
        return "plugin";
    else if (reasons & CompositingReasonIFrame)
        return "iframe";
    
    if (reasons & CompositingReasonBackfaceVisibilityHidden)
        return "backface-visibility: hidden";

    if (reasons & CompositingReasonClipsCompositingDescendants)
        return "clips compositing descendants";

    if (reasons & CompositingReasonAnimation)
        return "animation";

    if (reasons & CompositingReasonFilters)
        return "filters";

    if (reasons & CompositingReasonPositionFixed)
        return "position: fixed";

    if (reasons & CompositingReasonPositionSticky)
        return "position: sticky";

    if (reasons & CompositingReasonOverflowScrollingTouch)
        return "-webkit-overflow-scrolling: touch";

    if (reasons & CompositingReasonStacking)
        return "stacking";

    if (reasons & CompositingReasonOverlap)
        return "overlap";

    if (reasons & CompositingReasonNegativeZIndexChildren)
        return "negative z-index children";

    if (reasons & CompositingReasonTransformWithCompositedDescendants)
        return "transform with composited descendants";

    if (reasons & CompositingReasonOpacityWithCompositedDescendants)
        return "opacity with composited descendants";

    if (reasons & CompositingReasonMaskWithCompositedDescendants)
        return "mask with composited descendants";

    if (reasons & CompositingReasonReflectionWithCompositedDescendants)
        return "reflection with composited descendants";

    if (reasons & CompositingReasonFilterWithCompositedDescendants)
        return "filter with composited descendants";

#if ENABLE(CSS_COMPOSITING)
    if (reasons & CompositingReasonBlendingWithCompositedDescendants)
        return "blending with composited descendants";

    if (reasons & CompositingReasonIsolatesCompositedBlendingDescendants)
        return "isolates composited blending descendants";
#endif

    if (reasons & CompositingReasonPerspective)
        return "perspective";

    if (reasons & CompositingReasonPreserve3D)
        return "preserve-3d";

    if (reasons & CompositingReasonRoot)
        return "root";

    return "";
}
#endif

// Return true if the given layer has some ancestor in the RenderLayer hierarchy that clips,
// up to the enclosing compositing ancestor. This is required because compositing layers are parented
// according to the z-order hierarchy, yet clipping goes down the renderer hierarchy.
// Thus, a RenderLayer can be clipped by a RenderLayer that is an ancestor in the renderer hierarchy,
// but a sibling in the z-order hierarchy.
bool RenderLayerCompositor::clippedByAncestor(RenderLayer& layer) const
{
    if (!layer.isComposited() || !layer.parent())
        return false;

    RenderLayer* compositingAncestor = layer.ancestorCompositingLayer();
    if (!compositingAncestor)
        return false;

    // If the compositingAncestor clips, that will be taken care of by clipsCompositingDescendants(),
    // so we only care about clipping between its first child that is our ancestor (the computeClipRoot),
    // and layer. The exception is when the compositingAncestor isolates composited blending children,
    // in this case it is not allowed to clipsCompositingDescendants() and each of its children
    // will be clippedByAncestor()s, including the compositingAncestor.
    RenderLayer* computeClipRoot = compositingAncestor;
    if (!compositingAncestor->isolatesCompositedBlending()) {
        computeClipRoot = nullptr;
        RenderLayer* parent = &layer;
        while (parent) {
            RenderLayer* next = parent->parent();
            if (next == compositingAncestor) {
                computeClipRoot = parent;
                break;
            }
            parent = next;
        }

        if (!computeClipRoot || computeClipRoot == &layer)
            return false;
    }

    return layer.backgroundClipRect(RenderLayer::ClipRectsContext(computeClipRoot, TemporaryClipRects)).rect() != LayoutRect::infiniteRect(); // FIXME: Incorrect for CSS regions.
}

// Return true if the given layer is a stacking context and has compositing child
// layers that it needs to clip. In this case we insert a clipping GraphicsLayer
// into the hierarchy between this layer and its children in the z-order hierarchy.
bool RenderLayerCompositor::clipsCompositingDescendants(const RenderLayer& layer) const
{
    return layer.hasCompositingDescendant() && layer.renderer().hasClipOrOverflowClip() && !layer.isolatesCompositedBlending();
}

bool RenderLayerCompositor::requiresCompositingForScrollableFrame() const
{
    // Need this done first to determine overflow.
    ASSERT(!m_renderView.needsLayout());
    HTMLFrameOwnerElement* ownerElement = m_renderView.document().ownerElement();
    if (!ownerElement)
        return false;

    if (!(m_compositingTriggers & ChromeClient::ScrollableInnerFrameTrigger))
        return false;

    return m_renderView.frameView().isScrollable();
}

bool RenderLayerCompositor::requiresCompositingForTransform(RenderLayerModelObject& renderer) const
{
    if (!(m_compositingTriggers & ChromeClient::ThreeDTransformTrigger))
        return false;

    // Note that we ask the renderer if it has a transform, because the style may have transforms,
    // but the renderer may be an inline that doesn't suppport them.
    return renderer.hasTransform() && renderer.style().transform().has3DOperation();
}

bool RenderLayerCompositor::requiresCompositingForBackfaceVisibility(RenderLayerModelObject& renderer) const
{
    if (!(m_compositingTriggers & ChromeClient::ThreeDTransformTrigger))
        return false;

    return renderer.style().backfaceVisibility() == BackfaceVisibilityHidden && renderer.layer()->has3DTransformedAncestor();
}

bool RenderLayerCompositor::requiresCompositingForVideo(RenderLayerModelObject& renderer) const
{
    if (!(m_compositingTriggers & ChromeClient::VideoTrigger))
        return false;
#if ENABLE(VIDEO)
    if (renderer.isVideo()) {
        RenderVideo& video = toRenderVideo(renderer);
        return (video.requiresImmediateCompositing() || video.shouldDisplayVideo()) && canAccelerateVideoRendering(video);
    }
#else
    UNUSED_PARAM(renderer);
#endif
    return false;
}

bool RenderLayerCompositor::requiresCompositingForCanvas(RenderLayerModelObject& renderer) const
{
    if (!(m_compositingTriggers & ChromeClient::CanvasTrigger))
        return false;

    if (renderer.isCanvas()) {
#if USE(COMPOSITING_FOR_SMALL_CANVASES)
        bool isCanvasLargeEnoughToForceCompositing = true;
#else
        HTMLCanvasElement* canvas = toHTMLCanvasElement(renderer.element());
        bool isCanvasLargeEnoughToForceCompositing = canvas->size().area() >= canvasAreaThresholdRequiringCompositing;
#endif
        CanvasCompositingStrategy compositingStrategy = canvasCompositingStrategy(renderer);
        return compositingStrategy == CanvasAsLayerContents || (compositingStrategy == CanvasPaintedToLayer && isCanvasLargeEnoughToForceCompositing);
    }

    return false;
}

bool RenderLayerCompositor::requiresCompositingForPlugin(RenderLayerModelObject& renderer) const
{
    if (!(m_compositingTriggers & ChromeClient::PluginTrigger))
        return false;

    bool composite = renderer.isEmbeddedObject() && toRenderEmbeddedObject(&renderer)->allowsAcceleratedCompositing();
    if (!composite)
        return false;

    m_reevaluateCompositingAfterLayout = true;
    
    RenderWidget& pluginRenderer = *toRenderWidget(&renderer);
    // If we can't reliably know the size of the plugin yet, don't change compositing state.
    if (pluginRenderer.needsLayout())
        return pluginRenderer.hasLayer() && pluginRenderer.layer()->isComposited();

    // Don't go into compositing mode if height or width are zero, or size is 1x1.
    IntRect contentBox = pixelSnappedIntRect(pluginRenderer.contentBoxRect());
    return contentBox.height() * contentBox.width() > 1;
}

bool RenderLayerCompositor::requiresCompositingForFrame(RenderLayerModelObject& renderer) const
{
    if (!renderer.isWidget())
        return false;

    RenderWidget& frameRenderer = *toRenderWidget(&renderer);

    if (!frameRenderer.requiresAcceleratedCompositing())
        return false;

    m_reevaluateCompositingAfterLayout = true;

    RenderLayerCompositor* innerCompositor = frameContentsCompositor(&frameRenderer);
    if (!innerCompositor || !innerCompositor->shouldPropagateCompositingToEnclosingFrame())
        return false;

    // If we can't reliably know the size of the iframe yet, don't change compositing state.
    if (!frameRenderer.parent() || frameRenderer.needsLayout())
        return frameRenderer.hasLayer() && frameRenderer.layer()->isComposited();
    
    // Don't go into compositing mode if height or width are zero.
    return !pixelSnappedIntRect(frameRenderer.contentBoxRect()).isEmpty();
}

bool RenderLayerCompositor::requiresCompositingForAnimation(RenderLayerModelObject& renderer) const
{
    if (!(m_compositingTriggers & ChromeClient::AnimationTrigger))
        return false;

    const AnimationBase::RunningState activeAnimationState = AnimationBase::Running | AnimationBase::Paused | AnimationBase::FillingFowards;
    AnimationController& animController = renderer.animation();
    return (animController.isRunningAnimationOnRenderer(&renderer, CSSPropertyOpacity, activeAnimationState)
            && (inCompositingMode() || (m_compositingTriggers & ChromeClient::AnimatedOpacityTrigger)))
#if ENABLE(CSS_FILTERS)
            || animController.isRunningAnimationOnRenderer(&renderer, CSSPropertyWebkitFilter, activeAnimationState)
#endif // CSS_FILTERS
            || animController.isRunningAnimationOnRenderer(&renderer, CSSPropertyWebkitTransform, activeAnimationState);
}

bool RenderLayerCompositor::requiresCompositingForIndirectReason(RenderLayerModelObject& renderer, bool hasCompositedDescendants, bool has3DTransformedDescendants, RenderLayer::IndirectCompositingReason& reason) const
{
    RenderLayer& layer = *toRenderBoxModelObject(renderer).layer();

    // When a layer has composited descendants, some effects, like 2d transforms, filters, masks etc must be implemented
    // via compositing so that they also apply to those composited descendants.
    if (hasCompositedDescendants && (layer.isolatesCompositedBlending() || layer.transform() || renderer.createsGroup() || renderer.hasReflection() || renderer.isRenderNamedFlowFragmentContainer())) {
        reason = RenderLayer::IndirectCompositingReason::GraphicalEffect;
        return true;
    }

    // A layer with preserve-3d or perspective only needs to be composited if there are descendant layers that
    // will be affected by the preserve-3d or perspective.
    if (has3DTransformedDescendants) {
        if (renderer.style().transformStyle3D() == TransformStyle3DPreserve3D) {
            reason = RenderLayer::IndirectCompositingReason::Preserve3D;
            return true;
        }
    
        if (renderer.style().hasPerspective()) {
            reason = RenderLayer::IndirectCompositingReason::Perspective;
            return true;
        }
    }

    reason = RenderLayer::IndirectCompositingReason::None;
    return false;
}

bool RenderLayerCompositor::requiresCompositingForFilters(RenderLayerModelObject& renderer) const
{
#if ENABLE(CSS_FILTERS)
    if (!(m_compositingTriggers & ChromeClient::FilterTrigger))
        return false;

    return renderer.hasFilter();
#else
    UNUSED_PARAM(renderer);
    return false;
#endif
}

bool RenderLayerCompositor::isAsyncScrollableStickyLayer(const RenderLayer& layer, const RenderLayer** enclosingAcceleratedOverflowLayer) const
{
    ASSERT(layer.renderer().isStickyPositioned());

    RenderLayer* enclosingOverflowLayer = layer.enclosingOverflowClipLayer(ExcludeSelf);

#if PLATFORM(IOS)
    if (enclosingOverflowLayer && enclosingOverflowLayer->hasTouchScrollableOverflow()) {
        if (enclosingAcceleratedOverflowLayer)
            *enclosingAcceleratedOverflowLayer = enclosingOverflowLayer;
        return true;
    }
#else
    UNUSED_PARAM(enclosingAcceleratedOverflowLayer);
#endif
    // If the layer is inside normal overflow, it's not async-scrollable.
    if (enclosingOverflowLayer)
        return false;

    // No overflow ancestor, so see if the frame supports async scrolling.
    if (hasCoordinatedScrolling())
        return true;

#if PLATFORM(IOS)
    // iOS WK1 has fixed/sticky support in the main frame via WebFixedPositionContent.
    return m_renderView.frameView().frame().isMainFrame();
#else
    return false;
#endif
}

bool RenderLayerCompositor::isViewportConstrainedFixedOrStickyLayer(const RenderLayer& layer) const
{
    if (layer.renderer().isStickyPositioned())
        return isAsyncScrollableStickyLayer(layer);

    if (layer.renderer().style().position() != FixedPosition)
        return false;

    // FIXME: Handle fixed inside of a transform, which should not behave as fixed.
    for (RenderLayer* stackingContainer = layer.stackingContainer(); stackingContainer; stackingContainer = stackingContainer->stackingContainer()) {
        if (stackingContainer->isComposited() && stackingContainer->renderer().style().position() == FixedPosition)
            return false;
    }

    return true;
}

static bool useCoordinatedScrollingForLayer(RenderView& view, const RenderLayer& layer)
{
    if (layer.isRootLayer() && view.frameView().frame().isMainFrame())
        return true;

#if PLATFORM(IOS)
    return layer.hasTouchScrollableOverflow();
#else
    return layer.needsCompositedScrolling();
#endif
}

bool RenderLayerCompositor::requiresCompositingForPosition(RenderLayerModelObject& renderer, const RenderLayer& layer, RenderLayer::ViewportConstrainedNotCompositedReason* viewportConstrainedNotCompositedReason) const
{
    // position:fixed elements that create their own stacking context (e.g. have an explicit z-index,
    // opacity, transform) can get their own composited layer. A stacking context is required otherwise
    // z-index and clipping will be broken.
    if (!renderer.isPositioned())
        return false;
    
    EPosition position = renderer.style().position();
    bool isFixed = renderer.isOutOfFlowPositioned() && position == FixedPosition;
    if (isFixed && !layer.isStackingContainer())
        return false;
    
    bool isSticky = renderer.isInFlowPositioned() && position == StickyPosition;
    if (!isFixed && !isSticky)
        return false;

    // FIXME: acceleratedCompositingForFixedPositionEnabled should probably be renamed acceleratedCompositingForViewportConstrainedPositionEnabled().
    const Settings& settings = m_renderView.frameView().frame().settings();
    if (!settings.acceleratedCompositingForFixedPositionEnabled())
        return false;

    if (isSticky)
        return isAsyncScrollableStickyLayer(layer);

    auto container = renderer.container();
    // If the renderer is not hooked up yet then we have to wait until it is.
    if (!container) {
        m_reevaluateCompositingAfterLayout = true;
        return false;
    }

    // Don't promote fixed position elements that are descendants of a non-view container, e.g. transformed elements.
    // They will stay fixed wrt the container rather than the enclosing frame.
    if (container != &m_renderView && !renderer.fixedPositionedWithNamedFlowContainingBlock()) {
        if (viewportConstrainedNotCompositedReason)
            *viewportConstrainedNotCompositedReason = RenderLayer::NotCompositedForNonViewContainer;
        return false;
    }
    
    // Subsequent tests depend on layout. If we can't tell now, just keep things the way they are until layout is done.
    if (!m_inPostLayoutUpdate) {
        m_reevaluateCompositingAfterLayout = true;
        return layer.isComposited();
    }

    bool paintsContent = layer.isVisuallyNonEmpty() || layer.hasVisibleDescendant();
    if (!paintsContent) {
        if (viewportConstrainedNotCompositedReason)
            *viewportConstrainedNotCompositedReason = RenderLayer::NotCompositedForNoVisibleContent;
        return false;
    }

    // Fixed position elements that are invisible in the current view don't get their own layer.
    LayoutRect viewBounds = m_renderView.frameView().viewportConstrainedVisibleContentRect();
    LayoutRect layerBounds = layer.calculateLayerBounds(&layer, LayoutSize(), RenderLayer::UseLocalClipRectIfPossible | RenderLayer::IncludeLayerFilterOutsets | RenderLayer::UseFragmentBoxesExcludingCompositing
        | RenderLayer::ExcludeHiddenDescendants | RenderLayer::DontConstrainForMask | RenderLayer::IncludeCompositedDescendants);
    // Map to m_renderView to ignore page scale.
    FloatRect absoluteBounds = layer.renderer().localToContainerQuad(FloatRect(layerBounds), &m_renderView).boundingBox();
    if (!viewBounds.intersects(enclosingIntRect(absoluteBounds))) {
        if (viewportConstrainedNotCompositedReason)
            *viewportConstrainedNotCompositedReason = RenderLayer::NotCompositedForBoundsOutOfView;
        return false;
    }
    
    return true;
}

bool RenderLayerCompositor::requiresCompositingForOverflowScrolling(const RenderLayer& layer) const
{
    return layer.needsCompositedScrolling();
}

#if PLATFORM(IOS)
bool RenderLayerCompositor::requiresCompositingForScrolling(const RenderLayer& layer) const
{
    if (!layer.hasAcceleratedTouchScrolling())
        return false;

    if (!m_inPostLayoutUpdate) {
        m_reevaluateCompositingAfterLayout = true;
        return layer.isComposited();
    }

    return layer.hasTouchScrollableOverflow();
}
#endif

bool RenderLayerCompositor::isRunningAcceleratedTransformAnimation(RenderLayerModelObject& renderer) const
{
    if (!(m_compositingTriggers & ChromeClient::AnimationTrigger))
        return false;

    return renderer.animation().isRunningAcceleratedAnimationOnRenderer(&renderer, CSSPropertyWebkitTransform, AnimationBase::Running | AnimationBase::Paused);
}

// If an element has negative z-index children, those children render in front of the 
// layer background, so we need an extra 'contents' layer for the foreground of the layer
// object.
bool RenderLayerCompositor::needsContentsCompositingLayer(const RenderLayer& layer) const
{
    return layer.hasNegativeZOrderList();
}

bool RenderLayerCompositor::requiresScrollLayer(RootLayerAttachment attachment) const
{
    FrameView& frameView = m_renderView.frameView();

    // This applies when the application UI handles scrolling, in which case RenderLayerCompositor doesn't need to manage it.
    if (frameView.delegatesScrolling() && frameView.frame().isMainFrame())
        return false;

    // We need to handle our own scrolling if we're:
    return !m_renderView.frameView().platformWidget() // viewless (i.e. non-Mac, or Mac in WebKit2)
        || attachment == RootLayerAttachedViaEnclosingFrame; // a composited frame on Mac
}

static void paintScrollbar(Scrollbar* scrollbar, GraphicsContext& context, const IntRect& clip)
{
    if (!scrollbar)
        return;

    context.save();
    const IntRect& scrollbarRect = scrollbar->frameRect();
    context.translate(-scrollbarRect.x(), -scrollbarRect.y());
    IntRect transformedClip = clip;
    transformedClip.moveBy(scrollbarRect.location());
    scrollbar->paint(&context, transformedClip);
    context.restore();
}

void RenderLayerCompositor::paintContents(const GraphicsLayer* graphicsLayer, GraphicsContext& context, GraphicsLayerPaintingPhase, const FloatRect& clip)
{
    IntRect pixelSnappedRectForIntegralPositionedItems = pixelSnappedIntRect(LayoutRect(clip));
    if (graphicsLayer == layerForHorizontalScrollbar())
        paintScrollbar(m_renderView.frameView().horizontalScrollbar(), context, pixelSnappedRectForIntegralPositionedItems);
    else if (graphicsLayer == layerForVerticalScrollbar())
        paintScrollbar(m_renderView.frameView().verticalScrollbar(), context, pixelSnappedRectForIntegralPositionedItems);
    else if (graphicsLayer == layerForScrollCorner()) {
        const IntRect& scrollCorner = m_renderView.frameView().scrollCornerRect();
        context.save();
        context.translate(-scrollCorner.x(), -scrollCorner.y());
        IntRect transformedClip = pixelSnappedRectForIntegralPositionedItems;
        transformedClip.moveBy(scrollCorner.location());
        m_renderView.frameView().paintScrollCorner(&context, transformedClip);
        context.restore();
    }
}

bool RenderLayerCompositor::supportsFixedRootBackgroundCompositing() const
{
    RenderLayerBacking* renderViewBacking = m_renderView.layer()->backing();
    return renderViewBacking && renderViewBacking->usingTiledBacking();
}

bool RenderLayerCompositor::needsFixedRootBackgroundLayer(const RenderLayer& layer) const
{
    if (&layer != m_renderView.layer())
        return false;

    if (m_renderView.frameView().frame().settings().fixedBackgroundsPaintRelativeToDocument())
        return false;

    return supportsFixedRootBackgroundCompositing() && m_renderView.rootBackgroundIsEntirelyFixed();
}

GraphicsLayer* RenderLayerCompositor::fixedRootBackgroundLayer() const
{
    // Get the fixed root background from the RenderView layer's backing.
    RenderLayer* viewLayer = m_renderView.layer();
    if (!viewLayer)
        return nullptr;

    if (viewLayer->isComposited() && viewLayer->backing()->backgroundLayerPaintsFixedRootBackground())
        return viewLayer->backing()->backgroundLayer();

    return nullptr;
}

static void resetTrackedRepaintRectsRecursive(GraphicsLayer& graphicsLayer)
{
    graphicsLayer.resetTrackedRepaints();

    for (size_t i = 0, size = graphicsLayer.children().size(); i < size; ++i)
        resetTrackedRepaintRectsRecursive(*graphicsLayer.children()[i]);

    if (GraphicsLayer* replicaLayer = graphicsLayer.replicaLayer())
        resetTrackedRepaintRectsRecursive(*replicaLayer);

    if (GraphicsLayer* maskLayer = graphicsLayer.maskLayer())
        resetTrackedRepaintRectsRecursive(*maskLayer);
}

void RenderLayerCompositor::resetTrackedRepaintRects()
{
    if (GraphicsLayer* rootLayer = rootGraphicsLayer())
        resetTrackedRepaintRectsRecursive(*rootLayer);
}

void RenderLayerCompositor::setTracksRepaints(bool tracksRepaints)
{
    m_isTrackingRepaints = tracksRepaints;
}

bool RenderLayerCompositor::isTrackingRepaints() const
{
    return m_isTrackingRepaints;
}

float RenderLayerCompositor::deviceScaleFactor() const
{
    return m_renderView.document().deviceScaleFactor();
}

float RenderLayerCompositor::pageScaleFactor() const
{
    Page* page = this->page();
    return page ? page->pageScaleFactor() : 1;
}

float RenderLayerCompositor::zoomedOutPageScaleFactor() const
{
    Page* page = this->page();
    return page ? page->zoomedOutPageScaleFactor() : 0;
}

float RenderLayerCompositor::contentsScaleMultiplierForNewTiles(const GraphicsLayer*) const
{
#if PLATFORM(IOS)
    LegacyTileCache* tileCache = nullptr;
    if (Page* page = this->page()) {
        if (FrameView* frameView = page->mainFrame().view())
            tileCache = frameView->legacyTileCache();
    }

    if (!tileCache)
        return 1;

    return tileCache->tileControllerShouldUseLowScaleTiles() ? 0.125 : 1;
#else
    return 1;
#endif
}

void RenderLayerCompositor::didCommitChangesForLayer(const GraphicsLayer*) const
{
    // Nothing to do here yet.
}

bool RenderLayerCompositor::mainFrameBackingIsTiled() const
{
    RenderLayer* layer = m_renderView.layer();
    if (!layer)
        return false;

    RenderLayerBacking* backing = layer->backing();
    if (!backing)
        return false;

    return backing->usingTiledBacking();
}

bool RenderLayerCompositor::shouldCompositeOverflowControls() const
{
    FrameView& frameView = m_renderView.frameView();

    if (frameView.platformWidget())
        return false;

    if (frameView.delegatesScrolling())
        return false;

    if (mainFrameBackingIsTiled())
        return true;

    if (!frameView.hasOverlayScrollbars())
        return false;

    return true;
}

bool RenderLayerCompositor::requiresHorizontalScrollbarLayer() const
{
    return shouldCompositeOverflowControls() && m_renderView.frameView().horizontalScrollbar();
}

bool RenderLayerCompositor::requiresVerticalScrollbarLayer() const
{
    return shouldCompositeOverflowControls() && m_renderView.frameView().verticalScrollbar();
}

bool RenderLayerCompositor::requiresScrollCornerLayer() const
{
    return shouldCompositeOverflowControls() && m_renderView.frameView().isScrollCornerVisible();
}

#if ENABLE(RUBBER_BANDING)
bool RenderLayerCompositor::requiresOverhangAreasLayer() const
{
    // We don't want a layer if this is a subframe.
    if (m_renderView.document().ownerElement())
        return false;

    // We do want a layer if we're using tiled drawing and can scroll.
    if (mainFrameBackingIsTiled() && m_renderView.frameView().hasOpaqueBackground() && !m_renderView.frameView().prohibitsScrolling())
        return true;

    return false;
}

bool RenderLayerCompositor::requiresContentShadowLayer() const
{
    // We don't want a layer if this is a subframe.
    if (m_renderView.document().ownerElement())
        return false;

#if PLATFORM(COCOA)
    if (viewHasTransparentBackground())
        return false;

    // If the background is going to extend, then it doesn't make sense to have a shadow layer.
    if (m_renderView.frameView().frame().settings().backgroundShouldExtendBeyondPage())
        return false;

    // On Mac, we want a content shadow layer if we're using tiled drawing and can scroll.
    if (mainFrameBackingIsTiled() && !m_renderView.frameView().prohibitsScrolling())
        return true;
#endif

    return false;
}

GraphicsLayer* RenderLayerCompositor::updateLayerForTopOverhangArea(bool wantsLayer)
{
    if (m_renderView.document().ownerElement())
        return 0;

    if (!wantsLayer) {
        if (m_layerForTopOverhangArea) {
            m_layerForTopOverhangArea->removeFromParent();
            m_layerForTopOverhangArea = nullptr;
        }
        return 0;
    }

    if (!m_layerForTopOverhangArea) {
        m_layerForTopOverhangArea = GraphicsLayer::create(graphicsLayerFactory(), *this);
#ifndef NDEBUG
        m_layerForTopOverhangArea->setName("top overhang area");
#endif
        m_scrollLayer->addChildBelow(m_layerForTopOverhangArea.get(), m_rootContentLayer.get());
    }

    return m_layerForTopOverhangArea.get();
}

GraphicsLayer* RenderLayerCompositor::updateLayerForBottomOverhangArea(bool wantsLayer)
{
    if (m_renderView.document().ownerElement())
        return 0;

    if (!wantsLayer) {
        if (m_layerForBottomOverhangArea) {
            m_layerForBottomOverhangArea->removeFromParent();
            m_layerForBottomOverhangArea = nullptr;
        }
        return 0;
    }

    if (!m_layerForBottomOverhangArea) {
        m_layerForBottomOverhangArea = GraphicsLayer::create(graphicsLayerFactory(), *this);
#ifndef NDEBUG
        m_layerForBottomOverhangArea->setName("bottom overhang area");
#endif
        m_scrollLayer->addChildBelow(m_layerForBottomOverhangArea.get(), m_rootContentLayer.get());
    }

    m_layerForBottomOverhangArea->setPosition(FloatPoint(0, m_rootContentLayer->size().height() + m_renderView.frameView().headerHeight()
        + m_renderView.frameView().footerHeight() + m_renderView.frameView().topContentInset()));
    return m_layerForBottomOverhangArea.get();
}

GraphicsLayer* RenderLayerCompositor::updateLayerForHeader(bool wantsLayer)
{
    if (m_renderView.document().ownerElement())
        return 0;

    if (!wantsLayer) {
        if (m_layerForHeader) {
            m_layerForHeader->removeFromParent();
            m_layerForHeader = nullptr;

            // The ScrollingTree knows about the header layer, and the position of the root layer is affected
            // by the header layer, so if we remove the header, we need to tell the scrolling tree.
            if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
                scrollingCoordinator->frameViewRootLayerDidChange(&m_renderView.frameView());
        }
        return 0;
    }

    if (!m_layerForHeader) {
        m_layerForHeader = GraphicsLayer::create(graphicsLayerFactory(), *this);
#ifndef NDEBUG
        m_layerForHeader->setName("header");
#endif
        m_scrollLayer->addChildAbove(m_layerForHeader.get(), m_rootContentLayer.get());
        m_renderView.frameView().addPaintPendingMilestones(DidFirstFlushForHeaderLayer);
    }

    m_layerForHeader->setPosition(FloatPoint(0,
        FrameView::yPositionForHeaderLayer(m_renderView.frameView().scrollPosition(), m_renderView.frameView().topContentInset())));
    m_layerForHeader->setAnchorPoint(FloatPoint3D());
    m_layerForHeader->setSize(FloatSize(m_renderView.frameView().visibleWidth(), m_renderView.frameView().headerHeight()));

    if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
        scrollingCoordinator->frameViewRootLayerDidChange(&m_renderView.frameView());

    if (Page* page = this->page())
        page->chrome().client().didAddHeaderLayer(m_layerForHeader.get());

    return m_layerForHeader.get();
}

GraphicsLayer* RenderLayerCompositor::updateLayerForFooter(bool wantsLayer)
{
    if (m_renderView.document().ownerElement())
        return 0;

    if (!wantsLayer) {
        if (m_layerForFooter) {
            m_layerForFooter->removeFromParent();
            m_layerForFooter = nullptr;

            // The ScrollingTree knows about the footer layer, and the total scrollable size is affected
            // by the footer layer, so if we remove the footer, we need to tell the scrolling tree.
            if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
                scrollingCoordinator->frameViewRootLayerDidChange(&m_renderView.frameView());
        }
        return 0;
    }

    if (!m_layerForFooter) {
        m_layerForFooter = GraphicsLayer::create(graphicsLayerFactory(), *this);
#ifndef NDEBUG
        m_layerForFooter->setName("footer");
#endif
        m_scrollLayer->addChildAbove(m_layerForFooter.get(), m_rootContentLayer.get());
    }

    float totalContentHeight = m_rootContentLayer->size().height() + m_renderView.frameView().headerHeight() + m_renderView.frameView().footerHeight();
    m_layerForFooter->setPosition(FloatPoint(0, FrameView::yPositionForFooterLayer(m_renderView.frameView().scrollPosition(),
        m_renderView.frameView().topContentInset(), totalContentHeight, m_renderView.frameView().footerHeight())));
    m_layerForFooter->setAnchorPoint(FloatPoint3D());
    m_layerForFooter->setSize(FloatSize(m_renderView.frameView().visibleWidth(), m_renderView.frameView().footerHeight()));

    if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
        scrollingCoordinator->frameViewRootLayerDidChange(&m_renderView.frameView());

    if (Page* page = this->page())
        page->chrome().client().didAddFooterLayer(m_layerForFooter.get());

    return m_layerForFooter.get();
}

#endif

bool RenderLayerCompositor::viewHasTransparentBackground(Color* backgroundColor) const
{
    if (m_renderView.frameView().isTransparent()) {
        if (backgroundColor)
            *backgroundColor = Color(); // Return an invalid color.
        return true;
    }

    Color documentBackgroundColor = m_renderView.frameView().documentBackgroundColor();
    if (!documentBackgroundColor.isValid())
        documentBackgroundColor = Color::white;

    if (backgroundColor)
        *backgroundColor = documentBackgroundColor;
        
    return documentBackgroundColor.hasAlpha();
}

void RenderLayerCompositor::setRootExtendedBackgroundColor(const Color& color)
{
    if (color == m_rootExtendedBackgroundColor)
        return;

    m_rootExtendedBackgroundColor = color;

    if (Page* page = this->page())
        page->chrome().client().pageExtendedBackgroundColorDidChange(color);

#if ENABLE(RUBBER_BANDING)
    if (!m_layerForOverhangAreas)
        return;

    m_layerForOverhangAreas->setBackgroundColor(m_rootExtendedBackgroundColor);

    if (!m_rootExtendedBackgroundColor.isValid())
        m_layerForOverhangAreas->setCustomAppearance(GraphicsLayer::ScrollingOverhang);
#endif
}

void RenderLayerCompositor::updateOverflowControlsLayers()
{
#if ENABLE(RUBBER_BANDING)
    if (requiresOverhangAreasLayer()) {
        if (!m_layerForOverhangAreas) {
            m_layerForOverhangAreas = GraphicsLayer::create(graphicsLayerFactory(), *this);
#ifndef NDEBUG
            m_layerForOverhangAreas->setName("overhang areas");
#endif
            m_layerForOverhangAreas->setDrawsContent(false);

            float topContentInset = m_renderView.frameView().topContentInset();
            IntSize overhangAreaSize = m_renderView.frameView().frameRect().size();
            overhangAreaSize.setHeight(overhangAreaSize.height() - topContentInset);
            m_layerForOverhangAreas->setSize(overhangAreaSize);
            m_layerForOverhangAreas->setPosition(FloatPoint(0, topContentInset));
            m_layerForOverhangAreas->setAnchorPoint(FloatPoint3D());

            if (m_renderView.frameView().frame().settings().backgroundShouldExtendBeyondPage())
                m_layerForOverhangAreas->setBackgroundColor(m_renderView.frameView().documentBackgroundColor());
            else
                m_layerForOverhangAreas->setCustomAppearance(GraphicsLayer::ScrollingOverhang);

            // We want the overhang areas layer to be positioned below the frame contents,
            // so insert it below the clip layer.
            m_overflowControlsHostLayer->addChildBelow(m_layerForOverhangAreas.get(), m_clipLayer.get());
        }
    } else if (m_layerForOverhangAreas) {
        m_layerForOverhangAreas->removeFromParent();
        m_layerForOverhangAreas = nullptr;
    }

    if (requiresContentShadowLayer()) {
        if (!m_contentShadowLayer) {
            m_contentShadowLayer = GraphicsLayer::create(graphicsLayerFactory(), *this);
#ifndef NDEBUG
            m_contentShadowLayer->setName("content shadow");
#endif
            m_contentShadowLayer->setSize(m_rootContentLayer->size());
            m_contentShadowLayer->setPosition(m_rootContentLayer->position());
            m_contentShadowLayer->setAnchorPoint(FloatPoint3D());
            m_contentShadowLayer->setCustomAppearance(GraphicsLayer::ScrollingShadow);

            m_scrollLayer->addChildBelow(m_contentShadowLayer.get(), m_rootContentLayer.get());
        }
    } else if (m_contentShadowLayer) {
        m_contentShadowLayer->removeFromParent();
        m_contentShadowLayer = nullptr;
    }
#endif

    if (requiresHorizontalScrollbarLayer()) {
        if (!m_layerForHorizontalScrollbar) {
            m_layerForHorizontalScrollbar = GraphicsLayer::create(graphicsLayerFactory(), *this);
            m_layerForHorizontalScrollbar->setShowDebugBorder(m_showDebugBorders);
#ifndef NDEBUG
            m_layerForHorizontalScrollbar->setName("horizontal scrollbar container");

#endif
#if PLATFORM(COCOA) && USE(CA)
            m_layerForHorizontalScrollbar->setAcceleratesDrawing(acceleratedDrawingEnabled());
#endif
            m_overflowControlsHostLayer->addChild(m_layerForHorizontalScrollbar.get());

            if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
                scrollingCoordinator->scrollableAreaScrollbarLayerDidChange(&m_renderView.frameView(), HorizontalScrollbar);
        }
    } else if (m_layerForHorizontalScrollbar) {
        m_layerForHorizontalScrollbar->removeFromParent();
        m_layerForHorizontalScrollbar = nullptr;

        if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
            scrollingCoordinator->scrollableAreaScrollbarLayerDidChange(&m_renderView.frameView(), HorizontalScrollbar);
    }

    if (requiresVerticalScrollbarLayer()) {
        if (!m_layerForVerticalScrollbar) {
            m_layerForVerticalScrollbar = GraphicsLayer::create(graphicsLayerFactory(), *this);
            m_layerForVerticalScrollbar->setShowDebugBorder(m_showDebugBorders);
#ifndef NDEBUG
            m_layerForVerticalScrollbar->setName("vertical scrollbar container");
#endif
#if PLATFORM(COCOA) && USE(CA)
            m_layerForVerticalScrollbar->setAcceleratesDrawing(acceleratedDrawingEnabled());
#endif
            m_overflowControlsHostLayer->addChild(m_layerForVerticalScrollbar.get());

            if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
                scrollingCoordinator->scrollableAreaScrollbarLayerDidChange(&m_renderView.frameView(), VerticalScrollbar);
        }
    } else if (m_layerForVerticalScrollbar) {
        m_layerForVerticalScrollbar->removeFromParent();
        m_layerForVerticalScrollbar = nullptr;

        if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
            scrollingCoordinator->scrollableAreaScrollbarLayerDidChange(&m_renderView.frameView(), VerticalScrollbar);
    }

    if (requiresScrollCornerLayer()) {
        if (!m_layerForScrollCorner) {
            m_layerForScrollCorner = GraphicsLayer::create(graphicsLayerFactory(), *this);
            m_layerForScrollCorner->setShowDebugBorder(m_showDebugBorders);
#ifndef NDEBUG
            m_layerForScrollCorner->setName("scroll corner");
#endif
#if PLATFORM(COCOA) && USE(CA)
            m_layerForScrollCorner->setAcceleratesDrawing(acceleratedDrawingEnabled());
#endif
            m_overflowControlsHostLayer->addChild(m_layerForScrollCorner.get());
        }
    } else if (m_layerForScrollCorner) {
        m_layerForScrollCorner->removeFromParent();
        m_layerForScrollCorner = nullptr;
    }

    m_renderView.frameView().positionScrollbarLayers();
}

void RenderLayerCompositor::ensureRootLayer()
{
    RootLayerAttachment expectedAttachment = shouldPropagateCompositingToEnclosingFrame() ? RootLayerAttachedViaEnclosingFrame : RootLayerAttachedViaChromeClient;
    if (expectedAttachment == m_rootLayerAttachment)
         return;

    if (!m_rootContentLayer) {
        m_rootContentLayer = GraphicsLayer::create(graphicsLayerFactory(), *this);
#ifndef NDEBUG
        m_rootContentLayer->setName("content root");
#endif
        IntRect overflowRect = m_renderView.pixelSnappedLayoutOverflowRect();
        m_rootContentLayer->setSize(FloatSize(overflowRect.maxX(), overflowRect.maxY()));
        m_rootContentLayer->setPosition(FloatPoint());

#if PLATFORM(IOS) || PLATFORM(EFL)
        // Page scale is applied above this on iOS, so we'll just say that our root layer applies it.
        Frame& frame = m_renderView.frameView().frame();
        if (frame.isMainFrame())
            m_rootContentLayer->setAppliesPageScale();
#endif

        // Need to clip to prevent transformed content showing outside this frame
        m_rootContentLayer->setMasksToBounds(true);
    }

    if (requiresScrollLayer(expectedAttachment)) {
        if (!m_overflowControlsHostLayer) {
            ASSERT(!m_scrollLayer);
            ASSERT(!m_clipLayer);

            // Create a layer to host the clipping layer and the overflow controls layers.
            m_overflowControlsHostLayer = GraphicsLayer::create(graphicsLayerFactory(), *this);
#ifndef NDEBUG
            m_overflowControlsHostLayer->setName("overflow controls host");
#endif

            // Create a clipping layer if this is an iframe
            m_clipLayer = GraphicsLayer::create(graphicsLayerFactory(), *this);
#ifndef NDEBUG
            m_clipLayer->setName("frame clipping");
#endif
            m_clipLayer->setMasksToBounds(true);
            
            m_scrollLayer = GraphicsLayer::create(graphicsLayerFactory(), *this);
#ifndef NDEBUG
            m_scrollLayer->setName("frame scrolling");
#endif
            // Hook them up
            m_overflowControlsHostLayer->addChild(m_clipLayer.get());
            m_clipLayer->addChild(m_scrollLayer.get());
            m_scrollLayer->addChild(m_rootContentLayer.get());

            m_clipLayer->setSize(m_renderView.frameView().unscaledVisibleContentSizeIncludingObscuredArea());
            m_clipLayer->setPosition(positionForClipLayer());
            m_clipLayer->setAnchorPoint(FloatPoint3D());

            updateOverflowControlsLayers();

            if (hasCoordinatedScrolling())
                scheduleLayerFlush(true);
            else
                updateScrollLayerPosition();
        }
    } else {
        if (m_overflowControlsHostLayer) {
            m_overflowControlsHostLayer = nullptr;
            m_clipLayer = nullptr;
            m_scrollLayer = nullptr;
        }
    }

    // Check to see if we have to change the attachment
    if (m_rootLayerAttachment != RootLayerUnattached)
        detachRootLayer();

    attachRootLayer(expectedAttachment);
}

void RenderLayerCompositor::destroyRootLayer()
{
    if (!m_rootContentLayer)
        return;

    detachRootLayer();

#if ENABLE(RUBBER_BANDING)
    if (m_layerForOverhangAreas) {
        m_layerForOverhangAreas->removeFromParent();
        m_layerForOverhangAreas = nullptr;
    }
#endif

    if (m_layerForHorizontalScrollbar) {
        m_layerForHorizontalScrollbar->removeFromParent();
        m_layerForHorizontalScrollbar = nullptr;
        if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
            scrollingCoordinator->scrollableAreaScrollbarLayerDidChange(&m_renderView.frameView(), HorizontalScrollbar);
        if (Scrollbar* horizontalScrollbar = m_renderView.frameView().verticalScrollbar())
            m_renderView.frameView().invalidateScrollbar(horizontalScrollbar, IntRect(IntPoint(0, 0), horizontalScrollbar->frameRect().size()));
    }

    if (m_layerForVerticalScrollbar) {
        m_layerForVerticalScrollbar->removeFromParent();
        m_layerForVerticalScrollbar = nullptr;
        if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
            scrollingCoordinator->scrollableAreaScrollbarLayerDidChange(&m_renderView.frameView(), VerticalScrollbar);
        if (Scrollbar* verticalScrollbar = m_renderView.frameView().verticalScrollbar())
            m_renderView.frameView().invalidateScrollbar(verticalScrollbar, IntRect(IntPoint(0, 0), verticalScrollbar->frameRect().size()));
    }

    if (m_layerForScrollCorner) {
        m_layerForScrollCorner = nullptr;
        m_renderView.frameView().invalidateScrollCorner(m_renderView.frameView().scrollCornerRect());
    }

    if (m_overflowControlsHostLayer) {
        m_overflowControlsHostLayer = nullptr;
        m_clipLayer = nullptr;
        m_scrollLayer = nullptr;
    }
    ASSERT(!m_scrollLayer);
    m_rootContentLayer = nullptr;

    m_layerUpdater = nullptr;
}

void RenderLayerCompositor::attachRootLayer(RootLayerAttachment attachment)
{
    if (!m_rootContentLayer)
        return;

    switch (attachment) {
        case RootLayerUnattached:
            ASSERT_NOT_REACHED();
            break;
        case RootLayerAttachedViaChromeClient: {
            Frame& frame = m_renderView.frameView().frame();
            Page* page = frame.page();
            if (!page)
                return;

            page->chrome().client().attachRootGraphicsLayer(&frame, rootGraphicsLayer());
            break;
        }
        case RootLayerAttachedViaEnclosingFrame: {
            // The layer will get hooked up via RenderLayerBacking::updateConfiguration()
            // for the frame's renderer in the parent document.
            m_renderView.document().ownerElement()->scheduleSetNeedsStyleRecalc(SyntheticStyleChange);
            break;
        }
    }

    m_rootLayerAttachment = attachment;
    rootLayerAttachmentChanged();
    
    if (m_shouldFlushOnReattach) {
        scheduleLayerFlushNow();
        m_shouldFlushOnReattach = false;
    }
}

void RenderLayerCompositor::detachRootLayer()
{
    if (!m_rootContentLayer || m_rootLayerAttachment == RootLayerUnattached)
        return;

    switch (m_rootLayerAttachment) {
    case RootLayerAttachedViaEnclosingFrame: {
        // The layer will get unhooked up via RenderLayerBacking::updateGraphicsLayerConfiguration()
        // for the frame's renderer in the parent document.
        if (m_overflowControlsHostLayer)
            m_overflowControlsHostLayer->removeFromParent();
        else
            m_rootContentLayer->removeFromParent();

        if (HTMLFrameOwnerElement* ownerElement = m_renderView.document().ownerElement())
            ownerElement->scheduleSetNeedsStyleRecalc(SyntheticStyleChange);
        break;
    }
    case RootLayerAttachedViaChromeClient: {
        Frame& frame = m_renderView.frameView().frame();
        Page* page = frame.page();
        if (!page)
            return;

        page->chrome().client().attachRootGraphicsLayer(&frame, 0);
    }
    break;
    case RootLayerUnattached:
        break;
    }

    m_rootLayerAttachment = RootLayerUnattached;
    rootLayerAttachmentChanged();
}

void RenderLayerCompositor::updateRootLayerAttachment()
{
    ensureRootLayer();
}

void RenderLayerCompositor::rootLayerAttachmentChanged()
{
    // The attachment can affect whether the RenderView layer's paintsIntoWindow() behavior,
    // so call updateDrawsContent() to update that.
    RenderLayer* layer = m_renderView.layer();
    if (RenderLayerBacking* backing = layer ? layer->backing() : 0)
        backing->updateDrawsContent();
}

// IFrames are special, because we hook compositing layers together across iframe boundaries
// when both parent and iframe content are composited. So when this frame becomes composited, we have
// to use a synthetic style change to get the iframes into RenderLayers in order to allow them to composite.
void RenderLayerCompositor::notifyIFramesOfCompositingChange()
{
    Frame& frame = m_renderView.frameView().frame();
    for (Frame* child = frame.tree().firstChild(); child; child = child->tree().traverseNext(&frame)) {
        if (child->document() && child->document()->ownerElement())
            child->document()->ownerElement()->scheduleSetNeedsStyleRecalc(SyntheticStyleChange);
    }
    
    // Compositing also affects the answer to RenderIFrame::requiresAcceleratedCompositing(), so 
    // we need to schedule a style recalc in our parent document.
    if (HTMLFrameOwnerElement* ownerElement = m_renderView.document().ownerElement())
        ownerElement->scheduleSetNeedsStyleRecalc(SyntheticStyleChange);
}

bool RenderLayerCompositor::layerHas3DContent(const RenderLayer& layer) const
{
    const RenderStyle& style = layer.renderer().style();

    if (style.transformStyle3D() == TransformStyle3DPreserve3D || style.hasPerspective() || style.transform().has3DOperation())
        return true;

    const_cast<RenderLayer&>(layer).updateLayerListsIfNeeded();

#if !ASSERT_DISABLED
    LayerListMutationDetector mutationChecker(const_cast<RenderLayer*>(&layer));
#endif

    if (layer.isStackingContainer()) {
        if (Vector<RenderLayer*>* negZOrderList = layer.negZOrderList()) {
            for (size_t i = 0, size = negZOrderList->size(); i < size; ++i) {
                if (layerHas3DContent(*negZOrderList->at(i)))
                    return true;
            }
        }

        if (Vector<RenderLayer*>* posZOrderList = layer.posZOrderList()) {
            for (size_t i = 0, size = posZOrderList->size(); i < size; ++i) {
                if (layerHas3DContent(*posZOrderList->at(i)))
                    return true;
            }
        }
    }

    if (Vector<RenderLayer*>* normalFlowList = layer.normalFlowList()) {
        for (size_t i = 0, size = normalFlowList->size(); i < size; ++i) {
            if (layerHas3DContent(*normalFlowList->at(i)))
                return true;
        }
    }

    return false;
}

void RenderLayerCompositor::deviceOrPageScaleFactorChanged()
{
    // Page scale will only be applied at to the RenderView and sublayers, but the device scale factor
    // needs to be applied at the level of rootGraphicsLayer().
    GraphicsLayer* rootLayer = rootGraphicsLayer();
    if (rootLayer)
        rootLayer->noteDeviceOrPageScaleFactorChangedIncludingDescendants();
}

void RenderLayerCompositor::updateScrollCoordinatedStatus(RenderLayer& layer)
{
    ScrollCoordinationReasons coordinationReasons = 0;
    if (isViewportConstrainedFixedOrStickyLayer(layer))
        coordinationReasons |= FixedOrSticky;

    if (useCoordinatedScrollingForLayer(m_renderView, layer))
        coordinationReasons |= Scrolling;

    if (coordinationReasons) {
        if (m_scrollCoordinatedLayers.add(&layer).isNewEntry)
            m_subframeScrollLayersNeedReattach = true;

        updateScrollCoordinatedLayer(layer, coordinationReasons);
    } else
        removeFromScrollCoordinatedLayers(layer);
}

void RenderLayerCompositor::removeFromScrollCoordinatedLayers(RenderLayer& layer)
{
    if (!m_scrollCoordinatedLayers.contains(&layer))
        return;

    m_subframeScrollLayersNeedReattach = true;
    
    m_scrollCoordinatedLayers.remove(&layer);
    m_scrollCoordinatedLayersNeedingUpdate.remove(&layer);

    detachScrollCoordinatedLayer(layer);
}

FixedPositionViewportConstraints RenderLayerCompositor::computeFixedViewportConstraints(RenderLayer& layer) const
{
    ASSERT(layer.isComposited());

    GraphicsLayer* graphicsLayer = layer.backing()->graphicsLayer();
    LayoutRect viewportRect = m_renderView.frameView().viewportConstrainedVisibleContentRect();

    FixedPositionViewportConstraints constraints;
    constraints.setLayerPositionAtLastLayout(graphicsLayer->position());
    constraints.setViewportRectAtLastLayout(viewportRect);
    constraints.setAlignmentOffset(graphicsLayer->pixelAlignmentOffset());

    const RenderStyle& style = layer.renderer().style();
    if (!style.left().isAuto())
        constraints.addAnchorEdge(ViewportConstraints::AnchorEdgeLeft);

    if (!style.right().isAuto())
        constraints.addAnchorEdge(ViewportConstraints::AnchorEdgeRight);

    if (!style.top().isAuto())
        constraints.addAnchorEdge(ViewportConstraints::AnchorEdgeTop);

    if (!style.bottom().isAuto())
        constraints.addAnchorEdge(ViewportConstraints::AnchorEdgeBottom);

    // If left and right are auto, use left.
    if (style.left().isAuto() && style.right().isAuto())
        constraints.addAnchorEdge(ViewportConstraints::AnchorEdgeLeft);

    // If top and bottom are auto, use top.
    if (style.top().isAuto() && style.bottom().isAuto())
        constraints.addAnchorEdge(ViewportConstraints::AnchorEdgeTop);
        
    return constraints;
}

StickyPositionViewportConstraints RenderLayerCompositor::computeStickyViewportConstraints(RenderLayer& layer) const
{
    ASSERT(layer.isComposited());
#if !PLATFORM(IOS)
    // We should never get here for stickies constrained by an enclosing clipping layer.
    // FIXME: Why does this assertion fail on iOS?
    ASSERT(!layer.enclosingOverflowClipLayer(ExcludeSelf));
#endif

    RenderBoxModelObject& renderer = toRenderBoxModelObject(layer.renderer());

    StickyPositionViewportConstraints constraints;
    renderer.computeStickyPositionConstraints(constraints, renderer.constrainingRectForStickyPosition());

    GraphicsLayer* graphicsLayer = layer.backing()->graphicsLayer();

    constraints.setLayerPositionAtLastLayout(graphicsLayer->position());
    constraints.setStickyOffsetAtLastLayout(renderer.stickyPositionOffset());
    constraints.setAlignmentOffset(graphicsLayer->pixelAlignmentOffset());

    return constraints;
}

static ScrollingNodeID enclosingScrollingNodeID(RenderLayer& layer, IncludeSelfOrNot includeSelf)
{
    RenderLayer* currLayer = includeSelf == IncludeSelf ? &layer : layer.parent();
    while (currLayer) {
        if (RenderLayerBacking* backing = currLayer->backing()) {
            if (ScrollingNodeID nodeID = backing->scrollingNodeIDForChildren())
                return nodeID;
        }
        currLayer = currLayer->parent();
    }

    return 0;
}

static ScrollingNodeID scrollCoordinatedAncestorInParentOfFrame(Frame& frame)
{
    if (!frame.document() || !frame.view())
        return 0;

    // Find the frame's enclosing layer in our render tree.
    HTMLFrameOwnerElement* ownerElement = frame.document()->ownerElement();
    RenderElement* frameRenderer = ownerElement ? ownerElement->renderer() : nullptr;
    if (!frameRenderer)
        return 0;

    RenderLayer* layerInParentDocument = frameRenderer->enclosingLayer();
    if (!layerInParentDocument)
        return 0;

    return enclosingScrollingNodeID(*layerInParentDocument, IncludeSelf);
}

void RenderLayerCompositor::reattachSubframeScrollLayers()
{
    if (!m_subframeScrollLayersNeedReattach)
        return;
    
    m_subframeScrollLayersNeedReattach = false;

    ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator();

    for (Frame* child = m_renderView.frameView().frame().tree().firstChild(); child; child = child->tree().nextSibling()) {
        if (!child->document() || !child->view())
            continue;

        // Ignore frames that are not scroll-coordinated.
        FrameView* childFrameView = child->view();
        ScrollingNodeID frameScrollingNodeID = childFrameView->scrollLayerID();
        if (!frameScrollingNodeID)
            continue;

        ScrollingNodeID parentNodeID = scrollCoordinatedAncestorInParentOfFrame(*child);
        if (!parentNodeID)
            continue;

        scrollingCoordinator->attachToStateTree(FrameScrollingNode, frameScrollingNodeID, parentNodeID);
    }
}

ScrollingNodeID RenderLayerCompositor::attachScrollingNode(RenderLayer& layer, ScrollingNodeType nodeType, ScrollingNodeID parentNodeID)
{
    ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator();
    RenderLayerBacking* backing = layer.backing();

    ScrollingNodeID nodeID = backing->scrollingNodeIDForRole(nodeType);
    if (!nodeID)
        nodeID = scrollingCoordinator->uniqueScrollLayerID();

    nodeID = scrollingCoordinator->attachToStateTree(nodeType, nodeID, parentNodeID);
    if (!nodeID)
        return 0;

    backing->setScrollingNodeIDForRole(nodeID, nodeType);
    m_scrollingNodeToLayerMap.add(nodeID, &layer);
    
    return nodeID;
}

void RenderLayerCompositor::updateScrollCoordinationForThisFrame(ScrollingNodeID parentNodeID)
{
    ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator();
    ASSERT(scrollingCoordinator->coordinatesScrollingForFrameView(&m_renderView.frameView()));

    ScrollingNodeID nodeID = attachScrollingNode(*m_renderView.layer(), FrameScrollingNode, parentNodeID);
    scrollingCoordinator->updateFrameScrollingNode(nodeID, m_scrollLayer.get(), m_rootContentLayer.get(), fixedRootBackgroundLayer(), clipLayer());
}

void RenderLayerCompositor::updateScrollCoordinatedLayer(RenderLayer& layer, ScrollCoordinationReasons reasons)
{
    ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator();
    if (!scrollingCoordinator || !scrollingCoordinator->coordinatesScrollingForFrameView(&m_renderView.frameView()))
        return;

    bool isRootLayer = &layer == m_renderView.layer();

    // FIXME: Remove supportsFixedPositionLayers() since all platforms support them now.
    if (!scrollingCoordinator->supportsFixedPositionLayers() || (!layer.parent() && !isRootLayer))
        return;

    ASSERT(m_scrollCoordinatedLayers.contains(&layer));
    ASSERT(layer.isComposited());

    RenderLayerBacking* backing = layer.backing();
    if (!backing)
        return;

    if (!m_renderView.frame().isMainFrame()) {
        ScrollingNodeID parentDocumentHostingNodeID = scrollCoordinatedAncestorInParentOfFrame(m_renderView.frame());
        if (!parentDocumentHostingNodeID)
            return;

        updateScrollCoordinationForThisFrame(parentDocumentHostingNodeID);
        if (!(reasons & FixedOrSticky) && isRootLayer)
            return;
    }

    ScrollingNodeID parentNodeID = enclosingScrollingNodeID(layer, ExcludeSelf);
    if (!parentNodeID && !isRootLayer)
        return;
    
    // Always call this even if the backing is already attached because the parent may have changed.
    // If a node plays both roles, fixed/sticky is always the ancestor node of scrolling.
    if (reasons & FixedOrSticky) {
        ScrollingNodeType nodeType = FrameScrollingNode;
        if (layer.renderer().style().position() == FixedPosition)
            nodeType = FixedNode;
        else if (layer.renderer().style().position() == StickyPosition)
            nodeType = StickyNode;
        else
            ASSERT_NOT_REACHED();

        ScrollingNodeID nodeID = attachScrollingNode(layer, nodeType, parentNodeID);
        if (!nodeID)
            return;

        switch (nodeType) {
        case FixedNode:
            scrollingCoordinator->updateViewportConstrainedNode(nodeID, computeFixedViewportConstraints(layer), backing->graphicsLayer());
            break;
        case StickyNode:
            scrollingCoordinator->updateViewportConstrainedNode(nodeID, computeStickyViewportConstraints(layer), backing->graphicsLayer());
            break;
        case FrameScrollingNode:
        case OverflowScrollingNode:
            break;
        }
        
        parentNodeID = nodeID;
    }

    if (reasons & Scrolling) {
        if (isRootLayer)
            updateScrollCoordinationForThisFrame(parentNodeID);
        else {
            ScrollingNodeType nodeType = OverflowScrollingNode;
            ScrollingNodeID nodeID = attachScrollingNode(layer, nodeType, parentNodeID);
            if (!nodeID)
                return;

            ScrollingCoordinator::ScrollingGeometry scrollingGeometry;
            scrollingGeometry.scrollOrigin = layer.scrollOrigin();
            scrollingGeometry.scrollPosition = layer.scrollPosition();
            scrollingGeometry.scrollableAreaSize = layer.visibleSize();
            scrollingGeometry.contentSize = layer.contentsSize();
            scrollingGeometry.reachableContentSize = layer.scrollableContentsSize();
            scrollingCoordinator->updateOverflowScrollingNode(nodeID, backing->scrollingLayer(), backing->scrollingContentsLayer(), &scrollingGeometry);
        }
    }
}

void RenderLayerCompositor::detachScrollCoordinatedLayer(RenderLayer& layer)
{
    RenderLayerBacking* backing = layer.backing();
    if (!backing)
        return;

    if (ScrollingNodeID nodeID = backing->scrollingNodeIDForRole(FrameScrollingNode))
        m_scrollingNodeToLayerMap.remove(nodeID);

    if (ScrollingNodeID nodeID = backing->scrollingNodeIDForRole(FixedNode))
        m_scrollingNodeToLayerMap.remove(nodeID);

    backing->detachFromScrollingCoordinator();
}

ScrollableArea* RenderLayerCompositor::scrollableAreaForScrollLayerID(ScrollingNodeID nodeID) const
{
    if (!nodeID)
        return nullptr;

    return m_scrollingNodeToLayerMap.get(nodeID);
}

#if PLATFORM(IOS)
typedef HashMap<PlatformLayer*, std::unique_ptr<ViewportConstraints>> LayerMap;
typedef HashMap<PlatformLayer*, PlatformLayer*> StickyContainerMap;

void RenderLayerCompositor::registerAllViewportConstrainedLayers()
{
    // Only the main frame should register fixed/sticky layers.
    if (m_renderView.document().ownerElement())
        return;

    if (scrollingCoordinator())
        return;

    LayerMap layerMap;
    StickyContainerMap stickyContainerMap;

    for (auto it = m_scrollCoordinatedLayers.begin(), end = m_scrollCoordinatedLayers.end(); it != end; ++it) {
        RenderLayer& layer = **it;
        ASSERT(layer.isComposited());

        std::unique_ptr<ViewportConstraints> constraints;
        if (layer.renderer().isStickyPositioned()) {
            constraints = std::make_unique<StickyPositionViewportConstraints>(computeStickyViewportConstraints(layer));
            const RenderLayer* enclosingTouchScrollableLayer = nullptr;
            if (isAsyncScrollableStickyLayer(layer, &enclosingTouchScrollableLayer) && enclosingTouchScrollableLayer) {
                ASSERT(enclosingTouchScrollableLayer->isComposited());
                stickyContainerMap.add(layer.backing()->graphicsLayer()->platformLayer(), enclosingTouchScrollableLayer->backing()->scrollingLayer()->platformLayer());
            }
        } else if (layer.renderer().style().position() == FixedPosition)
            constraints = std::make_unique<FixedPositionViewportConstraints>(computeFixedViewportConstraints(layer));
        else
            continue;

        layerMap.add(layer.backing()->graphicsLayer()->platformLayer(), WTF::move(constraints));
    }
    
    if (ChromeClient* client = this->chromeClient())
        client->updateViewportConstrainedLayers(layerMap, stickyContainerMap);
}

void RenderLayerCompositor::unregisterAllViewportConstrainedLayers()
{
    // Only the main frame should register fixed/sticky layers.
    if (m_renderView.document().ownerElement())
        return;

    if (scrollingCoordinator())
        return;

    if (ChromeClient* client = this->chromeClient()) {
        LayerMap layerMap;
        StickyContainerMap stickyContainerMap;
        client->updateViewportConstrainedLayers(layerMap, stickyContainerMap);
    }
}

void RenderLayerCompositor::registerAllScrollingLayers()
{
    ChromeClient* client = this->chromeClient();
    if (!client)
        return;

    for (auto it = m_scrollingLayers.begin(), end = m_scrollingLayers.end(); it != end; ++it)
        updateScrollingLayerWithClient(**it, client);
}

void RenderLayerCompositor::unregisterAllScrollingLayers()
{
    ChromeClient* client = this->chromeClient();
    if (!client)
        return;

    for (auto it = m_scrollingLayers.begin(), end = m_scrollingLayers.end(); it != end; ++it) {
        RenderLayer& layer = **it;
        RenderLayerBacking* backing = layer.backing();
        ASSERT(backing);
        client->removeScrollingLayer(layer.renderer().element(), backing->scrollingLayer()->platformLayer(), backing->scrollingContentsLayer()->platformLayer());
    }
}
#endif

void RenderLayerCompositor::willRemoveScrollingLayerWithBacking(RenderLayer& layer, RenderLayerBacking& backing)
{
    if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator()) {
        backing.detachFromScrollingCoordinator();

        // For Coordinated Graphics.
        scrollingCoordinator->scrollableAreaScrollLayerDidChange(&layer);
        return;
    }

#if PLATFORM(IOS)
    m_scrollingLayersNeedingUpdate.remove(&layer);
    m_scrollingLayers.remove(&layer);

    if (m_renderView.document().inPageCache())
        return;

    if (ChromeClient* client = this->chromeClient()) {
        PlatformLayer* scrollingLayer = backing.scrollingLayer()->platformLayer();
        PlatformLayer* contentsLayer = backing.scrollingContentsLayer()->platformLayer();
        client->removeScrollingLayer(layer.renderer().element(), scrollingLayer, contentsLayer);
    }
#endif
}

void RenderLayerCompositor::didAddScrollingLayer(RenderLayer& layer)
{
    updateScrollCoordinatedStatus(layer);

    if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator()) {
        // For Coordinated Graphics.
        scrollingCoordinator->scrollableAreaScrollLayerDidChange(&layer);
        return;
    }

#if PLATFORM(IOS)
    ASSERT(!m_renderView.document().inPageCache());
    m_scrollingLayers.add(&layer);
#endif
}

void RenderLayerCompositor::windowScreenDidChange(PlatformDisplayID displayID)
{
    if (m_layerUpdater)
        m_layerUpdater->screenDidChange(displayID);
}

ScrollingCoordinator* RenderLayerCompositor::scrollingCoordinator() const
{
    if (Page* page = this->page())
        return page->scrollingCoordinator();

    return 0;
}

GraphicsLayerFactory* RenderLayerCompositor::graphicsLayerFactory() const
{
    if (Page* page = this->page())
        return page->chrome().client().graphicsLayerFactory();

    return 0;
}

Page* RenderLayerCompositor::page() const
{
    return m_renderView.frameView().frame().page();
}

void RenderLayerCompositor::setLayerFlushThrottlingEnabled(bool enabled)
{
    m_layerFlushThrottlingEnabled = enabled;
    if (m_layerFlushThrottlingEnabled)
        return;
    m_layerFlushTimer.stop();
    if (!m_hasPendingLayerFlush)
        return;
    scheduleLayerFlushNow();
}

void RenderLayerCompositor::disableLayerFlushThrottlingTemporarilyForInteraction()
{
    if (m_layerFlushThrottlingTemporarilyDisabledForInteraction)
        return;
    m_layerFlushThrottlingTemporarilyDisabledForInteraction = true;
}

bool RenderLayerCompositor::isThrottlingLayerFlushes() const
{
    if (!m_layerFlushThrottlingEnabled)
        return false;
    if (!m_layerFlushTimer.isActive())
        return false;
    if (m_layerFlushThrottlingTemporarilyDisabledForInteraction)
        return false;
    return true;
}

void RenderLayerCompositor::startLayerFlushTimerIfNeeded()
{
    m_layerFlushThrottlingTemporarilyDisabledForInteraction = false;
    m_layerFlushTimer.stop();
    if (!m_layerFlushThrottlingEnabled)
        return;
    m_layerFlushTimer.startOneShot(throttledLayerFlushDelay);
}

void RenderLayerCompositor::startInitialLayerFlushTimerIfNeeded()
{
    if (!m_layerFlushThrottlingEnabled)
        return;
    if (m_layerFlushTimer.isActive())
        return;
    m_layerFlushTimer.startOneShot(throttledLayerFlushInitialDelay);
}

void RenderLayerCompositor::layerFlushTimerFired(Timer<RenderLayerCompositor>&)
{
    if (!m_hasPendingLayerFlush)
        return;
    scheduleLayerFlushNow();
}

void RenderLayerCompositor::paintRelatedMilestonesTimerFired(Timer<RenderLayerCompositor>&)
{
    Frame& frame = m_renderView.frameView().frame();
    Page* page = frame.page();
    if (!page)
        return;

    // If the layer tree is frozen, we'll paint when it's unfrozen and schedule the timer again.
    if (page->chrome().client().layerTreeStateIsFrozen())
        return;

    m_renderView.frameView().firePaintRelatedMilestonesIfNeeded();
}

#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
PassRefPtr<DisplayRefreshMonitor> RenderLayerCompositor::createDisplayRefreshMonitor(PlatformDisplayID displayID) const
{
    Frame& frame = m_renderView.frameView().frame();
    Page* page = frame.page();
    if (!page)
        return nullptr;

    return page->chrome().client().createDisplayRefreshMonitor(displayID);
}
#endif

} // namespace WebCore
