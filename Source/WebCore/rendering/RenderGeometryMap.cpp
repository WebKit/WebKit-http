/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
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

#include "config.h"
#include "RenderGeometryMap.h"

#include "RenderView.h"
#include "TransformState.h"

namespace WebCore {


// Stores data about how to map from one renderer to its container.
class RenderGeometryMapStep {
    WTF_MAKE_NONCOPYABLE(RenderGeometryMapStep);
public:
    RenderGeometryMapStep(const RenderObject* renderer, bool accumulatingTransform, bool isNonUniform, bool isFixedPosition, bool hasTransform)
        : m_renderer(renderer)
        , m_accumulatingTransform(accumulatingTransform)
        , m_isNonUniform(isNonUniform)
        , m_isFixedPosition(isFixedPosition)
        , m_hasTransform(hasTransform)
    {
    }
        
    FloatPoint mapPoint(const FloatPoint& p) const
    {
        if (!m_transform)
            return p + m_offset;
        
        return m_transform->mapPoint(p);
    }
    
    FloatQuad mapQuad(const FloatQuad& quad) const
    {
        if (!m_transform) {
            FloatQuad q = quad;
            q.move(m_offset);
            return q;
        }
        
        return m_transform->mapQuad(quad);
    }
    
    const RenderObject* m_renderer;
    LayoutSize m_offset;
    OwnPtr<TransformationMatrix> m_transform; // Includes offset if non-null.
    bool m_accumulatingTransform;
    bool m_isNonUniform; // Mapping depends on the input point, e.g. because of CSS columns.
    bool m_isFixedPosition;
    bool m_hasTransform;
};


RenderGeometryMap::RenderGeometryMap()
    : m_insertionPosition(notFound)
    , m_nonUniformStepsCount(0)
    , m_transformedStepsCount(0)
    , m_fixedStepsCount(0)
{
}

RenderGeometryMap::~RenderGeometryMap()
{
}

FloatPoint RenderGeometryMap::absolutePoint(const FloatPoint& p) const
{
    FloatPoint result;
    
    if (!hasFixedPositionStep() && !hasTransformStep() && !hasNonUniformStep())
        result = p + m_accumulatedOffset;
    else {
        TransformState transformState(TransformState::ApplyTransformDirection, p);
        mapToAbsolute(transformState);
        result = transformState.lastPlanarPoint();
    }

#if !ASSERT_DISABLED
    FloatPoint rendererMappedResult = m_mapping.last()->m_renderer->localToAbsolute(p, false, true);
    ASSERT(rendererMappedResult == result);
#endif

    return result;
}

FloatRect RenderGeometryMap::absoluteRect(const FloatRect& rect) const
{
    FloatRect result;
    
    if (!hasFixedPositionStep() && !hasTransformStep() && !hasNonUniformStep()) {
        result = rect;
        result.move(m_accumulatedOffset);
    } else {
        TransformState transformState(TransformState::ApplyTransformDirection, rect.center(), rect);
        mapToAbsolute(transformState);
        result = transformState.lastPlanarQuad().boundingBox();
    }

#if !ASSERT_DISABLED
    FloatRect rendererMappedResult = m_mapping.last()->m_renderer->localToAbsoluteQuad(rect).boundingBox();
    // Inspector creates renderers with negative width <https://bugs.webkit.org/show_bug.cgi?id=87194>.
    // Taking FloatQuad bounds avoids spurious assertions because of that.
    ASSERT(enclosingIntRect(rendererMappedResult) == enclosingIntRect(FloatQuad(result).boundingBox()));
#endif

    return result;
}

void RenderGeometryMap::mapToAbsolute(TransformState& transformState) const
{
    // If the mapping includes something like columns, we have to go via renderers.
    if (hasNonUniformStep()) {
        bool fixed = false;
        m_mapping.last()->m_renderer->mapLocalToContainer(0, fixed, true, transformState, RenderObject::ApplyContainerFlip);
        return;
    }
    
    bool inFixed = false;

    for (int i = m_mapping.size() - 1; i >= 0; --i) {
        const RenderGeometryMapStep* currStep = m_mapping[i].get();

        // If this box has a transform, it acts as a fixed position container
        // for fixed descendants, which prevents the propagation of 'fixed'
        // unless the layer itself is also fixed position.
        if (currStep->m_hasTransform && !currStep->m_isFixedPosition)
            inFixed = false;
        else if (currStep->m_isFixedPosition)
            inFixed = true;

        if (!i) {
            if (currStep->m_transform)
                transformState.applyTransform(*currStep->m_transform.get());

            // The root gets special treatment for fixed position
            if (inFixed)
                transformState.move(currStep->m_offset.width(), currStep->m_offset.height());
        } else {
            TransformState::TransformAccumulation accumulate = currStep->m_accumulatingTransform ? TransformState::AccumulateTransform : TransformState::FlattenTransform;
            if (currStep->m_transform)
                transformState.applyTransform(*currStep->m_transform.get(), accumulate);
            else
                transformState.move(currStep->m_offset.width(), currStep->m_offset.height(), accumulate);
        }
    }

    transformState.flatten();    
}

void RenderGeometryMap::pushMappingsToAncestor(const RenderObject* renderer, const RenderBoxModelObject* ancestor)
{
    const RenderObject* currRenderer = renderer;
    
    // We need to push mappings in reverse order here, so do insertions rather than appends.
    m_insertionPosition = m_mapping.size();
    
    do {
        currRenderer = currRenderer->pushMappingToContainer(ancestor, *this);
    } while (currRenderer && currRenderer != ancestor);
    
    m_insertionPosition = notFound;
}

void RenderGeometryMap::push(const RenderObject* renderer, const LayoutSize& offsetFromContainer, bool accumulatingTransform, bool isNonUniform, bool isFixedPosition, bool hasTransform)
{
    ASSERT(m_insertionPosition != notFound);
    
    OwnPtr<RenderGeometryMapStep> step = adoptPtr(new RenderGeometryMapStep(renderer, accumulatingTransform, isNonUniform, isFixedPosition, hasTransform));
    step->m_offset = offsetFromContainer;
    
    stepInserted(*step.get());
    m_mapping.insert(m_insertionPosition, step.release());
}

void RenderGeometryMap::push(const RenderObject* renderer, const TransformationMatrix& t, bool accumulatingTransform, bool isNonUniform, bool isFixedPosition, bool hasTransform)
{
    ASSERT(m_insertionPosition != notFound);

    OwnPtr<RenderGeometryMapStep> step = adoptPtr(new RenderGeometryMapStep(renderer, accumulatingTransform, isNonUniform, isFixedPosition, hasTransform));
    step->m_transform = adoptPtr(new TransformationMatrix(t));

    stepInserted(*step.get());
    m_mapping.insert(m_insertionPosition, step.release());
}

void RenderGeometryMap::pushView(const RenderView* view, const LayoutSize& scrollOffset, const TransformationMatrix* t)
{
    ASSERT(m_insertionPosition != notFound);

    OwnPtr<RenderGeometryMapStep> step = adoptPtr(new RenderGeometryMapStep(view, false, false, false, t));
    step->m_offset = scrollOffset;
    if (t)
        step->m_transform = adoptPtr(new TransformationMatrix(*t));
        
    ASSERT(!m_mapping.size()); // The view should always be the first thing pushed.
    stepInserted(*step.get());
    m_mapping.insert(m_insertionPosition, step.release());
}

void RenderGeometryMap::popMappingsToAncestor(const RenderBoxModelObject* ancestor)
{
    ASSERT(m_mapping.size());

    while (m_mapping.size() && m_mapping.last()->m_renderer != ancestor) {
        stepRemoved(*m_mapping.last().get());
        m_mapping.removeLast();
    }
}

void RenderGeometryMap::stepInserted(const RenderGeometryMapStep& step)
{
    // Offset on the first step is the RenderView's offset, which is only applied when we have fixed-position.s
    if (m_mapping.size())
        m_accumulatedOffset += step.m_offset;

    if (step.m_isNonUniform)
        ++m_nonUniformStepsCount;

    if (step.m_transform)
        ++m_transformedStepsCount;
    
    if (step.m_isFixedPosition)
        ++m_fixedStepsCount;
}

void RenderGeometryMap::stepRemoved(const RenderGeometryMapStep& step)
{
    // Offset on the first step is the RenderView's offset, which is only applied when we have fixed-position.s
    if (m_mapping.size() > 1)
        m_accumulatedOffset -= step.m_offset;

    if (step.m_isNonUniform) {
        ASSERT(m_nonUniformStepsCount);
        --m_nonUniformStepsCount;
    }

    if (step.m_transform) {
        ASSERT(m_transformedStepsCount);
        --m_transformedStepsCount;
    }

    if (step.m_isFixedPosition) {
        ASSERT(m_fixedStepsCount);
        --m_fixedStepsCount;
    }
}

} // namespace WebCore
