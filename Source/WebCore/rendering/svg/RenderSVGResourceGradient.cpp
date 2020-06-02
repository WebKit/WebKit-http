/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008 Dirk Schulze <krit@webkit.org>
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
#include "RenderSVGResourceGradient.h"

#include "GradientAttributes.h"
#include "GraphicsContext.h"
#include "RenderSVGText.h"
#include "SVGRenderingContext.h"
#include <wtf/IsoMallocInlines.h>

namespace WebCore {

WTF_MAKE_ISO_ALLOCATED_IMPL(RenderSVGResourceGradient);

RenderSVGResourceGradient::RenderSVGResourceGradient(SVGGradientElement& node, RenderStyle&& style)
    : RenderSVGResourceContainer(node, WTFMove(style))
{
}

void RenderSVGResourceGradient::removeAllClientsFromCache(bool markForInvalidation)
{
    m_gradientMap.clear();
    m_shouldCollectGradientAttributes = true;
    markAllClientsForInvalidation(markForInvalidation ? RepaintInvalidation : ParentOnlyInvalidation);
}

void RenderSVGResourceGradient::removeClientFromCache(RenderElement& client, bool markForInvalidation)
{
    m_gradientMap.remove(&client);
    markClientForInvalidation(client, markForInvalidation ? RepaintInvalidation : ParentOnlyInvalidation);
}

#if USE(CG)
static inline bool createMaskAndSwapContextForTextGradient(GraphicsContext*& context, GraphicsContext*& savedContext, std::unique_ptr<ImageBuffer>& imageBuffer, RenderObject* object)
{
    auto* textRootBlock = RenderSVGText::locateRenderSVGTextAncestor(*object);
    ASSERT(textRootBlock);

    AffineTransform absoluteTransform = SVGRenderingContext::calculateTransformationToOutermostCoordinateSystem(*textRootBlock);
    FloatRect repaintRect = textRootBlock->repaintRectInLocalCoordinates();

    auto maskImage = SVGRenderingContext::createImageBuffer(repaintRect, absoluteTransform, ColorSpace::SRGB, context->renderingMode());
    if (!maskImage)
        return false;

    GraphicsContext& maskImageContext = maskImage->context();
    ASSERT(maskImage);
    savedContext = context;
    context = &maskImageContext;
    imageBuffer = WTFMove(maskImage);
    return true;
}

static inline AffineTransform clipToTextMask(GraphicsContext& context, std::unique_ptr<ImageBuffer>& imageBuffer, FloatRect& targetRect, RenderObject* object, bool boundingBoxMode, const AffineTransform& gradientTransform)
{
    auto* textRootBlock = RenderSVGText::locateRenderSVGTextAncestor(*object);
    ASSERT(textRootBlock);

    AffineTransform absoluteTransform = SVGRenderingContext::calculateTransformationToOutermostCoordinateSystem(*textRootBlock);

    targetRect = textRootBlock->repaintRectInLocalCoordinates();

    SVGRenderingContext::clipToImageBuffer(context, absoluteTransform, targetRect, imageBuffer, false);

    AffineTransform matrix;
    if (boundingBoxMode) {
        FloatRect maskBoundingBox = textRootBlock->objectBoundingBox();
        matrix.translate(maskBoundingBox.location());
        matrix.scale(maskBoundingBox.size());
    }
    matrix *= gradientTransform;
    return matrix;
}
#endif

bool RenderSVGResourceGradient::applyResource(RenderElement& renderer, const RenderStyle& style, GraphicsContext*& context, OptionSet<RenderSVGResourceMode> resourceMode)
{
    ASSERT(context);
    ASSERT(!resourceMode.isEmpty());

    // Be sure to synchronize all SVG properties on the gradientElement _before_ processing any further.
    // Otherwhise the call to collectGradientAttributes() in createTileImage(), may cause the SVG DOM property
    // synchronization to kick in, which causes removeAllClientsFromCache() to be called, which in turn deletes our
    // GradientData object! Leaving out the line below will cause svg/dynamic-updates/SVG*GradientElement-svgdom* to crash.
    if (m_shouldCollectGradientAttributes) {
        gradientElement().synchronizeAllAttributes();
        if (!collectGradientAttributes())
            return false;

        m_shouldCollectGradientAttributes = false;
    }

    // Spec: When the geometry of the applicable element has no width or height and objectBoundingBox is specified,
    // then the given effect (e.g. a gradient or a filter) will be ignored.
    FloatRect objectBoundingBox = renderer.objectBoundingBox();
    if (gradientUnits() == SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX && objectBoundingBox.isEmpty())
        return false;

    auto& gradientData = m_gradientMap.add(&renderer, nullptr).iterator->value;
    if (!gradientData)
        gradientData = makeUnique<GradientData>();

    bool isPaintingText = resourceMode.contains(RenderSVGResourceMode::ApplyToText);

    // Create gradient object
    if (!gradientData->gradient) {
        buildGradient(gradientData.get(), style);

        // CG platforms will handle the gradient space transform for text after applying the
        // resource, so don't apply it here. For non-CG platforms, we want the text bounding
        // box applied to the gradient space transform now, so the gradient shader can use it.
#if USE(CG)
        if (gradientUnits() == SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX && !objectBoundingBox.isEmpty() && !isPaintingText) {
#else
        if (gradientUnits() == SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX && !objectBoundingBox.isEmpty()) {
#endif
            gradientData->userspaceTransform.translate(objectBoundingBox.location());
            gradientData->userspaceTransform.scale(objectBoundingBox.size());
        }

        AffineTransform gradientTransform;
        calculateGradientTransform(gradientTransform);

        gradientData->userspaceTransform *= gradientTransform;
        if (isPaintingText) {
            // Depending on font scaling factor, we may need to rescale the gradient here since
            // text painting removes the scale factor from the context.
            AffineTransform additionalTextTransform;
            if (shouldTransformOnTextPainting(renderer, additionalTextTransform))
                gradientData->userspaceTransform *= additionalTextTransform;
        }
        gradientData->gradient->setGradientSpaceTransform(gradientData->userspaceTransform);
    }

    if (!gradientData->gradient)
        return false;

    // Draw gradient
    context->save();

    if (isPaintingText) {
#if USE(CG)
        if (!createMaskAndSwapContextForTextGradient(context, m_savedContext, m_imageBuffer, &renderer)) {
            context->restore();
            return false;
        }
#endif

        context->setTextDrawingMode(resourceMode.contains(RenderSVGResourceMode::ApplyToFill) ? TextDrawingMode::Fill : TextDrawingMode::Stroke);
    }

    const SVGRenderStyle& svgStyle = style.svgStyle();

    if (resourceMode.contains(RenderSVGResourceMode::ApplyToFill)) {
        context->setAlpha(svgStyle.fillOpacity());
        context->setFillGradient(*gradientData->gradient);
        context->setFillRule(svgStyle.fillRule());
    } else if (resourceMode.contains(RenderSVGResourceMode::ApplyToStroke)) {
        if (svgStyle.vectorEffect() == VectorEffect::NonScalingStroke)
            gradientData->gradient->setGradientSpaceTransform(transformOnNonScalingStroke(&renderer, gradientData->userspaceTransform));
        context->setAlpha(svgStyle.strokeOpacity());
        context->setStrokeGradient(*gradientData->gradient);
        SVGRenderSupport::applyStrokeStyleToContext(context, style, renderer);
    }

    return true;
}

void RenderSVGResourceGradient::postApplyResource(RenderElement& renderer, GraphicsContext*& context, OptionSet<RenderSVGResourceMode> resourceMode, const Path* path, const RenderSVGShape* shape)
{
    ASSERT(context);
    ASSERT(!resourceMode.isEmpty());

    if (resourceMode.contains(RenderSVGResourceMode::ApplyToText)) {
#if USE(CG)
        // CG requires special handling for gradient on text
        GradientData* gradientData = nullptr;
        if (m_savedContext && (gradientData = m_gradientMap.get(&renderer))) {
            // Restore on-screen drawing context
            context = m_savedContext;
            m_savedContext = nullptr;

            AffineTransform gradientTransform;
            calculateGradientTransform(gradientTransform);

            FloatRect targetRect;
            gradientData->gradient->setGradientSpaceTransform(clipToTextMask(*context, m_imageBuffer, targetRect, &renderer, gradientUnits() == SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX, gradientTransform));
            context->setFillGradient(*gradientData->gradient);

            context->fillRect(targetRect);
            m_imageBuffer.reset();
        }
#else
        UNUSED_PARAM(renderer);
#endif
    } else {
        if (resourceMode.contains(RenderSVGResourceMode::ApplyToFill)) {
            if (path)
                context->fillPath(*path);
            else if (shape)
                shape->fillShape(*context);
        }
        if (resourceMode.contains(RenderSVGResourceMode::ApplyToStroke)) {
            if (path)
                context->strokePath(*path);
            else if (shape)
                shape->strokeShape(*context);
        }
    }

    context->restore();
}

void RenderSVGResourceGradient::addStops(GradientData* gradientData, const Vector<Gradient::ColorStop>& stops, const RenderStyle& style) const
{
    ASSERT(gradientData->gradient);

    for (Gradient::ColorStop stop : stops) {
        stop.color = style.colorByApplyingColorFilter(stop.color);
        gradientData->gradient->addColorStop(stop);
    }
}

GradientSpreadMethod RenderSVGResourceGradient::platformSpreadMethodFromSVGType(SVGSpreadMethodType method) const
{
    switch (method) {
    case SVGSpreadMethodUnknown:
    case SVGSpreadMethodPad:
        return SpreadMethodPad;
    case SVGSpreadMethodReflect:
        return SpreadMethodReflect;
    case SVGSpreadMethodRepeat:
        return SpreadMethodRepeat;
    }

    ASSERT_NOT_REACHED();
    return SpreadMethodPad;
}

}
