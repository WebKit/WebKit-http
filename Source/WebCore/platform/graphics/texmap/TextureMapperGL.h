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

#if USE(TEXTURE_MAPPER)

#include "FilterOperation.h"
#include "FloatQuad.h"
#include "GraphicsContext3D.h"
#include "IntSize.h"
#include "TextureMapper.h"
#include "TransformationMatrix.h"

namespace WebCore {

class TextureMapperGLData;
class TextureMapperShaderProgram;
class FilterOperation;

// An OpenGL-ES2 implementation of TextureMapper.
class TextureMapperGL : public TextureMapper {
public:
    TextureMapperGL();
    virtual ~TextureMapperGL();

    enum Flag {
        ShouldBlend = 0x01,
        ShouldFlipTexture = 0x02,
        ShouldUseARBTextureRect = 0x04,
        ShouldAntialias = 0x08
    };

    typedef int Flags;

    // TextureMapper implementation
    virtual void drawBorder(const Color&, float borderWidth, const FloatRect&, const TransformationMatrix&) override;
    virtual void drawNumber(int number, const Color&, const FloatPoint&, const TransformationMatrix&) override;
    virtual void drawTexture(const BitmapTexture&, const FloatRect&, const TransformationMatrix&, float opacity, unsigned exposedEdges) override;
    virtual void drawTexture(Platform3DObject texture, Flags, const IntSize& textureSize, const FloatRect& targetRect, const TransformationMatrix& modelViewMatrix, float opacity, unsigned exposedEdges = AllEdges);
    virtual void drawSolidColor(const FloatRect&, const TransformationMatrix&, const Color&) override;

    virtual void bindSurface(BitmapTexture* surface) override;
    virtual void beginClip(const TransformationMatrix&, const FloatRect&) override;
    virtual void beginPainting(PaintFlags = 0) override;
    virtual void endPainting() override;
    virtual void endClip() override;
    virtual IntRect clipBounds() override;
    virtual IntSize maxTextureSize() const override { return IntSize(2000, 2000); }
    virtual PassRefPtr<BitmapTexture> createTexture() override;
    inline GraphicsContext3D* graphicsContext3D() const { return m_context3D.get(); }

#if ENABLE(CSS_FILTERS)
    void drawFiltered(const BitmapTexture& sourceTexture, const BitmapTexture* contentTexture, const FilterOperation&, int pass);
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
        ClipStack()
            : clipStateDirty(false)
        { }

        // Y-axis should be inverted only when painting into the window.
        enum YAxisMode {
            DefaultYAxis,
            InvertedYAxis
        };

        void push();
        void pop();
        void apply(GraphicsContext3D*);
        void applyIfNeeded(GraphicsContext3D*);
        inline ClipState& current() { return clipState; }
        void reset(const IntRect&, YAxisMode);
        void intersect(const IntRect&);
        void setStencilIndex(int);
        inline int getStencilIndex() const
        {
            return clipState.stencilIndex;
        }
        inline bool isCurrentScissorBoxEmpty() const
        {
            return clipState.scissorBox.isEmpty();
        }

    private:
        ClipState clipState;
        Vector<ClipState> clipStack;
        bool clipStateDirty;
        IntSize size;
        YAxisMode yAxisMode;
    };

    void drawTexturedQuadWithProgram(TextureMapperShaderProgram*, uint32_t texture, Flags, const IntSize&, const FloatRect&, const TransformationMatrix& modelViewMatrix, float opacity);
    void draw(const FloatRect&, const TransformationMatrix& modelViewMatrix, TextureMapperShaderProgram*, GC3Denum drawingMode, Flags);

    void drawUnitRect(TextureMapperShaderProgram*, GC3Denum drawingMode);
    void drawEdgeTriangles(TextureMapperShaderProgram*);

    bool beginScissorClip(const TransformationMatrix&, const FloatRect&);
    void bindDefaultSurface();
    ClipStack& clipStack();
    inline TextureMapperGLData& data() { return *m_data; }
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
    void bind(TextureMapperGL*);
    void initializeStencil();
    void initializeDepthBuffer();
    ~BitmapTextureGL();
    virtual uint32_t id() const { return m_id; }
    uint32_t textureTarget() const { return GraphicsContext3D::TEXTURE_2D; }
    IntSize textureSize() const { return m_textureSize; }
    void updateContents(Image*, const IntRect&, const IntPoint&, UpdateContentsFlag);
    virtual void updateContents(const void*, const IntRect& target, const IntPoint& sourceOffset, int bytesPerLine, UpdateContentsFlag);
    virtual bool isBackedByOpenGL() const { return true; }

#if ENABLE(CSS_FILTERS)
    virtual PassRefPtr<BitmapTexture> applyFilters(TextureMapper*, const FilterOperations&) override;
    struct FilterInfo {
        RefPtr<FilterOperation> filter;
        unsigned pass;
        RefPtr<BitmapTexture> contentTexture;

        FilterInfo(PassRefPtr<FilterOperation> f = 0, unsigned p = 0, PassRefPtr<BitmapTexture> t = 0)
            : filter(f)
            , pass(p)
            , contentTexture(t)
            { }
    };
    const FilterInfo* filterInfo() const { return &m_filterInfo; }
#endif

private:
    void updateContentsNoSwizzle(const void*, const IntRect& target, const IntPoint& sourceOffset, int bytesPerLine, unsigned bytesPerPixel = 4, Platform3DObject glFormat = GraphicsContext3D::RGBA);

    Platform3DObject m_id;
    IntSize m_textureSize;
    IntRect m_dirtyRect;
    Platform3DObject m_fbo;
    Platform3DObject m_rbo;
    Platform3DObject m_depthBufferObject;
    bool m_shouldClear;
    TextureMapperGL::ClipStack m_clipStack;
    RefPtr<GraphicsContext3D> m_context3D;

    explicit BitmapTextureGL(TextureMapperGL*);
    BitmapTextureGL();

    void clearIfNeeded();
    void createFboIfNeeded();

#if ENABLE(CSS_FILTERS)
    FilterInfo m_filterInfo;
#endif

    friend class TextureMapperGL;
};

BitmapTextureGL* toBitmapTextureGL(BitmapTexture*);

}
#endif

#endif
