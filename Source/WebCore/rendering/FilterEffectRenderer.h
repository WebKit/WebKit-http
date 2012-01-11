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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
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

#if ENABLE(CSS_FILTERS)

#include "Filter.h"
#include "FilterEffect.h"
#include "FilterOperations.h"
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "ImageBuffer.h"
#include "SVGFilterBuilder.h"
#include "SourceGraphic.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

#if ENABLE(CSS_SHADERS)
#include "CustomFilterProgramClient.h"
#endif

namespace WebCore {

typedef Vector<RefPtr<FilterEffect> > FilterEffectList;
class CachedShader;
class CustomFilterProgram;
class Document;
class FilterEffectObserver;
class GraphicsContext;
class RenderLayer;

class FilterEffectRendererHelper {
public:
    FilterEffectRendererHelper(bool haveFilterEffect)
        : m_savedGraphicsContext(0)
        , m_renderLayer(0)
        , m_haveFilterEffect(haveFilterEffect)
    {
    }
    
    bool haveFilterEffect() const { return m_haveFilterEffect; }
    bool hasStartedFilterEffect() const { return m_savedGraphicsContext; }

    GraphicsContext* beginFilterEffect(RenderLayer*, GraphicsContext* oldContext, const LayoutRect& filterRect);
    GraphicsContext* applyFilterEffect();

private:
    GraphicsContext* m_savedGraphicsContext;
    RenderLayer* m_renderLayer;
    LayoutPoint m_paintOffset;
    bool m_haveFilterEffect;
};

class FilterEffectRenderer : public Filter
#if ENABLE(CSS_SHADERS)
    , public CustomFilterProgramClient
#endif
{
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassRefPtr<FilterEffectRenderer> create(FilterEffectObserver* observer)
    {
        return adoptRef(new FilterEffectRenderer(observer));
    }

    virtual void setSourceImageRect(const FloatRect& sourceImageRect)
    { 
        m_sourceDrawingRegion = sourceImageRect;
        setMaxEffectRects(sourceImageRect);
        setFilterRegion(sourceImageRect);
        m_graphicsBufferAttached = false;
    }
    virtual FloatRect sourceImageRect() const { return m_sourceDrawingRegion; }

    virtual void setFilterRegion(const FloatRect& filterRegion) { m_filterRegion = filterRegion; }
    virtual FloatRect filterRegion() const { return m_filterRegion; }

    GraphicsContext* inputContext();
    ImageBuffer* output() const { return lastEffect()->asImageBuffer(); }

    void build(Document*, const FilterOperations&);
    void updateBackingStore(const FloatRect& filterRect);
    void prepare();
    void apply();
    
    IntRect outputRect() const { return lastEffect()->hasResult() ? lastEffect()->requestedRegionOfInputImageData(IntRect(m_filterRegion)) : IntRect(); }

private:
#if ENABLE(CSS_SHADERS)
    // Implementation of the CustomFilterProgramClient interface.
    virtual void notifyCustomFilterProgramLoaded(CustomFilterProgram*);
    
    void removeCustomFilterClients();
#endif

    void setMaxEffectRects(const FloatRect& effectRect)
    {
        for (size_t i = 0; i < m_effects.size(); ++i) {
            RefPtr<FilterEffect> effect = m_effects.at(i);
            effect->setMaxEffectRect(effectRect);
        }
    }
    PassRefPtr<FilterEffect> lastEffect() const
    {
        if (m_effects.size() > 0)
            return m_effects.last();
        return 0;
    }

    FilterEffectRenderer(FilterEffectObserver*);
    virtual ~FilterEffectRenderer();
    
    FloatRect m_sourceDrawingRegion;
    FloatRect m_filterRegion;
    
    FilterEffectList m_effects;
    RefPtr<SourceGraphic> m_sourceGraphic;
    FilterEffectObserver* m_observer; // No need for a strong references here. It owns us.
    
#if ENABLE(CSS_SHADERS) && ENABLE(WEBGL)
    typedef Vector<RefPtr<CustomFilterProgram> > CustomFilterProgramList;
    CustomFilterProgramList m_cachedCustomFilterPrograms;
#endif
    
    bool m_graphicsBufferAttached;
};

} // namespace WebCore

#endif // ENABLE(CSS_FILTERS)

#endif // FilterEffectRenderer_h
