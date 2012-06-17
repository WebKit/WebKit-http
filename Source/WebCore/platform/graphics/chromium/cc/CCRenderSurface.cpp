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

#if USE(ACCELERATED_COMPOSITING)

#include "cc/CCRenderSurface.h"

#include "GraphicsContext3D.h"
#include "LayerChromium.h"
#include "LayerRendererChromium.h"
#include "ManagedTexture.h"
#include "TextStream.h"
#include "cc/CCDamageTracker.h"
#include "cc/CCDebugBorderDrawQuad.h"
#include "cc/CCLayerImpl.h"
#include "cc/CCMathUtil.h"
#include "cc/CCQuadCuller.h"
#include "cc/CCRenderPassDrawQuad.h"
#include "cc/CCSharedQuadState.h"
#include <public/WebTransformationMatrix.h>
#include <wtf/text/CString.h>

using WebKit::WebTransformationMatrix;

namespace WebCore {

static const int debugSurfaceBorderWidth = 2;
static const int debugSurfaceBorderAlpha = 100;
static const int debugSurfaceBorderColorRed = 0;
static const int debugSurfaceBorderColorGreen = 0;
static const int debugSurfaceBorderColorBlue = 255;
static const int debugReplicaBorderColorRed = 160;
static const int debugReplicaBorderColorGreen = 0;
static const int debugReplicaBorderColorBlue = 255;

CCRenderSurface::CCRenderSurface(CCLayerImpl* owningLayer)
    : m_owningLayer(owningLayer)
    , m_surfacePropertyChanged(false)
    , m_drawOpacity(1)
    , m_drawOpacityIsAnimating(false)
    , m_targetSurfaceTransformsAreAnimating(false)
    , m_screenSpaceTransformsAreAnimating(false)
    , m_nearestAncestorThatMovesPixels(0)
    , m_targetRenderSurfaceLayerIndexHistory(0)
    , m_currentLayerIndexHistory(0)
{
    m_damageTracker = CCDamageTracker::create();
}

CCRenderSurface::~CCRenderSurface()
{
}

FloatRect CCRenderSurface::drawableContentRect() const
{
    FloatRect localContentRect(-0.5 * m_contentRect.width(), -0.5 * m_contentRect.height(),
                               m_contentRect.width(), m_contentRect.height());
    FloatRect drawableContentRect = m_drawTransform.mapRect(localContentRect);
    if (hasReplica())
        drawableContentRect.unite(m_replicaDrawTransform.mapRect(localContentRect));

    return drawableContentRect;
}

bool CCRenderSurface::prepareContentsTexture(LayerRendererChromium* layerRenderer)
{
    TextureManager* textureManager = layerRenderer->implTextureManager();

    if (!m_contentsTexture)
        m_contentsTexture = ManagedTexture::create(textureManager);

    if (m_contentsTexture->isReserved())
        return true;

    if (!m_contentsTexture->reserve(m_contentRect.size(), GraphicsContext3D::RGBA))
        return false;

    return true;
}

void CCRenderSurface::releaseContentsTexture()
{
    if (!m_contentsTexture || !m_contentsTexture->isReserved())
        return;
    m_contentsTexture->unreserve();
}

bool CCRenderSurface::hasValidContentsTexture() const
{
    return m_contentsTexture && m_contentsTexture->isReserved() && m_contentsTexture->isValid(m_contentRect.size(), GraphicsContext3D::RGBA);
}

bool CCRenderSurface::prepareBackgroundTexture(LayerRendererChromium* layerRenderer)
{
    TextureManager* textureManager = layerRenderer->implTextureManager();

    if (!m_backgroundTexture)
        m_backgroundTexture = ManagedTexture::create(textureManager);

    if (m_backgroundTexture->isReserved())
        return true;

    if (!m_backgroundTexture->reserve(m_contentRect.size(), GraphicsContext3D::RGBA))
        return false;

    return true;
}

void CCRenderSurface::releaseBackgroundTexture()
{
    if (!m_backgroundTexture || !m_backgroundTexture->isReserved())
        return;
    m_backgroundTexture->unreserve();
}

bool CCRenderSurface::hasValidBackgroundTexture() const
{
    return m_backgroundTexture && m_backgroundTexture->isReserved() && m_backgroundTexture->isValid(m_contentRect.size(), GraphicsContext3D::RGBA);
}

String CCRenderSurface::name() const
{
    return String::format("RenderSurface(id=%i,owner=%s)", m_owningLayer->id(), m_owningLayer->debugName().utf8().data());
}

static void writeIndent(TextStream& ts, int indent)
{
    for (int i = 0; i != indent; ++i)
        ts << "  ";
}

void CCRenderSurface::dumpSurface(TextStream& ts, int indent) const
{
    writeIndent(ts, indent);
    ts << name() << "\n";

    writeIndent(ts, indent+1);
    ts << "contentRect: (" << m_contentRect.x() << ", " << m_contentRect.y() << ", " << m_contentRect.width() << ", " << m_contentRect.height() << "\n";

    writeIndent(ts, indent+1);
    ts << "drawTransform: ";
    ts << m_drawTransform.m11() << ", " << m_drawTransform.m12() << ", " << m_drawTransform.m13() << ", " << m_drawTransform.m14() << "  //  ";
    ts << m_drawTransform.m21() << ", " << m_drawTransform.m22() << ", " << m_drawTransform.m23() << ", " << m_drawTransform.m24() << "  //  ";
    ts << m_drawTransform.m31() << ", " << m_drawTransform.m32() << ", " << m_drawTransform.m33() << ", " << m_drawTransform.m34() << "  //  ";
    ts << m_drawTransform.m41() << ", " << m_drawTransform.m42() << ", " << m_drawTransform.m43() << ", " << m_drawTransform.m44() << "\n";

    writeIndent(ts, indent+1);
    ts << "damageRect is pos(" << m_damageTracker->currentDamageRect().x() << "," << m_damageTracker->currentDamageRect().y() << "), ";
    ts << "size(" << m_damageTracker->currentDamageRect().width() << "," << m_damageTracker->currentDamageRect().height() << ")\n";
}

int CCRenderSurface::owningLayerId() const
{
    return m_owningLayer ? m_owningLayer->id() : 0;
}

CCRenderSurface* CCRenderSurface::targetRenderSurface() const
{
    CCLayerImpl* parent = m_owningLayer->parent();
    if (!parent)
        return 0;
    return parent->targetRenderSurface();
}

bool CCRenderSurface::hasReplica() const
{
    return m_owningLayer->replicaLayer();
}

bool CCRenderSurface::hasMask() const
{
    return m_owningLayer->maskLayer();
}

bool CCRenderSurface::replicaHasMask() const
{
    return hasReplica() && (m_owningLayer->maskLayer() || m_owningLayer->replicaLayer()->maskLayer());
}

void CCRenderSurface::setClipRect(const IntRect& clipRect)
{
    if (m_clipRect == clipRect)
        return;

    m_surfacePropertyChanged = true;
    m_clipRect = clipRect;
}

void CCRenderSurface::setContentRect(const IntRect& contentRect)
{
    if (m_contentRect == contentRect)
        return;

    m_surfacePropertyChanged = true;
    m_contentRect = contentRect;
}

bool CCRenderSurface::surfacePropertyChanged() const
{
    // Surface property changes are tracked as follows:
    //
    // - m_surfacePropertyChanged is flagged when the clipRect or contentRect change. As
    //   of now, these are the only two properties that can be affected by descendant layers.
    //
    // - all other property changes come from the owning layer (or some ancestor layer
    //   that propagates its change to the owning layer).
    //
    ASSERT(m_owningLayer);
    return m_surfacePropertyChanged || m_owningLayer->layerPropertyChanged();
}

bool CCRenderSurface::surfacePropertyChangedOnlyFromDescendant() const
{
    return m_surfacePropertyChanged && !m_owningLayer->layerPropertyChanged();
}

PassOwnPtr<CCSharedQuadState> CCRenderSurface::createSharedQuadState() const
{
    bool isOpaque = false;
    return CCSharedQuadState::create(originTransform(), drawTransform(), contentRect(), m_scissorRect, drawOpacity(), isOpaque);
}

PassOwnPtr<CCSharedQuadState> CCRenderSurface::createReplicaSharedQuadState() const
{
    bool isOpaque = false;
    return CCSharedQuadState::create(replicaOriginTransform(), replicaDrawTransform(), contentRect(), m_scissorRect, drawOpacity(), isOpaque);
}

FloatRect CCRenderSurface::computeRootScissorRectInCurrentSurface(const FloatRect& rootScissorRect) const
{
    WebTransformationMatrix inverseScreenSpaceTransform = m_screenSpaceTransform.inverse();
    return CCMathUtil::projectClippedRect(inverseScreenSpaceTransform, rootScissorRect);
}

void CCRenderSurface::appendQuads(CCQuadCuller& quadList, CCSharedQuadState* sharedQuadState, bool forReplica, const CCRenderPass* renderPass)
{
    ASSERT(!forReplica || hasReplica());

    if (m_owningLayer->hasDebugBorders()) {
        int red = forReplica ? debugReplicaBorderColorRed : debugSurfaceBorderColorRed;
        int green = forReplica ?  debugReplicaBorderColorGreen : debugSurfaceBorderColorGreen;
        int blue = forReplica ? debugReplicaBorderColorBlue : debugSurfaceBorderColorBlue;
        Color color(red, green, blue, debugSurfaceBorderAlpha);
        quadList.appendSurface(CCDebugBorderDrawQuad::create(sharedQuadState, contentRect(), color, debugSurfaceBorderWidth));
    }

    // FIXME: By using the same RenderSurface for both the content and its reflection,
    // it's currently not possible to apply a separate mask to the reflection layer
    // or correctly handle opacity in reflections (opacity must be applied after drawing
    // both the layer and its reflection). The solution is to introduce yet another RenderSurface
    // to draw the layer and its reflection in. For now we only apply a separate reflection
    // mask if the contents don't have a mask of their own.
    CCLayerImpl* maskLayer = m_owningLayer->maskLayer();
    if (maskLayer && (!maskLayer->drawsContent() || maskLayer->bounds().isEmpty()))
        maskLayer = 0;

    if (!maskLayer && forReplica) {
        maskLayer = m_owningLayer->replicaLayer()->maskLayer();
        if (maskLayer && (!maskLayer->drawsContent() || maskLayer->bounds().isEmpty()))
            maskLayer = 0;
    }

    int maskTextureId = maskLayer ? maskLayer->contentsTextureId() : 0;

    quadList.appendSurface(CCRenderPassDrawQuad::create(sharedQuadState, contentRect(), renderPass, forReplica, filters(), backgroundFilters(), maskTextureId));
}

}
#endif // USE(ACCELERATED_COMPOSITING)
