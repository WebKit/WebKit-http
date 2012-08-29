/*
 Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
 */

#ifndef TextureMapperGL_h
#define TextureMapperGL_h

#if USE(ACCELERATED_COMPOSITING)

#include "FloatQuad.h"
#include "GraphicsContext3D.h"
#include "IntSize.h"
#include "TextureMapper.h"
#include "TransformationMatrix.h"

namespace WebCore {

class TextureMapperGLData;
class GraphicsContext;
class TextureMapperShaderProgram;

// An OpenGL-ES2 implementation of TextureMapper.
class TextureMapperGL : public TextureMapper {
public:
    static PassOwnPtr<TextureMapperGL> create() { return adoptPtr(new TextureMapperGL); }
    virtual ~TextureMapperGL();

    enum Flag {
        SupportsBlending = 0x01,
        ShouldFlipTexture = 0x02
    };

    typedef int Flags;

    // TextureMapper implementation
    virtual void drawBorder(const Color&, float borderWidth, const FloatRect& targetRect, const TransformationMatrix& modelViewMatrix = TransformationMatrix()) OVERRIDE;
    virtual void drawRepaintCounter(int value, int pointSize, const FloatPoint&, const TransformationMatrix& modelViewMatrix = TransformationMatrix()) OVERRIDE;
    virtual void drawTexture(const BitmapTexture&, const FloatRect&, const TransformationMatrix&, float opacity, const BitmapTexture* maskTexture, unsigned exposedEdges) OVERRIDE;
    virtual void drawTexture(uint32_t texture, Flags, const IntSize& textureSize, const FloatRect& targetRect, const TransformationMatrix& modelViewMatrix, float opacity, const BitmapTexture* maskTexture, unsigned exposedEdges = AllEdges);

#if !USE(TEXMAP_OPENGL_ES_2)
    virtual void drawTextureRectangleARB(uint32_t texture, Flags, const IntSize& textureSize, const FloatRect& targetRect, const TransformationMatrix& modelViewMatrix, float opacity, const BitmapTexture* maskTexture);
#endif

    virtual void bindSurface(BitmapTexture* surface) OVERRIDE;
    virtual void beginClip(const TransformationMatrix&, const FloatRect&) OVERRIDE;
    virtual void beginPainting(PaintFlags = 0) OVERRIDE;
    virtual void endPainting() OVERRIDE;
    virtual void endClip() OVERRIDE;
    virtual IntSize maxTextureSize() const OVERRIDE { return IntSize(2000, 2000); }
    virtual PassRefPtr<BitmapTexture> createTexture() OVERRIDE;
    virtual GraphicsContext* graphicsContext() OVERRIDE { return m_context; }
    inline GraphicsContext3D* graphicsContext3D() const { return m_context3D.get(); }
    virtual void setGraphicsContext(GraphicsContext* context) OVERRIDE { m_context = context; }

#if ENABLE(CSS_FILTERS)
    void drawFiltered(const BitmapTexture& sourceTexture, const BitmapTexture& contentTexture, const FilterOperation&, int pass);
#endif

    void setEnableEdgeDistanceAntialiasing(bool enabled) { m_enableEdgeDistanceAntialiasing = enabled; }

private:
    struct ClipState {
        IntRect scissorBox;
        int stencilIndex;
        ClipState(const IntRect& scissors = IntRect(), int stencil = 1)
            : scissorBox(scissors)
            , stencilIndex(stencil)
        { }
    };

    class ClipStack {
    public:
        void push();
        void pop();
        void apply(GraphicsContext3D*);
        inline ClipState& current() { return clipState; }
        void init(const IntRect&);

    private:
        ClipState clipState;
        Vector<ClipState> clipStack;
    };

    struct DrawQuad {
        DrawQuad(const FloatRect& originalTargetRect, const FloatQuad& targetRectMappedToUnitSquare = FloatRect(FloatPoint(), FloatSize(1, 1)))
            : originalTargetRect(originalTargetRect)
            , targetRectMappedToUnitSquare(targetRectMappedToUnitSquare)
        {
        }

        FloatRect originalTargetRect;
        FloatQuad targetRectMappedToUnitSquare;
    };

    TextureMapperGL();

    bool drawTextureWithAntialiasing(uint32_t texture, Flags, const FloatRect& originalTargetRect, const TransformationMatrix& modelViewMatrix, float opacity, const BitmapTexture* maskTexture, unsigned exposedEdges);
    void drawTexturedQuadWithProgram(TextureMapperShaderProgram*, uint32_t texture, Flags, const DrawQuad&, const TransformationMatrix& modelViewMatrix, float opacity, const BitmapTexture* maskTexture);
    void drawQuad(const DrawQuad&, const TransformationMatrix& modelViewMatrix, TextureMapperShaderProgram*, GC3Denum drawingMode, bool needsBlending);

    bool beginScissorClip(const TransformationMatrix&, const FloatRect&);
    void bindDefaultSurface();
    ClipStack& clipStack();
    inline TextureMapperGLData& data() { return *m_data; }
    GraphicsContext* m_context;
    RefPtr<GraphicsContext3D> m_context3D;
    TextureMapperGLData* m_data;
    ClipStack m_clipStack;
    bool m_enableEdgeDistanceAntialiasing;

    friend class BitmapTextureGL;
};

class BitmapTextureGL : public BitmapTexture {
public:
    virtual IntSize size() const;
    virtual bool isValid() const;
    virtual bool canReuseWith(const IntSize& contentsSize, Flags = 0);
    virtual void didReset();
    void bind();
    void initializeStencil();
    ~BitmapTextureGL();
    virtual uint32_t id() const { return m_id; }
    uint32_t textureTarget() const { return GraphicsContext3D::TEXTURE_2D; }
    IntSize textureSize() const { return m_textureSize; }
    void setTextureMapper(TextureMapperGL* texmap) { m_textureMapper = texmap; }
    void updateContents(Image*, const IntRect&, const IntPoint&);
    virtual void updateContents(const void*, const IntRect& target, const IntPoint& sourceOffset, int bytesPerLine);
    virtual bool isBackedByOpenGL() const { return true; }

#if ENABLE(CSS_FILTERS)
    virtual PassRefPtr<BitmapTexture> applyFilters(const BitmapTexture& contentTexture, const FilterOperations&);
#endif

private:
    Platform3DObject m_id;
    IntSize m_textureSize;
    IntRect m_dirtyRect;
    Platform3DObject m_fbo;
    Platform3DObject m_rbo;
    bool m_shouldClear;
    TextureMapperGL::ClipStack m_clipStack;
    TextureMapperGL* m_textureMapper;

    BitmapTextureGL();
    BitmapTextureGL(TextureMapperGL* textureMapper)
        : m_id(0)
        , m_fbo(0)
        , m_rbo(0)
        , m_shouldClear(true)
        , m_textureMapper(textureMapper)
    {
    }

    void clearIfNeeded();
    void createFboIfNeeded();

    friend class TextureMapperGL;
};

typedef uint64_t ImageUID;
ImageUID uidForImage(Image*);
BitmapTextureGL* toBitmapTextureGL(BitmapTexture*);

}
#endif

#endif
