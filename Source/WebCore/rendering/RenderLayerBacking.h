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

#ifndef RenderLayerBacking_h
#define RenderLayerBacking_h

#if USE(ACCELERATED_COMPOSITING)

#include "FloatPoint.h"
#include "FloatPoint3D.h"
#include "GraphicsLayer.h"
#include "GraphicsLayerClient.h"
#include "RenderLayer.h"
#include "TransformationMatrix.h"

namespace WebCore {

class KeyframeList;
class RenderLayerCompositor;

enum CompositingLayerType {
    NormalCompositingLayer, // non-tiled layer with backing store
    TiledCompositingLayer, // tiled layer (always has backing store)
    MediaCompositingLayer, // layer that contains an image, video, webGL or plugin
    ContainerCompositingLayer // layer with no backing store
};

// RenderLayerBacking controls the compositing behavior for a single RenderLayer.
// It holds the various GraphicsLayers, and makes decisions about intra-layer rendering
// optimizations.
// 
// There is one RenderLayerBacking for each RenderLayer that is composited.

class RenderLayerBacking : public GraphicsLayerClient {
    WTF_MAKE_NONCOPYABLE(RenderLayerBacking); WTF_MAKE_FAST_ALLOCATED;
public:
    RenderLayerBacking(RenderLayer*);
    ~RenderLayerBacking();

    RenderLayer* owningLayer() const { return m_owningLayer; }

    enum UpdateDepth { CompositingChildren, AllDescendants };
    void updateAfterLayout(UpdateDepth, bool isUpdateRoot);
    
    // Returns true if layer configuration changed.
    bool updateGraphicsLayerConfiguration();
    // Update graphics layer position and bounds.
    void updateGraphicsLayerGeometry(); // make private
    // Update contents and clipping structure.
    void updateDrawsContent();
    
    GraphicsLayer* graphicsLayer() const { return m_graphicsLayer.get(); }

    // Layer to clip children
    bool hasClippingLayer() const { return (m_containmentLayer && !m_usingTiledCacheLayer); }
    GraphicsLayer* clippingLayer() const { return !m_usingTiledCacheLayer ? m_containmentLayer.get() : 0; }

    // Layer to get clipped by ancestor
    bool hasAncestorClippingLayer() const { return m_ancestorClippingLayer != 0; }
    GraphicsLayer* ancestorClippingLayer() const { return m_ancestorClippingLayer.get(); }

    bool hasContentsLayer() const { return m_foregroundLayer != 0; }
    GraphicsLayer* foregroundLayer() const { return m_foregroundLayer.get(); }

    bool hasScrollingLayer() const { return m_scrollingLayer; }
    GraphicsLayer* scrollingLayer() const { return m_scrollingLayer.get(); }
    GraphicsLayer* scrollingContentsLayer() const { return m_scrollingContentsLayer.get(); }
    
    bool hasMaskLayer() const { return m_maskLayer != 0; }

    GraphicsLayer* parentForSublayers() const;
    GraphicsLayer* childForSuperlayers() const { return m_ancestorClippingLayer ? m_ancestorClippingLayer.get() : m_graphicsLayer.get(); }

    // RenderLayers with backing normally short-circuit paintLayer() because
    // their content is rendered via callbacks from GraphicsLayer. However, the document
    // layer is special, because it has a GraphicsLayer to act as a container for the GraphicsLayers
    // for descendants, but its contents usually render into the window (in which case this returns true).
    // This returns false for other layers, and when the document layer actually needs to paint into its backing store
    // for some reason.
    bool paintsIntoWindow() const;
    
    // Returns true for a composited layer that has no backing store of its own, so
    // paints into some ancestor layer.
    bool paintsIntoCompositedAncestor() const { return !m_requiresOwnBackingStore; }

    void setRequiresOwnBackingStore(bool);

    void setContentsNeedDisplay();
    // r is in the coordinate space of the layer's render object
    void setContentsNeedDisplayInRect(const IntRect&);

    // Notification from the renderer that its content changed.
    void contentChanged(ContentChangeType);

    // Interface to start, finish, suspend and resume animations and transitions
    bool startTransition(double, CSSPropertyID, const RenderStyle* fromStyle, const RenderStyle* toStyle);
    void transitionPaused(double timeOffset, CSSPropertyID);
    void transitionFinished(CSSPropertyID);

    bool startAnimation(double timeOffset, const Animation* anim, const KeyframeList& keyframes);
    void animationPaused(double timeOffset, const String& name);
    void animationFinished(const String& name);

    void suspendAnimations(double time = 0);
    void resumeAnimations();

    IntRect compositedBounds() const;
    void setCompositedBounds(const IntRect&);
    void updateCompositedBounds();
    
    void updateAfterWidgetResize();
    void positionOverflowControlsLayers(const IntSize& offsetFromRoot);

    // GraphicsLayerClient interface
    virtual bool shouldUseTileCache(const GraphicsLayer*) const;
    virtual bool usingTileCache(const GraphicsLayer*) const { return m_usingTiledCacheLayer; }
    virtual void notifyAnimationStarted(const GraphicsLayer*, double startTime);
    virtual void notifySyncRequired(const GraphicsLayer*);

    virtual void paintContents(const GraphicsLayer*, GraphicsContext&, GraphicsLayerPaintingPhase, const IntRect& clip);

    virtual float deviceScaleFactor() const;
    virtual float pageScaleFactor() const;
    virtual void didCommitChangesForLayer(const GraphicsLayer*) const;

    virtual bool showDebugBorders(const GraphicsLayer*) const;
    virtual bool showRepaintCounter(const GraphicsLayer*) const;

#ifndef NDEBUG
    virtual void verifyNotPainting();
#endif

    IntRect contentsBox() const;
    
    // For informative purposes only.
    CompositingLayerType compositingLayerType() const;
    
    GraphicsLayer* layerForHorizontalScrollbar() const { return m_layerForHorizontalScrollbar.get(); }
    GraphicsLayer* layerForVerticalScrollbar() const { return m_layerForVerticalScrollbar.get(); }
    GraphicsLayer* layerForScrollCorner() const { return m_layerForScrollCorner.get(); }

#if ENABLE(CSS_FILTERS)
    bool canCompositeFilters() const { return m_canCompositeFilters; }
#endif

    // Return an estimate of the backing store area (in pixels) allocated by this object's GraphicsLayers.
    double backingStoreMemoryEstimate() const;

    String nameForLayer() const;
    
#if ENABLE(CSS_COMPOSITING)
    void setBlendMode(BlendMode);
#endif

private:
    void createPrimaryGraphicsLayer();
    void destroyGraphicsLayers();
    
    PassOwnPtr<GraphicsLayer> createGraphicsLayer(const String&);

    RenderBoxModelObject* renderer() const { return m_owningLayer->renderer(); }
    RenderLayerCompositor* compositor() const { return m_owningLayer->compositor(); }

    void updateInternalHierarchy();
    bool updateClippingLayers(bool needsAncestorClip, bool needsDescendantClip);
    bool updateOverflowControlsLayers(bool needsHorizontalScrollbarLayer, bool needsVerticalScrollbarLayer, bool needsScrollCornerLayer);
    bool updateForegroundLayer(bool needsForegroundLayer);
    bool updateMaskLayer(bool needsMaskLayer);
    bool requiresHorizontalScrollbarLayer() const;
    bool requiresVerticalScrollbarLayer() const;
    bool requiresScrollCornerLayer() const;
    bool updateScrollingLayers(bool scrollingLayers);

    GraphicsLayerPaintingPhase paintingPhaseForPrimaryLayer() const;
    
    IntSize contentOffsetInCompostingLayer() const;
    // Result is transform origin in pixels.
    FloatPoint3D computeTransformOrigin(const IntRect& borderBox) const;
    // Result is perspective origin in pixels.
    FloatPoint computePerspectiveOrigin(const IntRect& borderBox) const;

    void updateOpacity(const RenderStyle*);
    void updateTransform(const RenderStyle*);
#if ENABLE(CSS_FILTERS)
    void updateFilters(const RenderStyle*);
#endif
#if ENABLE(CSS_COMPOSITING)
    void updateLayerBlendMode(const RenderStyle*);
#endif
    // Return the opacity value that this layer should use for compositing.
    float compositingOpacity(float rendererOpacity) const;
    
    bool isMainFrameRenderViewLayer() const;
    
    bool paintsBoxDecorations() const;
    bool paintsChildren() const;

    // Returns true if this compositing layer has no visible content.
    bool isSimpleContainerCompositingLayer() const;
    // Returns true if this layer has content that needs to be rendered by painting into the backing store.
    bool containsPaintedContent() const;
    // Returns true if the RenderLayer just contains an image that we can composite directly.
    bool isDirectlyCompositedImage() const;
    void updateImageContents();

    Color rendererBackgroundColor() const;
    void updateBackgroundColor();

    bool containsNonEmptyRenderers() const;
    bool hasVisibleNonCompositingDescendantLayers() const;

    bool shouldClipCompositedBounds() const;

    bool hasTileCacheFlatteningLayer() const { return (m_containmentLayer && m_usingTiledCacheLayer); }
    GraphicsLayer* tileCacheFlatteningLayer() const { return m_usingTiledCacheLayer ? m_containmentLayer.get() : 0; }

    void paintIntoLayer(RenderLayer* rootLayer, GraphicsContext*, const IntRect& paintDirtyRect, PaintBehavior, GraphicsLayerPaintingPhase, RenderObject* paintingRoot);

    static CSSPropertyID graphicsLayerToCSSProperty(AnimatedPropertyID);
    static AnimatedPropertyID cssToGraphicsLayerProperty(CSSPropertyID);

    RenderLayer* m_owningLayer;

    OwnPtr<GraphicsLayer> m_ancestorClippingLayer; // only used if we are clipped by an ancestor which is not a stacking context
    OwnPtr<GraphicsLayer> m_graphicsLayer;
    OwnPtr<GraphicsLayer> m_foregroundLayer;       // only used in cases where we need to draw the foreground separately
    OwnPtr<GraphicsLayer> m_containmentLayer; // Only used if we have clipping on a stacking context with compositing children, or if the layer has a tile cache.
    OwnPtr<GraphicsLayer> m_maskLayer;             // only used if we have a mask

    OwnPtr<GraphicsLayer> m_layerForHorizontalScrollbar;
    OwnPtr<GraphicsLayer> m_layerForVerticalScrollbar;
    OwnPtr<GraphicsLayer> m_layerForScrollCorner;

    OwnPtr<GraphicsLayer> m_scrollingLayer; // only used if the layer is using composited scrolling.
    OwnPtr<GraphicsLayer> m_scrollingContentsLayer; // only used if the layer is using composited scrolling.

    IntRect m_compositedBounds;

    bool m_artificiallyInflatedBounds;      // bounds had to be made non-zero to make transform-origin work
    bool m_isMainFrameRenderViewLayer;
    bool m_usingTiledCacheLayer;
    bool m_requiresOwnBackingStore;
#if ENABLE(CSS_FILTERS)
    bool m_canCompositeFilters;
#endif

    static bool m_creatingPrimaryGraphicsLayer;
};

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)

#endif // RenderLayerBacking_h
