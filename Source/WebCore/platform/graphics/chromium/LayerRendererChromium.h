/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef LayerRendererChromium_h
#define LayerRendererChromium_h

#if USE(ACCELERATED_COMPOSITING)

#include "TextureCopier.h"
#include "ThrottledTextureUploader.h"
#include "TrackingTextureAllocator.h"
#include "cc/CCRenderer.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class CCCheckerboardDrawQuad;
class CCDebugBorderDrawQuad;
class CCDrawQuad;
class CCIOSurfaceDrawQuad;
class CCRenderPassDrawQuad;
class CCSolidColorDrawQuad;
class CCStreamVideoDrawQuad;
class CCTextureDrawQuad;
class CCTileDrawQuad;
class CCVideoDrawQuad;
class GeometryBinding;
class GraphicsContext3D;
class LayerRendererGpuMemoryAllocationChangedCallbackAdapter;
class LayerRendererSwapBuffersCompleteCallbackAdapter;
class ScopedEnsureFramebufferAllocation;

enum TextureUploaderOption { ThrottledUploader, UnthrottledUploader };

// Class that handles drawing of composited render layers using GL.
class LayerRendererChromium : public CCRenderer {
    WTF_MAKE_NONCOPYABLE(LayerRendererChromium);
public:
    static PassOwnPtr<LayerRendererChromium> create(CCRendererClient*, PassRefPtr<GraphicsContext3D>, TextureUploaderOption);

    virtual ~LayerRendererChromium();

    virtual const LayerRendererCapabilities& capabilities() const OVERRIDE { return m_capabilities; }

    GraphicsContext3D* context();

    virtual void viewportChanged() OVERRIDE;

    const FloatQuad& sharedGeometryQuad() const { return m_sharedGeometryQuad; }

    virtual void beginDrawingFrame(const CCRenderPass* defaultRenderPass) OVERRIDE;
    virtual void drawRenderPass(const CCRenderPass*, const FloatRect& framebufferDamageRect) OVERRIDE;
    virtual void finishDrawingFrame() OVERRIDE;

    virtual void drawHeadsUpDisplay(ManagedTexture*, const IntSize& hudSize) OVERRIDE;

    // waits for rendering to finish
    virtual void finish() OVERRIDE;

    virtual void doNoOp() OVERRIDE;
    // puts backbuffer onscreen
    virtual bool swapBuffers(const IntRect& subBuffer) OVERRIDE;

    static void debugGLCall(GraphicsContext3D*, const char* command, const char* file, int line);

    const GeometryBinding* sharedGeometry() const { return m_sharedGeometry.get(); }

    virtual void getFramebufferPixels(void *pixels, const IntRect&) OVERRIDE;
    bool getFramebufferTexture(ManagedTexture*, const IntRect& deviceRect);

    virtual TextureManager* implTextureManager() const OVERRIDE { return m_implTextureManager.get(); }
    virtual TextureCopier* textureCopier() const OVERRIDE { return m_textureCopier.get(); }
    virtual TextureUploader* textureUploader() const OVERRIDE { return m_textureUploader.get(); }
    virtual TextureAllocator* implTextureAllocator() const OVERRIDE { return m_implTextureAllocator.get(); }
    virtual TextureAllocator* contentsTextureAllocator() const OVERRIDE { return m_contentsTextureAllocator.get(); }

    virtual void setScissorToRect(const IntRect&) OVERRIDE;

    virtual bool isContextLost() OVERRIDE;

    virtual void setVisible(bool) OVERRIDE;

    void drawTexturedQuad(const WebKit::WebTransformationMatrix& layerMatrix,
                          float width, float height, float opacity, const FloatQuad&,
                          int matrixLocation, int alphaLocation, int quadLocation);
    void copyTextureToFramebuffer(int textureId, const IntSize& bounds, const WebKit::WebTransformationMatrix& drawMatrix);

protected:
    friend class LayerRendererGpuMemoryAllocationChangedCallbackAdapter;
    void discardFramebuffer();
    void ensureFramebuffer();
    bool isFramebufferDiscarded() const { return m_isFramebufferDiscarded; }

    LayerRendererChromium(CCRendererClient*, PassRefPtr<GraphicsContext3D>, TextureUploaderOption);
    bool initialize();

private:
    static void toGLMatrix(float*, const WebKit::WebTransformationMatrix&);

    void drawQuad(const CCDrawQuad*);
    void drawCheckerboardQuad(const CCCheckerboardDrawQuad*);
    void drawDebugBorderQuad(const CCDebugBorderDrawQuad*);
    void drawBackgroundFilters(const CCRenderPassDrawQuad*, const WebKit::WebTransformationMatrix& deviceTransform);
    void drawRenderPassQuad(const CCRenderPassDrawQuad*);
    void drawSolidColorQuad(const CCSolidColorDrawQuad*);
    void drawStreamVideoQuad(const CCStreamVideoDrawQuad*);
    void drawTextureQuad(const CCTextureDrawQuad*);
    void drawIOSurfaceQuad(const CCIOSurfaceDrawQuad*);
    void drawTileQuad(const CCTileDrawQuad*);
    void drawVideoQuad(const CCVideoDrawQuad*);

    void copyPlaneToTexture(const CCVideoDrawQuad*, const void* plane, int index);
    bool copyFrameToTextures(const CCVideoDrawQuad*);
    void drawStreamTexture(const CCVideoDrawQuad*);
    void drawYUV(const CCVideoDrawQuad*);

    void setDrawFramebufferRect(const IntRect&, bool flipY);

    // The current drawing target is either a RenderPass or ManagedTexture. Use these functions to switch to a new drawing target.
    bool useRenderPass(const CCRenderPass*);
    bool useManagedTexture(ManagedTexture*, const IntRect& viewportRect);
    bool isCurrentRenderPass(const CCRenderPass*);

    bool bindFramebufferToTexture(ManagedTexture*, const IntRect& viewportRect);

    void clearRenderPass(const CCRenderPass*, const FloatRect& framebufferDamageRect);

    void releaseRenderPassTextures();

    bool makeContextCurrent();

    bool initializeSharedObjects();
    void cleanupSharedObjects();

    friend class LayerRendererSwapBuffersCompleteCallbackAdapter;
    void onSwapBuffersComplete();

    LayerRendererCapabilities m_capabilities;

    const CCRenderPass* m_currentRenderPass;
    ManagedTexture* m_currentManagedTexture;
    unsigned m_offscreenFramebufferId;

    OwnPtr<GeometryBinding> m_sharedGeometry;
    FloatQuad m_sharedGeometryQuad;

    // This block of bindings defines all of the programs used by the compositor itself.

    // Tiled layer shaders.
    typedef ProgramBinding<VertexShaderTile, FragmentShaderRGBATexAlpha> TileProgram;
    typedef ProgramBinding<VertexShaderTile, FragmentShaderRGBATexClampAlphaAA> TileProgramAA;
    typedef ProgramBinding<VertexShaderTile, FragmentShaderRGBATexClampSwizzleAlphaAA> TileProgramSwizzleAA;
    typedef ProgramBinding<VertexShaderTile, FragmentShaderRGBATexOpaque> TileProgramOpaque;
    typedef ProgramBinding<VertexShaderTile, FragmentShaderRGBATexSwizzleAlpha> TileProgramSwizzle;
    typedef ProgramBinding<VertexShaderTile, FragmentShaderRGBATexSwizzleOpaque> TileProgramSwizzleOpaque;
    typedef ProgramBinding<VertexShaderPosTex, FragmentShaderCheckerboard> TileCheckerboardProgram;

    // Render surface shaders.
    typedef ProgramBinding<VertexShaderPosTex, FragmentShaderRGBATexAlpha> RenderPassProgram;
    typedef ProgramBinding<VertexShaderPosTex, FragmentShaderRGBATexAlphaMask> RenderPassMaskProgram;
    typedef ProgramBinding<VertexShaderQuad, FragmentShaderRGBATexAlphaAA> RenderPassProgramAA;
    typedef ProgramBinding<VertexShaderQuad, FragmentShaderRGBATexAlphaMaskAA> RenderPassMaskProgramAA;

    // Texture shaders.
    typedef ProgramBinding<VertexShaderPosTexTransform, FragmentShaderRGBATexAlpha> TextureProgram;
    typedef ProgramBinding<VertexShaderPosTexTransform, FragmentShaderRGBATexFlipAlpha> TextureProgramFlip;
    typedef ProgramBinding<VertexShaderPosTexTransform, FragmentShaderRGBATexRectFlipAlpha> TextureIOSurfaceProgram;

    // Video shaders.
    typedef ProgramBinding<VertexShaderVideoTransform, FragmentShaderOESImageExternal> VideoStreamTextureProgram;
    typedef ProgramBinding<VertexShaderPosTexYUVStretch, FragmentShaderYUVVideo> VideoYUVProgram;

    // Special purpose / effects shaders.
    typedef ProgramBinding<VertexShaderPos, FragmentShaderColor> SolidColorProgram;

    // Debugging shaders.
    typedef ProgramBinding<VertexShaderPosTex, FragmentShaderRGBATexSwizzleAlpha> HeadsUpDisplayProgram;


    const TileProgram* tileProgram();
    const TileProgramOpaque* tileProgramOpaque();
    const TileProgramAA* tileProgramAA();
    const TileProgramSwizzle* tileProgramSwizzle();
    const TileProgramSwizzleOpaque* tileProgramSwizzleOpaque();
    const TileProgramSwizzleAA* tileProgramSwizzleAA();
    const TileCheckerboardProgram* tileCheckerboardProgram();

    const RenderPassProgram* renderPassProgram();
    const RenderPassProgramAA* renderPassProgramAA();
    const RenderPassMaskProgram* renderPassMaskProgram();
    const RenderPassMaskProgramAA* renderPassMaskProgramAA();

    const TextureProgram* textureProgram();
    const TextureProgramFlip* textureProgramFlip();
    const TextureIOSurfaceProgram* textureIOSurfaceProgram();

    const VideoYUVProgram* videoYUVProgram();
    const VideoStreamTextureProgram* videoStreamTextureProgram();

    const SolidColorProgram* solidColorProgram();

    const HeadsUpDisplayProgram* headsUpDisplayProgram();

    OwnPtr<TileProgram> m_tileProgram;
    OwnPtr<TileProgramOpaque> m_tileProgramOpaque;
    OwnPtr<TileProgramAA> m_tileProgramAA;
    OwnPtr<TileProgramSwizzle> m_tileProgramSwizzle;
    OwnPtr<TileProgramSwizzleOpaque> m_tileProgramSwizzleOpaque;
    OwnPtr<TileProgramSwizzleAA> m_tileProgramSwizzleAA;
    OwnPtr<TileCheckerboardProgram> m_tileCheckerboardProgram;

    OwnPtr<RenderPassProgram> m_renderPassProgram;
    OwnPtr<RenderPassProgramAA> m_renderPassProgramAA;
    OwnPtr<RenderPassMaskProgram> m_renderPassMaskProgram;
    OwnPtr<RenderPassMaskProgramAA> m_renderPassMaskProgramAA;

    OwnPtr<TextureProgram> m_textureProgram;
    OwnPtr<TextureProgramFlip> m_textureProgramFlip;
    OwnPtr<TextureIOSurfaceProgram> m_textureIOSurfaceProgram;

    OwnPtr<VideoYUVProgram> m_videoYUVProgram;
    OwnPtr<VideoStreamTextureProgram> m_videoStreamTextureProgram;

    OwnPtr<SolidColorProgram> m_solidColorProgram;
    OwnPtr<HeadsUpDisplayProgram> m_headsUpDisplayProgram;

    OwnPtr<TextureManager> m_implTextureManager;
    OwnPtr<AcceleratedTextureCopier> m_textureCopier;
    OwnPtr<TextureUploader> m_textureUploader;
    OwnPtr<TrackingTextureAllocator> m_contentsTextureAllocator;
    OwnPtr<TrackingTextureAllocator> m_implTextureAllocator;

    RefPtr<GraphicsContext3D> m_context;

    const CCRenderPass* m_defaultRenderPass;

    bool m_isViewportChanged;
    bool m_isFramebufferDiscarded;
    TextureUploaderOption m_textureUploaderSetting;
};


// Setting DEBUG_GL_CALLS to 1 will call glGetError() after almost every GL
// call made by the compositor. Useful for debugging rendering issues but
// will significantly degrade performance.
#define DEBUG_GL_CALLS 0

#if DEBUG_GL_CALLS && !defined ( NDEBUG )
#define GLC(context, x) (x, LayerRendererChromium::debugGLCall(&*context, #x, __FILE__, __LINE__))
#else
#define GLC(context, x) (x)
#endif


}

#endif // USE(ACCELERATED_COMPOSITING)

#endif
