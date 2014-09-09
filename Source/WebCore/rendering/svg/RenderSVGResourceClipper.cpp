/*
 * Copyright (C) 2004, 2005, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Rob Buis <buis@kde.org>
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
 * Copyright (C) 2011 Dirk Schulze <krit@webkit.org>
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
#include "RenderSVGResourceClipper.h"

#include "ElementIterator.h"
#include "Frame.h"
#include "FrameView.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "IntRect.h"
#include "RenderObject.h"
#include "RenderStyle.h"
#include "RenderView.h"
#include "SVGNames.h"
#include "SVGRenderingContext.h"
#include "SVGResources.h"
#include "SVGResourcesCache.h"
#include "SVGUseElement.h"

namespace WebCore {

RenderSVGResourceType RenderSVGResourceClipper::s_resourceType = ClipperResourceType;

RenderSVGResourceClipper::RenderSVGResourceClipper(SVGClipPathElement& element, PassRef<RenderStyle> style)
    : RenderSVGResourceContainer(element, WTF::move(style))
{
}

RenderSVGResourceClipper::~RenderSVGResourceClipper()
{
}

void RenderSVGResourceClipper::removeAllClientsFromCache(bool markForInvalidation)
{
    m_clipBoundaries = FloatRect();
    m_clipper.clear();

    markAllClientsForInvalidation(markForInvalidation ? LayoutAndBoundariesInvalidation : ParentOnlyInvalidation);
}

void RenderSVGResourceClipper::removeClientFromCache(RenderElement& client, bool markForInvalidation)
{
    m_clipper.remove(&client);

    markClientForInvalidation(client, markForInvalidation ? BoundariesInvalidation : ParentOnlyInvalidation);
}

bool RenderSVGResourceClipper::applyResource(RenderElement& renderer, const RenderStyle&, GraphicsContext*& context, unsigned short resourceMode)
{
    ASSERT(context);
    ASSERT_UNUSED(resourceMode, resourceMode == ApplyToDefaultMode);

    return applyClippingToContext(renderer, renderer.objectBoundingBox(), renderer.repaintRectInLocalCoordinates(), context);
}

bool RenderSVGResourceClipper::pathOnlyClipping(GraphicsContext* context, const AffineTransform& animatedLocalTransform, const FloatRect& objectBoundingBox)
{
    // If the current clip-path gets clipped itself, we have to fallback to masking.
    if (!style().svgStyle().clipperResource().isEmpty())
        return false;
    WindRule clipRule = RULE_NONZERO;
    Path clipPath = Path();

    // If clip-path only contains one visible shape or path, we can use path-based clipping. Invisible
    // shapes don't affect the clipping and can be ignored. If clip-path contains more than one
    // visible shape, the additive clipping may not work, caused by the clipRule. EvenOdd
    // as well as NonZero can cause self-clipping of the elements.
    // See also http://www.w3.org/TR/SVG/painting.html#FillRuleProperty
    for (Node* childNode = clipPathElement().firstChild(); childNode; childNode = childNode->nextSibling()) {
        RenderObject* renderer = childNode->renderer();
        if (!renderer)
            continue;
        // Only shapes or paths are supported for direct clipping. We need to fallback to masking for texts.
        if (renderer->isSVGText())
            return false;
        if (!childNode->isSVGElement() || !toSVGElement(childNode)->isSVGGraphicsElement())
            continue;
        SVGGraphicsElement* styled = toSVGGraphicsElement(childNode);
        const RenderStyle& style = renderer->style();
        if (style.display() == NONE || style.visibility() != VISIBLE)
             continue;
        const SVGRenderStyle& svgStyle = style.svgStyle();
        // Current shape in clip-path gets clipped too. Fallback to masking.
        if (!svgStyle.clipperResource().isEmpty())
            return false;
        // Fallback to masking, if there is more than one clipping path.
        if (clipPath.isEmpty()) {
            styled->toClipPath(clipPath);
            clipRule = svgStyle.clipRule();
        } else
            return false;
    }
    // Only one visible shape/path was found. Directly continue clipping and transform the content to userspace if necessary.
    if (clipPathElement().clipPathUnits() == SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
        AffineTransform transform;
        transform.translate(objectBoundingBox.x(), objectBoundingBox.y());
        transform.scaleNonUniform(objectBoundingBox.width(), objectBoundingBox.height());
        clipPath.transform(transform);
    }

    // Transform path by animatedLocalTransform.
    clipPath.transform(animatedLocalTransform);

    // The SVG specification wants us to clip everything, if clip-path doesn't have a child.
    if (clipPath.isEmpty())
        clipPath.addRect(FloatRect());
    context->clipPath(clipPath, clipRule);
    return true;
}

bool RenderSVGResourceClipper::applyClippingToContext(RenderElement& renderer, const FloatRect& objectBoundingBox,
                                                      const FloatRect& repaintRect, GraphicsContext* context)
{
    bool missingClipperData = !m_clipper.contains(&renderer);
    if (missingClipperData)
        m_clipper.set(&renderer, std::make_unique<ClipperData>());

    bool shouldCreateClipData = false;
    AffineTransform animatedLocalTransform = clipPathElement().animatedLocalTransform();
    ClipperData* clipperData = m_clipper.get(&renderer);
    if (!clipperData->clipMaskImage) {
        if (pathOnlyClipping(context, animatedLocalTransform, objectBoundingBox))
            return true;
        shouldCreateClipData = true;
    }

    AffineTransform absoluteTransform;
    SVGRenderingContext::calculateTransformationToOutermostCoordinateSystem(renderer, absoluteTransform);

    if (shouldCreateClipData && !repaintRect.isEmpty()) {
        if (!SVGRenderingContext::createImageBuffer(repaintRect, absoluteTransform, clipperData->clipMaskImage, ColorSpaceDeviceRGB, Unaccelerated))
            return false;

        GraphicsContext* maskContext = clipperData->clipMaskImage->context();
        ASSERT(maskContext);

        maskContext->concatCTM(animatedLocalTransform);

        // clipPath can also be clipped by another clipPath.
        SVGResources* resources = SVGResourcesCache::cachedResourcesForRenderObject(*this);
        RenderSVGResourceClipper* clipper;
        bool succeeded;
        if (resources && (clipper = resources->clipper())) {
            GraphicsContextStateSaver stateSaver(*maskContext);

            if (!clipper->applyClippingToContext(*this, objectBoundingBox, repaintRect, maskContext))
                return false;

            succeeded = drawContentIntoMaskImage(clipperData, objectBoundingBox);
            // The context restore applies the clipping on non-CG platforms.
        } else
            succeeded = drawContentIntoMaskImage(clipperData, objectBoundingBox);

        if (!succeeded)
            clipperData->clipMaskImage.reset();
    }

    if (!clipperData->clipMaskImage)
        return false;

    SVGRenderingContext::clipToImageBuffer(context, absoluteTransform, repaintRect, clipperData->clipMaskImage, missingClipperData);
    return true;
}

bool RenderSVGResourceClipper::drawContentIntoMaskImage(ClipperData* clipperData, const FloatRect& objectBoundingBox)
{
    ASSERT(clipperData);
    ASSERT(clipperData->clipMaskImage);

    GraphicsContext* maskContext = clipperData->clipMaskImage->context();
    ASSERT(maskContext);

    AffineTransform maskContentTransformation;
    if (clipPathElement().clipPathUnits() == SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
        maskContentTransformation.translate(objectBoundingBox.x(), objectBoundingBox.y());
        maskContentTransformation.scaleNonUniform(objectBoundingBox.width(), objectBoundingBox.height());
        maskContext->concatCTM(maskContentTransformation);
    }

    // Switch to a paint behavior where all children of this <clipPath> will be rendered using special constraints:
    // - fill-opacity/stroke-opacity/opacity set to 1
    // - masker/filter not applied when rendering the children
    // - fill is set to the initial fill paint server (solid, black)
    // - stroke is set to the initial stroke paint server (none)
    PaintBehavior oldBehavior = view().frameView().paintBehavior();
    view().frameView().setPaintBehavior(oldBehavior | PaintBehaviorRenderingSVGMask);

    // Draw all clipPath children into a global mask.
    for (auto& child : childrenOfType<SVGElement>(clipPathElement())) {
        auto renderer = child.renderer();
        if (!renderer)
            continue;
        if (renderer->needsLayout()) {
            view().frameView().setPaintBehavior(oldBehavior);
            return false;
        }
        const RenderStyle& style = renderer->style();
        if (style.display() == NONE || style.visibility() != VISIBLE)
            continue;

        WindRule newClipRule = style.svgStyle().clipRule();
        bool isUseElement = child.hasTagName(SVGNames::useTag);
        if (isUseElement) {
            SVGUseElement& useElement = toSVGUseElement(child);
            renderer = useElement.rendererClipChild();
            if (!renderer)
                continue;
            if (!useElement.hasAttribute(SVGNames::clip_ruleAttr))
                newClipRule = renderer->style().svgStyle().clipRule();
        }

        // Only shapes, paths and texts are allowed for clipping.
        if (!renderer->isSVGShape() && !renderer->isSVGText())
            continue;

        maskContext->setFillRule(newClipRule);

        // In the case of a <use> element, we obtained its renderere above, to retrieve its clipRule.
        // We have to pass the <use> renderer itself to renderSubtreeToImageBuffer() to apply it's x/y/transform/etc. values when rendering.
        // So if isUseElement is true, refetch the childNode->renderer(), as renderer got overriden above.
        SVGRenderingContext::renderSubtreeToImageBuffer(clipperData->clipMaskImage.get(), isUseElement ? *child.renderer() : *renderer, maskContentTransformation);
    }

    view().frameView().setPaintBehavior(oldBehavior);
    return true;
}

void RenderSVGResourceClipper::calculateClipContentRepaintRect()
{
    // This is a rough heuristic to appraise the clip size and doesn't consider clip on clip.
    for (Node* childNode = clipPathElement().firstChild(); childNode; childNode = childNode->nextSibling()) {
        RenderObject* renderer = childNode->renderer();
        if (!childNode->isSVGElement() || !renderer)
            continue;
        if (!renderer->isSVGShape() && !renderer->isSVGText() && !childNode->hasTagName(SVGNames::useTag))
            continue;
        const RenderStyle& style = renderer->style();
        if (style.display() == NONE || style.visibility() != VISIBLE)
             continue;
        m_clipBoundaries.unite(renderer->localToParentTransform().mapRect(renderer->repaintRectInLocalCoordinates()));
    }
    m_clipBoundaries = clipPathElement().animatedLocalTransform().mapRect(m_clipBoundaries);
}

bool RenderSVGResourceClipper::hitTestClipContent(const FloatRect& objectBoundingBox, const FloatPoint& nodeAtPoint)
{
    FloatPoint point = nodeAtPoint;
    if (!SVGRenderSupport::pointInClippingArea(*this, point))
        return false;

    if (clipPathElement().clipPathUnits() == SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
        AffineTransform transform;
        transform.translate(objectBoundingBox.x(), objectBoundingBox.y());
        transform.scaleNonUniform(objectBoundingBox.width(), objectBoundingBox.height());
        point = transform.inverse().mapPoint(point);
    }

    point = clipPathElement().animatedLocalTransform().inverse().mapPoint(point);

    for (Node* childNode = clipPathElement().firstChild(); childNode; childNode = childNode->nextSibling()) {
        RenderObject* renderer = childNode->renderer();
        if (!childNode->isSVGElement() || !renderer)
            continue;
        if (!renderer->isSVGShape() && !renderer->isSVGText() && !childNode->hasTagName(SVGNames::useTag))
            continue;
        IntPoint hitPoint;
        HitTestResult result(hitPoint);
        if (renderer->nodeAtFloatPoint(HitTestRequest(HitTestRequest::SVGClipContent | HitTestRequest::DisallowShadowContent), result, point, HitTestForeground))
            return true;
    }

    return false;
}

FloatRect RenderSVGResourceClipper::resourceBoundingBox(const RenderObject& object)
{
    // Resource was not layouted yet. Give back the boundingBox of the object.
    if (selfNeedsLayout())
        return object.objectBoundingBox();
    
    if (m_clipBoundaries.isEmpty())
        calculateClipContentRepaintRect();

    if (clipPathElement().clipPathUnits() == SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
        FloatRect objectBoundingBox = object.objectBoundingBox();
        AffineTransform transform;
        transform.translate(objectBoundingBox.x(), objectBoundingBox.y());
        transform.scaleNonUniform(objectBoundingBox.width(), objectBoundingBox.height());
        return transform.mapRect(m_clipBoundaries);
    }

    return m_clipBoundaries;
}

}
