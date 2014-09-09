/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2005 Allan Sandfeld Jensen (kde@carewolf.com)
 *           (C) 2005, 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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

#include "config.h"
#include "RenderBox.h"

#include "Chrome.h"
#include "ChromeClient.h"
#include "Document.h"
#include "EventHandler.h"
#include "FloatQuad.h"
#include "FloatRoundedRect.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "HTMLElement.h"
#include "HTMLFrameOwnerElement.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "HTMLTextAreaElement.h"
#include "HitTestResult.h"
#include "InlineElementBox.h"
#include "Page.h"
#include "PaintInfo.h"
#include "RenderBoxRegionInfo.h"
#include "RenderFlexibleBox.h"
#include "RenderGeometryMap.h"
#include "RenderInline.h"
#include "RenderIterator.h"
#include "RenderLayer.h"
#include "RenderLayerCompositor.h"
#include "RenderNamedFlowFragment.h"
#include "RenderNamedFlowThread.h"
#include "RenderTableCell.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "TransformState.h"
#include "htmlediting.h"
#include <algorithm>
#include <math.h>
#include <wtf/StackStats.h>

#if PLATFORM(IOS)
#include "Settings.h"
#endif

namespace WebCore {

struct SameSizeAsRenderBox : public RenderBoxModelObject {
    virtual ~SameSizeAsRenderBox() { }
    LayoutRect frameRect;
    LayoutBoxExtent marginBox;
    LayoutUnit preferredLogicalWidths[2];
    void* pointers[2];
};

COMPILE_ASSERT(sizeof(RenderBox) == sizeof(SameSizeAsRenderBox), RenderBox_should_stay_small);

using namespace HTMLNames;

// Used by flexible boxes when flexing this element and by table cells.
typedef WTF::HashMap<const RenderBox*, LayoutUnit> OverrideSizeMap;
static OverrideSizeMap* gOverrideHeightMap = 0;
static OverrideSizeMap* gOverrideWidthMap = 0;

#if ENABLE(CSS_GRID_LAYOUT)
// Used by grid elements to properly size their grid items.
static OverrideSizeMap* gOverrideContainingBlockLogicalHeightMap = nullptr;
static OverrideSizeMap* gOverrideContainingBlockLogicalWidthMap = nullptr;
#endif

// Size of border belt for autoscroll. When mouse pointer in border belt,
// autoscroll is started.
static const int autoscrollBeltSize = 20;
static const unsigned backgroundObscurationTestMaxDepth = 4;

bool RenderBox::s_hadOverflowClip = false;

static bool skipBodyBackground(const RenderBox* bodyElementRenderer)
{
    ASSERT(bodyElementRenderer->isBody());
    // The <body> only paints its background if the root element has defined a background independent of the body,
    // or if the <body>'s parent is not the document element's renderer (e.g. inside SVG foreignObject).
    auto documentElementRenderer = bodyElementRenderer->document().documentElement()->renderer();
    return documentElementRenderer
        && !documentElementRenderer->hasBackground()
        && (documentElementRenderer == bodyElementRenderer->parent());
}

RenderBox::RenderBox(Element& element, PassRef<RenderStyle> style, unsigned baseTypeFlags)
    : RenderBoxModelObject(element, WTF::move(style), baseTypeFlags)
    , m_minPreferredLogicalWidth(-1)
    , m_maxPreferredLogicalWidth(-1)
    , m_inlineBoxWrapper(0)
{
    setIsBox();
}

RenderBox::RenderBox(Document& document, PassRef<RenderStyle> style, unsigned baseTypeFlags)
    : RenderBoxModelObject(document, WTF::move(style), baseTypeFlags)
    , m_minPreferredLogicalWidth(-1)
    , m_maxPreferredLogicalWidth(-1)
    , m_inlineBoxWrapper(0)
{
    setIsBox();
}

RenderBox::~RenderBox()
{
    view().unscheduleLazyRepaint(*this);
    if (hasControlStatesForRenderer(this))
        removeControlStatesForRenderer(this);
}

RenderRegion* RenderBox::clampToStartAndEndRegions(RenderRegion* region) const
{
    RenderFlowThread* flowThread = flowThreadContainingBlock();

    ASSERT(isRenderView() || (region && flowThread));
    if (isRenderView())
        return region;

    // We need to clamp to the block, since we want any lines or blocks that overflow out of the
    // logical top or logical bottom of the block to size as though the border box in the first and
    // last regions extended infinitely. Otherwise the lines are going to size according to the regions
    // they overflow into, which makes no sense when this block doesn't exist in |region| at all.
    RenderRegion* startRegion = nullptr;
    RenderRegion* endRegion = nullptr;
    if (!flowThread->getRegionRangeForBox(this, startRegion, endRegion))
        return region;

    if (region->logicalTopForFlowThreadContent() < startRegion->logicalTopForFlowThreadContent())
        return startRegion;
    if (region->logicalTopForFlowThreadContent() > endRegion->logicalTopForFlowThreadContent())
        return endRegion;

    return region;
}

bool RenderBox::hasRegionRangeInFlowThread() const
{
    RenderFlowThread* flowThread = flowThreadContainingBlock();
    if (!flowThread || !flowThread->hasValidRegionInfo())
        return false;

    return flowThread->hasCachedRegionRangeForBox(this);
}

LayoutRect RenderBox::clientBoxRectInRegion(RenderRegion* region) const
{
    if (!region)
        return clientBoxRect();

    LayoutRect clientBox = borderBoxRectInRegion(region);
    clientBox.setLocation(clientBox.location() + LayoutSize(borderLeft(), borderTop()));
    clientBox.setSize(clientBox.size() - LayoutSize(borderLeft() + borderRight() + verticalScrollbarWidth(), borderTop() + borderBottom() + horizontalScrollbarHeight()));

    return clientBox;
}

LayoutRect RenderBox::borderBoxRectInRegion(RenderRegion* region, RenderBoxRegionInfoFlags cacheFlag) const
{
    if (!region)
        return borderBoxRect();

    RenderFlowThread* flowThread = flowThreadContainingBlock();
    if (!flowThread)
        return borderBoxRect();

    RenderRegion* startRegion = nullptr;
    RenderRegion* endRegion = nullptr;
    if (!flowThread->getRegionRangeForBox(this, startRegion, endRegion)) {
        // FIXME: In a perfect world this condition should never happen.
        return borderBoxRect();
    }
    
    ASSERT(flowThread->regionInRange(region, startRegion, endRegion));

    // Compute the logical width and placement in this region.
    RenderBoxRegionInfo* boxInfo = renderBoxRegionInfo(region, cacheFlag);
    if (!boxInfo)
        return borderBoxRect();

    // We have cached insets.
    LayoutUnit logicalWidth = boxInfo->logicalWidth();
    LayoutUnit logicalLeft = boxInfo->logicalLeft();
        
    // Now apply the parent inset since it is cumulative whenever anything in the containing block chain shifts.
    // FIXME: Doesn't work right with perpendicular writing modes.
    const RenderBlock* currentBox = containingBlock();
    RenderBoxRegionInfo* currentBoxInfo = currentBox->renderBoxRegionInfo(region);
    while (currentBoxInfo && currentBoxInfo->isShifted()) {
        if (currentBox->style().direction() == LTR)
            logicalLeft += currentBoxInfo->logicalLeft();
        else
            logicalLeft -= (currentBox->logicalWidth() - currentBoxInfo->logicalWidth()) - currentBoxInfo->logicalLeft();
        currentBox = currentBox->containingBlock();
        region = currentBox->clampToStartAndEndRegions(region);
        currentBoxInfo = currentBox->renderBoxRegionInfo(region);
    }
    
    if (cacheFlag == DoNotCacheRenderBoxRegionInfo)
        delete boxInfo;

    if (isHorizontalWritingMode())
        return LayoutRect(logicalLeft, 0, logicalWidth, height());
    return LayoutRect(0, logicalLeft, width(), logicalWidth);
}

void RenderBox::willBeDestroyed()
{
    if (frame().eventHandler().autoscrollRenderer() == this)
        frame().eventHandler().stopAutoscrollTimer(true);

    clearOverrideSize();
#if ENABLE(CSS_GRID_LAYOUT)
    clearContainingBlockOverrideSize();
#endif

    RenderBlock::removePercentHeightDescendantIfNeeded(*this);

#if ENABLE(CSS_SHAPES)
    ShapeOutsideInfo::removeInfo(*this);
#endif

    RenderBoxModelObject::willBeDestroyed();
}

RenderBlockFlow* RenderBox::outermostBlockContainingFloatingObject()
{
    ASSERT(isFloating());
    RenderBlockFlow* parentBlock = nullptr;
    for (auto& ancestor : ancestorsOfType<RenderBlockFlow>(*this)) {
        if (ancestor.isRenderView())
            break;
        if (!parentBlock || ancestor.containsFloat(*this))
            parentBlock = &ancestor;
    }
    return parentBlock;
}

void RenderBox::removeFloatingOrPositionedChildFromBlockLists()
{
    ASSERT(isFloatingOrOutOfFlowPositioned());

    if (documentBeingDestroyed())
        return;

    if (isFloating()) {
        if (RenderBlockFlow* parentBlock = outermostBlockContainingFloatingObject()) {
            parentBlock->markSiblingsWithFloatsForLayout(this);
            parentBlock->markAllDescendantsWithFloatsForLayout(this, false);
        }
    }

    if (isOutOfFlowPositioned())
        RenderBlock::removePositionedObject(*this);
}

void RenderBox::styleWillChange(StyleDifference diff, const RenderStyle& newStyle)
{
    s_hadOverflowClip = hasOverflowClip();

    const RenderStyle* oldStyle = hasInitializedStyle() ? &style() : nullptr;
    if (oldStyle) {
        // The background of the root element or the body element could propagate up to
        // the canvas. Issue full repaint, when our style changes substantially.
        if (diff >= StyleDifferenceRepaint && (isRoot() || isBody())) {
            view().repaintRootContents();
            if (oldStyle->hasEntirelyFixedBackground() != newStyle.hasEntirelyFixedBackground())
                view().compositor().rootFixedBackgroundsChanged();
        }
        
        // When a layout hint happens and an object's position style changes, we have to do a layout
        // to dirty the render tree using the old position value now.
        if (diff == StyleDifferenceLayout && parent() && oldStyle->position() != newStyle.position()) {
            markContainingBlocksForLayout();
            if (oldStyle->position() == StaticPosition)
                repaint();
            else if (newStyle.hasOutOfFlowPosition())
                parent()->setChildNeedsLayout();
            if (isFloating() && !isOutOfFlowPositioned() && newStyle.hasOutOfFlowPosition())
                removeFloatingOrPositionedChildFromBlockLists();
        }
    } else if (isBody())
        view().repaintRootContents();

    RenderBoxModelObject::styleWillChange(diff, newStyle);
}

void RenderBox::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    // Horizontal writing mode definition is updated in RenderBoxModelObject::updateFromStyle,
    // (as part of the RenderBoxModelObject::styleDidChange call below). So, we can safely cache the horizontal
    // writing mode value before style change here.
    bool oldHorizontalWritingMode = isHorizontalWritingMode();

    RenderBoxModelObject::styleDidChange(diff, oldStyle);

    const RenderStyle& newStyle = style();
    if (needsLayout() && oldStyle) {
        RenderBlock::removePercentHeightDescendantIfNeeded(*this);

        // Normally we can do optimized positioning layout for absolute/fixed positioned objects. There is one special case, however, which is
        // when the positioned object's margin-before is changed. In this case the parent has to get a layout in order to run margin collapsing
        // to determine the new static position.
        if (isOutOfFlowPositioned() && newStyle.hasStaticBlockPosition(isHorizontalWritingMode()) && oldStyle->marginBefore() != newStyle.marginBefore()
            && parent() && !parent()->normalChildNeedsLayout())
            parent()->setChildNeedsLayout();
    }

    if (RenderBlock::hasPercentHeightContainerMap() && firstChild()
        && oldHorizontalWritingMode != isHorizontalWritingMode())
        RenderBlock::clearPercentHeightDescendantsFrom(*this);

    // If our zoom factor changes and we have a defined scrollLeft/Top, we need to adjust that value into the
    // new zoomed coordinate space.
    if (hasOverflowClip() && oldStyle && oldStyle->effectiveZoom() != newStyle.effectiveZoom()) {
        if (int left = layer()->scrollXOffset()) {
            left = (left / oldStyle->effectiveZoom()) * newStyle.effectiveZoom();
            layer()->scrollToXOffset(left);
        }
        if (int top = layer()->scrollYOffset()) {
            top = (top / oldStyle->effectiveZoom()) * newStyle.effectiveZoom();
            layer()->scrollToYOffset(top);
        }
    }

    // Our opaqueness might have changed without triggering layout.
    if (diff >= StyleDifferenceRepaint && diff <= StyleDifferenceRepaintLayer) {
        auto parentToInvalidate = parent();
        for (unsigned i = 0; i < backgroundObscurationTestMaxDepth && parentToInvalidate; ++i) {
            parentToInvalidate->invalidateBackgroundObscurationStatus();
            parentToInvalidate = parentToInvalidate->parent();
        }
    }

    bool isBodyRenderer = isBody();
    bool isRootRenderer = isRoot();

    // Set the text color if we're the body.
    if (isBodyRenderer)
        document().setTextColor(newStyle.visitedDependentColor(CSSPropertyColor));

    if (isRootRenderer || isBodyRenderer) {
        // Propagate the new writing mode and direction up to the RenderView.
        RenderStyle& viewStyle = view().style();
        bool viewChangedWritingMode = false;
        bool rootStyleChanged = false;
        bool viewStyleChanged = false;
        RenderObject* rootRenderer = isBodyRenderer ? document().documentElement()->renderer() : nullptr;
        if (viewStyle.direction() != newStyle.direction() && (isRootRenderer || !document().directionSetOnDocumentElement())) {
            viewStyle.setDirection(newStyle.direction());
            viewStyleChanged = true;
            if (isBodyRenderer) {
                rootRenderer->style().setDirection(newStyle.direction());
                rootStyleChanged = true;
            }
            setNeedsLayoutAndPrefWidthsRecalc();
        }

        if (viewStyle.writingMode() != newStyle.writingMode() && (isRootRenderer || !document().writingModeSetOnDocumentElement())) {
            viewStyle.setWritingMode(newStyle.writingMode());
            viewChangedWritingMode = true;
            viewStyleChanged = true;
            view().setHorizontalWritingMode(newStyle.isHorizontalWritingMode());
            view().markAllDescendantsWithFloatsForLayout();
            if (isBodyRenderer) {
                rootStyleChanged = true;
                rootRenderer->style().setWritingMode(newStyle.writingMode());
                rootRenderer->setHorizontalWritingMode(newStyle.isHorizontalWritingMode());
            }
            setNeedsLayoutAndPrefWidthsRecalc();
        }

        view().frameView().recalculateScrollbarOverlayStyle();
        
        const Pagination& pagination = view().frameView().pagination();
        if (viewChangedWritingMode && pagination.mode != Pagination::Unpaginated) {
            viewStyle.setColumnStylesFromPaginationMode(pagination.mode);
            if (view().multiColumnFlowThread())
                view().updateColumnProgressionFromStyle(&viewStyle);
        }
        
        if (viewStyleChanged && view().multiColumnFlowThread())
            view().updateStylesForColumnChildren();
        
        if (rootStyleChanged && rootRenderer && rootRenderer->isRenderBlockFlow() && toRenderBlockFlow(rootRenderer)->multiColumnFlowThread())
            toRenderBlockFlow(rootRenderer)->updateStylesForColumnChildren();
    }

#if ENABLE(CSS_SHAPES)
    if ((oldStyle && oldStyle->shapeOutside()) || style().shapeOutside())
        updateShapeOutsideInfoAfterStyleChange(style(), oldStyle);
#endif
}

#if ENABLE(CSS_SHAPES)
void RenderBox::updateShapeOutsideInfoAfterStyleChange(const RenderStyle& style, const RenderStyle* oldStyle)
{
    const ShapeValue* shapeOutside = style.shapeOutside();
    const ShapeValue* oldShapeOutside = oldStyle ? oldStyle->shapeOutside() : nullptr;

    Length shapeMargin = style.shapeMargin();
    Length oldShapeMargin = oldStyle ? oldStyle->shapeMargin() : RenderStyle::initialShapeMargin();

    float shapeImageThreshold = style.shapeImageThreshold();
    float oldShapeImageThreshold = oldStyle ? oldStyle->shapeImageThreshold() : RenderStyle::initialShapeImageThreshold();

    // FIXME: A future optimization would do a deep comparison for equality. (bug 100811)
    if (shapeOutside == oldShapeOutside && shapeMargin == oldShapeMargin && shapeImageThreshold == oldShapeImageThreshold)
        return;

    if (!shapeOutside)
        ShapeOutsideInfo::removeInfo(*this);
    else
        ShapeOutsideInfo::ensureInfo(*this).markShapeAsDirty();

    if (shapeOutside || shapeOutside != oldShapeOutside)
        markShapeOutsideDependentsForLayout();
}
#endif

void RenderBox::updateFromStyle()
{
    RenderBoxModelObject::updateFromStyle();

    const RenderStyle& styleToUse = style();
    bool isRootObject = isRoot();
    bool isViewObject = isRenderView();

    // The root and the RenderView always paint their backgrounds/borders.
    if (isRootObject || isViewObject)
        setHasBoxDecorations(true);

    setFloating(!isOutOfFlowPositioned() && styleToUse.isFloating());

    // We also handle <body> and <html>, whose overflow applies to the viewport.
    if (styleToUse.overflowX() != OVISIBLE && !isRootObject && isRenderBlock()) {
        bool boxHasOverflowClip = true;
        if (isBody()) {
            // Overflow on the body can propagate to the viewport under the following conditions.
            // (1) The root element is <html>.
            // (2) We are the primary <body> (can be checked by looking at document.body).
            // (3) The root element has visible overflow.
            if (document().documentElement()->hasTagName(htmlTag)
                && document().body() == element()
                && document().documentElement()->renderer()->style().overflowX() == OVISIBLE) {
                boxHasOverflowClip = false;
            }
        }
        
        // Check for overflow clip.
        // It's sufficient to just check one direction, since it's illegal to have visible on only one overflow value.
        if (boxHasOverflowClip) {
            if (!s_hadOverflowClip)
                // Erase the overflow
                repaint();
            setHasOverflowClip();
        }
    }

    setHasTransform(styleToUse.hasTransformRelatedProperty());
    setHasReflection(styleToUse.boxReflect());
}

void RenderBox::layout()
{
    StackStats::LayoutCheckPoint layoutCheckPoint;
    ASSERT(needsLayout());

    RenderObject* child = firstChild();
    if (!child) {
        clearNeedsLayout();
        return;
    }

    LayoutStateMaintainer statePusher(view(), *this, locationOffset(), style().isFlippedBlocksWritingMode());
    while (child) {
        if (child->needsLayout())
            toRenderElement(child)->layout();
        ASSERT(!child->needsLayout());
        child = child->nextSibling();
    }
    statePusher.pop();
    invalidateBackgroundObscurationStatus();
    clearNeedsLayout();
}

// More IE extensions.  clientWidth and clientHeight represent the interior of an object
// excluding border and scrollbar.
LayoutUnit RenderBox::clientWidth() const
{
    return width() - borderLeft() - borderRight() - verticalScrollbarWidth();
}

LayoutUnit RenderBox::clientHeight() const
{
    return height() - borderTop() - borderBottom() - horizontalScrollbarHeight();
}

int RenderBox::pixelSnappedClientWidth() const
{
    return snapSizeToPixel(clientWidth(), x() + clientLeft());
}

int RenderBox::pixelSnappedClientHeight() const
{
    return snapSizeToPixel(clientHeight(), y() + clientTop());
}

int RenderBox::pixelSnappedOffsetWidth() const
{
    return snapSizeToPixel(offsetWidth(), x() + clientLeft());
}

int RenderBox::pixelSnappedOffsetHeight() const
{
    return snapSizeToPixel(offsetHeight(), y() + clientTop());
}

int RenderBox::scrollWidth() const
{
    if (hasOverflowClip())
        return layer()->scrollWidth();
    // For objects with visible overflow, this matches IE.
    // FIXME: Need to work right with writing modes.
    if (style().isLeftToRightDirection())
        return snapSizeToPixel(std::max(clientWidth(), layoutOverflowRect().maxX() - borderLeft()), x() + clientLeft());
    return clientWidth() - std::min<LayoutUnit>(0, layoutOverflowRect().x() - borderLeft());
}

int RenderBox::scrollHeight() const
{
    if (hasOverflowClip())
        return layer()->scrollHeight();
    // For objects with visible overflow, this matches IE.
    // FIXME: Need to work right with writing modes.
    return snapSizeToPixel(std::max(clientHeight(), layoutOverflowRect().maxY() - borderTop()), y() + clientTop());
}

int RenderBox::scrollLeft() const
{
    return hasOverflowClip() ? layer()->scrollXOffset() : 0;
}

int RenderBox::scrollTop() const
{
    return hasOverflowClip() ? layer()->scrollYOffset() : 0;
}

void RenderBox::setScrollLeft(int newLeft)
{
    if (hasOverflowClip())
        layer()->scrollToXOffset(newLeft, RenderLayer::ScrollOffsetClamped);
}

void RenderBox::setScrollTop(int newTop)
{
    if (hasOverflowClip())
        layer()->scrollToYOffset(newTop, RenderLayer::ScrollOffsetClamped);
}

void RenderBox::absoluteRects(Vector<IntRect>& rects, const LayoutPoint& accumulatedOffset) const
{
    rects.append(pixelSnappedIntRect(accumulatedOffset, size()));
}

void RenderBox::absoluteQuads(Vector<FloatQuad>& quads, bool* wasFixed) const
{
    FloatRect localRect(0, 0, width(), height());

    RenderFlowThread* flowThread = flowThreadContainingBlock();
    if (flowThread && flowThread->absoluteQuadsForBox(quads, wasFixed, this, localRect.y(), localRect.maxY()))
        return;

    quads.append(localToAbsoluteQuad(localRect, 0 /* mode */, wasFixed));
}

void RenderBox::updateLayerTransform()
{
    // Transform-origin depends on box size, so we need to update the layer transform after layout.
    if (hasLayer())
        layer()->updateTransform();
}

LayoutUnit RenderBox::constrainLogicalWidthInRegionByMinMax(LayoutUnit logicalWidth, LayoutUnit availableWidth, RenderBlock* cb, RenderRegion* region) const
{
    const RenderStyle& styleToUse = style();
    if (!styleToUse.logicalMaxWidth().isUndefined())
        logicalWidth = std::min(logicalWidth, computeLogicalWidthInRegionUsing(MaxSize, styleToUse.logicalMaxWidth(), availableWidth, cb, region));
    return std::max(logicalWidth, computeLogicalWidthInRegionUsing(MinSize, styleToUse.logicalMinWidth(), availableWidth, cb, region));
}

LayoutUnit RenderBox::constrainLogicalHeightByMinMax(LayoutUnit logicalHeight) const
{
    const RenderStyle& styleToUse = style();
    if (!styleToUse.logicalMaxHeight().isUndefined()) {
        LayoutUnit maxH = computeLogicalHeightUsing(styleToUse.logicalMaxHeight());
        if (maxH != -1)
            logicalHeight = std::min(logicalHeight, maxH);
    }
    return std::max(logicalHeight, computeLogicalHeightUsing(styleToUse.logicalMinHeight()));
}

LayoutUnit RenderBox::constrainContentBoxLogicalHeightByMinMax(LayoutUnit logicalHeight) const
{
    const RenderStyle& styleToUse = style();
    if (!styleToUse.logicalMaxHeight().isUndefined()) {
        LayoutUnit maxH = computeContentLogicalHeight(styleToUse.logicalMaxHeight());
        if (maxH != -1)
            logicalHeight = std::min(logicalHeight, maxH);
    }
    return std::max(logicalHeight, computeContentLogicalHeight(styleToUse.logicalMinHeight()));
}

RoundedRect::Radii RenderBox::borderRadii() const
{
    RenderStyle& style = this->style();
    LayoutRect bounds = frameRect();

    unsigned borderLeft = style.borderLeftWidth();
    unsigned borderTop = style.borderTopWidth();
    bounds.moveBy(LayoutPoint(borderLeft, borderTop));
    bounds.contract(borderLeft + style.borderRightWidth(), borderTop + style.borderBottomWidth());
    return style.getRoundedBorderFor(bounds).radii();
}

IntRect RenderBox::absoluteContentBox() const
{
    // This is wrong with transforms and flipped writing modes.
    IntRect rect = pixelSnappedIntRect(contentBoxRect());
    FloatPoint absPos = localToAbsolute();
    rect.move(absPos.x(), absPos.y());
    return rect;
}

FloatQuad RenderBox::absoluteContentQuad() const
{
    LayoutRect rect = contentBoxRect();
    return localToAbsoluteQuad(FloatRect(rect));
}

LayoutRect RenderBox::outlineBoundsForRepaint(const RenderLayerModelObject* repaintContainer, const RenderGeometryMap* geometryMap) const
{
    LayoutRect box = borderBoundingBox();
    adjustRectForOutlineAndShadow(box);

    if (repaintContainer != this) {
        FloatQuad containerRelativeQuad;
        if (geometryMap)
            containerRelativeQuad = geometryMap->mapToContainer(box, repaintContainer);
        else
            containerRelativeQuad = localToContainerQuad(FloatRect(box), repaintContainer);

        box = LayoutRect(containerRelativeQuad.boundingBox());
    }
    
    // FIXME: layoutDelta needs to be applied in parts before/after transforms and
    // repaint containers. https://bugs.webkit.org/show_bug.cgi?id=23308
    box.move(view().layoutDelta());

    return LayoutRect(pixelSnappedForPainting(box, document().deviceScaleFactor()));
}

void RenderBox::addFocusRingRects(Vector<IntRect>& rects, const LayoutPoint& additionalOffset, const RenderLayerModelObject*)
{
    if (!size().isEmpty())
        rects.append(pixelSnappedIntRect(additionalOffset, size()));
}

LayoutRect RenderBox::reflectionBox() const
{
    LayoutRect result;
    if (!style().boxReflect())
        return result;
    LayoutRect box = borderBoxRect();
    result = box;
    switch (style().boxReflect()->direction()) {
        case ReflectionBelow:
            result.move(0, box.height() + reflectionOffset());
            break;
        case ReflectionAbove:
            result.move(0, -box.height() - reflectionOffset());
            break;
        case ReflectionLeft:
            result.move(-box.width() - reflectionOffset(), 0);
            break;
        case ReflectionRight:
            result.move(box.width() + reflectionOffset(), 0);
            break;
    }
    return result;
}

int RenderBox::reflectionOffset() const
{
    if (!style().boxReflect())
        return 0;
    if (style().boxReflect()->direction() == ReflectionLeft || style().boxReflect()->direction() == ReflectionRight)
        return valueForLength(style().boxReflect()->offset(), borderBoxRect().width());
    return valueForLength(style().boxReflect()->offset(), borderBoxRect().height());
}

LayoutRect RenderBox::reflectedRect(const LayoutRect& r) const
{
    if (!style().boxReflect())
        return LayoutRect();

    LayoutRect box = borderBoxRect();
    LayoutRect result = r;
    switch (style().boxReflect()->direction()) {
        case ReflectionBelow:
            result.setY(box.maxY() + reflectionOffset() + (box.maxY() - r.maxY()));
            break;
        case ReflectionAbove:
            result.setY(box.y() - reflectionOffset() - box.height() + (box.maxY() - r.maxY()));
            break;
        case ReflectionLeft:
            result.setX(box.x() - reflectionOffset() - box.width() + (box.maxX() - r.maxX()));
            break;
        case ReflectionRight:
            result.setX(box.maxX() + reflectionOffset() + (box.maxX() - r.maxX()));
            break;
    }
    return result;
}

bool RenderBox::fixedElementLaysOutRelativeToFrame(const FrameView& frameView) const
{
    return style().position() == FixedPosition && container()->isRenderView() && frameView.fixedElementsLayoutRelativeToFrame();
}

bool RenderBox::includeVerticalScrollbarSize() const
{
    return hasOverflowClip() && !layer()->hasOverlayScrollbars()
        && (style().overflowY() == OSCROLL || style().overflowY() == OAUTO);
}

bool RenderBox::includeHorizontalScrollbarSize() const
{
    return hasOverflowClip() && !layer()->hasOverlayScrollbars()
        && (style().overflowX() == OSCROLL || style().overflowX() == OAUTO);
}

int RenderBox::verticalScrollbarWidth() const
{
    return includeVerticalScrollbarSize() ? layer()->verticalScrollbarWidth() : 0;
}

int RenderBox::horizontalScrollbarHeight() const
{
    return includeHorizontalScrollbarSize() ? layer()->horizontalScrollbarHeight() : 0;
}

int RenderBox::instrinsicScrollbarLogicalWidth() const
{
    if (!hasOverflowClip())
        return 0;

    if (isHorizontalWritingMode() && style().overflowY() == OSCROLL) {
        ASSERT(layer()->hasVerticalScrollbar());
        return verticalScrollbarWidth();
    }

    if (!isHorizontalWritingMode() && style().overflowX() == OSCROLL) {
        ASSERT(layer()->hasHorizontalScrollbar());
        return horizontalScrollbarHeight();
    }

    return 0;
}

bool RenderBox::scrollLayer(ScrollDirection direction, ScrollGranularity granularity, float multiplier, Element** stopElement)
{
    RenderLayer* boxLayer = layer();
    if (boxLayer && boxLayer->scroll(direction, granularity, multiplier)) {
        if (stopElement)
            *stopElement = element();

        return true;
    }

    return false;
}

bool RenderBox::scroll(ScrollDirection direction, ScrollGranularity granularity, float multiplier, Element** stopElement, RenderBox* startBox, const IntPoint& wheelEventAbsolutePoint)
{
    if (scrollLayer(direction, granularity, multiplier, stopElement))
        return true;

    if (stopElement && *stopElement && *stopElement == element())
        return true;

    RenderBlock* nextScrollBlock = containingBlock();
    if (nextScrollBlock && nextScrollBlock->isRenderNamedFlowThread()) {
        ASSERT(startBox);
        nextScrollBlock = toRenderNamedFlowThread(nextScrollBlock)->fragmentFromAbsolutePointAndBox(wheelEventAbsolutePoint, *startBox);
    }

    if (nextScrollBlock && !nextScrollBlock->isRenderView())
        return nextScrollBlock->scroll(direction, granularity, multiplier, stopElement, startBox, wheelEventAbsolutePoint);

    return false;
}

bool RenderBox::logicalScroll(ScrollLogicalDirection direction, ScrollGranularity granularity, float multiplier, Element** stopElement)
{
    bool scrolled = false;
    
    RenderLayer* l = layer();
    if (l) {
#if PLATFORM(COCOA)
        // On Mac only we reset the inline direction position when doing a document scroll (e.g., hitting Home/End).
        if (granularity == ScrollByDocument)
            scrolled = l->scroll(logicalToPhysical(ScrollInlineDirectionBackward, isHorizontalWritingMode(), style().isFlippedBlocksWritingMode()), ScrollByDocument, multiplier);
#endif
        if (l->scroll(logicalToPhysical(direction, isHorizontalWritingMode(), style().isFlippedBlocksWritingMode()), granularity, multiplier))
            scrolled = true;
        
        if (scrolled) {
            if (stopElement)
                *stopElement = element();
            return true;
        }
    }

    if (stopElement && *stopElement && *stopElement == element())
        return true;

    RenderBlock* b = containingBlock();
    if (b && !b->isRenderView())
        return b->logicalScroll(direction, granularity, multiplier, stopElement);
    return false;
}

bool RenderBox::canBeScrolledAndHasScrollableArea() const
{
    return canBeProgramaticallyScrolled() && (scrollHeight() != clientHeight() || scrollWidth() != clientWidth());
}
    
bool RenderBox::canBeProgramaticallyScrolled() const
{
    if (isRenderView())
        return true;

    if (!hasOverflowClip())
        return false;

    bool hasScrollableOverflow = hasScrollableOverflowX() || hasScrollableOverflowY();
    if (scrollsOverflow() && hasScrollableOverflow)
        return true;

    return element() && element()->hasEditableStyle();
}

bool RenderBox::usesCompositedScrolling() const
{
    return hasOverflowClip() && hasLayer() && layer()->usesCompositedScrolling();
}

void RenderBox::autoscroll(const IntPoint& position)
{
    if (layer())
        layer()->autoscroll(position);
}

// There are two kinds of renderer that can autoscroll.
bool RenderBox::canAutoscroll() const
{
    if (isRenderView())
        return view().frameView().isScrollable();

    // Check for a box that can be scrolled in its own right.
    if (canBeScrolledAndHasScrollableArea())
        return true;

    return false;
}

// If specified point is in border belt, returned offset denotes direction of
// scrolling.
IntSize RenderBox::calculateAutoscrollDirection(const IntPoint& windowPoint) const
{
    IntRect box(absoluteBoundingBoxRect());
    box.move(view().frameView().scrollOffset());
    IntRect windowBox = view().frameView().contentsToWindow(box);

    IntPoint windowAutoscrollPoint = windowPoint;

    if (windowAutoscrollPoint.x() < windowBox.x() + autoscrollBeltSize)
        windowAutoscrollPoint.move(-autoscrollBeltSize, 0);
    else if (windowAutoscrollPoint.x() > windowBox.maxX() - autoscrollBeltSize)
        windowAutoscrollPoint.move(autoscrollBeltSize, 0);

    if (windowAutoscrollPoint.y() < windowBox.y() + autoscrollBeltSize)
        windowAutoscrollPoint.move(0, -autoscrollBeltSize);
    else if (windowAutoscrollPoint.y() > windowBox.maxY() - autoscrollBeltSize)
        windowAutoscrollPoint.move(0, autoscrollBeltSize);

    return windowAutoscrollPoint - windowPoint;
}

RenderBox* RenderBox::findAutoscrollable(RenderObject* renderer)
{
    while (renderer && !(renderer->isBox() && toRenderBox(renderer)->canAutoscroll())) {
        if (renderer->isRenderView() && renderer->document().ownerElement())
            renderer = renderer->document().ownerElement()->renderer();
        else
            renderer = renderer->parent();
    }

    return renderer && renderer->isBox() ? toRenderBox(renderer) : 0;
}

void RenderBox::panScroll(const IntPoint& source)
{
    if (layer())
        layer()->panScrollFromPoint(source);
}

bool RenderBox::needsPreferredWidthsRecalculation() const
{
    return style().paddingStart().isPercent() || style().paddingEnd().isPercent();
}

IntSize RenderBox::scrolledContentOffset() const
{
    if (!hasOverflowClip())
        return IntSize();

    ASSERT(hasLayer());
    return layer()->scrolledContentOffset();
}

LayoutSize RenderBox::cachedSizeForOverflowClip() const
{
    ASSERT(hasOverflowClip());
    ASSERT(hasLayer());
    return layer()->size();
}

void RenderBox::applyCachedClipAndScrollOffsetForRepaint(LayoutRect& paintRect) const
{
    flipForWritingMode(paintRect);
    paintRect.move(-scrolledContentOffset()); // For overflow:auto/scroll/hidden.

    // Do not clip scroll layer contents to reduce the number of repaints while scrolling.
    if (usesCompositedScrolling()) {
        flipForWritingMode(paintRect);
        return;
    }

    // height() is inaccurate if we're in the middle of a layout of this RenderBox, so use the
    // layer's size instead. Even if the layer's size is wrong, the layer itself will repaint
    // anyway if its size does change.
    LayoutRect clipRect(LayoutPoint(), cachedSizeForOverflowClip());
    paintRect = intersection(paintRect, clipRect);
    flipForWritingMode(paintRect);
}

void RenderBox::computeIntrinsicLogicalWidths(LayoutUnit& minLogicalWidth, LayoutUnit& maxLogicalWidth) const
{
    minLogicalWidth = minPreferredLogicalWidth() - borderAndPaddingLogicalWidth();
    maxLogicalWidth = maxPreferredLogicalWidth() - borderAndPaddingLogicalWidth();
}

LayoutUnit RenderBox::minPreferredLogicalWidth() const
{
    if (preferredLogicalWidthsDirty()) {
#ifndef NDEBUG
        SetLayoutNeededForbiddenScope layoutForbiddenScope(const_cast<RenderBox*>(this));
#endif
        const_cast<RenderBox*>(this)->computePreferredLogicalWidths();
    }
        
    return m_minPreferredLogicalWidth;
}

LayoutUnit RenderBox::maxPreferredLogicalWidth() const
{
    if (preferredLogicalWidthsDirty()) {
#ifndef NDEBUG
        SetLayoutNeededForbiddenScope layoutForbiddenScope(const_cast<RenderBox*>(this));
#endif
        const_cast<RenderBox*>(this)->computePreferredLogicalWidths();
    }
        
    return m_maxPreferredLogicalWidth;
}

bool RenderBox::hasOverrideHeight() const
{
    return gOverrideHeightMap && gOverrideHeightMap->contains(this);
}

bool RenderBox::hasOverrideWidth() const
{
    return gOverrideWidthMap && gOverrideWidthMap->contains(this);
}

void RenderBox::setOverrideLogicalContentHeight(LayoutUnit height)
{
    if (!gOverrideHeightMap)
        gOverrideHeightMap = new OverrideSizeMap();
    gOverrideHeightMap->set(this, height);
}

void RenderBox::setOverrideLogicalContentWidth(LayoutUnit width)
{
    if (!gOverrideWidthMap)
        gOverrideWidthMap = new OverrideSizeMap();
    gOverrideWidthMap->set(this, width);
}

void RenderBox::clearOverrideLogicalContentHeight()
{
    if (gOverrideHeightMap)
        gOverrideHeightMap->remove(this);
}

void RenderBox::clearOverrideLogicalContentWidth()
{
    if (gOverrideWidthMap)
        gOverrideWidthMap->remove(this);
}

void RenderBox::clearOverrideSize()
{
    clearOverrideLogicalContentHeight();
    clearOverrideLogicalContentWidth();
}

LayoutUnit RenderBox::overrideLogicalContentWidth() const
{
    ASSERT(hasOverrideWidth());
    return gOverrideWidthMap->get(this);
}

LayoutUnit RenderBox::overrideLogicalContentHeight() const
{
    ASSERT(hasOverrideHeight());
    return gOverrideHeightMap->get(this);
}

#if ENABLE(CSS_GRID_LAYOUT)
LayoutUnit RenderBox::overrideContainingBlockContentLogicalWidth() const
{
    ASSERT(hasOverrideContainingBlockLogicalWidth());
    return gOverrideContainingBlockLogicalWidthMap->get(this);
}

LayoutUnit RenderBox::overrideContainingBlockContentLogicalHeight() const
{
    ASSERT(hasOverrideContainingBlockLogicalHeight());
    return gOverrideContainingBlockLogicalHeightMap->get(this);
}

bool RenderBox::hasOverrideContainingBlockLogicalWidth() const
{
    return gOverrideContainingBlockLogicalWidthMap && gOverrideContainingBlockLogicalWidthMap->contains(this);
}

bool RenderBox::hasOverrideContainingBlockLogicalHeight() const
{
    return gOverrideContainingBlockLogicalHeightMap && gOverrideContainingBlockLogicalHeightMap->contains(this);
}

void RenderBox::setOverrideContainingBlockContentLogicalWidth(LayoutUnit logicalWidth)
{
    if (!gOverrideContainingBlockLogicalWidthMap)
        gOverrideContainingBlockLogicalWidthMap = new OverrideSizeMap;
    gOverrideContainingBlockLogicalWidthMap->set(this, logicalWidth);
}

void RenderBox::setOverrideContainingBlockContentLogicalHeight(LayoutUnit logicalHeight)
{
    if (!gOverrideContainingBlockLogicalHeightMap)
        gOverrideContainingBlockLogicalHeightMap = new OverrideSizeMap;
    gOverrideContainingBlockLogicalHeightMap->set(this, logicalHeight);
}

void RenderBox::clearContainingBlockOverrideSize()
{
    if (gOverrideContainingBlockLogicalWidthMap)
        gOverrideContainingBlockLogicalWidthMap->remove(this);
    clearOverrideContainingBlockContentLogicalHeight();
}

void RenderBox::clearOverrideContainingBlockContentLogicalHeight()
{
    if (gOverrideContainingBlockLogicalHeightMap)
        gOverrideContainingBlockLogicalHeightMap->remove(this);
}
#endif // ENABLE(CSS_GRID_LAYOUT)

LayoutUnit RenderBox::adjustBorderBoxLogicalWidthForBoxSizing(LayoutUnit width) const
{
    LayoutUnit bordersPlusPadding = borderAndPaddingLogicalWidth();
    if (style().boxSizing() == CONTENT_BOX)
        return width + bordersPlusPadding;
    return std::max(width, bordersPlusPadding);
}

LayoutUnit RenderBox::adjustBorderBoxLogicalHeightForBoxSizing(LayoutUnit height) const
{
    LayoutUnit bordersPlusPadding = borderAndPaddingLogicalHeight();
    if (style().boxSizing() == CONTENT_BOX)
        return height + bordersPlusPadding;
    return std::max(height, bordersPlusPadding);
}

LayoutUnit RenderBox::adjustContentBoxLogicalWidthForBoxSizing(LayoutUnit width) const
{
    if (style().boxSizing() == BORDER_BOX)
        width -= borderAndPaddingLogicalWidth();
    return std::max<LayoutUnit>(0, width);
}

LayoutUnit RenderBox::adjustContentBoxLogicalHeightForBoxSizing(LayoutUnit height) const
{
    if (style().boxSizing() == BORDER_BOX)
        height -= borderAndPaddingLogicalHeight();
    return std::max<LayoutUnit>(0, height);
}

// Hit Testing
bool RenderBox::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction action)
{
    LayoutPoint adjustedLocation = accumulatedOffset + location();

    // Check kids first.
    for (RenderObject* child = lastChild(); child; child = child->previousSibling()) {
        if (!child->hasLayer() && child->nodeAtPoint(request, result, locationInContainer, adjustedLocation, action)) {
            updateHitTestResult(result, locationInContainer.point() - toLayoutSize(adjustedLocation));
            return true;
        }
    }

    RenderFlowThread* flowThread = flowThreadContainingBlock();
    RenderRegion* regionToUse = flowThread ? toRenderNamedFlowFragment(flowThread->currentRegion()) : nullptr;

    // If the box is not contained by this region there's no point in going further.
    if (regionToUse && !flowThread->objectShouldFragmentInFlowRegion(this, regionToUse))
        return false;

    // Check our bounds next. For this purpose always assume that we can only be hit in the
    // foreground phase (which is true for replaced elements like images).
    LayoutRect boundsRect = borderBoxRectInRegion(regionToUse);
    boundsRect.moveBy(adjustedLocation);
    if (visibleToHitTesting() && action == HitTestForeground && locationInContainer.intersects(boundsRect)) {
        updateHitTestResult(result, locationInContainer.point() - toLayoutSize(adjustedLocation));
        if (!result.addNodeToRectBasedTestResult(element(), request, locationInContainer, boundsRect))
            return true;
    }

    return false;
}

// --------------------- painting stuff -------------------------------

void RenderBox::paintRootBoxFillLayers(const PaintInfo& paintInfo)
{
    if (paintInfo.skipRootBackground())
        return;

    auto& rootBackgroundRenderer = rendererForRootBackground();
    
    const FillLayer* bgLayer = rootBackgroundRenderer.style().backgroundLayers();
    Color bgColor = rootBackgroundRenderer.style().visitedDependentColor(CSSPropertyBackgroundColor);

    paintFillLayers(paintInfo, bgColor, bgLayer, view().backgroundRect(this), BackgroundBleedNone, CompositeSourceOver, &rootBackgroundRenderer);
}

BackgroundBleedAvoidance RenderBox::determineBackgroundBleedAvoidance(GraphicsContext* context) const
{
    if (context->paintingDisabled())
        return BackgroundBleedNone;

    const RenderStyle& style = this->style();

    if (!style.hasBackground() || !style.hasBorder() || !style.hasBorderRadius() || borderImageIsLoadedAndCanBeRendered())
        return BackgroundBleedNone;

    AffineTransform ctm = context->getCTM();
    FloatSize contextScaling(static_cast<float>(ctm.xScale()), static_cast<float>(ctm.yScale()));

    // Because RoundedRect uses IntRect internally the inset applied by the 
    // BackgroundBleedShrinkBackground strategy cannot be less than one integer
    // layout coordinate, even with subpixel layout enabled. To take that into
    // account, we clamp the contextScaling to 1.0 for the following test so
    // that borderObscuresBackgroundEdge can only return true if the border
    // widths are greater than 2 in both layout coordinates and screen
    // coordinates.
    // This precaution will become obsolete if RoundedRect is ever promoted to
    // a sub-pixel representation.
    if (contextScaling.width() > 1) 
        contextScaling.setWidth(1);
    if (contextScaling.height() > 1) 
        contextScaling.setHeight(1);

    if (borderObscuresBackgroundEdge(contextScaling))
        return BackgroundBleedShrinkBackground;
    if (!style.hasAppearance() && borderObscuresBackground() && backgroundHasOpaqueTopLayer())
        return BackgroundBleedBackgroundOverBorder;

    return BackgroundBleedUseTransparencyLayer;
}

void RenderBox::paintBoxDecorations(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    if (!paintInfo.shouldPaintWithinRoot(*this))
        return;

    LayoutRect paintRect = borderBoxRectInRegion(currentRenderNamedFlowFragment());
    paintRect.moveBy(paintOffset);

#if PLATFORM(IOS)
    // Workaround for <rdar://problem/6209763>. Force the painting bounds of checkboxes and radio controls to be square.
    if (style().appearance() == CheckboxPart || style().appearance() == RadioPart) {
        int width = std::min(paintRect.width(), paintRect.height());
        int height = width;
        paintRect = IntRect(paintRect.x(), paintRect.y() + (this->height() - height) / 2, width, height); // Vertically center the checkbox, like on desktop
    }
#endif
    BackgroundBleedAvoidance bleedAvoidance = determineBackgroundBleedAvoidance(paintInfo.context);

    // FIXME: Should eventually give the theme control over whether the box shadow should paint, since controls could have
    // custom shadows of their own.
    if (!boxShadowShouldBeAppliedToBackground(bleedAvoidance))
        paintBoxShadow(paintInfo, paintRect, style(), Normal);

    GraphicsContextStateSaver stateSaver(*paintInfo.context, false);
    if (bleedAvoidance == BackgroundBleedUseTransparencyLayer) {
        // To avoid the background color bleeding out behind the border, we'll render background and border
        // into a transparency layer, and then clip that in one go (which requires setting up the clip before
        // beginning the layer).
        stateSaver.save();
        paintInfo.context->clipRoundedRect(style().getRoundedBorderFor(paintRect).pixelSnappedRoundedRectForPainting(document().deviceScaleFactor()));
        paintInfo.context->beginTransparencyLayer(1);
    }

    // If we have a native theme appearance, paint that before painting our background.
    // The theme will tell us whether or not we should also paint the CSS background.
    ControlStates* controlStates = nullptr;
    if (style().hasAppearance()) {
        if (hasControlStatesForRenderer(this))
            controlStates = controlStatesForRenderer(this);
        else {
            controlStates = new ControlStates();
            addControlStatesForRenderer(this, controlStates);
        }
    }

    bool themePainted = style().hasAppearance() && !theme().paint(*this, controlStates, paintInfo, paintRect);

    if (controlStates && controlStates->needsRepaint())
        view().scheduleLazyRepaint(*this);

    if (!themePainted) {
        if (bleedAvoidance == BackgroundBleedBackgroundOverBorder)
            paintBorder(paintInfo, paintRect, style(), bleedAvoidance);

        paintBackground(paintInfo, paintRect, bleedAvoidance);

        if (style().hasAppearance())
            theme().paintDecorations(*this, paintInfo, paintRect);
    }
    paintBoxShadow(paintInfo, paintRect, style(), Inset);

    // The theme will tell us whether or not we should also paint the CSS border.
    if (bleedAvoidance != BackgroundBleedBackgroundOverBorder && (!style().hasAppearance() || (!themePainted && theme().paintBorderOnly(*this, paintInfo, paintRect))) && style().hasBorder())
        paintBorder(paintInfo, paintRect, style(), bleedAvoidance);

    if (bleedAvoidance == BackgroundBleedUseTransparencyLayer)
        paintInfo.context->endTransparencyLayer();
}

void RenderBox::paintBackground(const PaintInfo& paintInfo, const LayoutRect& paintRect, BackgroundBleedAvoidance bleedAvoidance)
{
    if (isRoot()) {
        paintRootBoxFillLayers(paintInfo);
        return;
    }
    if (isBody() && skipBodyBackground(this))
        return;
    if (backgroundIsKnownToBeObscured() && !boxShadowShouldBeAppliedToBackground(bleedAvoidance))
        return;
    paintFillLayers(paintInfo, style().visitedDependentColor(CSSPropertyBackgroundColor), style().backgroundLayers(), paintRect, bleedAvoidance);
}

bool RenderBox::getBackgroundPaintedExtent(LayoutRect& paintedExtent) const
{
    ASSERT(hasBackground());
    LayoutRect backgroundRect = pixelSnappedIntRect(borderBoxRect());

    Color backgroundColor = style().visitedDependentColor(CSSPropertyBackgroundColor);
    if (backgroundColor.isValid() && backgroundColor.alpha()) {
        paintedExtent = backgroundRect;
        return true;
    }

    if (!style().backgroundLayers()->image() || style().backgroundLayers()->next()) {
        paintedExtent =  backgroundRect;
        return true;
    }

    BackgroundImageGeometry geometry;
    calculateBackgroundImageGeometry(0, style().backgroundLayers(), backgroundRect, geometry);
    paintedExtent = geometry.destRect();
    return !geometry.hasNonLocalGeometry();
}

bool RenderBox::backgroundIsKnownToBeOpaqueInRect(const LayoutRect& localRect) const
{
    if (isBody() && skipBodyBackground(this))
        return false;

    Color backgroundColor = style().visitedDependentColor(CSSPropertyBackgroundColor);
    if (!backgroundColor.isValid() || backgroundColor.hasAlpha())
        return false;

    // If the element has appearance, it might be painted by theme.
    // We cannot be sure if theme paints the background opaque.
    // In this case it is safe to not assume opaqueness.
    // FIXME: May be ask theme if it paints opaque.
    if (style().hasAppearance())
        return false;
    // FIXME: Check the opaqueness of background images.

    if (hasClip() || hasClipPath())
        return false;

    // FIXME: Use rounded rect if border radius is present.
    if (style().hasBorderRadius())
        return false;
    
    // FIXME: The background color clip is defined by the last layer.
    if (style().backgroundLayers()->next())
        return false;
    LayoutRect backgroundRect;
    switch (style().backgroundClip()) {
    case BorderFillBox:
        backgroundRect = borderBoxRect();
        break;
    case PaddingFillBox:
        backgroundRect = paddingBoxRect();
        break;
    case ContentFillBox:
        backgroundRect = contentBoxRect();
        break;
    default:
        break;
    }
    return backgroundRect.contains(localRect);
}

static bool isCandidateForOpaquenessTest(const RenderBox& childBox)
{
    const RenderStyle& childStyle = childBox.style();
    if (childStyle.position() != StaticPosition && childBox.containingBlock() != childBox.parent())
        return false;
    if (childStyle.visibility() != VISIBLE)
        return false;
#if ENABLE(CSS_SHAPES)
    if (childStyle.shapeOutside())
        return false;
#endif
    if (!childBox.width() || !childBox.height())
        return false;
    if (RenderLayer* childLayer = childBox.layer()) {
        if (childLayer->isComposited())
            return false;
        // FIXME: Deal with z-index.
        if (!childStyle.hasAutoZIndex())
            return false;
        if (childLayer->hasTransform() || childLayer->isTransparent() || childLayer->hasFilter())
            return false;
    }
    return true;
}

bool RenderBox::foregroundIsKnownToBeOpaqueInRect(const LayoutRect& localRect, unsigned maxDepthToTest) const
{
    if (!maxDepthToTest)
        return false;
    for (auto& childBox : childrenOfType<RenderBox>(*this)) {
        if (!isCandidateForOpaquenessTest(childBox))
            continue;
        LayoutPoint childLocation = childBox.location();
        if (childBox.isRelPositioned())
            childLocation.move(childBox.relativePositionOffset());
        LayoutRect childLocalRect = localRect;
        childLocalRect.moveBy(-childLocation);
        if (childLocalRect.y() < 0 || childLocalRect.x() < 0) {
            // If there is unobscured area above/left of a static positioned box then the rect is probably not covered.
            if (childBox.style().position() == StaticPosition)
                return false;
            continue;
        }
        if (childLocalRect.maxY() > childBox.height() || childLocalRect.maxX() > childBox.width())
            continue;
        if (childBox.backgroundIsKnownToBeOpaqueInRect(childLocalRect))
            return true;
        if (childBox.foregroundIsKnownToBeOpaqueInRect(childLocalRect, maxDepthToTest - 1))
            return true;
    }
    return false;
}

bool RenderBox::computeBackgroundIsKnownToBeObscured()
{
    // Test to see if the children trivially obscure the background.
    // FIXME: This test can be much more comprehensive.
    if (!hasBackground())
        return false;
    // Table and root background painting is special.
    if (isTable() || isRoot())
        return false;

    LayoutRect backgroundRect;
    if (!getBackgroundPaintedExtent(backgroundRect))
        return false;
    return foregroundIsKnownToBeOpaqueInRect(backgroundRect, backgroundObscurationTestMaxDepth);
}

bool RenderBox::backgroundHasOpaqueTopLayer() const
{
    const FillLayer* fillLayer = style().backgroundLayers();
    if (!fillLayer || fillLayer->clip() != BorderFillBox)
        return false;

    // Clipped with local scrolling
    if (hasOverflowClip() && fillLayer->attachment() == LocalBackgroundAttachment)
        return false;

    if (fillLayer->hasOpaqueImage(*this) && fillLayer->hasRepeatXY() && fillLayer->image()->canRender(this, style().effectiveZoom()))
        return true;

    // If there is only one layer and no image, check whether the background color is opaque
    if (!fillLayer->next() && !fillLayer->hasImage()) {
        Color bgColor = style().visitedDependentColor(CSSPropertyBackgroundColor);
        if (bgColor.isValid() && bgColor.alpha() == 255)
            return true;
    }

    return false;
}

void RenderBox::paintMask(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    if (!paintInfo.shouldPaintWithinRoot(*this) || style().visibility() != VISIBLE || paintInfo.phase != PaintPhaseMask || paintInfo.context->paintingDisabled())
        return;

    LayoutRect paintRect = LayoutRect(paintOffset, size());
    paintMaskImages(paintInfo, paintRect);
}

void RenderBox::paintMaskImages(const PaintInfo& paintInfo, const LayoutRect& paintRect)
{
    // Figure out if we need to push a transparency layer to render our mask.
    bool pushTransparencyLayer = false;
    bool compositedMask = hasLayer() && layer()->hasCompositedMask();
    bool flattenCompositingLayers = view().frameView().paintBehavior() & PaintBehaviorFlattenCompositingLayers;
    CompositeOperator compositeOp = CompositeSourceOver;

    bool allMaskImagesLoaded = true;
    
    if (!compositedMask || flattenCompositingLayers) {
        pushTransparencyLayer = true;
        StyleImage* maskBoxImage = style().maskBoxImage().image();
        const FillLayer* maskLayers = style().maskLayers();

        // Don't render a masked element until all the mask images have loaded, to prevent a flash of unmasked content.
        if (maskBoxImage)
            allMaskImagesLoaded &= maskBoxImage->isLoaded();

        if (maskLayers)
            allMaskImagesLoaded &= maskLayers->imagesAreLoaded();

        paintInfo.context->setCompositeOperation(CompositeDestinationIn);
        paintInfo.context->beginTransparencyLayer(1);
        compositeOp = CompositeSourceOver;
    }

    if (allMaskImagesLoaded) {
        paintFillLayers(paintInfo, Color(), style().maskLayers(), paintRect, BackgroundBleedNone, compositeOp);
        paintNinePieceImage(paintInfo.context, paintRect, style(), style().maskBoxImage(), compositeOp);
    }
    
    if (pushTransparencyLayer)
        paintInfo.context->endTransparencyLayer();
}

LayoutRect RenderBox::maskClipRect()
{
    const NinePieceImage& maskBoxImage = style().maskBoxImage();
    if (maskBoxImage.image()) {
        LayoutRect borderImageRect = borderBoxRect();
        
        // Apply outsets to the border box.
        borderImageRect.expand(style().maskBoxImageOutsets());
        return borderImageRect;
    }
    
    LayoutRect result;
    LayoutRect borderBox = borderBoxRect();
    for (const FillLayer* maskLayer = style().maskLayers(); maskLayer; maskLayer = maskLayer->next()) {
        if (maskLayer->image()) {
            BackgroundImageGeometry geometry;
            // Masks should never have fixed attachment, so it's OK for paintContainer to be null.
            calculateBackgroundImageGeometry(0, maskLayer, borderBox, geometry);
            result.unite(geometry.destRect());
        }
    }
    return result;
}

void RenderBox::paintFillLayers(const PaintInfo& paintInfo, const Color& c, const FillLayer* fillLayer, const LayoutRect& rect,
    BackgroundBleedAvoidance bleedAvoidance, CompositeOperator op, RenderElement* backgroundObject)
{
    Vector<const FillLayer*, 8> layers;
    const FillLayer* curLayer = fillLayer;
    bool shouldDrawBackgroundInSeparateBuffer = false;
    while (curLayer) {
        layers.append(curLayer);
        // Stop traversal when an opaque layer is encountered.
        // FIXME : It would be possible for the following occlusion culling test to be more aggressive 
        // on layers with no repeat by testing whether the image covers the layout rect.
        // Testing that here would imply duplicating a lot of calculations that are currently done in
        // RenderBoxModelObject::paintFillLayerExtended. A more efficient solution might be to move
        // the layer recursion into paintFillLayerExtended, or to compute the layer geometry here
        // and pass it down.

        if (!shouldDrawBackgroundInSeparateBuffer && curLayer->blendMode() != BlendModeNormal)
            shouldDrawBackgroundInSeparateBuffer = true;

        // The clipOccludesNextLayers condition must be evaluated first to avoid short-circuiting.
        if (curLayer->clipOccludesNextLayers(curLayer == fillLayer) && curLayer->hasOpaqueImage(*this) && curLayer->image()->canRender(this, style().effectiveZoom()) && curLayer->hasRepeatXY() && curLayer->blendMode() == BlendModeNormal)
            break;
        curLayer = curLayer->next();
    }

    GraphicsContext* context = paintInfo.context;
    if (!context)
        shouldDrawBackgroundInSeparateBuffer = false;

    BaseBackgroundColorUsage baseBgColorUsage = BaseBackgroundColorUse;

    if (shouldDrawBackgroundInSeparateBuffer) {
        paintFillLayer(paintInfo, c, *layers.rbegin(), rect, bleedAvoidance, op, backgroundObject, BaseBackgroundColorOnly);
        baseBgColorUsage = BaseBackgroundColorSkip;
        context->beginTransparencyLayer(1);
    }

    Vector<const FillLayer*>::const_reverse_iterator topLayer = layers.rend();
    for (Vector<const FillLayer*>::const_reverse_iterator it = layers.rbegin(); it != topLayer; ++it)
        paintFillLayer(paintInfo, c, *it, rect, bleedAvoidance, op, backgroundObject, baseBgColorUsage);

    if (shouldDrawBackgroundInSeparateBuffer)
        context->endTransparencyLayer();
}

void RenderBox::paintFillLayer(const PaintInfo& paintInfo, const Color& c, const FillLayer* fillLayer, const LayoutRect& rect,
    BackgroundBleedAvoidance bleedAvoidance, CompositeOperator op, RenderElement* backgroundObject, BaseBackgroundColorUsage baseBgColorUsage)
{
    paintFillLayerExtended(paintInfo, c, fillLayer, rect, bleedAvoidance, 0, LayoutSize(), op, backgroundObject, baseBgColorUsage);
}

static bool layersUseImage(WrappedImagePtr image, const FillLayer* layers)
{
    for (const FillLayer* curLayer = layers; curLayer; curLayer = curLayer->next()) {
        if (curLayer->image() && image == curLayer->image()->data())
            return true;
    }

    return false;
}

void RenderBox::imageChanged(WrappedImagePtr image, const IntRect*)
{
    if (!parent())
        return;

    if ((style().borderImage().image() && style().borderImage().image()->data() == image) ||
        (style().maskBoxImage().image() && style().maskBoxImage().image()->data() == image)) {
        repaint();
        return;
    }

#if ENABLE(CSS_SHAPES)
    ShapeValue* shapeOutsideValue = style().shapeOutside();
    if (!view().frameView().isInLayout() && isFloating() && shapeOutsideValue && shapeOutsideValue->image() && shapeOutsideValue->image()->data() == image) {
        ShapeOutsideInfo::ensureInfo(*this).markShapeAsDirty();
        markShapeOutsideDependentsForLayout();
    }
#endif

    bool didFullRepaint = repaintLayerRectsForImage(image, style().backgroundLayers(), true);
    if (!didFullRepaint)
        repaintLayerRectsForImage(image, style().maskLayers(), false);

    if (!isComposited())
        return;

    if (layer()->hasCompositedMask() && layersUseImage(image, style().maskLayers()))
        layer()->contentChanged(MaskImageChanged);
    if (layersUseImage(image, style().backgroundLayers()))
        layer()->contentChanged(BackgroundImageChanged);
}

bool RenderBox::repaintLayerRectsForImage(WrappedImagePtr image, const FillLayer* layers, bool drawingBackground)
{
    LayoutRect rendererRect;
    RenderBox* layerRenderer = 0;

    for (const FillLayer* curLayer = layers; curLayer; curLayer = curLayer->next()) {
        if (curLayer->image() && image == curLayer->image()->data() && curLayer->image()->canRender(this, style().effectiveZoom())) {
            // Now that we know this image is being used, compute the renderer and the rect if we haven't already.
            bool drawingRootBackground = drawingBackground && (isRoot() || (isBody() && !document().documentElement()->renderer()->hasBackground()));
            if (!layerRenderer) {
                if (drawingRootBackground) {
                    layerRenderer = &view();

                    LayoutUnit rw = toRenderView(*layerRenderer).frameView().contentsWidth();
                    LayoutUnit rh = toRenderView(*layerRenderer).frameView().contentsHeight();

                    rendererRect = LayoutRect(-layerRenderer->marginLeft(),
                        -layerRenderer->marginTop(),
                        std::max(layerRenderer->width() + layerRenderer->horizontalMarginExtent() + layerRenderer->borderLeft() + layerRenderer->borderRight(), rw),
                        std::max(layerRenderer->height() + layerRenderer->verticalMarginExtent() + layerRenderer->borderTop() + layerRenderer->borderBottom(), rh));
                } else {
                    layerRenderer = this;
                    rendererRect = borderBoxRect();
                }
            }

            BackgroundImageGeometry geometry;
            layerRenderer->calculateBackgroundImageGeometry(0, curLayer, rendererRect, geometry);
            if (geometry.hasNonLocalGeometry()) {
                // Rather than incur the costs of computing the paintContainer for renderers with fixed backgrounds
                // in order to get the right destRect, just repaint the entire renderer.
                layerRenderer->repaint();
                return true;
            }
            
            LayoutRect rectToRepaint = geometry.destRect();
            bool shouldClipToLayer = true;

            // If this is the root background layer, we may need to extend the repaintRect if the FrameView has an
            // extendedBackground. We should only extend the rect if it is already extending the full width or height
            // of the rendererRect.
            if (drawingRootBackground && view().frameView().hasExtendedBackgroundRectForPainting()) {
                shouldClipToLayer = false;
                IntRect extendedBackgroundRect = view().frameView().extendedBackgroundRectForPainting();
                if (rectToRepaint.width() == rendererRect.width()) {
                    rectToRepaint.move(extendedBackgroundRect.x(), 0);
                    rectToRepaint.setWidth(extendedBackgroundRect.width());
                }
                if (rectToRepaint.height() == rendererRect.height()) {
                    rectToRepaint.move(0, extendedBackgroundRect.y());
                    rectToRepaint.setHeight(extendedBackgroundRect.height());
                }
            }

            layerRenderer->repaintRectangle(rectToRepaint, shouldClipToLayer);
            if (geometry.destRect() == rendererRect)
                return true;
        }
    }
    return false;
}

bool RenderBox::pushContentsClip(PaintInfo& paintInfo, const LayoutPoint& accumulatedOffset)
{
    if (paintInfo.phase == PaintPhaseBlockBackground || paintInfo.phase == PaintPhaseSelfOutline || paintInfo.phase == PaintPhaseMask)
        return false;
        
    bool isControlClip = hasControlClip();
    bool isOverflowClip = hasOverflowClip() && !layer()->isSelfPaintingLayer();
    
    if (!isControlClip && !isOverflowClip)
        return false;
    
    if (paintInfo.phase == PaintPhaseOutline)
        paintInfo.phase = PaintPhaseChildOutlines;
    else if (paintInfo.phase == PaintPhaseChildBlockBackground) {
        paintInfo.phase = PaintPhaseBlockBackground;
        paintObject(paintInfo, accumulatedOffset);
        paintInfo.phase = PaintPhaseChildBlockBackgrounds;
    }
    float deviceScaleFactor = document().deviceScaleFactor();
    FloatRect clipRect = pixelSnappedForPainting((isControlClip ? controlClipRect(accumulatedOffset) : overflowClipRect(accumulatedOffset, currentRenderNamedFlowFragment(), IgnoreOverlayScrollbarSize, paintInfo.phase)), deviceScaleFactor);
    paintInfo.context->save();
    if (style().hasBorderRadius())
        paintInfo.context->clipRoundedRect(style().getRoundedInnerBorderFor(LayoutRect(accumulatedOffset, size())).pixelSnappedRoundedRectForPainting(deviceScaleFactor));
    paintInfo.context->clip(clipRect);
    return true;
}

void RenderBox::popContentsClip(PaintInfo& paintInfo, PaintPhase originalPhase, const LayoutPoint& accumulatedOffset)
{
    ASSERT(hasControlClip() || (hasOverflowClip() && !layer()->isSelfPaintingLayer()));

    paintInfo.context->restore();
    if (originalPhase == PaintPhaseOutline) {
        paintInfo.phase = PaintPhaseSelfOutline;
        paintObject(paintInfo, accumulatedOffset);
        paintInfo.phase = originalPhase;
    } else if (originalPhase == PaintPhaseChildBlockBackground)
        paintInfo.phase = originalPhase;
}

LayoutRect RenderBox::overflowClipRect(const LayoutPoint& location, RenderRegion* region, OverlayScrollbarSizeRelevancy relevancy, PaintPhase)
{
    // FIXME: When overflow-clip (CSS3) is implemented, we'll obtain the property
    // here.
    LayoutRect clipRect = borderBoxRectInRegion(region);
    clipRect.setLocation(location + clipRect.location() + LayoutSize(borderLeft(), borderTop()));
    clipRect.setSize(clipRect.size() - LayoutSize(borderLeft() + borderRight(), borderTop() + borderBottom()));

    // Subtract out scrollbars if we have them.
     if (layer()) {
        if (style().shouldPlaceBlockDirectionScrollbarOnLogicalLeft())
            clipRect.move(layer()->verticalScrollbarWidth(relevancy), 0);
        clipRect.contract(layer()->verticalScrollbarWidth(relevancy), layer()->horizontalScrollbarHeight(relevancy));
     }

    return clipRect;
}

LayoutRect RenderBox::clipRect(const LayoutPoint& location, RenderRegion* region)
{
    LayoutRect borderBoxRect = borderBoxRectInRegion(region);
    LayoutRect clipRect = LayoutRect(borderBoxRect.location() + location, borderBoxRect.size());

    if (!style().clipLeft().isAuto()) {
        LayoutUnit c = valueForLength(style().clipLeft(), borderBoxRect.width());
        clipRect.move(c, 0);
        clipRect.contract(c, 0);
    }

    // We don't use the region-specific border box's width and height since clip offsets are (stupidly) specified
    // from the left and top edges. Therefore it's better to avoid constraining to smaller widths and heights.

    if (!style().clipRight().isAuto())
        clipRect.contract(width() - valueForLength(style().clipRight(), width()), 0);

    if (!style().clipTop().isAuto()) {
        LayoutUnit c = valueForLength(style().clipTop(), borderBoxRect.height());
        clipRect.move(0, c);
        clipRect.contract(0, c);
    }

    if (!style().clipBottom().isAuto())
        clipRect.contract(0, height() - valueForLength(style().clipBottom(), height()));

    return clipRect;
}

LayoutUnit RenderBox::shrinkLogicalWidthToAvoidFloats(LayoutUnit childMarginStart, LayoutUnit childMarginEnd, const RenderBlock* cb, RenderRegion* region) const
{    
    RenderRegion* containingBlockRegion = 0;
    LayoutUnit logicalTopPosition = logicalTop();
    if (region) {
        LayoutUnit offsetFromLogicalTopOfRegion = region ? region->logicalTopForFlowThreadContent() - offsetFromLogicalTopOfFirstPage() : LayoutUnit();
        logicalTopPosition = std::max(logicalTopPosition, logicalTopPosition + offsetFromLogicalTopOfRegion);
        containingBlockRegion = cb->clampToStartAndEndRegions(region);
    }

    LayoutUnit result = cb->availableLogicalWidthForLineInRegion(logicalTopPosition, false, containingBlockRegion) - childMarginStart - childMarginEnd;

    // We need to see if margins on either the start side or the end side can contain the floats in question. If they can,
    // then just using the line width is inaccurate. In the case where a float completely fits, we don't need to use the line
    // offset at all, but can instead push all the way to the content edge of the containing block. In the case where the float
    // doesn't fit, we can use the line offset, but we need to grow it by the margin to reflect the fact that the margin was
    // "consumed" by the float. Negative margins aren't consumed by the float, and so we ignore them.
    if (childMarginStart > 0) {
        LayoutUnit startContentSide = cb->startOffsetForContent(containingBlockRegion);
        LayoutUnit startContentSideWithMargin = startContentSide + childMarginStart;
        LayoutUnit startOffset = cb->startOffsetForLineInRegion(logicalTopPosition, false, containingBlockRegion);
        if (startOffset > startContentSideWithMargin)
            result += childMarginStart;
        else
            result += startOffset - startContentSide;
    }
    
    if (childMarginEnd > 0) {
        LayoutUnit endContentSide = cb->endOffsetForContent(containingBlockRegion);
        LayoutUnit endContentSideWithMargin = endContentSide + childMarginEnd;
        LayoutUnit endOffset = cb->endOffsetForLineInRegion(logicalTopPosition, false, containingBlockRegion);
        if (endOffset > endContentSideWithMargin)
            result += childMarginEnd;
        else
            result += endOffset - endContentSide;
    }

    return result;
}

LayoutUnit RenderBox::containingBlockLogicalWidthForContent() const
{
#if ENABLE(CSS_GRID_LAYOUT)
    if (hasOverrideContainingBlockLogicalWidth())
        return overrideContainingBlockContentLogicalWidth();
#endif

    RenderBlock* cb = containingBlock();
    return cb->availableLogicalWidth();
}

LayoutUnit RenderBox::containingBlockLogicalHeightForContent(AvailableLogicalHeightType heightType) const
{
#if ENABLE(CSS_GRID_LAYOUT)
    if (hasOverrideContainingBlockLogicalHeight())
        return overrideContainingBlockContentLogicalHeight();
#endif

    RenderBlock* cb = containingBlock();
    return cb->availableLogicalHeight(heightType);
}

LayoutUnit RenderBox::containingBlockLogicalWidthForContentInRegion(RenderRegion* region) const
{
    if (!region)
        return containingBlockLogicalWidthForContent();

    RenderBlock* cb = containingBlock();
    RenderRegion* containingBlockRegion = cb->clampToStartAndEndRegions(region);
    // FIXME: It's unclear if a region's content should use the containing block's override logical width.
    // If it should, the following line should call containingBlockLogicalWidthForContent.
    LayoutUnit result = cb->availableLogicalWidth();
    RenderBoxRegionInfo* boxInfo = cb->renderBoxRegionInfo(containingBlockRegion);
    if (!boxInfo)
        return result;
    return std::max<LayoutUnit>(0, result - (cb->logicalWidth() - boxInfo->logicalWidth()));
}

LayoutUnit RenderBox::containingBlockAvailableLineWidthInRegion(RenderRegion* region) const
{
    RenderBlock* cb = containingBlock();
    RenderRegion* containingBlockRegion = 0;
    LayoutUnit logicalTopPosition = logicalTop();
    if (region) {
        LayoutUnit offsetFromLogicalTopOfRegion = region ? region->logicalTopForFlowThreadContent() - offsetFromLogicalTopOfFirstPage() : LayoutUnit();
        logicalTopPosition = std::max(logicalTopPosition, logicalTopPosition + offsetFromLogicalTopOfRegion);
        containingBlockRegion = cb->clampToStartAndEndRegions(region);
    }
    return cb->availableLogicalWidthForLineInRegion(logicalTopPosition, false, containingBlockRegion, availableLogicalHeight(IncludeMarginBorderPadding));
}

LayoutUnit RenderBox::perpendicularContainingBlockLogicalHeight() const
{
#if ENABLE(CSS_GRID_LAYOUT)
    if (hasOverrideContainingBlockLogicalHeight())
        return overrideContainingBlockContentLogicalHeight();
#endif

    RenderBlock* cb = containingBlock();
    if (cb->hasOverrideHeight())
        return cb->overrideLogicalContentHeight();

    const RenderStyle& containingBlockStyle = cb->style();
    Length logicalHeightLength = containingBlockStyle.logicalHeight();

    // FIXME: For now just support fixed heights.  Eventually should support percentage heights as well.
    if (!logicalHeightLength.isFixed()) {
        LayoutUnit fillFallbackExtent = containingBlockStyle.isHorizontalWritingMode() ? view().frameView().visibleHeight() : view().frameView().visibleWidth();
        LayoutUnit fillAvailableExtent = containingBlock()->availableLogicalHeight(ExcludeMarginBorderPadding);
        return std::min(fillAvailableExtent, fillFallbackExtent);
    }

    // Use the content box logical height as specified by the style.
    return cb->adjustContentBoxLogicalHeightForBoxSizing(logicalHeightLength.value());
}

void RenderBox::mapLocalToContainer(const RenderLayerModelObject* repaintContainer, TransformState& transformState, MapCoordinatesFlags mode, bool* wasFixed) const
{
    if (repaintContainer == this)
        return;

    if (view().layoutStateEnabled() && !repaintContainer) {
        LayoutState* layoutState = view().layoutState();
        LayoutSize offset = layoutState->m_paintOffset + locationOffset();
        if (style().hasInFlowPosition() && layer())
            offset += layer()->offsetForInFlowPosition();
        transformState.move(offset);
        return;
    }

    bool containerSkipped;
    auto o = container(repaintContainer, &containerSkipped);
    if (!o)
        return;

    bool isFixedPos = style().position() == FixedPosition;
    bool hasTransform = hasLayer() && layer()->transform();
    // If this box has a transform, it acts as a fixed position container for fixed descendants,
    // and may itself also be fixed position. So propagate 'fixed' up only if this box is fixed position.
    if (hasTransform && !isFixedPos)
        mode &= ~IsFixed;
    else if (isFixedPos)
        mode |= IsFixed;

    if (wasFixed)
        *wasFixed = mode & IsFixed;
    
    LayoutSize containerOffset = offsetFromContainer(o, roundedLayoutPoint(transformState.mappedPoint()));
    
    bool preserve3D = mode & UseTransforms && (o->style().preserves3D() || style().preserves3D());
    if (mode & UseTransforms && shouldUseTransformFromContainer(o)) {
        TransformationMatrix t;
        getTransformFromContainer(o, containerOffset, t);
        transformState.applyTransform(t, preserve3D ? TransformState::AccumulateTransform : TransformState::FlattenTransform);
    } else
        transformState.move(containerOffset.width(), containerOffset.height(), preserve3D ? TransformState::AccumulateTransform : TransformState::FlattenTransform);

    if (containerSkipped) {
        // There can't be a transform between repaintContainer and o, because transforms create containers, so it should be safe
        // to just subtract the delta between the repaintContainer and o.
        LayoutSize containerOffset = repaintContainer->offsetFromAncestorContainer(o);
        transformState.move(-containerOffset.width(), -containerOffset.height(), preserve3D ? TransformState::AccumulateTransform : TransformState::FlattenTransform);
        return;
    }

    mode &= ~ApplyContainerFlip;

    // For fixed positioned elements inside out-of-flow named flows, we do not want to
    // map their position further to regions based on their coordinates inside the named flows.
    if (!o->isOutOfFlowRenderFlowThread() || !fixedPositionedWithNamedFlowContainingBlock())
        o->mapLocalToContainer(repaintContainer, transformState, mode, wasFixed);
    else
        o->mapLocalToContainer(toRenderLayerModelObject(o), transformState, mode, wasFixed);
}

const RenderObject* RenderBox::pushMappingToContainer(const RenderLayerModelObject* ancestorToStopAt, RenderGeometryMap& geometryMap) const
{
    ASSERT(ancestorToStopAt != this);

    bool ancestorSkipped;
    auto container = this->container(ancestorToStopAt, &ancestorSkipped);
    if (!container)
        return 0;

    bool isFixedPos = style().position() == FixedPosition;
    bool hasTransform = hasLayer() && layer()->transform();

    LayoutSize adjustmentForSkippedAncestor;
    if (ancestorSkipped) {
        // There can't be a transform between repaintContainer and o, because transforms create containers, so it should be safe
        // to just subtract the delta between the ancestor and o.
        adjustmentForSkippedAncestor = -ancestorToStopAt->offsetFromAncestorContainer(container);
    }

    bool offsetDependsOnPoint = false;
    LayoutSize containerOffset = offsetFromContainer(container, LayoutPoint(), &offsetDependsOnPoint);

    bool preserve3D = container->style().preserves3D() || style().preserves3D();
    if (shouldUseTransformFromContainer(container)) {
        TransformationMatrix t;
        getTransformFromContainer(container, containerOffset, t);
        t.translateRight(adjustmentForSkippedAncestor.width(), adjustmentForSkippedAncestor.height());
        
        geometryMap.push(this, t, preserve3D, offsetDependsOnPoint, isFixedPos, hasTransform);
    } else {
        containerOffset += adjustmentForSkippedAncestor;
        geometryMap.push(this, containerOffset, preserve3D, offsetDependsOnPoint, isFixedPos, hasTransform);
    }
    
    return ancestorSkipped ? ancestorToStopAt : container;
}

void RenderBox::mapAbsoluteToLocalPoint(MapCoordinatesFlags mode, TransformState& transformState) const
{
    bool isFixedPos = style().position() == FixedPosition;
    bool hasTransform = hasLayer() && layer()->transform();
    if (hasTransform && !isFixedPos) {
        // If this box has a transform, it acts as a fixed position container for fixed descendants,
        // and may itself also be fixed position. So propagate 'fixed' up only if this box is fixed position.
        mode &= ~IsFixed;
    } else if (isFixedPos)
        mode |= IsFixed;

    RenderBoxModelObject::mapAbsoluteToLocalPoint(mode, transformState);
}

LayoutSize RenderBox::offsetFromContainer(RenderObject* o, const LayoutPoint&, bool* offsetDependsOnPoint) const
{
    // A region "has" boxes inside it without being their container. 
    ASSERT(o == container() || o->isRenderRegion());

    LayoutSize offset;    
    if (isInFlowPositioned())
        offset += offsetForInFlowPosition();

    if (!isInline() || isReplaced())
        offset += topLeftLocationOffset();

    if (o->isBox())
        offset -= toRenderBox(o)->scrolledContentOffset();

    if (style().position() == AbsolutePosition && o->isInFlowPositioned() && o->isRenderInline())
        offset += toRenderInline(o)->offsetForInFlowPositionedInline(this);

    if (offsetDependsOnPoint)
        *offsetDependsOnPoint |= o->isRenderFlowThread();

    return offset;
}

std::unique_ptr<InlineElementBox> RenderBox::createInlineBox()
{
    return std::make_unique<InlineElementBox>(*this);
}

void RenderBox::dirtyLineBoxes(bool fullLayout)
{
    if (m_inlineBoxWrapper) {
        if (fullLayout) {
            delete m_inlineBoxWrapper;
            m_inlineBoxWrapper = nullptr;
        } else
            m_inlineBoxWrapper->dirtyLineBoxes();
    }
}

void RenderBox::positionLineBox(InlineElementBox& box)
{
    if (isOutOfFlowPositioned()) {
        // Cache the x position only if we were an INLINE type originally.
        bool wasInline = style().isOriginalDisplayInlineType();
        if (wasInline) {
            // The value is cached in the xPos of the box.  We only need this value if
            // our object was inline originally, since otherwise it would have ended up underneath
            // the inlines.
            RootInlineBox& rootBox = box.root();
            rootBox.blockFlow().setStaticInlinePositionForChild(*this, rootBox.lineTopWithLeading(), roundedLayoutUnit(box.logicalLeft()));
            if (style().hasStaticInlinePosition(box.isHorizontal()))
                setChildNeedsLayout(MarkOnlyThis); // Just go ahead and mark the positioned object as needing layout, so it will update its position properly.
        } else {
            // Our object was a block originally, so we make our normal flow position be
            // just below the line box (as though all the inlines that came before us got
            // wrapped in an anonymous block, which is what would have happened had we been
            // in flow).  This value was cached in the y() of the box.
            layer()->setStaticBlockPosition(box.logicalTop());
            if (style().hasStaticBlockPosition(box.isHorizontal()))
                setChildNeedsLayout(MarkOnlyThis); // Just go ahead and mark the positioned object as needing layout, so it will update its position properly.
        }

        // Nuke the box.
        box.removeFromParent();
        delete &box;
        return;
    }

    if (isReplaced()) {
        setLocation(roundedLayoutPoint(box.topLeft()));
        setInlineBoxWrapper(&box);
    }
}

void RenderBox::deleteLineBoxWrapper()
{
    if (m_inlineBoxWrapper) {
        if (!documentBeingDestroyed())
            m_inlineBoxWrapper->removeFromParent();
        delete m_inlineBoxWrapper;
        m_inlineBoxWrapper = nullptr;
    }
}

LayoutRect RenderBox::clippedOverflowRectForRepaint(const RenderLayerModelObject* repaintContainer) const
{
    if (style().visibility() != VISIBLE && !enclosingLayer()->hasVisibleContent())
        return LayoutRect();

    LayoutRect r = visualOverflowRect();

    // FIXME: layoutDelta needs to be applied in parts before/after transforms and
    // repaint containers. https://bugs.webkit.org/show_bug.cgi?id=23308
    r.move(view().layoutDelta());
    
    // We have to use maximalOutlineSize() because a child might have an outline
    // that projects outside of our overflowRect.
    ASSERT(style().outlineSize() <= view().maximalOutlineSize());
    r.inflate(view().maximalOutlineSize());
    
    computeRectForRepaint(repaintContainer, r);
    return r;
}

static inline bool shouldApplyContainersClipAndOffset(const RenderLayerModelObject* repaintContainer, RenderBox* containerBox)
{
#if PLATFORM(IOS)
    if (!repaintContainer || repaintContainer != containerBox)
        return true;

    return !containerBox->hasLayer() || !containerBox->layer()->usesCompositedScrolling();
#else
    UNUSED_PARAM(repaintContainer);
    UNUSED_PARAM(containerBox);
    return true;
#endif
}

void RenderBox::computeRectForRepaint(const RenderLayerModelObject* repaintContainer, LayoutRect& rect, bool fixed) const
{
    // The rect we compute at each step is shifted by our x/y offset in the parent container's coordinate space.
    // Only when we cross a writing mode boundary will we have to possibly flipForWritingMode (to convert into a more appropriate
    // offset corner for the enclosing container).  This allows for a fully RL or BT document to repaint
    // properly even during layout, since the rect remains flipped all the way until the end.
    //
    // RenderView::computeRectForRepaint then converts the rect to physical coordinates.  We also convert to
    // physical when we hit a repaintContainer boundary.  Therefore the final rect returned is always in the
    // physical coordinate space of the repaintContainer.
    const RenderStyle& styleToUse = style();
    // LayoutState is only valid for root-relative, non-fixed position repainting
    if (view().layoutStateEnabled() && !repaintContainer && styleToUse.position() != FixedPosition) {
        LayoutState* layoutState = view().layoutState();

        if (layer() && layer()->transform())
            rect = LayoutRect(layer()->transform()->mapRect(pixelSnappedForPainting(rect, document().deviceScaleFactor())));

        // We can't trust the bits on RenderObject, because this might be called while re-resolving style.
        if (styleToUse.hasInFlowPosition() && layer())
            rect.move(layer()->offsetForInFlowPosition());

        rect.moveBy(location());
        rect.move(layoutState->m_paintOffset);
        if (layoutState->m_clipped)
            rect.intersect(layoutState->m_clipRect);
        return;
    }

    if (hasReflection())
        rect.unite(reflectedRect(rect));

    if (repaintContainer == this) {
        if (repaintContainer->style().isFlippedBlocksWritingMode())
            flipForWritingMode(rect);
        return;
    }

    bool containerSkipped;
    auto o = container(repaintContainer, &containerSkipped);
    if (!o)
        return;
    
    EPosition position = styleToUse.position();

    // This code isn't necessary for in-flow RenderFlowThreads.
    // Don't add the location of the region in the flow thread for absolute positioned
    // elements because their absolute position already pushes them down through
    // the regions so adding this here and then adding the topLeft again would cause
    // us to add the height twice.
    // The same logic applies for elements flowed directly into the flow thread. Their topLeft member
    // will already contain the portion rect of the region.
    if (o->isOutOfFlowRenderFlowThread() && position != AbsolutePosition && containingBlock() != flowThreadContainingBlock()) {
        RenderRegion* firstRegion = nullptr;
        RenderRegion* lastRegion = nullptr;
        if (toRenderFlowThread(o)->getRegionRangeForBox(this, firstRegion, lastRegion))
            rect.moveBy(firstRegion->flowThreadPortionRect().location());
    }

    if (isWritingModeRoot() && !isOutOfFlowPositioned())
        flipForWritingMode(rect);

    LayoutPoint topLeft = rect.location();
    topLeft.move(locationOffset());

    // We are now in our parent container's coordinate space.  Apply our transform to obtain a bounding box
    // in the parent's coordinate space that encloses us.
    if (hasLayer() && layer()->transform()) {
        fixed = position == FixedPosition;
        rect = LayoutRect(layer()->transform()->mapRect(pixelSnappedForPainting(rect, document().deviceScaleFactor())));
        topLeft = rect.location();
        topLeft.move(locationOffset());
    } else if (position == FixedPosition)
        fixed = true;

    if (position == AbsolutePosition && o->isInFlowPositioned() && o->isRenderInline())
        topLeft += toRenderInline(o)->offsetForInFlowPositionedInline(this);
    else if (styleToUse.hasInFlowPosition() && layer()) {
        // Apply the relative position offset when invalidating a rectangle.  The layer
        // is translated, but the render box isn't, so we need to do this to get the
        // right dirty rect.  Since this is called from RenderObject::setStyle, the relative position
        // flag on the RenderObject has been cleared, so use the one on the style().
        topLeft += layer()->offsetForInFlowPosition();
    }

    // FIXME: We ignore the lightweight clipping rect that controls use, since if |o| is in mid-layout,
    // its controlClipRect will be wrong. For overflow clip we use the values cached by the layer.
    rect.setLocation(topLeft);
    if (o->hasOverflowClip()) {
        RenderBox* containerBox = toRenderBox(o);
        if (shouldApplyContainersClipAndOffset(repaintContainer, containerBox)) {
            containerBox->applyCachedClipAndScrollOffsetForRepaint(rect);
            if (rect.isEmpty())
                return;
        }
    }

    if (containerSkipped) {
        // If the repaintContainer is below o, then we need to map the rect into repaintContainer's coordinates.
        LayoutSize containerOffset = repaintContainer->offsetFromAncestorContainer(o);
        rect.move(-containerOffset);
        return;
    }

    o->computeRectForRepaint(repaintContainer, rect, fixed);
}

void RenderBox::repaintDuringLayoutIfMoved(const LayoutRect& oldRect)
{
    if (oldRect.location() != m_frameRect.location()) {
        LayoutRect newRect = m_frameRect;
        // The child moved.  Invalidate the object's old and new positions.  We have to do this
        // since the object may not have gotten a layout.
        m_frameRect = oldRect;
        repaint();
        repaintOverhangingFloats(true);
        m_frameRect = newRect;
        repaint();
        repaintOverhangingFloats(true);
    }
}

void RenderBox::repaintOverhangingFloats(bool)
{
}

void RenderBox::updateLogicalWidth()
{
    LogicalExtentComputedValues computedValues;
    computeLogicalWidthInRegion(computedValues);

    setLogicalWidth(computedValues.m_extent);
    setLogicalLeft(computedValues.m_position);
    setMarginStart(computedValues.m_margins.m_start);
    setMarginEnd(computedValues.m_margins.m_end);
}

void RenderBox::computeLogicalWidthInRegion(LogicalExtentComputedValues& computedValues, RenderRegion* region) const
{
    computedValues.m_extent = logicalWidth();
    computedValues.m_position = logicalLeft();
    computedValues.m_margins.m_start = marginStart();
    computedValues.m_margins.m_end = marginEnd();

    if (isOutOfFlowPositioned()) {
        // FIXME: This calculation is not patched for block-flow yet.
        // https://bugs.webkit.org/show_bug.cgi?id=46500
        computePositionedLogicalWidth(computedValues, region);
        return;
    }

    // If layout is limited to a subtree, the subtree root's logical width does not change.
    if (element() && view().frameView().layoutRoot(true) == this)
        return;

    // The parent box is flexing us, so it has increased or decreased our
    // width.  Use the width from the style context.
    // FIXME: Account for block-flow in flexible boxes.
    // https://bugs.webkit.org/show_bug.cgi?id=46418
    if (hasOverrideWidth() && (style().borderFit() == BorderFitLines || parent()->isFlexibleBoxIncludingDeprecated())) {
        computedValues.m_extent = overrideLogicalContentWidth() + borderAndPaddingLogicalWidth();
        return;
    }

    // FIXME: Account for block-flow in flexible boxes.
    // https://bugs.webkit.org/show_bug.cgi?id=46418
    bool inVerticalBox = parent()->isDeprecatedFlexibleBox() && (parent()->style().boxOrient() == VERTICAL);
    bool stretching = (parent()->style().boxAlign() == BSTRETCH);
    bool treatAsReplaced = shouldComputeSizeAsReplaced() && (!inVerticalBox || !stretching);

    const RenderStyle& styleToUse = style();
    Length logicalWidthLength = treatAsReplaced ? Length(computeReplacedLogicalWidth(), Fixed) : styleToUse.logicalWidth();

    RenderBlock* cb = containingBlock();
    LayoutUnit containerLogicalWidth = std::max<LayoutUnit>(0, containingBlockLogicalWidthForContentInRegion(region));
    bool hasPerpendicularContainingBlock = cb->isHorizontalWritingMode() != isHorizontalWritingMode();
    
    if (isInline() && !isInlineBlockOrInlineTable()) {
        // just calculate margins
        computedValues.m_margins.m_start = minimumValueForLength(styleToUse.marginStart(), containerLogicalWidth);
        computedValues.m_margins.m_end = minimumValueForLength(styleToUse.marginEnd(), containerLogicalWidth);
        if (treatAsReplaced)
            computedValues.m_extent = std::max<LayoutUnit>(floatValueForLength(logicalWidthLength, 0) + borderAndPaddingLogicalWidth(), minPreferredLogicalWidth());
        return;
    }

    // Width calculations
    if (treatAsReplaced)
        computedValues.m_extent = logicalWidthLength.value() + borderAndPaddingLogicalWidth();
    else {
        LayoutUnit containerWidthInInlineDirection = containerLogicalWidth;
        if (hasPerpendicularContainingBlock)
            containerWidthInInlineDirection = perpendicularContainingBlockLogicalHeight();
        LayoutUnit preferredWidth = computeLogicalWidthInRegionUsing(MainOrPreferredSize, styleToUse.logicalWidth(), containerWidthInInlineDirection, cb, region);
        computedValues.m_extent = constrainLogicalWidthInRegionByMinMax(preferredWidth, containerWidthInInlineDirection, cb, region);
    }

    // Margin calculations.
    if (hasPerpendicularContainingBlock || isFloating() || isInline()) {
        computedValues.m_margins.m_start = minimumValueForLength(styleToUse.marginStart(), containerLogicalWidth);
        computedValues.m_margins.m_end = minimumValueForLength(styleToUse.marginEnd(), containerLogicalWidth);
    } else {
        LayoutUnit containerLogicalWidthForAutoMargins = containerLogicalWidth;
        if (avoidsFloats() && cb->containsFloats())
            containerLogicalWidthForAutoMargins = containingBlockAvailableLineWidthInRegion(region);
        bool hasInvertedDirection = cb->style().isLeftToRightDirection() != style().isLeftToRightDirection();
        computeInlineDirectionMargins(cb, containerLogicalWidthForAutoMargins, computedValues.m_extent,
            hasInvertedDirection ? computedValues.m_margins.m_end : computedValues.m_margins.m_start,
            hasInvertedDirection ? computedValues.m_margins.m_start : computedValues.m_margins.m_end);
    }
    
    if (!hasPerpendicularContainingBlock && containerLogicalWidth && containerLogicalWidth != (computedValues.m_extent + computedValues.m_margins.m_start + computedValues.m_margins.m_end)
        && !isFloating() && !isInline() && !cb->isFlexibleBoxIncludingDeprecated()
#if ENABLE(CSS_GRID_LAYOUT)
        && !cb->isRenderGrid()
#endif
        ) {
        LayoutUnit newMargin = containerLogicalWidth - computedValues.m_extent - cb->marginStartForChild(*this);
        bool hasInvertedDirection = cb->style().isLeftToRightDirection() != style().isLeftToRightDirection();
        if (hasInvertedDirection)
            computedValues.m_margins.m_start = newMargin;
        else
            computedValues.m_margins.m_end = newMargin;
    }
}

LayoutUnit RenderBox::fillAvailableMeasure(LayoutUnit availableLogicalWidth) const
{
    LayoutUnit marginStart = 0;
    LayoutUnit marginEnd = 0;
    return fillAvailableMeasure(availableLogicalWidth, marginStart, marginEnd);
}

LayoutUnit RenderBox::fillAvailableMeasure(LayoutUnit availableLogicalWidth, LayoutUnit& marginStart, LayoutUnit& marginEnd) const
{
    marginStart = minimumValueForLength(style().marginStart(), availableLogicalWidth);
    marginEnd = minimumValueForLength(style().marginEnd(), availableLogicalWidth);
    return availableLogicalWidth - marginStart - marginEnd;
}

LayoutUnit RenderBox::computeIntrinsicLogicalWidthUsing(Length logicalWidthLength, LayoutUnit availableLogicalWidth, LayoutUnit borderAndPadding) const
{
    if (logicalWidthLength.type() == FillAvailable)
        return fillAvailableMeasure(availableLogicalWidth);

    LayoutUnit minLogicalWidth = 0;
    LayoutUnit maxLogicalWidth = 0;
    computeIntrinsicLogicalWidths(minLogicalWidth, maxLogicalWidth);

    if (logicalWidthLength.type() == MinContent)
        return minLogicalWidth + borderAndPadding;

    if (logicalWidthLength.type() == MaxContent)
        return maxLogicalWidth + borderAndPadding;

    if (logicalWidthLength.type() == FitContent) {
        minLogicalWidth += borderAndPadding;
        maxLogicalWidth += borderAndPadding;
        return std::max(minLogicalWidth, std::min(maxLogicalWidth, fillAvailableMeasure(availableLogicalWidth)));
    }

    ASSERT_NOT_REACHED();
    return 0;
}

LayoutUnit RenderBox::computeLogicalWidthInRegionUsing(SizeType widthType, Length logicalWidth, LayoutUnit availableLogicalWidth,
    const RenderBlock* cb, RenderRegion* region) const
{
    if (!logicalWidth.isIntrinsicOrAuto()) {
        // FIXME: If the containing block flow is perpendicular to our direction we need to use the available logical height instead.
        return adjustBorderBoxLogicalWidthForBoxSizing(valueForLength(logicalWidth, availableLogicalWidth));
    }

    if (logicalWidth.isIntrinsic())
        return computeIntrinsicLogicalWidthUsing(logicalWidth, availableLogicalWidth, borderAndPaddingLogicalWidth());

    LayoutUnit marginStart = 0;
    LayoutUnit marginEnd = 0;
    LayoutUnit logicalWidthResult = fillAvailableMeasure(availableLogicalWidth, marginStart, marginEnd);

    if (shrinkToAvoidFloats() && cb->containsFloats())
        logicalWidthResult = std::min(logicalWidthResult, shrinkLogicalWidthToAvoidFloats(marginStart, marginEnd, cb, region));

    if (widthType == MainOrPreferredSize && sizesLogicalWidthToFitContent(widthType))
        return std::max(minPreferredLogicalWidth(), std::min(maxPreferredLogicalWidth(), logicalWidthResult));
    return logicalWidthResult;
}

static bool flexItemHasStretchAlignment(const RenderBox& flexitem)
{
    auto parent = flexitem.parent();
    return flexitem.style().alignSelf() == AlignStretch || (flexitem.style().alignSelf() == AlignAuto && parent->style().alignItems() == AlignStretch);
}

static bool isStretchingColumnFlexItem(const RenderBox& flexitem)
{
    auto parent = flexitem.parent();
    if (parent->isDeprecatedFlexibleBox() && parent->style().boxOrient() == VERTICAL && parent->style().boxAlign() == BSTRETCH)
        return true;

    // We don't stretch multiline flexboxes because they need to apply line spacing (align-content) first.
    if (parent->isFlexibleBox() && parent->style().flexWrap() == FlexNoWrap && parent->style().isColumnFlexDirection() && flexItemHasStretchAlignment(flexitem))
        return true;
    return false;
}

bool RenderBox::sizesLogicalWidthToFitContent(SizeType widthType) const
{
    // Marquees in WinIE are like a mixture of blocks and inline-blocks.  They size as though they're blocks,
    // but they allow text to sit on the same line as the marquee.
    if (isFloating() || (isInlineBlockOrInlineTable() && !isHTMLMarquee()))
        return true;

    // This code may look a bit strange.  Basically width:intrinsic should clamp the size when testing both
    // min-width and width.  max-width is only clamped if it is also intrinsic.
    Length logicalWidth = (widthType == MaxSize) ? style().logicalMaxWidth() : style().logicalWidth();
    if (logicalWidth.type() == Intrinsic)
        return true;

    // Children of a horizontal marquee do not fill the container by default.
    // FIXME: Need to deal with MAUTO value properly.  It could be vertical.
    // FIXME: Think about block-flow here.  Need to find out how marquee direction relates to
    // block-flow (as well as how marquee overflow should relate to block flow).
    // https://bugs.webkit.org/show_bug.cgi?id=46472
    if (parent()->style().overflowX() == OMARQUEE) {
        EMarqueeDirection dir = parent()->style().marqueeDirection();
        if (dir == MAUTO || dir == MFORWARD || dir == MBACKWARD || dir == MLEFT || dir == MRIGHT)
            return true;
    }

    // Flexible box items should shrink wrap, so we lay them out at their intrinsic widths.
    // In the case of columns that have a stretch alignment, we go ahead and layout at the
    // stretched size to avoid an extra layout when applying alignment.
    if (parent()->isFlexibleBox()) {
        // For multiline columns, we need to apply align-content first, so we can't stretch now.
        if (!parent()->style().isColumnFlexDirection() || parent()->style().flexWrap() != FlexNoWrap)
            return true;
        if (!flexItemHasStretchAlignment(*this))
            return true;
    }

    // Flexible horizontal boxes lay out children at their intrinsic widths.  Also vertical boxes
    // that don't stretch their kids lay out their children at their intrinsic widths.
    // FIXME: Think about block-flow here.
    // https://bugs.webkit.org/show_bug.cgi?id=46473
    if (parent()->isDeprecatedFlexibleBox() && (parent()->style().boxOrient() == HORIZONTAL || parent()->style().boxAlign() != BSTRETCH))
        return true;

    // Button, input, select, textarea, and legend treat width value of 'auto' as 'intrinsic' unless it's in a
    // stretching column flexbox.
    // FIXME: Think about block-flow here.
    // https://bugs.webkit.org/show_bug.cgi?id=46473
    if (logicalWidth.type() == Auto && !isStretchingColumnFlexItem(*this) && element() && (isHTMLInputElement(element()) || element()->hasTagName(selectTag) || element()->hasTagName(buttonTag) || isHTMLTextAreaElement(element()) || element()->hasTagName(legendTag)))
        return true;

    if (isHorizontalWritingMode() != containingBlock()->isHorizontalWritingMode())
        return true;

    return false;
}

void RenderBox::computeInlineDirectionMargins(RenderBlock* containingBlock, LayoutUnit containerWidth, LayoutUnit childWidth, LayoutUnit& marginStart, LayoutUnit& marginEnd) const
{
    const RenderStyle& containingBlockStyle = containingBlock->style();
    Length marginStartLength = style().marginStartUsing(&containingBlockStyle);
    Length marginEndLength = style().marginEndUsing(&containingBlockStyle);

    if (isFloating() || isInline()) {
        // Inline blocks/tables and floats don't have their margins increased.
        marginStart = minimumValueForLength(marginStartLength, containerWidth);
        marginEnd = minimumValueForLength(marginEndLength, containerWidth);
        return;
    }

    // Case One: The object is being centered in the containing block's available logical width.
    if ((marginStartLength.isAuto() && marginEndLength.isAuto() && childWidth < containerWidth)
        || (!marginStartLength.isAuto() && !marginEndLength.isAuto() && containingBlock->style().textAlign() == WEBKIT_CENTER)) {
        // Other browsers center the margin box for align=center elements so we match them here.
        LayoutUnit marginStartWidth = minimumValueForLength(marginStartLength, containerWidth);
        LayoutUnit marginEndWidth = minimumValueForLength(marginEndLength, containerWidth);
        LayoutUnit centeredMarginBoxStart = std::max<LayoutUnit>(0, (containerWidth - childWidth - marginStartWidth - marginEndWidth) / 2);
        marginStart = centeredMarginBoxStart + marginStartWidth;
        marginEnd = containerWidth - childWidth - marginStart + marginEndWidth;
        return;
    } 
    
    // Case Two: The object is being pushed to the start of the containing block's available logical width.
    if (marginEndLength.isAuto() && childWidth < containerWidth) {
        marginStart = valueForLength(marginStartLength, containerWidth);
        marginEnd = containerWidth - childWidth - marginStart;
        return;
    } 
    
    // Case Three: The object is being pushed to the end of the containing block's available logical width.
    bool pushToEndFromTextAlign = !marginEndLength.isAuto() && ((!containingBlockStyle.isLeftToRightDirection() && containingBlockStyle.textAlign() == WEBKIT_LEFT)
        || (containingBlockStyle.isLeftToRightDirection() && containingBlockStyle.textAlign() == WEBKIT_RIGHT));
    if ((marginStartLength.isAuto() && childWidth < containerWidth) || pushToEndFromTextAlign) {
        marginEnd = valueForLength(marginEndLength, containerWidth);
        marginStart = containerWidth - childWidth - marginEnd;
        return;
    } 
    
    // Case Four: Either no auto margins, or our width is >= the container width (css2.1, 10.3.3).  In that case
    // auto margins will just turn into 0.
    marginStart = minimumValueForLength(marginStartLength, containerWidth);
    marginEnd = minimumValueForLength(marginEndLength, containerWidth);
}

RenderBoxRegionInfo* RenderBox::renderBoxRegionInfo(RenderRegion* region, RenderBoxRegionInfoFlags cacheFlag) const
{
    // Make sure nobody is trying to call this with a null region.
    if (!region)
        return 0;

    // If we have computed our width in this region already, it will be cached, and we can
    // just return it.
    RenderBoxRegionInfo* boxInfo = region->renderBoxRegionInfo(this);
    if (boxInfo && cacheFlag == CacheRenderBoxRegionInfo)
        return boxInfo;

    // No cached value was found, so we have to compute our insets in this region.
    // FIXME: For now we limit this computation to normal RenderBlocks. Future patches will expand
    // support to cover all boxes.
    RenderFlowThread* flowThread = flowThreadContainingBlock();
    if (isRenderFlowThread() || !flowThread || !canHaveBoxInfoInRegion() || flowThread->style().writingMode() != style().writingMode())
        return 0;

    LogicalExtentComputedValues computedValues;
    computeLogicalWidthInRegion(computedValues, region);

    // Now determine the insets based off where this object is supposed to be positioned.
    RenderBlock* cb = containingBlock();
    RenderRegion* clampedContainingBlockRegion = cb->clampToStartAndEndRegions(region);
    RenderBoxRegionInfo* containingBlockInfo = cb->renderBoxRegionInfo(clampedContainingBlockRegion);
    LayoutUnit containingBlockLogicalWidth = cb->logicalWidth();
    LayoutUnit containingBlockLogicalWidthInRegion = containingBlockInfo ? containingBlockInfo->logicalWidth() : containingBlockLogicalWidth;
    
    LayoutUnit marginStartInRegion = computedValues.m_margins.m_start;
    LayoutUnit startMarginDelta = marginStartInRegion - marginStart();
    LayoutUnit logicalWidthInRegion = computedValues.m_extent;
    LayoutUnit logicalLeftInRegion = computedValues.m_position;
    LayoutUnit widthDelta = logicalWidthInRegion - logicalWidth();
    LayoutUnit logicalLeftDelta = isOutOfFlowPositioned() ? logicalLeftInRegion - logicalLeft() : startMarginDelta;
    LayoutUnit logicalRightInRegion = containingBlockLogicalWidthInRegion - (logicalLeftInRegion + logicalWidthInRegion);
    LayoutUnit oldLogicalRight = containingBlockLogicalWidth - (logicalLeft() + logicalWidth());
    LayoutUnit logicalRightDelta = isOutOfFlowPositioned() ? logicalRightInRegion - oldLogicalRight : startMarginDelta;

    LayoutUnit logicalLeftOffset = 0;
    
    if (!isOutOfFlowPositioned() && avoidsFloats() && cb->containsFloats()) {
        LayoutUnit startPositionDelta = cb->computeStartPositionDeltaForChildAvoidingFloats(*this, marginStartInRegion, region);
        if (cb->style().isLeftToRightDirection())
            logicalLeftDelta += startPositionDelta;
        else
            logicalRightDelta += startPositionDelta;
    }

    if (cb->style().isLeftToRightDirection())
        logicalLeftOffset += logicalLeftDelta;
    else
        logicalLeftOffset -= (widthDelta + logicalRightDelta);
    
    LayoutUnit logicalRightOffset = logicalWidth() - (logicalLeftOffset + logicalWidthInRegion);
    bool isShifted = (containingBlockInfo && containingBlockInfo->isShifted())
            || (style().isLeftToRightDirection() && logicalLeftOffset)
            || (!style().isLeftToRightDirection() && logicalRightOffset);

    // FIXME: Although it's unlikely, these boxes can go outside our bounds, and so we will need to incorporate them into overflow.
    if (cacheFlag == CacheRenderBoxRegionInfo)
        return region->setRenderBoxRegionInfo(this, logicalLeftOffset, logicalWidthInRegion, isShifted);
    return new RenderBoxRegionInfo(logicalLeftOffset, logicalWidthInRegion, isShifted);
}

static bool shouldFlipBeforeAfterMargins(const RenderStyle& containingBlockStyle, const RenderStyle* childStyle)
{
    ASSERT(containingBlockStyle.isHorizontalWritingMode() != childStyle->isHorizontalWritingMode());
    WritingMode childWritingMode = childStyle->writingMode();
    bool shouldFlip = false;
    switch (containingBlockStyle.writingMode()) {
    case TopToBottomWritingMode:
        shouldFlip = (childWritingMode == RightToLeftWritingMode);
        break;
    case BottomToTopWritingMode:
        shouldFlip = (childWritingMode == RightToLeftWritingMode);
        break;
    case RightToLeftWritingMode:
        shouldFlip = (childWritingMode == BottomToTopWritingMode);
        break;
    case LeftToRightWritingMode:
        shouldFlip = (childWritingMode == BottomToTopWritingMode);
        break;
    }

    if (!containingBlockStyle.isLeftToRightDirection())
        shouldFlip = !shouldFlip;

    return shouldFlip;
}

void RenderBox::updateLogicalHeight()
{
    LogicalExtentComputedValues computedValues;
    computeLogicalHeight(logicalHeight(), logicalTop(), computedValues);

    setLogicalHeight(computedValues.m_extent);
    setLogicalTop(computedValues.m_position);
    setMarginBefore(computedValues.m_margins.m_before);
    setMarginAfter(computedValues.m_margins.m_after);
}

void RenderBox::computeLogicalHeight(LayoutUnit logicalHeight, LayoutUnit logicalTop, LogicalExtentComputedValues& computedValues) const
{
    computedValues.m_extent = logicalHeight;
    computedValues.m_position = logicalTop;

    // Cell height is managed by the table and inline non-replaced elements do not support a height property.
    if (isTableCell() || (isInline() && !isReplaced()))
        return;

    Length h;
    if (isOutOfFlowPositioned())
        computePositionedLogicalHeight(computedValues);
    else {
        RenderBlock* cb = containingBlock();
        bool hasPerpendicularContainingBlock = cb->isHorizontalWritingMode() != isHorizontalWritingMode();
    
        if (!hasPerpendicularContainingBlock) {
            bool shouldFlipBeforeAfter = cb->style().writingMode() != style().writingMode();
            computeBlockDirectionMargins(cb,
                shouldFlipBeforeAfter ? computedValues.m_margins.m_after : computedValues.m_margins.m_before,
                shouldFlipBeforeAfter ? computedValues.m_margins.m_before : computedValues.m_margins.m_after);
        }

        // For tables, calculate margins only.
        if (isTable()) {
            if (hasPerpendicularContainingBlock) {
                bool shouldFlipBeforeAfter = shouldFlipBeforeAfterMargins(cb->style(), &style());
                computeInlineDirectionMargins(cb, containingBlockLogicalWidthForContent(), computedValues.m_extent,
                    shouldFlipBeforeAfter ? computedValues.m_margins.m_after : computedValues.m_margins.m_before,
                    shouldFlipBeforeAfter ? computedValues.m_margins.m_before : computedValues.m_margins.m_after);
            }
            return;
        }

        // FIXME: Account for block-flow in flexible boxes.
        // https://bugs.webkit.org/show_bug.cgi?id=46418
        bool inHorizontalBox = parent()->isDeprecatedFlexibleBox() && parent()->style().boxOrient() == HORIZONTAL;
        bool stretching = parent()->style().boxAlign() == BSTRETCH;
        bool treatAsReplaced = shouldComputeSizeAsReplaced() && (!inHorizontalBox || !stretching);
        bool checkMinMaxHeight = false;

        // The parent box is flexing us, so it has increased or decreased our height.  We have to
        // grab our cached flexible height.
        // FIXME: Account for block-flow in flexible boxes.
        // https://bugs.webkit.org/show_bug.cgi?id=46418
        if (hasOverrideHeight() && parent()->isFlexibleBoxIncludingDeprecated())
            h = Length(overrideLogicalContentHeight(), Fixed);
        else if (treatAsReplaced)
            h = Length(computeReplacedLogicalHeight(), Fixed);
        else {
            h = style().logicalHeight();
            checkMinMaxHeight = true;
        }

        // Block children of horizontal flexible boxes fill the height of the box.
        // FIXME: Account for block-flow in flexible boxes.
        // https://bugs.webkit.org/show_bug.cgi?id=46418
        if (h.isAuto() && parent()->isDeprecatedFlexibleBox() && parent()->style().boxOrient() == HORIZONTAL
                && parent()->isStretchingChildren()) {
            h = Length(parentBox()->contentLogicalHeight() - marginBefore() - marginAfter() - borderAndPaddingLogicalHeight(), Fixed);
            checkMinMaxHeight = false;
        }

        LayoutUnit heightResult;
        if (checkMinMaxHeight) {
            heightResult = computeLogicalHeightUsing(style().logicalHeight());
            if (heightResult == -1)
                heightResult = computedValues.m_extent;
            heightResult = constrainLogicalHeightByMinMax(heightResult);
        } else {
            // The only times we don't check min/max height are when a fixed length has
            // been given as an override.  Just use that.  The value has already been adjusted
            // for box-sizing.
            heightResult = h.value() + borderAndPaddingLogicalHeight();
        }

        computedValues.m_extent = heightResult;
        
        if (hasPerpendicularContainingBlock) {
            bool shouldFlipBeforeAfter = shouldFlipBeforeAfterMargins(cb->style(), &style());
            computeInlineDirectionMargins(cb, containingBlockLogicalWidthForContent(), heightResult,
                    shouldFlipBeforeAfter ? computedValues.m_margins.m_after : computedValues.m_margins.m_before,
                    shouldFlipBeforeAfter ? computedValues.m_margins.m_before : computedValues.m_margins.m_after);
        }
    }

    // WinIE quirk: The <html> block always fills the entire canvas in quirks mode.  The <body> always fills the
    // <html> block in quirks mode.  Only apply this quirk if the block is normal flow and no height
    // is specified. When we're printing, we also need this quirk if the body or root has a percentage 
    // height since we don't set a height in RenderView when we're printing. So without this quirk, the 
    // height has nothing to be a percentage of, and it ends up being 0. That is bad.
    bool paginatedContentNeedsBaseHeight = document().printing() && h.isPercent()
        && (isRoot() || (isBody() && document().documentElement()->renderer()->style().logicalHeight().isPercent())) && !isInline();
    if (stretchesToViewport() || paginatedContentNeedsBaseHeight) {
        LayoutUnit margins = collapsedMarginBefore() + collapsedMarginAfter();
        LayoutUnit visibleHeight = view().pageOrViewLogicalHeight();
        if (isRoot())
            computedValues.m_extent = std::max(computedValues.m_extent, visibleHeight - margins);
        else {
            LayoutUnit marginsBordersPadding = margins + parentBox()->marginBefore() + parentBox()->marginAfter() + parentBox()->borderAndPaddingLogicalHeight();
            computedValues.m_extent = std::max(computedValues.m_extent, visibleHeight - marginsBordersPadding);
        }
    }
}

LayoutUnit RenderBox::computeLogicalHeightUsing(const Length& height) const
{
    LayoutUnit logicalHeight = computeContentAndScrollbarLogicalHeightUsing(height);
    if (logicalHeight != -1)
        logicalHeight = adjustBorderBoxLogicalHeightForBoxSizing(logicalHeight);
    return logicalHeight;
}

LayoutUnit RenderBox::computeContentLogicalHeight(const Length& height) const
{
    LayoutUnit heightIncludingScrollbar = computeContentAndScrollbarLogicalHeightUsing(height);
    if (heightIncludingScrollbar == -1)
        return -1;
    return std::max<LayoutUnit>(0, adjustContentBoxLogicalHeightForBoxSizing(heightIncludingScrollbar) - scrollbarLogicalHeight());
}

LayoutUnit RenderBox::computeContentAndScrollbarLogicalHeightUsing(const Length& height) const
{
    if (height.isFixed())
        return height.value();
    if (height.isPercent())
        return computePercentageLogicalHeight(height);
    return -1;
}

bool RenderBox::skipContainingBlockForPercentHeightCalculation(const RenderBox* containingBlock, bool isPerpendicularWritingMode) const
{
    // Flow threads for multicol or paged overflow should be skipped. They are invisible to the DOM,
    // and percent heights of children should be resolved against the multicol or paged container.
    if (containingBlock->isInFlowRenderFlowThread() && !isPerpendicularWritingMode)
        return true;

    // For quirks mode and anonymous blocks, we skip auto-height containingBlocks when computing percentages.
    // For standards mode, we treat the percentage as auto if it has an auto-height containing block.
    if (!document().inQuirksMode() && !containingBlock->isAnonymousBlock())
        return false;
    return !containingBlock->isTableCell() && !containingBlock->isOutOfFlowPositioned() && containingBlock->style().logicalHeight().isAuto() && isHorizontalWritingMode() == containingBlock->isHorizontalWritingMode();
}

LayoutUnit RenderBox::computePercentageLogicalHeight(const Length& height) const
{
    LayoutUnit availableHeight = -1;
    
    bool skippedAutoHeightContainingBlock = false;
    RenderBlock* cb = containingBlock();
    const RenderBox* containingBlockChild = this;
    LayoutUnit rootMarginBorderPaddingHeight = 0;
    bool isHorizontal = isHorizontalWritingMode();
    while (!cb->isRenderView() && skipContainingBlockForPercentHeightCalculation(cb, isHorizontal != cb->isHorizontalWritingMode())) {
        if (cb->isBody() || cb->isRoot())
            rootMarginBorderPaddingHeight += cb->marginBefore() + cb->marginAfter() + cb->borderAndPaddingLogicalHeight();
        skippedAutoHeightContainingBlock = true;
        containingBlockChild = cb;
        cb = cb->containingBlock();
        cb->addPercentHeightDescendant(const_cast<RenderBox&>(*this));
    }

    const RenderStyle& cbstyle = cb->style();

    // A positioned element that specified both top/bottom or that specifies height should be treated as though it has a height
    // explicitly specified that can be used for any percentage computations.
    bool isOutOfFlowPositionedWithSpecifiedHeight = cb->isOutOfFlowPositioned() && (!cbstyle.logicalHeight().isAuto() || (!cbstyle.logicalTop().isAuto() && !cbstyle.logicalBottom().isAuto()));

    bool includeBorderPadding = isTable();

    if (isHorizontal != cb->isHorizontalWritingMode())
        availableHeight = containingBlockChild->containingBlockLogicalWidthForContent();
#if ENABLE(CSS_GRID_LAYOUT)
    else if (hasOverrideContainingBlockLogicalHeight())
        availableHeight = overrideContainingBlockContentLogicalHeight();
#endif
    else if (cb->isTableCell()) {
        if (!skippedAutoHeightContainingBlock) {
            // Table cells violate what the CSS spec says to do with heights. Basically we
            // don't care if the cell specified a height or not. We just always make ourselves
            // be a percentage of the cell's current content height.
            if (!cb->hasOverrideHeight()) {
                // Normally we would let the cell size intrinsically, but scrolling overflow has to be
                // treated differently, since WinIE lets scrolled overflow regions shrink as needed.
                // While we can't get all cases right, we can at least detect when the cell has a specified
                // height or when the table has a specified height. In these cases we want to initially have
                // no size and allow the flexing of the table or the cell to its specified height to cause us
                // to grow to fill the space. This could end up being wrong in some cases, but it is
                // preferable to the alternative (sizing intrinsically and making the row end up too big).
                RenderTableCell* cell = toRenderTableCell(cb);
                if (scrollsOverflowY() && (!cell->style().logicalHeight().isAuto() || !cell->table()->style().logicalHeight().isAuto()))
                    return 0;
                return -1;
            }
            availableHeight = cb->overrideLogicalContentHeight();
            includeBorderPadding = true;
        }
    } else if (cbstyle.logicalHeight().isFixed()) {
        LayoutUnit contentBoxHeight = cb->adjustContentBoxLogicalHeightForBoxSizing(cbstyle.logicalHeight().value());
        availableHeight = std::max<LayoutUnit>(0, cb->constrainContentBoxLogicalHeightByMinMax(contentBoxHeight - cb->scrollbarLogicalHeight()));
    } else if (cbstyle.logicalHeight().isPercent() && !isOutOfFlowPositionedWithSpecifiedHeight) {
        // We need to recur and compute the percentage height for our containing block.
        LayoutUnit heightWithScrollbar = cb->computePercentageLogicalHeight(cbstyle.logicalHeight());
        if (heightWithScrollbar != -1) {
            LayoutUnit contentBoxHeightWithScrollbar = cb->adjustContentBoxLogicalHeightForBoxSizing(heightWithScrollbar);
            // We need to adjust for min/max height because this method does not
            // handle the min/max of the current block, its caller does. So the
            // return value from the recursive call will not have been adjusted
            // yet.
            LayoutUnit contentBoxHeight = cb->constrainContentBoxLogicalHeightByMinMax(contentBoxHeightWithScrollbar - cb->scrollbarLogicalHeight());
            availableHeight = std::max<LayoutUnit>(0, contentBoxHeight);
        }
    } else if (isOutOfFlowPositionedWithSpecifiedHeight) {
        // Don't allow this to affect the block' height() member variable, since this
        // can get called while the block is still laying out its kids.
        LogicalExtentComputedValues computedValues;
        cb->computeLogicalHeight(cb->logicalHeight(), 0, computedValues);
        availableHeight = computedValues.m_extent - cb->borderAndPaddingLogicalHeight() - cb->scrollbarLogicalHeight();
    } else if (cb->isRenderView())
        availableHeight = view().pageOrViewLogicalHeight();

    if (availableHeight == -1)
        return availableHeight;

    availableHeight -= rootMarginBorderPaddingHeight;

    LayoutUnit result = valueForLength(height, availableHeight);
    if (includeBorderPadding) {
        // FIXME: Table cells should default to box-sizing: border-box so we can avoid this hack.
        // It is necessary to use the border-box to match WinIE's broken
        // box model. This is essential for sizing inside
        // table cells using percentage heights.
        result -= borderAndPaddingLogicalHeight();
        return std::max<LayoutUnit>(0, result);
    }
    return result;
}

LayoutUnit RenderBox::computeReplacedLogicalWidth(ShouldComputePreferred shouldComputePreferred) const
{
    return computeReplacedLogicalWidthRespectingMinMaxWidth(computeReplacedLogicalWidthUsing(style().logicalWidth()), shouldComputePreferred);
}

LayoutUnit RenderBox::computeReplacedLogicalWidthRespectingMinMaxWidth(LayoutUnit logicalWidth, ShouldComputePreferred shouldComputePreferred) const
{
    LayoutUnit minLogicalWidth = (shouldComputePreferred == ComputePreferred && style().logicalMinWidth().isPercent()) || style().logicalMinWidth().isUndefined() ? logicalWidth : computeReplacedLogicalWidthUsing(style().logicalMinWidth());
    LayoutUnit maxLogicalWidth = (shouldComputePreferred == ComputePreferred && style().logicalMaxWidth().isPercent()) || style().logicalMaxWidth().isUndefined() ? logicalWidth : computeReplacedLogicalWidthUsing(style().logicalMaxWidth());
    return std::max(minLogicalWidth, std::min(logicalWidth, maxLogicalWidth));
}

LayoutUnit RenderBox::computeReplacedLogicalWidthUsing(Length logicalWidth) const
{
    switch (logicalWidth.type()) {
        case Fixed:
            return adjustContentBoxLogicalWidthForBoxSizing(logicalWidth.value());
        case MinContent:
        case MaxContent: {
            // MinContent/MaxContent don't need the availableLogicalWidth argument.
            LayoutUnit availableLogicalWidth = 0;
            return computeIntrinsicLogicalWidthUsing(logicalWidth, availableLogicalWidth, borderAndPaddingLogicalWidth()) - borderAndPaddingLogicalWidth();
        }
        case FitContent:
        case FillAvailable:
        case Percent: 
        case Calculated: {
            // FIXME: containingBlockLogicalWidthForContent() is wrong if the replaced element's block-flow is perpendicular to the
            // containing block's block-flow.
            // https://bugs.webkit.org/show_bug.cgi?id=46496
            const LayoutUnit cw = isOutOfFlowPositioned() ? containingBlockLogicalWidthForPositioned(toRenderBoxModelObject(container())) : containingBlockLogicalWidthForContent();
            Length containerLogicalWidth = containingBlock()->style().logicalWidth();
            // FIXME: Handle cases when containing block width is calculated or viewport percent.
            // https://bugs.webkit.org/show_bug.cgi?id=91071
            if (logicalWidth.isIntrinsic())
                return computeIntrinsicLogicalWidthUsing(logicalWidth, cw, borderAndPaddingLogicalWidth()) - borderAndPaddingLogicalWidth();
            if (cw > 0 || (!cw && (containerLogicalWidth.isFixed() || containerLogicalWidth.isPercent())))
                return adjustContentBoxLogicalWidthForBoxSizing(minimumValueForLength(logicalWidth, cw));
        }
        FALLTHROUGH;
        case Intrinsic:
        case MinIntrinsic:
        case Auto:
        case Relative:
        case Undefined:
            return intrinsicLogicalWidth();
    }

    ASSERT_NOT_REACHED();
    return 0;
}

LayoutUnit RenderBox::computeReplacedLogicalHeight() const
{
    return computeReplacedLogicalHeightRespectingMinMaxHeight(computeReplacedLogicalHeightUsing(style().logicalHeight()));
}

LayoutUnit RenderBox::computeReplacedLogicalHeightRespectingMinMaxHeight(LayoutUnit logicalHeight) const
{
    LayoutUnit minLogicalHeight = computeReplacedLogicalHeightUsing(style().logicalMinHeight());
    LayoutUnit maxLogicalHeight = style().logicalMaxHeight().isUndefined() ? logicalHeight : computeReplacedLogicalHeightUsing(style().logicalMaxHeight());
    return std::max(minLogicalHeight, std::min(logicalHeight, maxLogicalHeight));
}

LayoutUnit RenderBox::computeReplacedLogicalHeightUsing(Length logicalHeight) const
{
    switch (logicalHeight.type()) {
        case Fixed:
            return adjustContentBoxLogicalHeightForBoxSizing(logicalHeight.value());
        case Percent:
        case Calculated:
        {
            auto cb = isOutOfFlowPositioned() ? container() : containingBlock();
            while (cb->isAnonymous() && !cb->isRenderView()) {
                cb = cb->containingBlock();
                toRenderBlock(cb)->addPercentHeightDescendant(const_cast<RenderBox&>(*this));
            }

            // FIXME: This calculation is not patched for block-flow yet.
            // https://bugs.webkit.org/show_bug.cgi?id=46500
            if (cb->isOutOfFlowPositioned() && cb->style().height().isAuto() && !(cb->style().top().isAuto() || cb->style().bottom().isAuto())) {
                ASSERT_WITH_SECURITY_IMPLICATION(cb->isRenderBlock());
                RenderBlock* block = toRenderBlock(cb);
                LogicalExtentComputedValues computedValues;
                block->computeLogicalHeight(block->logicalHeight(), 0, computedValues);
                LayoutUnit newContentHeight = computedValues.m_extent - block->borderAndPaddingLogicalHeight() - block->scrollbarLogicalHeight();
                LayoutUnit newHeight = block->adjustContentBoxLogicalHeightForBoxSizing(newContentHeight);
                return adjustContentBoxLogicalHeightForBoxSizing(valueForLength(logicalHeight, newHeight));
            }
            
            // FIXME: availableLogicalHeight() is wrong if the replaced element's block-flow is perpendicular to the
            // containing block's block-flow.
            // https://bugs.webkit.org/show_bug.cgi?id=46496
            LayoutUnit availableHeight;
            if (isOutOfFlowPositioned())
                availableHeight = containingBlockLogicalHeightForPositioned(toRenderBoxModelObject(cb));
            else {
                availableHeight = containingBlockLogicalHeightForContent(IncludeMarginBorderPadding);
                // It is necessary to use the border-box to match WinIE's broken
                // box model.  This is essential for sizing inside
                // table cells using percentage heights.
                // FIXME: This needs to be made block-flow-aware.  If the cell and image are perpendicular block-flows, this isn't right.
                // https://bugs.webkit.org/show_bug.cgi?id=46997
                while (cb && !cb->isRenderView() && (cb->style().logicalHeight().isAuto() || cb->style().logicalHeight().isPercent())) {
                    if (cb->isTableCell()) {
                        // Don't let table cells squeeze percent-height replaced elements
                        // <http://bugs.webkit.org/show_bug.cgi?id=15359>
                        availableHeight = std::max(availableHeight, intrinsicLogicalHeight());
                        return valueForLength(logicalHeight, availableHeight - borderAndPaddingLogicalHeight());
                    }
                    toRenderBlock(cb)->addPercentHeightDescendant(const_cast<RenderBox&>(*this));
                    cb = cb->containingBlock();
                }
            }
            return adjustContentBoxLogicalHeightForBoxSizing(valueForLength(logicalHeight, availableHeight));
        }
        default:
            return intrinsicLogicalHeight();
    }
}

LayoutUnit RenderBox::availableLogicalHeight(AvailableLogicalHeightType heightType) const
{
    return constrainLogicalHeightByMinMax(availableLogicalHeightUsing(style().logicalHeight(), heightType));
}

LayoutUnit RenderBox::availableLogicalHeightUsing(const Length& h, AvailableLogicalHeightType heightType) const
{
    // We need to stop here, since we don't want to increase the height of the table
    // artificially.  We're going to rely on this cell getting expanded to some new
    // height, and then when we lay out again we'll use the calculation below.
    if (isTableCell() && (h.isAuto() || h.isPercent())) {
        if (hasOverrideHeight())
            return overrideLogicalContentHeight();
        return logicalHeight() - borderAndPaddingLogicalHeight();
    }

    if (h.isPercent() && isOutOfFlowPositioned() && !isRenderFlowThread()) {
        // FIXME: This is wrong if the containingBlock has a perpendicular writing mode.
        LayoutUnit availableHeight = containingBlockLogicalHeightForPositioned(containingBlock());
        return adjustContentBoxLogicalHeightForBoxSizing(valueForLength(h, availableHeight));
    }

    LayoutUnit heightIncludingScrollbar = computeContentAndScrollbarLogicalHeightUsing(h);
    if (heightIncludingScrollbar != -1)
        return std::max<LayoutUnit>(0, adjustContentBoxLogicalHeightForBoxSizing(heightIncludingScrollbar) - scrollbarLogicalHeight());

    // FIXME: Check logicalTop/logicalBottom here to correctly handle vertical writing-mode.
    // https://bugs.webkit.org/show_bug.cgi?id=46500
    if (isRenderBlock() && isOutOfFlowPositioned() && style().height().isAuto() && !(style().top().isAuto() || style().bottom().isAuto())) {
        RenderBlock* block = const_cast<RenderBlock*>(toRenderBlock(this));
        LogicalExtentComputedValues computedValues;
        block->computeLogicalHeight(block->logicalHeight(), 0, computedValues);
        LayoutUnit newContentHeight = computedValues.m_extent - block->borderAndPaddingLogicalHeight() - block->scrollbarLogicalHeight();
        return adjustContentBoxLogicalHeightForBoxSizing(newContentHeight);
    }

    // FIXME: This is wrong if the containingBlock has a perpendicular writing mode.
    LayoutUnit availableHeight = containingBlockLogicalHeightForContent(heightType);
    if (heightType == ExcludeMarginBorderPadding) {
        // FIXME: Margin collapsing hasn't happened yet, so this incorrectly removes collapsed margins.
        availableHeight -= marginBefore() + marginAfter() + borderAndPaddingLogicalHeight();
    }
    return availableHeight;
}

void RenderBox::computeBlockDirectionMargins(const RenderBlock* containingBlock, LayoutUnit& marginBefore, LayoutUnit& marginAfter) const
{
    if (isTableCell()) {
        // FIXME: Not right if we allow cells to have different directionality than the table.  If we do allow this, though,
        // we may just do it with an extra anonymous block inside the cell.
        marginBefore = 0;
        marginAfter = 0;
        return;
    }

    // Margins are calculated with respect to the logical width of
    // the containing block (8.3)
    LayoutUnit cw = containingBlockLogicalWidthForContent();
    const RenderStyle& containingBlockStyle = containingBlock->style();
    marginBefore = minimumValueForLength(style().marginBeforeUsing(&containingBlockStyle), cw);
    marginAfter = minimumValueForLength(style().marginAfterUsing(&containingBlockStyle), cw);
}

void RenderBox::computeAndSetBlockDirectionMargins(const RenderBlock* containingBlock)
{
    LayoutUnit marginBefore;
    LayoutUnit marginAfter;
    computeBlockDirectionMargins(containingBlock, marginBefore, marginAfter);
    containingBlock->setMarginBeforeForChild(*this, marginBefore);
    containingBlock->setMarginAfterForChild(*this, marginAfter);
}

LayoutUnit RenderBox::containingBlockLogicalWidthForPositioned(const RenderBoxModelObject* containingBlock, RenderRegion* region, bool checkForPerpendicularWritingMode) const
{
    if (checkForPerpendicularWritingMode && containingBlock->isHorizontalWritingMode() != isHorizontalWritingMode())
        return containingBlockLogicalHeightForPositioned(containingBlock, false);

    if (containingBlock->isBox()) {
        bool isFixedPosition = style().position() == FixedPosition;

        RenderFlowThread* flowThread = flowThreadContainingBlock();
        if (!flowThread) {
            if (isFixedPosition && containingBlock->isRenderView())
                return toRenderView(containingBlock)->clientLogicalWidthForFixedPosition();

            return toRenderBox(containingBlock)->clientLogicalWidth();
        }

        if (isFixedPosition && containingBlock->isRenderNamedFlowThread())
            return containingBlock->view().clientLogicalWidth();

        if (!containingBlock->isRenderBlock())
            return toRenderBox(*containingBlock).clientLogicalWidth();

        const RenderBlock* cb = toRenderBlock(containingBlock);
        RenderBoxRegionInfo* boxInfo = 0;
        if (!region) {
            if (containingBlock->isRenderFlowThread() && !checkForPerpendicularWritingMode)
                return toRenderFlowThread(containingBlock)->contentLogicalWidthOfFirstRegion();
            if (isWritingModeRoot()) {
                LayoutUnit cbPageOffset = cb->offsetFromLogicalTopOfFirstPage();
                RenderRegion* cbRegion = cb->regionAtBlockOffset(cbPageOffset);
                if (cbRegion)
                    boxInfo = cb->renderBoxRegionInfo(cbRegion);
            }
        } else if (region && flowThread->isHorizontalWritingMode() == containingBlock->isHorizontalWritingMode()) {
            RenderRegion* containingBlockRegion = cb->clampToStartAndEndRegions(region);
            boxInfo = cb->renderBoxRegionInfo(containingBlockRegion);
        }
        return (boxInfo) ? std::max<LayoutUnit>(0, cb->clientLogicalWidth() - (cb->logicalWidth() - boxInfo->logicalWidth())) : cb->clientLogicalWidth();
    }

    ASSERT(containingBlock->isRenderInline() && containingBlock->isInFlowPositioned());

    const RenderInline* flow = toRenderInline(containingBlock);
    InlineFlowBox* first = flow->firstLineBox();
    InlineFlowBox* last = flow->lastLineBox();

    // If the containing block is empty, return a width of 0.
    if (!first || !last)
        return 0;

    LayoutUnit fromLeft;
    LayoutUnit fromRight;
    if (containingBlock->style().isLeftToRightDirection()) {
        fromLeft = first->logicalLeft() + first->borderLogicalLeft();
        fromRight = last->logicalLeft() + last->logicalWidth() - last->borderLogicalRight();
    } else {
        fromRight = first->logicalLeft() + first->logicalWidth() - first->borderLogicalRight();
        fromLeft = last->logicalLeft() + last->borderLogicalLeft();
    }

    return std::max<LayoutUnit>(0, fromRight - fromLeft);
}

LayoutUnit RenderBox::containingBlockLogicalHeightForPositioned(const RenderBoxModelObject* containingBlock, bool checkForPerpendicularWritingMode) const
{
    if (checkForPerpendicularWritingMode && containingBlock->isHorizontalWritingMode() != isHorizontalWritingMode())
        return containingBlockLogicalWidthForPositioned(containingBlock, 0, false);

    if (containingBlock->isBox()) {
        bool isFixedPosition = style().position() == FixedPosition;

        if (isFixedPosition && containingBlock->isRenderView())
            return toRenderView(containingBlock)->clientLogicalHeightForFixedPosition();

        const RenderBlock* cb = containingBlock->isRenderBlock() ? toRenderBlock(containingBlock) : containingBlock->containingBlock();
        LayoutUnit result = cb->clientLogicalHeight();
        RenderFlowThread* flowThread = flowThreadContainingBlock();
        if (flowThread && containingBlock->isRenderFlowThread() && flowThread->isHorizontalWritingMode() == containingBlock->isHorizontalWritingMode()) {
            if (containingBlock->isRenderNamedFlowThread() && isFixedPosition)
                return containingBlock->view().clientLogicalHeight();
            return toRenderFlowThread(containingBlock)->contentLogicalHeightOfFirstRegion();
        }
        return result;
    }
        
    ASSERT(containingBlock->isRenderInline() && containingBlock->isInFlowPositioned());

    const RenderInline* flow = toRenderInline(containingBlock);
    InlineFlowBox* first = flow->firstLineBox();
    InlineFlowBox* last = flow->lastLineBox();

    // If the containing block is empty, return a height of 0.
    if (!first || !last)
        return 0;

    LayoutUnit heightResult;
    LayoutRect boundingBox = flow->linesBoundingBox();
    if (containingBlock->isHorizontalWritingMode())
        heightResult = boundingBox.height();
    else
        heightResult = boundingBox.width();
    heightResult -= (containingBlock->borderBefore() + containingBlock->borderAfter());
    return heightResult;
}

static void computeInlineStaticDistance(Length& logicalLeft, Length& logicalRight, const RenderBox* child, const RenderBoxModelObject* containerBlock, LayoutUnit containerLogicalWidth, RenderRegion* region)
{
    if (!logicalLeft.isAuto() || !logicalRight.isAuto())
        return;

    // FIXME: The static distance computation has not been patched for mixed writing modes yet.
    if (child->parent()->style().direction() == LTR) {
        LayoutUnit staticPosition = child->layer()->staticInlinePosition() - containerBlock->borderLogicalLeft();
        for (auto curr = child->parent(); curr && curr != containerBlock; curr = curr->container()) {
            if (curr->isBox()) {
                staticPosition += toRenderBox(curr)->logicalLeft();
                if (region && toRenderBox(curr)->isRenderBlock()) {
                    const RenderBlock* cb = toRenderBlock(curr);
                    region = cb->clampToStartAndEndRegions(region);
                    RenderBoxRegionInfo* boxInfo = cb->renderBoxRegionInfo(region);
                    if (boxInfo)
                        staticPosition += boxInfo->logicalLeft();
                }
            }
        }
        logicalLeft.setValue(Fixed, staticPosition);
    } else {
        RenderBox& enclosingBox = child->parent()->enclosingBox();
        LayoutUnit staticPosition = child->layer()->staticInlinePosition() + containerLogicalWidth + containerBlock->borderLogicalLeft();
        for (RenderElement* curr = &enclosingBox; curr; curr = curr->container()) {
            if (curr->isBox()) {
                if (curr != containerBlock)
                    staticPosition -= toRenderBox(curr)->logicalLeft();
                if (curr == &enclosingBox)
                    staticPosition -= enclosingBox.logicalWidth();
                if (region && curr->isRenderBlock()) {
                    const RenderBlock* cb = toRenderBlock(curr);
                    region = cb->clampToStartAndEndRegions(region);
                    RenderBoxRegionInfo* boxInfo = cb->renderBoxRegionInfo(region);
                    if (boxInfo) {
                        if (curr != containerBlock)
                            staticPosition -= cb->logicalWidth() - (boxInfo->logicalLeft() + boxInfo->logicalWidth());
                        if (curr == &enclosingBox)
                            staticPosition += enclosingBox.logicalWidth() - boxInfo->logicalWidth();
                    }
                }
            }
            if (curr == containerBlock)
                break;
        }
        logicalRight.setValue(Fixed, staticPosition);
    }
}

void RenderBox::computePositionedLogicalWidth(LogicalExtentComputedValues& computedValues, RenderRegion* region) const
{
    if (isReplaced()) {
        // FIXME: Positioned replaced elements inside a flow thread are not working properly
        // with variable width regions (see https://bugs.webkit.org/show_bug.cgi?id=69896 ).
        computePositionedLogicalWidthReplaced(computedValues);
        return;
    }

    // QUESTIONS
    // FIXME 1: Should we still deal with these the cases of 'left' or 'right' having
    // the type 'static' in determining whether to calculate the static distance?
    // NOTE: 'static' is not a legal value for 'left' or 'right' as of CSS 2.1.

    // FIXME 2: Can perhaps optimize out cases when max-width/min-width are greater
    // than or less than the computed width().  Be careful of box-sizing and
    // percentage issues.

    // The following is based off of the W3C Working Draft from April 11, 2006 of
    // CSS 2.1: Section 10.3.7 "Absolutely positioned, non-replaced elements"
    // <http://www.w3.org/TR/CSS21/visudet.html#abs-non-replaced-width>
    // (block-style-comments in this function and in computePositionedLogicalWidthUsing()
    // correspond to text from the spec)


    // We don't use containingBlock(), since we may be positioned by an enclosing
    // relative positioned inline.
    const RenderBoxModelObject* containerBlock = toRenderBoxModelObject(container());
    
    const LayoutUnit containerLogicalWidth = containingBlockLogicalWidthForPositioned(containerBlock, region);

    // Use the container block's direction except when calculating the static distance
    // This conforms with the reference results for abspos-replaced-width-margin-000.htm
    // of the CSS 2.1 test suite
    TextDirection containerDirection = containerBlock->style().direction();

    bool isHorizontal = isHorizontalWritingMode();
    const LayoutUnit bordersPlusPadding = borderAndPaddingLogicalWidth();
    const Length marginLogicalLeft = isHorizontal ? style().marginLeft() : style().marginTop();
    const Length marginLogicalRight = isHorizontal ? style().marginRight() : style().marginBottom();

    Length logicalLeftLength = style().logicalLeft();
    Length logicalRightLength = style().logicalRight();

    /*---------------------------------------------------------------------------*\
     * For the purposes of this section and the next, the term "static position"
     * (of an element) refers, roughly, to the position an element would have had
     * in the normal flow. More precisely:
     *
     * * The static position for 'left' is the distance from the left edge of the
     *   containing block to the left margin edge of a hypothetical box that would
     *   have been the first box of the element if its 'position' property had
     *   been 'static' and 'float' had been 'none'. The value is negative if the
     *   hypothetical box is to the left of the containing block.
     * * The static position for 'right' is the distance from the right edge of the
     *   containing block to the right margin edge of the same hypothetical box as
     *   above. The value is positive if the hypothetical box is to the left of the
     *   containing block's edge.
     *
     * But rather than actually calculating the dimensions of that hypothetical box,
     * user agents are free to make a guess at its probable position.
     *
     * For the purposes of calculating the static position, the containing block of
     * fixed positioned elements is the initial containing block instead of the
     * viewport, and all scrollable boxes should be assumed to be scrolled to their
     * origin.
    \*---------------------------------------------------------------------------*/

    // see FIXME 1
    // Calculate the static distance if needed.
    computeInlineStaticDistance(logicalLeftLength, logicalRightLength, this, containerBlock, containerLogicalWidth, region);
    
    // Calculate constraint equation values for 'width' case.
    computePositionedLogicalWidthUsing(style().logicalWidth(), containerBlock, containerDirection,
                                       containerLogicalWidth, bordersPlusPadding,
                                       logicalLeftLength, logicalRightLength, marginLogicalLeft, marginLogicalRight,
                                       computedValues);

    // Calculate constraint equation values for 'max-width' case.
    if (!style().logicalMaxWidth().isUndefined()) {
        LogicalExtentComputedValues maxValues;

        computePositionedLogicalWidthUsing(style().logicalMaxWidth(), containerBlock, containerDirection,
                                           containerLogicalWidth, bordersPlusPadding,
                                           logicalLeftLength, logicalRightLength, marginLogicalLeft, marginLogicalRight,
                                           maxValues);

        if (computedValues.m_extent > maxValues.m_extent) {
            computedValues.m_extent = maxValues.m_extent;
            computedValues.m_position = maxValues.m_position;
            computedValues.m_margins.m_start = maxValues.m_margins.m_start;
            computedValues.m_margins.m_end = maxValues.m_margins.m_end;
        }
    }

    // Calculate constraint equation values for 'min-width' case.
    if (!style().logicalMinWidth().isZero() || style().logicalMinWidth().isIntrinsic()) {
        LogicalExtentComputedValues minValues;

        computePositionedLogicalWidthUsing(style().logicalMinWidth(), containerBlock, containerDirection,
                                           containerLogicalWidth, bordersPlusPadding,
                                           logicalLeftLength, logicalRightLength, marginLogicalLeft, marginLogicalRight,
                                           minValues);

        if (computedValues.m_extent < minValues.m_extent) {
            computedValues.m_extent = minValues.m_extent;
            computedValues.m_position = minValues.m_position;
            computedValues.m_margins.m_start = minValues.m_margins.m_start;
            computedValues.m_margins.m_end = minValues.m_margins.m_end;
        }
    }

    computedValues.m_extent += bordersPlusPadding;
    
    // Adjust logicalLeft if we need to for the flipped version of our writing mode in regions.
    // FIXME: Add support for other types of objects as containerBlock, not only RenderBlock.
    RenderFlowThread* flowThread = flowThreadContainingBlock();
    if (flowThread && !region && isWritingModeRoot() && isHorizontalWritingMode() == containerBlock->isHorizontalWritingMode() && containerBlock->isRenderBlock()) {
        ASSERT(containerBlock->canHaveBoxInfoInRegion());
        LayoutUnit logicalLeftPos = computedValues.m_position;
        const RenderBlock* cb = toRenderBlock(containerBlock);
        LayoutUnit cbPageOffset = cb->offsetFromLogicalTopOfFirstPage();
        RenderRegion* cbRegion = cb->regionAtBlockOffset(cbPageOffset);
        if (cbRegion) {
            RenderBoxRegionInfo* boxInfo = cb->renderBoxRegionInfo(cbRegion);
            if (boxInfo) {
                logicalLeftPos += boxInfo->logicalLeft();
                computedValues.m_position = logicalLeftPos;
            }
        }
    }
}

static void computeLogicalLeftPositionedOffset(LayoutUnit& logicalLeftPos, const RenderBox* child, LayoutUnit logicalWidthValue, const RenderBoxModelObject* containerBlock, LayoutUnit containerLogicalWidth)
{
    // Deal with differing writing modes here.  Our offset needs to be in the containing block's coordinate space. If the containing block is flipped
    // along this axis, then we need to flip the coordinate.  This can only happen if the containing block is both a flipped mode and perpendicular to us.
    if (containerBlock->isHorizontalWritingMode() != child->isHorizontalWritingMode() && containerBlock->style().isFlippedBlocksWritingMode()) {
        logicalLeftPos = containerLogicalWidth - logicalWidthValue - logicalLeftPos;
        logicalLeftPos += (child->isHorizontalWritingMode() ? containerBlock->borderRight() : containerBlock->borderBottom());
    } else
        logicalLeftPos += (child->isHorizontalWritingMode() ? containerBlock->borderLeft() : containerBlock->borderTop());
}

void RenderBox::computePositionedLogicalWidthUsing(Length logicalWidth, const RenderBoxModelObject* containerBlock, TextDirection containerDirection,
                                                   LayoutUnit containerLogicalWidth, LayoutUnit bordersPlusPadding,
                                                   Length logicalLeft, Length logicalRight, Length marginLogicalLeft, Length marginLogicalRight,
                                                   LogicalExtentComputedValues& computedValues) const
{
    if (logicalWidth.isIntrinsic())
        logicalWidth = Length(computeIntrinsicLogicalWidthUsing(logicalWidth, containerLogicalWidth, bordersPlusPadding) - bordersPlusPadding, Fixed);

    // 'left' and 'right' cannot both be 'auto' because one would of been
    // converted to the static position already
    ASSERT(!(logicalLeft.isAuto() && logicalRight.isAuto()));

    LayoutUnit logicalLeftValue = 0;

    const LayoutUnit containerRelativeLogicalWidth = containingBlockLogicalWidthForPositioned(containerBlock, 0, false);

    bool logicalWidthIsAuto = logicalWidth.isIntrinsicOrAuto();
    bool logicalLeftIsAuto = logicalLeft.isAuto();
    bool logicalRightIsAuto = logicalRight.isAuto();
    LayoutUnit& marginLogicalLeftValue = style().isLeftToRightDirection() ? computedValues.m_margins.m_start : computedValues.m_margins.m_end;
    LayoutUnit& marginLogicalRightValue = style().isLeftToRightDirection() ? computedValues.m_margins.m_end : computedValues.m_margins.m_start;

    if (!logicalLeftIsAuto && !logicalWidthIsAuto && !logicalRightIsAuto) {
        /*-----------------------------------------------------------------------*\
         * If none of the three is 'auto': If both 'margin-left' and 'margin-
         * right' are 'auto', solve the equation under the extra constraint that
         * the two margins get equal values, unless this would make them negative,
         * in which case when direction of the containing block is 'ltr' ('rtl'),
         * set 'margin-left' ('margin-right') to zero and solve for 'margin-right'
         * ('margin-left'). If one of 'margin-left' or 'margin-right' is 'auto',
         * solve the equation for that value. If the values are over-constrained,
         * ignore the value for 'left' (in case the 'direction' property of the
         * containing block is 'rtl') or 'right' (in case 'direction' is 'ltr')
         * and solve for that value.
        \*-----------------------------------------------------------------------*/
        // NOTE:  It is not necessary to solve for 'right' in the over constrained
        // case because the value is not used for any further calculations.

        logicalLeftValue = valueForLength(logicalLeft, containerLogicalWidth);
        computedValues.m_extent = adjustContentBoxLogicalWidthForBoxSizing(valueForLength(logicalWidth, containerLogicalWidth));

        const LayoutUnit availableSpace = containerLogicalWidth - (logicalLeftValue + computedValues.m_extent + valueForLength(logicalRight, containerLogicalWidth) + bordersPlusPadding);

        // Margins are now the only unknown
        if (marginLogicalLeft.isAuto() && marginLogicalRight.isAuto()) {
            // Both margins auto, solve for equality
            if (availableSpace >= 0) {
                marginLogicalLeftValue = availableSpace / 2; // split the difference
                marginLogicalRightValue = availableSpace - marginLogicalLeftValue; // account for odd valued differences
            } else {
                // Use the containing block's direction rather than the parent block's
                // per CSS 2.1 reference test abspos-non-replaced-width-margin-000.
                if (containerDirection == LTR) {
                    marginLogicalLeftValue = 0;
                    marginLogicalRightValue = availableSpace; // will be negative
                } else {
                    marginLogicalLeftValue = availableSpace; // will be negative
                    marginLogicalRightValue = 0;
                }
            }
        } else if (marginLogicalLeft.isAuto()) {
            // Solve for left margin
            marginLogicalRightValue = valueForLength(marginLogicalRight, containerRelativeLogicalWidth);
            marginLogicalLeftValue = availableSpace - marginLogicalRightValue;
        } else if (marginLogicalRight.isAuto()) {
            // Solve for right margin
            marginLogicalLeftValue = valueForLength(marginLogicalLeft, containerRelativeLogicalWidth);
            marginLogicalRightValue = availableSpace - marginLogicalLeftValue;
        } else {
            // Over-constrained, solve for left if direction is RTL
            marginLogicalLeftValue = valueForLength(marginLogicalLeft, containerRelativeLogicalWidth);
            marginLogicalRightValue = valueForLength(marginLogicalRight, containerRelativeLogicalWidth);

            // Use the containing block's direction rather than the parent block's
            // per CSS 2.1 reference test abspos-non-replaced-width-margin-000.
            if (containerDirection == RTL)
                logicalLeftValue = (availableSpace + logicalLeftValue) - marginLogicalLeftValue - marginLogicalRightValue;
        }
    } else {
        /*--------------------------------------------------------------------*\
         * Otherwise, set 'auto' values for 'margin-left' and 'margin-right'
         * to 0, and pick the one of the following six rules that applies.
         *
         * 1. 'left' and 'width' are 'auto' and 'right' is not 'auto', then the
         *    width is shrink-to-fit. Then solve for 'left'
         *
         *              OMIT RULE 2 AS IT SHOULD NEVER BE HIT
         * ------------------------------------------------------------------
         * 2. 'left' and 'right' are 'auto' and 'width' is not 'auto', then if
         *    the 'direction' property of the containing block is 'ltr' set
         *    'left' to the static position, otherwise set 'right' to the
         *    static position. Then solve for 'left' (if 'direction is 'rtl')
         *    or 'right' (if 'direction' is 'ltr').
         * ------------------------------------------------------------------
         *
         * 3. 'width' and 'right' are 'auto' and 'left' is not 'auto', then the
         *    width is shrink-to-fit . Then solve for 'right'
         * 4. 'left' is 'auto', 'width' and 'right' are not 'auto', then solve
         *    for 'left'
         * 5. 'width' is 'auto', 'left' and 'right' are not 'auto', then solve
         *    for 'width'
         * 6. 'right' is 'auto', 'left' and 'width' are not 'auto', then solve
         *    for 'right'
         *
         * Calculation of the shrink-to-fit width is similar to calculating the
         * width of a table cell using the automatic table layout algorithm.
         * Roughly: calculate the preferred width by formatting the content
         * without breaking lines other than where explicit line breaks occur,
         * and also calculate the preferred minimum width, e.g., by trying all
         * possible line breaks. CSS 2.1 does not define the exact algorithm.
         * Thirdly, calculate the available width: this is found by solving
         * for 'width' after setting 'left' (in case 1) or 'right' (in case 3)
         * to 0.
         *
         * Then the shrink-to-fit width is:
         * min(max(preferred minimum width, available width), preferred width).
        \*--------------------------------------------------------------------*/
        // NOTE: For rules 3 and 6 it is not necessary to solve for 'right'
        // because the value is not used for any further calculations.

        // Calculate margins, 'auto' margins are ignored.
        marginLogicalLeftValue = minimumValueForLength(marginLogicalLeft, containerRelativeLogicalWidth);
        marginLogicalRightValue = minimumValueForLength(marginLogicalRight, containerRelativeLogicalWidth);

        const LayoutUnit availableSpace = containerLogicalWidth - (marginLogicalLeftValue + marginLogicalRightValue + bordersPlusPadding);

        // FIXME: Is there a faster way to find the correct case?
        // Use rule/case that applies.
        if (logicalLeftIsAuto && logicalWidthIsAuto && !logicalRightIsAuto) {
            // RULE 1: (use shrink-to-fit for width, and solve of left)
            LayoutUnit logicalRightValue = valueForLength(logicalRight, containerLogicalWidth);

            // FIXME: would it be better to have shrink-to-fit in one step?
            LayoutUnit preferredWidth = maxPreferredLogicalWidth() - bordersPlusPadding;
            LayoutUnit preferredMinWidth = minPreferredLogicalWidth() - bordersPlusPadding;
            LayoutUnit availableWidth = availableSpace - logicalRightValue;
            computedValues.m_extent = std::min(std::max(preferredMinWidth, availableWidth), preferredWidth);
            logicalLeftValue = availableSpace - (computedValues.m_extent + logicalRightValue);
        } else if (!logicalLeftIsAuto && logicalWidthIsAuto && logicalRightIsAuto) {
            // RULE 3: (use shrink-to-fit for width, and no need solve of right)
            logicalLeftValue = valueForLength(logicalLeft, containerLogicalWidth);

            // FIXME: would it be better to have shrink-to-fit in one step?
            LayoutUnit preferredWidth = maxPreferredLogicalWidth() - bordersPlusPadding;
            LayoutUnit preferredMinWidth = minPreferredLogicalWidth() - bordersPlusPadding;
            LayoutUnit availableWidth = availableSpace - logicalLeftValue;
            computedValues.m_extent = std::min(std::max(preferredMinWidth, availableWidth), preferredWidth);
        } else if (logicalLeftIsAuto && !logicalWidthIsAuto && !logicalRightIsAuto) {
            // RULE 4: (solve for left)
            computedValues.m_extent = adjustContentBoxLogicalWidthForBoxSizing(valueForLength(logicalWidth, containerLogicalWidth));
            logicalLeftValue = availableSpace - (computedValues.m_extent + valueForLength(logicalRight, containerLogicalWidth));
        } else if (!logicalLeftIsAuto && logicalWidthIsAuto && !logicalRightIsAuto) {
            // RULE 5: (solve for width)
            logicalLeftValue = valueForLength(logicalLeft, containerLogicalWidth);
            computedValues.m_extent = availableSpace - (logicalLeftValue + valueForLength(logicalRight, containerLogicalWidth));
        } else if (!logicalLeftIsAuto && !logicalWidthIsAuto && logicalRightIsAuto) {
            // RULE 6: (no need solve for right)
            logicalLeftValue = valueForLength(logicalLeft, containerLogicalWidth);
            computedValues.m_extent = adjustContentBoxLogicalWidthForBoxSizing(valueForLength(logicalWidth, containerLogicalWidth));
        }
    }

    // Use computed values to calculate the horizontal position.

    // FIXME: This hack is needed to calculate the  logical left position for a 'rtl' relatively
    // positioned, inline because right now, it is using the logical left position
    // of the first line box when really it should use the last line box.  When
    // this is fixed elsewhere, this block should be removed.
    if (containerBlock->isRenderInline() && !containerBlock->style().isLeftToRightDirection()) {
        const RenderInline* flow = toRenderInline(containerBlock);
        InlineFlowBox* firstLine = flow->firstLineBox();
        InlineFlowBox* lastLine = flow->lastLineBox();
        if (firstLine && lastLine && firstLine != lastLine) {
            computedValues.m_position = logicalLeftValue + marginLogicalLeftValue + lastLine->borderLogicalLeft() + (lastLine->logicalLeft() - firstLine->logicalLeft());
            return;
        }
    }

    computedValues.m_position = logicalLeftValue + marginLogicalLeftValue;
    computeLogicalLeftPositionedOffset(computedValues.m_position, this, computedValues.m_extent, containerBlock, containerLogicalWidth);
}

static void computeBlockStaticDistance(Length& logicalTop, Length& logicalBottom, const RenderBox* child, const RenderBoxModelObject* containerBlock)
{
    if (!logicalTop.isAuto() || !logicalBottom.isAuto())
        return;
    
    // FIXME: The static distance computation has not been patched for mixed writing modes.
    LayoutUnit staticLogicalTop = child->layer()->staticBlockPosition() - containerBlock->borderBefore();
    for (RenderElement* curr = child->parent(); curr && curr != containerBlock; curr = curr->container()) {
        if (curr->isBox() && !curr->isTableRow())
            staticLogicalTop += toRenderBox(curr)->logicalTop();
    }
    logicalTop.setValue(Fixed, staticLogicalTop);
}

void RenderBox::computePositionedLogicalHeight(LogicalExtentComputedValues& computedValues) const
{
    if (isReplaced()) {
        computePositionedLogicalHeightReplaced(computedValues);
        return;
    }

    // The following is based off of the W3C Working Draft from April 11, 2006 of
    // CSS 2.1: Section 10.6.4 "Absolutely positioned, non-replaced elements"
    // <http://www.w3.org/TR/2005/WD-CSS21-20050613/visudet.html#abs-non-replaced-height>
    // (block-style-comments in this function and in computePositionedLogicalHeightUsing()
    // correspond to text from the spec)


    // We don't use containingBlock(), since we may be positioned by an enclosing relpositioned inline.
    const RenderBoxModelObject* containerBlock = toRenderBoxModelObject(container());

    const LayoutUnit containerLogicalHeight = containingBlockLogicalHeightForPositioned(containerBlock);

    const RenderStyle& styleToUse = style();
    const LayoutUnit bordersPlusPadding = borderAndPaddingLogicalHeight();
    const Length marginBefore = styleToUse.marginBefore();
    const Length marginAfter = styleToUse.marginAfter();
    Length logicalTopLength = styleToUse.logicalTop();
    Length logicalBottomLength = styleToUse.logicalBottom();

    /*---------------------------------------------------------------------------*\
     * For the purposes of this section and the next, the term "static position"
     * (of an element) refers, roughly, to the position an element would have had
     * in the normal flow. More precisely, the static position for 'top' is the
     * distance from the top edge of the containing block to the top margin edge
     * of a hypothetical box that would have been the first box of the element if
     * its 'position' property had been 'static' and 'float' had been 'none'. The
     * value is negative if the hypothetical box is above the containing block.
     *
     * But rather than actually calculating the dimensions of that hypothetical
     * box, user agents are free to make a guess at its probable position.
     *
     * For the purposes of calculating the static position, the containing block
     * of fixed positioned elements is the initial containing block instead of
     * the viewport.
    \*---------------------------------------------------------------------------*/

    // see FIXME 1
    // Calculate the static distance if needed.
    computeBlockStaticDistance(logicalTopLength, logicalBottomLength, this, containerBlock);

    // Calculate constraint equation values for 'height' case.
    LayoutUnit logicalHeight = computedValues.m_extent;
    computePositionedLogicalHeightUsing(styleToUse.logicalHeight(), containerBlock, containerLogicalHeight, bordersPlusPadding, logicalHeight,
                                        logicalTopLength, logicalBottomLength, marginBefore, marginAfter,
                                        computedValues);

    // Avoid doing any work in the common case (where the values of min-height and max-height are their defaults).
    // see FIXME 2

    // Calculate constraint equation values for 'max-height' case.
    if (!styleToUse.logicalMaxHeight().isUndefined()) {
        LogicalExtentComputedValues maxValues;

        computePositionedLogicalHeightUsing(styleToUse.logicalMaxHeight(), containerBlock, containerLogicalHeight, bordersPlusPadding, logicalHeight,
                                            logicalTopLength, logicalBottomLength, marginBefore, marginAfter,
                                            maxValues);

        if (computedValues.m_extent > maxValues.m_extent) {
            computedValues.m_extent = maxValues.m_extent;
            computedValues.m_position = maxValues.m_position;
            computedValues.m_margins.m_before = maxValues.m_margins.m_before;
            computedValues.m_margins.m_after = maxValues.m_margins.m_after;
        }
    }

    // Calculate constraint equation values for 'min-height' case.
    if (!styleToUse.logicalMinHeight().isZero()) {
        LogicalExtentComputedValues minValues;

        computePositionedLogicalHeightUsing(styleToUse.logicalMinHeight(), containerBlock, containerLogicalHeight, bordersPlusPadding, logicalHeight,
                                            logicalTopLength, logicalBottomLength, marginBefore, marginAfter,
                                            minValues);

        if (computedValues.m_extent < minValues.m_extent) {
            computedValues.m_extent = minValues.m_extent;
            computedValues.m_position = minValues.m_position;
            computedValues.m_margins.m_before = minValues.m_margins.m_before;
            computedValues.m_margins.m_after = minValues.m_margins.m_after;
        }
    }

    // Set final height value.
    computedValues.m_extent += bordersPlusPadding;
    
    // Adjust logicalTop if we need to for perpendicular writing modes in regions.
    // FIXME: Add support for other types of objects as containerBlock, not only RenderBlock.
    RenderFlowThread* flowThread = flowThreadContainingBlock();
    if (flowThread && isHorizontalWritingMode() != containerBlock->isHorizontalWritingMode() && containerBlock->isRenderBlock()) {
        ASSERT(containerBlock->canHaveBoxInfoInRegion());
        LayoutUnit logicalTopPos = computedValues.m_position;
        const RenderBlock* cb = toRenderBlock(containerBlock);
        LayoutUnit cbPageOffset = cb->offsetFromLogicalTopOfFirstPage() - logicalLeft();
        RenderRegion* cbRegion = cb->regionAtBlockOffset(cbPageOffset);
        if (cbRegion) {
            RenderBoxRegionInfo* boxInfo = cb->renderBoxRegionInfo(cbRegion);
            if (boxInfo) {
                logicalTopPos += boxInfo->logicalLeft();
                computedValues.m_position = logicalTopPos;
            }
        }
    }
}

static void computeLogicalTopPositionedOffset(LayoutUnit& logicalTopPos, const RenderBox* child, LayoutUnit logicalHeightValue, const RenderBoxModelObject* containerBlock, LayoutUnit containerLogicalHeight)
{
    // Deal with differing writing modes here.  Our offset needs to be in the containing block's coordinate space. If the containing block is flipped
    // along this axis, then we need to flip the coordinate.  This can only happen if the containing block is both a flipped mode and perpendicular to us.
    if ((child->style().isFlippedBlocksWritingMode() && child->isHorizontalWritingMode() != containerBlock->isHorizontalWritingMode())
        || (child->style().isFlippedBlocksWritingMode() != containerBlock->style().isFlippedBlocksWritingMode() && child->isHorizontalWritingMode() == containerBlock->isHorizontalWritingMode()))
        logicalTopPos = containerLogicalHeight - logicalHeightValue - logicalTopPos;

    // Our offset is from the logical bottom edge in a flipped environment, e.g., right for vertical-rl and bottom for horizontal-bt.
    if (containerBlock->style().isFlippedBlocksWritingMode() && child->isHorizontalWritingMode() == containerBlock->isHorizontalWritingMode()) {
        if (child->isHorizontalWritingMode())
            logicalTopPos += containerBlock->borderBottom();
        else
            logicalTopPos += containerBlock->borderRight();
    } else {
        if (child->isHorizontalWritingMode())
            logicalTopPos += containerBlock->borderTop();
        else
            logicalTopPos += containerBlock->borderLeft();
    }
}

void RenderBox::computePositionedLogicalHeightUsing(Length logicalHeightLength, const RenderBoxModelObject* containerBlock,
                                                    LayoutUnit containerLogicalHeight, LayoutUnit bordersPlusPadding, LayoutUnit logicalHeight,
                                                    Length logicalTop, Length logicalBottom, Length marginBefore, Length marginAfter,
                                                    LogicalExtentComputedValues& computedValues) const
{
    // 'top' and 'bottom' cannot both be 'auto' because 'top would of been
    // converted to the static position in computePositionedLogicalHeight()
    ASSERT(!(logicalTop.isAuto() && logicalBottom.isAuto()));

    LayoutUnit logicalHeightValue;
    LayoutUnit contentLogicalHeight = logicalHeight - bordersPlusPadding;

    const LayoutUnit containerRelativeLogicalWidth = containingBlockLogicalWidthForPositioned(containerBlock, 0, false);

    LayoutUnit logicalTopValue = 0;
    LayoutUnit resolvedLogicalHeight = 0;

    bool logicalHeightIsAuto = logicalHeightLength.isAuto();
    bool logicalTopIsAuto = logicalTop.isAuto();
    bool logicalBottomIsAuto = logicalBottom.isAuto();

    // Height is never unsolved for tables.
    if (isTable()) {
        resolvedLogicalHeight = contentLogicalHeight;
        logicalHeightIsAuto = false;
    } else
        resolvedLogicalHeight = adjustContentBoxLogicalHeightForBoxSizing(valueForLength(logicalHeightLength, containerLogicalHeight));

    if (!logicalTopIsAuto && !logicalHeightIsAuto && !logicalBottomIsAuto) {
        /*-----------------------------------------------------------------------*\
         * If none of the three are 'auto': If both 'margin-top' and 'margin-
         * bottom' are 'auto', solve the equation under the extra constraint that
         * the two margins get equal values. If one of 'margin-top' or 'margin-
         * bottom' is 'auto', solve the equation for that value. If the values
         * are over-constrained, ignore the value for 'bottom' and solve for that
         * value.
        \*-----------------------------------------------------------------------*/
        // NOTE:  It is not necessary to solve for 'bottom' in the over constrained
        // case because the value is not used for any further calculations.

        logicalHeightValue = resolvedLogicalHeight;
        logicalTopValue = valueForLength(logicalTop, containerLogicalHeight);

        const LayoutUnit availableSpace = containerLogicalHeight - (logicalTopValue + logicalHeightValue + valueForLength(logicalBottom, containerLogicalHeight) + bordersPlusPadding);

        // Margins are now the only unknown
        if (marginBefore.isAuto() && marginAfter.isAuto()) {
            // Both margins auto, solve for equality
            // NOTE: This may result in negative values.
            computedValues.m_margins.m_before = availableSpace / 2; // split the difference
            computedValues.m_margins.m_after = availableSpace - computedValues.m_margins.m_before; // account for odd valued differences
        } else if (marginBefore.isAuto()) {
            // Solve for top margin
            computedValues.m_margins.m_after = valueForLength(marginAfter, containerRelativeLogicalWidth);
            computedValues.m_margins.m_before = availableSpace - computedValues.m_margins.m_after;
        } else if (marginAfter.isAuto()) {
            // Solve for bottom margin
            computedValues.m_margins.m_before = valueForLength(marginBefore, containerRelativeLogicalWidth);
            computedValues.m_margins.m_after = availableSpace - computedValues.m_margins.m_before;
        } else {
            // Over-constrained, (no need solve for bottom)
            computedValues.m_margins.m_before = valueForLength(marginBefore, containerRelativeLogicalWidth);
            computedValues.m_margins.m_after = valueForLength(marginAfter, containerRelativeLogicalWidth);
        }
    } else {
        /*--------------------------------------------------------------------*\
         * Otherwise, set 'auto' values for 'margin-top' and 'margin-bottom'
         * to 0, and pick the one of the following six rules that applies.
         *
         * 1. 'top' and 'height' are 'auto' and 'bottom' is not 'auto', then
         *    the height is based on the content, and solve for 'top'.
         *
         *              OMIT RULE 2 AS IT SHOULD NEVER BE HIT
         * ------------------------------------------------------------------
         * 2. 'top' and 'bottom' are 'auto' and 'height' is not 'auto', then
         *    set 'top' to the static position, and solve for 'bottom'.
         * ------------------------------------------------------------------
         *
         * 3. 'height' and 'bottom' are 'auto' and 'top' is not 'auto', then
         *    the height is based on the content, and solve for 'bottom'.
         * 4. 'top' is 'auto', 'height' and 'bottom' are not 'auto', and
         *    solve for 'top'.
         * 5. 'height' is 'auto', 'top' and 'bottom' are not 'auto', and
         *    solve for 'height'.
         * 6. 'bottom' is 'auto', 'top' and 'height' are not 'auto', and
         *    solve for 'bottom'.
        \*--------------------------------------------------------------------*/
        // NOTE: For rules 3 and 6 it is not necessary to solve for 'bottom'
        // because the value is not used for any further calculations.

        // Calculate margins, 'auto' margins are ignored.
        computedValues.m_margins.m_before = minimumValueForLength(marginBefore, containerRelativeLogicalWidth);
        computedValues.m_margins.m_after = minimumValueForLength(marginAfter, containerRelativeLogicalWidth);

        const LayoutUnit availableSpace = containerLogicalHeight - (computedValues.m_margins.m_before + computedValues.m_margins.m_after + bordersPlusPadding);

        // Use rule/case that applies.
        if (logicalTopIsAuto && logicalHeightIsAuto && !logicalBottomIsAuto) {
            // RULE 1: (height is content based, solve of top)
            logicalHeightValue = contentLogicalHeight;
            logicalTopValue = availableSpace - (logicalHeightValue + valueForLength(logicalBottom, containerLogicalHeight));
        } else if (!logicalTopIsAuto && logicalHeightIsAuto && logicalBottomIsAuto) {
            // RULE 3: (height is content based, no need solve of bottom)
            logicalTopValue = valueForLength(logicalTop, containerLogicalHeight);
            logicalHeightValue = contentLogicalHeight;
        } else if (logicalTopIsAuto && !logicalHeightIsAuto && !logicalBottomIsAuto) {
            // RULE 4: (solve of top)
            logicalHeightValue = resolvedLogicalHeight;
            logicalTopValue = availableSpace - (logicalHeightValue + valueForLength(logicalBottom, containerLogicalHeight));
        } else if (!logicalTopIsAuto && logicalHeightIsAuto && !logicalBottomIsAuto) {
            // RULE 5: (solve of height)
            logicalTopValue = valueForLength(logicalTop, containerLogicalHeight);
            logicalHeightValue = std::max<LayoutUnit>(0, availableSpace - (logicalTopValue + valueForLength(logicalBottom, containerLogicalHeight)));
        } else if (!logicalTopIsAuto && !logicalHeightIsAuto && logicalBottomIsAuto) {
            // RULE 6: (no need solve of bottom)
            logicalHeightValue = resolvedLogicalHeight;
            logicalTopValue = valueForLength(logicalTop, containerLogicalHeight);
        }
    }
    computedValues.m_extent = logicalHeightValue;

    // Use computed values to calculate the vertical position.
    computedValues.m_position = logicalTopValue + computedValues.m_margins.m_before;
    computeLogicalTopPositionedOffset(computedValues.m_position, this, logicalHeightValue, containerBlock, containerLogicalHeight);
}

void RenderBox::computePositionedLogicalWidthReplaced(LogicalExtentComputedValues& computedValues) const
{
    // The following is based off of the W3C Working Draft from April 11, 2006 of
    // CSS 2.1: Section 10.3.8 "Absolutely positioned, replaced elements"
    // <http://www.w3.org/TR/2005/WD-CSS21-20050613/visudet.html#abs-replaced-width>
    // (block-style-comments in this function correspond to text from the spec and
    // the numbers correspond to numbers in spec)

    // We don't use containingBlock(), since we may be positioned by an enclosing
    // relative positioned inline.
    const RenderBoxModelObject* containerBlock = toRenderBoxModelObject(container());

    const LayoutUnit containerLogicalWidth = containingBlockLogicalWidthForPositioned(containerBlock);
    const LayoutUnit containerRelativeLogicalWidth = containingBlockLogicalWidthForPositioned(containerBlock, 0, false);

    // To match WinIE, in quirks mode use the parent's 'direction' property
    // instead of the the container block's.
    TextDirection containerDirection = containerBlock->style().direction();

    // Variables to solve.
    bool isHorizontal = isHorizontalWritingMode();
    Length logicalLeft = style().logicalLeft();
    Length logicalRight = style().logicalRight();
    Length marginLogicalLeft = isHorizontal ? style().marginLeft() : style().marginTop();
    Length marginLogicalRight = isHorizontal ? style().marginRight() : style().marginBottom();
    LayoutUnit& marginLogicalLeftAlias = style().isLeftToRightDirection() ? computedValues.m_margins.m_start : computedValues.m_margins.m_end;
    LayoutUnit& marginLogicalRightAlias = style().isLeftToRightDirection() ? computedValues.m_margins.m_end : computedValues.m_margins.m_start;

    /*-----------------------------------------------------------------------*\
     * 1. The used value of 'width' is determined as for inline replaced
     *    elements.
    \*-----------------------------------------------------------------------*/
    // NOTE: This value of width is final in that the min/max width calculations
    // are dealt with in computeReplacedWidth().  This means that the steps to produce
    // correct max/min in the non-replaced version, are not necessary.
    computedValues.m_extent = computeReplacedLogicalWidth() + borderAndPaddingLogicalWidth();

    const LayoutUnit availableSpace = containerLogicalWidth - computedValues.m_extent;

    /*-----------------------------------------------------------------------*\
     * 2. If both 'left' and 'right' have the value 'auto', then if 'direction'
     *    of the containing block is 'ltr', set 'left' to the static position;
     *    else if 'direction' is 'rtl', set 'right' to the static position.
    \*-----------------------------------------------------------------------*/
    // see FIXME 1
    computeInlineStaticDistance(logicalLeft, logicalRight, this, containerBlock, containerLogicalWidth, 0); // FIXME: Pass the region.

    /*-----------------------------------------------------------------------*\
     * 3. If 'left' or 'right' are 'auto', replace any 'auto' on 'margin-left'
     *    or 'margin-right' with '0'.
    \*-----------------------------------------------------------------------*/
    if (logicalLeft.isAuto() || logicalRight.isAuto()) {
        if (marginLogicalLeft.isAuto())
            marginLogicalLeft.setValue(Fixed, 0);
        if (marginLogicalRight.isAuto())
            marginLogicalRight.setValue(Fixed, 0);
    }

    /*-----------------------------------------------------------------------*\
     * 4. If at this point both 'margin-left' and 'margin-right' are still
     *    'auto', solve the equation under the extra constraint that the two
     *    margins must get equal values, unless this would make them negative,
     *    in which case when the direction of the containing block is 'ltr'
     *    ('rtl'), set 'margin-left' ('margin-right') to zero and solve for
     *    'margin-right' ('margin-left').
    \*-----------------------------------------------------------------------*/
    LayoutUnit logicalLeftValue = 0;
    LayoutUnit logicalRightValue = 0;

    if (marginLogicalLeft.isAuto() && marginLogicalRight.isAuto()) {
        // 'left' and 'right' cannot be 'auto' due to step 3
        ASSERT(!(logicalLeft.isAuto() && logicalRight.isAuto()));

        logicalLeftValue = valueForLength(logicalLeft, containerLogicalWidth);
        logicalRightValue = valueForLength(logicalRight, containerLogicalWidth);

        LayoutUnit difference = availableSpace - (logicalLeftValue + logicalRightValue);
        if (difference > 0) {
            marginLogicalLeftAlias = difference / 2; // split the difference
            marginLogicalRightAlias = difference - marginLogicalLeftAlias; // account for odd valued differences
        } else {
            // Use the containing block's direction rather than the parent block's
            // per CSS 2.1 reference test abspos-replaced-width-margin-000.
            if (containerDirection == LTR) {
                marginLogicalLeftAlias = 0;
                marginLogicalRightAlias = difference; // will be negative
            } else {
                marginLogicalLeftAlias = difference; // will be negative
                marginLogicalRightAlias = 0;
            }
        }

    /*-----------------------------------------------------------------------*\
     * 5. If at this point there is an 'auto' left, solve the equation for
     *    that value.
    \*-----------------------------------------------------------------------*/
    } else if (logicalLeft.isAuto()) {
        marginLogicalLeftAlias = valueForLength(marginLogicalLeft, containerRelativeLogicalWidth);
        marginLogicalRightAlias = valueForLength(marginLogicalRight, containerRelativeLogicalWidth);
        logicalRightValue = valueForLength(logicalRight, containerLogicalWidth);

        // Solve for 'left'
        logicalLeftValue = availableSpace - (logicalRightValue + marginLogicalLeftAlias + marginLogicalRightAlias);
    } else if (logicalRight.isAuto()) {
        marginLogicalLeftAlias = valueForLength(marginLogicalLeft, containerRelativeLogicalWidth);
        marginLogicalRightAlias = valueForLength(marginLogicalRight, containerRelativeLogicalWidth);
        logicalLeftValue = valueForLength(logicalLeft, containerLogicalWidth);

        // Solve for 'right'
        logicalRightValue = availableSpace - (logicalLeftValue + marginLogicalLeftAlias + marginLogicalRightAlias);
    } else if (marginLogicalLeft.isAuto()) {
        marginLogicalRightAlias = valueForLength(marginLogicalRight, containerRelativeLogicalWidth);
        logicalLeftValue = valueForLength(logicalLeft, containerLogicalWidth);
        logicalRightValue = valueForLength(logicalRight, containerLogicalWidth);

        // Solve for 'margin-left'
        marginLogicalLeftAlias = availableSpace - (logicalLeftValue + logicalRightValue + marginLogicalRightAlias);
    } else if (marginLogicalRight.isAuto()) {
        marginLogicalLeftAlias = valueForLength(marginLogicalLeft, containerRelativeLogicalWidth);
        logicalLeftValue = valueForLength(logicalLeft, containerLogicalWidth);
        logicalRightValue = valueForLength(logicalRight, containerLogicalWidth);

        // Solve for 'margin-right'
        marginLogicalRightAlias = availableSpace - (logicalLeftValue + logicalRightValue + marginLogicalLeftAlias);
    } else {
        // Nothing is 'auto', just calculate the values.
        marginLogicalLeftAlias = valueForLength(marginLogicalLeft, containerRelativeLogicalWidth);
        marginLogicalRightAlias = valueForLength(marginLogicalRight, containerRelativeLogicalWidth);
        logicalRightValue = valueForLength(logicalRight, containerLogicalWidth);
        logicalLeftValue = valueForLength(logicalLeft, containerLogicalWidth);
        // If the containing block is right-to-left, then push the left position as far to the right as possible
        if (containerDirection == RTL) {
            int totalLogicalWidth = computedValues.m_extent + logicalLeftValue + logicalRightValue +  marginLogicalLeftAlias + marginLogicalRightAlias;
            logicalLeftValue = containerLogicalWidth - (totalLogicalWidth - logicalLeftValue);
        }
    }

    /*-----------------------------------------------------------------------*\
     * 6. If at this point the values are over-constrained, ignore the value
     *    for either 'left' (in case the 'direction' property of the
     *    containing block is 'rtl') or 'right' (in case 'direction' is
     *    'ltr') and solve for that value.
    \*-----------------------------------------------------------------------*/
    // NOTE: Constraints imposed by the width of the containing block and its content have already been accounted for above.

    // FIXME: Deal with differing writing modes here.  Our offset needs to be in the containing block's coordinate space, so that
    // can make the result here rather complicated to compute.

    // Use computed values to calculate the horizontal position.

    // FIXME: This hack is needed to calculate the logical left position for a 'rtl' relatively
    // positioned, inline containing block because right now, it is using the logical left position
    // of the first line box when really it should use the last line box.  When
    // this is fixed elsewhere, this block should be removed.
    if (containerBlock->isRenderInline() && !containerBlock->style().isLeftToRightDirection()) {
        const RenderInline* flow = toRenderInline(containerBlock);
        InlineFlowBox* firstLine = flow->firstLineBox();
        InlineFlowBox* lastLine = flow->lastLineBox();
        if (firstLine && lastLine && firstLine != lastLine) {
            computedValues.m_position = logicalLeftValue + marginLogicalLeftAlias + lastLine->borderLogicalLeft() + (lastLine->logicalLeft() - firstLine->logicalLeft());
            return;
        }
    }

    LayoutUnit logicalLeftPos = logicalLeftValue + marginLogicalLeftAlias;
    computeLogicalLeftPositionedOffset(logicalLeftPos, this, computedValues.m_extent, containerBlock, containerLogicalWidth);
    computedValues.m_position = logicalLeftPos;
}

void RenderBox::computePositionedLogicalHeightReplaced(LogicalExtentComputedValues& computedValues) const
{
    // The following is based off of the W3C Working Draft from April 11, 2006 of
    // CSS 2.1: Section 10.6.5 "Absolutely positioned, replaced elements"
    // <http://www.w3.org/TR/2005/WD-CSS21-20050613/visudet.html#abs-replaced-height>
    // (block-style-comments in this function correspond to text from the spec and
    // the numbers correspond to numbers in spec)

    // We don't use containingBlock(), since we may be positioned by an enclosing relpositioned inline.
    const RenderBoxModelObject* containerBlock = toRenderBoxModelObject(container());

    const LayoutUnit containerLogicalHeight = containingBlockLogicalHeightForPositioned(containerBlock);
    const LayoutUnit containerRelativeLogicalWidth = containingBlockLogicalWidthForPositioned(containerBlock, 0, false);

    // Variables to solve.
    Length marginBefore = style().marginBefore();
    Length marginAfter = style().marginAfter();
    LayoutUnit& marginBeforeAlias = computedValues.m_margins.m_before;
    LayoutUnit& marginAfterAlias = computedValues.m_margins.m_after;

    Length logicalTop = style().logicalTop();
    Length logicalBottom = style().logicalBottom();

    /*-----------------------------------------------------------------------*\
     * 1. The used value of 'height' is determined as for inline replaced
     *    elements.
    \*-----------------------------------------------------------------------*/
    // NOTE: This value of height is final in that the min/max height calculations
    // are dealt with in computeReplacedHeight().  This means that the steps to produce
    // correct max/min in the non-replaced version, are not necessary.
    computedValues.m_extent = computeReplacedLogicalHeight() + borderAndPaddingLogicalHeight();
    const LayoutUnit availableSpace = containerLogicalHeight - computedValues.m_extent;

    /*-----------------------------------------------------------------------*\
     * 2. If both 'top' and 'bottom' have the value 'auto', replace 'top'
     *    with the element's static position.
    \*-----------------------------------------------------------------------*/
    // see FIXME 1
    computeBlockStaticDistance(logicalTop, logicalBottom, this, containerBlock);

    /*-----------------------------------------------------------------------*\
     * 3. If 'bottom' is 'auto', replace any 'auto' on 'margin-top' or
     *    'margin-bottom' with '0'.
    \*-----------------------------------------------------------------------*/
    // FIXME: The spec. says that this step should only be taken when bottom is
    // auto, but if only top is auto, this makes step 4 impossible.
    if (logicalTop.isAuto() || logicalBottom.isAuto()) {
        if (marginBefore.isAuto())
            marginBefore.setValue(Fixed, 0);
        if (marginAfter.isAuto())
            marginAfter.setValue(Fixed, 0);
    }

    /*-----------------------------------------------------------------------*\
     * 4. If at this point both 'margin-top' and 'margin-bottom' are still
     *    'auto', solve the equation under the extra constraint that the two
     *    margins must get equal values.
    \*-----------------------------------------------------------------------*/
    LayoutUnit logicalTopValue = 0;
    LayoutUnit logicalBottomValue = 0;

    if (marginBefore.isAuto() && marginAfter.isAuto()) {
        // 'top' and 'bottom' cannot be 'auto' due to step 2 and 3 combined.
        ASSERT(!(logicalTop.isAuto() || logicalBottom.isAuto()));

        logicalTopValue = valueForLength(logicalTop, containerLogicalHeight);
        logicalBottomValue = valueForLength(logicalBottom, containerLogicalHeight);

        LayoutUnit difference = availableSpace - (logicalTopValue + logicalBottomValue);
        // NOTE: This may result in negative values.
        marginBeforeAlias =  difference / 2; // split the difference
        marginAfterAlias = difference - marginBeforeAlias; // account for odd valued differences

    /*-----------------------------------------------------------------------*\
     * 5. If at this point there is only one 'auto' left, solve the equation
     *    for that value.
    \*-----------------------------------------------------------------------*/
    } else if (logicalTop.isAuto()) {
        marginBeforeAlias = valueForLength(marginBefore, containerRelativeLogicalWidth);
        marginAfterAlias = valueForLength(marginAfter, containerRelativeLogicalWidth);
        logicalBottomValue = valueForLength(logicalBottom, containerLogicalHeight);

        // Solve for 'top'
        logicalTopValue = availableSpace - (logicalBottomValue + marginBeforeAlias + marginAfterAlias);
    } else if (logicalBottom.isAuto()) {
        marginBeforeAlias = valueForLength(marginBefore, containerRelativeLogicalWidth);
        marginAfterAlias = valueForLength(marginAfter, containerRelativeLogicalWidth);
        logicalTopValue = valueForLength(logicalTop, containerLogicalHeight);

        // Solve for 'bottom'
        // NOTE: It is not necessary to solve for 'bottom' because we don't ever
        // use the value.
    } else if (marginBefore.isAuto()) {
        marginAfterAlias = valueForLength(marginAfter, containerRelativeLogicalWidth);
        logicalTopValue = valueForLength(logicalTop, containerLogicalHeight);
        logicalBottomValue = valueForLength(logicalBottom, containerLogicalHeight);

        // Solve for 'margin-top'
        marginBeforeAlias = availableSpace - (logicalTopValue + logicalBottomValue + marginAfterAlias);
    } else if (marginAfter.isAuto()) {
        marginBeforeAlias = valueForLength(marginBefore, containerRelativeLogicalWidth);
        logicalTopValue = valueForLength(logicalTop, containerLogicalHeight);
        logicalBottomValue = valueForLength(logicalBottom, containerLogicalHeight);

        // Solve for 'margin-bottom'
        marginAfterAlias = availableSpace - (logicalTopValue + logicalBottomValue + marginBeforeAlias);
    } else {
        // Nothing is 'auto', just calculate the values.
        marginBeforeAlias = valueForLength(marginBefore, containerRelativeLogicalWidth);
        marginAfterAlias = valueForLength(marginAfter, containerRelativeLogicalWidth);
        logicalTopValue = valueForLength(logicalTop, containerLogicalHeight);
        // NOTE: It is not necessary to solve for 'bottom' because we don't ever
        // use the value.
     }

    /*-----------------------------------------------------------------------*\
     * 6. If at this point the values are over-constrained, ignore the value
     *    for 'bottom' and solve for that value.
    \*-----------------------------------------------------------------------*/
    // NOTE: It is not necessary to do this step because we don't end up using
    // the value of 'bottom' regardless of whether the values are over-constrained
    // or not.

    // Use computed values to calculate the vertical position.
    LayoutUnit logicalTopPos = logicalTopValue + marginBeforeAlias;
    computeLogicalTopPositionedOffset(logicalTopPos, this, computedValues.m_extent, containerBlock, containerLogicalHeight);
    computedValues.m_position = logicalTopPos;
}

LayoutRect RenderBox::localCaretRect(InlineBox* box, int caretOffset, LayoutUnit* extraWidthToEndOfLine)
{
    // VisiblePositions at offsets inside containers either a) refer to the positions before/after
    // those containers (tables and select elements) or b) refer to the position inside an empty block.
    // They never refer to children.
    // FIXME: Paint the carets inside empty blocks differently than the carets before/after elements.

    LayoutRect rect(location(), LayoutSize(caretWidth, height()));
    bool ltr = box ? box->isLeftToRightDirection() : style().isLeftToRightDirection();

    if ((!caretOffset) ^ ltr)
        rect.move(LayoutSize(width() - caretWidth, 0));

    if (box) {
        const RootInlineBox& rootBox = box->root();
        LayoutUnit top = rootBox.lineTop();
        rect.setY(top);
        rect.setHeight(rootBox.lineBottom() - top);
    }

    // If height of box is smaller than font height, use the latter one,
    // otherwise the caret might become invisible.
    //
    // Also, if the box is not a replaced element, always use the font height.
    // This prevents the "big caret" bug described in:
    // <rdar://problem/3777804> Deleting all content in a document can result in giant tall-as-window insertion point
    //
    // FIXME: ignoring :first-line, missing good reason to take care of
    LayoutUnit fontHeight = style().fontMetrics().height();
    if (fontHeight > rect.height() || (!isReplaced() && !isTable()))
        rect.setHeight(fontHeight);

    if (extraWidthToEndOfLine)
        *extraWidthToEndOfLine = x() + width() - rect.maxX();

    // Move to local coords
    rect.moveBy(-location());

    // FIXME: Border/padding should be added for all elements but this workaround
    // is needed because we use offsets inside an "atomic" element to represent
    // positions before and after the element in deprecated editing offsets.
    if (element() && !(editingIgnoresContent(element()) || isRenderedTable(element()))) {
        rect.setX(rect.x() + borderLeft() + paddingLeft());
        rect.setY(rect.y() + paddingTop() + borderTop());
    }

    if (!isHorizontalWritingMode())
        return rect.transposedRect();

    return rect;
}

VisiblePosition RenderBox::positionForPoint(const LayoutPoint& point, const RenderRegion* region)
{
    // no children...return this render object's element, if there is one, and offset 0
    if (!firstChild())
        return createVisiblePosition(nonPseudoElement() ? firstPositionInOrBeforeNode(nonPseudoElement()) : Position());

    if (isTable() && nonPseudoElement()) {
        LayoutUnit right = contentWidth() + horizontalBorderAndPaddingExtent();
        LayoutUnit bottom = contentHeight() + verticalBorderAndPaddingExtent();
        
        if (point.x() < 0 || point.x() > right || point.y() < 0 || point.y() > bottom) {
            if (point.x() <= right / 2)
                return createVisiblePosition(firstPositionInOrBeforeNode(nonPseudoElement()));
            return createVisiblePosition(lastPositionInOrAfterNode(nonPseudoElement()));
        }
    }

    // Pass off to the closest child.
    LayoutUnit minDist = LayoutUnit::max();
    RenderBox* closestRenderer = 0;
    LayoutPoint adjustedPoint = point;
    if (isTableRow())
        adjustedPoint.moveBy(location());

    for (RenderObject* renderObject = firstChild(); renderObject; renderObject = renderObject->nextSibling()) {
        if (!renderObject->isBox())
            continue;

        if (isRenderFlowThread()) {
            ASSERT(region);
            if (!toRenderFlowThread(this)->objectShouldFragmentInFlowRegion(renderObject, region))
                continue;
        }

        RenderBox* renderer = toRenderBox(renderObject);

        if ((!renderer->firstChild() && !renderer->isInline() && !renderer->isRenderBlockFlow() )
            || renderer->style().visibility() != VISIBLE)
            continue;

        LayoutUnit top = renderer->borderTop() + renderer->paddingTop() + (isTableRow() ? LayoutUnit() : renderer->y());
        LayoutUnit bottom = top + renderer->contentHeight();
        LayoutUnit left = renderer->borderLeft() + renderer->paddingLeft() + (isTableRow() ? LayoutUnit() : renderer->x());
        LayoutUnit right = left + renderer->contentWidth();
        
        if (point.x() <= right && point.x() >= left && point.y() <= top && point.y() >= bottom) {
            if (renderer->isTableRow())
                return renderer->positionForPoint(point + adjustedPoint - renderer->locationOffset(), region);
            return renderer->positionForPoint(point - renderer->locationOffset(), region);
        }

        // Find the distance from (x, y) to the box.  Split the space around the box into 8 pieces
        // and use a different compare depending on which piece (x, y) is in.
        LayoutPoint cmp;
        if (point.x() > right) {
            if (point.y() < top)
                cmp = LayoutPoint(right, top);
            else if (point.y() > bottom)
                cmp = LayoutPoint(right, bottom);
            else
                cmp = LayoutPoint(right, point.y());
        } else if (point.x() < left) {
            if (point.y() < top)
                cmp = LayoutPoint(left, top);
            else if (point.y() > bottom)
                cmp = LayoutPoint(left, bottom);
            else
                cmp = LayoutPoint(left, point.y());
        } else {
            if (point.y() < top)
                cmp = LayoutPoint(point.x(), top);
            else
                cmp = LayoutPoint(point.x(), bottom);
        }

        LayoutSize difference = cmp - point;

        LayoutUnit dist = difference.width() * difference.width() + difference.height() * difference.height();
        if (dist < minDist) {
            closestRenderer = renderer;
            minDist = dist;
        }
    }
    
    if (closestRenderer)
        return closestRenderer->positionForPoint(adjustedPoint - closestRenderer->locationOffset(), region);
    
    return createVisiblePosition(firstPositionInOrBeforeNode(nonPseudoElement()));
}

bool RenderBox::shrinkToAvoidFloats() const
{
    // Floating objects don't shrink.  Objects that don't avoid floats don't shrink.  Marquees don't shrink.
    if ((isInline() && !isHTMLMarquee()) || !avoidsFloats() || isFloating())
        return false;
    
    // Only auto width objects can possibly shrink to avoid floats.
    return style().width().isAuto();
}

bool RenderBox::avoidsFloats() const
{
    return isReplaced() || hasOverflowClip() || isHR() || isLegend() || isWritingModeRoot() || isFlexItemIncludingDeprecated();
}

void RenderBox::addVisualEffectOverflow()
{
    if (!style().boxShadow() && !style().hasBorderImageOutsets())
        return;

    LayoutRect borderBox = borderBoxRect();
    addVisualOverflow(applyVisualEffectOverflow(borderBox));

    RenderFlowThread* flowThread = flowThreadContainingBlock();
    if (flowThread)
        flowThread->addRegionsVisualEffectOverflow(this);
}

LayoutRect RenderBox::applyVisualEffectOverflow(const LayoutRect& borderBox) const
{
    bool isFlipped = style().isFlippedBlocksWritingMode();
    bool isHorizontal = isHorizontalWritingMode();
    
    LayoutUnit overflowMinX = borderBox.x();
    LayoutUnit overflowMaxX = borderBox.maxX();
    LayoutUnit overflowMinY = borderBox.y();
    LayoutUnit overflowMaxY = borderBox.maxY();
    
    // Compute box-shadow overflow first.
    if (style().boxShadow()) {
        LayoutUnit shadowLeft;
        LayoutUnit shadowRight;
        LayoutUnit shadowTop;
        LayoutUnit shadowBottom;
        style().getBoxShadowExtent(shadowTop, shadowRight, shadowBottom, shadowLeft);

        // In flipped blocks writing modes such as vertical-rl, the physical right shadow value is actually at the lower x-coordinate.
        overflowMinX = borderBox.x() + ((!isFlipped || isHorizontal) ? shadowLeft : -shadowRight);
        overflowMaxX = borderBox.maxX() + ((!isFlipped || isHorizontal) ? shadowRight : -shadowLeft);
        overflowMinY = borderBox.y() + ((!isFlipped || !isHorizontal) ? shadowTop : -shadowBottom);
        overflowMaxY = borderBox.maxY() + ((!isFlipped || !isHorizontal) ? shadowBottom : -shadowTop);
    }

    // Now compute border-image-outset overflow.
    if (style().hasBorderImageOutsets()) {
        LayoutBoxExtent borderOutsets = style().borderImageOutsets();
        
        // In flipped blocks writing modes, the physical sides are inverted. For example in vertical-rl, the right
        // border is at the lower x coordinate value.
        overflowMinX = std::min(overflowMinX, borderBox.x() - ((!isFlipped || isHorizontal) ? borderOutsets.left() : borderOutsets.right()));
        overflowMaxX = std::max(overflowMaxX, borderBox.maxX() + ((!isFlipped || isHorizontal) ? borderOutsets.right() : borderOutsets.left()));
        overflowMinY = std::min(overflowMinY, borderBox.y() - ((!isFlipped || !isHorizontal) ? borderOutsets.top() : borderOutsets.bottom()));
        overflowMaxY = std::max(overflowMaxY, borderBox.maxY() + ((!isFlipped || !isHorizontal) ? borderOutsets.bottom() : borderOutsets.top()));
    }

    // Add in the final overflow with shadows and outsets combined.
    return LayoutRect(overflowMinX, overflowMinY, overflowMaxX - overflowMinX, overflowMaxY - overflowMinY);
}

void RenderBox::addOverflowFromChild(RenderBox* child, const LayoutSize& delta)
{
    // Never allow flow threads to propagate overflow up to a parent.
    if (child->isRenderFlowThread())
        return;

    RenderFlowThread* flowThread = flowThreadContainingBlock();
    if (flowThread)
        flowThread->addRegionsOverflowFromChild(this, child, delta);

    // Only propagate layout overflow from the child if the child isn't clipping its overflow.  If it is, then
    // its overflow is internal to it, and we don't care about it.  layoutOverflowRectForPropagation takes care of this
    // and just propagates the border box rect instead.
    LayoutRect childLayoutOverflowRect = child->layoutOverflowRectForPropagation(&style());
    childLayoutOverflowRect.move(delta);
    addLayoutOverflow(childLayoutOverflowRect);
            
    // Add in visual overflow from the child.  Even if the child clips its overflow, it may still
    // have visual overflow of its own set from box shadows or reflections.  It is unnecessary to propagate this
    // overflow if we are clipping our own overflow.
    if (child->hasSelfPaintingLayer() || hasOverflowClip())
        return;
    LayoutRect childVisualOverflowRect = child->visualOverflowRectForPropagation(&style());
    childVisualOverflowRect.move(delta);
    addVisualOverflow(childVisualOverflowRect);
}

void RenderBox::addLayoutOverflow(const LayoutRect& rect)
{
    LayoutRect clientBox = flippedClientBoxRect();
    if (clientBox.contains(rect) || rect.isEmpty())
        return;
    
    // For overflow clip objects, we don't want to propagate overflow into unreachable areas.
    LayoutRect overflowRect(rect);
    if (hasOverflowClip() || isRenderView()) {
        // Overflow is in the block's coordinate space and thus is flipped for horizontal-bt and vertical-rl 
        // writing modes.  At this stage that is actually a simplification, since we can treat horizontal-tb/bt as the same
        // and vertical-lr/rl as the same.
        bool hasTopOverflow = isTopLayoutOverflowAllowed();
        bool hasLeftOverflow = isLeftLayoutOverflowAllowed();

        if (!hasTopOverflow)
            overflowRect.shiftYEdgeTo(std::max(overflowRect.y(), clientBox.y()));
        else
            overflowRect.shiftMaxYEdgeTo(std::min(overflowRect.maxY(), clientBox.maxY()));
        if (!hasLeftOverflow)
            overflowRect.shiftXEdgeTo(std::max(overflowRect.x(), clientBox.x()));
        else
            overflowRect.shiftMaxXEdgeTo(std::min(overflowRect.maxX(), clientBox.maxX()));
        
        // Now re-test with the adjusted rectangle and see if it has become unreachable or fully
        // contained.
        if (clientBox.contains(overflowRect) || overflowRect.isEmpty())
            return;
    }

    if (!m_overflow)
        m_overflow = adoptRef(new RenderOverflow(clientBox, borderBoxRect()));
    
    m_overflow->addLayoutOverflow(overflowRect);
}

void RenderBox::addVisualOverflow(const LayoutRect& rect)
{
    LayoutRect borderBox = borderBoxRect();
    if (borderBox.contains(rect) || rect.isEmpty())
        return;
        
    if (!m_overflow)
        m_overflow = adoptRef(new RenderOverflow(flippedClientBoxRect(), borderBox));
    
    m_overflow->addVisualOverflow(rect);
}

void RenderBox::clearOverflow()
{
    m_overflow.clear();
    RenderFlowThread* flowThread = flowThreadContainingBlock();
    if (flowThread)
        flowThread->clearRegionsOverflow(this);
}

inline static bool percentageLogicalHeightIsResolvable(const RenderBox* box)
{
    return RenderBox::percentageLogicalHeightIsResolvableFromBlock(box->containingBlock(), box->isOutOfFlowPositioned());
}

bool RenderBox::percentageLogicalHeightIsResolvableFromBlock(const RenderBlock* containingBlock, bool isOutOfFlowPositioned)
{
    // In quirks mode, blocks with auto height are skipped, and we keep looking for an enclosing
    // block that may have a specified height and then use it. In strict mode, this violates the
    // specification, which states that percentage heights just revert to auto if the containing
    // block has an auto height. We still skip anonymous containing blocks in both modes, though, and look
    // only at explicit containers.
    const RenderBlock* cb = containingBlock;
    bool inQuirksMode = cb->document().inQuirksMode();
    while (!cb->isRenderView() && !cb->isBody() && !cb->isTableCell() && !cb->isOutOfFlowPositioned() && cb->style().logicalHeight().isAuto()) {
        if (!inQuirksMode && !cb->isAnonymousBlock())
            break;
        cb = cb->containingBlock();
    }

    // A positioned element that specified both top/bottom or that specifies height should be treated as though it has a height
    // explicitly specified that can be used for any percentage computations.
    // FIXME: We can't just check top/bottom here.
    // https://bugs.webkit.org/show_bug.cgi?id=46500
    bool isOutOfFlowPositionedWithSpecifiedHeight = cb->isOutOfFlowPositioned() && (!cb->style().logicalHeight().isAuto() || (!cb->style().top().isAuto() && !cb->style().bottom().isAuto()));

    // Table cells violate what the CSS spec says to do with heights.  Basically we
    // don't care if the cell specified a height or not.  We just always make ourselves
    // be a percentage of the cell's current content height.
    if (cb->isTableCell())
        return true;

    // Otherwise we only use our percentage height if our containing block had a specified
    // height.
    if (cb->style().logicalHeight().isFixed())
        return true;
    if (cb->style().logicalHeight().isPercent() && !isOutOfFlowPositionedWithSpecifiedHeight)
        return percentageLogicalHeightIsResolvableFromBlock(cb->containingBlock(), cb->isOutOfFlowPositioned());
    if (cb->isRenderView() || inQuirksMode || isOutOfFlowPositionedWithSpecifiedHeight)
        return true;
    if (cb->isRoot() && isOutOfFlowPositioned) {
        // Match the positioned objects behavior, which is that positioned objects will fill their viewport
        // always.  Note we could only hit this case by recurring into computePercentageLogicalHeight on a positioned containing block.
        return true;
    }

    return false;
}

bool RenderBox::hasUnsplittableScrollingOverflow() const
{
    // We will paginate as long as we don't scroll overflow in the pagination direction.
    bool isHorizontal = isHorizontalWritingMode();
    if ((isHorizontal && !scrollsOverflowY()) || (!isHorizontal && !scrollsOverflowX()))
        return false;
    
    // We do have overflow. We'll still be willing to paginate as long as the block
    // has auto logical height, auto or undefined max-logical-height and a zero or auto min-logical-height.
    // Note this is just a heuristic, and it's still possible to have overflow under these
    // conditions, but it should work out to be good enough for common cases. Paginating overflow
    // with scrollbars present is not the end of the world and is what we used to do in the old model anyway.
    return !style().logicalHeight().isIntrinsicOrAuto()
        || (!style().logicalMaxHeight().isIntrinsicOrAuto() && !style().logicalMaxHeight().isUndefined() && (!style().logicalMaxHeight().isPercent() || percentageLogicalHeightIsResolvable(this)))
        || (!style().logicalMinHeight().isIntrinsicOrAuto() && style().logicalMinHeight().isPositive() && (!style().logicalMinHeight().isPercent() || percentageLogicalHeightIsResolvable(this)));
}

bool RenderBox::isUnsplittableForPagination() const
{
    return isReplaced()
        || hasUnsplittableScrollingOverflow()
        || (parent() && isWritingModeRoot())
        || isRenderNamedFlowFragmentContainer()
        || fixedPositionedWithNamedFlowContainingBlock();
}

LayoutUnit RenderBox::lineHeight(bool /*firstLine*/, LineDirectionMode direction, LinePositionMode /*linePositionMode*/) const
{
    if (isReplaced())
        return direction == HorizontalLine ? m_marginBox.top() + height() + m_marginBox.bottom() : m_marginBox.right() + width() + m_marginBox.left();
    return 0;
}

int RenderBox::baselinePosition(FontBaseline baselineType, bool /*firstLine*/, LineDirectionMode direction, LinePositionMode /*linePositionMode*/) const
{
    if (isReplaced()) {
        int result = direction == HorizontalLine ? m_marginBox.top() + height() + m_marginBox.bottom() : m_marginBox.right() + width() + m_marginBox.left();
        if (baselineType == AlphabeticBaseline)
            return result;
        return result - result / 2;
    }
    return 0;
}


RenderLayer* RenderBox::enclosingFloatPaintingLayer() const
{
    for (auto& box : lineageOfType<RenderBox>(*this)) {
        if (box.layer() && box.layer()->isSelfPaintingLayer())
            return box.layer();
    }
    return nullptr;
}

LayoutRect RenderBox::logicalVisualOverflowRectForPropagation(RenderStyle* parentStyle) const
{
    LayoutRect rect = visualOverflowRectForPropagation(parentStyle);
    if (!parentStyle->isHorizontalWritingMode())
        return rect.transposedRect();
    return rect;
}

LayoutRect RenderBox::visualOverflowRectForPropagation(RenderStyle* parentStyle) const
{
    // If the writing modes of the child and parent match, then we don't have to 
    // do anything fancy. Just return the result.
    LayoutRect rect = visualOverflowRect();
    if (parentStyle->writingMode() == style().writingMode())
        return rect;
    
    // We are putting ourselves into our parent's coordinate space.  If there is a flipped block mismatch
    // in a particular axis, then we have to flip the rect along that axis.
    if (style().writingMode() == RightToLeftWritingMode || parentStyle->writingMode() == RightToLeftWritingMode)
        rect.setX(width() - rect.maxX());
    else if (style().writingMode() == BottomToTopWritingMode || parentStyle->writingMode() == BottomToTopWritingMode)
        rect.setY(height() - rect.maxY());

    return rect;
}

LayoutRect RenderBox::logicalLayoutOverflowRectForPropagation(RenderStyle* parentStyle) const
{
    LayoutRect rect = layoutOverflowRectForPropagation(parentStyle);
    if (!parentStyle->isHorizontalWritingMode())
        return rect.transposedRect();
    return rect;
}

LayoutRect RenderBox::layoutOverflowRectForPropagation(RenderStyle* parentStyle) const
{
    // Only propagate interior layout overflow if we don't clip it.
    LayoutRect rect = borderBoxRect();
    if (!hasOverflowClip())
        rect.unite(layoutOverflowRect());

    bool hasTransform = hasLayer() && layer()->transform();
#if PLATFORM(IOS)
    if (isInFlowPositioned() || (hasTransform && document().settings()->shouldTransformsAffectOverflow())) {
#else
    if (isInFlowPositioned() || hasTransform) {
#endif
        // If we are relatively positioned or if we have a transform, then we have to convert
        // this rectangle into physical coordinates, apply relative positioning and transforms
        // to it, and then convert it back.
        flipForWritingMode(rect);
        
        if (hasTransform)
            rect = layer()->currentTransform().mapRect(rect);

        if (isInFlowPositioned())
            rect.move(offsetForInFlowPosition());
        
        // Now we need to flip back.
        flipForWritingMode(rect);
    }
    
    // If the writing modes of the child and parent match, then we don't have to 
    // do anything fancy. Just return the result.
    if (parentStyle->writingMode() == style().writingMode())
        return rect;
    
    // We are putting ourselves into our parent's coordinate space.  If there is a flipped block mismatch
    // in a particular axis, then we have to flip the rect along that axis.
    if (style().writingMode() == RightToLeftWritingMode || parentStyle->writingMode() == RightToLeftWritingMode)
        rect.setX(width() - rect.maxX());
    else if (style().writingMode() == BottomToTopWritingMode || parentStyle->writingMode() == BottomToTopWritingMode)
        rect.setY(height() - rect.maxY());

    return rect;
}

LayoutRect RenderBox::flippedClientBoxRect() const
{
    // Because of the special coodinate system used for overflow rectangles (not quite logical, not
    // quite physical), we need to flip the block progression coordinate in vertical-rl and
    // horizontal-bt writing modes. Apart from that, this method does the same as clientBoxRect().

    LayoutUnit left = borderLeft();
    LayoutUnit top = borderTop();
    LayoutUnit right = borderRight();
    LayoutUnit bottom = borderBottom();
    // Calculate physical padding box.
    LayoutRect rect(left, top, width() - left - right, height() - top - bottom);
    // Flip block progression axis if writing mode is vertical-rl or horizontal-bt.
    flipForWritingMode(rect);
    // Subtract space occupied by scrollbars. They are at their physical edge in this coordinate
    // system, so order is important here: first flip, then subtract scrollbars.
    rect.contract(verticalScrollbarWidth(), horizontalScrollbarHeight());
    return rect;
}

LayoutRect RenderBox::overflowRectForPaintRejection(RenderNamedFlowFragment* namedFlowFragment) const
{
    LayoutRect overflowRect = visualOverflowRect();
    
    // When using regions, some boxes might have their frame rect relative to the flow thread, which could
    // cause the paint rejection algorithm to prevent them from painting when using different width regions.
    // e.g. an absolutely positioned box with bottom:0px and right:0px would have it's frameRect.x relative
    // to the flow thread, not the last region (in which it will end up because of bottom:0px)
    if (namedFlowFragment && namedFlowFragment->isValid()) {
        RenderFlowThread* flowThread = namedFlowFragment->flowThread();
        RenderRegion* startRegion = nullptr;
        RenderRegion* endRegion = nullptr;
        if (flowThread->getRegionRangeForBox(this, startRegion, endRegion))
            overflowRect.unite(namedFlowFragment->visualOverflowRectForBox(this));
    }
    
    if (!m_overflow || !usesCompositedScrolling())
        return overflowRect;

    overflowRect.unite(layoutOverflowRect());
    overflowRect.move(-scrolledContentOffset());
    return overflowRect;
}

LayoutUnit RenderBox::offsetLeft() const
{
    return adjustedPositionRelativeToOffsetParent(topLeftLocation()).x();
}

LayoutUnit RenderBox::offsetTop() const
{
    return adjustedPositionRelativeToOffsetParent(topLeftLocation()).y();
}

LayoutPoint RenderBox::flipForWritingModeForChild(const RenderBox* child, const LayoutPoint& point) const
{
    if (!style().isFlippedBlocksWritingMode())
        return point;
    
    // The child is going to add in its x() and y(), so we have to make sure it ends up in
    // the right place.
    if (isHorizontalWritingMode())
        return LayoutPoint(point.x(), point.y() + height() - child->height() - (2 * child->y()));
    return LayoutPoint(point.x() + width() - child->width() - (2 * child->x()), point.y());
}

void RenderBox::flipForWritingMode(LayoutRect& rect) const
{
    if (!style().isFlippedBlocksWritingMode())
        return;

    if (isHorizontalWritingMode())
        rect.setY(height() - rect.maxY());
    else
        rect.setX(width() - rect.maxX());
}

LayoutUnit RenderBox::flipForWritingMode(LayoutUnit position) const
{
    if (!style().isFlippedBlocksWritingMode())
        return position;
    return logicalHeight() - position;
}

LayoutPoint RenderBox::flipForWritingMode(const LayoutPoint& position) const
{
    if (!style().isFlippedBlocksWritingMode())
        return position;
    return isHorizontalWritingMode() ? LayoutPoint(position.x(), height() - position.y()) : LayoutPoint(width() - position.x(), position.y());
}

LayoutSize RenderBox::flipForWritingMode(const LayoutSize& offset) const
{
    if (!style().isFlippedBlocksWritingMode())
        return offset;
    return isHorizontalWritingMode() ? LayoutSize(offset.width(), height() - offset.height()) : LayoutSize(width() - offset.width(), offset.height());
}

FloatPoint RenderBox::flipForWritingMode(const FloatPoint& position) const
{
    if (!style().isFlippedBlocksWritingMode())
        return position;
    return isHorizontalWritingMode() ? FloatPoint(position.x(), height() - position.y()) : FloatPoint(width() - position.x(), position.y());
}

void RenderBox::flipForWritingMode(FloatRect& rect) const
{
    if (!style().isFlippedBlocksWritingMode())
        return;

    if (isHorizontalWritingMode())
        rect.setY(height() - rect.maxY());
    else
        rect.setX(width() - rect.maxX());
}

LayoutPoint RenderBox::topLeftLocation() const
{
    RenderBlock* containerBlock = containingBlock();
    if (!containerBlock || containerBlock == this)
        return location();
    return containerBlock->flipForWritingModeForChild(this, location());
}

LayoutSize RenderBox::topLeftLocationOffset() const
{
    RenderBlock* containerBlock = containingBlock();
    if (!containerBlock || containerBlock == this)
        return locationOffset();
    
    LayoutRect rect(frameRect());
    containerBlock->flipForWritingMode(rect); // FIXME: This is wrong if we are an absolutely positioned object enclosed by a relative-positioned inline.
    return LayoutSize(rect.x(), rect.y());
}

bool RenderBox::hasRelativeDimensions() const
{
    return style().height().isPercent() || style().width().isPercent()
            || style().maxHeight().isPercent() || style().maxWidth().isPercent()
            || style().minHeight().isPercent() || style().minWidth().isPercent();
}

bool RenderBox::hasRelativeLogicalHeight() const
{
    return style().logicalHeight().isPercent()
            || style().logicalMinHeight().isPercent()
            || style().logicalMaxHeight().isPercent();
}

static void markBoxForRelayoutAfterSplit(RenderBox* box)
{
    // FIXME: The table code should handle that automatically. If not,
    // we should fix it and remove the table part checks.
    if (box->isTable()) {
        // Because we may have added some sections with already computed column structures, we need to
        // sync the table structure with them now. This avoids crashes when adding new cells to the table.
        toRenderTable(box)->forceSectionsRecalc();
    } else if (box->isTableSection())
        toRenderTableSection(box)->setNeedsCellRecalc();

    box->setNeedsLayoutAndPrefWidthsRecalc();
}

RenderObject* RenderBox::splitAnonymousBoxesAroundChild(RenderObject* beforeChild)
{
    bool didSplitParentAnonymousBoxes = false;

    while (beforeChild->parent() != this) {
        RenderBox* boxToSplit = toRenderBox(beforeChild->parent());
        if (boxToSplit->firstChild() != beforeChild && boxToSplit->isAnonymous()) {
            didSplitParentAnonymousBoxes = true;

            // We have to split the parent box into two boxes and move children
            // from |beforeChild| to end into the new post box.
            RenderBox* postBox = boxToSplit->createAnonymousBoxWithSameTypeAs(this);
            postBox->setChildrenInline(boxToSplit->childrenInline());
            RenderBox* parentBox = toRenderBox(boxToSplit->parent());
            // We need to invalidate the |parentBox| before inserting the new node
            // so that the table repainting logic knows the structure is dirty.
            // See for example RenderTableCell:clippedOverflowRectForRepaint.
            markBoxForRelayoutAfterSplit(parentBox);
            parentBox->insertChildInternal(postBox, boxToSplit->nextSibling(), NotifyChildren);
            boxToSplit->moveChildrenTo(postBox, beforeChild, 0, true);

            markBoxForRelayoutAfterSplit(boxToSplit);
            markBoxForRelayoutAfterSplit(postBox);

            beforeChild = postBox;
        } else
            beforeChild = boxToSplit;
    }

    if (didSplitParentAnonymousBoxes)
        markBoxForRelayoutAfterSplit(this);

    ASSERT(beforeChild->parent() == this);
    return beforeChild;
}

LayoutUnit RenderBox::offsetFromLogicalTopOfFirstPage() const
{
    LayoutState* layoutState = view().layoutState();
    if ((layoutState && !layoutState->isPaginated()) || (!layoutState && !flowThreadContainingBlock()))
        return 0;

    RenderBlock* containerBlock = containingBlock();
    return containerBlock->offsetFromLogicalTopOfFirstPage() + logicalTop();
}

} // namespace WebCore
