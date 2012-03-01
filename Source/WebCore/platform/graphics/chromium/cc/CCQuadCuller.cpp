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

#include "cc/CCQuadCuller.h"

#include "Region.h"
#include "TransformationMatrix.h"
#include "cc/CCLayerImpl.h"
#include "cc/CCRenderPass.h"
#include "cc/CCRenderSurfaceDrawQuad.h"

using namespace std;

namespace std {

// Specialize for OwnPtr<CCDrawQuad> since Vector doesn't know how to reverse a Vector of OwnPtr<T> in general.
template<>
void swap(OwnPtr<WebCore::CCDrawQuad>& a, OwnPtr<WebCore::CCDrawQuad>& b)
{
    a.swap(b);
}

}

namespace WebCore {

// Determines what portion of rect, if any, is visible (not occluded by region). If
// the resulting visible region is not rectangular, we just return the original rect.
static IntRect rectSubtractRegion(const Region& region, const IntRect& rect)
{
    Region rectRegion(rect);
    Region intersectRegion(intersect(region, rectRegion));

    if (intersectRegion.isEmpty())
        return rect;

    // Test if intersectRegion = rectRegion, if so return empty rect.
    rectRegion.subtract(intersectRegion);
    IntRect boundsRect = rectRegion.bounds();
    if (boundsRect.isEmpty())
        return boundsRect;

    // Test if rectRegion is still a rectangle. If it is, it will be identical to its bounds.
    Region boundsRegion(boundsRect);
    boundsRegion.subtract(rectRegion);
    if (boundsRegion.isEmpty())
        return boundsRect;

    return rect;
}

static float wedgeProduct(const FloatPoint& p1, const FloatPoint& p2)
{
    return p1.x() * p2.y() - p1.y() * p2.x();
}

// Computes area of quads that are possibly non-rectangular. Can
// be easily extended to polygons.
static float quadArea(const FloatQuad& quad)
{
    return fabs(0.5 * (wedgeProduct(quad.p1(), quad.p2()) +
                   wedgeProduct(quad.p2(), quad.p3()) +
                   wedgeProduct(quad.p3(), quad.p4()) +
                   wedgeProduct(quad.p4(), quad.p1())));
}

void CCQuadCuller::cullOccludedQuads(CCQuadList& quadList, bool haveDamageRect, const FloatRect& damageRect, CCOverdrawCounts* overdrawMetrics)
{
    if (!quadList.size())
        return;

    CCQuadList culledList;
    culledList.reserveCapacity(quadList.size());

    Region opaqueCoverageThusFar;

    for (int i = quadList.size() - 1; i >= 0; --i) {
        CCDrawQuad* drawQuad = quadList[i].get();

        FloatRect floatTransformedRect = drawQuad->quadTransform().mapRect(FloatRect(drawQuad->quadRect()));
        if (haveDamageRect)
            floatTransformedRect.intersect(damageRect);
        // Inflate rect to be tested to stay conservative.
        IntRect transformedQuadRect(enclosingIntRect(floatTransformedRect));

        IntRect transformedVisibleQuadRect = rectSubtractRegion(opaqueCoverageThusFar, transformedQuadRect);
        bool keepQuad = !transformedVisibleQuadRect.isEmpty();

        // See if we can reduce the number of pixels to draw by reducing the size of the draw
        // quad - we do this by changing its visible rect.
        bool didReduceQuadSize = false;
        if (keepQuad) {
            if (transformedVisibleQuadRect != transformedQuadRect && drawQuad->isLayerAxisAlignedIntRect()) {
                drawQuad->setQuadVisibleRect(drawQuad->quadTransform().inverse().mapRect(transformedVisibleQuadRect));
                didReduceQuadSize = true;
            }

            // When adding rect to opaque region, deflate it to stay conservative.
            if (drawQuad->isLayerAxisAlignedIntRect() && !drawQuad->opaqueRect().isEmpty()) {
                FloatRect floatOpaqueRect = drawQuad->quadTransform().mapRect(FloatRect(drawQuad->opaqueRect()));
                opaqueCoverageThusFar.unite(Region(enclosedIntRect(floatOpaqueRect)));
            }

            culledList.append(quadList[i].release());
        }

        if (overdrawMetrics) {
            TRACE_EVENT("CCQuadCuller::cullOccludedQuads_OverdrawMetrics", 0, 0);
            // We compute the area of the transformed quad, as this should be in pixels.
            float area = quadArea(drawQuad->quadTransform().mapQuad(FloatQuad(drawQuad->quadRect())));
            if (keepQuad) {
                if (didReduceQuadSize) {
                    float visibleQuadRectArea = quadArea(drawQuad->quadTransform().mapQuad(FloatQuad(drawQuad->quadVisibleRect())));
                    overdrawMetrics->m_pixelsCulled += area - visibleQuadRectArea;
                    area = visibleQuadRectArea;
                }
                IntRect visibleOpaqueRect(drawQuad->quadVisibleRect());
                visibleOpaqueRect.intersect(drawQuad->opaqueRect());
                FloatQuad visibleOpaqueQuad = drawQuad->quadTransform().mapQuad(FloatQuad(visibleOpaqueRect));
                float opaqueArea = quadArea(visibleOpaqueQuad);
                overdrawMetrics->m_pixelsDrawnOpaque += opaqueArea;
                overdrawMetrics->m_pixelsDrawnTransparent += area - opaqueArea;
            } else
                overdrawMetrics->m_pixelsCulled += area;
        }
    }
    quadList.clear(); // Release anything that remains.

    culledList.reverse();
    quadList.swap(culledList);
}

} // namespace WebCore
#endif // USE(ACCELERATED_COMPOSITING)
