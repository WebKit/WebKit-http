/*
 * Copyright (C) 2009, 2014 Apple Inc. All rights reserved.
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

#ifndef GraphicsLayer_h
#define GraphicsLayer_h

#include "Animation.h"
#include "Color.h"
#include "FloatPoint.h"
#include "FloatPoint3D.h"
#include "FloatRect.h"
#include "FloatSize.h"
#include "GraphicsLayerClient.h"
#include "IntRect.h"
#include "PlatformLayer.h"
#include "TransformOperations.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

#if ENABLE(CSS_FILTERS)
#include "FilterOperations.h"
#endif

#if ENABLE(CSS_COMPOSITING)
#include "GraphicsTypes.h"
#endif

namespace WebCore {

enum LayerTreeAsTextBehaviorFlags {
    LayerTreeAsTextBehaviorNormal = 0,
    LayerTreeAsTextDebug = 1 << 0, // Dump extra debugging info like layer addresses.
    LayerTreeAsTextIncludeVisibleRects = 1 << 1,
    LayerTreeAsTextIncludeTileCaches = 1 << 2,
    LayerTreeAsTextIncludeRepaintRects = 1 << 3,
    LayerTreeAsTextIncludePaintingPhases = 1 << 4,
    LayerTreeAsTextIncludeContentLayers = 1 << 5
};
typedef unsigned LayerTreeAsTextBehavior;

class GraphicsContext;
class GraphicsLayerFactory;
class Image;
class TextStream;
class TiledBacking;
class TimingFunction;
class TransformationMatrix;

// Base class for animation values (also used for transitions). Here to
// represent values for properties being animated via the GraphicsLayer,
// without pulling in style-related data from outside of the platform directory.
// FIXME: Should be moved to its own header file.
class AnimationValue {
    WTF_MAKE_FAST_ALLOCATED;
public:
    virtual ~AnimationValue() { }

    double keyTime() const { return m_keyTime; }
    const TimingFunction* timingFunction() const { return m_timingFunction.get(); }
    virtual PassOwnPtr<AnimationValue> clone() const = 0;

protected:
    AnimationValue(double keyTime, TimingFunction* timingFunction = nullptr)
        : m_keyTime(keyTime)
        , m_timingFunction(timingFunction)
    {
    }

private:
    double m_keyTime;
    RefPtr<TimingFunction> m_timingFunction;
};

// Used to store one float value of an animation.
// FIXME: Should be moved to its own header file.
class FloatAnimationValue : public AnimationValue {
public:
    static PassOwnPtr<FloatAnimationValue> create(double keyTime, float value, TimingFunction* timingFunction = nullptr)
    {
        return adoptPtr(new FloatAnimationValue(keyTime, value, timingFunction));
    }

    virtual PassOwnPtr<AnimationValue> clone() const override
    {
        return adoptPtr(new FloatAnimationValue(*this));
    }

    float value() const { return m_value; }

private:
    FloatAnimationValue(double keyTime, float value, TimingFunction* timingFunction)
        : AnimationValue(keyTime, timingFunction)
        , m_value(value)
    {
    }

    float m_value;
};

// Used to store one transform value in a keyframe list.
// FIXME: Should be moved to its own header file.
class TransformAnimationValue : public AnimationValue {
public:
    static PassOwnPtr<TransformAnimationValue> create(double keyTime, const TransformOperations& value, TimingFunction* timingFunction = nullptr)
    {
        return adoptPtr(new TransformAnimationValue(keyTime, value, timingFunction));
    }

    virtual PassOwnPtr<AnimationValue> clone() const override
    {
        return adoptPtr(new TransformAnimationValue(*this));
    }

    const TransformOperations& value() const { return m_value; }

private:
    TransformAnimationValue(double keyTime, const TransformOperations& value, TimingFunction* timingFunction)
        : AnimationValue(keyTime, timingFunction)
        , m_value(value)
    {
    }

    TransformOperations m_value;
};

#if ENABLE(CSS_FILTERS)
// Used to store one filter value in a keyframe list.
// FIXME: Should be moved to its own header file.
class FilterAnimationValue : public AnimationValue {
public:
    static PassOwnPtr<FilterAnimationValue> create(double keyTime, const FilterOperations& value, TimingFunction* timingFunction = nullptr)
    {
        return adoptPtr(new FilterAnimationValue(keyTime, value, timingFunction));
    }

    virtual PassOwnPtr<AnimationValue> clone() const override
    {
        return adoptPtr(new FilterAnimationValue(*this));
    }

    const FilterOperations& value() const { return m_value; }

private:
    FilterAnimationValue(double keyTime, const FilterOperations& value, TimingFunction* timingFunction)
        : AnimationValue(keyTime, timingFunction)
        , m_value(value)
    {
    }

    FilterOperations m_value;
};
#endif

// Used to store a series of values in a keyframe list.
// Values will all be of the same type, which can be inferred from the property.
// FIXME: Should be moved to its own header file.
class KeyframeValueList {
public:
    explicit KeyframeValueList(AnimatedPropertyID property)
        : m_property(property)
    {
    }

    KeyframeValueList(const KeyframeValueList& other)
        : m_property(other.property())
    {
        for (size_t i = 0; i < other.m_values.size(); ++i)
            m_values.append(other.m_values[i]->clone());
    }

    ~KeyframeValueList()
    {
    }

    KeyframeValueList& operator=(const KeyframeValueList& other)
    {
        KeyframeValueList copy(other);
        swap(copy);
        return *this;
    }

    void swap(KeyframeValueList& other)
    {
        std::swap(m_property, other.m_property);
        m_values.swap(other.m_values);
    }

    AnimatedPropertyID property() const { return m_property; }

    size_t size() const { return m_values.size(); }
    const AnimationValue& at(size_t i) const { return *m_values.at(i); }
    
    // Insert, sorted by keyTime.
    void insert(PassOwnPtr<const AnimationValue>);
    
protected:
    Vector<OwnPtr<const AnimationValue>> m_values;
    AnimatedPropertyID m_property;
};

// GraphicsLayer is an abstraction for a rendering surface with backing store,
// which may have associated transformation and animations.

class GraphicsLayer {
    WTF_MAKE_NONCOPYABLE(GraphicsLayer); WTF_MAKE_FAST_ALLOCATED;
public:
    static std::unique_ptr<GraphicsLayer> create(GraphicsLayerFactory*, GraphicsLayerClient&);
    
    virtual ~GraphicsLayer();

    virtual void initialize() { }

    typedef uint64_t PlatformLayerID;
    virtual PlatformLayerID primaryLayerID() const { return 0; }

    GraphicsLayerClient& client() const { return m_client; }

    // Layer name. Only used to identify layers in debug output
    const String& name() const { return m_name; }
    virtual void setName(const String& name) { m_name = name; }

    GraphicsLayer* parent() const { return m_parent; };
    void setParent(GraphicsLayer*); // Internal use only.
    
    // Returns true if the layer has the given layer as an ancestor (excluding self).
    bool hasAncestor(GraphicsLayer*) const;
    
    const Vector<GraphicsLayer*>& children() const { return m_children; }
    // Returns true if the child list changed.
    virtual bool setChildren(const Vector<GraphicsLayer*>&);

    // Add child layers. If the child is already parented, it will be removed from its old parent.
    virtual void addChild(GraphicsLayer*);
    virtual void addChildAtIndex(GraphicsLayer*, int index);
    virtual void addChildAbove(GraphicsLayer* layer, GraphicsLayer* sibling);
    virtual void addChildBelow(GraphicsLayer* layer, GraphicsLayer* sibling);
    virtual bool replaceChild(GraphicsLayer* oldChild, GraphicsLayer* newChild);

    void removeAllChildren();
    virtual void removeFromParent();

    // The parent() of a maskLayer is set to the layer being masked.
    GraphicsLayer* maskLayer() const { return m_maskLayer; }
    virtual void setMaskLayer(GraphicsLayer*);

    void setIsMaskLayer(bool isMask) { m_isMaskLayer = isMask; }
    bool isMaskLayer() const { return m_isMaskLayer; }
    
    // The given layer will replicate this layer and its children; the replica renders behind this layer.
    virtual void setReplicatedByLayer(GraphicsLayer*);
    // Whether this layer is being replicated by another layer.
    bool isReplicated() const { return m_replicaLayer; }
    // The layer that replicates this layer (if any).
    GraphicsLayer* replicaLayer() const { return m_replicaLayer; }

    const FloatPoint& replicatedLayerPosition() const { return m_replicatedLayerPosition; }
    void setReplicatedLayerPosition(const FloatPoint& p) { m_replicatedLayerPosition = p; }

    enum ShouldSetNeedsDisplay {
        DontSetNeedsDisplay,
        SetNeedsDisplay
    };

    // Offset is origin of the renderer minus origin of the graphics layer.
    FloatSize offsetFromRenderer() const { return m_offsetFromRenderer; }
    void setOffsetFromRenderer(const FloatSize&, ShouldSetNeedsDisplay = SetNeedsDisplay);

    // The position of the layer (the location of its top-left corner in its parent)
    const FloatPoint& position() const { return m_position; }
    virtual void setPosition(const FloatPoint& p) { m_position = p; }

    // For platforms that move underlying platform layers on a different thread for scrolling; just update the GraphicsLayer state.
    virtual void syncPosition(const FloatPoint& p) { m_position = p; }

    // Anchor point: (0, 0) is top left, (1, 1) is bottom right. The anchor point
    // affects the origin of the transforms.
    const FloatPoint3D& anchorPoint() const { return m_anchorPoint; }
    virtual void setAnchorPoint(const FloatPoint3D& p) { m_anchorPoint = p; }

    // The size of the layer.
    const FloatSize& size() const { return m_size; }
    virtual void setSize(const FloatSize&);

    // The boundOrigin affects the offset at which content is rendered, and sublayers are positioned.
    const FloatPoint& boundsOrigin() const { return m_boundsOrigin; }
    virtual void setBoundsOrigin(const FloatPoint& origin) { m_boundsOrigin = origin; }

    // For platforms that move underlying platform layers on a different thread for scrolling; just update the GraphicsLayer state.
    virtual void syncBoundsOrigin(const FloatPoint& origin) { m_boundsOrigin = origin; }

    const TransformationMatrix& transform() const { return m_transform; }
    virtual void setTransform(const TransformationMatrix& t) { m_transform = t; }

    const TransformationMatrix& childrenTransform() const { return m_childrenTransform; }
    virtual void setChildrenTransform(const TransformationMatrix& t) { m_childrenTransform = t; }

    bool preserves3D() const { return m_preserves3D; }
    virtual void setPreserves3D(bool b) { m_preserves3D = b; }
    
    bool masksToBounds() const { return m_masksToBounds; }
    virtual void setMasksToBounds(bool b) { m_masksToBounds = b; }
    
    bool drawsContent() const { return m_drawsContent; }
    virtual void setDrawsContent(bool b) { m_drawsContent = b; }

    bool contentsAreVisible() const { return m_contentsVisible; }
    virtual void setContentsVisible(bool b) { m_contentsVisible = b; }

    bool acceleratesDrawing() const { return m_acceleratesDrawing; }
    virtual void setAcceleratesDrawing(bool b) { m_acceleratesDrawing = b; }

    // The color used to paint the layer background. Pass an invalid color to remove it.
    // Note that this covers the entire layer. Use setContentsToSolidColor() if the color should
    // only cover the contentsRect.
    const Color& backgroundColor() const { return m_backgroundColor; }
    virtual void setBackgroundColor(const Color&);

    // opaque means that we know the layer contents have no alpha
    bool contentsOpaque() const { return m_contentsOpaque; }
    virtual void setContentsOpaque(bool b) { m_contentsOpaque = b; }

    bool backfaceVisibility() const { return m_backfaceVisibility; }
    virtual void setBackfaceVisibility(bool b) { m_backfaceVisibility = b; }

    float opacity() const { return m_opacity; }
    virtual void setOpacity(float opacity) { m_opacity = opacity; }

#if ENABLE(CSS_FILTERS)
    const FilterOperations& filters() const { return m_filters; }
    
    // Returns true if filter can be rendered by the compositor
    virtual bool setFilters(const FilterOperations& filters) { m_filters = filters; return true; }
#endif

#if ENABLE(CSS_COMPOSITING)
    BlendMode blendMode() const { return m_blendMode; }
    virtual void setBlendMode(BlendMode blendMode) { m_blendMode = blendMode; }
#endif

    // Some GraphicsLayers paint only the foreground or the background content
    GraphicsLayerPaintingPhase paintingPhase() const { return m_paintingPhase; }
    void setPaintingPhase(GraphicsLayerPaintingPhase phase) { m_paintingPhase = phase; }

    enum ShouldClipToLayer {
        DoNotClipToLayer,
        ClipToLayer
    };

    virtual void setNeedsDisplay() = 0;
    // mark the given rect (in layer coords) as needing dispay. Never goes deep.
    virtual void setNeedsDisplayInRect(const FloatRect&, ShouldClipToLayer = ClipToLayer) = 0;

    virtual void setContentsNeedsDisplay() { };

    // The tile phase is relative to the GraphicsLayer bounds.
    virtual void setContentsTilePhase(const FloatPoint& p) { m_contentsTilePhase = p; }
    FloatPoint contentsTilePhase() const { return m_contentsTilePhase; }

    virtual void setContentsTileSize(const FloatSize& s) { m_contentsTileSize = s; }
    FloatSize contentsTileSize() const { return m_contentsTileSize; }
    bool hasContentsTiling() const { return !m_contentsTileSize.isEmpty(); }

    // Set that the position/size of the contents (image or video).
    FloatRect contentsRect() const { return m_contentsRect; }
    virtual void setContentsRect(const FloatRect& r) { m_contentsRect = r; }

    FloatRect contentsClippingRect() const { return m_contentsClippingRect; }
    virtual void setContentsClippingRect(const FloatRect& r) { m_contentsClippingRect = r; }

    // Transitions are identified by a special animation name that cannot clash with a keyframe identifier.
    static String animationNameForTransition(AnimatedPropertyID);
    
    // Return true if the animation is handled by the compositing system. If this returns
    // false, the animation will be run by AnimationController.
    // These methods handle both transitions and keyframe animations.
    virtual bool addAnimation(const KeyframeValueList&, const FloatSize& /*boxSize*/, const Animation*, const String& /*animationName*/, double /*timeOffset*/)  { return false; }
    virtual void pauseAnimation(const String& /*animationName*/, double /*timeOffset*/) { }
    virtual void removeAnimation(const String& /*animationName*/) { }

    virtual void suspendAnimations(double time);
    virtual void resumeAnimations();
    
    // Layer contents
    virtual void setContentsToImage(Image*) { }
    virtual bool shouldDirectlyCompositeImage(Image*) const { return true; }
    virtual void setContentsToMedia(PlatformLayer*) { } // video or plug-in
#if PLATFORM(IOS)
    virtual PlatformLayer* contentsLayerForMedia() const { return 0; }
#endif
    // Pass an invalid color to remove the contents layer.
    virtual void setContentsToSolidColor(const Color&) { }
    virtual void setContentsToCanvas(PlatformLayer*) { }
    // FIXME: webkit.org/b/109658
    // Should unify setContentsToMedia and setContentsToCanvas
    virtual void setContentsToPlatformLayer(PlatformLayer* layer) { setContentsToMedia(layer); }
    virtual bool usesContentsLayer() const { return false; }

    // Callback from the underlying graphics system to draw layer contents.
    void paintGraphicsLayerContents(GraphicsContext&, const FloatRect& clip);
    
    // For hosting this GraphicsLayer in a native layer hierarchy.
    virtual PlatformLayer* platformLayer() const { return 0; }
    
    enum CompositingCoordinatesOrientation { CompositingCoordinatesTopDown, CompositingCoordinatesBottomUp };

    // Flippedness of the contents of this layer. Does not affect sublayer geometry.
    virtual void setContentsOrientation(CompositingCoordinatesOrientation orientation) { m_contentsOrientation = orientation; }
    CompositingCoordinatesOrientation contentsOrientation() const { return m_contentsOrientation; }

    void dumpLayer(TextStream&, int indent = 0, LayerTreeAsTextBehavior = LayerTreeAsTextBehaviorNormal) const;

    virtual void setShowDebugBorder(bool show) { m_showDebugBorder = show; }
    bool isShowingDebugBorder() const { return m_showDebugBorder; }

    virtual void setShowRepaintCounter(bool show) { m_showRepaintCounter = show; }
    bool isShowingRepaintCounter() const { return m_showRepaintCounter; }

    // FIXME: this is really a paint count.
    int repaintCount() const { return m_repaintCount; }
    int incrementRepaintCount() { return ++m_repaintCount; }

    virtual void setDebugBackgroundColor(const Color&) { }
    virtual void setDebugBorder(const Color&, float /*borderWidth*/) { }

    enum CustomAppearance { NoCustomAppearance, ScrollingOverhang, ScrollingShadow };
    virtual void setCustomAppearance(CustomAppearance customAppearance) { m_customAppearance = customAppearance; }
    CustomAppearance customAppearance() const { return m_customAppearance; }

    enum CustomBehavior { NoCustomBehavior, CustomScrollingBehavior, CustomScrolledContentsBehavior };
    virtual void setCustomBehavior(CustomBehavior customBehavior) { m_customBehavior = customBehavior; }
    CustomBehavior customBehavior() const { return m_customBehavior; }

    // z-position is the z-equivalent of position(). It's only used for debugging purposes.
    virtual float zPosition() const { return m_zPosition; }
    virtual void setZPosition(float);

    virtual void distributeOpacity(float);
    virtual float accumulatedOpacity() const;

#if PLATFORM(IOS)
    bool hasFlattenedPerspectiveTransform() const { return !preserves3D() && m_childrenTransform.hasPerspective(); }
#endif
    virtual FloatSize pixelAlignmentOffset() const { return FloatSize(); }
    
    virtual void setAppliesPageScale(bool appliesScale = true) { m_appliesPageScale = appliesScale; }
    virtual bool appliesPageScale() const { return m_appliesPageScale; }

    float pageScaleFactor() const { return m_client.pageScaleFactor(); }
    float deviceScaleFactor() const { return m_client.deviceScaleFactor(); }

    virtual void deviceOrPageScaleFactorChanged() { }
    void noteDeviceOrPageScaleFactorChangedIncludingDescendants();

    // Some compositing systems may do internal batching to synchronize compositing updates
    // with updates drawn into the window. These methods flush internal batched state on this layer
    // and descendant layers, and this layer only.
    virtual void flushCompositingState(const FloatRect& /* clipRect */) { }
    virtual void flushCompositingStateForThisLayerOnly() { }

    // If the exposed rect of this layer changes, returns true if this or descendant layers need a flush,
    // for example to allocate new tiles.
    virtual bool visibleRectChangeRequiresFlush(const FloatRect& /* clipRect */) const { return false; }

    // Return a string with a human readable form of the layer tree, If debug is true
    // pointers for the layers and timing data will be included in the returned string.
    String layerTreeAsText(LayerTreeAsTextBehavior = LayerTreeAsTextBehaviorNormal) const;

    // Return an estimate of the backing store memory cost (in bytes). May be incorrect for tiled layers.
    virtual double backingStoreMemoryEstimate() const;

    bool usingTiledBacking() const { return m_usingTiledBacking; }
    virtual TiledBacking* tiledBacking() const { return 0; }

    void resetTrackedRepaints();
    void addRepaintRect(const FloatRect&);

    static bool supportsBackgroundColorContent()
    {
#if USE(CA) || USE(TEXTURE_MAPPER)
        return true;
#else
        return false;
#endif
    }

#if USE(COORDINATED_GRAPHICS)
    static bool supportsContentsTiling();
#else
    static bool supportsContentsTiling()
    {
        // FIXME: Enable the feature on different ports.
        return false;
    }
#endif

    void updateDebugIndicators();

    virtual bool canThrottleLayerFlush() const { return false; }

    virtual bool isGraphicsLayerCA() const { return false; }
    virtual bool isGraphicsLayerCARemote() const { return false; }

protected:
    // Should be called from derived class destructors. Should call willBeDestroyed() on super.
    virtual void willBeDestroyed();

#if ENABLE(CSS_FILTERS)
    // This method is used by platform GraphicsLayer classes to clear the filters
    // when compositing is not done in hardware. It is not virtual, so the caller
    // needs to notifiy the change to the platform layer as needed.
    void clearFilters() { m_filters.clear(); }

    // Given a KeyframeValueList containing filterOperations, return true if the operations are valid.
    static int validateFilterOperations(const KeyframeValueList&);
#endif

    // Given a list of TransformAnimationValues, see if all the operations for each keyframe match. If so
    // return the index of the KeyframeValueList entry that has that list of operations (it may not be
    // the first entry because some keyframes might have an empty transform and those match any list).
    // If the lists don't match return -1. On return, if hasBigRotation is true, functions contain 
    // rotations of >= 180 degrees
    static int validateTransformOperations(const KeyframeValueList&, bool& hasBigRotation);

    virtual bool shouldRepaintOnSizeChange() const { return drawsContent(); }

    virtual void setOpacityInternal(float) { }

    // The layer being replicated.
    GraphicsLayer* replicatedLayer() const { return m_replicatedLayer; }
    virtual void setReplicatedLayer(GraphicsLayer* layer) { m_replicatedLayer = layer; }

    explicit GraphicsLayer(GraphicsLayerClient&);

    void dumpProperties(TextStream&, int indent, LayerTreeAsTextBehavior) const;
    virtual void dumpAdditionalProperties(TextStream&, int /*indent*/, LayerTreeAsTextBehavior) const { }

    virtual void getDebugBorderInfo(Color&, float& width) const;

    GraphicsLayerClient& m_client;
    String m_name;
    
    // Offset from the owning renderer
    FloatSize m_offsetFromRenderer;
    
    // Position is relative to the parent GraphicsLayer
    FloatPoint m_position;
    FloatPoint3D m_anchorPoint;
    FloatSize m_size;
    FloatPoint m_boundsOrigin;

    TransformationMatrix m_transform;
    TransformationMatrix m_childrenTransform;

    Color m_backgroundColor;
    float m_opacity;
    float m_zPosition;
    
#if ENABLE(CSS_FILTERS)
    FilterOperations m_filters;
#endif

#if ENABLE(CSS_COMPOSITING)
    BlendMode m_blendMode;
#endif

    bool m_contentsOpaque : 1;
    bool m_preserves3D: 1;
    bool m_backfaceVisibility : 1;
    bool m_usingTiledBacking : 1;
    bool m_masksToBounds : 1;
    bool m_drawsContent : 1;
    bool m_contentsVisible : 1;
    bool m_acceleratesDrawing : 1;
    bool m_appliesPageScale : 1; // Set for the layer which has the page scale applied to it.
    bool m_showDebugBorder : 1;
    bool m_showRepaintCounter : 1;
    bool m_isMaskLayer : 1;
    
    GraphicsLayerPaintingPhase m_paintingPhase;
    CompositingCoordinatesOrientation m_contentsOrientation; // affects orientation of layer contents

    Vector<GraphicsLayer*> m_children;
    GraphicsLayer* m_parent;

    GraphicsLayer* m_maskLayer; // Reference to mask layer. We don't own this.

    GraphicsLayer* m_replicaLayer; // A layer that replicates this layer. We only allow one, for now.
                                   // The replica is not parented; this is the primary reference to it.
    GraphicsLayer* m_replicatedLayer; // For a replica layer, a reference to the original layer.
    FloatPoint m_replicatedLayerPosition; // For a replica layer, the position of the replica.

    FloatRect m_contentsRect;
    FloatRect m_contentsClippingRect;
    FloatPoint m_contentsTilePhase;
    FloatSize m_contentsTileSize;

    int m_repaintCount;
    CustomAppearance m_customAppearance;
    CustomBehavior m_customBehavior;
};

#define GRAPHICSLAYER_TYPE_CASTS(ToValueTypeName, predicate) \
    TYPE_CASTS_BASE(ToValueTypeName, WebCore::GraphicsLayer, value, value->predicate, value.predicate)

} // namespace WebCore

#ifndef NDEBUG
// Outside the WebCore namespace for ease of invocation from gdb.
void showGraphicsLayerTree(const WebCore::GraphicsLayer* layer);
#endif

#endif // GraphicsLayer_h
