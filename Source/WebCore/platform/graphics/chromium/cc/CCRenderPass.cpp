/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#include "cc/CCRenderPass.h"

#include "Color.h"
#include "cc/CCLayerImpl.h"
#include "cc/CCQuadCuller.h"
#include "cc/CCSharedQuadState.h"
#include "cc/CCSolidColorDrawQuad.h"

using WebKit::WebTransformationMatrix;

namespace WebCore {

PassOwnPtr<CCRenderPass> CCRenderPass::create(CCRenderSurface* targetSurface)
{
    return adoptPtr(new CCRenderPass(targetSurface));
}

CCRenderPass::CCRenderPass(CCRenderSurface* targetSurface)
    : m_targetSurface(targetSurface)
    , m_framebufferOutputRect(targetSurface->contentRect())
    , m_hasTransparentBackground(true)
{
    ASSERT(m_targetSurface);
}

void CCRenderPass::appendQuadsForLayer(CCLayerImpl* layer, CCOcclusionTrackerImpl* occlusionTracker, bool& hadMissingTiles)
{
    CCQuadCuller quadCuller(m_quadList, layer, occlusionTracker, layer->hasDebugBorders());

    OwnPtr<CCSharedQuadState> sharedQuadState = layer->createSharedQuadState();
    layer->appendDebugBorderQuad(quadCuller, sharedQuadState.get());
    layer->appendQuads(quadCuller, sharedQuadState.get(), hadMissingTiles);
    m_sharedQuadStateList.append(sharedQuadState.release());
}

void CCRenderPass::appendQuadsForRenderSurfaceLayer(CCLayerImpl* layer, const CCRenderPass* contributingRenderPass, CCOcclusionTrackerImpl* occlusionTracker)
{
    // FIXME: render surface layers should be a CCLayerImpl-derived class and
    // not be handled specially here.
    CCQuadCuller quadCuller(m_quadList, layer, occlusionTracker, layer->hasDebugBorders());

    CCRenderSurface* surface = layer->renderSurface();

    OwnPtr<CCSharedQuadState> sharedQuadState = surface->createSharedQuadState();
    bool isReplica = false;
    surface->appendQuads(quadCuller, sharedQuadState.get(), isReplica, contributingRenderPass);
    m_sharedQuadStateList.append(sharedQuadState.release());

    if (!surface->hasReplica())
        return;

    // Add replica after the surface so that it appears below the surface.
    OwnPtr<CCSharedQuadState> replicaSharedQuadState = surface->createReplicaSharedQuadState();
    isReplica = true;
    surface->appendQuads(quadCuller, replicaSharedQuadState.get(), isReplica, contributingRenderPass);
    m_sharedQuadStateList.append(replicaSharedQuadState.release());
}

void CCRenderPass::appendQuadsToFillScreen(CCLayerImpl* rootLayer, const Color& screenBackgroundColor, const CCOcclusionTrackerImpl& occlusionTracker)
{
    if (!rootLayer || !screenBackgroundColor.isValid())
        return;

    Region fillRegion = occlusionTracker.computeVisibleRegionInScreen();
    if (fillRegion.isEmpty())
        return;

    OwnPtr<CCSharedQuadState> sharedQuadState = rootLayer->createSharedQuadState();
    WebTransformationMatrix transformToLayerSpace = rootLayer->screenSpaceTransform().inverse();
    Vector<IntRect> fillRects = fillRegion.rects();
    for (size_t i = 0; i < fillRects.size(); ++i) {
        IntRect layerRect = transformToLayerSpace.mapRect(fillRects[i]);
        m_quadList.append(CCSolidColorDrawQuad::create(sharedQuadState.get(), layerRect, screenBackgroundColor));
    }
    m_sharedQuadStateList.append(sharedQuadState.release());
}

}
