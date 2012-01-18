/*
 * Copyright 2011 Adobe Systems Incorporated. All Rights Reserved.
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

#include "config.h"

#if ENABLE(CSS_SHADERS) && ENABLE(WEBGL)
#include "CustomFilterShader.h"
#include "GraphicsContext3D.h"

namespace WebCore {

#define SHADER(Src) (#Src)

String CustomFilterShader::defaultVertexShaderString()
{
    DEFINE_STATIC_LOCAL(String, vertexShaderString, SHADER(
        precision mediump float;
        attribute vec4 a_position;
        attribute vec2 a_texCoord;
        uniform mat4 u_projectionMatrix;
        varying vec2 v_texCoord;
        void main()
        {
            gl_Position = u_projectionMatrix * a_position;
            v_texCoord = a_texCoord;
        }
    ));
    return vertexShaderString;
}

String CustomFilterShader::defaultFragmentShaderString()
{
    DEFINE_STATIC_LOCAL(String, fragmentShaderString, SHADER(
        precision mediump float;
        varying vec2 v_texCoord;
        uniform sampler2D s_texture;
        void main()
        {
            gl_FragColor = texture2D(s_texture, v_texCoord);
        }
    ));
    return fragmentShaderString;
}

CustomFilterShader::CustomFilterShader(GraphicsContext3D* context, const String& vertexShaderString, const String& fragmentShaderString)
    : m_context(context)
    , m_vertexShaderString(!vertexShaderString.isNull() ? vertexShaderString : defaultVertexShaderString())
    , m_fragmentShaderString(!fragmentShaderString.isNull() ? fragmentShaderString : defaultFragmentShaderString())
    , m_program(0)
    , m_positionAttribLocation(-1)
    , m_texAttribLocation(-1)
    , m_meshAttribLocation(-1)
    , m_triangleAttribLocation(-1)
    , m_meshBoxLocation(-1)
    , m_projectionMatrixLocation(-1)
    , m_tileSizeLocation(-1)
    , m_meshSizeLocation(-1)
    , m_samplerLocation(-1)
    , m_samplerSizeLocation(-1)
    , m_contentSamplerLocation(-1)
    , m_isInitialized(false)
{
    Platform3DObject vertexShader = compileShader(GraphicsContext3D::VERTEX_SHADER, m_vertexShaderString);
    if (!vertexShader)
        return;
    
    Platform3DObject fragmentShader = compileShader(GraphicsContext3D::FRAGMENT_SHADER, m_fragmentShaderString);
    if (!fragmentShader) {
        m_context->deleteShader(vertexShader);
        return;
    }
    
    m_program = linkProgram(vertexShader, fragmentShader);
    
    m_context->deleteShader(vertexShader);
    m_context->deleteShader(fragmentShader);
    
    if (!m_program)
        return;
    
    initializeParameterLocations();
    
    m_isInitialized = true;
}

Platform3DObject CustomFilterShader::compileShader(GC3Denum shaderType, const String& shaderString)
{
    Platform3DObject shader = m_context->createShader(shaderType);
    m_context->shaderSource(shader, shaderString);
    m_context->compileShader(shader);
    
    int compiled = 0;
    m_context->getShaderiv(shader, GraphicsContext3D::COMPILE_STATUS, &compiled);
    if (!compiled) {
        // FIXME: This is an invalid shader. Throw some errors.
        // https://bugs.webkit.org/show_bug.cgi?id=74416
        m_context->deleteShader(shader);
        return 0;
    }
    
    return shader;
}

Platform3DObject CustomFilterShader::linkProgram(Platform3DObject vertexShader, Platform3DObject fragmentShader)
{
    Platform3DObject program = m_context->createProgram();
    m_context->attachShader(program, vertexShader);
    m_context->attachShader(program, fragmentShader);
    m_context->linkProgram(program);
    
    int linked = 0;
    m_context->getProgramiv(program, GraphicsContext3D::LINK_STATUS, &linked);
    if (!linked) {
        // FIXME: Invalid vertex/fragment shader combination. Throw some errors here.
        // https://bugs.webkit.org/show_bug.cgi?id=74416
        m_context->deleteProgram(program);
        return 0;
    }
    
    return program;
}

void CustomFilterShader::initializeParameterLocations()
{
    m_positionAttribLocation = m_context->getAttribLocation(m_program, "a_position");
    m_texAttribLocation = m_context->getAttribLocation(m_program, "a_texCoord");
    m_meshAttribLocation = m_context->getAttribLocation(m_program, "a_meshCoord");
    m_triangleAttribLocation = m_context->getAttribLocation(m_program, "a_triangleCoord");
    m_meshBoxLocation = m_context->getUniformLocation(m_program, "u_meshBox");
    m_tileSizeLocation = m_context->getUniformLocation(m_program, "u_tileSize");
    m_meshSizeLocation = m_context->getUniformLocation(m_program, "u_meshSize");
    m_projectionMatrixLocation = m_context->getUniformLocation(m_program, "u_projectionMatrix");
    m_samplerLocation = m_context->getUniformLocation(m_program, "s_texture");
    m_samplerSizeLocation = m_context->getUniformLocation(m_program, "s_textureSize");
    m_contentSamplerLocation = m_context->getUniformLocation(m_program, "s_contentTexture");
}

int CustomFilterShader::uniformLocationByName(const String& name)
{
    ASSERT(m_isInitialized);
    // FIXME: Improve this by caching the uniform locations.
    return m_context->getUniformLocation(m_program, name);
}
    
CustomFilterShader::~CustomFilterShader()
{
    if (m_program)
        m_context->deleteProgram(m_program);
}

} // namespace WebCore
#endif // ENABLE(CSS_SHADERS) && ENABLE(WEBGL)
