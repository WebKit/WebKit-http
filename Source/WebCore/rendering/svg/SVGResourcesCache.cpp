/*
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
#include "SVGResourcesCache.h"

#if ENABLE(SVG)
#include "HTMLNames.h"
#include "RenderSVGResourceContainer.h"
#include "SVGDocumentExtensions.h"
#include "SVGResources.h"
#include "SVGResourcesCycleSolver.h"
#include "SVGStyledElement.h"

namespace WebCore {

SVGResourcesCache::SVGResourcesCache()
{
}

SVGResourcesCache::~SVGResourcesCache()
{
    deleteAllValues(m_cache);
}

void SVGResourcesCache::addResourcesFromRenderObject(RenderObject* object, const RenderStyle* style)
{
    ASSERT(object);
    ASSERT(style);
    ASSERT(!m_cache.contains(object));

    const SVGRenderStyle* svgStyle = style->svgStyle();
    ASSERT(svgStyle);

    // Build a list of all resources associated with the passed RenderObject
    SVGResources* resources = new SVGResources;
    if (!resources->buildCachedResources(object, svgStyle)) {
        delete resources;
        return;
    }

    // Put object in cache.
    m_cache.set(object, resources);

    // Run cycle-detection _afterwards_, so self-references can be caught as well.
    SVGResourcesCycleSolver solver(object, resources);
    solver.resolveCycles();

    // Walk resources and register the render object at each resources.
    HashSet<RenderSVGResourceContainer*> resourceSet;
    resources->buildSetOfResources(resourceSet);

    HashSet<RenderSVGResourceContainer*>::iterator end = resourceSet.end();
    for (HashSet<RenderSVGResourceContainer*>::iterator it = resourceSet.begin(); it != end; ++it)
        (*it)->addClient(object);
}

void SVGResourcesCache::removeResourcesFromRenderObject(RenderObject* object)
{
    if (!m_cache.contains(object))
        return;

    SVGResources* resources = m_cache.get(object);

    // Walk resources and register the render object at each resources.
    HashSet<RenderSVGResourceContainer*> resourceSet;
    resources->buildSetOfResources(resourceSet);

    HashSet<RenderSVGResourceContainer*>::iterator end = resourceSet.end();
    for (HashSet<RenderSVGResourceContainer*>::iterator it = resourceSet.begin(); it != end; ++it)
        (*it)->removeClient(object);

    delete m_cache.take(object);
}

static inline SVGResourcesCache* resourcesCacheFromRenderObject(RenderObject* renderer)
{
    Document* document = renderer->document();
    ASSERT(document);

    SVGDocumentExtensions* extensions = document->accessSVGExtensions();
    ASSERT(extensions);

    SVGResourcesCache* cache = extensions->resourcesCache();
    ASSERT(cache);

    return cache;
}

SVGResources* SVGResourcesCache::cachedResourcesForRenderObject(RenderObject* renderer)
{
    ASSERT(renderer);
    SVGResourcesCache* cache = resourcesCacheFromRenderObject(renderer);
    if (!cache->m_cache.contains(renderer))
        return 0;

    return cache->m_cache.get(renderer);
}

void SVGResourcesCache::clientLayoutChanged(RenderObject* object)
{
    SVGResources* resources = SVGResourcesCache::cachedResourcesForRenderObject(object);
    if (!resources)
        return;

    resources->removeClientFromCache(object);
}

void SVGResourcesCache::clientStyleChanged(RenderObject* renderer, StyleDifference diff, const RenderStyle* newStyle)
{
    ASSERT(renderer);
    if (diff == StyleDifferenceEqual)
        return;

    // In this case the proper SVGFE*Element will decide whether the modified CSS properties require a relayout or repaint.
    if (renderer->isSVGResourceFilterPrimitive() && diff == StyleDifferenceRepaint)
        return;

    clientUpdatedFromElement(renderer, newStyle);
    RenderSVGResource::markForLayoutAndParentResourceInvalidation(renderer, false);
}

void SVGResourcesCache::clientUpdatedFromElement(RenderObject* renderer, const RenderStyle* newStyle)
{
    ASSERT(renderer);
    ASSERT(renderer->parent());

    SVGResourcesCache* cache = resourcesCacheFromRenderObject(renderer);
    cache->removeResourcesFromRenderObject(renderer);
    cache->addResourcesFromRenderObject(renderer, newStyle);

#if ENABLE(FILTERS)
    SVGResources* resources = SVGResourcesCache::cachedResourcesForRenderObject(renderer);
    if (resources && resources->filter())
        resources->removeClientFromCache(renderer);
#endif
}

void SVGResourcesCache::clientDestroyed(RenderObject* renderer)
{
    ASSERT(renderer);
    SVGResourcesCache* cache = resourcesCacheFromRenderObject(renderer);
    cache->removeResourcesFromRenderObject(renderer);
}

void SVGResourcesCache::resourceDestroyed(RenderSVGResourceContainer* resource)
{
    ASSERT(resource);
    SVGResourcesCache* cache = resourcesCacheFromRenderObject(resource);

    // The resource itself may have clients, that need to be notified.
    cache->removeResourcesFromRenderObject(resource);

    HashMap<RenderObject*, SVGResources*>::iterator end = cache->m_cache.end();
    for (HashMap<RenderObject*, SVGResources*>::iterator it = cache->m_cache.begin(); it != end; ++it) {
        it->second->resourceDestroyed(resource);

        // Mark users of destroyed resources as pending resolution based on the id of the old resource.
        Element* resourceElement = toElement(resource->node());
        SVGStyledElement* clientElement = toSVGStyledElement(it->first->node());
        SVGDocumentExtensions* extensions = clientElement->document()->accessSVGExtensions();

        extensions->addPendingResource(resourceElement->fastGetAttribute(HTMLNames::idAttr), clientElement);
    }
}

}

#endif
