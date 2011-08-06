/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2007, 2008, 2009 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Google, Inc.
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
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
 */

#include "config.h"

#if ENABLE(SVG)
#include "RenderSVGRoot.h"

#include "Frame.h"
#include "GraphicsContext.h"
#include "HitTestResult.h"
#include "RenderPart.h"
#include "RenderSVGContainer.h"
#include "RenderSVGResource.h"
#include "RenderView.h"
#include "SVGLength.h"
#include "SVGRenderSupport.h"
#include "SVGResources.h"
#include "SVGSVGElement.h"
#include "SVGStyledElement.h"
#include "SVGViewSpec.h"
#include "TransformState.h"

#if ENABLE(FILTERS)
#include "RenderSVGResourceFilter.h"
#endif

using namespace std;

namespace WebCore {

RenderSVGRoot::RenderSVGRoot(SVGStyledElement* node)
    : RenderBox(node)
    , m_isLayoutSizeChanged(false)
    , m_needsBoundariesOrTransformUpdate(true)
    , m_needsSizeNegotiationWithHostDocument(false)
{
    setReplaced(true);
}

RenderSVGRoot::~RenderSVGRoot()
{
}

void RenderSVGRoot::computeIntrinsicRatioInformation(FloatSize& intrinsicRatio, bool& isPercentageIntrinsicSize) const
{
    // Spec: http://dev.w3.org/SVG/profiles/1.1F2/publish/coords.html#IntrinsicSizing
    // The intrinsic aspect ratio of the viewport of SVG content is necessary for example, when including
    // SVG from an ‘object’ element in HTML styled with CSS. It is possible (indeed, common) for an SVG
    // graphic to have an intrinsic aspect ratio but not to have an intrinsic width or height.
    // The intrinsic aspect ratio must be calculated based upon the following rules:
    // The aspect ratio is calculated by dividing a width by a height.

    // If the ‘width’ and ‘height’ of the rootmost ‘svg’ element are both specified with unit identifiers
    // (in, mm, cm, pt, pc, px, em, ex) or in user units, then the aspect ratio is calculated from the
    // ‘width’ and ‘height’ attributes after resolving both values to user units.
    isPercentageIntrinsicSize = false;
    if (style()->width().isFixed() && style()->height().isFixed()) {
        intrinsicRatio = FloatSize(width(), height());
        return;
    }

    // If either/both of the ‘width’ and ‘height’ of the rootmost ‘svg’ element are in percentage units (or omitted),
    // the aspect ratio is calculated from the width and height values of the ‘viewBox’ specified for the current SVG
    // document fragment. If the ‘viewBox’ is not correctly specified, or set to 'none', the intrinsic aspect ratio
    // cannot be calculated and is considered unspecified.
    intrinsicRatio = static_cast<SVGSVGElement*>(node())->currentViewBoxRect().size();

    // Compatibility with authors expectations and Firefox/Opera: when percentage units are used, take them into
    // account for certain cases of the intrinsic width/height calculation in RenderPart::computeReplacedLogicalWidth/Height.
    if (intrinsicRatio.isEmpty() && style()->width().isPercent() && style()->height().isPercent()) {
        isPercentageIntrinsicSize = true;
        intrinsicRatio = FloatSize(style()->width().percent(), style()->height().percent());
    }
}

void RenderSVGRoot::computePreferredLogicalWidths()
{
    ASSERT(preferredLogicalWidthsDirty());

    LayoutUnit borderAndPadding = borderAndPaddingWidth();
    LayoutUnit width = computeReplacedLogicalWidth(false) + borderAndPadding;

    if (style()->maxWidth().isFixed() && style()->maxWidth().value() != undefinedLength)
        width = min(width, style()->maxWidth().value() + (style()->boxSizing() == CONTENT_BOX ? borderAndPadding : 0));

    if (style()->width().isPercent() || (style()->width().isAuto() && style()->height().isPercent())) {
        m_minPreferredLogicalWidth = 0;
        m_maxPreferredLogicalWidth = width;
    } else
        m_minPreferredLogicalWidth = m_maxPreferredLogicalWidth = width;

    setPreferredLogicalWidthsDirty(false);
}

LayoutUnit RenderSVGRoot::computeIntrinsicWidth(LayoutUnit replacedWidth) const
{
    if (!style()->width().isPercent())
        return replacedWidth;
    // FIXME: Remove unnecessary rounding when layout is off ints: webkit.org/b/63656
    return static_cast<int>(ceilf(replacedWidth * style()->effectiveZoom()));
}

LayoutUnit RenderSVGRoot::computeIntrinsicHeight(LayoutUnit replacedHeight) const
{
    if (!style()->height().isPercent())
        return replacedHeight;
    // FIXME: Remove unnecessary rounding when layout is off ints: webkit.org/b/63656
    return static_cast<int>(ceilf(replacedHeight * style()->effectiveZoom()));
}

LayoutUnit RenderSVGRoot::computeReplacedLogicalWidth(bool includeMaxWidth) const
{
    LayoutUnit replacedWidth = RenderBox::computeReplacedLogicalWidth(includeMaxWidth);
    Frame* frame = node() && node()->document() ? node()->document()->frame() : 0;
    if (!frame)
        return computeIntrinsicWidth(replacedWidth);

    // If our frame has an owner renderer, we're embedded through eg. object/embed.
    RenderPart* ownerRenderer = frame->ownerRenderer();
    if (!ownerRenderer)
        return computeIntrinsicWidth(replacedWidth);

    RenderStyle* ownerRendererStyle = ownerRenderer->style();
    ASSERT(ownerRendererStyle);
    ASSERT(frame->contentRenderer());

    Length ownerWidth = ownerRendererStyle->width();
    if (ownerWidth.isAuto())
        return replacedWidth;

    // Spec: http://dev.w3.org/SVG/profiles/1.1F2/publish/coords.html#ViewportSpace
    // The SVG user agent negotiates with its parent user agent to determine the viewport into which the SVG user agent can render
    // the document. In some circumstances, SVG content will be embedded (by reference or inline) within a containing document.
    // This containing document might include attributes, properties and/or other parameters (explicit or implicit) which specify
    // or provide hints about the dimensions of the viewport for the SVG content. SVG content itself optionally can provide
    // information about the appropriate viewport region for the content via the ‘width’ and ‘height’ XML attributes on the
    // outermost svg element. The negotiation process uses any information provided by the containing document and the SVG
    // content itself to choose the viewport location and size.

    // The ‘width’ attribute on the outermost svg element establishes the viewport's width, unless the following conditions are met:
    // * the SVG content is a separately stored resource that is embedded by reference (such as the ‘object’ element in XHTML [XHTML]),
    //   or the SVG content is embedded inline within a containing document;
    // * and the referencing element or containing document is styled using CSS [CSS2] or XSL [XSL];
    // * and there are CSS-compatible positioning properties ([CSS2], section 9.3) specified on the referencing element
    //   (e.g., the ‘object’ element) or on the containing document's outermost svg element that are sufficient to establish
    //   the width of the viewport.
    //
    // Under these conditions, the positioning properties establish the viewport's width.
    return ownerRenderer->computeReplacedLogicalWidthRespectingMinMaxWidth(ownerRenderer->computeReplacedLogicalWidthUsing(ownerWidth), includeMaxWidth);
}

LayoutUnit RenderSVGRoot::computeReplacedLogicalHeight() const
{
    LayoutUnit replacedHeight = RenderBox::computeReplacedLogicalHeight();

    Frame* frame = node() && node()->document() ? node()->document()->frame() : 0;
    if (!frame)
        return computeIntrinsicHeight(replacedHeight);

    // If our frame has an owner renderer, we're embedded through eg. object/embed.
    RenderPart* ownerRenderer = frame->ownerRenderer();
    if (!ownerRenderer)
        return computeIntrinsicHeight(replacedHeight);

    RenderStyle* ownerRendererStyle = ownerRenderer->style();
    ASSERT(ownerRendererStyle);
    ASSERT(frame->contentRenderer());

    Length ownerHeight = ownerRendererStyle->height();
    if (ownerHeight.isAuto())
        return replacedHeight;

    // Spec: http://dev.w3.org/SVG/profiles/1.1F2/publish/coords.html#ViewportSpace
    // See comment in RenderSVGRoot::computeReplacedLogicalWidth().
    // Similarly, if there are positioning properties specified on the referencing element or on the outermost svg element that
    // are sufficient to establish the height of the viewport, then these positioning properties establish the viewport's height;
    // otherwise, the ‘height’ attribute on the outermost svg element establishes the viewport's height.
    return ownerRenderer->computeReplacedLogicalHeightRespectingMinMaxHeight(ownerRenderer->computeReplacedLogicalHeightUsing(ownerHeight));
}

void RenderSVGRoot::layout()
{
    ASSERT(needsLayout());

    // Arbitrary affine transforms are incompatible with LayoutState.
    LayoutStateDisabler layoutStateDisabler(view());

    bool needsLayout = selfNeedsLayout();
    LayoutRepainter repainter(*this, checkForRepaintDuringLayout() && needsLayout);

    LayoutSize oldSize(width(), height());
    computeLogicalWidth();
    computeLogicalHeight();
    calcViewport();

    SVGSVGElement* svg = static_cast<SVGSVGElement*>(node());
    m_isLayoutSizeChanged = svg->hasRelativeLengths() && oldSize != size();
 
    if (view() && view()->frameView() && view()->frameView()->embeddedContentBox()) {
        if (!m_needsSizeNegotiationWithHostDocument)
            m_needsSizeNegotiationWithHostDocument = !m_everHadLayout || oldSize != size();
    } else
        ASSERT(!m_needsSizeNegotiationWithHostDocument);

    SVGRenderSupport::layoutChildren(this, needsLayout);
    m_isLayoutSizeChanged = false;

    // At this point LayoutRepainter already grabbed the old bounds,
    // recalculate them now so repaintAfterLayout() uses the new bounds.
    if (m_needsBoundariesOrTransformUpdate) {
        updateCachedBoundaries();
        m_needsBoundariesOrTransformUpdate = false;
    }

    repainter.repaintAfterLayout();

    setNeedsLayout(false);
}

bool RenderSVGRoot::selfWillPaint()
{
#if ENABLE(FILTERS)
    SVGResources* resources = SVGResourcesCache::cachedResourcesForRenderObject(this);
    return resources && resources->filter();
#else
    return false;
#endif
}

void RenderSVGRoot::paint(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    if (paintInfo.context->paintingDisabled())
        return;

    bool isVisible = style()->visibility() == VISIBLE;
    LayoutPoint borderBoxOriginInContainer = paintOffset + parentOriginToBorderBox();

    if (hasBoxDecorations() && (paintInfo.phase == PaintPhaseBlockBackground || paintInfo.phase == PaintPhaseChildBlockBackground) && isVisible)
        paintBoxDecorations(paintInfo, borderBoxOriginInContainer);

    if (paintInfo.phase == PaintPhaseBlockBackground)
        return;

    // An empty viewport disables rendering.  FIXME: Should we still render filters?
    if (m_viewportSize.isEmpty())
        return;

    // Don't paint if we don't have kids, except if we have filters we should paint those.
    if (!firstChild() && !selfWillPaint())
        return;

    // Make a copy of the PaintInfo because applyTransform will modify the damage rect.
    PaintInfo childPaintInfo(paintInfo);
    childPaintInfo.context->save();

    // Apply initial viewport clip - not affected by overflow handling
    childPaintInfo.context->clip(overflowClipRect(borderBoxOriginInContainer));

    // Convert from container offsets (html renderers) to a relative transform (svg renderers).
    // Transform from our paint container's coordinate system to our local coords.
    childPaintInfo.applyTransform(localToRepaintContainerTransform(paintOffset));

    bool continueRendering = true;
    if (childPaintInfo.phase == PaintPhaseForeground)
        continueRendering = SVGRenderSupport::prepareToRenderSVGContent(this, childPaintInfo);

    if (continueRendering)
        RenderBox::paint(childPaintInfo, LayoutPoint());

    if (childPaintInfo.phase == PaintPhaseForeground)
        SVGRenderSupport::finishRenderSVGContent(this, childPaintInfo, paintInfo.context);

    childPaintInfo.context->restore();

    if ((paintInfo.phase == PaintPhaseOutline || paintInfo.phase == PaintPhaseSelfOutline) && style()->outlineWidth() && isVisible)
        paintOutline(paintInfo.context, LayoutRect(borderBoxOriginInContainer, size()));
}

void RenderSVGRoot::willBeDestroyed()
{
    SVGResourcesCache::clientDestroyed(this);
    RenderBox::willBeDestroyed();
}

void RenderSVGRoot::styleWillChange(StyleDifference diff, const RenderStyle* newStyle)
{
    if (diff == StyleDifferenceLayout)
        setNeedsBoundariesUpdate();
    RenderBox::styleWillChange(diff, newStyle);
}

void RenderSVGRoot::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBox::styleDidChange(diff, oldStyle);
    SVGResourcesCache::clientStyleChanged(this, diff, style());
}

void RenderSVGRoot::updateFromElement()
{
    RenderBox::updateFromElement();
    SVGResourcesCache::clientUpdatedFromElement(this, style());
}

void RenderSVGRoot::calcViewport()
{
    SVGSVGElement* svg = static_cast<SVGSVGElement*>(node());

    if (!svg->hasSetContainerSize()) {
        // In the normal case of <svg> being stand-alone or in a CSSBoxModel object we use
        // RenderBox::width()/height() (which pulls data from RenderStyle)
        m_viewportSize = FloatSize(width(), height());
        return;
    }

    // In the SVGImage case grab the SVGLength values off of SVGSVGElement and use
    // the special relativeWidthValue accessors which respect the specified containerSize
    // FIXME: Check how SVGImage + zooming is supposed to be handled?
    SVGLength width = svg->width();
    SVGLength height = svg->height();
    m_viewportSize = FloatSize(width.unitType() == LengthTypePercentage ? svg->relativeWidthValue() : width.value(svg),
                               height.unitType() == LengthTypePercentage ? svg->relativeHeightValue() : height.value(svg));
}

// RenderBox methods will expect coordinates w/o any transforms in coordinates
// relative to our borderBox origin.  This method gives us exactly that.
AffineTransform RenderSVGRoot::localToBorderBoxTransform() const
{
    IntSize borderAndPadding = borderOriginToContentBox();
    SVGSVGElement* svg = static_cast<SVGSVGElement*>(node());
    float scale = style()->effectiveZoom();
    FloatPoint translate = svg->currentTranslate();
    AffineTransform ctm(scale, 0, 0, scale, borderAndPadding.width() + translate.x(), borderAndPadding.height() + translate.y());
    return ctm * svg->viewBoxToViewTransform(width() / scale, height() / scale);
}

IntSize RenderSVGRoot::parentOriginToBorderBox() const
{
    return IntSize(x(), y());
}

IntSize RenderSVGRoot::borderOriginToContentBox() const
{
    return IntSize(borderLeft() + paddingLeft(), borderTop() + paddingTop());
}

AffineTransform RenderSVGRoot::localToRepaintContainerTransform(const IntPoint& parentOriginInContainer) const
{
    return AffineTransform::translation(parentOriginInContainer.x(), parentOriginInContainer.y()) * localToParentTransform();
}

const AffineTransform& RenderSVGRoot::localToParentTransform() const
{
    IntSize parentToBorderBoxOffset = parentOriginToBorderBox();

    m_localToParentTransform = AffineTransform::translation(parentToBorderBoxOffset.width(), parentToBorderBoxOffset.height()) * localToBorderBoxTransform();

    return m_localToParentTransform;
}

IntRect RenderSVGRoot::clippedOverflowRectForRepaint(RenderBoxModelObject* repaintContainer) const
{
    return SVGRenderSupport::clippedOverflowRectForRepaint(this, repaintContainer);
}

void RenderSVGRoot::computeRectForRepaint(RenderBoxModelObject* repaintContainer, IntRect& repaintRect, bool fixed) const
{
    // Apply our local transforms (except for x/y translation), then our shadow, 
    // and then call RenderBox's method to handle all the normal CSS Box model bits
    repaintRect = localToBorderBoxTransform().mapRect(repaintRect);

    // Apply initial viewport clip - not affected by overflow settings    
    repaintRect.intersect(enclosingIntRect(FloatRect(FloatPoint(), m_viewportSize)));

    const SVGRenderStyle* svgStyle = style()->svgStyle();
    if (const ShadowData* shadow = svgStyle->shadow())
        shadow->adjustRectForShadow(repaintRect);

    RenderBox::computeRectForRepaint(repaintContainer, repaintRect, fixed);
}

void RenderSVGRoot::mapLocalToContainer(RenderBoxModelObject* repaintContainer, bool fixed, bool useTransforms, TransformState& transformState, bool* wasFixed) const
{
    ASSERT(!fixed); // We should have no fixed content in the SVG rendering tree.
    ASSERT(useTransforms); // mapping a point through SVG w/o respecting trasnforms is useless.

    // Transform to our border box and let RenderBox transform the rest of the way.
    transformState.applyTransform(localToBorderBoxTransform());
    RenderBox::mapLocalToContainer(repaintContainer, fixed, useTransforms, transformState, wasFixed);
}

void RenderSVGRoot::updateCachedBoundaries()
{
    m_objectBoundingBox = FloatRect();
    m_strokeBoundingBox = FloatRect();
    m_repaintBoundingBox = FloatRect();

    SVGRenderSupport::computeContainerBoundingBoxes(this, m_objectBoundingBox, m_strokeBoundingBox, m_repaintBoundingBox);
    SVGRenderSupport::intersectRepaintRectWithResources(this, m_repaintBoundingBox);
    m_repaintBoundingBox.inflate(borderAndPaddingWidth());
}

bool RenderSVGRoot::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, const LayoutPoint& pointInContainer, const LayoutPoint& accumulatedOffset, HitTestAction hitTestAction)
{
    LayoutPoint pointInParent = pointInContainer - toLayoutSize(accumulatedOffset);
    LayoutPoint pointInBorderBox = pointInParent - parentOriginToBorderBox();

    // Note: For now, we're ignoring hits to border and padding for <svg>
    LayoutPoint pointInContentBox = pointInBorderBox - borderOriginToContentBox();
    if (!contentBoxRect().contains(pointInContentBox))
        return false;

    LayoutPoint localPoint = localToParentTransform().inverse().mapPoint(pointInParent);

    for (RenderObject* child = lastChild(); child; child = child->previousSibling()) {
        if (child->nodeAtFloatPoint(request, result, localPoint, hitTestAction)) {
            // FIXME: CSS/HTML assumes the local point is relative to the border box, right?
            updateHitTestResult(result, pointInBorderBox);
            // FIXME: nodeAtFloatPoint() doesn't handle rect-based hit tests yet.
            result.addNodeToRectBasedTestResult(child->node(), pointInContainer);
            return true;
        }
    }

    // If we didn't early exit above, we've just hit the container <svg> element. Unlike SVG 1.1, 2nd Edition allows container elements to be hit.
    if (hitTestAction == HitTestBlockBackground && style()->pointerEvents() != PE_NONE) {
        // Only return true here, if the last hit testing phase 'BlockBackground' is executed. If we'd return true in the 'Foreground' phase,
        // hit testing would stop immediately. For SVG only trees this doesn't matter. Though when we have a <foreignObject> subtree we need
        // to be able to detect hits on the background of a <div> element. If we'd return true here in the 'Foreground' phase, we are not able 
        // to detect these hits anymore.
        updateHitTestResult(result, roundedLayoutPoint(localPoint));
        return true;
    }

    return false;
}

}

#endif // ENABLE(SVG)
