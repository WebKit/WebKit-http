/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if USE(ACCELERATED_COMPOSITING)
#include "CCDebugRectHistory.h"

#include "CCDamageTracker.h"
#include "CCLayerImpl.h"
#include "CCLayerTreeHost.h"
#include "CCMathUtil.h"

namespace WebCore {

CCDebugRectHistory::CCDebugRectHistory()
{
}

void CCDebugRectHistory::saveDebugRectsForCurrentFrame(CCLayerImpl* rootLayer, const Vector<CCLayerImpl*>& renderSurfaceLayerList, const Vector<IntRect>& occludingScreenSpaceRects, const CCLayerTreeSettings& settings)
{
    // For now, clear all rects from previous frames. In the future we may want to store
    // all debug rects for a history of many frames.
    m_debugRects.clear();

    if (settings.showPaintRects)
        savePaintRects(rootLayer);

    if (settings.showPropertyChangedRects)
        savePropertyChangedRects(renderSurfaceLayerList);

    if (settings.showSurfaceDamageRects)
        saveSurfaceDamageRects(renderSurfaceLayerList);

    if (settings.showScreenSpaceRects)
        saveScreenSpaceRects(renderSurfaceLayerList);

    if (settings.showOccludingRects)
        saveOccludingRects(occludingScreenSpaceRects);
}


void CCDebugRectHistory::savePaintRects(CCLayerImpl* layer)
{
    // We would like to visualize where any layer's paint rect (update rect) has changed,
    // regardless of whether this layer is skipped for actual drawing or not. Therefore
    // we traverse recursively over all layers, not just the render surface list.

    if (!layer->updateRect().isEmpty() && layer->drawsContent()) {
        FloatRect updateContentRect = layer->updateRect();
        updateContentRect.scale(layer->contentBounds().width() / static_cast<float>(layer->bounds().width()), layer->contentBounds().height() / static_cast<float>(layer->bounds().height()));
        m_debugRects.append(CCDebugRect(PaintRectType, CCMathUtil::mapClippedRect(layer->screenSpaceTransform(), updateContentRect)));
    }

    for (unsigned i = 0; i < layer->children().size(); ++i)
        savePaintRects(layer->children()[i].get());
}

void CCDebugRectHistory::savePropertyChangedRects(const Vector<CCLayerImpl*>& renderSurfaceLayerList)
{
    for (int surfaceIndex = renderSurfaceLayerList.size() - 1; surfaceIndex >= 0 ; --surfaceIndex) {
        CCLayerImpl* renderSurfaceLayer = renderSurfaceLayerList[surfaceIndex];
        CCRenderSurface* renderSurface = renderSurfaceLayer->renderSurface();
        ASSERT(renderSurface);

        const Vector<CCLayerImpl*>& layerList = renderSurface->layerList();
        for (unsigned layerIndex = 0; layerIndex < layerList.size(); ++layerIndex) {
            CCLayerImpl* layer = layerList[layerIndex];

            if (CCLayerTreeHostCommon::renderSurfaceContributesToTarget<CCLayerImpl>(layer, renderSurfaceLayer->id()))
                continue;

            if (layer->layerIsAlwaysDamaged())
                continue;

            if (layer->layerPropertyChanged() || layer->layerSurfacePropertyChanged())
                m_debugRects.append(CCDebugRect(PropertyChangedRectType, CCMathUtil::mapClippedRect(layer->screenSpaceTransform(), FloatRect(FloatPoint::zero(), layer->contentBounds()))));
        }
    }
}

void CCDebugRectHistory::saveSurfaceDamageRects(const Vector<CCLayerImpl* >& renderSurfaceLayerList)
{
    for (int surfaceIndex = renderSurfaceLayerList.size() - 1; surfaceIndex >= 0 ; --surfaceIndex) {
        CCLayerImpl* renderSurfaceLayer = renderSurfaceLayerList[surfaceIndex];
        CCRenderSurface* renderSurface = renderSurfaceLayer->renderSurface();
        ASSERT(renderSurface);

        m_debugRects.append(CCDebugRect(SurfaceDamageRectType, CCMathUtil::mapClippedRect(renderSurface->screenSpaceTransform(), renderSurface->damageTracker()->currentDamageRect())));
    }
}

void CCDebugRectHistory::saveScreenSpaceRects(const Vector<CCLayerImpl* >& renderSurfaceLayerList)
{
    for (int surfaceIndex = renderSurfaceLayerList.size() - 1; surfaceIndex >= 0 ; --surfaceIndex) {
        CCLayerImpl* renderSurfaceLayer = renderSurfaceLayerList[surfaceIndex];
        CCRenderSurface* renderSurface = renderSurfaceLayer->renderSurface();
        ASSERT(renderSurface);

        m_debugRects.append(CCDebugRect(ScreenSpaceRectType, CCMathUtil::mapClippedRect(renderSurface->screenSpaceTransform(), renderSurface->contentRect())));

        if (renderSurfaceLayer->replicaLayer())
            m_debugRects.append(CCDebugRect(ReplicaScreenSpaceRectType, CCMathUtil::mapClippedRect(renderSurface->replicaScreenSpaceTransform(), renderSurface->contentRect())));
    }
}

void CCDebugRectHistory::saveOccludingRects(const Vector<IntRect>& occludingRects)
{
    for (size_t i = 0; i < occludingRects.size(); ++i)
        m_debugRects.append(CCDebugRect(OccludingRectType, occludingRects[i]));
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)
