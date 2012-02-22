/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if USE(ACCELERATED_COMPOSITING)

#include "cc/CCOcclusionTracker.h"

#include "LayerChromium.h"
#include "cc/CCLayerImpl.h"

#include <algorithm>

using namespace std;

namespace WebCore {

template<typename LayerType, typename RenderSurfaceType>
void CCOcclusionTrackerBase<LayerType, RenderSurfaceType>::enterTargetRenderSurface(const RenderSurfaceType* newTarget)
{
    if (!m_stack.isEmpty() && m_stack.last().surface == newTarget)
        return;

    const RenderSurfaceType* oldTarget = m_stack.isEmpty() ? 0 : m_stack.last().surface;
    const RenderSurfaceType* oldAncestorThatMovesPixels = !oldTarget ? 0 : oldTarget->nearestAncestorThatMovesPixels();
    const RenderSurfaceType* newAncestorThatMovesPixels = newTarget->nearestAncestorThatMovesPixels();

    m_stack.append(StackObject());
    m_stack.last().surface = newTarget;

    // We copy the screen occlusion into the new RenderSurface subtree, but we never copy in the
    // target occlusion, since we are looking at a new RenderSurface target.

    // If we are entering a subtree that is going to move pixels around, then the occlusion we've computed
    // so far won't apply to the pixels we're drawing here in the same way. We discard the occlusion thus
    // far to be safe, and ensure we don't cull any pixels that are moved such that they become visible.
    bool enteringSubtreeThatMovesPixels = newAncestorThatMovesPixels && newAncestorThatMovesPixels != oldAncestorThatMovesPixels;

    bool copyScreenOcclusionForward = m_stack.size() > 1 && !enteringSubtreeThatMovesPixels;
    if (copyScreenOcclusionForward) {
        int lastIndex = m_stack.size() - 1;
        m_stack[lastIndex].occlusionInScreen = m_stack[lastIndex - 1].occlusionInScreen;
    }
}

template<typename LayerType, typename RenderSurfaceType>
void CCOcclusionTrackerBase<LayerType, RenderSurfaceType>::finishedTargetRenderSurface(const LayerType* owningLayer, const RenderSurfaceType* finishedTarget)
{
    // FIXME: Remove the owningLayer parameter when we can get all the info from the surface.
    ASSERT(owningLayer->renderSurface() == finishedTarget);

    // Make sure we know about the target surface.
    enterTargetRenderSurface(finishedTarget);

    if (owningLayer->maskLayer() || finishedTarget->drawOpacity() < 1 || finishedTarget->filters().hasFilterThatAffectsOpacity()) {
        m_stack.last().occlusionInScreen = Region();
        m_stack.last().occlusionInTarget = Region();
    }
}

template<typename RenderSurfaceType>
static inline Region transformSurfaceOpaqueRegion(const RenderSurfaceType* surface, const Region& region, const TransformationMatrix& transform)
{
    // Verify that rects within the |surface| will remain rects in its target surface after applying |transform|. If this is true, then
    // apply |transform| to each rect within |region| in order to transform the entire Region.

    IntRect bounds = region.bounds();
    FloatRect centeredBounds(-bounds.width() / 2.0, -bounds.height() / 2.0, bounds.width(), bounds.height());
    FloatQuad transformedBoundsQuad = transform.mapQuad(FloatQuad(centeredBounds));
    if (!transformedBoundsQuad.isRectilinear())
        return Region();

    Region transformedRegion;

    IntRect surfaceBounds = surface->contentRect();
    Vector<IntRect> rects = region.rects();
    Vector<IntRect>::const_iterator end = rects.end();
    for (Vector<IntRect>::const_iterator i = rects.begin(); i != end; ++i) {
        FloatRect centeredOriginRect(-i->width() / 2.0 + i->x() - surfaceBounds.x(), -i->height() / 2.0 + i->y() - surfaceBounds.y(), i->width(), i->height());
        FloatRect transformedRect = transform.mapRect(FloatRect(centeredOriginRect));
        transformedRegion.unite(enclosedIntRect(transformedRect));
    }
    return transformedRegion;
}

template<typename LayerType, typename RenderSurfaceType>
void CCOcclusionTrackerBase<LayerType, RenderSurfaceType>::leaveToTargetRenderSurface(const RenderSurfaceType* newTarget)
{
    int lastIndex = m_stack.size() - 1;
    bool surfaceWillBeAtTopAfterPop = m_stack.size() > 1 && m_stack[lastIndex - 1].surface == newTarget;

    // We merge the screen occlusion from the current RenderSurface subtree out to its parent target RenderSurface.
    // The target occlusion can be merged out as well but needs to be transformed to the new target.

    const RenderSurfaceType* oldTarget = m_stack[lastIndex].surface;
    Region oldTargetOcclusionInNewTarget = transformSurfaceOpaqueRegion<RenderSurfaceType>(oldTarget, m_stack[lastIndex].occlusionInTarget, oldTarget->drawTransform());

    if (surfaceWillBeAtTopAfterPop) {
        // Merge the top of the stack down.

        m_stack[lastIndex - 1].occlusionInScreen.unite(m_stack[lastIndex].occlusionInScreen);
        m_stack[lastIndex - 1].occlusionInTarget.unite(oldTargetOcclusionInNewTarget);
        m_stack.removeLast();
    } else {
        // Replace the top of the stack with the new pushed surface. Copy the occluded screen region to the top.
        m_stack.last().surface = newTarget;
        m_stack.last().occlusionInTarget = oldTargetOcclusionInNewTarget;
    }
}

template<typename LayerType>
static inline TransformationMatrix contentToScreenSpaceTransform(const LayerType* layer)
{
    IntSize boundsInLayerSpace = layer->bounds();
    IntSize boundsInContentSpace = layer->contentBounds();

    TransformationMatrix transform = layer->screenSpaceTransform();

    if (boundsInContentSpace.isEmpty())
        return transform;

    // Scale from content space to layer space
    transform.scaleNonUniform(boundsInLayerSpace.width() / static_cast<double>(boundsInContentSpace.width()),
                              boundsInLayerSpace.height() / static_cast<double>(boundsInContentSpace.height()));

    return transform;
}

template<typename LayerType>
static inline TransformationMatrix contentToTargetSurfaceTransform(const LayerType* layer)
{
    IntSize boundsInLayerSpace = layer->bounds();
    IntSize boundsInContentSpace = layer->contentBounds();

    TransformationMatrix transform = layer->drawTransform();

    if (boundsInContentSpace.isEmpty())
        return transform;

    // Scale from content space to layer space
    transform.scaleNonUniform(boundsInLayerSpace.width() / static_cast<double>(boundsInContentSpace.width()),
                              boundsInLayerSpace.height() / static_cast<double>(boundsInContentSpace.height()));

    // The draw transform expects the origin to be in the middle of the layer.
    transform.translate(-boundsInContentSpace.width() / 2.0, -boundsInContentSpace.height() / 2.0);

    return transform;
}

template<typename LayerType>
static inline Region computeOcclusionBehindLayer(const LayerType* layer, const TransformationMatrix& transform)
{
    Region opaqueRegion;

    FloatQuad unoccludedQuad = transform.mapQuad(FloatQuad(layer->visibleLayerRect()));
    bool isPaintedAxisAligned = unoccludedQuad.isRectilinear();
    if (!isPaintedAxisAligned)
        return opaqueRegion;

    if (layer->opaque())
        opaqueRegion = enclosedIntRect(unoccludedQuad.boundingBox());
    // FIXME: Capture opaque paints: else opaqueRegion = layer->opaqueContentsRegion(transform);
    return opaqueRegion;
}

template<typename LayerType, typename RenderSurfaceType>
void CCOcclusionTrackerBase<LayerType, RenderSurfaceType>::markOccludedBehindLayer(const LayerType* layer)
{
    ASSERT(!m_stack.isEmpty());
    ASSERT(layer->targetRenderSurface() == m_stack.last().surface);

    if (layer->drawOpacity() != 1)
        return;

    TransformationMatrix contentToScreenSpace = contentToScreenSpaceTransform<LayerType>(layer);
    TransformationMatrix contentToTargetSurface = contentToTargetSurfaceTransform<LayerType>(layer);

    m_stack.last().occlusionInScreen.unite(computeOcclusionBehindLayer<LayerType>(layer, contentToScreenSpace));
    m_stack.last().occlusionInTarget.unite(computeOcclusionBehindLayer<LayerType>(layer, contentToTargetSurface));
}

static inline bool testContentRectOccluded(const IntRect& contentRect, const TransformationMatrix& contentSpaceTransform, const Region& occlusion)
{
    FloatQuad transformedQuad = contentSpaceTransform.mapQuad(FloatQuad(contentRect));
    return occlusion.contains(transformedQuad.enclosingBoundingBox());
}

template<typename LayerType, typename RenderSurfaceType>
bool CCOcclusionTrackerBase<LayerType, RenderSurfaceType>::occluded(const LayerType* layer, const IntRect& contentRect) const
{
    if (m_stack.isEmpty())
        return false;

    ASSERT(layer->targetRenderSurface() == m_stack.last().surface);

    if (testContentRectOccluded(contentRect, contentToScreenSpaceTransform<LayerType>(layer), m_stack.last().occlusionInScreen))
        return true;
    if (testContentRectOccluded(contentRect, contentToTargetSurfaceTransform<LayerType>(layer), m_stack.last().occlusionInTarget))
        return true;
    return false;
}

template<typename LayerType, typename RenderSurfaceType>
bool CCOcclusionTrackerBase<LayerType, RenderSurfaceType>::surfaceOccluded(const LayerType* layer, const IntRect& surfaceContentRect) const
{
    // A surface is not occluded by layers drawing into itself, so we need to use occlusion from one spot down on the stack.
    if (m_stack.size() < 2)
        return false;

    ASSERT(layer->renderSurface());
    ASSERT(layer->renderSurface() == m_stack.last().surface);

    TransformationMatrix surfaceContentToScreenSpace = contentToScreenSpaceTransform<LayerType>(layer);

    const StackObject& secondLast = m_stack[m_stack.size()-2];
    if (testContentRectOccluded(surfaceContentRect, surfaceContentToScreenSpace, secondLast.occlusionInScreen))
        return true;
    return false;
}

// Determines what portion of rect, if any, is unoccluded (not occluded by region). If
// the resulting unoccluded region is not rectangular, we return a rect containing it.
static inline IntRect rectSubtractRegion(const IntRect& rect, const Region& region)
{
    Region rectRegion(rect);
    Region intersectRegion(intersect(region, rectRegion));

    if (intersectRegion.isEmpty())
        return rect;

    rectRegion.subtract(intersectRegion);
    IntRect boundsRect = rectRegion.bounds();
    return boundsRect;
}

static IntRect computeUnoccludedContentRect(const IntRect& contentRect, const TransformationMatrix& contentSpaceTransform, const Region& occlusion)
{
    FloatQuad transformedQuad = contentSpaceTransform.mapQuad(FloatQuad(contentRect));
    if (!transformedQuad.isRectilinear())
        return contentRect;
    // Take the enclosingIntRect at each step here, as we want to contain any unoccluded partial pixels in the resulting IntRect.
    IntRect shrunkRect = rectSubtractRegion(enclosingIntRect(transformedQuad.boundingBox()), occlusion);
    IntRect unoccludedRect = enclosingIntRect(contentSpaceTransform.inverse().mapRect(FloatRect(shrunkRect)));
    // The use of enclosingIntRect, with floating point rounding, can give us a result that is not a sub-rect of contentRect, but our
    // return value should be a sub-rect.
    return intersection(unoccludedRect, contentRect);
}

template<typename LayerType, typename RenderSurfaceType>
IntRect CCOcclusionTrackerBase<LayerType, RenderSurfaceType>::unoccludedContentRect(const LayerType* layer, const IntRect& contentRect) const
{
    if (m_stack.isEmpty())
        return contentRect;

    // We want to return a rect that contains all the visible parts of |contentRect| in both screen space and in the target surface.
    // So we find the visible parts of |contentRect| in each space, and take the intersection.

    TransformationMatrix contentToScreenSpace = contentToScreenSpaceTransform<LayerType>(layer);
    TransformationMatrix contentToTargetSurface = contentToTargetSurfaceTransform<LayerType>(layer);

    IntRect unoccludedInScreen = computeUnoccludedContentRect(contentRect, contentToScreenSpace, m_stack.last().occlusionInScreen);
    IntRect unoccludedInTarget = computeUnoccludedContentRect(contentRect, contentToTargetSurface, m_stack.last().occlusionInTarget);

    return intersection(unoccludedInScreen, unoccludedInTarget);
}

template<typename LayerType, typename RenderSurfaceType>
IntRect CCOcclusionTrackerBase<LayerType, RenderSurfaceType>::surfaceUnoccludedContentRect(const LayerType* layer, const IntRect& surfaceContentRect) const
{
    // A surface is not occluded by layers drawing into itself, so we need to use occlusion from one spot down on the stack.
    if (m_stack.size() < 2)
        return surfaceContentRect;

    ASSERT(layer->renderSurface());
    ASSERT(layer->renderSurface() == m_stack.last().surface);

    // We want to return a rect that contains all the visible parts of |contentRect| in both screen space and in the target surface.
    // So we find the visible parts of |contentRect| in each space, and take the intersection.

    TransformationMatrix contentToScreenSpace = contentToScreenSpaceTransform<LayerType>(layer);

    const StackObject& secondLast = m_stack[m_stack.size()-2];
    IntRect unoccludedInScreen = computeUnoccludedContentRect(surfaceContentRect, contentToScreenSpace, secondLast.occlusionInScreen);

    return unoccludedInScreen;
}

template<typename LayerType, typename RenderSurfaceType>
const Region& CCOcclusionTrackerBase<LayerType, RenderSurfaceType>::currentOcclusionInScreenSpace() const
{
    ASSERT(!m_stack.isEmpty());
    return m_stack.last().occlusionInScreen;
}

template<typename LayerType, typename RenderSurfaceType>
const Region& CCOcclusionTrackerBase<LayerType, RenderSurfaceType>::currentOcclusionInTargetSurface() const
{
    ASSERT(!m_stack.isEmpty());
    return m_stack.last().occlusionInTarget;
}


// Declare the possible functions here for the linker.
template void CCOcclusionTrackerBase<LayerChromium, RenderSurfaceChromium>::enterTargetRenderSurface(const RenderSurfaceChromium* newTarget);
template void CCOcclusionTrackerBase<LayerChromium, RenderSurfaceChromium>::finishedTargetRenderSurface(const LayerChromium* owningLayer, const RenderSurfaceChromium* finishedTarget);
template void CCOcclusionTrackerBase<LayerChromium, RenderSurfaceChromium>::leaveToTargetRenderSurface(const RenderSurfaceChromium* newTarget);
template void CCOcclusionTrackerBase<LayerChromium, RenderSurfaceChromium>::markOccludedBehindLayer(const LayerChromium*);
template bool CCOcclusionTrackerBase<LayerChromium, RenderSurfaceChromium>::occluded(const LayerChromium*, const IntRect& contentRect) const;
template bool CCOcclusionTrackerBase<LayerChromium, RenderSurfaceChromium>::surfaceOccluded(const LayerChromium*, const IntRect& surfaceContentRect) const;
template IntRect CCOcclusionTrackerBase<LayerChromium, RenderSurfaceChromium>::unoccludedContentRect(const LayerChromium*, const IntRect& contentRect) const;
template IntRect CCOcclusionTrackerBase<LayerChromium, RenderSurfaceChromium>::surfaceUnoccludedContentRect(const LayerChromium*, const IntRect& surfaceContentRect) const;
template const Region& CCOcclusionTrackerBase<LayerChromium, RenderSurfaceChromium>::currentOcclusionInScreenSpace() const;
template const Region& CCOcclusionTrackerBase<LayerChromium, RenderSurfaceChromium>::currentOcclusionInTargetSurface() const;

template void CCOcclusionTrackerBase<CCLayerImpl, CCRenderSurface>::enterTargetRenderSurface(const CCRenderSurface* newTarget);
template void CCOcclusionTrackerBase<CCLayerImpl, CCRenderSurface>::finishedTargetRenderSurface(const CCLayerImpl* owningLayer, const CCRenderSurface* finishedTarget);
template void CCOcclusionTrackerBase<CCLayerImpl, CCRenderSurface>::leaveToTargetRenderSurface(const CCRenderSurface* newTarget);
template void CCOcclusionTrackerBase<CCLayerImpl, CCRenderSurface>::markOccludedBehindLayer(const CCLayerImpl*);
template bool CCOcclusionTrackerBase<CCLayerImpl, CCRenderSurface>::occluded(const CCLayerImpl*, const IntRect& contentRect) const;
template bool CCOcclusionTrackerBase<CCLayerImpl, CCRenderSurface>::surfaceOccluded(const CCLayerImpl*, const IntRect& surfaceContentRect) const;
template IntRect CCOcclusionTrackerBase<CCLayerImpl, CCRenderSurface>::unoccludedContentRect(const CCLayerImpl*, const IntRect& contentRect) const;
template IntRect CCOcclusionTrackerBase<CCLayerImpl, CCRenderSurface>::surfaceUnoccludedContentRect(const CCLayerImpl*, const IntRect& surfaceContentRect) const;
template const Region& CCOcclusionTrackerBase<CCLayerImpl, CCRenderSurface>::currentOcclusionInScreenSpace() const;
template const Region& CCOcclusionTrackerBase<CCLayerImpl, CCRenderSurface>::currentOcclusionInTargetSurface() const;


} // namespace WebCore
#endif // USE(ACCELERATED_COMPOSITING)
