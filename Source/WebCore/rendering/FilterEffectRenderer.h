/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef FilterEffectRenderer_h
#define FilterEffectRenderer_h

#include "Filter.h"
#include "FilterEffect.h"
#include "FilterOperations.h"
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "ImageBuffer.h"
#include "IntRectExtent.h"
#include "LayoutRect.h"
#include "SVGFilterBuilder.h"
#include "SourceGraphic.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class Document;
class GraphicsContext;
class RenderElement;
class RenderLayer;

typedef Vector<RefPtr<FilterEffect>> FilterEffectList;

enum FilterConsumer {
    FilterProperty,
    FilterFunction
};

class FilterEffectRendererHelper {
public:
    FilterEffectRendererHelper(bool haveFilterEffect)
        : m_renderLayer(0)
        , m_haveFilterEffect(haveFilterEffect)
        , m_startedFilterEffect(false)
    {
    }
    
    bool haveFilterEffect() const { return m_haveFilterEffect; }
    bool hasStartedFilterEffect() const { return m_startedFilterEffect; }

    bool prepareFilterEffect(RenderLayer*, const LayoutRect& filterBoxRect, const LayoutRect& dirtyRect, const LayoutRect& layerRepaintRect);
    bool beginFilterEffect();
    void applyFilterEffect(GraphicsContext* destinationContext);
    
    GraphicsContext* filterContext() const;

    const LayoutRect& repaintRect() const { return m_repaintRect; }

private:
    RenderLayer* m_renderLayer; // FIXME: this is mainly used to get the FilterEffectRenderer. FilterEffectRendererHelper should be weaned off it.
    LayoutPoint m_paintOffset;
    LayoutRect m_repaintRect;
    bool m_haveFilterEffect;
    bool m_startedFilterEffect;
};

class FilterEffectRenderer final : public Filter {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static RefPtr<FilterEffectRenderer> create()
    {
        return adoptRef(new FilterEffectRenderer);
    }

    void setSourceImageRect(const FloatRect& sourceImageRect)
    { 
        m_sourceDrawingRegion = sourceImageRect;
        setMaxEffectRects(sourceImageRect);
        setFilterRegion(sourceImageRect);
        m_graphicsBufferAttached = false;
    }
    virtual FloatRect sourceImageRect() const override { return m_sourceDrawingRegion; }

    void setFilterRegion(const FloatRect& filterRegion) { m_filterRegion = filterRegion; }
    virtual FloatRect filterRegion() const override { return m_filterRegion; }

    GraphicsContext* inputContext();
    ImageBuffer* output() const { return lastEffect()->asImageBuffer(); }

    bool build(RenderElement*, const FilterOperations&, FilterConsumer);
    PassRefPtr<FilterEffect> buildReferenceFilter(RenderElement*, PassRefPtr<FilterEffect> previousEffect, ReferenceFilterOperation*);
    bool updateBackingStoreRect(const FloatRect& filterRect);
    void allocateBackingStoreIfNeeded();
    void clearIntermediateResults();
    void apply();
    
    IntRect outputRect() const { return lastEffect()->hasResult() ? lastEffect()->requestedRegionOfInputImageData(IntRect(m_filterRegion)) : IntRect(); }

    bool hasFilterThatMovesPixels() const { return m_hasFilterThatMovesPixels; }
    LayoutRect computeSourceImageRectForDirtyRect(const LayoutRect& filterBoxRect, const LayoutRect& dirtyRect);

private:
    void setMaxEffectRects(const FloatRect& effectRect)
    {
        for (size_t i = 0; i < m_effects.size(); ++i) {
            RefPtr<FilterEffect> effect = m_effects.at(i);
            effect->setMaxEffectRect(effectRect);
        }
    }

    FilterEffect* lastEffect() const
    {
        if (!m_effects.isEmpty())
            return m_effects.last().get();
        return nullptr;
    }

    FilterEffectRenderer();
    virtual ~FilterEffectRenderer();
    
    FloatRect m_sourceDrawingRegion;
    FloatRect m_filterRegion;
    
    FilterEffectList m_effects;
    RefPtr<SourceGraphic> m_sourceGraphic;
    
    IntRectExtent m_outsets;

    bool m_graphicsBufferAttached;
    bool m_hasFilterThatMovesPixels;
};

} // namespace WebCore

#endif // FilterEffectRenderer_h
