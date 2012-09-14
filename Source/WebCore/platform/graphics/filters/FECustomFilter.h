/*
 * Copyright (C) 2011 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER “AS IS” AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef FECustomFilter_h
#define FECustomFilter_h

#if ENABLE(CSS_SHADERS) && USE(3D_GRAPHICS)

#include "CustomFilterOperation.h"
#include "Filter.h"
#include "FilterEffect.h"
#include "GraphicsTypes3D.h"
#include <wtf/RefPtr.h>

namespace JSC {
class Uint8ClampedArray;
}

namespace WebCore {

class CachedShader;
class CustomFilterArrayParameter;
class CustomFilterCompiledProgram;
class CustomFilterGlobalContext;
class CustomFilterMesh;
class CustomFilterNumberParameter;
class CustomFilterProgram;
class CustomFilterTransformParameter;
class CustomFilterValidatedProgram;
class DrawingBuffer;
class GraphicsContext3D;
class IntSize;

class FECustomFilter : public FilterEffect {
public:
    static PassRefPtr<FECustomFilter> create(Filter*, CustomFilterGlobalContext*, PassRefPtr<CustomFilterValidatedProgram>, const CustomFilterParameterList&,
                   unsigned meshRows, unsigned meshColumns, CustomFilterOperation::MeshBoxType, 
                   CustomFilterOperation::MeshType);

    virtual void platformApplySoftware();
    virtual void dump();

    virtual TextStream& externalRepresentation(TextStream&, int indention) const;

private:
    FECustomFilter(Filter*, CustomFilterGlobalContext*, PassRefPtr<CustomFilterValidatedProgram>, const CustomFilterParameterList&,
                   unsigned meshRows, unsigned meshColumns, CustomFilterOperation::MeshBoxType, 
                   CustomFilterOperation::MeshType);
    ~FECustomFilter();
    
    bool applyShader();
    void clearShaderResult();
    bool initializeContext();
    
    enum CustomFilterDrawType {
        NEEDS_INPUT_TEXTURE,
        NO_INPUT_TEXTURE
    };
    bool prepareForDrawing(CustomFilterDrawType = NEEDS_INPUT_TEXTURE);

    void drawFilterMesh(Platform3DObject inputTexture);
    bool programNeedsInputTexture() const;
    bool ensureInputTexture();
    void uploadInputTexture(Uint8ClampedArray* srcPixelArray);
    bool resizeContextIfNeeded(const IntSize&);
    bool resizeContext(const IntSize&);

    bool canUseMultisampleBuffers() const;
    bool createMultisampleBuffer();
    bool resizeMultisampleBuffers(const IntSize&);
    void resolveMultisampleBuffer();
    void deleteMultisampleRenderBuffers();

    bool ensureFrameBuffer();
    void deleteRenderBuffers();

    void bindVertexAttribute(int attributeLocation, unsigned size, unsigned offset);
    void unbindVertexAttribute(int attributeLocation);
    void bindProgramArrayParameters(int uniformLocation, CustomFilterArrayParameter*);
    void bindProgramNumberParameters(int uniformLocation, CustomFilterNumberParameter*);
    void bindProgramTransformParameter(int uniformLocation, CustomFilterTransformParameter*);
    void bindProgramParameters();
    void bindProgramAndBuffers(Platform3DObject inputTexture);
    void unbindVertexAttributes();
    
    // No need to keep a reference here. It is owned by the RenderView.
    CustomFilterGlobalContext* m_globalContext;
    
    RefPtr<GraphicsContext3D> m_context;
    RefPtr<CustomFilterValidatedProgram> m_validatedProgram;
    RefPtr<CustomFilterCompiledProgram> m_compiledProgram;
    RefPtr<CustomFilterMesh> m_mesh;
    IntSize m_contextSize;

    Platform3DObject m_inputTexture;
    Platform3DObject m_frameBuffer;
    Platform3DObject m_depthBuffer;
    Platform3DObject m_destTexture;

    bool m_triedMultisampleBuffer;
    Platform3DObject m_multisampleFrameBuffer;
    Platform3DObject m_multisampleRenderBuffer;
    Platform3DObject m_multisampleDepthBuffer;

    RefPtr<CustomFilterProgram> m_program;
    CustomFilterParameterList m_parameters;

    unsigned m_meshRows;
    unsigned m_meshColumns;
    CustomFilterOperation::MeshType m_meshType;
};

} // namespace WebCore

#endif // ENABLE(CSS_SHADERS) && USE(3D_GRAPHICS)

#endif // FECustomFilter_h
