/*
 * Copyright (C) 2003, 2009, 2012 Apple Inc. All rights reserved.
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

#include "GraphicsLayer.h"
#include "PaintInfo.h"
#include "RenderBox.h"
#include "RenderPtr.h"
#include "ScrollableArea.h"
#include <memory>

namespace WebCore {

class FilterEffectRenderer;
class FilterEffectRendererHelper;
class FilterOperations;
class HitTestRequest;
class HitTestResult;
class HitTestingTransformState;
class RenderFlowThread;
class RenderGeometryMap;
class RenderLayerBacking;
class RenderLayerCompositor;
class RenderMarquee;
class RenderNamedFlowFragment;
class RenderReplica;
class RenderScrollbarPart;
class RenderStyle;
class RenderView;
class Scrollbar;
class TransformationMatrix;

enum BorderRadiusClippingRule { IncludeSelfForBorderRadius, DoNotIncludeSelfForBorderRadius };
enum IncludeSelfOrNot { IncludeSelf, ExcludeSelf };

enum RepaintStatus {
    NeedsNormalRepaint,
    NeedsFullRepaint,
    NeedsFullRepaintForPositionedMovementLayout
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
    void moveBy(const LayoutPoint& point) { m_rect.moveBy(point); }

    bool isEmpty() const { return m_rect.isEmpty(); }
    bool intersects(const LayoutRect& rect) const { return m_rect.intersects(rect); }
    bool intersects(const HitTestLocation&) const;

    void inflateX(LayoutUnit dx) { m_rect.inflateX(dx); }
    void inflateY(LayoutUnit dy) { m_rect.inflateY(dy); }
    void inflate(LayoutUnit d) { inflateX(d); inflateY(d); }

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

enum ShouldRespectOverflowClip {
    IgnoreOverflowClip,
    RespectOverflowClip
};

enum ShouldApplyRootOffsetToFragments {
    ApplyRootOffsetToFragments,
    IgnoreRootOffsetForFragments
};

struct ClipRectsCache {
    WTF_MAKE_FAST_ALLOCATED;
public:
    ClipRectsCache()
    {
#ifndef NDEBUG
        for (int i = 0; i < NumCachedClipRectsTypes; ++i) {
            m_clipRectsRoot[i] = 0;
            m_scrollbarRelevancy[i] = IgnoreOverlayScrollbarSize;
        }
#endif
    }

    PassRefPtr<ClipRects> getClipRects(ClipRectsType clipRectsType, ShouldRespectOverflowClip respectOverflow) { return m_clipRects[getIndex(clipRectsType, respectOverflow)]; }
    void setClipRects(ClipRectsType clipRectsType, ShouldRespectOverflowClip respectOverflow, PassRefPtr<ClipRects> clipRects) { m_clipRects[getIndex(clipRectsType, respectOverflow)] = clipRects; }

#ifndef NDEBUG
    const RenderLayer* m_clipRectsRoot[NumCachedClipRectsTypes];
    OverlayScrollbarSizeRelevancy m_scrollbarRelevancy[NumCachedClipRectsTypes];
#endif

private:
    int getIndex(ClipRectsType clipRectsType, ShouldRespectOverflowClip respectOverflow)
    {
        int index = static_cast<int>(clipRectsType);
        if (respectOverflow == RespectOverflowClip)
            index += static_cast<int>(NumCachedClipRectsTypes);
        return index;
    }

    RefPtr<ClipRects> m_clipRects[NumCachedClipRectsTypes * 2];
};

struct LayerFragment {
public:
    LayerFragment()
        : shouldPaintContent(false)
        , hasBoundingBox(false)
    { }

    void setRects(const LayoutRect& bounds, const ClipRect& background, const ClipRect& foreground, const ClipRect& outline, const LayoutRect* bbox)
    {
        layerBounds = bounds;
        backgroundRect = background;
        foregroundRect = foreground;
        outlineRect = outline;
        if (bbox) {
            boundingBox = *bbox;
            hasBoundingBox = true;
        }
    }
    
    void moveBy(const LayoutPoint& offset)
    {
        layerBounds.moveBy(offset);
        backgroundRect.moveBy(offset);
        foregroundRect.moveBy(offset);
        outlineRect.moveBy(offset);
        paginationClip.moveBy(offset);
        boundingBox.moveBy(offset);
    }
    
    void intersect(const LayoutRect& rect)
    {
        backgroundRect.intersect(rect);
        foregroundRect.intersect(rect);
        outlineRect.intersect(rect);
        boundingBox.intersect(rect);
    }
    
    bool shouldPaintContent;
    bool hasBoundingBox;
    LayoutRect layerBounds;
    ClipRect backgroundRect;
    ClipRect foregroundRect;
    ClipRect outlineRect;
    LayoutRect boundingBox;
    
    // Unique to paginated fragments. The physical translation to apply to shift the layer when painting/hit-testing.
    LayoutSize paginationOffset;
    
    // Also unique to paginated fragments. An additional clip that applies to the layer. It is in layer-local
    // (physical) coordinates.
    LayoutRect paginationClip;
};

typedef Vector<LayerFragment, 1> LayerFragments;

class RenderLayer final : public ScrollableArea {
    WTF_MAKE_FAST_ALLOCATED;
public:
    friend class RenderReplica;

    explicit RenderLayer(RenderLayerModelObject&);
    virtual ~RenderLayer();

#if PLATFORM(IOS)
    // Called before the renderer's widget (if any) has been nulled out.
    void willBeDestroyed();
#endif
    String name() const;

    RenderLayerModelObject& renderer() const { return m_renderer; }
    RenderBox* renderBox() const { return renderer().isBox() ? &toRenderBox(renderer()) : nullptr; }
    RenderLayer* parent() const { return m_parent; }
    RenderLayer* previousSibling() const { return m_previous; }
    RenderLayer* nextSibling() const { return m_next; }
    RenderLayer* firstChild() const { return m_first; }
    RenderLayer* lastChild() const { return m_last; }

    void addChild(RenderLayer* newChild, RenderLayer* beforeChild = nullptr);
    RenderLayer* removeChild(RenderLayer*);

    void removeOnlyThisLayer();
    void insertOnlyThisLayer();

    void repaintIncludingDescendants();

    // Indicate that the layer contents need to be repainted. Only has an effect
    // if layer compositing is being used.
    void setBackingNeedsRepaint(GraphicsLayer::ShouldClipToLayer = GraphicsLayer::ClipToLayer);

    // The rect is in the coordinate space of the layer's render object.
    void setBackingNeedsRepaintInRect(const LayoutRect&, GraphicsLayer::ShouldClipToLayer = GraphicsLayer::ClipToLayer);
    void repaintIncludingNonCompositingDescendants(RenderLayerModelObject* repaintContainer);

    void styleChanged(StyleDifference, const RenderStyle* oldStyle);

    RenderMarquee* marquee() const { return m_marquee.get(); }

    bool isNormalFlowOnly() const { return m_isNormalFlowOnly; }
    bool isSelfPaintingLayer() const { return m_isSelfPaintingLayer; }

    bool cannotBlitToWindow() const;

    bool isTransparent() const { return renderer().isTransparent() || renderer().hasMask(); }

    bool hasReflection() const { return renderer().hasReflection(); }
    bool isReflection() const { return renderer().isReplica(); }
    RenderReplica* reflection() const { return m_reflection.get(); }
    RenderLayer* reflectionLayer() const;

    const RenderLayer* root() const
    {
        const RenderLayer* curr = this;
        while (curr->parent())
            curr = curr->parent();
        return curr;
    }
    
    const LayoutPoint& location() const { return m_topLeft; }
    void setLocation(const LayoutPoint& p) { m_topLeft = p; }

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
    void scrollByRecursively(const IntSize&, ScrollOffsetClamping = ScrollOffsetUnclamped, ScrollableArea** scrolledArea = nullptr);
    void scrollToOffset(const IntSize&, ScrollOffsetClamping = ScrollOffsetUnclamped);
    void scrollToXOffset(int x, ScrollOffsetClamping clamp = ScrollOffsetUnclamped) { scrollToOffset(IntSize(x, scrollYOffset()), clamp); }
    void scrollToYOffset(int y, ScrollOffsetClamping clamp = ScrollOffsetUnclamped) { scrollToOffset(IntSize(scrollXOffset(), y), clamp); }

    int scrollXOffset() const { return m_scrollOffset.width() + scrollOrigin().x(); }
    int scrollYOffset() const { return m_scrollOffset.height() + scrollOrigin().y(); }
    IntSize scrollOffset() const { return IntSize(scrollXOffset(), scrollYOffset()); }
    IntSize scrollableContentsSize() const;

    void scrollRectToVisible(const LayoutRect&, const ScrollAlignment& alignX, const ScrollAlignment& alignY);

    LayoutRect getRectToExpose(const LayoutRect& visibleRect, const LayoutRect& visibleRectRelativeToDocument, const LayoutRect& exposeRect, const ScrollAlignment& alignX, const ScrollAlignment& alignY);

    bool scrollsOverflow() const;
    bool hasScrollbars() const { return m_hBar || m_vBar; }
    void setHasHorizontalScrollbar(bool);
    void setHasVerticalScrollbar(bool);

    PassRefPtr<Scrollbar> createScrollbar(ScrollbarOrientation);
    void destroyScrollbar(ScrollbarOrientation);

    bool hasHorizontalScrollbar() const { return horizontalScrollbar(); }
    bool hasVerticalScrollbar() const { return verticalScrollbar(); }

    // ScrollableArea overrides
    virtual Scrollbar* horizontalScrollbar() const override { return m_hBar.get(); }
    virtual Scrollbar* verticalScrollbar() const override { return m_vBar.get(); }
    virtual ScrollableArea* enclosingScrollableArea() const override;

#if PLATFORM(IOS)
#if ENABLE(TOUCH_EVENTS)
    virtual bool handleTouchEvent(const PlatformTouchEvent&) override;
    virtual bool isTouchScrollable() const override { return true; }
#endif
    virtual bool isOverflowScroll() const override { return true; }
    
    virtual void didStartScroll() override;
    virtual void didEndScroll() override;
    virtual void didUpdateScroll() override;
    virtual void setIsUserScroll(bool isUserScroll) override { m_inUserScroll = isUserScroll; }

    bool isInUserScroll() const { return m_inUserScroll; }

    bool requiresScrollBoundsOriginUpdate() const { return m_requiresScrollBoundsOriginUpdate; }
    void setRequiresScrollBoundsOriginUpdate(bool requiresUpdate = true) { m_requiresScrollBoundsOriginUpdate = requiresUpdate; }

    // Returns true when the layer could do touch scrolling, but doesn't look at whether there is actually scrollable overflow.
    bool hasAcceleratedTouchScrolling() const;
    // Returns true when there is actually scrollable overflow (requires layout to be up-to-date).
    bool hasTouchScrollableOverflow() const;
#endif

    int verticalScrollbarWidth(OverlayScrollbarSizeRelevancy = IgnoreOverlayScrollbarSize) const;
    int horizontalScrollbarHeight(OverlayScrollbarSizeRelevancy = IgnoreOverlayScrollbarSize) const;

    bool hasOverflowControls() const;
    bool isPointInResizeControl(const IntPoint& absolutePoint) const;
    bool hitTestOverflowControls(HitTestResult&, const IntPoint& localPoint);
    IntSize offsetFromResizeCorner(const IntPoint& absolutePoint) const;

    void paintOverflowControls(GraphicsContext*, const IntPoint&, const IntRect& damageRect, bool paintingOverlayControls = false);
    void paintScrollCorner(GraphicsContext*, const IntPoint&, const IntRect& damageRect);
    void paintResizer(GraphicsContext*, const LayoutPoint&, const LayoutRect& damageRect);

    void updateScrollInfoAfterLayout();

    bool scroll(ScrollDirection, ScrollGranularity, float multiplier = 1);
    void autoscroll(const IntPoint&);

    bool canResize() const;
    void resize(const PlatformMouseEvent&, const LayoutSize&);
    bool inResizeMode() const { return m_inResizeMode; }
    void setInResizeMode(bool b) { m_inResizeMode = b; }

    bool isRootLayer() const { return m_isRootLayer; }

    RenderLayerCompositor& compositor() const;
    
    // Notification from the renderer that its content changed (e.g. current frame of image changed).
    // Allows updates of layer content without repainting.
    void contentChanged(ContentChangeType);

    bool canRender3DTransforms() const;

    enum UpdateLayerPositionsFlag {
        CheckForRepaint = 1 << 0,
        NeedsFullRepaintInBacking = 1 << 1,
        IsCompositingUpdateRoot = 1 << 2,
        UpdateCompositingLayers = 1 << 3,
        UpdatePagination = 1 << 4,
        SeenTransformedLayer = 1 << 5,
        Seen3DTransformedLayer = 1 << 6
    };
    typedef unsigned UpdateLayerPositionsFlags;
    static const UpdateLayerPositionsFlags defaultFlags = CheckForRepaint | IsCompositingUpdateRoot | UpdateCompositingLayers;

    void updateLayerPositionsAfterLayout(const RenderLayer* rootLayer, UpdateLayerPositionsFlags);

    void updateLayerPositionsAfterOverflowScroll();
    void updateLayerPositionsAfterDocumentScroll();

    void positionNewlyCreatedOverflowControls();

    bool hasCompositedLayerInEnclosingPaginationChain() const;
    enum PaginationInclusionMode { ExcludeCompositedPaginatedLayers, IncludeCompositedPaginatedLayers };
    RenderLayer* enclosingPaginationLayer(PaginationInclusionMode mode) const
    {
        if (mode == ExcludeCompositedPaginatedLayers && hasCompositedLayerInEnclosingPaginationChain())
            return nullptr;
        return m_enclosingPaginationLayer;
    }

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

    // A stacking context is a layer that has a non-auto z-index.
    bool isStackingContext() const { return isStackingContext(&renderer().style()); }

    // A stacking container can have z-order lists. All stacking contexts are
    // stacking containers, but the converse is not true. Layers that use
    // composited scrolling are stacking containers, but they may not
    // necessarily be stacking contexts.
    bool isStackingContainer() const { return isStackingContext() || needsCompositedScrolling(); }

    // Gets the enclosing stacking container for this layer, excluding this
    // layer itself.
    RenderLayer* stackingContainer() const;

    // Gets the enclosing stacking container for this layer, possibly the layer
    // itself, if it is a stacking container.
    RenderLayer* enclosingStackingContainer() { return isStackingContainer() ? this : stackingContainer(); }

    void dirtyZOrderLists();
    void dirtyStackingContainerZOrderLists();

    Vector<RenderLayer*>* posZOrderList() const
    {
        ASSERT(!m_zOrderListsDirty);
        ASSERT(isStackingContainer() || !m_posZOrderList);
        return m_posZOrderList.get();
    }

    bool hasNegativeZOrderList() const { return negZOrderList() && negZOrderList()->size(); }

    Vector<RenderLayer*>* negZOrderList() const
    {
        ASSERT(!m_zOrderListsDirty);
        ASSERT(isStackingContainer() || !m_negZOrderList);
        return m_negZOrderList.get();
    }

    void dirtyNormalFlowList();
    Vector<RenderLayer*>* normalFlowList() const { ASSERT(!m_normalFlowListDirty); return m_normalFlowList.get(); }

    // Update our normal and z-index lists.
    void updateLayerListsIfNeeded();

    // Update the normal and z-index lists of our descendants.
    void updateDescendantsLayerListsIfNeeded(bool recursive);

    // FIXME: We should ASSERT(!m_visibleContentStatusDirty) here, but see https://bugs.webkit.org/show_bug.cgi?id=71044
    // ditto for hasVisibleDescendant(), see https://bugs.webkit.org/show_bug.cgi?id=71277
    bool hasVisibleContent() const { return m_hasVisibleContent; }
    bool hasVisibleDescendant() const { return m_hasVisibleDescendant; }

    void setHasVisibleContent();
    void dirtyVisibleContentStatus();

    bool hasBoxDecorationsOrBackground() const;
    bool hasVisibleBoxDecorations() const;
    // Returns true if this layer has visible content (ignoring any child layers).
    bool isVisuallyNonEmpty() const;
    // True if this layer container renderers that paint.
    bool hasNonEmptyChildRenderers() const;

    // FIXME: We should ASSERT(!m_hasSelfPaintingLayerDescendantDirty); here but we hit the same bugs as visible content above.
    // Part of the issue is with subtree relayout: we don't check if our ancestors have some descendant flags dirty, missing some updates.
    bool hasSelfPaintingLayerDescendant() const { return m_hasSelfPaintingLayerDescendant; }

    // This returns true if we have an out of flow positioned descendant whose
    // containing block is not a descendant of ours. If this is true, we cannot
    // automatically opt into composited scrolling since this out of flow
    // positioned descendant would become clipped by us, possibly altering the 
    // rendering of the page.
    // FIXME: We should ASSERT(!m_hasOutOfFlowPositionedDescendantDirty); here but we may hit the same bugs as visible content above.
    bool hasOutOfFlowPositionedDescendant() const { return m_hasOutOfFlowPositionedDescendant; }

    // Gets the nearest enclosing positioned ancestor layer (also includes
    // the <html> layer and the root layer).
    RenderLayer* enclosingPositionedAncestor() const;

    // Returns the nearest enclosing layer that is scrollable.
    RenderLayer* enclosingScrollableLayer() const;

    // The layer relative to which clipping rects for this layer are computed.
    RenderLayer* clippingRootForPainting() const;

    RenderLayer* enclosingOverflowClipLayer(IncludeSelfOrNot) const;

    // Enclosing compositing layer; if includeSelf is true, may return this.
    RenderLayer* enclosingCompositingLayer(IncludeSelfOrNot = IncludeSelf) const;
    RenderLayer* enclosingCompositingLayerForRepaint(IncludeSelfOrNot = IncludeSelf) const;
    // Ancestor compositing layer, excluding this.
    RenderLayer* ancestorCompositingLayer() const { return enclosingCompositingLayer(ExcludeSelf); }

#if ENABLE(CSS_FILTERS)
    RenderLayer* enclosingFilterLayer(IncludeSelfOrNot = IncludeSelf) const;
    RenderLayer* enclosingFilterRepaintLayer() const;
    void setFilterBackendNeedsRepaintingInRect(const LayoutRect&);
    bool hasAncestorWithFilterOutsets() const;
#endif

    bool canUseConvertToLayerCoords() const
    {
        // These RenderObject have an impact on their layers' without them knowing about it.
        return !renderer().hasTransform() && !renderer().isSVGRoot();
    }

    // FIXME: adjustForColumns allows us to position compositing layers in columns correctly, but eventually they need to be split across columns too.
    enum ColumnOffsetAdjustment { DontAdjustForColumns, AdjustForColumns };
    void convertToPixelSnappedLayerCoords(const RenderLayer* ancestorLayer, IntPoint& location, ColumnOffsetAdjustment adjustForColumns = DontAdjustForColumns) const;
    LayoutPoint convertToLayerCoords(const RenderLayer* ancestorLayer, const LayoutPoint&, ColumnOffsetAdjustment adjustForColumns = DontAdjustForColumns) const;
    LayoutSize offsetFromAncestor(const RenderLayer*) const;

    int zIndex() const { return renderer().style().zIndex(); }

    enum PaintLayerFlag {
        PaintLayerHaveTransparency = 1,
        PaintLayerAppliedTransform = 1 << 1,
        PaintLayerTemporaryClipRects = 1 << 2,
        PaintLayerPaintingReflection = 1 << 3,
        PaintLayerPaintingOverlayScrollbars = 1 << 4,
        PaintLayerPaintingCompositingBackgroundPhase = 1 << 5,
        PaintLayerPaintingCompositingForegroundPhase = 1 << 6,
        PaintLayerPaintingCompositingMaskPhase = 1 << 7,
        PaintLayerPaintingCompositingScrollingPhase = 1 << 8,
        PaintLayerPaintingOverflowContents = 1 << 9,
        PaintLayerPaintingRootBackgroundOnly = 1 << 10,
        PaintLayerPaintingSkipRootBackground = 1 << 11,
        PaintLayerPaintingCompositingAllPhases = (PaintLayerPaintingCompositingBackgroundPhase | PaintLayerPaintingCompositingForegroundPhase | PaintLayerPaintingCompositingMaskPhase)
    };
    
    typedef unsigned PaintLayerFlags;

    // The two main functions that use the layer system.  The paint method
    // paints the layers that intersect the damage rect from back to
    // front.  The hitTest method looks for mouse events by walking
    // layers that intersect the point from front to back.
    void paint(GraphicsContext*, const LayoutRect& damageRect, const LayoutSize& subpixelAccumulation = LayoutSize(), PaintBehavior = PaintBehaviorNormal,
        RenderObject* subtreePaintRoot = nullptr, PaintLayerFlags = 0);
    bool hitTest(const HitTestRequest&, HitTestResult&);
    bool hitTest(const HitTestRequest&, const HitTestLocation&, HitTestResult&);
    void paintOverlayScrollbars(GraphicsContext*, const LayoutRect& damageRect, PaintBehavior, RenderObject* subtreePaintRoot = nullptr);

    void paintNamedFlowThreadInsideRegion(GraphicsContext*, RenderNamedFlowFragment*, LayoutRect, LayoutPoint, PaintBehavior = PaintBehaviorNormal, PaintLayerFlags = 0);

    struct ClipRectsContext {
        ClipRectsContext(const RenderLayer* inRootLayer, ClipRectsType inClipRectsType, OverlayScrollbarSizeRelevancy inOverlayScrollbarSizeRelevancy = IgnoreOverlayScrollbarSize, ShouldRespectOverflowClip inRespectOverflowClip = RespectOverflowClip)
            : rootLayer(inRootLayer)
            , clipRectsType(inClipRectsType)
            , overlayScrollbarSizeRelevancy(inOverlayScrollbarSizeRelevancy)
            , respectOverflowClip(inRespectOverflowClip)
        { }
        const RenderLayer* rootLayer;
        ClipRectsType clipRectsType;
        OverlayScrollbarSizeRelevancy overlayScrollbarSizeRelevancy;
        ShouldRespectOverflowClip respectOverflowClip;
    };

    // This method figures out our layerBounds in coordinates relative to
    // |rootLayer}.  It also computes our background and foreground clip rects
    // for painting/event handling.
    // Pass offsetFromRoot if known.
    void calculateRects(const ClipRectsContext&, const LayoutRect& paintDirtyRect, LayoutRect& layerBounds,
        ClipRect& backgroundRect, ClipRect& foregroundRect, ClipRect& outlineRect, const LayoutSize& offsetFromRoot) const;

    // Compute and cache clip rects computed with the given layer as the root
    void updateClipRects(const ClipRectsContext&);
    // Compute and return the clip rects. If useCached is true, will used previously computed clip rects on ancestors
    // (rather than computing them all from scratch up the parent chain).
    void calculateClipRects(const ClipRectsContext&, ClipRects&) const;

    ClipRects* clipRects(const ClipRectsContext& context) const
    {
        ASSERT(context.clipRectsType < NumCachedClipRectsTypes);
        return m_clipRectsCache ? m_clipRectsCache->getClipRects(context.clipRectsType, context.respectOverflowClip).get() : 0;
    }

    LayoutRect childrenClipRect() const; // Returns the foreground clip rect of the layer in the document's coordinate space.
    LayoutRect selfClipRect() const; // Returns the background clip rect of the layer in the document's coordinate space.
    LayoutRect localClipRect(bool& clipExceedsBounds) const; // Returns the background clip rect of the layer in the local coordinate space.

    // Pass offsetFromRoot if known.
    bool intersectsDamageRect(const LayoutRect& layerBounds, const LayoutRect& damageRect, const RenderLayer* rootLayer, const LayoutSize& offsetFromRoot, const LayoutRect* cachedBoundingBox = nullptr) const;

    enum CalculateLayerBoundsFlag {
        IncludeSelfTransform = 1 << 0,
        UseLocalClipRectIfPossible = 1 << 1,
        IncludeLayerFilterOutsets = 1 << 2,
        ExcludeHiddenDescendants = 1 << 3,
        DontConstrainForMask = 1 << 4,
        IncludeCompositedDescendants = 1 << 5,
        UseFragmentBoxesExcludingCompositing = 1 << 6,
        UseFragmentBoxesIncludingCompositing = 1 << 7,
        DefaultCalculateLayerBoundsFlags =  IncludeSelfTransform | UseLocalClipRectIfPossible | IncludeLayerFilterOutsets | UseFragmentBoxesExcludingCompositing
    };
    typedef unsigned CalculateLayerBoundsFlags;

    // Bounding box relative to some ancestor layer. Pass offsetFromRoot if known.
    LayoutRect boundingBox(const RenderLayer* rootLayer, const LayoutSize& offsetFromRoot = LayoutSize(), CalculateLayerBoundsFlags = 0) const;
    // Bounding box in the coordinates of this layer.
    LayoutRect localBoundingBox(CalculateLayerBoundsFlags = 0) const;
    // Deprecated: Pixel snapped bounding box relative to the root.
    IntRect absoluteBoundingBox() const;
    // Device pixel snapped bounding box relative to the root. absoluteBoundingBox() callers will be directed to this.
    FloatRect absoluteBoundingBoxForPainting() const;

    // Bounds used for layer overlap testing in RenderLayerCompositor.
    LayoutRect overlapBounds() const { return overlapBoundsIncludeChildren() ? calculateLayerBounds(this, LayoutSize()) : localBoundingBox(); }

#if ENABLE(CSS_FILTERS)
    // If true, this layer's children are included in its bounds for overlap testing.
    // We can't rely on the children's positions if this layer has a filter that could have moved the children's pixels around.
    bool overlapBoundsIncludeChildren() const { return hasFilter() && renderer().style().filter().hasFilterThatMovesPixels(); }
#else
    bool overlapBoundsIncludeChildren() const { return false; }
#endif

    // Can pass offsetFromRoot if known.
    LayoutRect calculateLayerBounds(const RenderLayer* ancestorLayer, const LayoutSize& offsetFromRoot, CalculateLayerBoundsFlags = DefaultCalculateLayerBoundsFlags) const;
    
    // Return a cached repaint rect, computed relative to the layer renderer's containerForRepaint.
    LayoutRect repaintRect() const { return m_repaintRect; }
    LayoutRect repaintRectIncludingNonCompositingDescendants() const;

    void setRepaintStatus(RepaintStatus status) { m_repaintStatus = status; }

    LayoutUnit staticInlinePosition() const { return m_staticInlinePosition; }
    LayoutUnit staticBlockPosition() const { return m_staticBlockPosition; }
   
    void setStaticInlinePosition(LayoutUnit position) { m_staticInlinePosition = position; }
    void setStaticBlockPosition(LayoutUnit position) { m_staticBlockPosition = position; }

#if PLATFORM(IOS)
    bool adjustForIOSCaretWhenScrolling() const { return m_adjustForIOSCaretWhenScrolling; }
    void setAdjustForIOSCaretWhenScrolling(bool adjustForIOSCaretWhenScrolling) { m_adjustForIOSCaretWhenScrolling = adjustForIOSCaretWhenScrolling; }
#endif

    bool hasTransform() const { return renderer().hasTransform(); }
    // Note that this transform has the transform-origin baked in.
    TransformationMatrix* transform() const { return m_transform.get(); }
    // currentTransform computes a transform which takes accelerated animations into account. The
    // resulting transform has transform-origin baked in. If the layer does not have a transform,
    // returns the identity matrix.
    TransformationMatrix currentTransform(RenderStyle::ApplyTransformOrigin = RenderStyle::IncludeTransformOrigin) const;
    TransformationMatrix renderableTransform(PaintBehavior) const;
    
    // Get the perspective transform, which is applied to transformed sublayers.
    // Returns true if the layer has a -webkit-perspective.
    // Note that this transform has the perspective-origin baked in.
    TransformationMatrix perspectiveTransform() const;
    FloatPoint perspectiveOrigin() const;
    bool preserves3D() const { return renderer().style().transformStyle3D() == TransformStyle3DPreserve3D; }
    bool has3DTransform() const { return m_transform && !m_transform->isAffine(); }

#if ENABLE(CSS_FILTERS)
    virtual void filterNeedsRepaint();
    bool hasFilter() const { return renderer().hasFilter(); }
#else
    bool hasFilter() const { return false; }
#endif

#if ENABLE(CSS_COMPOSITING)
    bool hasBlendMode() const { return renderer().hasBlendMode(); }
    BlendMode blendMode() const { return static_cast<BlendMode>(m_blendMode); }

    bool isolatesCompositedBlending() const { return m_hasNotIsolatedCompositedBlendingDescendants && isStackingContext(); }
    bool hasNotIsolatedCompositedBlendingDescendants() const { return m_hasNotIsolatedCompositedBlendingDescendants; }
    void setHasNotIsolatedCompositedBlendingDescendants(bool hasNotIsolatedCompositedBlendingDescendants)
    {
        m_hasNotIsolatedCompositedBlendingDescendants = hasNotIsolatedCompositedBlendingDescendants;
    }

    bool isolatesBlending() const { return hasNotIsolatedBlendingDescendants() && isStackingContext(); }
    
    // FIXME: We should ASSERT(!m_hasNotIsolatedBlendingDescendantsStatusDirty); here but we hit the same bugs as visible content above.
    bool hasNotIsolatedBlendingDescendants() const { return m_hasNotIsolatedBlendingDescendants; }
    bool hasNotIsolatedBlendingDescendantsStatusDirty() const { return m_hasNotIsolatedBlendingDescendantsStatusDirty; }
#else
    bool hasBlendMode() const { return false; }
    bool isolatesCompositedBlending() const { return false; }
    bool isolatesBlending() const { return false; }
    bool hasNotIsolatedBlendingDescendantsStatusDirty() const { return false; }
#endif

    bool isComposited() const { return m_backing != 0; }
    bool hasCompositingDescendant() const { return m_hasCompositingDescendant; }
    bool hasCompositedMask() const;
    RenderLayerBacking* backing() const { return m_backing.get(); }
    RenderLayerBacking* ensureBacking();
    void clearBacking(bool layerBeingDestroyed = false);
    virtual GraphicsLayer* layerForScrolling() const override;
    virtual GraphicsLayer* layerForHorizontalScrollbar() const override;
    virtual GraphicsLayer* layerForVerticalScrollbar() const override;
    virtual GraphicsLayer* layerForScrollCorner() const override;
    virtual bool usesCompositedScrolling() const override;
    bool needsCompositedScrolling() const;
    bool needsCompositingLayersRebuiltForClip(const RenderStyle* oldStyle, const RenderStyle* newStyle) const;
    bool needsCompositingLayersRebuiltForOverflow(const RenderStyle* oldStyle, const RenderStyle* newStyle) const;

    bool paintsWithTransparency(PaintBehavior paintBehavior) const
    {
        return (isTransparent() || hasBlendMode() || (isolatesBlending() && !renderer().isRoot())) && ((paintBehavior & PaintBehaviorFlattenCompositingLayers) || !isComposited());
    }

    bool paintsWithTransform(PaintBehavior) const;

    // Returns true if background phase is painted opaque in the given rect.
    // The query rect is given in local coordinates.
    bool backgroundIsKnownToBeOpaqueInRect(const LayoutRect&) const;

    bool containsDirtyOverlayScrollbars() const { return m_containsDirtyOverlayScrollbars; }
    void setContainsDirtyOverlayScrollbars(bool dirtyScrollbars) { m_containsDirtyOverlayScrollbars = dirtyScrollbars; }

#if ENABLE(CSS_FILTERS)
    bool paintsWithFilters() const;
    bool requiresFullLayerImageForFilters() const;
    FilterEffectRenderer* filterRenderer() const;
#endif

#if !ASSERT_DISABLED
    bool layerListMutationAllowed() const { return m_layerListMutationAllowed; }
    void setLayerListMutationAllowed(bool flag) { m_layerListMutationAllowed = flag; }
#endif

    Element* enclosingElement() const;

    enum ViewportConstrainedNotCompositedReason {
        NoNotCompositedReason,
        NotCompositedForBoundsOutOfView,
        NotCompositedForNonViewContainer,
        NotCompositedForNoVisibleContent,
    };

    void setViewportConstrainedNotCompositedReason(ViewportConstrainedNotCompositedReason reason) { m_viewportConstrainedNotCompositedReason = reason; }
    ViewportConstrainedNotCompositedReason viewportConstrainedNotCompositedReason() const { return static_cast<ViewportConstrainedNotCompositedReason>(m_viewportConstrainedNotCompositedReason); }
    
    bool isRenderFlowThread() const { return renderer().isRenderFlowThread(); }
    bool isOutOfFlowRenderFlowThread() const { return renderer().isOutOfFlowRenderFlowThread(); }
    bool isInsideFlowThread() const { return renderer().flowThreadState() != RenderObject::NotInsideFlowThread; }
    bool isInsideOutOfFlowThread() const { return renderer().flowThreadState() == RenderObject::InsideOutOfFlowThread; }
    bool isDirtyRenderFlowThread() const
    {
        ASSERT(isRenderFlowThread());
        return m_zOrderListsDirty || m_normalFlowListDirty;
    }

    bool isFlowThreadCollectingGraphicsLayersUnderRegions() const;

    RenderLayer* enclosingFlowThreadAncestor() const;

private:
    enum CollectLayersBehavior { StopAtStackingContexts, StopAtStackingContainers };

    struct LayerPaintingInfo {
        LayerPaintingInfo(RenderLayer* inRootLayer, const LayoutRect& inDirtyRect, PaintBehavior inPaintBehavior, const LayoutSize& inSubpixelAccumulation, RenderObject* inSubtreePaintRoot = nullptr, OverlapTestRequestMap* inOverlapTestRequests = nullptr)
            : rootLayer(inRootLayer)
            , subtreePaintRoot(inSubtreePaintRoot)
            , paintDirtyRect(inDirtyRect)
            , subpixelAccumulation(inSubpixelAccumulation)
            , overlapTestRequests(inOverlapTestRequests)
            , paintBehavior(inPaintBehavior)
            , clipToDirtyRect(true)
        { }
        RenderLayer* rootLayer;
        RenderObject* subtreePaintRoot; // only paint descendants of this object
        LayoutRect paintDirtyRect; // relative to rootLayer;
        LayoutSize subpixelAccumulation;
        OverlapTestRequestMap* overlapTestRequests; // May be null.
        PaintBehavior paintBehavior;
        bool clipToDirtyRect;
    };

    void updateZOrderLists();
    void rebuildZOrderLists();
    void rebuildZOrderLists(CollectLayersBehavior, std::unique_ptr<Vector<RenderLayer*>>&, std::unique_ptr<Vector<RenderLayer*>>&);
    void clearZOrderLists();

    void updateNormalFlowList();

    // Non-auto z-index always implies stacking context here, because StyleResolver::adjustRenderStyle already adjusts z-index
    // based on positioning and other criteria.
    bool isStackingContext(const RenderStyle* style) const { return !style->hasAutoZIndex() || isRootLayer(); }

    bool isDirtyStackingContainer() const { return m_zOrderListsDirty && isStackingContainer(); }

    void setAncestorChainHasSelfPaintingLayerDescendant();
    void dirtyAncestorChainHasSelfPaintingLayerDescendantStatus();

    bool acceleratedCompositingForOverflowScrollEnabled() const;
    void updateDescendantsAreContiguousInStackingOrder();
    void updateDescendantsAreContiguousInStackingOrderRecursive(const HashMap<const RenderLayer*, int>&, int& minIndex, int& maxIndex, int& count, bool firstIteration);

    void computeRepaintRects(const RenderLayerModelObject* repaintContainer, const RenderGeometryMap* = nullptr);
    void computeRepaintRectsIncludingDescendants();
    void clearRepaintRects();

    void clipToRect(const LayerPaintingInfo&, GraphicsContext*, const ClipRect&, BorderRadiusClippingRule = IncludeSelfForBorderRadius);
    void restoreClip(GraphicsContext*, const LayoutRect& paintDirtyRect, const ClipRect&);

    bool shouldRepaintAfterLayout() const;

    void updateSelfPaintingLayer();
    void updateStackingContextsAfterStyleChange(const RenderStyle* oldStyle);

    void updateScrollbarsAfterStyleChange(const RenderStyle* oldStyle);
    void updateScrollbarsAfterLayout();

    void setAncestorChainHasOutOfFlowPositionedDescendant(RenderBlock* containingBlock);
    void dirtyAncestorChainHasOutOfFlowPositionedDescendantStatus();
    void updateOutOfFlowPositioned(const RenderStyle* oldStyle);

    void updateNeedsCompositedScrolling();

    // Returns true if the position changed.
    bool updateLayerPosition();

    void updateLayerPositions(RenderGeometryMap* = nullptr, UpdateLayerPositionsFlags = defaultFlags);

    enum UpdateLayerPositionsAfterScrollFlag {
        NoFlag = 0,
        IsOverflowScroll = 1 << 0,
        HasSeenViewportConstrainedAncestor = 1 << 1,
        HasSeenAncestorWithOverflowClip = 1 << 2,
        HasChangedAncestor = 1 << 3
    };
    typedef unsigned UpdateLayerPositionsAfterScrollFlags;
    void updateLayerPositionsAfterScroll(RenderGeometryMap*, UpdateLayerPositionsAfterScrollFlags = NoFlag);

    friend IntSize RenderBox::scrolledContentOffset() const;
    IntSize scrolledContentOffset() const { return m_scrollOffset; }

    IntSize clampScrollOffset(const IntSize&) const;

    RenderLayer* enclosingPaginationLayerInSubtree(const RenderLayer* rootLayer, PaginationInclusionMode) const;

    void setNextSibling(RenderLayer* next) { m_next = next; }
    void setPreviousSibling(RenderLayer* prev) { m_previous = prev; }
    void setParent(RenderLayer* parent);
    void setFirstChild(RenderLayer* first) { m_first = first; }
    void setLastChild(RenderLayer* last) { m_last = last; }

    LayoutPoint renderBoxLocation() const { return renderer().isBox() ? toRenderBox(renderer()).location() : LayoutPoint(); }

    void collectLayers(bool includeHiddenLayers, CollectLayersBehavior, std::unique_ptr<Vector<RenderLayer*>>&, std::unique_ptr<Vector<RenderLayer*>>&);

    void updateCompositingAndLayerListsIfNeeded();

    bool setupFontSubpixelQuantization(GraphicsContext*, bool& didQuantizeFonts);
    bool setupClipPath(GraphicsContext*, const LayerPaintingInfo&, const LayoutSize& offsetFromRoot, LayoutRect& rootRelativeBounds, bool& rootRelativeBoundsComputed);
#if ENABLE(CSS_FILTERS)
    std::unique_ptr<FilterEffectRendererHelper> setupFilters(GraphicsContext*, LayerPaintingInfo&, PaintLayerFlags, const LayoutSize& offsetFromRoot, LayoutRect& rootRelativeBounds, bool& rootRelativeBoundsComputed);
    GraphicsContext* applyFilters(FilterEffectRendererHelper*, GraphicsContext* originalContext, LayerPaintingInfo&, LayerFragments&);
#endif

    void paintLayer(GraphicsContext*, const LayerPaintingInfo&, PaintLayerFlags);
    void paintFixedLayersInNamedFlows(GraphicsContext*, const LayerPaintingInfo&, PaintLayerFlags);
    void paintLayerContentsAndReflection(GraphicsContext*, const LayerPaintingInfo&, PaintLayerFlags);
    void paintLayerByApplyingTransform(GraphicsContext*, const LayerPaintingInfo&, PaintLayerFlags, const LayoutSize& translationOffset = LayoutSize());
    void paintLayerContents(GraphicsContext*, const LayerPaintingInfo&, PaintLayerFlags);
    void paintList(Vector<RenderLayer*>*, GraphicsContext*, const LayerPaintingInfo&, PaintLayerFlags);

    void collectFragments(LayerFragments&, const RenderLayer* rootLayer, const LayoutRect& dirtyRect,
        PaginationInclusionMode,
        ClipRectsType, OverlayScrollbarSizeRelevancy inOverlayScrollbarSizeRelevancy, ShouldRespectOverflowClip, const LayoutSize& offsetFromRoot,
        const LayoutRect* layerBoundingBox = nullptr, ShouldApplyRootOffsetToFragments = IgnoreRootOffsetForFragments);
    void updatePaintingInfoForFragments(LayerFragments&, const LayerPaintingInfo&, PaintLayerFlags, bool shouldPaintContent, const LayoutSize& offsetFromRoot);
    void paintBackgroundForFragments(const LayerFragments&, GraphicsContext*, GraphicsContext* transparencyLayerContext,
        const LayoutRect& transparencyPaintDirtyRect, bool haveTransparency, const LayerPaintingInfo&, PaintBehavior, RenderObject* paintingRootForRenderer);
    void paintForegroundForFragments(const LayerFragments&, GraphicsContext*, GraphicsContext* transparencyLayerContext,
        const LayoutRect& transparencyPaintDirtyRect, bool haveTransparency, const LayerPaintingInfo&, PaintBehavior, RenderObject* paintingRootForRenderer,
        bool selectionOnly);
    void paintForegroundForFragmentsWithPhase(PaintPhase, const LayerFragments&, GraphicsContext*, const LayerPaintingInfo&, PaintBehavior, RenderObject* paintingRootForRenderer);
    void paintOutlineForFragments(const LayerFragments&, GraphicsContext*, const LayerPaintingInfo&, PaintBehavior, RenderObject* paintingRootForRenderer);
    void paintOverflowControlsForFragments(const LayerFragments&, GraphicsContext*, const LayerPaintingInfo&);
    void paintMaskForFragments(const LayerFragments&, GraphicsContext*, const LayerPaintingInfo&, RenderObject* paintingRootForRenderer);
    void paintTransformedLayerIntoFragments(GraphicsContext*, const LayerPaintingInfo&, PaintLayerFlags);

    RenderLayer* transparentPaintingAncestor();
    void beginTransparencyLayers(GraphicsContext*, const LayerPaintingInfo&, const LayoutRect& dirtyRect);

    RenderLayer* hitTestLayer(RenderLayer* rootLayer, RenderLayer* containerLayer, const HitTestRequest& request, HitTestResult& result,
        const LayoutRect& hitTestRect, const HitTestLocation&, bool appliedTransform,
        const HitTestingTransformState* = nullptr, double* zOffset = nullptr);
    RenderLayer* hitTestLayerByApplyingTransform(RenderLayer* rootLayer, RenderLayer* containerLayer, const HitTestRequest&, HitTestResult&,
        const LayoutRect& hitTestRect, const HitTestLocation&, const HitTestingTransformState* = nullptr, double* zOffset = nullptr,
        const LayoutSize& translationOffset = LayoutSize());
    RenderLayer* hitTestList(Vector<RenderLayer*>*, RenderLayer* rootLayer, const HitTestRequest& request, HitTestResult& result,
        const LayoutRect& hitTestRect, const HitTestLocation&,
        const HitTestingTransformState*, double* zOffsetForDescendants, double* zOffset,
        const HitTestingTransformState* unflattenedTransformState, bool depthSortDescendants);

    RenderLayer* hitTestFixedLayersInNamedFlows(RenderLayer* rootLayer,
        const HitTestRequest&, HitTestResult&,
        const LayoutRect& hitTestRect, const HitTestLocation&,
        const HitTestingTransformState*,
        double* zOffsetForDescendants, double* zOffset,
        const HitTestingTransformState* unflattenedTransformState,
        bool depthSortDescendants);

    PassRefPtr<HitTestingTransformState> createLocalTransformState(RenderLayer* rootLayer, RenderLayer* containerLayer,
        const LayoutRect& hitTestRect, const HitTestLocation&,
        const HitTestingTransformState* containerTransformState,
        const LayoutSize& translationOffset = LayoutSize()) const;
    
    bool hitTestContents(const HitTestRequest&, HitTestResult&, const LayoutRect& layerBounds, const HitTestLocation&, HitTestFilter) const;
    bool hitTestContentsForFragments(const LayerFragments&, const HitTestRequest&, HitTestResult&, const HitTestLocation&, HitTestFilter, bool& insideClipRect) const;
    bool hitTestResizerInFragments(const LayerFragments&, const HitTestLocation&) const;
    RenderLayer* hitTestTransformedLayerInFragments(RenderLayer* rootLayer, RenderLayer* containerLayer, const HitTestRequest&, HitTestResult&,
        const LayoutRect& hitTestRect, const HitTestLocation&, const HitTestingTransformState* = nullptr, double* zOffset = nullptr);

    bool listBackgroundIsKnownToBeOpaqueInRect(const Vector<RenderLayer*>*, const LayoutRect&) const;

    void computeScrollDimensions();
    bool hasHorizontalOverflow() const;
    bool hasVerticalOverflow() const;
    bool hasScrollableHorizontalOverflow() const;
    bool hasScrollableVerticalOverflow() const;

    bool showsOverflowControls() const;

    bool shouldBeNormalFlowOnly() const;

    bool shouldBeSelfPaintingLayer() const;

    virtual int scrollPosition(Scrollbar*) const override;
    
    // ScrollableArea interface
    virtual void invalidateScrollbarRect(Scrollbar*, const IntRect&) override;
    virtual void invalidateScrollCornerRect(const IntRect&) override;
    virtual bool isActive() const override;
    virtual bool isScrollCornerVisible() const override;
    virtual IntRect scrollCornerRect() const override;
    virtual IntRect convertFromScrollbarToContainingView(const Scrollbar*, const IntRect&) const override;
    virtual IntRect convertFromContainingViewToScrollbar(const Scrollbar*, const IntRect&) const override;
    virtual IntPoint convertFromScrollbarToContainingView(const Scrollbar*, const IntPoint&) const override;
    virtual IntPoint convertFromContainingViewToScrollbar(const Scrollbar*, const IntPoint&) const override;
    virtual int scrollSize(ScrollbarOrientation) const override;
    virtual void setScrollOffset(const IntPoint&) override;
    virtual IntPoint scrollPosition() const override;
    virtual IntPoint minimumScrollPosition() const override;
    virtual IntPoint maximumScrollPosition() const override;
    virtual IntRect visibleContentRectInternal(VisibleContentRectIncludesScrollbars, VisibleContentRectBehavior) const override;
    virtual IntSize visibleSize() const override;
    virtual IntSize contentsSize() const override;
    virtual IntSize overhangAmount() const override;
    virtual IntPoint lastKnownMousePosition() const override;
    virtual bool isHandlingWheelEvent() const override;
    virtual bool shouldSuspendScrollAnimations() const override;
    virtual IntRect scrollableAreaBoundingBox() const override;
    virtual bool updatesScrollLayerPositionOnMainThread() const override { return true; }
    virtual bool forceUpdateScrollbarsOnMainThreadForPerformanceTesting() const override;

#if PLATFORM(IOS)
    void registerAsTouchEventListenerForScrolling();
    void unregisterAsTouchEventListenerForScrolling();
#endif

    // Rectangle encompassing the scroll corner and resizer rect.
    LayoutRect scrollCornerAndResizerRect() const;

    // NOTE: This should only be called by the overriden setScrollOffset from ScrollableArea.
    void scrollTo(int, int);
    void updateCompositingLayersAfterScroll();

    IntSize scrollbarOffset(const Scrollbar*) const;
    
    void updateScrollableAreaSet(bool hasOverflow);

    void dirtyAncestorChainVisibleDescendantStatus();
    void setAncestorChainHasVisibleDescendant();

    void updateDescendantDependentFlags(HashSet<const RenderObject*>* outOfFlowDescendantContainingBlocks = nullptr);
    bool checkIfDescendantClippingContextNeedsUpdate(bool isClipping);

    bool has3DTransformedDescendant() const { return m_has3DTransformedDescendant; }

    bool hasTransformedAncestor() const { return m_hasTransformedAncestor; }
    bool has3DTransformedAncestor() const { return m_has3DTransformedAncestor; }

    void dirty3DTransformedDescendantStatus();
    // Both updates the status, and returns true if descendants of this have 3d.
    bool update3DTransformedDescendantStatus();

    void createReflection();
    void removeReflection();

    PassRef<RenderStyle> createReflectionStyle();
    bool paintingInsideReflection() const { return m_paintingInsideReflection; }
    void setPaintingInsideReflection(bool b) { m_paintingInsideReflection = b; }

#if ENABLE(CSS_FILTERS)
    void updateOrRemoveFilterClients();
    void updateOrRemoveFilterEffectRenderer();
#endif

#if ENABLE(CSS_COMPOSITING)
    void updateAncestorChainHasBlendingDescendants();
    void dirtyAncestorChainHasBlendingDescendants();
#endif

    void parentClipRects(const ClipRectsContext&, ClipRects&) const;
    ClipRect backgroundClipRect(const ClipRectsContext&) const;

    RenderLayer* enclosingTransformedAncestor() const;

    // Convert a point in absolute coords into layer coords, taking transforms into account
    LayoutPoint absoluteToContents(const LayoutPoint&) const;

    void positionOverflowControls(const IntSize&);
    void updateScrollCornerStyle();
    void updateResizerStyle();

    void drawPlatformResizerImage(GraphicsContext*, const LayoutRect& resizerCornerRect);

    void updatePagination();

    void setHasCompositingDescendant(bool b)  { m_hasCompositingDescendant = b; }
    
    enum class IndirectCompositingReason {
        None,
        Stacking,
        Overlap,
        BackgroundLayer,
        GraphicalEffect, // opacity, mask, filter, transform etc.
        Perspective,
        Preserve3D
    };
    
    void setIndirectCompositingReason(IndirectCompositingReason reason) { m_indirectCompositingReason = static_cast<unsigned>(reason); }
    IndirectCompositingReason indirectCompositingReason() const { return static_cast<IndirectCompositingReason>(m_indirectCompositingReason); }
    bool mustCompositeForIndirectReasons() const { return m_indirectCompositingReason; }

    // Returns true if z ordering would not change if this layer were a stacking container.
    bool canBeStackingContainer() const;

    friend class RenderLayerBacking;
    friend class RenderLayerCompositor;
    friend class RenderLayerModelObject;

    LayoutUnit overflowTop() const;
    LayoutUnit overflowBottom() const;
    LayoutUnit overflowLeft() const;
    LayoutUnit overflowRight() const;

    IntRect rectForHorizontalScrollbar(const IntRect& borderBoxRect) const;
    IntRect rectForVerticalScrollbar(const IntRect& borderBoxRect) const;

    LayoutUnit verticalScrollbarStart(int minX, int maxX) const;
    LayoutUnit horizontalScrollbarStart(int minX) const;

    bool overflowControlsIntersectRect(const IntRect& localRect) const;

    RenderLayer* hitTestFlowThreadIfRegionForFragments(const LayerFragments&, RenderLayer*, const HitTestRequest&, HitTestResult&,
        const LayoutRect&, const HitTestLocation&,
        const HitTestingTransformState*, double* zOffsetForDescendants,
        double* zOffset, const HitTestingTransformState* unflattenedTransformState, bool depthSortDescendants);
    void paintFlowThreadIfRegionForFragments(const LayerFragments&, GraphicsContext*, const LayerPaintingInfo&, PaintLayerFlags);
    bool mapLayerClipRectsToFragmentationLayer(ClipRects&) const;

    RenderNamedFlowFragment* currentRenderNamedFlowFragment() const;

private:
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

    // If we have no out of flow positioned descendants and no non-descendant
    // appears between our descendants in stacking order, then we may become a
    // stacking context.
    bool m_hasOutOfFlowPositionedDescendant : 1;
    bool m_hasOutOfFlowPositionedDescendantDirty : 1;

    bool m_needsCompositedScrolling : 1;

    // If this is true, then no non-descendant appears between any of our
    // descendants in stacking order. This is one of the requirements of being
    // able to safely become a stacking context.
    bool m_descendantsAreContiguousInStackingOrder : 1;

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

    bool m_3DTransformedDescendantStatusDirty : 1;
    bool m_has3DTransformedDescendant : 1;  // Set on a stacking context layer that has 3D descendants anywhere
                                            // in a preserves3D hierarchy. Hint to do 3D-aware hit testing.
    bool m_hasCompositingDescendant : 1; // In the z-order tree.

    bool m_hasTransformedAncestor : 1;
    bool m_has3DTransformedAncestor : 1;

    unsigned m_indirectCompositingReason : 3;
    unsigned m_viewportConstrainedNotCompositedReason : 2;

#if PLATFORM(IOS)
    bool m_adjustForIOSCaretWhenScrolling : 1;
    bool m_registeredAsTouchEventListenerForScrolling : 1;
    bool m_inUserScroll : 1;
    bool m_requiresScrollBoundsOriginUpdate : 1;
#endif

    bool m_containsDirtyOverlayScrollbars : 1;
    bool m_updatingMarqueePosition : 1;

#if !ASSERT_DISABLED
    bool m_layerListMutationAllowed : 1;
#endif

#if ENABLE(CSS_FILTERS)
    bool m_hasFilterInfo : 1;
#endif

#if ENABLE(CSS_COMPOSITING)
    unsigned m_blendMode : 5;
    bool m_hasNotIsolatedCompositedBlendingDescendants : 1;
    bool m_hasNotIsolatedBlendingDescendants : 1;
    bool m_hasNotIsolatedBlendingDescendantsStatusDirty : 1;
#endif

    RenderLayerModelObject& m_renderer;

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
    std::unique_ptr<Vector<RenderLayer*>> m_posZOrderList;
    std::unique_ptr<Vector<RenderLayer*>> m_negZOrderList;

    // This list contains child layers that cannot create stacking contexts.  For now it is just
    // overflow layers, but that may change in the future.
    std::unique_ptr<Vector<RenderLayer*>> m_normalFlowList;

    std::unique_ptr<ClipRectsCache> m_clipRectsCache;
    
    IntPoint m_cachedOverlayScrollbarOffset;

    std::unique_ptr<RenderMarquee> m_marquee; // Used by layers with overflow:marquee
    
    // Cached normal flow values for absolute positioned elements with static left/top values.
    LayoutUnit m_staticInlinePosition;
    LayoutUnit m_staticBlockPosition;

    std::unique_ptr<TransformationMatrix> m_transform;
    
    // May ultimately be extended to many replicas (with their own paint order).
    RenderPtr<RenderReplica> m_reflection;

    // Renderers to hold our custom scroll corner and resizer.
    RenderPtr<RenderScrollbarPart> m_scrollCorner;
    RenderPtr<RenderScrollbarPart> m_resizer;

    // Pointer to the enclosing RenderLayer that caused us to be paginated. It is 0 if we are not paginated.
    RenderLayer* m_enclosingPaginationLayer;

    IntRect m_blockSelectionGapsBounds;

    std::unique_ptr<RenderLayerBacking> m_backing;

    class FilterInfo;
};

inline void RenderLayer::clearZOrderLists()
{
    ASSERT(!isStackingContainer());

    m_posZOrderList = nullptr;
    m_negZOrderList = nullptr;
}

inline void RenderLayer::updateZOrderLists()
{
    if (!m_zOrderListsDirty)
        return;

    if (!isStackingContainer()) {
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

void makeMatrixRenderable(TransformationMatrix&, bool has3DRendering);

} // namespace WebCore

#ifndef NDEBUG
// Outside the WebCore namespace for ease of invocation from gdb.
void showLayerTree(const WebCore::RenderLayer*);
void showLayerTree(const WebCore::RenderObject*);
#endif

#endif // RenderLayer_h
