/*
 * Copyright (C) 2003, 2009 Apple Inc. All rights reserved.
 *
 * Portions are Copyright (C) 1998 Netscape Communications Corporation.
 *
 * Other contributors:
 *   Robert O'Callahan <roc+@cs.cmu.edu>
 *   David Baron <dbaron@fas.harvard.edu>
 *   Christian Biesinger <cbiesinger@web.de>
 *   Randall Jesup <rjesup@wgate.com>
 *   Roland Mainz <roland.mainz@informatik.med.uni-giessen.de>
 *   Josh Soref <timeless@mac.com>
 *   Boris Zbarsky <bzbarsky@mit.edu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Alternatively, the contents of this file may be used under the terms
 * of either the Mozilla Public License Version 1.1, found at
 * http://www.mozilla.org/MPL/ (the "MPL") or the GNU General Public
 * License Version 2.0, found at http://www.fsf.org/copyleft/gpl.html
 * (the "GPL"), in which case the provisions of the MPL or the GPL are
 * applicable instead of those above.  If you wish to allow use of your
 * version of this file only under the terms of one of those two
 * licenses (the MPL or the GPL) and not to allow others to use your
 * version of this file under the LGPL, indicate your decision by
 * deletingthe provisions above and replace them with the notice and
 * other provisions required by the MPL or the GPL, as the case may be.
 * If you do not delete the provisions above, a recipient may use your
 * version of this file under any of the LGPL, the MPL or the GPL.
 */

#ifndef RenderLayer_h
#define RenderLayer_h

#include "PaintInfo.h"
#include "RenderBox.h"
#include "ScrollableArea.h"
#include <wtf/OwnPtr.h>

#if ENABLE(CSS_FILTERS)
#include "RenderLayerFilterInfo.h"
#endif

namespace WebCore {

#if ENABLE(CSS_FILTERS)
class FilterEffectRenderer;
#endif
class HitTestRequest;
class HitTestResult;
class HitTestingTransformState;
class RenderMarquee;
class RenderReplica;
class RenderScrollbarPart;
class RenderStyle;
class RenderView;
class Scrollbar;
class TransformationMatrix;

#if USE(ACCELERATED_COMPOSITING)
class RenderLayerBacking;
class RenderLayerCompositor;
#endif

enum BorderRadiusClippingRule { IncludeSelfForBorderRadius, DoNotIncludeSelfForBorderRadius };

enum RepaintStatus {
    NeedsNormalRepaint = 0,
    NeedsFullRepaint = 1 << 0,
    NeedsFullRepaintForPositionedMovementLayout = 1 << 1
};

class ClipRect {
public:
    ClipRect()
    : m_hasRadius(false)
    { }
    
    ClipRect(const LayoutRect& rect)
    : m_rect(rect)
    , m_hasRadius(false)
    { }
    
    const LayoutRect& rect() const { return m_rect; }
    void setRect(const LayoutRect& rect) { m_rect = rect; }

    bool hasRadius() const { return m_hasRadius; }
    void setHasRadius(bool hasRadius) { m_hasRadius = hasRadius; }

    bool operator==(const ClipRect& other) const { return rect() == other.rect() && hasRadius() == other.hasRadius(); }
    bool operator!=(const ClipRect& other) const { return rect() != other.rect() || hasRadius() != other.hasRadius(); }
    bool operator!=(const LayoutRect& otherRect) const { return rect() != otherRect; }

    void intersect(const LayoutRect& other) { m_rect.intersect(other); }
    void intersect(const ClipRect& other)
    {
        m_rect.intersect(other.rect());
        if (other.hasRadius())
            m_hasRadius = true;
    }
    void move(LayoutUnit x, LayoutUnit y) { m_rect.move(x, y); }
    void move(const LayoutSize& size) { m_rect.move(size); }

    bool isEmpty() const { return m_rect.isEmpty(); }
    bool intersects(const LayoutRect& rect) { return m_rect.intersects(rect); }
    bool intersects(const HitTestLocation&);

private:
    LayoutRect m_rect;
    bool m_hasRadius;
};

inline ClipRect intersection(const ClipRect& a, const ClipRect& b)
{
    ClipRect c = a;
    c.intersect(b);
    return c;
}

class ClipRects {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassRefPtr<ClipRects> create()
    {
        return adoptRef(new ClipRects);
    }

    static PassRefPtr<ClipRects> create(const ClipRects& other)
    {
        return adoptRef(new ClipRects(other));
    }

    ClipRects()
        : m_refCnt(1)
        , m_fixed(false)
    {
    }

    void reset(const LayoutRect& r)
    {
        m_overflowClipRect = r;
        m_fixedClipRect = r;
        m_posClipRect = r;
        m_fixed = false;
    }
    
    const ClipRect& overflowClipRect() const { return m_overflowClipRect; }
    void setOverflowClipRect(const ClipRect& r) { m_overflowClipRect = r; }

    const ClipRect& fixedClipRect() const { return m_fixedClipRect; }
    void setFixedClipRect(const ClipRect&r) { m_fixedClipRect = r; }

    const ClipRect& posClipRect() const { return m_posClipRect; }
    void setPosClipRect(const ClipRect& r) { m_posClipRect = r; }

    bool fixed() const { return m_fixed; }
    void setFixed(bool fixed) { m_fixed = fixed; }

    void ref() { m_refCnt++; }
    void deref()
    {
        if (!--m_refCnt)
            delete this;
    }

    bool operator==(const ClipRects& other) const
    {
        return m_overflowClipRect == other.overflowClipRect() &&
               m_fixedClipRect == other.fixedClipRect() &&
               m_posClipRect == other.posClipRect() &&
               m_fixed == other.fixed();
    }

    ClipRects& operator=(const ClipRects& other)
    {
        m_overflowClipRect = other.overflowClipRect();
        m_fixedClipRect = other.fixedClipRect();
        m_posClipRect = other.posClipRect();
        m_fixed = other.fixed();
        return *this;
    }

private:
    ClipRects(const LayoutRect& r)
        : m_overflowClipRect(r)
        , m_fixedClipRect(r)
        , m_posClipRect(r)
        , m_refCnt(1)
        , m_fixed(false)
    {
    }

    ClipRects(const ClipRects& other)
        : m_overflowClipRect(other.overflowClipRect())
        , m_fixedClipRect(other.fixedClipRect())
        , m_posClipRect(other.posClipRect())
        , m_refCnt(1)
        , m_fixed(other.fixed())
    {
    }

    ClipRect m_overflowClipRect;
    ClipRect m_fixedClipRect;
    ClipRect m_posClipRect;
    unsigned m_refCnt : 31;
    bool m_fixed : 1;
};

enum ClipRectsType {
    PaintingClipRects, // Relative to painting ancestor. Used for painting.
    RootRelativeClipRects, // Relative to the ancestor treated as the root (e.g. transformed layer). Used for hit testing.
    AbsoluteClipRects, // Relative to the RenderView's layer. Used for compositing overlap testing.
    NumCachedClipRectsTypes,
    AllClipRectTypes,
    TemporaryClipRects
};

struct ClipRectsCache {
    WTF_MAKE_FAST_ALLOCATED;
public:
    ClipRectsCache()
    {
#ifndef NDEBUG
        for (int i = 0; i < NumCachedClipRectsTypes; ++i) {
            m_clipRectsRoot[i] = 0;
            m_respectingOverflowClip[i] = false;
        }
#endif
    }

    RefPtr<ClipRects> m_clipRects[NumCachedClipRectsTypes];
#ifndef NDEBUG
    const RenderLayer* m_clipRectsRoot[NumCachedClipRectsTypes];
    bool m_respectingOverflowClip[NumCachedClipRectsTypes];
#endif
};

class RenderLayer : public ScrollableArea {
public:
    friend class RenderReplica;

    RenderLayer(RenderBoxModelObject*);
    ~RenderLayer();

    RenderBoxModelObject* renderer() const { return m_renderer; }
    RenderBox* renderBox() const { return m_renderer && m_renderer->isBox() ? toRenderBox(m_renderer) : 0; }
    RenderLayer* parent() const { return m_parent; }
    RenderLayer* previousSibling() const { return m_previous; }
    RenderLayer* nextSibling() const { return m_next; }
    RenderLayer* firstChild() const { return m_first; }
    RenderLayer* lastChild() const { return m_last; }

    void addChild(RenderLayer* newChild, RenderLayer* beforeChild = 0);
    RenderLayer* removeChild(RenderLayer*);

    void removeOnlyThisLayer();
    void insertOnlyThisLayer();

    void repaintIncludingDescendants();

#if USE(ACCELERATED_COMPOSITING)
    // Indicate that the layer contents need to be repainted. Only has an effect
    // if layer compositing is being used,
    void setBackingNeedsRepaint();
    void setBackingNeedsRepaintInRect(const LayoutRect&); // r is in the coordinate space of the layer's render object
    void repaintIncludingNonCompositingDescendants(RenderBoxModelObject* repaintContainer);
#endif

    void styleChanged(StyleDifference, const RenderStyle* oldStyle);

    RenderMarquee* marquee() const { return m_marquee; }

    bool isNormalFlowOnly() const { return m_isNormalFlowOnly; }
    bool isSelfPaintingLayer() const { return m_isSelfPaintingLayer; }

    bool cannotBlitToWindow() const;

    bool isTransparent() const;
    RenderLayer* transparentPaintingAncestor();
    void beginTransparencyLayers(GraphicsContext*, const RenderLayer* rootLayer, const LayoutRect& paintDirtyRect, PaintBehavior);

    bool hasReflection() const { return renderer()->hasReflection(); }
    bool isReflection() const { return renderer()->isReplica(); }
    RenderReplica* reflection() const { return m_reflection; }
    RenderLayer* reflectionLayer() const;

    const RenderLayer* root() const
    {
        const RenderLayer* curr = this;
        while (curr->parent())
            curr = curr->parent();
        return curr;
    }
    
    const LayoutPoint& location() const { return m_topLeft; }
    void setLocation(LayoutUnit x, LayoutUnit y) { m_topLeft = LayoutPoint(x, y); }

    const IntSize& size() const { return m_layerSize; }
    void setSize(const IntSize& size) { m_layerSize = size; }

    LayoutRect rect() const { return LayoutRect(location(), size()); }

    int scrollWidth() const;
    int scrollHeight() const;

    void panScrollFromPoint(const IntPoint&);

    enum ScrollOffsetClamping {
        ScrollOffsetUnclamped,
        ScrollOffsetClamped
    };

    // Scrolling methods for layers that can scroll their overflow.
    void scrollByRecursively(const IntSize&, ScrollOffsetClamping = ScrollOffsetUnclamped);
    void scrollToOffset(const IntSize&, ScrollOffsetClamping = ScrollOffsetUnclamped);
    void scrollToXOffset(int x, ScrollOffsetClamping clamp = ScrollOffsetUnclamped) { scrollToOffset(IntSize(x, scrollYOffset()), clamp); }
    void scrollToYOffset(int y, ScrollOffsetClamping clamp = ScrollOffsetUnclamped) { scrollToOffset(IntSize(scrollXOffset(), y), clamp); }

    int scrollXOffset() const { return m_scrollOffset.width() + scrollOrigin().x(); }
    int scrollYOffset() const { return m_scrollOffset.height() + scrollOrigin().y(); }
    IntSize scrollOffset() const { return IntSize(scrollXOffset(), scrollYOffset()); }

    void scrollRectToVisible(const LayoutRect&, const ScrollAlignment& alignX, const ScrollAlignment& alignY);

    LayoutRect getRectToExpose(const LayoutRect& visibleRect, const LayoutRect& exposeRect, const ScrollAlignment& alignX, const ScrollAlignment& alignY);

    bool scrollsOverflow() const;
    bool allowsScrolling() const; // Returns true if at least one scrollbar is visible and enabled.
    bool hasScrollbars() const { return m_hBar || m_vBar; }
    void setHasHorizontalScrollbar(bool);
    void setHasVerticalScrollbar(bool);

    PassRefPtr<Scrollbar> createScrollbar(ScrollbarOrientation);
    void destroyScrollbar(ScrollbarOrientation);

    bool hasHorizontalScrollbar() const { return horizontalScrollbar(); }
    bool hasVerticalScrollbar() const { return verticalScrollbar(); }

    // ScrollableArea overrides
    virtual Scrollbar* horizontalScrollbar() const { return m_hBar.get(); }
    virtual Scrollbar* verticalScrollbar() const { return m_vBar.get(); }
    virtual ScrollableArea* enclosingScrollableArea() const;

    int verticalScrollbarWidth(OverlayScrollbarSizeRelevancy = IgnoreOverlayScrollbarSize) const;
    int horizontalScrollbarHeight(OverlayScrollbarSizeRelevancy = IgnoreOverlayScrollbarSize) const;

    bool hasOverflowControls() const;
    bool isPointInResizeControl(const IntPoint& absolutePoint) const;
    bool hitTestOverflowControls(HitTestResult&, const IntPoint& localPoint);
    IntSize offsetFromResizeCorner(const IntPoint& absolutePoint) const;

    void paintOverflowControls(GraphicsContext*, const IntPoint&, const IntRect& damageRect, bool paintingOverlayControls = false);
    void paintScrollCorner(GraphicsContext*, const IntPoint&, const IntRect& damageRect);
    void paintResizer(GraphicsContext*, const IntPoint&, const IntRect& damageRect);

    void updateScrollInfoAfterLayout();

    bool scroll(ScrollDirection, ScrollGranularity, float multiplier = 1);
    void autoscroll();

    void resize(const PlatformMouseEvent&, const LayoutSize&);
    bool inResizeMode() const { return m_inResizeMode; }
    void setInResizeMode(bool b) { m_inResizeMode = b; }

    bool isRootLayer() const { return m_isRootLayer; }

#if USE(ACCELERATED_COMPOSITING)
    RenderLayerCompositor* compositor() const;
    
    // Notification from the renderer that its content changed (e.g. current frame of image changed).
    // Allows updates of layer content without repainting.
    void contentChanged(ContentChangeType);
#endif

    bool canRender3DTransforms() const;

    void updateLayerPosition();
    
    enum UpdateLayerPositionsFlag {
        CheckForRepaint = 1,
        IsCompositingUpdateRoot = 1 << 1,
        UpdateCompositingLayers = 1 << 2,
        UpdatePagination = 1 << 3
    };
    typedef unsigned UpdateLayerPositionsFlags;
    static const UpdateLayerPositionsFlags defaultFlags = CheckForRepaint | IsCompositingUpdateRoot | UpdateCompositingLayers;
    // Providing |cachedOffset| prevents a outlineBoxForRepaint from walking back to the root for each layer in our subtree.
    // This is an optimistic optimization that is not guaranteed to succeed.
    void updateLayerPositions(LayoutPoint* offsetFromRoot, UpdateLayerPositionsFlags = defaultFlags);
    
    bool isPaginated() const { return m_isPaginated; }

    void updateTransform();
    
#if ENABLE(CSS_COMPOSITING)
    void updateBlendMode();
#endif

    const LayoutSize& offsetForInFlowPosition() const { return m_offsetForInFlowPosition; }

    void clearClipRectsIncludingDescendants(ClipRectsType typeToClear = AllClipRectTypes);
    void clearClipRects(ClipRectsType typeToClear = AllClipRectTypes);

    void addBlockSelectionGapsBounds(const LayoutRect&);
    void clearBlockSelectionGapsBounds();
    void repaintBlockSelectionGaps();

    // Get the enclosing stacking context for this layer.  A stacking context is a layer
    // that has a non-auto z-index.
    RenderLayer* stackingContext() const;
    bool isStackingContext() const { return isStackingContext(renderer()->style()); }

    void dirtyZOrderLists();
    void dirtyStackingContextZOrderLists();

    Vector<RenderLayer*>* posZOrderList() const
    {
        ASSERT(!m_zOrderListsDirty);
        ASSERT(isStackingContext() || !m_posZOrderList);
        return m_posZOrderList;
    }

    bool hasNegativeZOrderList() const { return negZOrderList() && negZOrderList()->size(); }

    Vector<RenderLayer*>* negZOrderList() const
    {
        ASSERT(!m_zOrderListsDirty);
        ASSERT(isStackingContext() || !m_negZOrderList);
        return m_negZOrderList;
    }

    void dirtyNormalFlowList();
    Vector<RenderLayer*>* normalFlowList() const { ASSERT(!m_normalFlowListDirty); return m_normalFlowList; }

    // Update our normal and z-index lists.
    void updateLayerListsIfNeeded();

    // FIXME: We should ASSERT(!m_visibleContentStatusDirty) here, but see https://bugs.webkit.org/show_bug.cgi?id=71044
    // ditto for hasVisibleDescendant(), see https://bugs.webkit.org/show_bug.cgi?id=71277
    bool hasVisibleContent() const { return m_hasVisibleContent; }
    bool hasVisibleDescendant() const { return m_hasVisibleDescendant; }

    void setHasVisibleContent();
    void dirtyVisibleContentStatus();

    // FIXME: We should ASSERT(!m_hasSelfPaintingLayerDescendantDirty); here but we hit the same bugs as visible content above.
    // Part of the issue is with subtree relayout: we don't check if our ancestors have some descendant flags dirty, missing some updates.
    bool hasSelfPaintingLayerDescendant() const { return m_hasSelfPaintingLayerDescendant; }

    // Gets the nearest enclosing positioned ancestor layer (also includes
    // the <html> layer and the root layer).
    RenderLayer* enclosingPositionedAncestor() const;

    // Returns the nearest enclosing layer that is scrollable.
    RenderLayer* enclosingScrollableLayer() const;

    // The layer relative to which clipping rects for this layer are computed.
    RenderLayer* clippingRootForPainting() const;

#if USE(ACCELERATED_COMPOSITING)
    // Enclosing compositing layer; if includeSelf is true, may return this.
    RenderLayer* enclosingCompositingLayer(bool includeSelf = true) const;
    RenderLayer* enclosingCompositingLayerForRepaint(bool includeSelf = true) const;
    // Ancestor compositing layer, excluding this.
    RenderLayer* ancestorCompositingLayer() const { return enclosingCompositingLayer(false); }
#endif

#if ENABLE(CSS_FILTERS)
    RenderLayer* enclosingFilterLayer(bool includeSelf = true) const;
    RenderLayer* enclosingFilterRepaintLayer() const;
    void setFilterBackendNeedsRepaintingInRect(const LayoutRect&, bool immediate);
    bool hasAncestorWithFilterOutsets() const;
#endif

    bool canUseConvertToLayerCoords() const
    {
        // These RenderObject have an impact on their layers' without them knowing about it.
        return !renderer()->hasColumns() && !renderer()->hasTransform() && !isComposited()
#if ENABLE(SVG)
            && !renderer()->isSVGRoot()
#endif
            ;
    }

    void convertToPixelSnappedLayerCoords(const RenderLayer* ancestorLayer, IntPoint& location) const;
    void convertToPixelSnappedLayerCoords(const RenderLayer* ancestorLayer, IntRect&) const;
    void convertToLayerCoords(const RenderLayer* ancestorLayer, LayoutPoint& location) const;
    void convertToLayerCoords(const RenderLayer* ancestorLayer, LayoutRect&) const;

    int zIndex() const { return renderer()->style()->zIndex(); }

    enum PaintLayerFlag {
        PaintLayerHaveTransparency = 1,
        PaintLayerAppliedTransform = 1 << 1,
        PaintLayerTemporaryClipRects = 1 << 2,
        PaintLayerPaintingReflection = 1 << 3,
        PaintLayerPaintingOverlayScrollbars = 1 << 4,
        PaintLayerPaintingCompositingBackgroundPhase = 1 << 5,
        PaintLayerPaintingCompositingForegroundPhase = 1 << 6,
        PaintLayerPaintingCompositingMaskPhase = 1 << 7,
        PaintLayerPaintingOverflowContents = 1 << 8,
        PaintLayerPaintingCompositingAllPhases = (PaintLayerPaintingCompositingBackgroundPhase | PaintLayerPaintingCompositingForegroundPhase | PaintLayerPaintingCompositingMaskPhase)
    };
    
    typedef unsigned PaintLayerFlags;

    // The two main functions that use the layer system.  The paint method
    // paints the layers that intersect the damage rect from back to
    // front.  The hitTest method looks for mouse events by walking
    // layers that intersect the point from front to back.
    void paint(GraphicsContext*, const LayoutRect& damageRect, PaintBehavior = PaintBehaviorNormal, RenderObject* paintingRoot = 0,
        RenderRegion* = 0, PaintLayerFlags = 0);
    bool hitTest(const HitTestRequest&, HitTestResult&);
    bool hitTest(const HitTestRequest&, const HitTestLocation&, HitTestResult&);
    void paintOverlayScrollbars(GraphicsContext*, const LayoutRect& damageRect, PaintBehavior, RenderObject* paintingRoot);

    enum ShouldRespectOverflowClip { IgnoreOverflowClip, RespectOverflowClip };

    // This method figures out our layerBounds in coordinates relative to
    // |rootLayer}.  It also computes our background and foreground clip rects
    // for painting/event handling.
    void calculateRects(const RenderLayer* rootLayer, RenderRegion*, ClipRectsType, const LayoutRect& paintDirtyRect, LayoutRect& layerBounds,
                        ClipRect& backgroundRect, ClipRect& foregroundRect, ClipRect& outlineRect,
                        OverlayScrollbarSizeRelevancy = IgnoreOverlayScrollbarSize, ShouldRespectOverflowClip = RespectOverflowClip) const;

    // Compute and cache clip rects computed with the given layer as the root
    void updateClipRects(const RenderLayer* rootLayer, RenderRegion*, ClipRectsType, OverlayScrollbarSizeRelevancy = IgnoreOverlayScrollbarSize, ShouldRespectOverflowClip = RespectOverflowClip);
    // Compute and return the clip rects. If useCached is true, will used previously computed clip rects on ancestors
    // (rather than computing them all from scratch up the parent chain).
    void calculateClipRects(const RenderLayer* rootLayer, RenderRegion*, ClipRectsType, ClipRects&, OverlayScrollbarSizeRelevancy = IgnoreOverlayScrollbarSize, ShouldRespectOverflowClip = RespectOverflowClip) const;

    ClipRects* clipRects(ClipRectsType type) const { ASSERT(type < NumCachedClipRectsTypes); return m_clipRectsCache ? m_clipRectsCache->m_clipRects[type].get() : 0; }

    LayoutRect childrenClipRect() const; // Returns the foreground clip rect of the layer in the document's coordinate space.
    LayoutRect selfClipRect() const; // Returns the background clip rect of the layer in the document's coordinate space.
    LayoutRect localClipRect() const; // Returns the background clip rect of the layer in the local coordinate space.

    bool intersectsDamageRect(const LayoutRect& layerBounds, const LayoutRect& damageRect, const RenderLayer* rootLayer) const;

    // Bounding box relative to some ancestor layer.
    LayoutRect boundingBox(const RenderLayer* rootLayer) const;
    // Bounding box in the coordinates of this layer.
    LayoutRect localBoundingBox() const;
    // Pixel snapped bounding box relative to the root.
    IntRect absoluteBoundingBox() const;

    enum CalculateLayerBoundsFlag {
        IncludeSelfTransform = 1 << 0,
        UseLocalClipRectIfPossible = 1 << 1,
        IncludeLayerFilterOutsets = 1 << 2,
        ExcludeHiddenDescendants = 1 << 3,
        DefaultCalculateLayerBoundsFlags =  IncludeSelfTransform | UseLocalClipRectIfPossible | IncludeLayerFilterOutsets
    };
    typedef unsigned CalculateLayerBoundsFlags;
    static IntRect calculateLayerBounds(const RenderLayer*, const RenderLayer* ancestorLayer, CalculateLayerBoundsFlags = DefaultCalculateLayerBoundsFlags);
    
    // WARNING: This method returns the offset for the parent as this is what updateLayerPositions expects.
    LayoutPoint computeOffsetFromRoot(bool& hasLayerOffset) const;

    // Return a cached repaint rect, computed relative to the layer renderer's containerForRepaint.
    LayoutRect repaintRect() const { return m_repaintRect; }
    LayoutRect repaintRectIncludingNonCompositingDescendants() const;

    enum UpdateLayerPositionsAfterScrollFlag {
        NoFlag = 0,
        HasSeenViewportConstrainedAncestor = 1 << 0,
        HasSeenAncestorWithOverflowClip = 1 << 1
    };

    typedef unsigned UpdateLayerPositionsAfterScrollFlags;
    void updateLayerPositionsAfterScroll(UpdateLayerPositionsAfterScrollFlags = NoFlag);
    void setRepaintStatus(RepaintStatus status) { m_repaintStatus = status; }

    LayoutUnit staticInlinePosition() const { return m_staticInlinePosition; }
    LayoutUnit staticBlockPosition() const { return m_staticBlockPosition; }
   
    void setStaticInlinePosition(LayoutUnit position) { m_staticInlinePosition = position; }
    void setStaticBlockPosition(LayoutUnit position) { m_staticBlockPosition = position; }

    bool hasTransform() const { return renderer()->hasTransform(); }
    // Note that this transform has the transform-origin baked in.
    TransformationMatrix* transform() const { return m_transform.get(); }
    // currentTransform computes a transform which takes accelerated animations into account. The
    // resulting transform has transform-origin baked in. If the layer does not have a transform,
    // returns the identity matrix.
    TransformationMatrix currentTransform() const;
    TransformationMatrix renderableTransform(PaintBehavior) const;
    
    // Get the perspective transform, which is applied to transformed sublayers.
    // Returns true if the layer has a -webkit-perspective.
    // Note that this transform has the perspective-origin baked in.
    TransformationMatrix perspectiveTransform() const;
    FloatPoint perspectiveOrigin() const;
    bool preserves3D() const { return renderer()->style()->transformStyle3D() == TransformStyle3DPreserve3D; }
    bool has3DTransform() const { return m_transform && !m_transform->isAffine(); }

#if ENABLE(CSS_FILTERS)
    virtual void filterNeedsRepaint();
    bool hasFilter() const { return renderer()->hasFilter(); }
#else
    bool hasFilter() const { return false; }
#endif

#if ENABLE(CSS_COMPOSITING)
    bool hasBlendMode() const { return renderer()->hasBlendMode(); }
#else
    bool hasBlendMode() const { return false; }
#endif

    // Overloaded new operator. Derived classes must override operator new
    // in order to allocate out of the RenderArena.
    void* operator new(size_t, RenderArena*);

    // Overridden to prevent the normal delete from being called.
    void operator delete(void*, size_t);

#if USE(ACCELERATED_COMPOSITING)
    bool isComposited() const { return m_backing != 0; }
    bool hasCompositedMask() const;
    RenderLayerBacking* backing() const { return m_backing.get(); }
    RenderLayerBacking* ensureBacking();
    void clearBacking(bool layerBeingDestroyed = false);
    virtual GraphicsLayer* layerForHorizontalScrollbar() const;
    virtual GraphicsLayer* layerForVerticalScrollbar() const;
    virtual GraphicsLayer* layerForScrollCorner() const;
    virtual bool usesCompositedScrolling() const OVERRIDE;
#else
    bool isComposited() const { return false; }
    bool hasCompositedMask() const { return false; }
    bool usesCompositedScrolling() const { return false; }
#endif

    bool paintsWithTransparency(PaintBehavior paintBehavior) const
    {
        return isTransparent() && ((paintBehavior & PaintBehaviorFlattenCompositingLayers) || !isComposited());
    }

    bool paintsWithTransform(PaintBehavior) const;

    bool containsDirtyOverlayScrollbars() const { return m_containsDirtyOverlayScrollbars; }
    void setContainsDirtyOverlayScrollbars(bool dirtyScrollbars) { m_containsDirtyOverlayScrollbars = dirtyScrollbars; }

#if ENABLE(CSS_FILTERS)
    bool paintsWithFilters() const;
    bool requiresFullLayerImageForFilters() const;
    FilterEffectRenderer* filterRenderer() const 
    {
        RenderLayerFilterInfo* filterInfo = this->filterInfo();
        return filterInfo ? filterInfo->renderer() : 0;
    }
    
    RenderLayerFilterInfo* filterInfo() const { return hasFilterInfo() ? RenderLayerFilterInfo::filterInfoForRenderLayer(this) : 0; }
    RenderLayerFilterInfo* ensureFilterInfo() { return RenderLayerFilterInfo::createFilterInfoForRenderLayerIfNeeded(this); }
    void removeFilterInfoIfNeeded() 
    {
        if (hasFilterInfo())
            RenderLayerFilterInfo::removeFilterInfoForRenderLayer(this); 
    }
    
    bool hasFilterInfo() const { return m_hasFilterInfo; }
    void setHasFilterInfo(bool hasFilterInfo) { m_hasFilterInfo = hasFilterInfo; }
#endif

#if !ASSERT_DISABLED
    bool layerListMutationAllowed() const { return m_layerListMutationAllowed; }
    void setLayerListMutationAllowed(bool flag) { m_layerListMutationAllowed = flag; }
#endif

    Node* enclosingElement() const;

private:
    void updateZOrderLists();
    void rebuildZOrderLists();
    void clearZOrderLists();

    void updateNormalFlowList();

    bool isStackingContext(const RenderStyle* style) const { return !style->hasAutoZIndex() || isRootLayer(); }
    bool isDirtyStackingContext() const { return m_zOrderListsDirty && isStackingContext(); }

    void setAncestorChainHasSelfPaintingLayerDescendant();
    void dirtyAncestorChainHasSelfPaintingLayerDescendantStatus();

    void computeRepaintRects(LayoutPoint* offsetFromRoot = 0);
    void computeRepaintRectsIncludingDescendants();
    void clearRepaintRects();

    void clipToRect(RenderLayer* rootLayer, GraphicsContext*, const LayoutRect& paintDirtyRect, const ClipRect&,
                    BorderRadiusClippingRule = IncludeSelfForBorderRadius);
    void restoreClip(GraphicsContext*, const LayoutRect& paintDirtyRect, const ClipRect&);

    bool shouldRepaintAfterLayout() const;

    void updateSelfPaintingLayer();
    void updateStackingContextsAfterStyleChange(const RenderStyle* oldStyle);

    void updateScrollbarsAfterStyleChange(const RenderStyle* oldStyle);
    void updateScrollbarsAfterLayout();

    friend IntSize RenderBox::scrolledContentOffset() const;
    IntSize scrolledContentOffset() const { return m_scrollOffset; }

    IntSize clampScrollOffset(const IntSize&) const;

    // The normal operator new is disallowed on all render objects.
    void* operator new(size_t) throw();

    void setNextSibling(RenderLayer* next) { m_next = next; }
    void setPreviousSibling(RenderLayer* prev) { m_previous = prev; }
    void setParent(RenderLayer* parent);
    void setFirstChild(RenderLayer* first) { m_first = first; }
    void setLastChild(RenderLayer* last) { m_last = last; }

    LayoutPoint renderBoxLocation() const { return renderer()->isBox() ? toRenderBox(renderer())->location() : LayoutPoint(); }
    LayoutUnit renderBoxX() const { return renderBoxLocation().x(); }
    LayoutUnit renderBoxY() const { return renderBoxLocation().y(); }

    void collectLayers(bool includeHiddenLayers, Vector<RenderLayer*>*&, Vector<RenderLayer*>*&);

    void updateCompositingAndLayerListsIfNeeded();

    void paintLayer(RenderLayer* rootLayer, GraphicsContext*, const LayoutRect& paintDirtyRect, const LayoutSize& subPixelAccumulation,
                    PaintBehavior, RenderObject* paintingRoot, RenderRegion* = 0, OverlapTestRequestMap* = 0,
                    PaintLayerFlags = 0);
    void paintLayerContentsAndReflection(RenderLayer* rootLayer, GraphicsContext*, const LayoutRect& paintDirtyRect, const LayoutSize& subPixelAccumulation,
                    PaintBehavior, RenderObject* paintingRoot, RenderRegion* = 0, OverlapTestRequestMap* = 0,
                    PaintLayerFlags = 0);
    void paintLayerContents(RenderLayer* rootLayer, GraphicsContext*, const LayoutRect& paintDirtyRect, const LayoutSize& subPixelAccumulation,
                    PaintBehavior, RenderObject* paintingRoot, RenderRegion* = 0, OverlapTestRequestMap* = 0,
                    PaintLayerFlags = 0);
    void paintList(Vector<RenderLayer*>*, RenderLayer* rootLayer, GraphicsContext* p,
                   const LayoutRect& paintDirtyRect, PaintBehavior,
                   RenderObject* paintingRoot, RenderRegion*, OverlapTestRequestMap*,
                   PaintLayerFlags);
    void paintPaginatedChildLayer(RenderLayer* childLayer, RenderLayer* rootLayer, GraphicsContext*,
                                  const LayoutRect& paintDirtyRect, PaintBehavior,
                                  RenderObject* paintingRoot, RenderRegion*, OverlapTestRequestMap*,
                                  PaintLayerFlags);
    void paintChildLayerIntoColumns(RenderLayer* childLayer, RenderLayer* rootLayer, GraphicsContext*,
                                    const LayoutRect& paintDirtyRect, PaintBehavior,
                                    RenderObject* paintingRoot, RenderRegion*, OverlapTestRequestMap*,
                                    PaintLayerFlags, const Vector<RenderLayer*>& columnLayers, size_t columnIndex);

    RenderLayer* hitTestLayer(RenderLayer* rootLayer, RenderLayer* containerLayer, const HitTestRequest& request, HitTestResult& result,
                              const LayoutRect& hitTestRect, const HitTestLocation&, bool appliedTransform,
                              const HitTestingTransformState* transformState = 0, double* zOffset = 0);
    RenderLayer* hitTestList(Vector<RenderLayer*>*, RenderLayer* rootLayer, const HitTestRequest& request, HitTestResult& result,
                             const LayoutRect& hitTestRect, const HitTestLocation&,
                             const HitTestingTransformState* transformState, double* zOffsetForDescendants, double* zOffset,
                             const HitTestingTransformState* unflattenedTransformState, bool depthSortDescendants);
    RenderLayer* hitTestPaginatedChildLayer(RenderLayer* childLayer, RenderLayer* rootLayer, const HitTestRequest& request, HitTestResult& result,
                                            const LayoutRect& hitTestRect, const HitTestLocation&,
                                            const HitTestingTransformState* transformState, double* zOffset);
    RenderLayer* hitTestChildLayerColumns(RenderLayer* childLayer, RenderLayer* rootLayer, const HitTestRequest& request, HitTestResult& result,
                                          const LayoutRect& hitTestRect, const HitTestLocation&,
                                          const HitTestingTransformState* transformState, double* zOffset,
                                          const Vector<RenderLayer*>& columnLayers, size_t columnIndex);

    PassRefPtr<HitTestingTransformState> createLocalTransformState(RenderLayer* rootLayer, RenderLayer* containerLayer,
                            const LayoutRect& hitTestRect, const HitTestLocation&,
                            const HitTestingTransformState* containerTransformState) const;
    
    bool hitTestContents(const HitTestRequest&, HitTestResult&, const LayoutRect& layerBounds, const HitTestLocation&, HitTestFilter) const;

    void computeScrollDimensions();
    bool hasHorizontalOverflow() const;
    bool hasVerticalOverflow() const;

    bool shouldBeNormalFlowOnly() const;

    bool shouldBeSelfPaintingLayer() const;

    int scrollPosition(Scrollbar*) const;
    
    // ScrollableArea interface
    virtual void invalidateScrollbarRect(Scrollbar*, const IntRect&);
    virtual void invalidateScrollCornerRect(const IntRect&);
    virtual bool isActive() const;
    virtual bool isScrollCornerVisible() const;
    virtual IntRect scrollCornerRect() const;
    virtual IntRect convertFromScrollbarToContainingView(const Scrollbar*, const IntRect&) const;
    virtual IntRect convertFromContainingViewToScrollbar(const Scrollbar*, const IntRect&) const;
    virtual IntPoint convertFromScrollbarToContainingView(const Scrollbar*, const IntPoint&) const;
    virtual IntPoint convertFromContainingViewToScrollbar(const Scrollbar*, const IntPoint&) const;
    virtual int scrollSize(ScrollbarOrientation) const;
    virtual void setScrollOffset(const IntPoint&);
    virtual IntPoint scrollPosition() const;
    virtual IntPoint minimumScrollPosition() const;
    virtual IntPoint maximumScrollPosition() const;
    virtual IntRect visibleContentRect(bool includeScrollbars) const;
    virtual int visibleHeight() const;
    virtual int visibleWidth() const;
    virtual IntSize contentsSize() const;
    virtual IntSize overhangAmount() const;
    virtual IntPoint currentMousePosition() const;
    virtual bool shouldSuspendScrollAnimations() const;
    virtual bool scrollbarsCanBeActive() const;
    virtual IntRect scrollableAreaBoundingBox() const OVERRIDE;

    // Rectangle encompassing the scroll corner and resizer rect.
    IntRect scrollCornerAndResizerRect() const;

    // NOTE: This should only be called by the overriden setScrollOffset from ScrollableArea.
    void scrollTo(int, int);
    void updateCompositingLayersAfterScroll();

    IntSize scrollbarOffset(const Scrollbar*) const;
    
    void updateScrollableAreaSet(bool hasOverflow);

    void dirtyAncestorChainVisibleDescendantStatus();
    void setAncestorChainHasVisibleDescendant();

    void updateDescendantDependentFlags();

    // This flag is computed by RenderLayerCompositor, which knows more about 3d hierarchies than we do.
    void setHas3DTransformedDescendant(bool b) { m_has3DTransformedDescendant = b; }
    bool has3DTransformedDescendant() const { return m_has3DTransformedDescendant; }
    
    void dirty3DTransformedDescendantStatus();
    // Both updates the status, and returns true if descendants of this have 3d.
    bool update3DTransformedDescendantStatus();

    void createReflection();
    void removeReflection();

    void updateReflectionStyle();
    bool paintingInsideReflection() const { return m_paintingInsideReflection; }
    void setPaintingInsideReflection(bool b) { m_paintingInsideReflection = b; }

#if ENABLE(CSS_FILTERS)
    void updateOrRemoveFilterEffect();
#endif

    void parentClipRects(const RenderLayer* rootLayer, RenderRegion*, ClipRectsType, ClipRects&, OverlayScrollbarSizeRelevancy = IgnoreOverlayScrollbarSize, ShouldRespectOverflowClip = RespectOverflowClip) const;
    ClipRect backgroundClipRect(const RenderLayer* rootLayer, RenderRegion*, ClipRectsType, OverlayScrollbarSizeRelevancy = IgnoreOverlayScrollbarSize, ShouldRespectOverflowClip = RespectOverflowClip) const;
    LayoutRect paintingExtent(const RenderLayer* rootLayer, const LayoutRect& paintDirtyRect, PaintBehavior);

    RenderLayer* enclosingTransformedAncestor() const;

    // Convert a point in absolute coords into layer coords, taking transforms into account
    LayoutPoint absoluteToContents(const LayoutPoint&) const;

    void positionOverflowControls(const IntSize&);
    void updateScrollCornerStyle();
    void updateResizerStyle();

    void drawPlatformResizerImage(GraphicsContext*, IntRect resizerCornerRect);

    void updatePagination();
    
#if USE(ACCELERATED_COMPOSITING)    
    bool hasCompositingDescendant() const { return m_hasCompositingDescendant; }
    void setHasCompositingDescendant(bool b)  { m_hasCompositingDescendant = b; }
    
    enum IndirectCompositingReason {
        NoIndirectCompositingReason,
        IndirectCompositingForStacking,
        IndirectCompositingForOverlap,
        IndirectCompositingForBackgroundLayer,
        IndirectCompositingForGraphicalEffect, // opacity, mask, filter, transform etc.
        IndirectCompositingForPerspective,
        IndirectCompositingForPreserve3D
    };
    
    void setIndirectCompositingReason(IndirectCompositingReason reason) { m_indirectCompositingReason = reason; }
    IndirectCompositingReason indirectCompositingReason() const { return static_cast<IndirectCompositingReason>(m_indirectCompositingReason); }
    bool mustCompositeForIndirectReasons() const { return m_indirectCompositingReason; }
#endif

    friend class RenderLayerBacking;
    friend class RenderLayerCompositor;
    friend class RenderBoxModelObject;

    // Only safe to call from RenderBoxModelObject::destroyLayer(RenderArena*)
    void destroy(RenderArena*);

    LayoutUnit overflowTop() const;
    LayoutUnit overflowBottom() const;
    LayoutUnit overflowLeft() const;
    LayoutUnit overflowRight() const;

    LayoutUnit verticalScrollbarStart(int minX, int maxX) const;
    LayoutUnit horizontalScrollbarStart(int minX) const;

protected:
    // The bitfields are up here so they will fall into the padding from ScrollableArea on 64-bit.

    // Keeps track of whether the layer is currently resizing, so events can cause resizing to start and stop.
    bool m_inResizeMode : 1;

    bool m_scrollDimensionsDirty : 1;
    bool m_zOrderListsDirty : 1;
    bool m_normalFlowListDirty: 1;
    bool m_isNormalFlowOnly : 1;

    bool m_isSelfPaintingLayer : 1;

    // If have no self-painting descendants, we don't have to walk our children during painting. This can lead to
    // significant savings, especially if the tree has lots of non-self-painting layers grouped together (e.g. table cells).
    bool m_hasSelfPaintingLayerDescendant : 1;
    bool m_hasSelfPaintingLayerDescendantDirty : 1;

    const bool m_isRootLayer : 1;

    bool m_usedTransparency : 1; // Tracks whether we need to close a transparent layer, i.e., whether
                                 // we ended up painting this layer or any descendants (and therefore need to
                                 // blend).
    bool m_paintingInsideReflection : 1;  // A state bit tracking if we are painting inside a replica.
    bool m_inOverflowRelayout : 1;
    unsigned m_repaintStatus : 2; // RepaintStatus

    bool m_visibleContentStatusDirty : 1;
    bool m_hasVisibleContent : 1;
    bool m_visibleDescendantStatusDirty : 1;
    bool m_hasVisibleDescendant : 1;

    bool m_isPaginated : 1; // If we think this layer is split by a multi-column ancestor, then this bit will be set.

    bool m_3DTransformedDescendantStatusDirty : 1;
    bool m_has3DTransformedDescendant : 1;  // Set on a stacking context layer that has 3D descendants anywhere
                                            // in a preserves3D hierarchy. Hint to do 3D-aware hit testing.
#if USE(ACCELERATED_COMPOSITING)
    bool m_hasCompositingDescendant : 1; // In the z-order tree.
    unsigned m_indirectCompositingReason : 3;
#endif

    bool m_containsDirtyOverlayScrollbars : 1;
#if !ASSERT_DISABLED
    bool m_layerListMutationAllowed : 1;
#endif
    // This is an optimization added for <table>.
    // Currently cells do not need to update their repaint rectangles when scrolling. This also
    // saves a lot of time when scrolling on a table.
    const bool m_canSkipRepaintRectsUpdateOnScroll : 1;

#if ENABLE(CSS_FILTERS)
    bool m_hasFilterInfo : 1;
#endif

#if ENABLE(CSS_COMPOSITING)
    BlendMode m_blendMode;
#endif

    RenderBoxModelObject* m_renderer;

    RenderLayer* m_parent;
    RenderLayer* m_previous;
    RenderLayer* m_next;
    RenderLayer* m_first;
    RenderLayer* m_last;

    LayoutRect m_repaintRect; // Cached repaint rects. Used by layout.
    LayoutRect m_outlineBox;

    // Our current relative position offset.
    LayoutSize m_offsetForInFlowPosition;

    // Our (x,y) coordinates are in our parent layer's coordinate space.
    LayoutPoint m_topLeft;

    // The layer's width/height
    IntSize m_layerSize;

    // This is the (scroll) offset from scrollOrigin().
    IntSize m_scrollOffset;

    // The width/height of our scrolled area.
    LayoutSize m_scrollSize;

    // For layers with overflow, we have a pair of scrollbars.
    RefPtr<Scrollbar> m_hBar;
    RefPtr<Scrollbar> m_vBar;

    // For layers that establish stacking contexts, m_posZOrderList holds a sorted list of all the
    // descendant layers within the stacking context that have z-indices of 0 or greater
    // (auto will count as 0).  m_negZOrderList holds descendants within our stacking context with negative
    // z-indices.
    Vector<RenderLayer*>* m_posZOrderList;
    Vector<RenderLayer*>* m_negZOrderList;

    // This list contains child layers that cannot create stacking contexts.  For now it is just
    // overflow layers, but that may change in the future.
    Vector<RenderLayer*>* m_normalFlowList;

    OwnPtr<ClipRectsCache> m_clipRectsCache;
    
    IntPoint m_cachedOverlayScrollbarOffset;

    RenderMarquee* m_marquee; // Used by layers with overflow:marquee
    
    // Cached normal flow values for absolute positioned elements with static left/top values.
    LayoutUnit m_staticInlinePosition;
    LayoutUnit m_staticBlockPosition;

    OwnPtr<TransformationMatrix> m_transform;
    
    // May ultimately be extended to many replicas (with their own paint order).
    RenderReplica* m_reflection;
        
    // Renderers to hold our custom scroll corner and resizer.
    RenderScrollbarPart* m_scrollCorner;
    RenderScrollbarPart* m_resizer;

private:
    IntRect m_blockSelectionGapsBounds;

#if USE(ACCELERATED_COMPOSITING)
    OwnPtr<RenderLayerBacking> m_backing;
#endif
};

inline void RenderLayer::clearZOrderLists()
{
    ASSERT(!isStackingContext());

    if (m_posZOrderList) {
        delete m_posZOrderList;
        m_posZOrderList = 0;
    }

    if (m_negZOrderList) {
        delete m_negZOrderList;
        m_negZOrderList = 0;
    }
}

inline void RenderLayer::updateZOrderLists()
{
    if (!m_zOrderListsDirty)
        return;

    if (!isStackingContext()) {
        clearZOrderLists();
        m_zOrderListsDirty = false;
        return;
    }

    rebuildZOrderLists();
}

#if !ASSERT_DISABLED
class LayerListMutationDetector {
public:
    LayerListMutationDetector(RenderLayer* layer)
        : m_layer(layer)
        , m_previousMutationAllowedState(layer->layerListMutationAllowed())
    {
        m_layer->setLayerListMutationAllowed(false);
    }
    
    ~LayerListMutationDetector()
    {
        m_layer->setLayerListMutationAllowed(m_previousMutationAllowedState);
    }

private:
    RenderLayer* m_layer;
    bool m_previousMutationAllowedState;
};
#endif


} // namespace WebCore

#ifndef NDEBUG
// Outside the WebCore namespace for ease of invocation from gdb.
void showLayerTree(const WebCore::RenderLayer*);
void showLayerTree(const WebCore::RenderObject*);
#endif

#endif // RenderLayer_h
