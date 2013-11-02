/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2006, 2007, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#ifndef RenderBoxModelObject_h
#define RenderBoxModelObject_h

#include "LayoutRect.h"
#include "RenderLayerModelObject.h"

namespace WebCore {

// Modes for some of the line-related functions.
enum LinePositionMode { PositionOnContainingLine, PositionOfInteriorLineBoxes };
enum LineDirectionMode { HorizontalLine, VerticalLine };
typedef unsigned BorderEdgeFlags;

enum BackgroundBleedAvoidance {
    BackgroundBleedNone,
    BackgroundBleedShrinkBackground,
    BackgroundBleedUseTransparencyLayer,
    BackgroundBleedBackgroundOverBorder
};

enum ContentChangeType {
    ImageChanged,
    MaskImageChanged,
    BackgroundImageChanged,
    CanvasChanged,
    CanvasPixelsChanged,
    VideoChanged,
    FullScreenChanged
};

class KeyframeList;
class InlineFlowBox;
class RenderTextFragment;
class StickyPositionViewportConstraints;

// This class is the base for all objects that adhere to the CSS box model as described
// at http://www.w3.org/TR/CSS21/box.html

class RenderBoxModelObject : public RenderLayerModelObject {
public:
    virtual ~RenderBoxModelObject();
    
    LayoutSize relativePositionOffset() const;
    LayoutSize relativePositionLogicalOffset() const { return style().isHorizontalWritingMode() ? relativePositionOffset() : relativePositionOffset().transposedSize(); }

    void computeStickyPositionConstraints(StickyPositionViewportConstraints&, const FloatRect& viewportRect) const;
    LayoutSize stickyPositionOffset() const;
    LayoutSize stickyPositionLogicalOffset() const { return style().isHorizontalWritingMode() ? stickyPositionOffset() : stickyPositionOffset().transposedSize(); }

    LayoutSize offsetForInFlowPosition() const;

    // IE extensions. Used to calculate offsetWidth/Height.  Overridden by inlines (RenderFlow)
    // to return the remaining width on a given line (and the height of a single line).
    virtual LayoutUnit offsetLeft() const;
    virtual LayoutUnit offsetTop() const;
    virtual LayoutUnit offsetWidth() const = 0;
    virtual LayoutUnit offsetHeight() const = 0;

    int pixelSnappedOffsetLeft() const { return roundToInt(offsetLeft()); }
    int pixelSnappedOffsetTop() const { return roundToInt(offsetTop()); }
    virtual int pixelSnappedOffsetWidth() const;
    virtual int pixelSnappedOffsetHeight() const;

    virtual void updateFromStyle() OVERRIDE;

    virtual bool requiresLayer() const OVERRIDE { return isRoot() || isPositioned() || createsGroup() || hasClipPath() || hasTransform() || hasHiddenBackface() || hasReflection() || style().specifiesColumns(); }

    // This will work on inlines to return the bounding box of all of the lines' border boxes.
    virtual IntRect borderBoundingBox() const = 0;

    // These return the CSS computed padding values.
    LayoutUnit computedCSSPaddingTop() const { return computedCSSPadding(style().paddingTop()); }
    LayoutUnit computedCSSPaddingBottom() const { return computedCSSPadding(style().paddingBottom()); }
    LayoutUnit computedCSSPaddingLeft() const { return computedCSSPadding(style().paddingLeft()); }
    LayoutUnit computedCSSPaddingRight() const { return computedCSSPadding(style().paddingRight()); }
    LayoutUnit computedCSSPaddingBefore() const { return computedCSSPadding(style().paddingBefore()); }
    LayoutUnit computedCSSPaddingAfter() const { return computedCSSPadding(style().paddingAfter()); }
    LayoutUnit computedCSSPaddingStart() const { return computedCSSPadding(style().paddingStart()); }
    LayoutUnit computedCSSPaddingEnd() const { return computedCSSPadding(style().paddingEnd()); }

    // These functions are used during layout. Table cells and the MathML
    // code override them to include some extra intrinsic padding.
    virtual LayoutUnit paddingTop() const { return computedCSSPaddingTop(); }
    virtual LayoutUnit paddingBottom() const { return computedCSSPaddingBottom(); }
    virtual LayoutUnit paddingLeft() const { return computedCSSPaddingLeft(); }
    virtual LayoutUnit paddingRight() const { return computedCSSPaddingRight(); }
    virtual LayoutUnit paddingBefore() const { return computedCSSPaddingBefore(); }
    virtual LayoutUnit paddingAfter() const { return computedCSSPaddingAfter(); }
    virtual LayoutUnit paddingStart() const { return computedCSSPaddingStart(); }
    virtual LayoutUnit paddingEnd() const { return computedCSSPaddingEnd(); }

    virtual int borderTop() const { return style().borderTopWidth(); }
    virtual int borderBottom() const { return style().borderBottomWidth(); }
    virtual int borderLeft() const { return style().borderLeftWidth(); }
    virtual int borderRight() const { return style().borderRightWidth(); }
    virtual int borderBefore() const { return style().borderBeforeWidth(); }
    virtual int borderAfter() const { return style().borderAfterWidth(); }
    virtual int borderStart() const { return style().borderStartWidth(); }
    virtual int borderEnd() const { return style().borderEndWidth(); }

    LayoutUnit borderAndPaddingStart() const { return borderStart() + paddingStart(); }
    LayoutUnit borderAndPaddingBefore() const { return borderBefore() + paddingBefore(); }
    LayoutUnit borderAndPaddingAfter() const { return borderAfter() + paddingAfter(); }

    LayoutUnit borderAndPaddingHeight() const { return borderTop() + borderBottom() + paddingTop() + paddingBottom(); }
    LayoutUnit borderAndPaddingWidth() const { return borderLeft() + borderRight() + paddingLeft() + paddingRight(); }
    LayoutUnit borderAndPaddingLogicalHeight() const { return borderAndPaddingBefore() + borderAndPaddingAfter(); }
    LayoutUnit borderAndPaddingLogicalWidth() const { return borderStart() + borderEnd() + paddingStart() + paddingEnd(); }
    LayoutUnit borderAndPaddingLogicalLeft() const { return style().isHorizontalWritingMode() ? borderLeft() + paddingLeft() : borderTop() + paddingTop(); }

    LayoutUnit borderLogicalLeft() const { return style().isHorizontalWritingMode() ? borderLeft() : borderTop(); }
    LayoutUnit borderLogicalRight() const { return style().isHorizontalWritingMode() ? borderRight() : borderBottom(); }

    LayoutUnit paddingLogicalLeft() const { return style().isHorizontalWritingMode() ? paddingLeft() : paddingTop(); }
    LayoutUnit paddingLogicalRight() const { return style().isHorizontalWritingMode() ? paddingRight() : paddingBottom(); }
    
    virtual LayoutUnit marginTop() const = 0;
    virtual LayoutUnit marginBottom() const = 0;
    virtual LayoutUnit marginLeft() const = 0;
    virtual LayoutUnit marginRight() const = 0;
    virtual LayoutUnit marginBefore(const RenderStyle* otherStyle = 0) const = 0;
    virtual LayoutUnit marginAfter(const RenderStyle* otherStyle = 0) const = 0;
    virtual LayoutUnit marginStart(const RenderStyle* otherStyle = 0) const = 0;
    virtual LayoutUnit marginEnd(const RenderStyle* otherStyle = 0) const = 0;
    LayoutUnit marginHeight() const { return marginTop() + marginBottom(); }
    LayoutUnit marginWidth() const { return marginLeft() + marginRight(); }
    LayoutUnit marginLogicalHeight() const { return marginBefore() + marginAfter(); }
    LayoutUnit marginLogicalWidth() const { return marginStart() + marginEnd(); }

    bool hasInlineDirectionBordersPaddingOrMargin() const { return hasInlineDirectionBordersOrPadding() || marginStart()|| marginEnd(); }
    bool hasInlineDirectionBordersOrPadding() const { return borderStart() || borderEnd() || paddingStart()|| paddingEnd(); }

    virtual LayoutUnit containingBlockLogicalWidthForContent() const;

    virtual void childBecameNonInline(RenderObject* /*child*/) { }

    void paintBorder(const PaintInfo&, const LayoutRect&, const RenderStyle*, BackgroundBleedAvoidance = BackgroundBleedNone, bool includeLogicalLeftEdge = true, bool includeLogicalRightEdge = true);
    bool paintNinePieceImage(GraphicsContext*, const LayoutRect&, const RenderStyle*, const NinePieceImage&, CompositeOperator = CompositeSourceOver);
    void paintBoxShadow(const PaintInfo&, const LayoutRect&, const RenderStyle*, ShadowStyle, bool includeLogicalLeftEdge = true, bool includeLogicalRightEdge = true);
    void paintFillLayerExtended(const PaintInfo&, const Color&, const FillLayer*, const LayoutRect&, BackgroundBleedAvoidance, InlineFlowBox* = 0, const LayoutSize& = LayoutSize(), CompositeOperator = CompositeSourceOver, RenderElement* backgroundObject = 0);

    virtual bool boxShadowShouldBeAppliedToBackground(BackgroundBleedAvoidance, InlineFlowBox* = 0) const;

    // Overridden by subclasses to determine line height and baseline position.
    virtual LayoutUnit lineHeight(bool firstLine, LineDirectionMode, LinePositionMode = PositionOnContainingLine) const = 0;
    virtual int baselinePosition(FontBaseline, bool firstLine, LineDirectionMode, LinePositionMode = PositionOnContainingLine) const = 0;

    virtual void mapAbsoluteToLocalPoint(MapCoordinatesFlags, TransformState&) const OVERRIDE;

    void highQualityRepaintTimerFired(Timer<RenderBoxModelObject>*);

    virtual void setSelectionState(SelectionState) OVERRIDE;

    bool canHaveBoxInfoInRegion() const { return !isFloating() && !isReplaced() && !isInline() && !hasColumns() && !isTableCell() && isRenderBlock() && !isRenderSVGBlock(); }


    void getGeometryForBackgroundImage(const RenderLayerModelObject* paintContainer, IntRect& destRect, IntPoint& phase, IntSize& tileSize) const;
#if USE(ACCELERATED_COMPOSITING)
    void contentChanged(ContentChangeType);
    bool hasAcceleratedCompositing() const;

    bool startTransition(double, CSSPropertyID, const RenderStyle* fromStyle, const RenderStyle* toStyle);
    void transitionPaused(double timeOffset, CSSPropertyID);
    void transitionFinished(CSSPropertyID);

    bool startAnimation(double timeOffset, const Animation*, const KeyframeList& keyframes);
    void animationPaused(double timeOffset, const String& name);
    void animationFinished(const String& name);

    void suspendAnimations(double time = 0);
#endif

protected:
    RenderBoxModelObject(Element&, PassRef<RenderStyle>, unsigned baseTypeFlags);
    RenderBoxModelObject(Document&, PassRef<RenderStyle>, unsigned baseTypeFlags);

    virtual void willBeDestroyed() OVERRIDE;

    class BackgroundImageGeometry {
    public:
        BackgroundImageGeometry()
            : m_hasNonLocalGeometry(false)
        { }
        IntPoint destOrigin() const { return m_destOrigin; }
        void setDestOrigin(const IntPoint& destOrigin)
        {
            m_destOrigin = destOrigin;
        }
        
        IntRect destRect() const { return m_destRect; }
        void setDestRect(const IntRect& destRect)
        {
            m_destRect = destRect;
        }

        // Returns the phase relative to the destination rectangle.
        IntPoint relativePhase() const;
        
        IntPoint phase() const { return m_phase; }   
        void setPhase(const IntPoint& phase)
        {
            m_phase = phase;
        }

        IntSize tileSize() const { return m_tileSize; }    
        void setTileSize(const IntSize& tileSize)
        {
            m_tileSize = tileSize;
        }
        FloatSize spaceSize() const { return m_space; }
        void setSpaceSize(const FloatSize& space)
        {
            m_space = space;
        }

        void setPhaseX(int x) { m_phase.setX(x); }
        void setPhaseY(int y) { m_phase.setY(y); }
        
        void setNoRepeatX(int xOffset);
        void setNoRepeatY(int yOffset);
        
        void useFixedAttachment(const IntPoint& attachmentPoint);
        
        void clip(const IntRect&);
        
        void setHasNonLocalGeometry(bool hasNonLocalGeometry = true) { m_hasNonLocalGeometry = hasNonLocalGeometry; }
        bool hasNonLocalGeometry() const { return m_hasNonLocalGeometry; }

    private:
        IntRect m_destRect;
        IntPoint m_destOrigin;
        IntPoint m_phase;
        IntSize m_tileSize;
        FloatSize m_space;
        bool m_hasNonLocalGeometry; // Has background-attachment: fixed. Implies that we can't always cheaply compute destRect.
    };

    LayoutPoint adjustedPositionRelativeToOffsetParent(const LayoutPoint&) const;

    void calculateBackgroundImageGeometry(const RenderLayerModelObject* paintContainer, const FillLayer*, const LayoutRect& paintRect, BackgroundImageGeometry&, RenderElement* = 0) const;
    void getBorderEdgeInfo(class BorderEdge[], const RenderStyle*, bool includeLogicalLeftEdge = true, bool includeLogicalRightEdge = true) const;
    bool borderObscuresBackgroundEdge(const FloatSize& contextScale) const;
    bool borderObscuresBackground() const;
    RoundedRect backgroundRoundedRectAdjustedForBleedAvoidance(GraphicsContext*, const LayoutRect&, BackgroundBleedAvoidance, InlineFlowBox*, const LayoutSize&, bool includeLogicalLeftEdge, bool includeLogicalRightEdge) const;
    LayoutRect borderInnerRectAdjustedForBleedAvoidance(GraphicsContext*, const LayoutRect&, BackgroundBleedAvoidance) const;

    bool shouldPaintAtLowQuality(GraphicsContext*, Image*, const void*, const LayoutSize&);

    RenderBoxModelObject* continuation() const;
    void setContinuation(RenderBoxModelObject*);

    LayoutRect localCaretRectForEmptyElement(LayoutUnit width, LayoutUnit textIndentOffset);

    static bool shouldAntialiasLines(GraphicsContext*);

    static void clipRoundedInnerRect(GraphicsContext*, const LayoutRect&, const RoundedRect& clipRect);

    bool hasAutoHeightOrContainingBlockWithAutoHeight() const;

public:
    // For RenderBlocks and RenderInlines with m_style->styleType() == FIRST_LETTER, this tracks their remaining text fragments
    RenderTextFragment* firstLetterRemainingText() const;
    void setFirstLetterRemainingText(RenderTextFragment*);

    // These functions are only used internally to manipulate the render tree structure via remove/insert/appendChildNode.
    // Since they are typically called only to move objects around within anonymous blocks (which only have layers in
    // the case of column spans), the default for fullRemoveInsert is false rather than true.
    void moveChildTo(RenderBoxModelObject* toBoxModelObject, RenderObject* child, RenderObject* beforeChild, bool fullRemoveInsert = false);
    void moveChildTo(RenderBoxModelObject* toBoxModelObject, RenderObject* child, bool fullRemoveInsert = false)
    {
        moveChildTo(toBoxModelObject, child, 0, fullRemoveInsert);
    }
    void moveAllChildrenTo(RenderBoxModelObject* toBoxModelObject, bool fullRemoveInsert = false)
    {
        moveAllChildrenTo(toBoxModelObject, 0, fullRemoveInsert);
    }
    void moveAllChildrenTo(RenderBoxModelObject* toBoxModelObject, RenderObject* beforeChild, bool fullRemoveInsert = false)
    {
        moveChildrenTo(toBoxModelObject, firstChild(), 0, beforeChild, fullRemoveInsert);
    }
    // Move all of the kids from |startChild| up to but excluding |endChild|. 0 can be passed as the |endChild| to denote
    // that all the kids from |startChild| onwards should be moved.
    void moveChildrenTo(RenderBoxModelObject* toBoxModelObject, RenderObject* startChild, RenderObject* endChild, bool fullRemoveInsert = false)
    {
        moveChildrenTo(toBoxModelObject, startChild, endChild, 0, fullRemoveInsert);
    }
    void moveChildrenTo(RenderBoxModelObject* toBoxModelObject, RenderObject* startChild, RenderObject* endChild, RenderObject* beforeChild, bool fullRemoveInsert = false);

private:
    LayoutUnit computedCSSPadding(Length) const;
    
    virtual LayoutRect frameRectForStickyPositioning() const = 0;

    IntSize calculateFillTileSize(const FillLayer*, const IntSize& scaledPositioningAreaSize) const;

    enum ScaleByEffectiveZoomOrNot { ScaleByEffectiveZoom, DoNotScaleByEffectiveZoom };
    IntSize calculateImageIntrinsicDimensions(StyleImage*, const IntSize& scaledPositioningAreaSize, ScaleByEffectiveZoomOrNot) const;

    RoundedRect getBackgroundRoundedRect(const LayoutRect&, InlineFlowBox*, LayoutUnit inlineBoxWidth, LayoutUnit inlineBoxHeight,
        bool includeLogicalLeftEdge, bool includeLogicalRightEdge) const;
    
    bool fixedBackgroundPaintsInLocalCoordinates() const;

    void clipBorderSidePolygon(GraphicsContext*, const RoundedRect& outerBorder, const RoundedRect& innerBorder,
                               BoxSide, bool firstEdgeMatches, bool secondEdgeMatches);
    void clipBorderSideForComplexInnerPath(GraphicsContext*, const RoundedRect&, const RoundedRect&, BoxSide, const class BorderEdge[]);
    void paintOneBorderSide(GraphicsContext*, const RenderStyle*, const RoundedRect& outerBorder, const RoundedRect& innerBorder,
                                const IntRect& sideRect, BoxSide, BoxSide adjacentSide1, BoxSide adjacentSide2, const class BorderEdge[],
                                const Path*, BackgroundBleedAvoidance, bool includeLogicalLeftEdge, bool includeLogicalRightEdge, bool antialias, const Color* overrideColor = 0);
    void paintTranslucentBorderSides(GraphicsContext*, const RenderStyle*, const RoundedRect& outerBorder, const RoundedRect& innerBorder, const IntPoint& innerBorderAdjustment,
        const class BorderEdge[], BorderEdgeFlags, BackgroundBleedAvoidance, bool includeLogicalLeftEdge, bool includeLogicalRightEdge, bool antialias = false);
    void paintBorderSides(GraphicsContext*, const RenderStyle*, const RoundedRect& outerBorder, const RoundedRect& innerBorder,
        const IntPoint& innerBorderAdjustment, const class BorderEdge[], BorderEdgeFlags, BackgroundBleedAvoidance,
        bool includeLogicalLeftEdge, bool includeLogicalRightEdge, bool antialias = false, const Color* overrideColor = 0);
    void drawBoxSideFromPath(GraphicsContext*, const LayoutRect&, const Path&, const class BorderEdge[],
                            float thickness, float drawThickness, BoxSide, const RenderStyle*, 
                            Color, EBorderStyle, BackgroundBleedAvoidance, bool includeLogicalLeftEdge, bool includeLogicalRightEdge);
};

RENDER_OBJECT_TYPE_CASTS(RenderBoxModelObject, isBoxModelObject())

} // namespace WebCore

#endif // RenderBoxModelObject_h
