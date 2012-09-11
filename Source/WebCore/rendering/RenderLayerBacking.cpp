/*
 * Copyright (C) 2009, 2010, 2011 Apple Inc. All rights reserved.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#if USE(ACCELERATED_COMPOSITING)

#include "RenderLayerBacking.h"

#include "AnimationController.h"
#include "CanvasRenderingContext.h"
#include "CSSPropertyNames.h"
#include "Chrome.h"
#include "FontCache.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "GraphicsLayer.h"
#include "HTMLCanvasElement.h"
#include "HTMLIFrameElement.h"
#include "HTMLMediaElement.h"
#include "HTMLNames.h"
#include "InspectorInstrumentation.h"
#include "KeyframeList.h"
#include "PluginViewBase.h"
#include "RenderApplet.h"
#include "RenderIFrame.h"
#include "RenderImage.h"
#include "RenderLayerCompositor.h"
#include "RenderEmbeddedObject.h"
#include "RenderVideo.h"
#include "RenderView.h"
#include "ScrollingCoordinator.h"
#include "Settings.h"
#include "StyleResolver.h"
#include "TiledBacking.h"
#include <wtf/CurrentTime.h>
#include <wtf/text/StringBuilder.h>

#if ENABLE(CSS_FILTERS)
#include "FilterEffectRenderer.h"
#endif

#if ENABLE(WEBGL) || ENABLE(ACCELERATED_2D_CANVAS)
#include "GraphicsContext3D.h"
#endif

using namespace std;

namespace WebCore {

using namespace HTMLNames;

static bool hasBoxDecorations(const RenderStyle*);
static bool hasBoxDecorationsOrBackground(const RenderObject*);
static bool hasBoxDecorationsOrBackgroundImage(const RenderStyle*);
static IntRect clipBox(RenderBox* renderer);

static inline bool isAcceleratedCanvas(RenderObject* renderer)
{
#if ENABLE(WEBGL) || ENABLE(ACCELERATED_2D_CANVAS)
    if (renderer->isCanvas()) {
        HTMLCanvasElement* canvas = static_cast<HTMLCanvasElement*>(renderer->node());
        if (CanvasRenderingContext* context = canvas->renderingContext())
            return context->isAccelerated();
    }
#else
    UNUSED_PARAM(renderer);
#endif
    return false;
}

bool RenderLayerBacking::m_creatingPrimaryGraphicsLayer = false;

RenderLayerBacking::RenderLayerBacking(RenderLayer* layer)
    : m_owningLayer(layer)
    , m_artificiallyInflatedBounds(false)
    , m_isMainFrameRenderViewLayer(false)
    , m_usingTiledCacheLayer(false)
    , m_requiresOwnBackingStore(true)
#if ENABLE(CSS_FILTERS)
    , m_canCompositeFilters(false)
#endif
{
    if (renderer()->isRenderView()) {
        Frame* frame = toRenderView(renderer())->frameView()->frame();
        Page* page = frame ? frame->page() : 0;
        if (page && frame && page->mainFrame() == frame) {
            m_isMainFrameRenderViewLayer = true;

#if PLATFORM(MAC)
            // FIXME: It's a little weird that we base this decision on whether there's a scrolling coordinator or not.
            if (page->scrollingCoordinator())
                m_usingTiledCacheLayer = true;
#endif
        }
    }
    
    createPrimaryGraphicsLayer();

    if (m_usingTiledCacheLayer) {
        if (Page* page = renderer()->frame()->page()) {
            if (TiledBacking* tiledBacking = m_graphicsLayer->tiledBacking()) {
                Frame* frame = renderer()->frame();

                tiledBacking->setIsInWindow(page->isOnscreen());
                tiledBacking->setCanHaveScrollbars(frame->view()->canHaveScrollbars());
                tiledBacking->setScrollingPerformanceLoggingEnabled(frame->settings() && frame->settings()->scrollingPerformanceLoggingEnabled());
            }
        }
    }
}

RenderLayerBacking::~RenderLayerBacking()
{
    updateClippingLayers(false, false);
    updateOverflowControlsLayers(false, false, false);
    updateForegroundLayer(false);
    updateMaskLayer(false);
    updateScrollingLayers(false);
    destroyGraphicsLayers();
}

PassOwnPtr<GraphicsLayer> RenderLayerBacking::createGraphicsLayer(const String& name)
{
    OwnPtr<GraphicsLayer> graphicsLayer = GraphicsLayer::create(this);
#ifndef NDEBUG
    graphicsLayer->setName(name);
#else
    UNUSED_PARAM(name);
#endif
    graphicsLayer->setMaintainsPixelAlignment(compositor()->keepLayersPixelAligned());

#if PLATFORM(MAC) && USE(CA)
    graphicsLayer->setAcceleratesDrawing(compositor()->acceleratedDrawingEnabled());
#endif    
    
    return graphicsLayer.release();
}

bool RenderLayerBacking::shouldUseTileCache(const GraphicsLayer*) const
{
    return m_usingTiledCacheLayer && m_creatingPrimaryGraphicsLayer;
}

void RenderLayerBacking::createPrimaryGraphicsLayer()
{
    String layerName;
#ifndef NDEBUG
    layerName = nameForLayer();
#endif
    
    // The call to createGraphicsLayer ends calling back into here as
    // a GraphicsLayerClient to ask if it shouldUseTileCache(). We only want
    // the tile cache on our main layer. This is pretty ugly, but saves us from
    // exposing the API to all clients.

    m_creatingPrimaryGraphicsLayer = true;
    m_graphicsLayer = createGraphicsLayer(layerName);
    m_creatingPrimaryGraphicsLayer = false;

    if (m_usingTiledCacheLayer)
        m_containmentLayer = createGraphicsLayer("TileCache Flattening Layer");

    if (m_isMainFrameRenderViewLayer) {
        bool isTransparent = false;
        if (FrameView* frameView = toRenderView(renderer())->frameView())
            isTransparent = frameView->isTransparent();

        m_graphicsLayer->setContentsOpaque(!isTransparent);
        m_graphicsLayer->setAppliesPageScale();
    }

#if PLATFORM(MAC) && USE(CA)
    if (!compositor()->acceleratedDrawingEnabled() && renderer()->isCanvas()) {
        HTMLCanvasElement* canvas = static_cast<HTMLCanvasElement*>(renderer()->node());
        if (canvas->shouldAccelerate(canvas->size()))
            m_graphicsLayer->setAcceleratesDrawing(true);
    }
#endif    
    
    updateOpacity(renderer()->style());
    updateTransform(renderer()->style());
#if ENABLE(CSS_FILTERS)
    updateFilters(renderer()->style());
#endif
#if ENABLE(CSS_COMPOSITING)
    updateLayerBlendMode(renderer()->style());
#endif
}

void RenderLayerBacking::destroyGraphicsLayers()
{
    if (m_graphicsLayer)
        m_graphicsLayer->removeFromParent();

    m_graphicsLayer = nullptr;
    m_foregroundLayer = nullptr;
    m_containmentLayer = nullptr;
    m_maskLayer = nullptr;

    m_scrollingLayer = nullptr;
    m_scrollingContentsLayer = nullptr;
}

void RenderLayerBacking::updateOpacity(const RenderStyle* style)
{
    m_graphicsLayer->setOpacity(compositingOpacity(style->opacity()));
}

void RenderLayerBacking::updateTransform(const RenderStyle* style)
{
    // FIXME: This could use m_owningLayer->transform(), but that currently has transform-origin
    // baked into it, and we don't want that.
    TransformationMatrix t;
    if (m_owningLayer->hasTransform()) {
        style->applyTransform(t, toRenderBox(renderer())->pixelSnappedBorderBoxRect().size(), RenderStyle::ExcludeTransformOrigin);
        makeMatrixRenderable(t, compositor()->canRender3DTransforms());
    }
    
    m_graphicsLayer->setTransform(t);
}

#if ENABLE(CSS_FILTERS)
void RenderLayerBacking::updateFilters(const RenderStyle* style)
{
    m_canCompositeFilters = m_graphicsLayer->setFilters(style->filter());
}
#endif

#if ENABLE(CSS_COMPOSITING)
void RenderLayerBacking::updateLayerBlendMode(const RenderStyle*)
{
}
#endif

static bool hasNonZeroTransformOrigin(const RenderObject* renderer)
{
    RenderStyle* style = renderer->style();
    return (style->transformOriginX().type() == Fixed && style->transformOriginX().value())
        || (style->transformOriginY().type() == Fixed && style->transformOriginY().value());
}

static bool layerOrAncestorIsTransformedOrUsingCompositedScrolling(RenderLayer* layer)
{
    for (RenderLayer* curr = layer; curr; curr = curr->parent()) {
        if (curr->hasTransform() || curr->usesCompositedScrolling())
            return true;
    }
    
    return false;
}

bool RenderLayerBacking::shouldClipCompositedBounds() const
{
    // Scrollbar layers use this layer for relative positioning, so don't clip.
    if (layerForHorizontalScrollbar() || layerForVerticalScrollbar())
        return false;

    if (m_usingTiledCacheLayer)
        return true;

    if (!compositor()->compositingConsultsOverlap())
        return false;

    if (layerOrAncestorIsTransformedOrUsingCompositedScrolling(m_owningLayer))
        return false;

    return true;
}


void RenderLayerBacking::updateCompositedBounds()
{
    IntRect layerBounds = compositor()->calculateCompositedBounds(m_owningLayer, m_owningLayer);

    // Clip to the size of the document or enclosing overflow-scroll layer.
    // If this or an ancestor is transformed, we can't currently compute the correct rect to intersect with.
    // We'd need RenderObject::convertContainerToLocalQuad(), which doesn't yet exist.
    if (shouldClipCompositedBounds()) {
        RenderView* view = m_owningLayer->renderer()->view();
        RenderLayer* rootLayer = view->layer();

        // Start by clipping to the view's bounds.
        LayoutRect clippingBounds = view->unscaledDocumentRect();

        if (m_owningLayer != rootLayer)
            clippingBounds.intersect(m_owningLayer->backgroundClipRect(rootLayer, 0, AbsoluteClipRects).rect()); // FIXME: Incorrect for CSS regions.

        LayoutPoint delta;
        m_owningLayer->convertToLayerCoords(rootLayer, delta);
        clippingBounds.move(-delta.x(), -delta.y());

        layerBounds.intersect(pixelSnappedIntRect(clippingBounds));
    }
    
    // If the element has a transform-origin that has fixed lengths, and the renderer has zero size,
    // then we need to ensure that the compositing layer has non-zero size so that we can apply
    // the transform-origin via the GraphicsLayer anchorPoint (which is expressed as a fractional value).
    if (layerBounds.isEmpty() && hasNonZeroTransformOrigin(renderer())) {
        layerBounds.setWidth(1);
        layerBounds.setHeight(1);
        m_artificiallyInflatedBounds = true;
    } else
        m_artificiallyInflatedBounds = false;

    setCompositedBounds(layerBounds);
}

void RenderLayerBacking::updateAfterWidgetResize()
{
    if (renderer()->isRenderPart()) {
        if (RenderLayerCompositor* innerCompositor = RenderLayerCompositor::frameContentsCompositor(toRenderPart(renderer()))) {
            innerCompositor->frameViewDidChangeSize();
            innerCompositor->frameViewDidChangeLocation(contentsBox().location());
        }
    }
}

void RenderLayerBacking::updateAfterLayout(UpdateDepth updateDepth, bool isUpdateRoot)
{
    RenderLayerCompositor* layerCompositor = compositor();
    if (!layerCompositor->compositingLayersNeedRebuild()) {
        // Calling updateGraphicsLayerGeometry() here gives incorrect results, because the
        // position of this layer's GraphicsLayer depends on the position of our compositing
        // ancestor's GraphicsLayer. That cannot be determined until all the descendant 
        // RenderLayers of that ancestor have been processed via updateLayerPositions().
        //
        // The solution is to update compositing children of this layer here,
        // via updateCompositingChildrenGeometry().
        updateCompositedBounds();
        layerCompositor->updateCompositingDescendantGeometry(m_owningLayer, m_owningLayer, updateDepth);
        
        if (isUpdateRoot) {
            updateGraphicsLayerGeometry();
            layerCompositor->updateRootLayerPosition();
        }
    }
}

bool RenderLayerBacking::updateGraphicsLayerConfiguration()
{
    RenderLayerCompositor* compositor = this->compositor();
    RenderObject* renderer = this->renderer();

    m_owningLayer->updateZOrderLists();

    bool layerConfigChanged = false;
    if (updateForegroundLayer(compositor->needsContentsCompositingLayer(m_owningLayer)))
        layerConfigChanged = true;
    
    bool needsDescendentsClippingLayer = compositor->clipsCompositingDescendants(m_owningLayer);

    // Our scrolling layer will clip.
    if (m_owningLayer->usesCompositedScrolling())
        needsDescendentsClippingLayer = false;

    if (updateClippingLayers(compositor->clippedByAncestor(m_owningLayer), needsDescendentsClippingLayer))
        layerConfigChanged = true;

    if (updateOverflowControlsLayers(requiresHorizontalScrollbarLayer(), requiresVerticalScrollbarLayer(), requiresScrollCornerLayer()))
        layerConfigChanged = true;

    if (updateScrollingLayers(m_owningLayer->usesCompositedScrolling()))
        layerConfigChanged = true;

    if (layerConfigChanged)
        updateInternalHierarchy();

    if (GraphicsLayer* flatteningLayer = tileCacheFlatteningLayer()) {
        flatteningLayer->removeFromParent();
        m_graphicsLayer->addChild(flatteningLayer);
    }

    if (updateMaskLayer(renderer->hasMask()))
        m_graphicsLayer->setMaskLayer(m_maskLayer.get());

    if (m_owningLayer->hasReflection()) {
        if (m_owningLayer->reflectionLayer()->backing()) {
            GraphicsLayer* reflectionLayer = m_owningLayer->reflectionLayer()->backing()->graphicsLayer();
            m_graphicsLayer->setReplicatedByLayer(reflectionLayer);
        }
    } else
        m_graphicsLayer->setReplicatedByLayer(0);

    if (isDirectlyCompositedImage())
        updateImageContents();

    if (renderer->isEmbeddedObject() && toRenderEmbeddedObject(renderer)->allowsAcceleratedCompositing()) {
        PluginViewBase* pluginViewBase = static_cast<PluginViewBase*>(toRenderWidget(renderer)->widget());
        m_graphicsLayer->setContentsToMedia(pluginViewBase->platformLayer());
    }
#if ENABLE(VIDEO)
    else if (renderer->isVideo()) {
        HTMLMediaElement* mediaElement = static_cast<HTMLMediaElement*>(renderer->node());
        m_graphicsLayer->setContentsToMedia(mediaElement->platformLayer());
    }
#endif
#if ENABLE(WEBGL) || ENABLE(ACCELERATED_2D_CANVAS)
    else if (isAcceleratedCanvas(renderer)) {
        HTMLCanvasElement* canvas = static_cast<HTMLCanvasElement*>(renderer->node());
        if (CanvasRenderingContext* context = canvas->renderingContext())
            m_graphicsLayer->setContentsToCanvas(context->platformLayer());
        layerConfigChanged = true;
    }
#endif
#if ENABLE(FULLSCREEN_API)
    else if (renderer->isRenderFullScreen()) {
        // RenderFullScreen renderers have no content, and only a solid
        // background color.  They also can be large enough to trigger the
        // creation of a tiled-layer, which can cause flashing problems
        // during repainting.  Special case the RenderFullScreen case because
        // we know its style does not come from CSS and it is therefore will
        // not contain paintable content (e.g. background images, gradients,
        // etc), so safe to set the layer's background color to the renderer's 
        // style's background color.
        updateBackgroundColor();
    }
#endif
    if (renderer->isRenderPart())
        layerConfigChanged = RenderLayerCompositor::parentFrameContentLayers(toRenderPart(renderer));

    return layerConfigChanged;
}

static IntRect clipBox(RenderBox* renderer)
{
    LayoutRect result = PaintInfo::infiniteRect();
    if (renderer->hasOverflowClip())
        result = renderer->overflowClipRect(LayoutPoint(), 0); // FIXME: Incorrect for CSS regions.

    if (renderer->hasClip())
        result.intersect(renderer->clipRect(LayoutPoint(), 0)); // FIXME: Incorrect for CSS regions.

    return pixelSnappedIntRect(result);
}

void RenderLayerBacking::updateGraphicsLayerGeometry()
{
    // If we haven't built z-order lists yet, wait until later.
    if (m_owningLayer->isStackingContext() && m_owningLayer->m_zOrderListsDirty)
        return;

    // Set transform property, if it is not animating. We have to do this here because the transform
    // is affected by the layer dimensions.
    if (!renderer()->animation()->isRunningAcceleratedAnimationOnRenderer(renderer(), CSSPropertyWebkitTransform))
        updateTransform(renderer()->style());

    // Set opacity, if it is not animating.
    if (!renderer()->animation()->isRunningAcceleratedAnimationOnRenderer(renderer(), CSSPropertyOpacity))
        updateOpacity(renderer()->style());
        
#if ENABLE(CSS_FILTERS)
    updateFilters(renderer()->style());
#endif

#if ENABLE(CSS_COMPOSITING)
    updateLayerBlendMode(renderer()->style());
#endif
    
    m_owningLayer->updateDescendantDependentFlags();

    // m_graphicsLayer is the corresponding GraphicsLayer for this RenderLayer and its non-compositing
    // descendants. So, the visibility flag for m_graphicsLayer should be true if there are any
    // non-compositing visible layers.
    m_graphicsLayer->setContentsVisible(m_owningLayer->hasVisibleContent() || hasVisibleNonCompositingDescendantLayers());

    RenderStyle* style = renderer()->style();
    m_graphicsLayer->setPreserves3D(style->transformStyle3D() == TransformStyle3DPreserve3D && !renderer()->hasReflection());
    m_graphicsLayer->setBackfaceVisibility(style->backfaceVisibility() == BackfaceVisibilityVisible);

    // Register fixed position layers and their containers with the scrolling coordinator.
    if (Page* page = renderer()->frame()->page()) {
        if (ScrollingCoordinator* scrollingCoordinator = page->scrollingCoordinator()) {
            if (style->position() == FixedPosition || compositor()->fixedPositionedByAncestor(m_owningLayer))
                scrollingCoordinator->setLayerIsFixedToContainerLayer(childForSuperlayers(), true);
            else {
                if (m_ancestorClippingLayer)
                    scrollingCoordinator->setLayerIsFixedToContainerLayer(m_ancestorClippingLayer.get(), false);
                scrollingCoordinator->setLayerIsFixedToContainerLayer(m_graphicsLayer.get(), false);
            }
            // Page scale is applied as a transform on the root render view layer. Because the scroll
            // layer is further up in the hierarchy, we need to avoid marking the root render view
            // layer as a container.
            bool isContainer = m_owningLayer->hasTransform() && !m_owningLayer->isRootLayer();
            scrollingCoordinator->setLayerIsContainerForFixedPositionLayers(childForSuperlayers(), isContainer);
        }
    }
    RenderLayer* compAncestor = m_owningLayer->ancestorCompositingLayer();
    
    // We compute everything relative to the enclosing compositing layer.
    IntRect ancestorCompositingBounds;
    if (compAncestor) {
        ASSERT(compAncestor->backing());
        ancestorCompositingBounds = pixelSnappedIntRect(compAncestor->backing()->compositedBounds());
    }

    IntRect localCompositingBounds = pixelSnappedIntRect(compositedBounds());

    IntRect relativeCompositingBounds(localCompositingBounds);
    IntPoint delta;
    m_owningLayer->convertToPixelSnappedLayerCoords(compAncestor, delta);
    relativeCompositingBounds.moveBy(delta);

    IntPoint graphicsLayerParentLocation;
    if (compAncestor && compAncestor->backing()->hasClippingLayer()) {
        // If the compositing ancestor has a layer to clip children, we parent in that, and therefore
        // position relative to it.
        IntRect clippingBox = clipBox(toRenderBox(compAncestor->renderer()));
        graphicsLayerParentLocation = clippingBox.location();
    } else if (compAncestor)
        graphicsLayerParentLocation = ancestorCompositingBounds.location();
    else
        graphicsLayerParentLocation = renderer()->view()->documentRect().location();

    if (compAncestor && compAncestor->usesCompositedScrolling()) {
        RenderBox* renderBox = toRenderBox(compAncestor->renderer());
        IntSize scrollOffset = compAncestor->scrolledContentOffset();
        IntPoint scrollOrigin(renderBox->borderLeft(), renderBox->borderTop());
        graphicsLayerParentLocation = scrollOrigin - scrollOffset;
    }
    
    if (compAncestor && m_ancestorClippingLayer) {
        // Call calculateRects to get the backgroundRect which is what is used to clip the contents of this
        // layer. Note that we call it with temporaryClipRects = true because normally when computing clip rects
        // for a compositing layer, rootLayer is the layer itself.
        IntRect parentClipRect = pixelSnappedIntRect(m_owningLayer->backgroundClipRect(compAncestor, 0, TemporaryClipRects, IgnoreOverlayScrollbarSize, RenderLayer::IgnoreOverflowClip).rect()); // FIXME: Incorrect for CSS regions.
        ASSERT(parentClipRect != PaintInfo::infiniteRect());
        m_ancestorClippingLayer->setPosition(FloatPoint() + (parentClipRect.location() - graphicsLayerParentLocation));
        m_ancestorClippingLayer->setSize(parentClipRect.size());

        // backgroundRect is relative to compAncestor, so subtract deltaX/deltaY to get back to local coords.
        m_ancestorClippingLayer->setOffsetFromRenderer(parentClipRect.location() - delta);

        // The primary layer is then parented in, and positioned relative to this clipping layer.
        graphicsLayerParentLocation = parentClipRect.location();
    }

    m_graphicsLayer->setPosition(FloatPoint() + (relativeCompositingBounds.location() - graphicsLayerParentLocation));
    m_graphicsLayer->setOffsetFromRenderer(localCompositingBounds.location() - IntPoint());
    
    FloatSize oldSize = m_graphicsLayer->size();
    FloatSize newSize = relativeCompositingBounds.size();
    if (oldSize != newSize) {
        m_graphicsLayer->setSize(newSize);
        // A bounds change will almost always require redisplay. Usually that redisplay
        // will happen because of a repaint elsewhere, but not always:
        // e.g. see RenderView::setMaximalOutlineSize()
        m_graphicsLayer->setNeedsDisplay();
    }

    // If we have a layer that clips children, position it.
    IntRect clippingBox;
    if (GraphicsLayer* clipLayer = clippingLayer()) {
        clippingBox = clipBox(toRenderBox(renderer()));
        clipLayer->setPosition(FloatPoint() + (clippingBox.location() - localCompositingBounds.location()));
        clipLayer->setSize(clippingBox.size());
        clipLayer->setOffsetFromRenderer(clippingBox.location() - IntPoint());
    }
    
    if (m_maskLayer) {
        if (m_maskLayer->size() != m_graphicsLayer->size()) {
            m_maskLayer->setSize(m_graphicsLayer->size());
            m_maskLayer->setNeedsDisplay();
        }
        m_maskLayer->setPosition(FloatPoint());
        m_maskLayer->setOffsetFromRenderer(m_graphicsLayer->offsetFromRenderer());
    }
    
    if (m_owningLayer->hasTransform()) {
        const IntRect borderBox = toRenderBox(renderer())->pixelSnappedBorderBoxRect();

        // Get layout bounds in the coords of compAncestor to match relativeCompositingBounds.
        IntRect layerBounds = IntRect(delta, borderBox.size());

        // Update properties that depend on layer dimensions
        FloatPoint3D transformOrigin = computeTransformOrigin(borderBox);
        // Compute the anchor point, which is in the center of the renderer box unless transform-origin is set.
        FloatPoint3D anchor(relativeCompositingBounds.width()  != 0.0f ? ((layerBounds.x() - relativeCompositingBounds.x()) + transformOrigin.x()) / relativeCompositingBounds.width()  : 0.5f,
                            relativeCompositingBounds.height() != 0.0f ? ((layerBounds.y() - relativeCompositingBounds.y()) + transformOrigin.y()) / relativeCompositingBounds.height() : 0.5f,
                            transformOrigin.z());
        m_graphicsLayer->setAnchorPoint(anchor);

        RenderStyle* style = renderer()->style();
        GraphicsLayer* clipLayer = clippingLayer();
        if (style->hasPerspective()) {
            TransformationMatrix t = owningLayer()->perspectiveTransform();
            
            if (clipLayer) {
                clipLayer->setChildrenTransform(t);
                m_graphicsLayer->setChildrenTransform(TransformationMatrix());
            }
            else
                m_graphicsLayer->setChildrenTransform(t);
        } else {
            if (clipLayer)
                clipLayer->setChildrenTransform(TransformationMatrix());
            else
                m_graphicsLayer->setChildrenTransform(TransformationMatrix());
        }
    } else {
        m_graphicsLayer->setAnchorPoint(FloatPoint3D(0.5f, 0.5f, 0));
    }

    if (m_foregroundLayer) {
        FloatPoint foregroundPosition;
        FloatSize foregroundSize = newSize;
        IntSize foregroundOffset = m_graphicsLayer->offsetFromRenderer();
        if (hasClippingLayer()) {
            // If we have a clipping layer (which clips descendants), then the foreground layer is a child of it,
            // so that it gets correctly sorted with children. In that case, position relative to the clipping layer.
            foregroundSize = FloatSize(clippingBox.size());
            foregroundOffset = clippingBox.location() - IntPoint();
        }

        m_foregroundLayer->setPosition(foregroundPosition);
        if (foregroundSize != m_foregroundLayer->size()) {
            m_foregroundLayer->setSize(foregroundSize);
            m_foregroundLayer->setNeedsDisplay();
        }
        m_foregroundLayer->setOffsetFromRenderer(foregroundOffset);
    }

    if (m_owningLayer->reflectionLayer() && m_owningLayer->reflectionLayer()->isComposited()) {
        RenderLayerBacking* reflectionBacking = m_owningLayer->reflectionLayer()->backing();
        reflectionBacking->updateGraphicsLayerGeometry();
        
        // The reflection layer has the bounds of m_owningLayer->reflectionLayer(),
        // but the reflected layer is the bounds of this layer, so we need to position it appropriately.
        FloatRect layerBounds = compositedBounds();
        FloatRect reflectionLayerBounds = reflectionBacking->compositedBounds();
        reflectionBacking->graphicsLayer()->setReplicatedLayerPosition(FloatPoint() + (layerBounds.location() - reflectionLayerBounds.location()));
    }

    if (m_scrollingLayer) {
        ASSERT(m_scrollingContentsLayer);
        RenderBox* renderBox = toRenderBox(renderer());
        IntRect paddingBox(renderBox->borderLeft(), renderBox->borderTop(), renderBox->width() - renderBox->borderLeft() - renderBox->borderRight(), renderBox->height() - renderBox->borderTop() - renderBox->borderBottom());
        IntSize scrollOffset = m_owningLayer->scrolledContentOffset();

        m_scrollingLayer->setPosition(FloatPoint() + (paddingBox.location() - localCompositingBounds.location()));

        m_scrollingLayer->setSize(paddingBox.size());
        m_scrollingContentsLayer->setPosition(FloatPoint(-scrollOffset.width(), -scrollOffset.height()));

        IntSize oldScrollingLayerOffset = m_scrollingLayer->offsetFromRenderer();
        m_scrollingLayer->setOffsetFromRenderer(IntPoint() - paddingBox.location());

        bool paddingBoxOffsetChanged = oldScrollingLayerOffset != m_scrollingLayer->offsetFromRenderer();

        IntSize scrollSize(m_owningLayer->scrollWidth(), m_owningLayer->scrollHeight());
        if (scrollSize != m_scrollingContentsLayer->size() || paddingBoxOffsetChanged)
            m_scrollingContentsLayer->setNeedsDisplay();

        IntSize scrollingContentsOffset = paddingBox.location() - IntPoint() - scrollOffset;
        if (scrollingContentsOffset != m_scrollingContentsLayer->offsetFromRenderer() || scrollSize != m_scrollingContentsLayer->size())
            compositor()->scrollingLayerDidChange(m_owningLayer);

        m_scrollingContentsLayer->setSize(scrollSize);
        // FIXME: Scrolling the content layer does not need to trigger a repaint. The offset will be compensated away during painting.
        // FIXME: The paint offset and the scroll offset should really be separate concepts.
        m_scrollingContentsLayer->setOffsetFromRenderer(scrollingContentsOffset);
    }

    m_graphicsLayer->setContentsRect(contentsBox());

    // If this layer was created just for clipping or to apply perspective, it doesn't need its own backing store.
    setRequiresOwnBackingStore(compositor()->requiresOwnBackingStore(m_owningLayer, compAncestor));

    updateDrawsContent();
    updateAfterWidgetResize();
}

void RenderLayerBacking::updateInternalHierarchy()
{
    // m_foregroundLayer has to be inserted in the correct order with child layers,
    // so it's not inserted here.
    if (m_ancestorClippingLayer) {
        m_ancestorClippingLayer->removeAllChildren();
        m_graphicsLayer->removeFromParent();
        m_ancestorClippingLayer->addChild(m_graphicsLayer.get());
    }

    if (m_containmentLayer) {
        m_containmentLayer->removeFromParent();
        m_graphicsLayer->addChild(m_containmentLayer.get());
    }

    if (m_scrollingLayer) {
        GraphicsLayer* superlayer = m_containmentLayer ? m_containmentLayer.get() : m_graphicsLayer.get();
        m_scrollingLayer->removeFromParent();
        superlayer->addChild(m_scrollingLayer.get());
    }

    // The clip for child layers does not include space for overflow controls, so they exist as
    // siblings of the clipping layer if we have one. Normal children of this layer are set as
    // children of the clipping layer.
    if (m_layerForHorizontalScrollbar) {
        m_layerForHorizontalScrollbar->removeFromParent();
        m_graphicsLayer->addChild(m_layerForHorizontalScrollbar.get());
    }
    if (m_layerForVerticalScrollbar) {
        m_layerForVerticalScrollbar->removeFromParent();
        m_graphicsLayer->addChild(m_layerForVerticalScrollbar.get());
    }
    if (m_layerForScrollCorner) {
        m_layerForScrollCorner->removeFromParent();
        m_graphicsLayer->addChild(m_layerForScrollCorner.get());
    }
}

void RenderLayerBacking::updateDrawsContent()
{
    if (m_scrollingLayer) {
        // We don't have to consider overflow controls, because we know that the scrollbars are drawn elsewhere.
        // m_graphicsLayer only needs backing store if the non-scrolling parts (background, outlines, borders, shadows etc) need to paint.
        // m_scrollingLayer never has backing store.
        // m_scrollingContentsLayer only needs backing store if the scrolled contents need to paint.
        bool hasNonScrollingPaintedContent = m_owningLayer->hasVisibleContent() && hasBoxDecorationsOrBackground(renderer());
        m_graphicsLayer->setDrawsContent(hasNonScrollingPaintedContent);

        bool hasScrollingPaintedContent = m_owningLayer->hasVisibleContent() && (renderer()->hasBackground() || paintsChildren());
        m_scrollingContentsLayer->setDrawsContent(hasScrollingPaintedContent);
        return;
    }

    bool hasPaintedContent = containsPaintedContent();

    // FIXME: we could refine this to only allocate backing for one of these layers if possible.
    m_graphicsLayer->setDrawsContent(hasPaintedContent);
    if (m_foregroundLayer)
        m_foregroundLayer->setDrawsContent(hasPaintedContent);
}

// Return true if the layers changed.
bool RenderLayerBacking::updateClippingLayers(bool needsAncestorClip, bool needsDescendantClip)
{
    bool layersChanged = false;

    if (needsAncestorClip) {
        if (!m_ancestorClippingLayer) {
            m_ancestorClippingLayer = createGraphicsLayer("Ancestor clipping Layer");
            m_ancestorClippingLayer->setMasksToBounds(true);
            layersChanged = true;
        }
    } else if (m_ancestorClippingLayer) {
        m_ancestorClippingLayer->removeFromParent();
        m_ancestorClippingLayer = nullptr;
        layersChanged = true;
    }
    
    if (needsDescendantClip) {
        if (!m_containmentLayer && !m_usingTiledCacheLayer) {
            m_containmentLayer = createGraphicsLayer("Child clipping Layer");
            m_containmentLayer->setMasksToBounds(true);
            layersChanged = true;
        }
    } else if (hasClippingLayer()) {
        m_containmentLayer->removeFromParent();
        m_containmentLayer = nullptr;
        layersChanged = true;
    }
    
    return layersChanged;
}

bool RenderLayerBacking::requiresHorizontalScrollbarLayer() const
{
#if !PLATFORM(CHROMIUM)
    if (!m_owningLayer->hasOverlayScrollbars())
        return false;
#endif
    return m_owningLayer->horizontalScrollbar();
}

bool RenderLayerBacking::requiresVerticalScrollbarLayer() const
{
#if !PLATFORM(CHROMIUM)
    if (!m_owningLayer->hasOverlayScrollbars())
        return false;
#endif
    return m_owningLayer->verticalScrollbar();
}

bool RenderLayerBacking::requiresScrollCornerLayer() const
{
#if !PLATFORM(CHROMIUM)
    if (!m_owningLayer->hasOverlayScrollbars())
        return false;
#endif
    return !m_owningLayer->scrollCornerAndResizerRect().isEmpty();
}

bool RenderLayerBacking::updateOverflowControlsLayers(bool needsHorizontalScrollbarLayer, bool needsVerticalScrollbarLayer, bool needsScrollCornerLayer)
{
    bool layersChanged = false;
    if (needsHorizontalScrollbarLayer) {
        if (!m_layerForHorizontalScrollbar) {
            m_layerForHorizontalScrollbar = createGraphicsLayer("horizontal scrollbar");
            layersChanged = true;
        }
    } else if (m_layerForHorizontalScrollbar) {
        m_layerForHorizontalScrollbar.clear();
        layersChanged = true;
    }

    if (needsVerticalScrollbarLayer) {
        if (!m_layerForVerticalScrollbar) {
            m_layerForVerticalScrollbar = createGraphicsLayer("vertical scrollbar");
            layersChanged = true;
        }
    } else if (m_layerForVerticalScrollbar) {
        m_layerForVerticalScrollbar.clear();
        layersChanged = true;
    }

    if (needsScrollCornerLayer) {
        if (!m_layerForScrollCorner) {
            m_layerForScrollCorner = createGraphicsLayer("scroll corner");
            layersChanged = true;
        }
    } else if (m_layerForScrollCorner) {
        m_layerForScrollCorner.clear();
        layersChanged = true;
    }

    return layersChanged;
}

void RenderLayerBacking::positionOverflowControlsLayers(const IntSize& offsetFromRoot)
{
    IntSize offsetFromRenderer = m_graphicsLayer->offsetFromRenderer();
    if (GraphicsLayer* layer = layerForHorizontalScrollbar()) {
        Scrollbar* hBar = m_owningLayer->horizontalScrollbar();
        if (hBar) {
            layer->setPosition(hBar->frameRect().location() - offsetFromRoot - offsetFromRenderer);
            layer->setSize(hBar->frameRect().size());
        }
        layer->setDrawsContent(hBar);
    }
    
    if (GraphicsLayer* layer = layerForVerticalScrollbar()) {
        Scrollbar* vBar = m_owningLayer->verticalScrollbar();
        if (vBar) {
            layer->setPosition(vBar->frameRect().location() - offsetFromRoot - offsetFromRenderer);
            layer->setSize(vBar->frameRect().size());
        }
        layer->setDrawsContent(vBar);
    }

    if (GraphicsLayer* layer = layerForScrollCorner()) {
        const LayoutRect& scrollCornerAndResizer = m_owningLayer->scrollCornerAndResizerRect();
        layer->setPosition(scrollCornerAndResizer.location() - offsetFromRenderer);
        layer->setSize(scrollCornerAndResizer.size());
        layer->setDrawsContent(!scrollCornerAndResizer.isEmpty());
    }
}

bool RenderLayerBacking::updateForegroundLayer(bool needsForegroundLayer)
{
    bool layerChanged = false;
    if (needsForegroundLayer) {
        if (!m_foregroundLayer) {
            String layerName;
#ifndef NDEBUG
            layerName = nameForLayer() + " (foreground)";
#endif
            m_foregroundLayer = createGraphicsLayer(layerName);
            m_foregroundLayer->setDrawsContent(true);
            m_foregroundLayer->setPaintingPhase(GraphicsLayerPaintForeground);
            layerChanged = true;
        }
    } else if (m_foregroundLayer) {
        m_foregroundLayer->removeFromParent();
        m_foregroundLayer = nullptr;
        layerChanged = true;
    }

    if (layerChanged)
        m_graphicsLayer->setPaintingPhase(paintingPhaseForPrimaryLayer());

    return layerChanged;
}

bool RenderLayerBacking::updateMaskLayer(bool needsMaskLayer)
{
    bool layerChanged = false;
    if (needsMaskLayer) {
        if (!m_maskLayer) {
            m_maskLayer = createGraphicsLayer("Mask");
            m_maskLayer->setDrawsContent(true);
            m_maskLayer->setPaintingPhase(GraphicsLayerPaintMask);
            layerChanged = true;
        }
    } else if (m_maskLayer) {
        m_maskLayer = nullptr;
        layerChanged = true;
    }

    if (layerChanged)
        m_graphicsLayer->setPaintingPhase(paintingPhaseForPrimaryLayer());

    return layerChanged;
}

bool RenderLayerBacking::updateScrollingLayers(bool needsScrollingLayers)
{
    bool layerChanged = false;
    if (needsScrollingLayers) {
        if (!m_scrollingLayer) {
            // Outer layer which corresponds with the scroll view.
            m_scrollingLayer = createGraphicsLayer("Scrolling container");
            m_scrollingLayer->setDrawsContent(false);
            m_scrollingLayer->setMasksToBounds(true);

            // Inner layer which renders the content that scrolls.
            m_scrollingContentsLayer = createGraphicsLayer("Scrolled Contents");
            m_scrollingContentsLayer->setDrawsContent(true);
            m_scrollingContentsLayer->setPaintingPhase(GraphicsLayerPaintForeground | GraphicsLayerPaintOverflowContents);
            m_scrollingLayer->addChild(m_scrollingContentsLayer.get());

            layerChanged = true;
        }
    } else if (m_scrollingLayer) {
        m_scrollingLayer = nullptr;
        m_scrollingContentsLayer = nullptr;
        layerChanged = true;
    }

    if (layerChanged) {
        updateInternalHierarchy();
        m_graphicsLayer->setPaintingPhase(paintingPhaseForPrimaryLayer());
        m_graphicsLayer->setNeedsDisplay();
        if (renderer()->view())
            compositor()->scrollingLayerDidChange(m_owningLayer);
    }

    return layerChanged;
}

GraphicsLayerPaintingPhase RenderLayerBacking::paintingPhaseForPrimaryLayer() const
{
    unsigned phase = GraphicsLayerPaintBackground;
    if (!m_foregroundLayer)
        phase |= GraphicsLayerPaintForeground;
    if (!m_maskLayer)
        phase |= GraphicsLayerPaintMask;

    if (m_scrollingContentsLayer)
        phase &= ~GraphicsLayerPaintForeground;

    return static_cast<GraphicsLayerPaintingPhase>(phase);
}

float RenderLayerBacking::compositingOpacity(float rendererOpacity) const
{
    float finalOpacity = rendererOpacity;
    
    for (RenderLayer* curr = m_owningLayer->parent(); curr; curr = curr->parent()) {
        // We only care about parents that are stacking contexts.
        // Recall that opacity creates stacking context.
        if (!curr->isStackingContext())
            continue;
        
        // If we found a compositing layer, we want to compute opacity
        // relative to it. So we can break here.
        if (curr->isComposited())
            break;
        
        finalOpacity *= curr->renderer()->opacity();
    }

    return finalOpacity;
}

static bool hasBoxDecorations(const RenderStyle* style)
{
    return style->hasBorder() || style->hasBorderRadius() || style->hasOutline() || style->hasAppearance() || style->boxShadow() || style->hasFilter();
}

static bool hasBoxDecorationsOrBackground(const RenderObject* renderer)
{
    return hasBoxDecorations(renderer->style()) || renderer->hasBackground();
}

static bool hasBoxDecorationsOrBackgroundImage(const RenderStyle* style)
{
    return hasBoxDecorations(style) || style->hasBackgroundImage();
}

Color RenderLayerBacking::rendererBackgroundColor() const
{
    RenderObject* backgroundRenderer = renderer();
    if (backgroundRenderer->isRoot())
        backgroundRenderer = backgroundRenderer->rendererForRootBackground();

    return backgroundRenderer->style()->visitedDependentColor(CSSPropertyBackgroundColor);
}

void RenderLayerBacking::updateBackgroundColor()
{
    m_graphicsLayer->setContentsToBackgroundColor(rendererBackgroundColor());
}

bool RenderLayerBacking::paintsBoxDecorations() const
{
    if (!m_owningLayer->hasVisibleContent())
        return false;

    if (hasBoxDecorationsOrBackground(renderer()))
        return true;

    if (m_owningLayer->hasOverflowControls())
        return true;

    return false;
}

bool RenderLayerBacking::paintsChildren() const
{
    if (m_owningLayer->hasVisibleContent() && containsNonEmptyRenderers())
        return true;
        
    if (hasVisibleNonCompositingDescendantLayers())
        return true;

    return false;
}

static bool isCompositedPlugin(RenderObject* renderer)
{
    return renderer->isEmbeddedObject() && toRenderEmbeddedObject(renderer)->allowsAcceleratedCompositing();
}

// A "simple container layer" is a RenderLayer which has no visible content to render.
// It may have no children, or all its children may be themselves composited.
// This is a useful optimization, because it allows us to avoid allocating backing store.
bool RenderLayerBacking::isSimpleContainerCompositingLayer() const
{
    RenderObject* renderObject = renderer();
    if (renderObject->hasMask()) // masks require special treatment
        return false;

    if (renderObject->isReplaced() && !isCompositedPlugin(renderObject))
            return false;
    
    if (paintsBoxDecorations() || paintsChildren())
        return false;
    
    if (renderObject->node() && renderObject->node()->isDocumentNode()) {
        // Look to see if the root object has a non-simple background
        RenderObject* rootObject = renderObject->document()->documentElement() ? renderObject->document()->documentElement()->renderer() : 0;
        if (!rootObject)
            return false;
        
        RenderStyle* style = rootObject->style();
        
        // Reject anything that has a border, a border-radius or outline,
        // or is not a simple background (no background, or solid color).
        if (hasBoxDecorationsOrBackgroundImage(style))
            return false;
        
        // Now look at the body's renderer.
        HTMLElement* body = renderObject->document()->body();
        RenderObject* bodyObject = (body && body->hasLocalName(bodyTag)) ? body->renderer() : 0;
        if (!bodyObject)
            return false;
        
        style = bodyObject->style();
        
        if (hasBoxDecorationsOrBackgroundImage(style))
            return false;
    }

    return true;
}

bool RenderLayerBacking::containsNonEmptyRenderers() const
{
    // Some HTML can cause whitespace text nodes to have renderers, like:
    // <div>
    // <img src=...>
    // </div>
    // so test for 0x0 RenderTexts here
    for (RenderObject* child = renderer()->firstChild(); child; child = child->nextSibling()) {
        if (!child->hasLayer()) {
            if (child->isRenderInline() || !child->isBox())
                return true;
            
            if (toRenderBox(child)->width() > 0 || toRenderBox(child)->height() > 0)
                return true;
        }
    }
    return false;
}

// Conservative test for having no rendered children.
bool RenderLayerBacking::hasVisibleNonCompositingDescendantLayers() const
{
    // FIXME: We shouldn't be called with a stale z-order lists. See bug 85512.
    m_owningLayer->updateLayerListsIfNeeded();

#if !ASSERT_DISABLED
    LayerListMutationDetector mutationChecker(m_owningLayer);
#endif

    if (Vector<RenderLayer*>* normalFlowList = m_owningLayer->normalFlowList()) {
        size_t listSize = normalFlowList->size();
        for (size_t i = 0; i < listSize; ++i) {
            RenderLayer* curLayer = normalFlowList->at(i);
            if (!curLayer->isComposited() && curLayer->hasVisibleContent())
                return true;
        }
    }

    if (m_owningLayer->isStackingContext()) {
        if (!m_owningLayer->hasVisibleDescendant())
            return false;

        // Use the m_hasCompositingDescendant bit to optimize?
        if (Vector<RenderLayer*>* negZOrderList = m_owningLayer->negZOrderList()) {
            size_t listSize = negZOrderList->size();
            for (size_t i = 0; i < listSize; ++i) {
                RenderLayer* curLayer = negZOrderList->at(i);
                if (!curLayer->isComposited() && curLayer->hasVisibleContent())
                    return true;
            }
        }

        if (Vector<RenderLayer*>* posZOrderList = m_owningLayer->posZOrderList()) {
            size_t listSize = posZOrderList->size();
            for (size_t i = 0; i < listSize; ++i) {
                RenderLayer* curLayer = posZOrderList->at(i);
                if (!curLayer->isComposited() && curLayer->hasVisibleContent())
                    return true;
            }
        }
    }

    return false;
}

bool RenderLayerBacking::containsPaintedContent() const
{
    if (isSimpleContainerCompositingLayer() || paintsIntoWindow() || paintsIntoCompositedAncestor() || m_artificiallyInflatedBounds || m_owningLayer->isReflection())
        return false;

    if (isDirectlyCompositedImage())
        return false;

    // FIXME: we could optimize cases where the image, video or canvas is known to fill the border box entirely,
    // and set background color on the layer in that case, instead of allocating backing store and painting.
#if ENABLE(VIDEO)
    if (renderer()->isVideo() && toRenderVideo(renderer())->shouldDisplayVideo())
        return hasBoxDecorationsOrBackground(renderer());
#endif
#if PLATFORM(MAC) && USE(CA) && (PLATFORM(IOS) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070)
#elif ENABLE(WEBGL) || ENABLE(ACCELERATED_2D_CANVAS)
    if (isAcceleratedCanvas(renderer()))
        return hasBoxDecorationsOrBackground(renderer());
#endif

    return true;
}

// An image can be directly compositing if it's the sole content of the layer, and has no box decorations
// that require painting. Direct compositing saves backing store.
bool RenderLayerBacking::isDirectlyCompositedImage() const
{
    RenderObject* renderObject = renderer();
    
    if (!renderObject->isImage() || hasBoxDecorationsOrBackground(renderObject) || renderObject->hasClip())
        return false;

    RenderImage* imageRenderer = toRenderImage(renderObject);
    if (CachedImage* cachedImage = imageRenderer->cachedImage()) {
        if (cachedImage->hasImage())
            return cachedImage->imageForRenderer(imageRenderer)->isBitmapImage();
    }

    return false;
}

void RenderLayerBacking::contentChanged(ContentChangeType changeType)
{
    if ((changeType == ImageChanged) && isDirectlyCompositedImage()) {
        updateImageContents();
        return;
    }
    
    if ((changeType == MaskImageChanged) && m_maskLayer) {
        // The composited layer bounds relies on box->maskClipRect(), which changes
        // when the mask image becomes available.
        bool isUpdateRoot = true;
        updateAfterLayout(CompositingChildren, isUpdateRoot);
    }

#if ENABLE(WEBGL) || ENABLE(ACCELERATED_2D_CANVAS)
    if ((changeType == CanvasChanged || changeType == CanvasPixelsChanged) && isAcceleratedCanvas(renderer())) {
        m_graphicsLayer->setContentsNeedsDisplay();
        return;
    }
#endif
}

void RenderLayerBacking::updateImageContents()
{
    ASSERT(renderer()->isImage());
    RenderImage* imageRenderer = toRenderImage(renderer());

    CachedImage* cachedImage = imageRenderer->cachedImage();
    if (!cachedImage)
        return;

    Image* image = cachedImage->imageForRenderer(imageRenderer);
    if (!image)
        return;

    // We have to wait until the image is fully loaded before setting it on the layer.
    if (!cachedImage->isLoaded())
        return;

    // This is a no-op if the layer doesn't have an inner layer for the image.
    m_graphicsLayer->setContentsToImage(image);
    updateDrawsContent();
    
    // Image animation is "lazy", in that it automatically stops unless someone is drawing
    // the image. So we have to kick the animation each time; this has the downside that the
    // image will keep animating, even if its layer is not visible.
    image->startAnimation();
}

FloatPoint3D RenderLayerBacking::computeTransformOrigin(const IntRect& borderBox) const
{
    RenderStyle* style = renderer()->style();

    FloatPoint3D origin;
    origin.setX(floatValueForLength(style->transformOriginX(), borderBox.width()));
    origin.setY(floatValueForLength(style->transformOriginY(), borderBox.height()));
    origin.setZ(style->transformOriginZ());

    return origin;
}

FloatPoint RenderLayerBacking::computePerspectiveOrigin(const IntRect& borderBox) const
{
    RenderStyle* style = renderer()->style();

    float boxWidth = borderBox.width();
    float boxHeight = borderBox.height();

    FloatPoint origin;
    origin.setX(floatValueForLength(style->perspectiveOriginX(), boxWidth));
    origin.setY(floatValueForLength(style->perspectiveOriginY(), boxHeight));

    return origin;
}

// Return the offset from the top-left of this compositing layer at which the renderer's contents are painted.
IntSize RenderLayerBacking::contentOffsetInCompostingLayer() const
{
    return IntSize(-m_compositedBounds.x(), -m_compositedBounds.y());
}

IntRect RenderLayerBacking::contentsBox() const
{
    if (!renderer()->isBox())
        return IntRect();

    IntRect contentsRect;
#if ENABLE(VIDEO)
    if (renderer()->isVideo()) {
        RenderVideo* videoRenderer = toRenderVideo(renderer());
        contentsRect = videoRenderer->videoBox();
    } else
#endif
        contentsRect = pixelSnappedIntRect(toRenderBox(renderer())->contentBoxRect());

    IntSize contentOffset = contentOffsetInCompostingLayer();
    contentsRect.move(contentOffset);
    return contentsRect;
}

GraphicsLayer* RenderLayerBacking::parentForSublayers() const
{
    if (m_scrollingContentsLayer)
        return m_scrollingContentsLayer.get();

    return m_containmentLayer ? m_containmentLayer.get() : m_graphicsLayer.get();
}

bool RenderLayerBacking::paintsIntoWindow() const
{
    if (m_usingTiledCacheLayer)
        return false;

    if (m_owningLayer->isRootLayer()) {
#if PLATFORM(BLACKBERRY)
        if (compositor()->inForcedCompositingMode())
            return false;
#endif

        return compositor()->rootLayerAttachment() != RenderLayerCompositor::RootLayerAttachedViaEnclosingFrame;
    }
    
    return false;
}

void RenderLayerBacking::setRequiresOwnBackingStore(bool requiresOwnBacking)
{
    if (requiresOwnBacking == m_requiresOwnBackingStore)
        return;
    
    // This affects the answer to paintsIntoCompositedAncestor(), which in turn affects
    // cached clip rects, so when it changes we have to clear clip rects on descendants.
    m_owningLayer->clearClipRectsIncludingDescendants(PaintingClipRects);
    m_requiresOwnBackingStore = requiresOwnBacking;
    
    compositor()->repaintInCompositedAncestor(m_owningLayer, compositedBounds());
}

#if ENABLE(CSS_COMPOSITING)
void RenderLayerBacking::setBlendMode(BlendMode)
{
}
#endif

void RenderLayerBacking::setContentsNeedDisplay()
{
    ASSERT(!paintsIntoCompositedAncestor());
    
    if (m_graphicsLayer && m_graphicsLayer->drawsContent())
        m_graphicsLayer->setNeedsDisplay();
    
    if (m_foregroundLayer && m_foregroundLayer->drawsContent())
        m_foregroundLayer->setNeedsDisplay();

    if (m_maskLayer && m_maskLayer->drawsContent())
        m_maskLayer->setNeedsDisplay();

    if (m_scrollingContentsLayer && m_scrollingContentsLayer->drawsContent())
        m_scrollingContentsLayer->setNeedsDisplay();
}

// r is in the coordinate space of the layer's render object
void RenderLayerBacking::setContentsNeedDisplayInRect(const IntRect& r)
{
    ASSERT(!paintsIntoCompositedAncestor());

    if (m_graphicsLayer && m_graphicsLayer->drawsContent()) {
        IntRect layerDirtyRect = r;
        layerDirtyRect.move(-m_graphicsLayer->offsetFromRenderer());
        m_graphicsLayer->setNeedsDisplayInRect(layerDirtyRect);
    }

    if (m_foregroundLayer && m_foregroundLayer->drawsContent()) {
        IntRect layerDirtyRect = r;
        layerDirtyRect.move(-m_foregroundLayer->offsetFromRenderer());
        m_foregroundLayer->setNeedsDisplayInRect(layerDirtyRect);
    }

    if (m_maskLayer && m_maskLayer->drawsContent()) {
        IntRect layerDirtyRect = r;
        layerDirtyRect.move(-m_maskLayer->offsetFromRenderer());
        m_maskLayer->setNeedsDisplayInRect(layerDirtyRect);
    }

    if (m_scrollingContentsLayer && m_scrollingContentsLayer->drawsContent()) {
        IntRect layerDirtyRect = r;
        layerDirtyRect.move(-m_scrollingContentsLayer->offsetFromRenderer());
        m_scrollingContentsLayer->setNeedsDisplayInRect(layerDirtyRect);
    }
}

void RenderLayerBacking::paintIntoLayer(RenderLayer* rootLayer, GraphicsContext* context,
                    const IntRect& paintDirtyRect, // In the coords of rootLayer.
                    PaintBehavior paintBehavior, GraphicsLayerPaintingPhase paintingPhase,
                    RenderObject* paintingRoot)
{
    if (paintsIntoWindow() || paintsIntoCompositedAncestor()) {
        ASSERT_NOT_REACHED();
        return;
    }

    FontCachePurgePreventer fontCachePurgePreventer;
    
    RenderLayer::PaintLayerFlags paintFlags = 0;
    if (paintingPhase & GraphicsLayerPaintBackground)
        paintFlags |= RenderLayer::PaintLayerPaintingCompositingBackgroundPhase;
    if (paintingPhase & GraphicsLayerPaintForeground)
        paintFlags |= RenderLayer::PaintLayerPaintingCompositingForegroundPhase;
    if (paintingPhase & GraphicsLayerPaintMask)
        paintFlags |= RenderLayer::PaintLayerPaintingCompositingMaskPhase;
    if (paintingPhase & GraphicsLayerPaintOverflowContents)
        paintFlags |= RenderLayer::PaintLayerPaintingOverflowContents;
    
    // FIXME: GraphicsLayers need a way to split for RenderRegions.
    m_owningLayer->paintLayerContents(rootLayer, context, paintDirtyRect, LayoutSize(), paintBehavior, paintingRoot, 0, 0, paintFlags);

    if (m_owningLayer->containsDirtyOverlayScrollbars())
        m_owningLayer->paintOverlayScrollbars(context, paintDirtyRect, paintBehavior, paintingRoot);

    ASSERT(!m_owningLayer->m_usedTransparency);
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

// Up-call from compositing layer drawing callback.
void RenderLayerBacking::paintContents(const GraphicsLayer* graphicsLayer, GraphicsContext& context, GraphicsLayerPaintingPhase paintingPhase, const IntRect& clip)
{
#ifndef NDEBUG
    if (Page* page = renderer()->frame()->page())
        page->setIsPainting(true);
#endif

    if (graphicsLayer == m_graphicsLayer.get() || graphicsLayer == m_foregroundLayer.get() || graphicsLayer == m_maskLayer.get() || graphicsLayer == m_scrollingContentsLayer.get()) {
        InspectorInstrumentationCookie cookie = InspectorInstrumentation::willPaint(m_owningLayer->renderer()->frame(), &context, clip);

        // The dirtyRect is in the coords of the painting root.
        IntRect dirtyRect = clip;
        if (!(paintingPhase & GraphicsLayerPaintOverflowContents))
            dirtyRect.intersect(compositedBounds());

        // We have to use the same root as for hit testing, because both methods can compute and cache clipRects.
        paintIntoLayer(m_owningLayer, &context, dirtyRect, PaintBehaviorNormal, paintingPhase, renderer());

        if (m_usingTiledCacheLayer)
            m_owningLayer->renderer()->frame()->view()->setLastPaintTime(currentTime());

        InspectorInstrumentation::didPaint(cookie);
    } else if (graphicsLayer == layerForHorizontalScrollbar()) {
        paintScrollbar(m_owningLayer->horizontalScrollbar(), context, clip);
    } else if (graphicsLayer == layerForVerticalScrollbar()) {
        paintScrollbar(m_owningLayer->verticalScrollbar(), context, clip);
    } else if (graphicsLayer == layerForScrollCorner()) {
        const IntRect& scrollCornerAndResizer = m_owningLayer->scrollCornerAndResizerRect();
        context.save();
        context.translate(-scrollCornerAndResizer.x(), -scrollCornerAndResizer.y());
        IntRect transformedClip = clip;
        transformedClip.moveBy(scrollCornerAndResizer.location());
        m_owningLayer->paintScrollCorner(&context, IntPoint(), transformedClip);
        m_owningLayer->paintResizer(&context, IntPoint(), transformedClip);
        context.restore();
    }
#ifndef NDEBUG
    if (Page* page = renderer()->frame()->page())
        page->setIsPainting(false);
#endif
}

float RenderLayerBacking::pageScaleFactor() const
{
    return compositor()->pageScaleFactor();
}

float RenderLayerBacking::deviceScaleFactor() const
{
    return compositor()->deviceScaleFactor();
}

void RenderLayerBacking::didCommitChangesForLayer(const GraphicsLayer*) const
{
    compositor()->didFlushChangesForLayer(m_owningLayer);
}

bool RenderLayerBacking::showDebugBorders(const GraphicsLayer*) const
{
    return compositor() ? compositor()->compositorShowDebugBorders() : false;
}

bool RenderLayerBacking::showRepaintCounter(const GraphicsLayer*) const
{
    return compositor() ? compositor()->compositorShowRepaintCounter() : false;
}

#ifndef NDEBUG
void RenderLayerBacking::verifyNotPainting()
{
    ASSERT(!renderer()->frame()->page() || !renderer()->frame()->page()->isPainting());
}
#endif

bool RenderLayerBacking::startAnimation(double timeOffset, const Animation* anim, const KeyframeList& keyframes)
{
    bool hasOpacity = keyframes.containsProperty(CSSPropertyOpacity);
    bool hasTransform = renderer()->isBox() && keyframes.containsProperty(CSSPropertyWebkitTransform);
#if ENABLE(CSS_FILTERS)
    bool hasFilter = keyframes.containsProperty(CSSPropertyWebkitFilter);
#else
    bool hasFilter = false;
#endif

    if (!hasOpacity && !hasTransform && !hasFilter)
        return false;
    
    KeyframeValueList transformVector(AnimatedPropertyWebkitTransform);
    KeyframeValueList opacityVector(AnimatedPropertyOpacity);
#if ENABLE(CSS_FILTERS)
    KeyframeValueList filterVector(AnimatedPropertyWebkitFilter);
#endif

    size_t numKeyframes = keyframes.size();
    for (size_t i = 0; i < numKeyframes; ++i) {
        const KeyframeValue& currentKeyframe = keyframes[i];
        const RenderStyle* keyframeStyle = currentKeyframe.style();
        float key = currentKeyframe.key();

        if (!keyframeStyle)
            continue;
            
        // Get timing function.
        RefPtr<TimingFunction> tf = keyframeStyle->hasAnimations() ? (*keyframeStyle->animations()).animation(0)->timingFunction() : 0;
        
        bool isFirstOrLastKeyframe = key == 0 || key == 1;
        if ((hasTransform && isFirstOrLastKeyframe) || currentKeyframe.containsProperty(CSSPropertyWebkitTransform))
            transformVector.insert(new TransformAnimationValue(key, &(keyframeStyle->transform()), tf));
        
        if ((hasOpacity && isFirstOrLastKeyframe) || currentKeyframe.containsProperty(CSSPropertyOpacity))
            opacityVector.insert(new FloatAnimationValue(key, keyframeStyle->opacity(), tf));

#if ENABLE(CSS_FILTERS)
        if ((hasFilter && isFirstOrLastKeyframe) || currentKeyframe.containsProperty(CSSPropertyWebkitFilter))
            filterVector.insert(new FilterAnimationValue(key, &(keyframeStyle->filter()), tf));
#endif
    }

    bool didAnimateTransform = false;
    bool didAnimateOpacity = false;
#if ENABLE(CSS_FILTERS)
    bool didAnimateFilter = false;
#endif
    
    if (hasTransform && m_graphicsLayer->addAnimation(transformVector, toRenderBox(renderer())->pixelSnappedBorderBoxRect().size(), anim, keyframes.animationName(), timeOffset))
        didAnimateTransform = true;

    if (hasOpacity && m_graphicsLayer->addAnimation(opacityVector, IntSize(), anim, keyframes.animationName(), timeOffset))
        didAnimateOpacity = true;

#if ENABLE(CSS_FILTERS)
    if (hasFilter && m_graphicsLayer->addAnimation(filterVector, IntSize(), anim, keyframes.animationName(), timeOffset))
        didAnimateFilter = true;
#endif

#if ENABLE(CSS_FILTERS)
    return didAnimateTransform || didAnimateOpacity || didAnimateFilter;
#else
    return didAnimateTransform || didAnimateOpacity;
#endif
}

void RenderLayerBacking::animationPaused(double timeOffset, const String& animationName)
{
    m_graphicsLayer->pauseAnimation(animationName, timeOffset);
}

void RenderLayerBacking::animationFinished(const String& animationName)
{
    m_graphicsLayer->removeAnimation(animationName);
}

bool RenderLayerBacking::startTransition(double timeOffset, CSSPropertyID property, const RenderStyle* fromStyle, const RenderStyle* toStyle)
{
    bool didAnimateOpacity = false;
    bool didAnimateTransform = false;
#if ENABLE(CSS_FILTERS)
    bool didAnimateFilter = false;
#endif

    ASSERT(property != CSSPropertyInvalid);

    if (property == CSSPropertyOpacity) {
        const Animation* opacityAnim = toStyle->transitionForProperty(CSSPropertyOpacity);
        if (opacityAnim && !opacityAnim->isEmptyOrZeroDuration()) {
            KeyframeValueList opacityVector(AnimatedPropertyOpacity);
            opacityVector.insert(new FloatAnimationValue(0, compositingOpacity(fromStyle->opacity())));
            opacityVector.insert(new FloatAnimationValue(1, compositingOpacity(toStyle->opacity())));
            // The boxSize param is only used for transform animations (which can only run on RenderBoxes), so we pass an empty size here.
            if (m_graphicsLayer->addAnimation(opacityVector, IntSize(), opacityAnim, GraphicsLayer::animationNameForTransition(AnimatedPropertyOpacity), timeOffset)) {
                // To ensure that the correct opacity is visible when the animation ends, also set the final opacity.
                updateOpacity(toStyle);
                didAnimateOpacity = true;
            }
        }
    }

    if (property == CSSPropertyWebkitTransform && m_owningLayer->hasTransform()) {
        const Animation* transformAnim = toStyle->transitionForProperty(CSSPropertyWebkitTransform);
        if (transformAnim && !transformAnim->isEmptyOrZeroDuration()) {
            KeyframeValueList transformVector(AnimatedPropertyWebkitTransform);
            transformVector.insert(new TransformAnimationValue(0, &fromStyle->transform()));
            transformVector.insert(new TransformAnimationValue(1, &toStyle->transform()));
            if (m_graphicsLayer->addAnimation(transformVector, toRenderBox(renderer())->pixelSnappedBorderBoxRect().size(), transformAnim, GraphicsLayer::animationNameForTransition(AnimatedPropertyWebkitTransform), timeOffset)) {
                // To ensure that the correct transform is visible when the animation ends, also set the final transform.
                updateTransform(toStyle);
                didAnimateTransform = true;
            }
        }
    }

#if ENABLE(CSS_FILTERS)
    if (property == CSSPropertyWebkitFilter && m_owningLayer->hasFilter()) {
        const Animation* filterAnim = toStyle->transitionForProperty(CSSPropertyWebkitFilter);
        if (filterAnim && !filterAnim->isEmptyOrZeroDuration()) {
            KeyframeValueList filterVector(AnimatedPropertyWebkitFilter);
            filterVector.insert(new FilterAnimationValue(0, &fromStyle->filter()));
            filterVector.insert(new FilterAnimationValue(1, &toStyle->filter()));
            if (m_graphicsLayer->addAnimation(filterVector, IntSize(), filterAnim, GraphicsLayer::animationNameForTransition(AnimatedPropertyWebkitFilter), timeOffset)) {
                // To ensure that the correct filter is visible when the animation ends, also set the final filter.
                updateFilters(toStyle);
                didAnimateFilter = true;
            }
        }
    }
#endif

#if ENABLE(CSS_FILTERS)
    return didAnimateOpacity || didAnimateTransform || didAnimateFilter;
#else
    return didAnimateOpacity || didAnimateTransform;
#endif
}

void RenderLayerBacking::transitionPaused(double timeOffset, CSSPropertyID property)
{
    AnimatedPropertyID animatedProperty = cssToGraphicsLayerProperty(property);
    if (animatedProperty != AnimatedPropertyInvalid)
        m_graphicsLayer->pauseAnimation(GraphicsLayer::animationNameForTransition(animatedProperty), timeOffset);
}

void RenderLayerBacking::transitionFinished(CSSPropertyID property)
{
    AnimatedPropertyID animatedProperty = cssToGraphicsLayerProperty(property);
    if (animatedProperty != AnimatedPropertyInvalid)
        m_graphicsLayer->removeAnimation(GraphicsLayer::animationNameForTransition(animatedProperty));
}

void RenderLayerBacking::notifyAnimationStarted(const GraphicsLayer*, double time)
{
    renderer()->animation()->notifyAnimationStarted(renderer(), time);
}

void RenderLayerBacking::notifySyncRequired(const GraphicsLayer*)
{
    if (!renderer()->documentBeingDestroyed())
        compositor()->scheduleLayerFlush();
}

// This is used for the 'freeze' API, for testing only.
void RenderLayerBacking::suspendAnimations(double time)
{
    m_graphicsLayer->suspendAnimations(time);
}

void RenderLayerBacking::resumeAnimations()
{
    m_graphicsLayer->resumeAnimations();
}

IntRect RenderLayerBacking::compositedBounds() const
{
    return m_compositedBounds;
}

void RenderLayerBacking::setCompositedBounds(const IntRect& bounds)
{
    m_compositedBounds = bounds;
}

CSSPropertyID RenderLayerBacking::graphicsLayerToCSSProperty(AnimatedPropertyID property)
{
    CSSPropertyID cssProperty = CSSPropertyInvalid;
    switch (property) {
        case AnimatedPropertyWebkitTransform:
            cssProperty = CSSPropertyWebkitTransform;
            break;
        case AnimatedPropertyOpacity:
            cssProperty = CSSPropertyOpacity;
            break;
        case AnimatedPropertyBackgroundColor:
            cssProperty = CSSPropertyBackgroundColor;
            break;
        case AnimatedPropertyWebkitFilter:
#if ENABLE(CSS_FILTERS)
            cssProperty = CSSPropertyWebkitFilter;
#else
            ASSERT_NOT_REACHED();
#endif
            break;
        case AnimatedPropertyInvalid:
            ASSERT_NOT_REACHED();
    }
    return cssProperty;
}

AnimatedPropertyID RenderLayerBacking::cssToGraphicsLayerProperty(CSSPropertyID cssProperty)
{
    switch (cssProperty) {
        case CSSPropertyWebkitTransform:
            return AnimatedPropertyWebkitTransform;
        case CSSPropertyOpacity:
            return AnimatedPropertyOpacity;
        case CSSPropertyBackgroundColor:
            return AnimatedPropertyBackgroundColor;
#if ENABLE(CSS_FILTERS)
        case CSSPropertyWebkitFilter:
            return AnimatedPropertyWebkitFilter;
#endif
        default:
            // It's fine if we see other css properties here; they are just not accelerated.
            break;
    }
    return AnimatedPropertyInvalid;
}

String RenderLayerBacking::nameForLayer() const
{
    StringBuilder name;
    name.append(renderer()->renderName());
    if (Node* node = renderer()->node()) {
        if (node->isElementNode()) {
            name.append(' ');
            name.append(static_cast<Element*>(node)->tagName());
        }
        if (node->hasID()) {
            name.appendLiteral(" id=\'");
            name.append(static_cast<Element*>(node)->getIdAttribute());
            name.append('\'');
        }

        if (node->hasClass()) {
            name.appendLiteral(" class=\'");
            StyledElement* styledElement = static_cast<StyledElement*>(node);
            for (size_t i = 0; i < styledElement->classNames().size(); ++i) {
                if (i > 0)
                    name.append(' ');
                name.append(styledElement->classNames()[i]);
            }
            name.append('\'');
        }
    }

    if (m_owningLayer->isReflection())
        name.appendLiteral(" (reflection)");

    return name.toString();
}

CompositingLayerType RenderLayerBacking::compositingLayerType() const
{
    if (m_graphicsLayer->hasContentsLayer())
        return MediaCompositingLayer;

    if (m_graphicsLayer->drawsContent())
        return m_graphicsLayer->usingTiledLayer() ? TiledCompositingLayer : NormalCompositingLayer;
    
    return ContainerCompositingLayer;
}

double RenderLayerBacking::backingStoreMemoryEstimate() const
{
    double backingMemory;
    
    // m_ancestorClippingLayer and m_containmentLayer are just used for masking or containment, so have no backing.
    backingMemory = m_graphicsLayer->backingStoreMemoryEstimate();
    if (m_foregroundLayer)
        backingMemory += m_foregroundLayer->backingStoreMemoryEstimate();
    if (m_maskLayer)
        backingMemory += m_maskLayer->backingStoreMemoryEstimate();

    if (m_scrollingContentsLayer)
        backingMemory += m_scrollingContentsLayer->backingStoreMemoryEstimate();

    if (m_layerForHorizontalScrollbar)
        backingMemory += m_layerForHorizontalScrollbar->backingStoreMemoryEstimate();

    if (m_layerForVerticalScrollbar)
        backingMemory += m_layerForVerticalScrollbar->backingStoreMemoryEstimate();

    if (m_layerForScrollCorner)
        backingMemory += m_layerForScrollCorner->backingStoreMemoryEstimate();
    
    return backingMemory;
}

#if PLATFORM(BLACKBERRY)
bool RenderLayerBacking::contentsVisible(const GraphicsLayer*, const IntRect& localContentRect) const
{
    Frame* frame = renderer()->frame();
    FrameView* view = frame ? frame->view() : 0;
    if (!view)
        return false;

    IntRect visibleContentRect(view->visibleContentRect());
    FloatQuad absoluteContentQuad = renderer()->localToAbsoluteQuad(FloatRect(localContentRect));
    return absoluteContentQuad.enclosingBoundingBox().intersects(visibleContentRect);
}
#endif

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)
