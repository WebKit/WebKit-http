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


#ifndef CCRenderSurface_h
#define CCRenderSurface_h

#if USE(ACCELERATED_COMPOSITING)

#include "FloatRect.h"
#include "IntRect.h"
#include "ProgramBinding.h"
#include "ShaderChromium.h"
#include "TextureManager.h"
#include "TransformationMatrix.h"
#include <wtf/Noncopyable.h>

namespace WebCore {

class CCLayerImpl;
class LayerRendererChromium;
class ManagedTexture;

class CCRenderSurface {
    WTF_MAKE_NONCOPYABLE(CCRenderSurface);
public:
    explicit CCRenderSurface(CCLayerImpl*);
    ~CCRenderSurface();

    bool prepareContentsTexture();
    void releaseContentsTexture();
    void cleanupResources();
    void draw(const IntRect& targetSurfaceRect);

    String name() const;
    void dumpSurface(TextStream&, int indent) const;

    FloatPoint contentRectCenter() const { return FloatRect(m_contentRect).center(); }

    // Returns the rect that encloses the RenderSurface including any reflection.
    FloatRect drawableContentRect() const;

    float drawOpacity() const { return m_drawOpacity; }
    void setDrawOpacity(float opacity) { m_drawOpacity = opacity; }

    void setDrawTransform(const TransformationMatrix& drawTransform) { m_drawTransform = drawTransform; }
    const TransformationMatrix& drawTransform() const { return m_drawTransform; }

    void setReplicaDrawTransform(const TransformationMatrix& replicaDrawTransform) { m_replicaDrawTransform = replicaDrawTransform; }
    const TransformationMatrix& replicaDrawTransform() const { return m_replicaDrawTransform; }

    void setOriginTransform(const TransformationMatrix& originTransform) { m_originTransform = originTransform; }
    const TransformationMatrix& originTransform() const { return m_originTransform; }

    void setScissorRect(const IntRect& scissorRect) { m_scissorRect = scissorRect; }
    const IntRect& scissorRect() const { return m_scissorRect; }

    void setContentRect(const IntRect& contentRect) { m_contentRect = contentRect; }
    const IntRect& contentRect() const { return m_contentRect; }

    void setSkipsDraw(bool skipsDraw) { m_skipsDraw = skipsDraw; }
    bool skipsDraw() const { return m_skipsDraw; }

    void clearLayerList() { m_layerList.clear(); }
    Vector<RefPtr<CCLayerImpl> >& layerList() { return m_layerList; }

    void setMaskLayer(CCLayerImpl* maskLayer) { m_maskLayer = maskLayer; }

    typedef ProgramBinding<VertexShaderPosTex, FragmentShaderRGBATexAlpha> Program;
    typedef ProgramBinding<VertexShaderPosTex, FragmentShaderRGBATexAlphaMask> MaskProgram;

    ManagedTexture* contentsTexture() const { return m_contentsTexture.get(); }

    int owningLayerId() const;
private:
    LayerRendererChromium* layerRenderer();
    void drawSurface(CCLayerImpl* maskLayer, const TransformationMatrix& drawTransform);

    CCLayerImpl* m_owningLayer;
    CCLayerImpl* m_maskLayer;

    IntRect m_contentRect;
    bool m_skipsDraw;

    OwnPtr<ManagedTexture> m_contentsTexture;
    float m_drawOpacity;
    TransformationMatrix m_drawTransform;
    TransformationMatrix m_replicaDrawTransform;
    TransformationMatrix m_originTransform;
    IntRect m_scissorRect;
    Vector<RefPtr<CCLayerImpl> > m_layerList;
};

}
#endif // USE(ACCELERATED_COMPOSITING)

#endif
