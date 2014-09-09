/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2007 Rob Buis <buis@kde.org>
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
#include "RenderSVGResource.h"

#include "Frame.h"
#include "FrameView.h"
#include "RenderSVGResourceClipper.h"
#include "RenderSVGResourceFilter.h"
#include "RenderSVGResourceMasker.h"
#include "RenderSVGResourceSolidColor.h"
#include "RenderView.h"
#include "SVGResources.h"
#include "SVGResourcesCache.h"
#include "SVGURIReference.h"

namespace WebCore {

static inline bool inheritColorFromParentStyleIfNeeded(RenderElement& object, bool applyToFill, Color& color)
{
    if (color.isValid())
        return true;
    if (!object.parent())
        return false;
    const SVGRenderStyle& parentSVGStyle = object.parent()->style().svgStyle();
    color = applyToFill ? parentSVGStyle.fillPaintColor() : parentSVGStyle.strokePaintColor();
    return true;
}

static inline RenderSVGResource* requestPaintingResource(RenderSVGResourceMode mode, RenderElement& renderer, const RenderStyle& style, Color& fallbackColor)
{
    const SVGRenderStyle& svgStyle = style.svgStyle();

    bool isRenderingMask = renderer.view().frameView().paintBehavior() & PaintBehaviorRenderingSVGMask;

    // If we have no fill/stroke, return nullptr.
    if (mode == ApplyToFillMode) {
        // When rendering the mask for a RenderSVGResourceClipper, always use the initial fill paint server, and ignore stroke.
        if (isRenderingMask) {
            RenderSVGResourceSolidColor* colorResource = RenderSVGResource::sharedSolidPaintingResource();
            colorResource->setColor(SVGRenderStyle::initialFillPaintColor());
            return colorResource;
        }

        if (!svgStyle.hasFill())
            return nullptr;
    } else {
        if (!svgStyle.hasStroke() || isRenderingMask)
            return nullptr;
    }

    bool applyToFill = mode == ApplyToFillMode;
    SVGPaint::SVGPaintType paintType = applyToFill ? svgStyle.fillPaintType() : svgStyle.strokePaintType();
    if (paintType == SVGPaint::SVG_PAINTTYPE_NONE)
        return nullptr;

    Color color;
    switch (paintType) {
    case SVGPaint::SVG_PAINTTYPE_CURRENTCOLOR:
    case SVGPaint::SVG_PAINTTYPE_RGBCOLOR:
    case SVGPaint::SVG_PAINTTYPE_RGBCOLOR_ICCCOLOR:
    case SVGPaint::SVG_PAINTTYPE_URI_CURRENTCOLOR:
    case SVGPaint::SVG_PAINTTYPE_URI_RGBCOLOR:
    case SVGPaint::SVG_PAINTTYPE_URI_RGBCOLOR_ICCCOLOR:
        color = applyToFill ? svgStyle.fillPaintColor() : svgStyle.strokePaintColor();
        break;
    default:
        break;
    }

    if (style.insideLink() == InsideVisitedLink) {
        // FIXME: This code doesn't support the uri component of the visited link paint, https://bugs.webkit.org/show_bug.cgi?id=70006
        SVGPaint::SVGPaintType visitedPaintType = applyToFill ? svgStyle.visitedLinkFillPaintType() : svgStyle.visitedLinkStrokePaintType();

        // For SVG_PAINTTYPE_CURRENTCOLOR, 'color' already contains the 'visitedColor'.
        if (visitedPaintType < SVGPaint::SVG_PAINTTYPE_URI_NONE && visitedPaintType != SVGPaint::SVG_PAINTTYPE_CURRENTCOLOR) {
            const Color& visitedColor = applyToFill ? svgStyle.visitedLinkFillPaintColor() : svgStyle.visitedLinkStrokePaintColor();
            if (visitedColor.isValid())
                color = Color(visitedColor.red(), visitedColor.green(), visitedColor.blue(), color.alpha());
        }
    }

    // If the primary resource is just a color, return immediately.
    RenderSVGResourceSolidColor* colorResource = RenderSVGResource::sharedSolidPaintingResource();
    if (paintType < SVGPaint::SVG_PAINTTYPE_URI_NONE) {
        if (!inheritColorFromParentStyleIfNeeded(renderer, applyToFill, color))
            return nullptr;

        colorResource->setColor(color);
        return colorResource;
    }

    // If no resources are associated with the given renderer, return the color resource.
    SVGResources* resources = SVGResourcesCache::cachedResourcesForRenderObject(renderer);
    if (!resources) {
        if (paintType == SVGPaint::SVG_PAINTTYPE_URI_NONE || !inheritColorFromParentStyleIfNeeded(renderer, applyToFill, color))
            return nullptr;

        colorResource->setColor(color);
        return colorResource;
    }

    // If the requested resource is not available, return the color resource.
    RenderSVGResource* uriResource = mode == ApplyToFillMode ? resources->fill() : resources->stroke();
    if (!uriResource) {
        if (!inheritColorFromParentStyleIfNeeded(renderer, applyToFill, color))
            return nullptr;

        colorResource->setColor(color);
        return colorResource;
    }

    // The paint server resource exists, though it may be invalid (pattern with width/height=0). Pass the fallback color to our caller
    // so it can use the solid color painting resource, if applyResource() on the URI resource failed.
    fallbackColor = color;
    return uriResource;
}

RenderSVGResource* RenderSVGResource::fillPaintingResource(RenderElement& renderer, const RenderStyle& style, Color& fallbackColor)
{
    return requestPaintingResource(ApplyToFillMode, renderer, style, fallbackColor);
}

RenderSVGResource* RenderSVGResource::strokePaintingResource(RenderElement& renderer, const RenderStyle& style, Color& fallbackColor)
{
    return requestPaintingResource(ApplyToStrokeMode, renderer, style, fallbackColor);
}

RenderSVGResourceSolidColor* RenderSVGResource::sharedSolidPaintingResource()
{
    static RenderSVGResourceSolidColor* s_sharedSolidPaintingResource = 0;
    if (!s_sharedSolidPaintingResource)
        s_sharedSolidPaintingResource = new RenderSVGResourceSolidColor;
    return s_sharedSolidPaintingResource;
}

static inline void removeFromCacheAndInvalidateDependencies(RenderElement& renderer, bool needsLayout)
{
    if (SVGResources* resources = SVGResourcesCache::cachedResourcesForRenderObject(renderer)) {
#if ENABLE(FILTERS)
        if (RenderSVGResourceFilter* filter = resources->filter())
            filter->removeClientFromCache(renderer);
#endif
        if (RenderSVGResourceMasker* masker = resources->masker())
            masker->removeClientFromCache(renderer);

        if (RenderSVGResourceClipper* clipper = resources->clipper())
            clipper->removeClientFromCache(renderer);
    }

    if (!renderer.element() || !renderer.element()->isSVGElement())
        return;
    HashSet<SVGElement*>* dependencies = renderer.document().accessSVGExtensions()->setOfElementsReferencingTarget(toSVGElement(renderer.element()));
    if (!dependencies)
        return;
    for (auto* element : *dependencies) {
        if (auto* renderer = element->renderer())
            RenderSVGResource::markForLayoutAndParentResourceInvalidation(*renderer, needsLayout);
    }
}

void RenderSVGResource::markForLayoutAndParentResourceInvalidation(RenderObject& object, bool needsLayout)
{
    ASSERT(object.node());

    if (needsLayout && !object.documentBeingDestroyed())
        object.setNeedsLayout();

    if (object.isRenderElement())
        removeFromCacheAndInvalidateDependencies(toRenderElement(object), needsLayout);

    // Invalidate resources in ancestor chain, if needed.
    auto current = object.parent();
    while (current) {
        removeFromCacheAndInvalidateDependencies(*current, needsLayout);

        if (current->isSVGResourceContainer()) {
            // This will process the rest of the ancestors.
            toRenderSVGResourceContainer(*current).removeAllClientsFromCache();
            break;
        }

        current = current->parent();
    }
}

}
