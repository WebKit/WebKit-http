/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2012 ChangSeok Oh <shivamidow@gmail.com>
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#if USE(3D_GRAPHICS)

#include "GraphicsContext3D.h"

#if USE(OPENGL_ES_2)
#include "Extensions3DOpenGLES.h"
#else
#include "Extensions3DOpenGL.h"
#endif
#include "GraphicsContext.h"
#include "ImageBuffer.h"
#include "ImageData.h"
#include "IntRect.h"
#include "IntSize.h"
#include "NotImplemented.h"
#include <cstring>
#include <runtime/ArrayBuffer.h>
#include <runtime/ArrayBufferView.h>
#include <runtime/Float32Array.h>
#include <runtime/Int32Array.h>
#include <runtime/Uint8Array.h>
#include <wtf/MainThread.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/text/CString.h>

#if PLATFORM(QT)
#include <private/qopenglextensions_p.h>
#elif USE(OPENGL_ES_2)
#include "OpenGLESShims.h"
#elif PLATFORM(MAC)
#include <OpenGL/gl.h>
#elif PLATFORM(GTK) || PLATFORM(EFL) || PLATFORM(WIN)
#include "OpenGLShims.h"
#endif

#if PLATFORM(BLACKBERRY)
#include <BlackBerryPlatformLog.h>
#endif

namespace WebCore {

PassRefPtr<GraphicsContext3D> GraphicsContext3D::createForCurrentGLContext()
{
    RefPtr<GraphicsContext3D> context = adoptRef(new GraphicsContext3D(Attributes(), 0, GraphicsContext3D::RenderToCurrentGLContext));
    return context->m_private ? context.release() : 0;
}

void GraphicsContext3D::validateDepthStencil(const char* packedDepthStencilExtension)
{
    Extensions3D* extensions = getExtensions();
    if (m_attrs.stencil) {
        if (extensions->supports(packedDepthStencilExtension)) {
            extensions->ensureEnabled(packedDepthStencilExtension);
            // Force depth if stencil is true.
            m_attrs.depth = true;
        } else
            m_attrs.stencil = false;
    }
    if (m_attrs.antialias) {
        if (!extensions->maySupportMultisampling() || !extensions->supports("GL_ANGLE_framebuffer_multisample") || isGLES2Compliant())
            m_attrs.antialias = false;
        else
            extensions->ensureEnabled("GL_ANGLE_framebuffer_multisample");
    }
}

bool GraphicsContext3D::isResourceSafe()
{
    return false;
}

void GraphicsContext3D::paintRenderingResultsToCanvas(ImageBuffer* imageBuffer, DrawingBuffer*)
{
    int rowBytes = m_currentWidth * 4;
    int totalBytes = rowBytes * m_currentHeight;

    OwnArrayPtr<unsigned char> pixels = adoptArrayPtr(new unsigned char[totalBytes]);
    if (!pixels)
        return;

    readRenderingResults(pixels.get(), totalBytes);

    if (!m_attrs.premultipliedAlpha) {
        for (int i = 0; i < totalBytes; i += 4) {
            // Premultiply alpha.
            pixels[i + 0] = std::min(255, pixels[i + 0] * pixels[i + 3] / 255);
            pixels[i + 1] = std::min(255, pixels[i + 1] * pixels[i + 3] / 255);
            pixels[i + 2] = std::min(255, pixels[i + 2] * pixels[i + 3] / 255);
        }
    }

#if PLATFORM(BLACKBERRY) || USE(CG)
    paintToCanvas(pixels.get(), m_currentWidth, m_currentHeight,
                  imageBuffer->internalSize().width(), imageBuffer->internalSize().height(), imageBuffer->context());
#else
    paintToCanvas(pixels.get(), m_currentWidth, m_currentHeight,
                  imageBuffer->internalSize().width(), imageBuffer->internalSize().height(), imageBuffer->context()->platformContext());
#endif
}

bool GraphicsContext3D::paintCompositedResultsToCanvas(ImageBuffer*)
{
    // Not needed at the moment, so return that nothing was done.
    return false;
}

PassRefPtr<ImageData> GraphicsContext3D::paintRenderingResultsToImageData(DrawingBuffer*)
{
    // Reading premultiplied alpha would involve unpremultiplying, which is
    // lossy.
    if (m_attrs.premultipliedAlpha)
        return 0;

    RefPtr<ImageData> imageData = ImageData::create(IntSize(m_currentWidth, m_currentHeight));
    unsigned char* pixels = imageData->data()->data();
    int totalBytes = 4 * m_currentWidth * m_currentHeight;

    readRenderingResults(pixels, totalBytes);

    // Convert to RGBA.
    for (int i = 0; i < totalBytes; i += 4)
        std::swap(pixels[i], pixels[i + 2]);

    return imageData.release();
}

#if !PLATFORM(BLACKBERRY)
void GraphicsContext3D::prepareTexture()
{
    if (m_layerComposited)
        return;

    makeContextCurrent();
    if (m_attrs.antialias)
        resolveMultisamplingIfNecessary();

    m_functions->glBindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_fbo);
    m_functions->glActiveTexture(GL_TEXTURE0);
    m_functions->glBindTexture(GL_TEXTURE_2D, m_compositorTexture);
    m_functions->glCopyTexImage2D(GL_TEXTURE_2D, 0, m_internalColorFormat, 0, 0, m_currentWidth, m_currentHeight, 0);
    m_functions->glBindTexture(GL_TEXTURE_2D, m_state.boundTexture0);
    m_functions->glActiveTexture(m_state.activeTexture);
    if (m_state.boundFBO != m_fbo)
        m_functions->glBindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_state.boundFBO);
    m_functions->glFinish();
    m_layerComposited = true;
}
#endif

void GraphicsContext3D::readRenderingResults(unsigned char *pixels, int pixelsSize)
{
    if (pixelsSize < m_currentWidth * m_currentHeight * 4)
        return;

    makeContextCurrent();

    bool mustRestoreFBO = false;
    if (m_attrs.antialias) {
        resolveMultisamplingIfNecessary();
        m_functions->glBindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_fbo);
        mustRestoreFBO = true;
    } else {
        if (m_state.boundFBO != m_fbo) {
            mustRestoreFBO = true;
            m_functions->glBindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_fbo);
        }
    }

    GLint packAlignment = 4;
    bool mustRestorePackAlignment = false;
    m_functions->glGetIntegerv(GL_PACK_ALIGNMENT, &packAlignment);
    if (packAlignment > 4) {
        m_functions->glPixelStorei(GL_PACK_ALIGNMENT, 4);
        mustRestorePackAlignment = true;
    }

    readPixelsAndConvertToBGRAIfNecessary(0, 0, m_currentWidth, m_currentHeight, pixels);

    if (mustRestorePackAlignment)
        m_functions->glPixelStorei(GL_PACK_ALIGNMENT, packAlignment);

    if (mustRestoreFBO)
        m_functions->glBindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_state.boundFBO);
}

void GraphicsContext3D::reshape(int width, int height)
{
    if (!platformGraphicsContext3D())
        return;

    if (width == m_currentWidth && height == m_currentHeight)
        return;

#if (PLATFORM(QT) || PLATFORM(EFL)) && USE(GRAPHICS_SURFACE)
    m_functions->glFlush(); // Make sure all GL calls have been committed before resizing.
    createGraphicsSurfaces(IntSize(width, height));
#endif

    m_currentWidth = width;
    m_currentHeight = height;

    makeContextCurrent();
    validateAttributes();

    bool mustRestoreFBO = reshapeFBOs(IntSize(width, height));

    // Initialize renderbuffers to 0.
    GLfloat clearColor[] = {0, 0, 0, 0}, clearDepth = 0;
    GLint clearStencil = 0;
    GLboolean colorMask[] = {GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE}, depthMask = GL_TRUE;
    GLuint stencilMask = 0xffffffff;
    GLboolean isScissorEnabled = GL_FALSE;
    GLboolean isDitherEnabled = GL_FALSE;
    GLbitfield clearMask = GL_COLOR_BUFFER_BIT;
    m_functions->glGetFloatv(GL_COLOR_CLEAR_VALUE, clearColor);
    m_functions->glClearColor(0, 0, 0, 0);
    m_functions->glGetBooleanv(GL_COLOR_WRITEMASK, colorMask);
    m_functions->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    if (m_attrs.depth) {
        m_functions->glGetFloatv(GL_DEPTH_CLEAR_VALUE, &clearDepth);
        GraphicsContext3D::clearDepth(1);
        m_functions->glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
        m_functions->glDepthMask(GL_TRUE);
        clearMask |= GL_DEPTH_BUFFER_BIT;
    }
    if (m_attrs.stencil) {
        m_functions->glGetIntegerv(GL_STENCIL_CLEAR_VALUE, &clearStencil);
        m_functions->glClearStencil(0);
        m_functions->glGetIntegerv(GL_STENCIL_WRITEMASK, reinterpret_cast<GLint*>(&stencilMask));
        m_functions->glStencilMaskSeparate(GL_FRONT, 0xffffffff);
        clearMask |= GL_STENCIL_BUFFER_BIT;
    }
    isScissorEnabled = m_functions->glIsEnabled(GL_SCISSOR_TEST);
    m_functions->glDisable(GL_SCISSOR_TEST);
    isDitherEnabled = m_functions->glIsEnabled(GL_DITHER);
    m_functions->glDisable(GL_DITHER);

    m_functions->glClear(clearMask);

    m_functions->glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
    m_functions->glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
    if (m_attrs.depth) {
        GraphicsContext3D::clearDepth(clearDepth);
        m_functions->glDepthMask(depthMask);
    }
    if (m_attrs.stencil) {
        m_functions->glClearStencil(clearStencil);
        m_functions->glStencilMaskSeparate(GL_FRONT, stencilMask);
    }
    if (isScissorEnabled)
        m_functions->glEnable(GL_SCISSOR_TEST);
    else
        m_functions->glDisable(GL_SCISSOR_TEST);
    if (isDitherEnabled)
        m_functions->glEnable(GL_DITHER);
    else
        m_functions->glDisable(GL_DITHER);

    if (mustRestoreFBO)
        m_functions->glBindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_state.boundFBO);

    m_functions->glFlush();
}

IntSize GraphicsContext3D::getInternalFramebufferSize() const
{
    return IntSize(m_currentWidth, m_currentHeight);
}

void GraphicsContext3D::activeTexture(GC3Denum texture)
{
    makeContextCurrent();
    m_state.activeTexture = texture;
    m_functions->glActiveTexture(texture);
}

void GraphicsContext3D::attachShader(Platform3DObject program, Platform3DObject shader)
{
    ASSERT(program);
    ASSERT(shader);
    makeContextCurrent();
    m_functions->glAttachShader(program, shader);
}

void GraphicsContext3D::bindAttribLocation(Platform3DObject program, GC3Duint index, const String& name)
{
    ASSERT(program);
    makeContextCurrent();
    m_functions->glBindAttribLocation(program, index, name.utf8().data());
}

void GraphicsContext3D::bindBuffer(GC3Denum target, Platform3DObject buffer)
{
    makeContextCurrent();
    m_functions->glBindBuffer(target, buffer);
}

void GraphicsContext3D::bindFramebuffer(GC3Denum target, Platform3DObject buffer)
{
    makeContextCurrent();
    GLuint fbo;
    if (buffer)
        fbo = buffer;
    else
#if PLATFORM(BLACKBERRY)
        fbo = m_fbo;
#else
        fbo = (m_attrs.antialias ? m_multisampleFBO : m_fbo);
#endif
    if (fbo != m_state.boundFBO) {
        m_functions->glBindFramebuffer(target, fbo);
        m_state.boundFBO = fbo;
    }
}

void GraphicsContext3D::bindRenderbuffer(GC3Denum target, Platform3DObject renderbuffer)
{
    makeContextCurrent();
    m_functions->glBindRenderbuffer(target, renderbuffer);
}


void GraphicsContext3D::bindTexture(GC3Denum target, Platform3DObject texture)
{
    makeContextCurrent();
    if (m_state.activeTexture == GL_TEXTURE0 && target == GL_TEXTURE_2D)
        m_state.boundTexture0 = texture;
    m_functions->glBindTexture(target, texture);
}

void GraphicsContext3D::blendColor(GC3Dclampf red, GC3Dclampf green, GC3Dclampf blue, GC3Dclampf alpha)
{
    makeContextCurrent();
    m_functions->glBlendColor(red, green, blue, alpha);
}

void GraphicsContext3D::blendEquation(GC3Denum mode)
{
    makeContextCurrent();
    m_functions->glBlendEquation(mode);
}

void GraphicsContext3D::blendEquationSeparate(GC3Denum modeRGB, GC3Denum modeAlpha)
{
    makeContextCurrent();
    m_functions->glBlendEquationSeparate(modeRGB, modeAlpha);
}


void GraphicsContext3D::blendFunc(GC3Denum sfactor, GC3Denum dfactor)
{
    makeContextCurrent();
    m_functions->glBlendFunc(sfactor, dfactor);
}       

void GraphicsContext3D::blendFuncSeparate(GC3Denum srcRGB, GC3Denum dstRGB, GC3Denum srcAlpha, GC3Denum dstAlpha)
{
    makeContextCurrent();
    m_functions->glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

void GraphicsContext3D::bufferData(GC3Denum target, GC3Dsizeiptr size, GC3Denum usage)
{
    makeContextCurrent();
    m_functions->glBufferData(target, size, 0, usage);
}

void GraphicsContext3D::bufferData(GC3Denum target, GC3Dsizeiptr size, const void* data, GC3Denum usage)
{
    makeContextCurrent();
    m_functions->glBufferData(target, size, data, usage);
}

void GraphicsContext3D::bufferSubData(GC3Denum target, GC3Dintptr offset, GC3Dsizeiptr size, const void* data)
{
    makeContextCurrent();
    m_functions->glBufferSubData(target, offset, size, data);
}

GC3Denum GraphicsContext3D::checkFramebufferStatus(GC3Denum target)
{
    makeContextCurrent();
    return m_functions->glCheckFramebufferStatus(target);
}

void GraphicsContext3D::clearColor(GC3Dclampf r, GC3Dclampf g, GC3Dclampf b, GC3Dclampf a)
{
    makeContextCurrent();
    m_functions->glClearColor(r, g, b, a);
}

void GraphicsContext3D::clear(GC3Dbitfield mask)
{
    makeContextCurrent();
    m_functions->glClear(mask);
}

void GraphicsContext3D::clearStencil(GC3Dint s)
{
    makeContextCurrent();
    m_functions->glClearStencil(s);
}

void GraphicsContext3D::colorMask(GC3Dboolean red, GC3Dboolean green, GC3Dboolean blue, GC3Dboolean alpha)
{
    makeContextCurrent();
    m_functions->glColorMask(red, green, blue, alpha);
}

void GraphicsContext3D::compileShader(Platform3DObject shader)
{
    ASSERT(shader);
    makeContextCurrent();

    String translatedShaderSource = m_extensions->getTranslatedShaderSourceANGLE(shader);

    if (!translatedShaderSource.length())
        return;

    const CString& translatedShaderCString = translatedShaderSource.utf8();
    const char* translatedShaderPtr = translatedShaderCString.data();
    int translatedShaderLength = translatedShaderCString.length();
    
    m_functions->glShaderSource(shader, 1, &translatedShaderPtr, &translatedShaderLength);

    m_functions->glCompileShader(shader);
    
    int GLCompileSuccess;
    
    m_functions->glGetShaderiv(shader, COMPILE_STATUS, &GLCompileSuccess);

    // Populate the shader log
    GLint length = 0;
    m_functions->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

    if (length) {
        ShaderSourceMap::iterator result = m_shaderSourceMap.find(shader);
        GraphicsContext3D::ShaderSourceEntry& entry = result->value;

        GLsizei size = 0;
        OwnArrayPtr<GLchar> info = adoptArrayPtr(new GLchar[length]);
        m_functions->glGetShaderInfoLog(shader, length, &size, info.get());

        entry.log = info.get();
    }
    
    // ASSERT that ANGLE generated GLSL will be accepted by OpenGL.
    ASSERT(GLCompileSuccess == GL_TRUE);
#if PLATFORM(BLACKBERRY) && !defined(NDEBUG)
    if (GLCompileSuccess != GL_TRUE)
        BBLOG(BlackBerry::Platform::LogLevelWarn, "The shader validated, but didn't compile.\n");
#endif
}

void GraphicsContext3D::copyTexImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height, GC3Dint border)
{
    makeContextCurrent();
#if !PLATFORM(BLACKBERRY)
    if (m_attrs.antialias && m_state.boundFBO == m_multisampleFBO) {
        resolveMultisamplingIfNecessary(IntRect(x, y, width, height));
        m_functions->glBindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_fbo);
    }
#endif
    m_functions->glCopyTexImage2D(target, level, internalformat, x, y, width, height, border);
#if !PLATFORM(BLACKBERRY)
    if (m_attrs.antialias && m_state.boundFBO == m_multisampleFBO)
        m_functions->glBindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_multisampleFBO);
#endif
}

void GraphicsContext3D::copyTexSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset, GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height)
{
    makeContextCurrent();
#if !PLATFORM(BLACKBERRY)
    if (m_attrs.antialias && m_state.boundFBO == m_multisampleFBO) {
        resolveMultisamplingIfNecessary(IntRect(x, y, width, height));
        m_functions->glBindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_fbo);
    }
#endif
    m_functions->glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
#if !PLATFORM(BLACKBERRY)
    if (m_attrs.antialias && m_state.boundFBO == m_multisampleFBO)
        m_functions->glBindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_multisampleFBO);
#endif
}

void GraphicsContext3D::cullFace(GC3Denum mode)
{
    makeContextCurrent();
    m_functions->glCullFace(mode);
}

void GraphicsContext3D::depthFunc(GC3Denum func)
{
    makeContextCurrent();
    m_functions->glDepthFunc(func);
}

void GraphicsContext3D::depthMask(GC3Dboolean flag)
{
    makeContextCurrent();
    m_functions->glDepthMask(flag);
}

void GraphicsContext3D::detachShader(Platform3DObject program, Platform3DObject shader)
{
    ASSERT(program);
    ASSERT(shader);
    makeContextCurrent();
    m_functions->glDetachShader(program, shader);
}

void GraphicsContext3D::disable(GC3Denum cap)
{
    makeContextCurrent();
    m_functions->glDisable(cap);
}

void GraphicsContext3D::disableVertexAttribArray(GC3Duint index)
{
    makeContextCurrent();
    m_functions->glDisableVertexAttribArray(index);
}

void GraphicsContext3D::drawArrays(GC3Denum mode, GC3Dint first, GC3Dsizei count)
{
    makeContextCurrent();
    m_functions->glDrawArrays(mode, first, count);
}

void GraphicsContext3D::drawElements(GC3Denum mode, GC3Dsizei count, GC3Denum type, GC3Dintptr offset)
{
    makeContextCurrent();
    m_functions->glDrawElements(mode, count, type, reinterpret_cast<GLvoid*>(static_cast<intptr_t>(offset)));
}

void GraphicsContext3D::enable(GC3Denum cap)
{
    makeContextCurrent();
    m_functions->glEnable(cap);
}

void GraphicsContext3D::enableVertexAttribArray(GC3Duint index)
{
    makeContextCurrent();
    m_functions->glEnableVertexAttribArray(index);
}

void GraphicsContext3D::finish()
{
    makeContextCurrent();
    m_functions->glFinish();
}

void GraphicsContext3D::flush()
{
    makeContextCurrent();
    m_functions->glFlush();
}

void GraphicsContext3D::framebufferRenderbuffer(GC3Denum target, GC3Denum attachment, GC3Denum renderbuffertarget, Platform3DObject buffer)
{
    makeContextCurrent();
    m_functions->glFramebufferRenderbuffer(target, attachment, renderbuffertarget, buffer);
}

void GraphicsContext3D::framebufferTexture2D(GC3Denum target, GC3Denum attachment, GC3Denum textarget, Platform3DObject texture, GC3Dint level)
{
    makeContextCurrent();
    m_functions->glFramebufferTexture2D(target, attachment, textarget, texture, level);
}

void GraphicsContext3D::frontFace(GC3Denum mode)
{
    makeContextCurrent();
    m_functions->glFrontFace(mode);
}

void GraphicsContext3D::generateMipmap(GC3Denum target)
{
    makeContextCurrent();
    m_functions->glGenerateMipmap(target);
}

bool GraphicsContext3D::getActiveAttrib(Platform3DObject program, GC3Duint index, ActiveInfo& info)
{
    if (!program) {
        synthesizeGLError(INVALID_VALUE);
        return false;
    }
    makeContextCurrent();
    GLint maxAttributeSize = 0;
    m_functions->glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxAttributeSize);
    OwnArrayPtr<GLchar> name = adoptArrayPtr(new GLchar[maxAttributeSize]); // GL_ACTIVE_ATTRIBUTE_MAX_LENGTH includes null termination.
    GLsizei nameLength = 0;
    GLint size = 0;
    GLenum type = 0;
    m_functions->glGetActiveAttrib(program, index, maxAttributeSize, &nameLength, &size, &type, name.get());
    if (!nameLength)
        return false;
    
    String originalName = originalSymbolName(program, SHADER_SYMBOL_TYPE_ATTRIBUTE, String(name.get(), nameLength));
    
    info.name = originalName;
    info.type = type;
    info.size = size;
    return true;
}

bool GraphicsContext3D::getActiveUniform(Platform3DObject program, GC3Duint index, ActiveInfo& info)
{
    if (!program) {
        synthesizeGLError(INVALID_VALUE);
        return false;
    }

    makeContextCurrent();
    GLint maxUniformSize = 0;
    m_functions->glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxUniformSize);

    OwnArrayPtr<GLchar> name = adoptArrayPtr(new GLchar[maxUniformSize]); // GL_ACTIVE_UNIFORM_MAX_LENGTH includes null termination.
    GLsizei nameLength = 0;
    GLint size = 0;
    GLenum type = 0;
    m_functions->glGetActiveUniform(program, index, maxUniformSize, &nameLength, &size, &type, name.get());
    if (!nameLength)
        return false;
    
    String originalName = originalSymbolName(program, SHADER_SYMBOL_TYPE_UNIFORM, String(name.get(), nameLength));
    
    info.name = originalName;
    info.type = type;
    info.size = size;
    return true;
}
    
void GraphicsContext3D::getAttachedShaders(Platform3DObject program, GC3Dsizei maxCount, GC3Dsizei* count, Platform3DObject* shaders)
{
    if (!program) {
        synthesizeGLError(INVALID_VALUE);
        return;
    }
    makeContextCurrent();
    m_functions->glGetAttachedShaders(program, maxCount, count, shaders);
}

String GraphicsContext3D::mappedSymbolName(Platform3DObject program, ANGLEShaderSymbolType symbolType, const String& name)
{
    GC3Dsizei count;
    Platform3DObject shaders[2];
    getAttachedShaders(program, 2, &count, shaders);

    for (GC3Dsizei i = 0; i < count; ++i) {
        ShaderSourceMap::iterator result = m_shaderSourceMap.find(shaders[i]);
        if (result == m_shaderSourceMap.end())
            continue;

        const ShaderSymbolMap& symbolMap = result->value.symbolMap(symbolType);
        ShaderSymbolMap::const_iterator symbolEntry = symbolMap.find(name);
        if (symbolEntry != symbolMap.end())
            return symbolEntry->value.mappedName;
    }
    return name;
}
    
String GraphicsContext3D::originalSymbolName(Platform3DObject program, ANGLEShaderSymbolType symbolType, const String& name)
{
    GC3Dsizei count;
    Platform3DObject shaders[2];
    getAttachedShaders(program, 2, &count, shaders);
    
    for (GC3Dsizei i = 0; i < count; ++i) {
        ShaderSourceMap::iterator result = m_shaderSourceMap.find(shaders[i]);
        if (result == m_shaderSourceMap.end())
            continue;
        
        const ShaderSymbolMap& symbolMap = result->value.symbolMap(symbolType);
        ShaderSymbolMap::const_iterator symbolEntry;
        for (symbolEntry = symbolMap.begin(); symbolEntry != symbolMap.end(); ++symbolEntry) {
            if (symbolEntry->value.mappedName == name)
                return symbolEntry->key;
        }
    }
    return name;
}

int GraphicsContext3D::getAttribLocation(Platform3DObject program, const String& name)
{
    if (!program)
        return -1;

    makeContextCurrent();

    // The attribute name may have been translated during ANGLE compilation.
    // Look through the corresponding ShaderSourceMap to make sure we
    // reference the mapped name rather than the external name.
    String mappedName = mappedSymbolName(program, SHADER_SYMBOL_TYPE_ATTRIBUTE, name);

    return m_functions->glGetAttribLocation(program, mappedName.utf8().data());
}

GraphicsContext3D::Attributes GraphicsContext3D::getContextAttributes()
{
    return m_attrs;
}

GC3Denum GraphicsContext3D::getError()
{
    if (m_syntheticErrors.size() > 0) {
        ListHashSet<GC3Denum>::iterator iter = m_syntheticErrors.begin();
        GC3Denum err = *iter;
        m_syntheticErrors.remove(iter);
        return err;
    }

    makeContextCurrent();
    return m_functions->glGetError();
}

String GraphicsContext3D::getString(GC3Denum name)
{
    makeContextCurrent();
    return String(reinterpret_cast<const char*>(m_functions->glGetString(name)));
}

void GraphicsContext3D::hint(GC3Denum target, GC3Denum mode)
{
    makeContextCurrent();
    m_functions->glHint(target, mode);
}

GC3Dboolean GraphicsContext3D::isBuffer(Platform3DObject buffer)
{
    if (!buffer)
        return GL_FALSE;

    makeContextCurrent();
    return m_functions->glIsBuffer(buffer);
}

GC3Dboolean GraphicsContext3D::isEnabled(GC3Denum cap)
{
    makeContextCurrent();
    return m_functions->glIsEnabled(cap);
}

GC3Dboolean GraphicsContext3D::isFramebuffer(Platform3DObject framebuffer)
{
    if (!framebuffer)
        return GL_FALSE;

    makeContextCurrent();
    return m_functions->glIsFramebuffer(framebuffer);
}

GC3Dboolean GraphicsContext3D::isProgram(Platform3DObject program)
{
    if (!program)
        return GL_FALSE;

    makeContextCurrent();
    return m_functions->glIsProgram(program);
}

GC3Dboolean GraphicsContext3D::isRenderbuffer(Platform3DObject renderbuffer)
{
    if (!renderbuffer)
        return GL_FALSE;

    makeContextCurrent();
    return m_functions->glIsRenderbuffer(renderbuffer);
}

GC3Dboolean GraphicsContext3D::isShader(Platform3DObject shader)
{
    if (!shader)
        return GL_FALSE;

    makeContextCurrent();
    return m_functions->glIsShader(shader);
}

GC3Dboolean GraphicsContext3D::isTexture(Platform3DObject texture)
{
    if (!texture)
        return GL_FALSE;

    makeContextCurrent();
    return m_functions->glIsTexture(texture);
}

void GraphicsContext3D::lineWidth(GC3Dfloat width)
{
    makeContextCurrent();
    m_functions->glLineWidth(width);
}

void GraphicsContext3D::linkProgram(Platform3DObject program)
{
    ASSERT(program);
    makeContextCurrent();
    m_functions->glLinkProgram(program);
}

void GraphicsContext3D::pixelStorei(GC3Denum pname, GC3Dint param)
{
    makeContextCurrent();
    m_functions->glPixelStorei(pname, param);
}

void GraphicsContext3D::polygonOffset(GC3Dfloat factor, GC3Dfloat units)
{
    makeContextCurrent();
    m_functions->glPolygonOffset(factor, units);
}

void GraphicsContext3D::sampleCoverage(GC3Dclampf value, GC3Dboolean invert)
{
    makeContextCurrent();
    m_functions->glSampleCoverage(value, invert);
}

void GraphicsContext3D::scissor(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height)
{
    makeContextCurrent();
    m_functions->glScissor(x, y, width, height);
}

void GraphicsContext3D::shaderSource(Platform3DObject shader, const String& string)
{
    ASSERT(shader);

    makeContextCurrent();

    ShaderSourceEntry entry;

    entry.source = string;

    m_shaderSourceMap.set(shader, entry);
}

void GraphicsContext3D::stencilFunc(GC3Denum func, GC3Dint ref, GC3Duint mask)
{
    makeContextCurrent();
    m_functions->glStencilFunc(func, ref, mask);
}

void GraphicsContext3D::stencilFuncSeparate(GC3Denum face, GC3Denum func, GC3Dint ref, GC3Duint mask)
{
    makeContextCurrent();
    m_functions->glStencilFuncSeparate(face, func, ref, mask);
}

void GraphicsContext3D::stencilMask(GC3Duint mask)
{
    makeContextCurrent();
    m_functions->glStencilMask(mask);
}

void GraphicsContext3D::stencilMaskSeparate(GC3Denum face, GC3Duint mask)
{
    makeContextCurrent();
    m_functions->glStencilMaskSeparate(face, mask);
}

void GraphicsContext3D::stencilOp(GC3Denum fail, GC3Denum zfail, GC3Denum zpass)
{
    makeContextCurrent();
    m_functions->glStencilOp(fail, zfail, zpass);
}

void GraphicsContext3D::stencilOpSeparate(GC3Denum face, GC3Denum fail, GC3Denum zfail, GC3Denum zpass)
{
    makeContextCurrent();
    m_functions->glStencilOpSeparate(face, fail, zfail, zpass);
}

void GraphicsContext3D::texParameterf(GC3Denum target, GC3Denum pname, GC3Dfloat value)
{
    makeContextCurrent();
    m_functions->glTexParameterf(target, pname, value);
}

void GraphicsContext3D::texParameteri(GC3Denum target, GC3Denum pname, GC3Dint value)
{
    makeContextCurrent();
    m_functions->glTexParameteri(target, pname, value);
}

void GraphicsContext3D::uniform1f(GC3Dint location, GC3Dfloat v0)
{
    makeContextCurrent();
    m_functions->glUniform1f(location, v0);
}

void GraphicsContext3D::uniform1fv(GC3Dint location, GC3Dsizei size, GC3Dfloat* array)
{
    makeContextCurrent();
    m_functions->glUniform1fv(location, size, array);
}

void GraphicsContext3D::uniform2f(GC3Dint location, GC3Dfloat v0, GC3Dfloat v1)
{
    makeContextCurrent();
    m_functions->glUniform2f(location, v0, v1);
}

void GraphicsContext3D::uniform2fv(GC3Dint location, GC3Dsizei size, GC3Dfloat* array)
{
    // FIXME: length needs to be a multiple of 2.
    makeContextCurrent();
    m_functions->glUniform2fv(location, size, array);
}

void GraphicsContext3D::uniform3f(GC3Dint location, GC3Dfloat v0, GC3Dfloat v1, GC3Dfloat v2)
{
    makeContextCurrent();
    m_functions->glUniform3f(location, v0, v1, v2);
}

void GraphicsContext3D::uniform3fv(GC3Dint location, GC3Dsizei size, GC3Dfloat* array)
{
    // FIXME: length needs to be a multiple of 3.
    makeContextCurrent();
    m_functions->glUniform3fv(location, size, array);
}

void GraphicsContext3D::uniform4f(GC3Dint location, GC3Dfloat v0, GC3Dfloat v1, GC3Dfloat v2, GC3Dfloat v3)
{
    makeContextCurrent();
    m_functions->glUniform4f(location, v0, v1, v2, v3);
}

void GraphicsContext3D::uniform4fv(GC3Dint location, GC3Dsizei size, GC3Dfloat* array)
{
    // FIXME: length needs to be a multiple of 4.
    makeContextCurrent();
    m_functions->glUniform4fv(location, size, array);
}

void GraphicsContext3D::uniform1i(GC3Dint location, GC3Dint v0)
{
    makeContextCurrent();
    m_functions->glUniform1i(location, v0);
}

void GraphicsContext3D::uniform1iv(GC3Dint location, GC3Dsizei size, GC3Dint* array)
{
    makeContextCurrent();
    m_functions->glUniform1iv(location, size, array);
}

void GraphicsContext3D::uniform2i(GC3Dint location, GC3Dint v0, GC3Dint v1)
{
    makeContextCurrent();
    m_functions->glUniform2i(location, v0, v1);
}

void GraphicsContext3D::uniform2iv(GC3Dint location, GC3Dsizei size, GC3Dint* array)
{
    // FIXME: length needs to be a multiple of 2.
    makeContextCurrent();
    m_functions->glUniform2iv(location, size, array);
}

void GraphicsContext3D::uniform3i(GC3Dint location, GC3Dint v0, GC3Dint v1, GC3Dint v2)
{
    makeContextCurrent();
    m_functions->glUniform3i(location, v0, v1, v2);
}

void GraphicsContext3D::uniform3iv(GC3Dint location, GC3Dsizei size, GC3Dint* array)
{
    // FIXME: length needs to be a multiple of 3.
    makeContextCurrent();
    m_functions->glUniform3iv(location, size, array);
}

void GraphicsContext3D::uniform4i(GC3Dint location, GC3Dint v0, GC3Dint v1, GC3Dint v2, GC3Dint v3)
{
    makeContextCurrent();
    m_functions->glUniform4i(location, v0, v1, v2, v3);
}

void GraphicsContext3D::uniform4iv(GC3Dint location, GC3Dsizei size, GC3Dint* array)
{
    // FIXME: length needs to be a multiple of 4.
    makeContextCurrent();
    m_functions->glUniform4iv(location, size, array);
}

void GraphicsContext3D::uniformMatrix2fv(GC3Dint location, GC3Dsizei size, GC3Dboolean transpose, GC3Dfloat* array)
{
    // FIXME: length needs to be a multiple of 4.
    makeContextCurrent();
    m_functions->glUniformMatrix2fv(location, size, transpose, array);
}

void GraphicsContext3D::uniformMatrix3fv(GC3Dint location, GC3Dsizei size, GC3Dboolean transpose, GC3Dfloat* array)
{
    // FIXME: length needs to be a multiple of 9.
    makeContextCurrent();
    m_functions->glUniformMatrix3fv(location, size, transpose, array);
}

void GraphicsContext3D::uniformMatrix4fv(GC3Dint location, GC3Dsizei size, GC3Dboolean transpose, GC3Dfloat* array)
{
    // FIXME: length needs to be a multiple of 16.
    makeContextCurrent();
    m_functions->glUniformMatrix4fv(location, size, transpose, array);
}

void GraphicsContext3D::useProgram(Platform3DObject program)
{
    makeContextCurrent();
    m_functions->glUseProgram(program);
}

void GraphicsContext3D::validateProgram(Platform3DObject program)
{
    ASSERT(program);

    makeContextCurrent();
    m_functions->glValidateProgram(program);
}

void GraphicsContext3D::vertexAttrib1f(GC3Duint index, GC3Dfloat v0)
{
    makeContextCurrent();
    m_functions->glVertexAttrib1f(index, v0);
}

void GraphicsContext3D::vertexAttrib1fv(GC3Duint index, GC3Dfloat* array)
{
    makeContextCurrent();
    m_functions->glVertexAttrib1fv(index, array);
}

void GraphicsContext3D::vertexAttrib2f(GC3Duint index, GC3Dfloat v0, GC3Dfloat v1)
{
    makeContextCurrent();
    m_functions->glVertexAttrib2f(index, v0, v1);
}

void GraphicsContext3D::vertexAttrib2fv(GC3Duint index, GC3Dfloat* array)
{
    makeContextCurrent();
    m_functions->glVertexAttrib2fv(index, array);
}

void GraphicsContext3D::vertexAttrib3f(GC3Duint index, GC3Dfloat v0, GC3Dfloat v1, GC3Dfloat v2)
{
    makeContextCurrent();
    m_functions->glVertexAttrib3f(index, v0, v1, v2);
}

void GraphicsContext3D::vertexAttrib3fv(GC3Duint index, GC3Dfloat* array)
{
    makeContextCurrent();
    m_functions->glVertexAttrib3fv(index, array);
}

void GraphicsContext3D::vertexAttrib4f(GC3Duint index, GC3Dfloat v0, GC3Dfloat v1, GC3Dfloat v2, GC3Dfloat v3)
{
    makeContextCurrent();
    m_functions->glVertexAttrib4f(index, v0, v1, v2, v3);
}

void GraphicsContext3D::vertexAttrib4fv(GC3Duint index, GC3Dfloat* array)
{
    makeContextCurrent();
    m_functions->glVertexAttrib4fv(index, array);
}

void GraphicsContext3D::vertexAttribPointer(GC3Duint index, GC3Dint size, GC3Denum type, GC3Dboolean normalized, GC3Dsizei stride, GC3Dintptr offset)
{
    makeContextCurrent();
    m_functions->glVertexAttribPointer(index, size, type, normalized, stride, reinterpret_cast<GLvoid*>(static_cast<intptr_t>(offset)));
}

void GraphicsContext3D::viewport(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height)
{
    makeContextCurrent();
    m_functions->glViewport(x, y, width, height);
}

void GraphicsContext3D::getBooleanv(GC3Denum pname, GC3Dboolean* value)
{
    makeContextCurrent();
    m_functions->glGetBooleanv(pname, value);
}

void GraphicsContext3D::getBufferParameteriv(GC3Denum target, GC3Denum pname, GC3Dint* value)
{
    makeContextCurrent();
    m_functions->glGetBufferParameteriv(target, pname, value);
}

void GraphicsContext3D::getFloatv(GC3Denum pname, GC3Dfloat* value)
{
    makeContextCurrent();
    m_functions->glGetFloatv(pname, value);
}

void GraphicsContext3D::getFramebufferAttachmentParameteriv(GC3Denum target, GC3Denum attachment, GC3Denum pname, GC3Dint* value)
{
    makeContextCurrent();
    if (attachment == DEPTH_STENCIL_ATTACHMENT)
        attachment = DEPTH_ATTACHMENT; // Or STENCIL_ATTACHMENT, either works.
    m_functions->glGetFramebufferAttachmentParameteriv(target, attachment, pname, value);
}

void GraphicsContext3D::getProgramiv(Platform3DObject program, GC3Denum pname, GC3Dint* value)
{
    makeContextCurrent();
    m_functions->glGetProgramiv(program, pname, value);
}

String GraphicsContext3D::getProgramInfoLog(Platform3DObject program)
{
    ASSERT(program);

    makeContextCurrent();
    GLint length = 0;
    m_functions->glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    if (!length)
        return String(); 

    GLsizei size = 0;
    OwnArrayPtr<GLchar> info = adoptArrayPtr(new GLchar[length]);
    m_functions->glGetProgramInfoLog(program, length, &size, info.get());

    return String(info.get());
}

void GraphicsContext3D::getRenderbufferParameteriv(GC3Denum target, GC3Denum pname, GC3Dint* value)
{
    makeContextCurrent();
    m_functions->glGetRenderbufferParameteriv(target, pname, value);
}

void GraphicsContext3D::getShaderiv(Platform3DObject shader, GC3Denum pname, GC3Dint* value)
{
    ASSERT(shader);

    makeContextCurrent();

    ShaderSourceMap::iterator result = m_shaderSourceMap.find(shader);
    
    switch (pname) {
    case DELETE_STATUS:
    case SHADER_TYPE:
        m_functions->glGetShaderiv(shader, pname, value);
        break;
    case COMPILE_STATUS:
        if (result == m_shaderSourceMap.end()) {
            *value = static_cast<int>(false);
            return;
        }
        *value = static_cast<int>(result->value.isValid);
        break;
    case INFO_LOG_LENGTH:
        if (result == m_shaderSourceMap.end()) {
            *value = 0;
            return;
        }
        *value = getShaderInfoLog(shader).length();
        break;
    case SHADER_SOURCE_LENGTH:
        *value = getShaderSource(shader).length();
        break;
    default:
        synthesizeGLError(INVALID_ENUM);
    }
}

String GraphicsContext3D::getShaderInfoLog(Platform3DObject shader)
{
    ASSERT(shader);

    makeContextCurrent();

    ShaderSourceMap::iterator result = m_shaderSourceMap.find(shader);
    if (result == m_shaderSourceMap.end())
        return String(); 

    ShaderSourceEntry entry = result->value;
    if (!entry.isValid)
        return entry.log;

    GLint length = 0;
    m_functions->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    if (!length)
        return String(); 

    GLsizei size = 0;
    OwnArrayPtr<GLchar> info = adoptArrayPtr(new GLchar[length]);
    m_functions->glGetShaderInfoLog(shader, length, &size, info.get());

    return String(info.get());
}

String GraphicsContext3D::getShaderSource(Platform3DObject shader)
{
    ASSERT(shader);

    makeContextCurrent();

    ShaderSourceMap::iterator result = m_shaderSourceMap.find(shader);
    if (result == m_shaderSourceMap.end())
        return String(); 

    return result->value.source;
}


void GraphicsContext3D::getTexParameterfv(GC3Denum target, GC3Denum pname, GC3Dfloat* value)
{
    makeContextCurrent();
    m_functions->glGetTexParameterfv(target, pname, value);
}

void GraphicsContext3D::getTexParameteriv(GC3Denum target, GC3Denum pname, GC3Dint* value)
{
    makeContextCurrent();
    m_functions->glGetTexParameteriv(target, pname, value);
}

void GraphicsContext3D::getUniformfv(Platform3DObject program, GC3Dint location, GC3Dfloat* value)
{
    makeContextCurrent();
    m_functions->glGetUniformfv(program, location, value);
}

void GraphicsContext3D::getUniformiv(Platform3DObject program, GC3Dint location, GC3Dint* value)
{
    makeContextCurrent();
    m_functions->glGetUniformiv(program, location, value);
}

GC3Dint GraphicsContext3D::getUniformLocation(Platform3DObject program, const String& name)
{
    ASSERT(program);

    makeContextCurrent();

    // The uniform name may have been translated during ANGLE compilation.
    // Look through the corresponding ShaderSourceMap to make sure we
    // reference the mapped name rather than the external name.
    String mappedName = mappedSymbolName(program, SHADER_SYMBOL_TYPE_UNIFORM, name);

    return m_functions->glGetUniformLocation(program, mappedName.utf8().data());
}

void GraphicsContext3D::getVertexAttribfv(GC3Duint index, GC3Denum pname, GC3Dfloat* value)
{
    makeContextCurrent();
    m_functions->glGetVertexAttribfv(index, pname, value);
}

void GraphicsContext3D::getVertexAttribiv(GC3Duint index, GC3Denum pname, GC3Dint* value)
{
    makeContextCurrent();
    m_functions->glGetVertexAttribiv(index, pname, value);
}

GC3Dsizeiptr GraphicsContext3D::getVertexAttribOffset(GC3Duint index, GC3Denum pname)
{
    makeContextCurrent();

    GLvoid* pointer = 0;
    m_functions->glGetVertexAttribPointerv(index, pname, &pointer);
    return static_cast<GC3Dsizeiptr>(reinterpret_cast<intptr_t>(pointer));
}

void GraphicsContext3D::texSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoff, GC3Dint yoff, GC3Dsizei width, GC3Dsizei height, GC3Denum format, GC3Denum type, const void* pixels)
{
    makeContextCurrent();

    // FIXME: we will need to deal with PixelStore params when dealing with image buffers that differ from the subimage size.
    m_functions->glTexSubImage2D(target, level, xoff, yoff, width, height, format, type, pixels);
}

void GraphicsContext3D::compressedTexImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height, GC3Dint border, GC3Dsizei imageSize, const void* data)
{
    makeContextCurrent();
    m_functions->glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
}

void GraphicsContext3D::compressedTexSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset, GC3Dsizei width, GC3Dsizei height, GC3Denum format, GC3Dsizei imageSize, const void* data)
{
    makeContextCurrent();
    m_functions->glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
}

Platform3DObject GraphicsContext3D::createBuffer()
{
    makeContextCurrent();
    GLuint o = 0;
    m_functions->glGenBuffers(1, &o);
    return o;
}

Platform3DObject GraphicsContext3D::createFramebuffer()
{
    makeContextCurrent();
    GLuint o = 0;
    m_functions->glGenFramebuffers(1, &o);
    return o;
}

Platform3DObject GraphicsContext3D::createProgram()
{
    makeContextCurrent();
    return m_functions->glCreateProgram();
}

Platform3DObject GraphicsContext3D::createRenderbuffer()
{
    makeContextCurrent();
    GLuint o = 0;
    m_functions->glGenRenderbuffers(1, &o);
    return o;
}

Platform3DObject GraphicsContext3D::createShader(GC3Denum type)
{
    makeContextCurrent();
    return m_functions->glCreateShader((type == FRAGMENT_SHADER) ? GL_FRAGMENT_SHADER : GL_VERTEX_SHADER);
}

Platform3DObject GraphicsContext3D::createTexture()
{
    makeContextCurrent();
    GLuint o = 0;
    m_functions->glGenTextures(1, &o);
    return o;
}

void GraphicsContext3D::deleteBuffer(Platform3DObject buffer)
{
    makeContextCurrent();
    m_functions->glDeleteBuffers(1, &buffer);
}

void GraphicsContext3D::deleteFramebuffer(Platform3DObject framebuffer)
{
    makeContextCurrent();
    if (framebuffer == m_state.boundFBO) {
        // Make sure the framebuffer is not going to be used for drawing
        // operations after it gets deleted.
        bindFramebuffer(FRAMEBUFFER, 0);
    }
    m_functions->glDeleteFramebuffers(1, &framebuffer);
}

void GraphicsContext3D::deleteProgram(Platform3DObject program)
{
    makeContextCurrent();
    m_functions->glDeleteProgram(program);
}

void GraphicsContext3D::deleteRenderbuffer(Platform3DObject renderbuffer)
{
    makeContextCurrent();
    m_functions->glDeleteRenderbuffers(1, &renderbuffer);
}

void GraphicsContext3D::deleteShader(Platform3DObject shader)
{
    makeContextCurrent();
    m_functions->glDeleteShader(shader);
}

void GraphicsContext3D::deleteTexture(Platform3DObject texture)
{
    makeContextCurrent();
    if (m_state.boundTexture0 == texture)
        m_state.boundTexture0 = 0;
    m_functions->glDeleteTextures(1, &texture);
}

void GraphicsContext3D::synthesizeGLError(GC3Denum error)
{
    m_syntheticErrors.add(error);
}

void GraphicsContext3D::markContextChanged()
{
    m_layerComposited = false;
}

void GraphicsContext3D::markLayerComposited()
{
    m_layerComposited = true;
}

bool GraphicsContext3D::layerComposited() const
{
    return m_layerComposited;
}

void GraphicsContext3D::texImage2DDirect(GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height, GC3Dint border, GC3Denum format, GC3Denum type, const void* pixels)
{
    makeContextCurrent();
    m_functions->glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}

}

#endif // USE(3D_GRAPHICS)
