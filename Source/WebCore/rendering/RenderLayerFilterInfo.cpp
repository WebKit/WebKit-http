/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(CSS_FILTERS)
#include "RenderLayerFilterInfo.h"

#include "CachedSVGDocument.h"
#include "CachedSVGDocumentReference.h"
#include "CustomFilterOperation.h"
#include "CustomFilterProgram.h"
#include "FilterEffectRenderer.h"
#include "SVGElement.h"
#include "SVGFilter.h"
#include "SVGFilterPrimitiveStandardAttributes.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {

HashMap<const RenderLayer*, OwnPtr<RenderLayer::FilterInfo>>& RenderLayer::FilterInfo::map()
{
    static NeverDestroyed<HashMap<const RenderLayer*, OwnPtr<FilterInfo>>> map;
    return map;
}

RenderLayer::FilterInfo* RenderLayer::FilterInfo::getIfExists(const RenderLayer& layer)
{
    ASSERT(layer.m_hasFilterInfo == map().contains(&layer));

    return layer.m_hasFilterInfo ? map().get(&layer) : 0;
}

RenderLayer::FilterInfo& RenderLayer::FilterInfo::get(RenderLayer& layer)
{
    ASSERT(layer.m_hasFilterInfo == map().contains(&layer));

    OwnPtr<FilterInfo>& info = map().add(&layer, nullptr).iterator->value;
    if (!info) {
        info = adoptPtr(new FilterInfo(layer));
        layer.m_hasFilterInfo = true;
    }
    return *info;
}

void RenderLayer::FilterInfo::remove(RenderLayer& layer)
{
    ASSERT(layer.m_hasFilterInfo == map().contains(&layer));

    if (map().remove(&layer))
        layer.m_hasFilterInfo = false;
}

RenderLayer::FilterInfo::FilterInfo(RenderLayer& layer)
    : m_layer(layer)
{
}

RenderLayer::FilterInfo::~FilterInfo()
{
#if ENABLE(CSS_SHADERS)
    removeCustomFilterClients();
#endif
#if ENABLE(SVG)
    removeReferenceFilterClients();
#endif
}

void RenderLayer::FilterInfo::setRenderer(PassRefPtr<FilterEffectRenderer> renderer)
{ 
    m_renderer = renderer; 
}

#if ENABLE(SVG)

void RenderLayer::FilterInfo::notifyFinished(CachedResource*)
{
    m_layer.renderer().element()->setNeedsStyleRecalc(SyntheticStyleChange);
    m_layer.renderer().repaint();
}

void RenderLayer::FilterInfo::updateReferenceFilterClients(const FilterOperations& operations)
{
    removeReferenceFilterClients();
    for (size_t i = 0, size = operations.size(); i < size; ++i) {
        FilterOperation* filterOperation = operations.operations()[i].get();
        if (filterOperation->type() != FilterOperation::REFERENCE)
            continue;
        ReferenceFilterOperation* referenceFilterOperation = static_cast<ReferenceFilterOperation*>(filterOperation);
        CachedSVGDocumentReference* documentReference = referenceFilterOperation->cachedSVGDocumentReference();
        CachedSVGDocument* cachedSVGDocument = documentReference ? documentReference->document() : 0;

        if (cachedSVGDocument) {
            // Reference is external; wait for notifyFinished().
            cachedSVGDocument->addClient(this);
            m_externalSVGReferences.append(cachedSVGDocument);
        } else {
            // Reference is internal; add layer as a client so we can trigger
            // filter repaint on SVG attribute change.
            Element* filter = m_layer.renderer().element()->document().getElementById(referenceFilterOperation->fragment());
            if (!filter || !filter->renderer() || !filter->renderer()->isSVGResourceFilter())
                continue;
            filter->renderer()->toRenderSVGResourceContainer()->addClientRenderLayer(&m_layer);
            m_internalSVGReferences.append(filter);
        }
    }
}

void RenderLayer::FilterInfo::removeReferenceFilterClients()
{
    for (size_t i = 0, size = m_externalSVGReferences.size(); i < size; ++i)
        m_externalSVGReferences[i]->removeClient(this);
    m_externalSVGReferences.clear();
    for (size_t i = 0, size = m_internalSVGReferences.size(); i < size; ++i) {
        Element* filter = m_internalSVGReferences[i].get();
        if (!filter->renderer())
            continue;
        filter->renderer()->toRenderSVGResourceContainer()->removeClientRenderLayer(&m_layer);
    }
    m_internalSVGReferences.clear();
}

#endif

#if ENABLE(CSS_SHADERS)

void RenderLayer::FilterInfo::notifyCustomFilterProgramLoaded(CustomFilterProgram*)
{
    m_layer.renderer().element()->setNeedsStyleRecalc(SyntheticStyleChange);
    m_layer.renderer().repaint();
}

void RenderLayer::FilterInfo::updateCustomFilterClients(const FilterOperations& operations)
{
    if (!operations.size()) {
        removeCustomFilterClients();
        return;
    }
    Vector<RefPtr<CustomFilterProgram>> cachedCustomFilterPrograms;
    for (size_t i = 0, size = operations.size(); i < size; ++i) {
        const FilterOperation* filterOperation = operations.operations()[i].get();
        if (filterOperation->type() != FilterOperation::CUSTOM)
            continue;
        const CustomFilterOperation* customFilterOperation = static_cast<const CustomFilterOperation*>(filterOperation);
        CustomFilterProgram* program = customFilterOperation->program();
        cachedCustomFilterPrograms.append(program);
        program->addClient(this);
    }
    // Remove the old clients here, after we've added the new ones, so that we don't flicker if some shaders are unchanged.
    removeCustomFilterClients();
    m_cachedCustomFilterPrograms.swap(cachedCustomFilterPrograms);
}

void RenderLayer::FilterInfo::removeCustomFilterClients()
{
    for (size_t i = 0, size = m_cachedCustomFilterPrograms.size(); i < size; ++i)
        m_cachedCustomFilterPrograms[i]->removeClient(this);
    m_cachedCustomFilterPrograms.clear();
}

#endif

} // namespace WebCore

#endif // ENABLE(CSS_FILTERS)
