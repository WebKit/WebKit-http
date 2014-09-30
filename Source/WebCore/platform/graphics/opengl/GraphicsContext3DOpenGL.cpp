/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2012 ChangSeok Oh <shivamidow@gmail.com>
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 * Copyright (C) 2014 Digia Plc. and/or its subsidiary(-ies).
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

// Note this implementation serves a double role for Qt where it also handles OpenGLES.

#if USE(3D_GRAPHICS)

#include "GraphicsContext3D.h"

#include "Extensions3DOpenGL.h"
#include "IntRect.h"
#include "IntSize.h"
#include "NotImplemented.h"

#include <QOpenGLContext>
#include <private/qopenglextensions_p.h>

#ifndef GL_BGRA
#define GL_BGRA                         0x80E1
#endif

#ifndef GL_READ_FRAMEBUFFER
#define GL_READ_FRAMEBUFFER             0x8CA8
#endif

#ifndef GL_DRAW_FRAMEBUFFER
#define GL_DRAW_FRAMEBUFFER             0x8CA9
#endif

#ifndef GL_MAX_VARYING_FLOATS
#define GL_MAX_VARYING_FLOATS             0x8B4B
#endif

#ifndef GL_ALPHA16F_ARB
#define GL_ALPHA16F_ARB                   0x881C
#endif

#ifndef GL_LUMINANCE16F_ARB
#define GL_LUMINANCE16F_ARB               0x881E
#endif

#ifndef GL_LUMINANCE_ALPHA16F_ARB
#define GL_LUMINANCE_ALPHA16F_ARB         0x881F
#endif

#ifndef GL_HALF_FLOAT_OES
#define GL_HALF_FLOAT_OES                 0x8D61
#endif

namespace WebCore {

void GraphicsContext3D::releaseShaderCompiler()
{
    ASSERT(m_private);
    makeContextCurrent();
    m_functions->glReleaseShaderCompiler();
}

void GraphicsContext3D::readPixelsAndConvertToBGRAIfNecessary(int x, int y, int width, int height, unsigned char* pixels)
{
    ASSERT(m_private);
    bool readBGRA = !isGLES2Compliant() || platformGraphicsContext3D()->hasExtension("GL_EXT_read_format_bgra");

    if (readBGRA)
        m_functions->glReadPixels(x, y, width, height, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
    else
        m_functions->glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    int totalBytes = width * height * 4;
    if (!readBGRA) {
        for (int i = 0; i < totalBytes; i += 4)
            std::swap(pixels[i], pixels[i + 2]); // Convert to BGRA.
    }
}

void GraphicsContext3D::validateAttributes()
{
    if (isGLES2Compliant())
        validateDepthStencil("GL_OES_packed_depth_stencil");
    else
        validateDepthStencil("GL_EXT_packed_depth_stencil");

    if (m_attrs.antialias && isGLES2Compliant()) {
        if (!m_functions->hasOpenGLExtension(QOpenGLExtensions::FramebufferMultisample) || !m_functions->hasOpenGLExtension(QOpenGLExtensions::FramebufferBlit))
            m_attrs.antialias = false;
    }
}

bool GraphicsContext3D::reshapeFBOs(const IntSize& size)
{
    const int width = size.width();
    const int height = size.height();
    GLuint colorFormat = 0, pixelDataType = 0;
    if (m_attrs.alpha) {
        m_internalColorFormat = isGLES2Compliant() ? GL_RGBA : GL_RGBA8;
        colorFormat = GL_RGBA;
        pixelDataType = GL_UNSIGNED_BYTE;
    } else {
        m_internalColorFormat = isGLES2Compliant() ? GL_RGB : GL_RGB8;
        colorFormat = GL_RGB;
        pixelDataType = isGLES2Compliant() ? GL_UNSIGNED_SHORT_5_6_5 : GL_UNSIGNED_BYTE;
    }

    // We don't allow the logic where stencil is required and depth is not.
    bool supportPackedDepthStencilBuffer = false;
    if (m_attrs.stencil || m_attrs.depth)
        supportPackedDepthStencilBuffer = m_functions->hasOpenGLExtension(QOpenGLExtensions::PackedDepthStencil);

    bool mustRestoreFBO = false;

    // Resize multisample FBO.
    if (m_attrs.antialias && !isGLES2Compliant()) {
        GLint maxSampleCount;
        m_functions->glGetIntegerv(GL_MAX_SAMPLES, &maxSampleCount);
        GLint sampleCount = std::min(8, maxSampleCount);
        if (sampleCount > maxSampleCount)
            sampleCount = maxSampleCount;
        if (m_state.boundFBO != m_multisampleFBO) {
            m_functions->glBindFramebuffer(GL_FRAMEBUFFER, m_multisampleFBO);
            mustRestoreFBO = true;
        }
        m_functions->glBindRenderbuffer(GL_RENDERBUFFER, m_multisampleColorBuffer);
        m_functions->glRenderbufferStorageMultisample(GL_RENDERBUFFER, sampleCount, m_internalColorFormat, width, height);
        m_functions->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_multisampleColorBuffer);
        if (m_attrs.stencil || m_attrs.depth) {
            m_functions->glBindRenderbuffer(GL_RENDERBUFFER, m_multisampleDepthStencilBuffer);
            if (supportPackedDepthStencilBuffer)
                m_functions->glRenderbufferStorageMultisample(GL_RENDERBUFFER, sampleCount, GL_DEPTH24_STENCIL8, width, height);
            else
                m_functions->glRenderbufferStorageMultisample(GL_RENDERBUFFER, sampleCount, GL_DEPTH_COMPONENT, width, height);
            if (m_attrs.stencil)
                m_functions->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_multisampleDepthStencilBuffer);
            if (m_attrs.depth)
                m_functions->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_multisampleDepthStencilBuffer);
        }
        m_functions->glBindRenderbuffer(GL_RENDERBUFFER, 0);
        if (m_functions->glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            // FIXME: cleanup.
            notImplemented();
        }
    }

    // resize regular FBO
    if (m_state.boundFBO != m_fbo) {
        mustRestoreFBO = true;
        m_functions->glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    }

    m_functions->glBindTexture(GL_TEXTURE_2D, m_texture);
    m_functions->glTexImage2D(GL_TEXTURE_2D, 0, m_internalColorFormat, width, height, 0, colorFormat, pixelDataType, 0);
    m_functions->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);

    m_functions->glBindTexture(GL_TEXTURE_2D, m_compositorTexture);
    m_functions->glTexImage2D(GL_TEXTURE_2D, 0, m_internalColorFormat, width, height, 0, colorFormat, pixelDataType, 0);
    m_functions->glBindTexture(GL_TEXTURE_2D, 0);

    if (!m_attrs.antialias && (m_attrs.stencil || m_attrs.depth)) {
        // Use a 24 bit depth buffer where we know we have it.
        if (supportPackedDepthStencilBuffer || !isGLES2Compliant()) {
            m_functions->glBindRenderbuffer(GL_RENDERBUFFER, m_depthStencilBuffer);
            if (supportPackedDepthStencilBuffer)
                m_functions->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
            else
                m_functions->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
            if (m_attrs.stencil)
                m_functions->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthStencilBuffer);
            if (m_attrs.depth)
                m_functions->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthStencilBuffer);
            m_functions->glBindRenderbuffer(GL_RENDERBUFFER, 0);
        } else {
            if (m_attrs.stencil) {
                m_functions->glBindRenderbuffer(GL_RENDERBUFFER, m_stencilBuffer);
                m_functions->glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, width, height);
                m_functions->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_stencilBuffer);
            }
            if (m_attrs.depth) {
                m_functions->glBindRenderbuffer(GL_RENDERBUFFER, m_depthBuffer);
                m_functions->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
                m_functions->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthBuffer);
            }
            m_functions->glBindRenderbuffer(GL_RENDERBUFFER, 0);
        }
    }

    if (m_functions->glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        // FIXME: cleanup
        notImplemented();
    }

    if (m_attrs.antialias && !isGLES2Compliant()) {
        m_functions->glBindFramebuffer(GL_FRAMEBUFFER, m_multisampleFBO);
        if (m_state.boundFBO == m_multisampleFBO)
            mustRestoreFBO = false;
    }

    return mustRestoreFBO;
}

void GraphicsContext3D::resolveMultisamplingIfNecessary(const IntRect& rect)
{
    Q_ASSERT(m_private);
    if (!m_attrs.antialias)
        return;

    if (!isGLES2Compliant()) {
        m_functions->glBindFramebuffer(GL_READ_FRAMEBUFFER, m_multisampleFBO);
        m_functions->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);

        IntRect resolveRect = rect;
        if (rect.isEmpty())
            resolveRect = IntRect(0, 0, m_currentWidth, m_currentHeight);

        m_functions->glBlitFramebuffer(resolveRect.x(), resolveRect.y(), resolveRect.maxX(), resolveRect.maxY(), resolveRect.x(), resolveRect.y(), resolveRect.maxX(), resolveRect.maxY(), GL_COLOR_BUFFER_BIT, GL_LINEAR);
    } else {
        notImplemented();
    }
}

void GraphicsContext3D::renderbufferStorage(GC3Denum target, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height)
{
    makeContextCurrent();
    if (!isGLES2Compliant()) {
        switch (internalformat) {
        case DEPTH_STENCIL:
            internalformat = GL_DEPTH24_STENCIL8;
            break;
        case DEPTH_COMPONENT16:
            internalformat = GL_DEPTH_COMPONENT;
            break;
        case RGBA4:
        case RGB5_A1:
            internalformat = GL_RGBA;
            break;
        case RGB565:
            internalformat = GL_RGB;
            break;
        }
    }
    m_functions->glRenderbufferStorage(target, internalformat, width, height);
}

void GraphicsContext3D::getIntegerv(GC3Denum pname, GC3Dint* value)
{
    makeContextCurrent();
    if (!isGLES2Compliant()) {
        // Need to emulate MAX_FRAGMENT/VERTEX_UNIFORM_VECTORS and MAX_VARYING_VECTORS
        // because desktop GL's corresponding queries return the number of components
        // whereas GLES2 return the number of vectors (each vector has 4 components).
        // Therefore, the value returned by desktop GL needs to be divided by 4.
        switch (pname) {
        case MAX_FRAGMENT_UNIFORM_VECTORS:
            m_functions->glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, value);
            *value /= 4;
            break;
        case MAX_VERTEX_UNIFORM_VECTORS:
            m_functions->glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, value);
            *value /= 4;
            break;
        case MAX_VARYING_VECTORS:
            m_functions->glGetIntegerv(GL_MAX_VARYING_FLOATS, value);
            *value /= 4;
            break;
        default:
            m_functions->glGetIntegerv(pname, value);
        }
    } else {
        m_functions->glGetIntegerv(pname, value);
    }
}

void GraphicsContext3D::getShaderPrecisionFormat(GC3Denum shaderType, GC3Denum precisionType, GC3Dint* range, GC3Dint* precision)
{
    ASSERT(range);
    ASSERT(precision);

    makeContextCurrent();
    m_functions->glGetShaderPrecisionFormat(shaderType, precisionType, range, precision);
}

bool GraphicsContext3D::texImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height, GC3Dint border, GC3Denum format, GC3Denum type, const void* pixels)
{
    if (width && height && !pixels) {
        synthesizeGLError(INVALID_VALUE);
        return false;
    }

    GC3Denum openGLInternalFormat = internalformat;
    if (!isGLES2Compliant()) {
        if (type == GL_FLOAT) {
            if (format == GL_RGBA)
                openGLInternalFormat = GL_RGBA32F;
            else if (format == GL_RGB)
                openGLInternalFormat = GL_RGB32F;
        } else if (type == GL_HALF_FLOAT_OES) {
            if (format == GL_RGBA)
                openGLInternalFormat = GL_RGBA16F;
            else if (format == GL_RGB)
                openGLInternalFormat = GL_RGB16F;
            else if (format == GL_LUMINANCE)
                openGLInternalFormat = GL_LUMINANCE16F_ARB;
            else if (format == GL_ALPHA)
                openGLInternalFormat = GL_ALPHA16F_ARB;
            else if (format == GL_LUMINANCE_ALPHA)
                openGLInternalFormat = GL_LUMINANCE_ALPHA16F_ARB;
            type = GL_HALF_FLOAT;
        }
    }
    texImage2DDirect(target, level, openGLInternalFormat, width, height, border, format, type, pixels);
    return true;
}

void GraphicsContext3D::depthRange(GC3Dclampf zNear, GC3Dclampf zFar)
{
    makeContextCurrent();
    m_functions->glDepthRangef(zNear, zFar);
}

void GraphicsContext3D::clearDepth(GC3Dclampf depth)
{
    makeContextCurrent();
    m_functions->glClearDepthf(depth);
}

Extensions3D* GraphicsContext3D::getExtensions()
{
    if (!m_extensions)
        m_extensions = adoptPtr(new Extensions3DOpenGL(this));
    return m_extensions.get();
}

void GraphicsContext3D::readPixels(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height, GC3Denum format, GC3Denum type, void* data)
{
    ASSERT(m_private);
    // FIXME: remove the two glFlush calls when the driver bug is fixed, i.e.,
    // all previous rendering calls should be done before reading pixels.
    makeContextCurrent();
    m_functions->glFlush();
    if (m_attrs.antialias && m_state.boundFBO == m_multisampleFBO) {
        resolveMultisamplingIfNecessary(IntRect(x, y, width, height));
        m_functions->glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
        m_functions->glFlush();
    }
    m_functions->glReadPixels(x, y, width, height, format, type, data);
    if (m_attrs.antialias && m_state.boundFBO == m_multisampleFBO)
        m_functions->glBindFramebuffer(GL_FRAMEBUFFER, m_multisampleFBO);
}

}

#endif // USE(3D_GRAPHICS)
