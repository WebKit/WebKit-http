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

#ifndef ShaderChromium_h
#define ShaderChromium_h

#if USE(ACCELERATED_COMPOSITING)

#include "PlatformString.h"

#if USE(SKIA)
#include "SkColorPriv.h"
#endif

namespace WebCore {

class GraphicsContext3D;

class VertexShaderPosTex {
public:
    VertexShaderPosTex();

    void init(GraphicsContext3D*, unsigned program);
    String getShaderString() const;

    int matrixLocation() const { return m_matrixLocation; }

private:
    int m_matrixLocation;
};

class VertexShaderPosTexStretch {
public:
    VertexShaderPosTexStretch();

    void init(GraphicsContext3D*, unsigned program);
    String getShaderString() const;

    int matrixLocation() const { return m_matrixLocation; }
    int offsetLocation() const { return m_offsetLocation; }
    int scaleLocation() const { return m_scaleLocation; }

private:
    int m_matrixLocation;
    int m_offsetLocation;
    int m_scaleLocation;
};

class VertexShaderPosTexYUVStretch {
public:
    VertexShaderPosTexYUVStretch();

    void init(GraphicsContext3D*, unsigned program);
    String getShaderString() const;

    int matrixLocation() const { return m_matrixLocation; }
    int yWidthScaleFactorLocation() const { return m_yWidthScaleFactorLocation; }
    int uvWidthScaleFactorLocation() const { return m_uvWidthScaleFactorLocation; }

private:
    int m_matrixLocation;
    int m_yWidthScaleFactorLocation;
    int m_uvWidthScaleFactorLocation;
};

class VertexShaderPos {
public:
    VertexShaderPos();

    void init(GraphicsContext3D*, unsigned program);
    String getShaderString() const;

    int matrixLocation() const { return m_matrixLocation; }

private:
    int m_matrixLocation;
};

class VertexShaderPosTexTransform {
public:
    VertexShaderPosTexTransform();

    void init(GraphicsContext3D*, unsigned program);
    String getShaderString() const;

    int matrixLocation() const { return m_matrixLocation; }
    int texTransformLocation() const { return m_texTransformLocation; }

private:
    int m_matrixLocation;
    int m_texTransformLocation;
};

class VertexShaderQuad {
public:
    VertexShaderQuad();

    void init(GraphicsContext3D*, unsigned program);
    String getShaderString() const;

    int matrixLocation() const { return m_matrixLocation; }
    int pointLocation() const { return m_pointLocation; }

private:
    int m_matrixLocation;
    int m_pointLocation;
};

class VertexShaderTile {
public:
    VertexShaderTile();

    void init(GraphicsContext3D*, unsigned program);
    String getShaderString() const;

    int matrixLocation() const { return m_matrixLocation; }
    int pointLocation() const { return m_pointLocation; }
    int vertexTexTransformLocation() const { return m_vertexTexTransformLocation; }

private:
    int m_matrixLocation;
    int m_pointLocation;
    int m_vertexTexTransformLocation;
};

class FragmentTexAlphaBinding {
public:
    FragmentTexAlphaBinding();

    void init(GraphicsContext3D*, unsigned program);
    int alphaLocation() const { return m_alphaLocation; }
    int samplerLocation() const { return m_samplerLocation; }

private:
    int m_samplerLocation;
    int m_alphaLocation;
};

class FragmentTexOpaqueBinding {
public:
    FragmentTexOpaqueBinding();

    void init(GraphicsContext3D*, unsigned program);
    int alphaLocation() const { return -1; }
    int samplerLocation() const { return m_samplerLocation; }

private:
    int m_samplerLocation;
};

class FragmentShaderRGBATexFlipAlpha : public FragmentTexAlphaBinding {
public:
    String getShaderString() const;
};

class FragmentShaderRGBATexAlpha : public FragmentTexAlphaBinding {
public:
    String getShaderString() const;
};

class FragmentShaderRGBATexRectFlipAlpha : public FragmentTexAlphaBinding {
public:
    String getShaderString() const;
};

class FragmentShaderRGBATexRectAlpha : public FragmentTexAlphaBinding {
public:
    String getShaderString() const;
};

class FragmentShaderRGBATexOpaque : public FragmentTexOpaqueBinding {
public:
    String getShaderString() const;
};

// Swizzles the red and blue component of sampled texel with alpha.
class FragmentShaderRGBATexSwizzleAlpha : public FragmentTexAlphaBinding {
public:
    String getShaderString() const;
};

// Swizzles the red and blue component of sampled texel without alpha.
class FragmentShaderRGBATexSwizzleOpaque : public FragmentTexOpaqueBinding {
public:
    String getShaderString() const;
};

class FragmentShaderRGBATexAlphaAA {
public:
    FragmentShaderRGBATexAlphaAA();

    void init(GraphicsContext3D*, unsigned program);
    String getShaderString() const;

    int alphaLocation() const { return m_alphaLocation; }
    int samplerLocation() const { return m_samplerLocation; }
    int edgeLocation() const { return m_edgeLocation; }

private:
    int m_samplerLocation;
    int m_alphaLocation;
    int m_edgeLocation;
};

class FragmentTexClampAlphaAABinding {
public:
    FragmentTexClampAlphaAABinding();

    void init(GraphicsContext3D*, unsigned program);
    int alphaLocation() const { return m_alphaLocation; }
    int samplerLocation() const { return m_samplerLocation; }
    int fragmentTexTransformLocation() const { return m_fragmentTexTransformLocation; }
    int edgeLocation() const { return m_edgeLocation; }

private:
    int m_samplerLocation;
    int m_alphaLocation;
    int m_fragmentTexTransformLocation;
    int m_edgeLocation;
};

class FragmentShaderRGBATexClampAlphaAA : public FragmentTexClampAlphaAABinding {
public:
    String getShaderString() const;
};

// Swizzles the red and blue component of sampled texel.
class FragmentShaderRGBATexClampSwizzleAlphaAA : public FragmentTexClampAlphaAABinding {
public:
    String getShaderString() const;
};

class FragmentShaderRGBATexAlphaMask {
public:
    FragmentShaderRGBATexAlphaMask();
    String getShaderString() const;

    void init(GraphicsContext3D*, unsigned program);
    int alphaLocation() const { return m_alphaLocation; }
    int samplerLocation() const { return m_samplerLocation; }
    int maskSamplerLocation() const { return m_maskSamplerLocation; }

private:
    int m_samplerLocation;
    int m_maskSamplerLocation;
    int m_alphaLocation;
};

class FragmentShaderRGBATexAlphaMaskAA {
public:
    FragmentShaderRGBATexAlphaMaskAA();
    String getShaderString() const;

    void init(GraphicsContext3D*, unsigned program);
    int alphaLocation() const { return m_alphaLocation; }
    int samplerLocation() const { return m_samplerLocation; }
    int maskSamplerLocation() const { return m_maskSamplerLocation; }
    int edgeLocation() const { return m_edgeLocation; }

private:
    int m_samplerLocation;
    int m_maskSamplerLocation;
    int m_alphaLocation;
    int m_edgeLocation;
};

class FragmentShaderYUVVideo {
public:
    FragmentShaderYUVVideo();
    String getShaderString() const;

    void init(GraphicsContext3D*, unsigned program);

    int yTextureLocation() const { return m_yTextureLocation; }
    int uTextureLocation() const { return m_uTextureLocation; }
    int vTextureLocation() const { return m_vTextureLocation; }
    int alphaLocation() const { return m_alphaLocation; }
    int ccMatrixLocation() const { return m_ccMatrixLocation; }
    int yuvAdjLocation() const { return m_yuvAdjLocation; }

private:
    int m_yTextureLocation;
    int m_uTextureLocation;
    int m_vTextureLocation;
    int m_alphaLocation;
    int m_ccMatrixLocation;
    int m_yuvAdjLocation;
};

class FragmentShaderColor {
public:
    FragmentShaderColor();
    String getShaderString() const;

    void init(GraphicsContext3D*, unsigned program);
    int colorLocation() const { return m_colorLocation; }

private:
    int m_colorLocation;
};

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)

#endif
