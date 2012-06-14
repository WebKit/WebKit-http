/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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
#include "RenderSVGResourcePattern.h"

#include "FrameView.h"
#include "GraphicsContext.h"
#include "PatternAttributes.h"
#include "RenderSVGRoot.h"
#include "SVGFitToViewBox.h"
#include "SVGRenderSupport.h"
#include "SVGRenderingContext.h"

namespace WebCore {

RenderSVGResourceType RenderSVGResourcePattern::s_resourceType = PatternResourceType;

RenderSVGResourcePattern::RenderSVGResourcePattern(SVGPatternElement* node)
    : RenderSVGResourceContainer(node)
    , m_shouldCollectPatternAttributes(true)
{
}

void RenderSVGResourcePattern::removeAllClientsFromCache(bool markForInvalidation)
{
    m_patternMap.clear();
    m_shouldCollectPatternAttributes = true;
    markAllClientsForInvalidation(markForInvalidation ? RepaintInvalidation : ParentOnlyInvalidation);
}

void RenderSVGResourcePattern::removeClientFromCache(RenderObject* client, bool markForInvalidation)
{
    ASSERT(client);
    m_patternMap.remove(client);
    markClientForInvalidation(client, markForInvalidation ? RepaintInvalidation : ParentOnlyInvalidation);
}

bool RenderSVGResourcePattern::applyResource(RenderObject* object, RenderStyle* style, GraphicsContext*& context, unsigned short resourceMode)
{
    ASSERT(object);
    ASSERT(style);
    ASSERT(context);
    ASSERT(resourceMode != ApplyToDefaultMode);

    // Be sure to synchronize all SVG properties on the patternElement _before_ processing any further.
    // Otherwhise the call to collectPatternAttributes() below, may cause the SVG DOM property
    // synchronization to kick in, which causes removeAllClientsFromCache() to be called, which in turn deletes our
    // PatternData object! Leaving out the line below will cause svg/dynamic-updates/SVGPatternElement-svgdom* to crash.
    SVGPatternElement* patternElement = static_cast<SVGPatternElement*>(node());
    if (!patternElement)
        return false;

    if (m_shouldCollectPatternAttributes) {
        patternElement->updateAnimatedSVGAttribute(anyQName());

        m_attributes = PatternAttributes();
        patternElement->collectPatternAttributes(m_attributes);
        m_shouldCollectPatternAttributes = false;
    }

    // Spec: When the geometry of the applicable element has no width or height and objectBoundingBox is specified,
    // then the given effect (e.g. a gradient or a filter) will be ignored.
    FloatRect objectBoundingBox = object->objectBoundingBox();
    if (m_attributes.patternUnits() == SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX && objectBoundingBox.isEmpty())
        return false;

    OwnPtr<PatternData>& patternData = m_patternMap.add(object, nullptr).iterator->second;
    if (!patternData)
        patternData = adoptPtr(new PatternData);

    if (!patternData->pattern) {
        // If we couldn't determine the pattern content element root, stop here.
        if (!m_attributes.patternContentElement())
            return false;

        // Compute all necessary transformations to build the tile image & the pattern.
        FloatRect tileBoundaries;
        AffineTransform tileImageTransform;
        if (!buildTileImageTransform(object, m_attributes, patternElement, tileBoundaries, tileImageTransform))
            return false;

        AffineTransform absoluteTransformIgnoringRotation;
        SVGRenderingContext::calculateTransformationToOutermostSVGCoordinateSystem(object, absoluteTransformIgnoringRotation);

        // Ignore 2D rotation, as it doesn't affect the size of the tile.
        SVGRenderingContext::clear2DRotation(absoluteTransformIgnoringRotation);
        FloatRect absoluteTileBoundaries = absoluteTransformIgnoringRotation.mapRect(tileBoundaries);
        FloatRect clampedAbsoluteTileBoundaries;

        // Scale the tile size to match the scale level of the patternTransform.
        absoluteTileBoundaries.scale(static_cast<float>(m_attributes.patternTransform().xScale()),
            static_cast<float>(m_attributes.patternTransform().yScale()));

        // Build tile image.
        OwnPtr<ImageBuffer> tileImage = createTileImage(m_attributes, tileBoundaries, absoluteTileBoundaries, tileImageTransform, clampedAbsoluteTileBoundaries);
        if (!tileImage)
            return false;

        RefPtr<Image> copiedImage = tileImage->copyImage(CopyBackingStore);
        if (!copiedImage)
            return false;

        // Build pattern.
        patternData->pattern = Pattern::create(copiedImage, true, true);
        if (!patternData->pattern)
            return false;

        // Compute pattern space transformation.
        patternData->transform.translate(tileBoundaries.x(), tileBoundaries.y());
        patternData->transform.scale(tileBoundaries.width() / clampedAbsoluteTileBoundaries.width(), tileBoundaries.height() / clampedAbsoluteTileBoundaries.height());

        AffineTransform patternTransform = m_attributes.patternTransform();
        if (!patternTransform.isIdentity())
            patternData->transform = patternTransform * patternData->transform;

        // Account for text drawing resetting the context to non-scaled, see SVGInlineTextBox::paintTextWithShadows.
        if (resourceMode & ApplyToTextMode) {
            AffineTransform additionalTextTransformation;
            if (shouldTransformOnTextPainting(object, additionalTextTransformation))
                patternData->transform *= additionalTextTransformation;
        }
        patternData->pattern->setPatternSpaceTransform(patternData->transform);
    }

    // Draw pattern
    context->save();

    const SVGRenderStyle* svgStyle = style->svgStyle();
    ASSERT(svgStyle);

    if (resourceMode & ApplyToFillMode) {
        context->setAlpha(svgStyle->fillOpacity());
        context->setFillPattern(patternData->pattern);
        context->setFillRule(svgStyle->fillRule());
    } else if (resourceMode & ApplyToStrokeMode) {
        if (svgStyle->vectorEffect() == VE_NON_SCALING_STROKE)
            patternData->pattern->setPatternSpaceTransform(transformOnNonScalingStroke(object, patternData->transform));
        context->setAlpha(svgStyle->strokeOpacity());
        context->setStrokePattern(patternData->pattern);
        SVGRenderSupport::applyStrokeStyleToContext(context, style, object);
    }

    if (resourceMode & ApplyToTextMode) {
        if (resourceMode & ApplyToFillMode) {
            context->setTextDrawingMode(TextModeFill);

#if USE(CG)
            context->applyFillPattern();
#endif
        } else if (resourceMode & ApplyToStrokeMode) {
            context->setTextDrawingMode(TextModeStroke);

#if USE(CG)
            context->applyStrokePattern();
#endif
        }
    }

    return true;
}

void RenderSVGResourcePattern::postApplyResource(RenderObject*, GraphicsContext*& context, unsigned short resourceMode, const Path* path, const RenderSVGShape* shape)
{
    ASSERT(context);
    ASSERT(resourceMode != ApplyToDefaultMode);

    if (resourceMode & ApplyToFillMode) {
        if (path)
            context->fillPath(*path);
        else if (shape)
            shape->fillShape(context);
    }
    if (resourceMode & ApplyToStrokeMode) {
        if (path)
            context->strokePath(*path);
        else if (shape)
            shape->strokeShape(context);
    }

    context->restore();
}

static inline FloatRect calculatePatternBoundaries(const PatternAttributes& attributes,
                                                   const FloatRect& objectBoundingBox,
                                                   const SVGPatternElement* patternElement)
{
    ASSERT(patternElement);
    return SVGLengthContext::resolveRectangle(patternElement, attributes.patternUnits(), objectBoundingBox, attributes.x(), attributes.y(), attributes.width(), attributes.height());
}

bool RenderSVGResourcePattern::buildTileImageTransform(RenderObject* renderer,
                                                       const PatternAttributes& attributes,
                                                       const SVGPatternElement* patternElement,
                                                       FloatRect& patternBoundaries,
                                                       AffineTransform& tileImageTransform) const
{
    ASSERT(renderer);
    ASSERT(patternElement);

    FloatRect objectBoundingBox = renderer->objectBoundingBox();
    patternBoundaries = calculatePatternBoundaries(attributes, objectBoundingBox, patternElement); 
    if (patternBoundaries.width() <= 0 || patternBoundaries.height() <= 0)
        return false;

    AffineTransform viewBoxCTM = SVGFitToViewBox::viewBoxToViewTransform(attributes.viewBox(), attributes.preserveAspectRatio(), patternBoundaries.width(), patternBoundaries.height());

    // Apply viewBox/objectBoundingBox transformations.
    if (!viewBoxCTM.isIdentity())
        tileImageTransform = viewBoxCTM;
    else if (attributes.patternContentUnits() == SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX)
        tileImageTransform.scale(objectBoundingBox.width(), objectBoundingBox.height());

    return true;
}

PassOwnPtr<ImageBuffer> RenderSVGResourcePattern::createTileImage(const PatternAttributes& attributes,
                                                                  const FloatRect& tileBoundaries,
                                                                  const FloatRect& absoluteTileBoundaries,
                                                                  const AffineTransform& tileImageTransform,
                                                                  FloatRect& clampedAbsoluteTileBoundaries) const
{
    clampedAbsoluteTileBoundaries = SVGRenderingContext::clampedAbsoluteTargetRect(absoluteTileBoundaries);

    OwnPtr<ImageBuffer> tileImage;

    if (!SVGRenderingContext::createImageBufferForPattern(absoluteTileBoundaries, clampedAbsoluteTileBoundaries, tileImage, ColorSpaceDeviceRGB, Unaccelerated))
        return nullptr;

    GraphicsContext* tileImageContext = tileImage->context();
    ASSERT(tileImageContext);

    // The image buffer represents the final rendered size, so the content has to be scaled (to avoid pixelation).
    tileImageContext->scale(FloatSize(clampedAbsoluteTileBoundaries.width() / tileBoundaries.width(),
                                      clampedAbsoluteTileBoundaries.height() / tileBoundaries.height()));

    // Apply tile image transformations.
    if (!tileImageTransform.isIdentity())
        tileImageContext->concatCTM(tileImageTransform);

    AffineTransform contentTransformation;
    if (attributes.patternContentUnits() == SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX)
        contentTransformation = tileImageTransform;

    // Draw the content into the ImageBuffer.
    for (Node* node = attributes.patternContentElement()->firstChild(); node; node = node->nextSibling()) {
        if (!node->isSVGElement() || !static_cast<SVGElement*>(node)->isStyled() || !node->renderer())
            continue;
        if (node->renderer()->needsLayout())
            return nullptr;
        SVGRenderingContext::renderSubtreeToImageBuffer(tileImage.get(), node->renderer(), contentTransformation);
    }

    return tileImage.release();
}

}

#endif
