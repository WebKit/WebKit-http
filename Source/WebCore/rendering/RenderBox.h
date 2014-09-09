/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2006, 2007 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef RenderBox_h
#define RenderBox_h

#include "RenderBoxModelObject.h"
#include "RenderOverflow.h"
#include "ScrollTypes.h"
#if ENABLE(CSS_SHAPES)
#include "ShapeOutsideInfo.h"
#endif

namespace WebCore {

class InlineElementBox;
class RenderBlockFlow;
class RenderBoxRegionInfo;
class RenderRegion;
struct PaintInfo;

enum SizeType { MainOrPreferredSize, MinSize, MaxSize };
enum AvailableLogicalHeightType { ExcludeMarginBorderPadding, IncludeMarginBorderPadding };
enum OverlayScrollbarSizeRelevancy { IgnoreOverlayScrollbarSize, IncludeOverlayScrollbarSize };

enum ShouldComputePreferred { ComputeActual, ComputePreferred };

class RenderBox : public RenderBoxModelObject {
public:
    virtual ~RenderBox();

    // hasAutoZIndex only returns true if the element is positioned or a flex-item since
    // position:static elements that are not flex-items get their z-index coerced to auto.
    virtual bool requiresLayer() const override
    {
        return isRoot() || isPositioned() || createsGroup() || hasClipPath() || hasOverflowClip()
            || hasTransform() || hasHiddenBackface() || hasReflection() || style().specifiesColumns()
            || !style().hasAutoZIndex();
    }

    virtual bool backgroundIsKnownToBeOpaqueInRect(const LayoutRect& localRect) const override final;

    // Use this with caution! No type checking is done!
    RenderBox* firstChildBox() const;
    RenderBox* lastChildBox() const;

    LayoutUnit x() const { return m_frameRect.x(); }
    LayoutUnit y() const { return m_frameRect.y(); }
    LayoutUnit width() const { return m_frameRect.width(); }
    LayoutUnit height() const { return m_frameRect.height(); }

    int pixelSnappedWidth() const { return m_frameRect.pixelSnappedWidth(); }
    int pixelSnappedHeight() const { return m_frameRect.pixelSnappedHeight(); }

    // These represent your location relative to your container as a physical offset.
    // In layout related methods you almost always want the logical location (e.g. x() and y()).
    LayoutUnit top() const { return topLeftLocation().y(); }
    LayoutUnit left() const { return topLeftLocation().x(); }

    void setX(LayoutUnit x) { m_frameRect.setX(x); }
    void setY(LayoutUnit y) { m_frameRect.setY(y); }
    void setWidth(LayoutUnit width) { m_frameRect.setWidth(width); }
    void setHeight(LayoutUnit height) { m_frameRect.setHeight(height); }

    LayoutUnit logicalLeft() const { return style().isHorizontalWritingMode() ? x() : y(); }
    LayoutUnit logicalRight() const { return logicalLeft() + logicalWidth(); }
    LayoutUnit logicalTop() const { return style().isHorizontalWritingMode() ? y() : x(); }
    LayoutUnit logicalBottom() const { return logicalTop() + logicalHeight(); }
    LayoutUnit logicalWidth() const { return style().isHorizontalWritingMode() ? width() : height(); }
    LayoutUnit logicalHeight() const { return style().isHorizontalWritingMode() ? height() : width(); }

    LayoutUnit constrainLogicalWidthInRegionByMinMax(LayoutUnit, LayoutUnit, RenderBlock*, RenderRegion* = 0) const;
    LayoutUnit constrainLogicalHeightByMinMax(LayoutUnit) const;
    LayoutUnit constrainContentBoxLogicalHeightByMinMax(LayoutUnit) const;

    int pixelSnappedLogicalHeight() const { return style().isHorizontalWritingMode() ? pixelSnappedHeight() : pixelSnappedWidth(); }
    int pixelSnappedLogicalWidth() const { return style().isHorizontalWritingMode() ? pixelSnappedWidth() : pixelSnappedHeight(); }

    void setLogicalLeft(LayoutUnit left)
    {
        if (style().isHorizontalWritingMode())
            setX(left);
        else
            setY(left);
    }
    void setLogicalTop(LayoutUnit top)
    {
        if (style().isHorizontalWritingMode())
            setY(top);
        else
            setX(top);
    }
    void setLogicalLocation(const LayoutPoint& location)
    {
        if (style().isHorizontalWritingMode())
            setLocation(location);
        else
            setLocation(location.transposedPoint());
    }
    void setLogicalWidth(LayoutUnit size)
    {
        if (style().isHorizontalWritingMode())
            setWidth(size);
        else
            setHeight(size);
    }
    void setLogicalHeight(LayoutUnit size)
    {
        if (style().isHorizontalWritingMode())
            setHeight(size);
        else
            setWidth(size);
    }
    void setLogicalSize(const LayoutSize& size)
    {
        if (style().isHorizontalWritingMode())
            setSize(size);
        else
            setSize(size.transposedSize());
    }

    LayoutPoint location() const { return m_frameRect.location(); }
    LayoutSize locationOffset() const { return LayoutSize(x(), y()); }
    LayoutSize size() const { return m_frameRect.size(); }
    IntSize pixelSnappedSize() const { return m_frameRect.pixelSnappedSize(); }

    void setLocation(const LayoutPoint& location) { m_frameRect.setLocation(location); }
    
    void setSize(const LayoutSize& size) { m_frameRect.setSize(size); }
    void move(LayoutUnit dx, LayoutUnit dy) { m_frameRect.move(dx, dy); }

    LayoutRect frameRect() const { return m_frameRect; }
    IntRect pixelSnappedFrameRect() const { return pixelSnappedIntRect(m_frameRect); }
    void setFrameRect(const LayoutRect& rect) { m_frameRect = rect; }

    LayoutRect marginBoxRect() const
    {
        LayoutRect box = borderBoxRect();
        box.expand(m_marginBox);
        return box;
    }
    LayoutRect borderBoxRect() const { return LayoutRect(LayoutPoint(), size()); }
    LayoutRect paddingBoxRect() const { return LayoutRect(borderLeft(), borderTop(), contentWidth() + paddingLeft() + paddingRight(), contentHeight() + paddingTop() + paddingBottom()); }
    IntRect pixelSnappedBorderBoxRect() const { return IntRect(IntPoint(), m_frameRect.pixelSnappedSize()); }
    virtual IntRect borderBoundingBox() const override final { return pixelSnappedBorderBoxRect(); }

    RoundedRect::Radii borderRadii() const;

    // The content area of the box (excludes padding - and intrinsic padding for table cells, etc... - and border).
    LayoutRect contentBoxRect() const { return LayoutRect(borderLeft() + paddingLeft(), borderTop() + paddingTop(), contentWidth(), contentHeight()); }
    // The content box in absolute coords. Ignores transforms.
    IntRect absoluteContentBox() const;
    // The content box converted to absolute coords (taking transforms into account).
    FloatQuad absoluteContentQuad() const;

    // This returns the content area of the box (excluding padding and border). The only difference with contentBoxRect is that computedCSSContentBoxRect
    // does include the intrinsic padding in the content box as this is what some callers expect (like getComputedStyle).
    LayoutRect computedCSSContentBoxRect() const { return LayoutRect(borderLeft() + computedCSSPaddingLeft(), borderTop() + computedCSSPaddingTop(), clientWidth() - computedCSSPaddingLeft() - computedCSSPaddingRight(), clientHeight() - computedCSSPaddingTop() - computedCSSPaddingBottom()); }

    // Bounds of the outline box in absolute coords. Respects transforms
    virtual LayoutRect outlineBoundsForRepaint(const RenderLayerModelObject* /*repaintContainer*/, const RenderGeometryMap*) const override final;
    virtual void addFocusRingRects(Vector<IntRect>&, const LayoutPoint& additionalOffset, const RenderLayerModelObject* paintContainer = 0) override;

    // Use this with caution! No type checking is done!
    RenderBox* previousSiblingBox() const;
    RenderBox* nextSiblingBox() const;
    RenderBox* parentBox() const;

    // Visual and layout overflow are in the coordinate space of the box.  This means that they aren't purely physical directions.
    // For horizontal-tb and vertical-lr they will match physical directions, but for horizontal-bt and vertical-rl, the top/bottom and left/right
    // respectively are flipped when compared to their physical counterparts.  For example minX is on the left in vertical-lr,
    // but it is on the right in vertical-rl.
    LayoutRect flippedClientBoxRect() const;
    LayoutRect layoutOverflowRect() const { return m_overflow ? m_overflow->layoutOverflowRect() : flippedClientBoxRect(); }
    LayoutUnit logicalLeftLayoutOverflow() const { return style().isHorizontalWritingMode() ? layoutOverflowRect().x() : layoutOverflowRect().y(); }
    LayoutUnit logicalRightLayoutOverflow() const { return style().isHorizontalWritingMode() ? layoutOverflowRect().maxX() : layoutOverflowRect().maxY(); }
    
    virtual LayoutRect visualOverflowRect() const { return m_overflow ? m_overflow->visualOverflowRect() : borderBoxRect(); }
    LayoutUnit logicalLeftVisualOverflow() const { return style().isHorizontalWritingMode() ? visualOverflowRect().x() : visualOverflowRect().y(); }
    LayoutUnit logicalRightVisualOverflow() const { return style().isHorizontalWritingMode() ? visualOverflowRect().maxX() : visualOverflowRect().maxY(); }

    LayoutRect overflowRectForPaintRejection(RenderNamedFlowFragment*) const;
    
    void addLayoutOverflow(const LayoutRect&);
    void addVisualOverflow(const LayoutRect&);
    void clearOverflow();
    
    virtual bool isTopLayoutOverflowAllowed() const { return !style().isLeftToRightDirection() && !isHorizontalWritingMode(); }
    virtual bool isLeftLayoutOverflowAllowed() const { return !style().isLeftToRightDirection() && isHorizontalWritingMode(); }
    
    void addVisualEffectOverflow();
    LayoutRect applyVisualEffectOverflow(const LayoutRect&) const;
    void addOverflowFromChild(RenderBox* child) { addOverflowFromChild(child, child->locationOffset()); }
    void addOverflowFromChild(RenderBox* child, const LayoutSize& delta);
    
    void updateLayerTransform();

    LayoutUnit contentWidth() const { return clientWidth() - paddingLeft() - paddingRight(); }
    LayoutUnit contentHeight() const { return clientHeight() - paddingTop() - paddingBottom(); }
    LayoutUnit contentLogicalWidth() const { return style().isHorizontalWritingMode() ? contentWidth() : contentHeight(); }
    LayoutUnit contentLogicalHeight() const { return style().isHorizontalWritingMode() ? contentHeight() : contentWidth(); }

    // IE extensions. Used to calculate offsetWidth/Height.  Overridden by inlines (RenderFlow)
    // to return the remaining width on a given line (and the height of a single line).
    virtual LayoutUnit offsetWidth() const override { return width(); }
    virtual LayoutUnit offsetHeight() const override { return height(); }

    virtual int pixelSnappedOffsetWidth() const override final;
    virtual int pixelSnappedOffsetHeight() const override final;

    // More IE extensions.  clientWidth and clientHeight represent the interior of an object
    // excluding border and scrollbar.  clientLeft/Top are just the borderLeftWidth and borderTopWidth.
    LayoutUnit clientLeft() const { return borderLeft(); }
    LayoutUnit clientTop() const { return borderTop(); }
    LayoutUnit clientWidth() const;
    LayoutUnit clientHeight() const;
    LayoutUnit clientLogicalWidth() const { return style().isHorizontalWritingMode() ? clientWidth() : clientHeight(); }
    LayoutUnit clientLogicalHeight() const { return style().isHorizontalWritingMode() ? clientHeight() : clientWidth(); }
    LayoutUnit clientLogicalBottom() const { return borderBefore() + clientLogicalHeight(); }
    LayoutRect clientBoxRect() const { return LayoutRect(clientLeft(), clientTop(), clientWidth(), clientHeight()); }

    int pixelSnappedClientWidth() const;
    int pixelSnappedClientHeight() const;

    // scrollWidth/scrollHeight will be the same as clientWidth/clientHeight unless the
    // object has overflow:hidden/scroll/auto specified and also has overflow.
    // scrollLeft/Top return the current scroll position.  These methods are virtual so that objects like
    // textareas can scroll shadow content (but pretend that they are the objects that are
    // scrolling).
    virtual int scrollLeft() const;
    virtual int scrollTop() const;
    virtual int scrollWidth() const;
    virtual int scrollHeight() const;
    virtual void setScrollLeft(int);
    virtual void setScrollTop(int);

    virtual LayoutUnit marginTop() const override { return m_marginBox.top(); }
    virtual LayoutUnit marginBottom() const override { return m_marginBox.bottom(); }
    virtual LayoutUnit marginLeft() const override { return m_marginBox.left(); }
    virtual LayoutUnit marginRight() const override { return m_marginBox.right(); }
    void setMarginTop(LayoutUnit margin) { m_marginBox.setTop(margin); }
    void setMarginBottom(LayoutUnit margin) { m_marginBox.setBottom(margin); }
    void setMarginLeft(LayoutUnit margin) { m_marginBox.setLeft(margin); }
    void setMarginRight(LayoutUnit margin) { m_marginBox.setRight(margin); }

    LayoutUnit marginLogicalLeft() const { return m_marginBox.logicalLeft(style().writingMode()); }
    LayoutUnit marginLogicalRight() const { return m_marginBox.logicalRight(style().writingMode()); }
    
    virtual LayoutUnit marginBefore(const RenderStyle* overrideStyle = 0) const override final { return m_marginBox.before((overrideStyle ? overrideStyle : &style())->writingMode()); }
    virtual LayoutUnit marginAfter(const RenderStyle* overrideStyle = 0) const override final { return m_marginBox.after((overrideStyle ? overrideStyle : &style())->writingMode()); }
    virtual LayoutUnit marginStart(const RenderStyle* overrideStyle = 0) const override final
    {
        const RenderStyle* styleToUse = overrideStyle ? overrideStyle : &style();
        return m_marginBox.start(styleToUse->writingMode(), styleToUse->direction());
    }
    virtual LayoutUnit marginEnd(const RenderStyle* overrideStyle = 0) const override final
    {
        const RenderStyle* styleToUse = overrideStyle ? overrideStyle : &style();
        return m_marginBox.end(styleToUse->writingMode(), styleToUse->direction());
    }
    void setMarginBefore(LayoutUnit value, const RenderStyle* overrideStyle = 0) { m_marginBox.setBefore((overrideStyle ? overrideStyle : &style())->writingMode(), value); }
    void setMarginAfter(LayoutUnit value, const RenderStyle* overrideStyle = 0) { m_marginBox.setAfter((overrideStyle ? overrideStyle : &style())->writingMode(), value); }
    void setMarginStart(LayoutUnit value, const RenderStyle* overrideStyle = 0)
    {
        const RenderStyle* styleToUse = overrideStyle ? overrideStyle : &style();
        m_marginBox.setStart(styleToUse->writingMode(), styleToUse->direction(), value);
    }
    void setMarginEnd(LayoutUnit value, const RenderStyle* overrideStyle = 0)
    {
        const RenderStyle* styleToUse = overrideStyle ? overrideStyle : &style();
        m_marginBox.setEnd(styleToUse->writingMode(), styleToUse->direction(), value);
    }

    // The following five functions are used to implement collapsing margins.
    // All objects know their maximal positive and negative margins.  The
    // formula for computing a collapsed margin is |maxPosMargin| - |maxNegmargin|.
    // For a non-collapsing box, such as a leaf element, this formula will simply return
    // the margin of the element.  Blocks override the maxMarginBefore and maxMarginAfter
    // methods.
    enum MarginSign { PositiveMargin, NegativeMargin };
    virtual bool isSelfCollapsingBlock() const { return false; }
    virtual LayoutUnit collapsedMarginBefore() const { return marginBefore(); }
    virtual LayoutUnit collapsedMarginAfter() const { return marginAfter(); }

    virtual void absoluteRects(Vector<IntRect>&, const LayoutPoint& accumulatedOffset) const override;
    virtual void absoluteQuads(Vector<FloatQuad>&, bool* wasFixed) const override;
    
    LayoutRect reflectionBox() const;
    int reflectionOffset() const;
    // Given a rect in the object's coordinate space, returns the corresponding rect in the reflection.
    LayoutRect reflectedRect(const LayoutRect&) const;

    virtual void layout() override;
    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction) override;

    virtual LayoutUnit minPreferredLogicalWidth() const override;
    virtual LayoutUnit maxPreferredLogicalWidth() const override;

    // FIXME: We should rename these back to overrideLogicalHeight/Width and have them store
    // the border-box height/width like the regular height/width accessors on RenderBox.
    // Right now, these are different than contentHeight/contentWidth because they still
    // include the scrollbar height/width.
    LayoutUnit overrideLogicalContentWidth() const;
    LayoutUnit overrideLogicalContentHeight() const;
    bool hasOverrideHeight() const;
    bool hasOverrideWidth() const;
    void setOverrideLogicalContentHeight(LayoutUnit);
    void setOverrideLogicalContentWidth(LayoutUnit);
    void clearOverrideSize();
    void clearOverrideLogicalContentHeight();
    void clearOverrideLogicalContentWidth();

#if ENABLE(CSS_GRID_LAYOUT)
    LayoutUnit overrideContainingBlockContentLogicalWidth() const;
    LayoutUnit overrideContainingBlockContentLogicalHeight() const;
    bool hasOverrideContainingBlockLogicalWidth() const;
    bool hasOverrideContainingBlockLogicalHeight() const;
    void setOverrideContainingBlockContentLogicalWidth(LayoutUnit);
    void setOverrideContainingBlockContentLogicalHeight(LayoutUnit);
    void clearContainingBlockOverrideSize();
    void clearOverrideContainingBlockContentLogicalHeight();
#endif

    virtual LayoutSize offsetFromContainer(RenderObject*, const LayoutPoint&, bool* offsetDependsOnPoint = 0) const override;
    
    LayoutUnit adjustBorderBoxLogicalWidthForBoxSizing(LayoutUnit width) const;
    LayoutUnit adjustBorderBoxLogicalHeightForBoxSizing(LayoutUnit height) const;
    LayoutUnit adjustContentBoxLogicalWidthForBoxSizing(LayoutUnit width) const;
    LayoutUnit adjustContentBoxLogicalHeightForBoxSizing(LayoutUnit height) const;

    struct ComputedMarginValues {
        ComputedMarginValues()
            : m_before(0)
            , m_after(0)
            , m_start(0)
            , m_end(0)
        {
        }
        LayoutUnit m_before;
        LayoutUnit m_after;
        LayoutUnit m_start;
        LayoutUnit m_end;
    };
    struct LogicalExtentComputedValues {
        LogicalExtentComputedValues()
            : m_extent(0)
            , m_position(0)
        {
        }

        LayoutUnit m_extent;
        LayoutUnit m_position;
        ComputedMarginValues m_margins;
    };
    // Resolve auto margins in the inline direction of the containing block so that objects can be pushed to the start, middle or end
    // of the containing block.
    void computeInlineDirectionMargins(RenderBlock* containingBlock, LayoutUnit containerWidth, LayoutUnit childWidth, LayoutUnit& marginStart, LayoutUnit& marginEnd) const;

    // Used to resolve margins in the containing block's block-flow direction.
    void computeBlockDirectionMargins(const RenderBlock* containingBlock, LayoutUnit& marginBefore, LayoutUnit& marginAfter) const;
    void computeAndSetBlockDirectionMargins(const RenderBlock* containingBlock);

    enum RenderBoxRegionInfoFlags { CacheRenderBoxRegionInfo, DoNotCacheRenderBoxRegionInfo };
    LayoutRect borderBoxRectInRegion(RenderRegion*, RenderBoxRegionInfoFlags = CacheRenderBoxRegionInfo) const;
    LayoutRect clientBoxRectInRegion(RenderRegion*) const;
    RenderRegion* clampToStartAndEndRegions(RenderRegion*) const;
    bool hasRegionRangeInFlowThread() const;
    virtual LayoutUnit offsetFromLogicalTopOfFirstPage() const;
    
    void positionLineBox(InlineElementBox&);

    virtual std::unique_ptr<InlineElementBox> createInlineBox();
    void dirtyLineBoxes(bool fullLayout);

    // For inline replaced elements, this function returns the inline box that owns us.  Enables
    // the replaced RenderObject to quickly determine what line it is contained on and to easily
    // iterate over structures on the line.
    InlineElementBox* inlineBoxWrapper() const { return m_inlineBoxWrapper; }
    void setInlineBoxWrapper(InlineElementBox*);
    void deleteLineBoxWrapper();

    virtual LayoutRect clippedOverflowRectForRepaint(const RenderLayerModelObject* repaintContainer) const override;
    virtual void computeRectForRepaint(const RenderLayerModelObject* repaintContainer, LayoutRect&, bool fixed = false) const override;
    void repaintDuringLayoutIfMoved(const LayoutRect&);
    virtual void repaintOverhangingFloats(bool paintAllDescendants);

    virtual LayoutUnit containingBlockLogicalWidthForContent() const override;
    LayoutUnit containingBlockLogicalHeightForContent(AvailableLogicalHeightType) const;

    LayoutUnit containingBlockLogicalWidthForContentInRegion(RenderRegion*) const;
    LayoutUnit containingBlockAvailableLineWidthInRegion(RenderRegion*) const;
    LayoutUnit perpendicularContainingBlockLogicalHeight() const;

    virtual void updateLogicalWidth();
    virtual void updateLogicalHeight();
    virtual void computeLogicalHeight(LayoutUnit logicalHeight, LayoutUnit logicalTop, LogicalExtentComputedValues&) const;

    RenderBoxRegionInfo* renderBoxRegionInfo(RenderRegion*, RenderBoxRegionInfoFlags = CacheRenderBoxRegionInfo) const;
    void computeLogicalWidthInRegion(LogicalExtentComputedValues&, RenderRegion* = 0) const;

    bool stretchesToViewport() const
    {
        return document().inQuirksMode() && style().logicalHeight().isAuto() && !isFloatingOrOutOfFlowPositioned() && (isRoot() || isBody()) && !isInline();
    }

    virtual LayoutSize intrinsicSize() const { return LayoutSize(); }
    LayoutUnit intrinsicLogicalWidth() const { return style().isHorizontalWritingMode() ? intrinsicSize().width() : intrinsicSize().height(); }
    LayoutUnit intrinsicLogicalHeight() const { return style().isHorizontalWritingMode() ? intrinsicSize().height() : intrinsicSize().width(); }

    // Whether or not the element shrinks to its intrinsic width (rather than filling the width
    // of a containing block).  HTML4 buttons, <select>s, <input>s, legends, and floating/compact elements do this.
    bool sizesLogicalWidthToFitContent(SizeType) const;

    LayoutUnit shrinkLogicalWidthToAvoidFloats(LayoutUnit childMarginStart, LayoutUnit childMarginEnd, const RenderBlock* cb, RenderRegion*) const;

    LayoutUnit computeLogicalWidthInRegionUsing(SizeType, Length logicalWidth, LayoutUnit availableLogicalWidth, const RenderBlock* containingBlock, RenderRegion*) const;
    LayoutUnit computeLogicalHeightUsing(const Length& height) const;
    LayoutUnit computeContentLogicalHeight(const Length& height) const;
    LayoutUnit computeContentAndScrollbarLogicalHeightUsing(const Length& height) const;
    LayoutUnit computeReplacedLogicalWidthUsing(Length width) const;
    LayoutUnit computeReplacedLogicalWidthRespectingMinMaxWidth(LayoutUnit logicalWidth, ShouldComputePreferred  = ComputeActual) const;
    LayoutUnit computeReplacedLogicalHeightUsing(Length height) const;
    LayoutUnit computeReplacedLogicalHeightRespectingMinMaxHeight(LayoutUnit logicalHeight) const;

    virtual LayoutUnit computeReplacedLogicalWidth(ShouldComputePreferred  = ComputeActual) const;
    virtual LayoutUnit computeReplacedLogicalHeight() const;

    static bool percentageLogicalHeightIsResolvableFromBlock(const RenderBlock* containingBlock, bool outOfFlowPositioned);
    LayoutUnit computePercentageLogicalHeight(const Length& height) const;

    virtual LayoutUnit availableLogicalWidth() const { return contentLogicalWidth(); }
    virtual LayoutUnit availableLogicalHeight(AvailableLogicalHeightType) const;
    LayoutUnit availableLogicalHeightUsing(const Length&, AvailableLogicalHeightType) const;
    
    // There are a few cases where we need to refer specifically to the available physical width and available physical height.
    // Relative positioning is one of those cases, since left/top offsets are physical.
    LayoutUnit availableWidth() const { return style().isHorizontalWritingMode() ? availableLogicalWidth() : availableLogicalHeight(IncludeMarginBorderPadding); }
    LayoutUnit availableHeight() const { return style().isHorizontalWritingMode() ? availableLogicalHeight(IncludeMarginBorderPadding) : availableLogicalWidth(); }

    virtual int verticalScrollbarWidth() const;
    int horizontalScrollbarHeight() const;
    int instrinsicScrollbarLogicalWidth() const;
    int scrollbarLogicalHeight() const { return style().isHorizontalWritingMode() ? horizontalScrollbarHeight() : verticalScrollbarWidth(); }
    virtual bool scroll(ScrollDirection, ScrollGranularity, float multiplier = 1, Element** stopElement = nullptr, RenderBox* startBox = nullptr, const IntPoint& wheelEventAbsolutePoint = IntPoint());
    virtual bool logicalScroll(ScrollLogicalDirection, ScrollGranularity, float multiplier = 1, Element** stopElement = 0);
    bool canBeScrolledAndHasScrollableArea() const;
    virtual bool canBeProgramaticallyScrolled() const;
    virtual void autoscroll(const IntPoint&);
    bool canAutoscroll() const;
    IntSize calculateAutoscrollDirection(const IntPoint& windowPoint) const;
    static RenderBox* findAutoscrollable(RenderObject*);
    virtual void stopAutoscroll() { }
    virtual void panScroll(const IntPoint&);

    bool hasAutoVerticalScrollbar() const { return hasOverflowClip() && (style().overflowY() == OAUTO || style().overflowY() == OOVERLAY); }
    bool hasAutoHorizontalScrollbar() const { return hasOverflowClip() && (style().overflowX() == OAUTO || style().overflowX() == OOVERLAY); }
    bool scrollsOverflow() const { return scrollsOverflowX() || scrollsOverflowY(); }
    bool scrollsOverflowX() const { return hasOverflowClip() && (style().overflowX() == OSCROLL || hasAutoHorizontalScrollbar()); }
    bool scrollsOverflowY() const { return hasOverflowClip() && (style().overflowY() == OSCROLL || hasAutoVerticalScrollbar()); }
    bool hasScrollableOverflowX() const { return scrollsOverflowX() && scrollWidth() != clientWidth(); }
    bool hasScrollableOverflowY() const { return scrollsOverflowY() && scrollHeight() != clientHeight(); }

    bool usesCompositedScrolling() const;
    
    bool hasUnsplittableScrollingOverflow() const;
    bool isUnsplittableForPagination() const;

    virtual LayoutRect localCaretRect(InlineBox*, int caretOffset, LayoutUnit* extraWidthToEndOfLine = 0) override;

    virtual LayoutRect overflowClipRect(const LayoutPoint& location, RenderRegion*, OverlayScrollbarSizeRelevancy = IgnoreOverlayScrollbarSize, PaintPhase = PaintPhaseBlockBackground);
    virtual LayoutRect overflowClipRectForChildLayers(const LayoutPoint& location, RenderRegion* region, OverlayScrollbarSizeRelevancy relevancy) { return overflowClipRect(location, region, relevancy); }
    LayoutRect clipRect(const LayoutPoint& location, RenderRegion*);
    virtual bool hasControlClip() const { return false; }
    virtual LayoutRect controlClipRect(const LayoutPoint&) const { return LayoutRect(); }
    bool pushContentsClip(PaintInfo&, const LayoutPoint& accumulatedOffset);
    void popContentsClip(PaintInfo&, PaintPhase originalPhase, const LayoutPoint& accumulatedOffset);

    virtual void paintObject(PaintInfo&, const LayoutPoint&) { ASSERT_NOT_REACHED(); }
    virtual void paintBoxDecorations(PaintInfo&, const LayoutPoint&);
    virtual void paintMask(PaintInfo&, const LayoutPoint&);
    virtual void imageChanged(WrappedImagePtr, const IntRect* = 0) override;

    // Called when a positioned object moves but doesn't necessarily change size.  A simplified layout is attempted
    // that just updates the object's position. If the size does change, the object remains dirty.
    bool tryLayoutDoingPositionedMovementOnly()
    {
        LayoutUnit oldWidth = width();
        updateLogicalWidth();
        // If we shrink to fit our width may have changed, so we still need full layout.
        if (oldWidth != width())
            return false;
        updateLogicalHeight();
        return true;
    }

    LayoutRect maskClipRect();

    virtual VisiblePosition positionForPoint(const LayoutPoint&, const RenderRegion*) override;

    RenderBlockFlow* outermostBlockContainingFloatingObject();

    void removeFloatingOrPositionedChildFromBlockLists();
    
    RenderLayer* enclosingFloatPaintingLayer() const;
    
    virtual int firstLineBaseline() const { return -1; }
    virtual int inlineBlockBaseline(LineDirectionMode) const { return -1; } // Returns -1 if we should skip this box when computing the baseline of an inline-block.

    bool shrinkToAvoidFloats() const;
    virtual bool avoidsFloats() const;

    virtual void markForPaginationRelayoutIfNeeded() { }

    bool isWritingModeRoot() const { return !parent() || parent()->style().writingMode() != style().writingMode(); }

    bool isDeprecatedFlexItem() const { return !isInline() && !isFloatingOrOutOfFlowPositioned() && parent() && parent()->isDeprecatedFlexibleBox(); }
    bool isFlexItemIncludingDeprecated() const { return !isInline() && !isFloatingOrOutOfFlowPositioned() && parent() && parent()->isFlexibleBoxIncludingDeprecated(); }
    
    virtual LayoutUnit lineHeight(bool firstLine, LineDirectionMode, LinePositionMode = PositionOnContainingLine) const override;
    virtual int baselinePosition(FontBaseline, bool firstLine, LineDirectionMode, LinePositionMode = PositionOnContainingLine) const override;

    virtual LayoutUnit offsetLeft() const override;
    virtual LayoutUnit offsetTop() const override;

    LayoutPoint flipForWritingModeForChild(const RenderBox* child, const LayoutPoint&) const;
    LayoutUnit flipForWritingMode(LayoutUnit position) const; // The offset is in the block direction (y for horizontal writing modes, x for vertical writing modes).
    LayoutPoint flipForWritingMode(const LayoutPoint&) const;
    LayoutSize flipForWritingMode(const LayoutSize&) const;
    void flipForWritingMode(LayoutRect&) const;
    FloatPoint flipForWritingMode(const FloatPoint&) const;
    void flipForWritingMode(FloatRect&) const;
    // These represent your location relative to your container as a physical offset.
    // In layout related methods you almost always want the logical location (e.g. x() and y()).
    LayoutPoint topLeftLocation() const;
    LayoutSize topLeftLocationOffset() const;

    LayoutRect logicalVisualOverflowRectForPropagation(RenderStyle*) const;
    LayoutRect visualOverflowRectForPropagation(RenderStyle*) const;
    LayoutRect logicalLayoutOverflowRectForPropagation(RenderStyle*) const;
    LayoutRect layoutOverflowRectForPropagation(RenderStyle*) const;

    bool hasRenderOverflow() const { return m_overflow; }    
    bool hasVisualOverflow() const { return m_overflow && !borderBoxRect().contains(m_overflow->visualOverflowRect()); }

    virtual bool needsPreferredWidthsRecalculation() const;
    virtual void computeIntrinsicRatioInformation(FloatSize& /* intrinsicSize */, double& /* intrinsicRatio */) const { }

    IntSize scrolledContentOffset() const;
    LayoutSize cachedSizeForOverflowClip() const;
    void applyCachedClipAndScrollOffsetForRepaint(LayoutRect& paintRect) const;

    virtual bool hasRelativeDimensions() const;
    virtual bool hasRelativeLogicalHeight() const;

    bool hasHorizontalLayoutOverflow() const
    {
        if (!m_overflow)
            return false;

        LayoutRect layoutOverflowRect = m_overflow->layoutOverflowRect();
        flipForWritingMode(layoutOverflowRect);
        return layoutOverflowRect.x() < x() || layoutOverflowRect.maxX() > x() + logicalWidth();
    }

    bool hasVerticalLayoutOverflow() const
    {
        if (!m_overflow)
            return false;

        LayoutRect layoutOverflowRect = m_overflow->layoutOverflowRect();
        flipForWritingMode(layoutOverflowRect);
        return layoutOverflowRect.y() < y() || layoutOverflowRect.maxY() > y() + logicalHeight();
    }

    virtual RenderBox* createAnonymousBoxWithSameTypeAs(const RenderObject*) const
    {
        ASSERT_NOT_REACHED();
        return 0;
    }

    bool hasSameDirectionAs(const RenderBox* object) const { return style().direction() == object->style().direction(); }

#if ENABLE(CSS_SHAPES)
    ShapeOutsideInfo* shapeOutsideInfo() const
    {
        return ShapeOutsideInfo::isEnabledFor(*this) ? ShapeOutsideInfo::info(*this) : nullptr;
    }

    void markShapeOutsideDependentsForLayout()
    {
        if (isFloating())
            removeFloatingOrPositionedChildFromBlockLists();
    }
#endif

    // True if this box can have a range in an outside fragmentation context.
    bool canHaveOutsideRegionRange() const { return !isInFlowRenderFlowThread(); }
    virtual bool needsLayoutAfterRegionRangeChange() const { return false; }

protected:
    RenderBox(Element&, PassRef<RenderStyle>, unsigned baseTypeFlags);
    RenderBox(Document&, PassRef<RenderStyle>, unsigned baseTypeFlags);

    virtual void willBeDestroyed() override;

    virtual void styleWillChange(StyleDifference, const RenderStyle& newStyle) override;
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle) override;
    virtual void updateFromStyle() override;

    // Returns false if it could not cheaply compute the extent (e.g. fixed background), in which case the returned rect may be incorrect.
    bool getBackgroundPaintedExtent(LayoutRect&) const;
    virtual bool foregroundIsKnownToBeOpaqueInRect(const LayoutRect& localRect, unsigned maxDepthToTest) const;
    virtual bool computeBackgroundIsKnownToBeObscured() override;

    void paintBackground(const PaintInfo&, const LayoutRect&, BackgroundBleedAvoidance = BackgroundBleedNone);
    
    void paintFillLayer(const PaintInfo&, const Color&, const FillLayer*, const LayoutRect&, BackgroundBleedAvoidance, CompositeOperator, RenderElement* backgroundObject, BaseBackgroundColorUsage = BaseBackgroundColorUse);
    void paintFillLayers(const PaintInfo&, const Color&, const FillLayer*, const LayoutRect&, BackgroundBleedAvoidance = BackgroundBleedNone, CompositeOperator = CompositeSourceOver, RenderElement* backgroundObject = nullptr);

    void paintMaskImages(const PaintInfo&, const LayoutRect&);

    BackgroundBleedAvoidance determineBackgroundBleedAvoidance(GraphicsContext*) const;
    bool backgroundHasOpaqueTopLayer() const;

    void computePositionedLogicalWidth(LogicalExtentComputedValues&, RenderRegion* = 0) const;

    LayoutUnit computeIntrinsicLogicalWidthUsing(Length logicalWidthLength, LayoutUnit availableLogicalWidth, LayoutUnit borderAndPadding) const;
    
    virtual bool shouldComputeSizeAsReplaced() const { return isReplaced() && !isInlineBlockOrInlineTable(); }

    virtual void mapLocalToContainer(const RenderLayerModelObject* repaintContainer, TransformState&, MapCoordinatesFlags = ApplyContainerFlip, bool* wasFixed = 0) const override;
    virtual const RenderObject* pushMappingToContainer(const RenderLayerModelObject*, RenderGeometryMap&) const override;
    virtual void mapAbsoluteToLocalPoint(MapCoordinatesFlags, TransformState&) const override;

    void paintRootBoxFillLayers(const PaintInfo&);

    RenderObject* splitAnonymousBoxesAroundChild(RenderObject* beforeChild);
 
private:
#if ENABLE(CSS_SHAPES)
    void updateShapeOutsideInfoAfterStyleChange(const RenderStyle&, const RenderStyle* oldStyle);
#endif

    bool scrollLayer(ScrollDirection, ScrollGranularity, float multiplier, Element** stopElement);

    bool fixedElementLaysOutRelativeToFrame(const FrameView&) const;

    bool includeVerticalScrollbarSize() const;
    bool includeHorizontalScrollbarSize() const;

    // Returns true if we did a full repaint
    bool repaintLayerRectsForImage(WrappedImagePtr image, const FillLayer* layers, bool drawingBackground);

    bool skipContainingBlockForPercentHeightCalculation(const RenderBox* containingBlock, bool isPerpendicularWritingMode) const;
   
    LayoutUnit containingBlockLogicalWidthForPositioned(const RenderBoxModelObject* containingBlock, RenderRegion* = 0, bool checkForPerpendicularWritingMode = true) const;
    LayoutUnit containingBlockLogicalHeightForPositioned(const RenderBoxModelObject* containingBlock, bool checkForPerpendicularWritingMode = true) const;

    LayoutUnit viewLogicalHeightForPercentages() const;

    void computePositionedLogicalHeight(LogicalExtentComputedValues&) const;
    void computePositionedLogicalWidthUsing(Length logicalWidth, const RenderBoxModelObject* containerBlock, TextDirection containerDirection,
                                            LayoutUnit containerLogicalWidth, LayoutUnit bordersPlusPadding,
                                            Length logicalLeft, Length logicalRight, Length marginLogicalLeft, Length marginLogicalRight,
                                            LogicalExtentComputedValues&) const;
    void computePositionedLogicalHeightUsing(Length logicalHeightLength, const RenderBoxModelObject* containerBlock,
                                             LayoutUnit containerLogicalHeight, LayoutUnit bordersPlusPadding, LayoutUnit logicalHeight,
                                             Length logicalTop, Length logicalBottom, Length marginLogicalTop, Length marginLogicalBottom,
                                             LogicalExtentComputedValues&) const;

    void computePositionedLogicalHeightReplaced(LogicalExtentComputedValues&) const;
    void computePositionedLogicalWidthReplaced(LogicalExtentComputedValues&) const;

    LayoutUnit fillAvailableMeasure(LayoutUnit availableLogicalWidth) const;
    LayoutUnit fillAvailableMeasure(LayoutUnit availableLogicalWidth, LayoutUnit& marginStart, LayoutUnit& marginEnd) const;

    virtual void computeIntrinsicLogicalWidths(LayoutUnit& minLogicalWidth, LayoutUnit& maxLogicalWidth) const;

    // This function calculates the minimum and maximum preferred widths for an object.
    // These values are used in shrink-to-fit layout systems.
    // These include tables, positioned objects, floats and flexible boxes.
    virtual void computePreferredLogicalWidths() { setPreferredLogicalWidthsDirty(false); }

    virtual LayoutRect frameRectForStickyPositioning() const override final { return frameRect(); }

private:
    // The width/height of the contents + borders + padding.  The x/y location is relative to our container (which is not always our parent).
    LayoutRect m_frameRect;

protected:
    LayoutBoxExtent m_marginBox;

    // The preferred logical width of the element if it were to break its lines at every possible opportunity.
    LayoutUnit m_minPreferredLogicalWidth;
    
    // The preferred logical width of the element if it never breaks any lines at all.
    LayoutUnit m_maxPreferredLogicalWidth;

    // For inline replaced elements, the inline box that owns us.
    InlineElementBox* m_inlineBoxWrapper;

    // Our overflow information.
    RefPtr<RenderOverflow> m_overflow;

private:
    // Used to store state between styleWillChange and styleDidChange
    static bool s_hadOverflowClip;
};

RENDER_OBJECT_TYPE_CASTS(RenderBox, isBox())

inline RenderBox* RenderBox::previousSiblingBox() const
{
    return toRenderBox(previousSibling());
}

inline RenderBox* RenderBox::nextSiblingBox() const
{ 
    return toRenderBox(nextSibling());
}

inline RenderBox* RenderBox::parentBox() const
{
    return toRenderBox(parent());
}

inline RenderBox* RenderBox::firstChildBox() const
{
    return toRenderBox(firstChild());
}

inline RenderBox* RenderBox::lastChildBox() const
{
    return toRenderBox(lastChild());
}

inline void RenderBox::setInlineBoxWrapper(InlineElementBox* boxWrapper)
{
    if (boxWrapper) {
        ASSERT(!m_inlineBoxWrapper);
        // m_inlineBoxWrapper should already be 0. Deleting it is a safeguard against security issues.
        // Otherwise, there will two line box wrappers keeping the reference to this renderer, and
        // only one will be notified when the renderer is getting destroyed. The second line box wrapper
        // will keep a stale reference.
        if (UNLIKELY(m_inlineBoxWrapper != 0))
            deleteLineBoxWrapper();
    }

    m_inlineBoxWrapper = boxWrapper;
}

} // namespace WebCore

#endif // RenderBox_h
