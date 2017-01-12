/*
 * Copyright (C) 2010, 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
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

#if ENABLE(GRAPHICS_CONTEXT_3D)

#include "GraphicsContext3D.h"
#if PLATFORM(IOS)
#include "GraphicsContext3DIOS.h"
#endif

#include "Extensions3DOpenGL.h"
#include "IntRect.h"
#include "IntSize.h"
#include "NotImplemented.h"
#include "TemporaryOpenGLSetting.h"

#include <algorithm>
#include <cstring>
#include <wtf/MainThread.h>
#include <wtf/text/CString.h>

#if USE(ACCELERATE)
#include <Accelerate/Accelerate.h>
#endif

#if PLATFORM(IOS)
#import <OpenGLES/ES2/glext.h>
// From <OpenGLES/glext.h>
#define GL_RGBA32F_ARB                      0x8814
#define GL_RGB32F_ARB                       0x8815
#elif PLATFORM(MAC)
#include <OpenGL/gl.h>
#elif PLATFORM(GTK) || PLATFORM(EFL) || PLATFORM(WIN)
#include "OpenGLShims.h"
#endif

#if PLATFORM(QT)

#define FUNCTIONS m_functions
#include "OpenGLShimsQt.h"
#include <QOpenGLContext>

#define scopedScissor(c, s)     scopedScissor(m_functions, c, s)
#define scopedDither(c, s)      scopedDither(m_functions, c, s)
#define scopedDepth(c, s)       scopedDepth(m_functions, c, s)
#define scopedStencil(c, s)     scopedStencil(m_functions, c, s)

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

#endif

namespace WebCore {

void GraphicsContext3D::releaseShaderCompiler()
{
    makeContextCurrent();
#if PLATFORM(QT)
    ASSERT(m_private);
    m_functions->glReleaseShaderCompiler();
#else
    notImplemented();
#endif
}

void GraphicsContext3D::readPixelsAndConvertToBGRAIfNecessary(int x, int y, int width, int height, unsigned char* pixels)
{
    // NVIDIA drivers have a bug where calling readPixels in BGRA can return the wrong values for the alpha channel when the alpha is off for the context.
    if (!m_attrs.alpha && getExtensions()->isNVIDIA()) {
        ::glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
#if USE(ACCELERATE)
        vImage_Buffer src;
        src.height = height;
        src.width = width;
        src.rowBytes = width * 4;
        src.data = pixels;

        vImage_Buffer dest;
        dest.height = height;
        dest.width = width;
        dest.rowBytes = width * 4;
        dest.data = pixels;

        // Swap pixel channels from RGBA to BGRA.
        const uint8_t map[4] = { 2, 1, 0, 3 };
        vImagePermuteChannels_ARGB8888(&src, &dest, map, kvImageNoFlags);
#else
        int totalBytes = width * height * 4;
        for (int i = 0; i < totalBytes; i += 4)
            std::swap(pixels[i], pixels[i + 2]);
#endif
    } else {
#if PLATFORM(QT)
        ASSERT(m_private);
        bool readBGRA = !isGLES2Compliant() || platformGraphicsContext3D()->hasExtension("GL_EXT_read_format_bgra");

        if (readBGRA)
            glReadPixels(x, y, width, height, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
        else
            glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        int totalBytes = width * height * 4;
        if (!readBGRA) {
            for (int i = 0; i < totalBytes; i += 4)
                std::swap(pixels[i], pixels[i + 2]); // Convert to BGRA.
        }
#else
        ::glReadPixels(x, y, width, height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, pixels);
#endif
    }
}

void GraphicsContext3D::validateAttributes()
{
#if PLATFORM(QT)
    if (isGLES2Compliant())
        validateDepthStencil("GL_OES_packed_depth_stencil");
    else
        validateDepthStencil("GL_EXT_packed_depth_stencil");

    if (m_attrs.antialias && isGLES2Compliant()) {
        if (!m_functions->hasOpenGLExtension(QOpenGLExtensions::FramebufferMultisample) || !m_functions->hasOpenGLExtension(QOpenGLExtensions::FramebufferBlit))
            m_attrs.antialias = false;
    }
#else
    validateDepthStencil("GL_EXT_packed_depth_stencil");
#endif
}

bool GraphicsContext3D::reshapeFBOs(const IntSize& size)
{
    const int width = size.width();
    const int height = size.height();
    GLuint colorFormat, internalDepthStencilFormat = 0;
    GLuint pixelDataType = 0;
    if (m_attrs.alpha) {
        m_internalColorFormat = isGLES2Compliant() ? GL_RGBA : GL_RGBA8;
        colorFormat = GL_RGBA;
        pixelDataType = GL_UNSIGNED_BYTE;
    } else {
        m_internalColorFormat = isGLES2Compliant() ? GL_RGB : GL_RGB8;
        colorFormat = GL_RGB;
        pixelDataType = isGLES2Compliant() ? GL_UNSIGNED_SHORT_5_6_5 : GL_UNSIGNED_BYTE;
    }

    if (m_attrs.stencil || m_attrs.depth) {
        // We don't allow the logic where stencil is required and depth is not.
        // See GraphicsContext3D::validateAttributes.

        Extensions3D* extensions = getExtensions();
        // Use a 24 bit depth buffer where we know we have it.
        if (extensions->supports("GL_EXT_packed_depth_stencil"))
            internalDepthStencilFormat = GL_DEPTH24_STENCIL8_EXT;
        else
#if PLATFORM(IOS)
            internalDepthStencilFormat = GL_DEPTH_COMPONENT16;
#else
            internalDepthStencilFormat = GL_DEPTH_COMPONENT;
#endif
    }

    // Resize multisample FBO.
    if (m_attrs.antialias && !isGLES2Compliant()) {
        GLint maxSampleCount;
        ::glGetIntegerv(GL_MAX_SAMPLES_EXT, &maxSampleCount);
        GLint sampleCount = std::min(8, maxSampleCount);
        if (sampleCount > maxSampleCount)
            sampleCount = maxSampleCount;
        ::glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_multisampleFBO);
        ::glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, m_multisampleColorBuffer);
#if PLATFORM(IOS)
        ::glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, sampleCount, GL_RGBA8_OES, width, height);
#else
        ::glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, sampleCount, m_internalColorFormat, width, height);
#endif
        ::glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, m_multisampleColorBuffer);
        if (m_attrs.stencil || m_attrs.depth) {
            ::glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, m_multisampleDepthStencilBuffer);
            ::glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, sampleCount, internalDepthStencilFormat, width, height);
            if (m_attrs.stencil)
                ::glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, m_multisampleDepthStencilBuffer);
            if (m_attrs.depth)
                ::glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, m_multisampleDepthStencilBuffer);
        }
        ::glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
        if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT) {
            // FIXME: cleanup.
            notImplemented();
        }
    }

    // resize regular FBO
    ::glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fbo);
    ASSERT(m_texture);
#if PLATFORM(IOS)
    ::glBindRenderbuffer(GL_RENDERBUFFER, m_texture);
    ::glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_texture);
    setRenderbufferStorageFromDrawable(m_currentWidth, m_currentHeight);
#else
    ::glBindTexture(GL_TEXTURE_2D, m_texture);
    ::glTexImage2D(GL_TEXTURE_2D, 0, m_internalColorFormat, width, height, 0, colorFormat, pixelDataType, 0);
    ::glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_texture, 0);

    if (m_compositorTexture) {
        ::glBindTexture(GL_TEXTURE_2D, m_compositorTexture);
        ::glTexImage2D(GL_TEXTURE_2D, 0, m_internalColorFormat, width, height, 0, colorFormat, pixelDataType, 0);
        ::glBindTexture(GL_TEXTURE_2D, 0);
#if USE(COORDINATED_GRAPHICS_THREADED)
        ::glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_compositorFBO);
        ::glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_compositorTexture, 0);
        attachDepthAndStencilBufferIfNeeded(internalDepthStencilFormat, width, height);
        ::glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fbo);
#endif
    }
#endif

    attachDepthAndStencilBufferIfNeeded(internalDepthStencilFormat, width, height);

    bool mustRestoreFBO = true;
    if (m_attrs.antialias && !isGLES2Compliant()) {
        ::glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_multisampleFBO);
        if (m_state.boundFBO == m_multisampleFBO)
            mustRestoreFBO = false;
    } else {
        if (m_state.boundFBO == m_fbo)
            mustRestoreFBO = false;
    }

    return mustRestoreFBO;
}

void GraphicsContext3D::attachDepthAndStencilBufferIfNeeded(GLuint internalDepthStencilFormat, int width, int height)
{
    if (!m_attrs.antialias && (m_attrs.stencil || m_attrs.depth)) {
#if PLATFORM(QT)
        bool supportPackedDepthStencilBuffer = internalDepthStencilFormat == GL_DEPTH24_STENCIL8_EXT;
        if (supportPackedDepthStencilBuffer || !isGLES2Compliant()) {
#endif
        ::glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, m_depthStencilBuffer);
        ::glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, internalDepthStencilFormat, width, height);
        if (m_attrs.stencil)
            ::glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, m_depthStencilBuffer);
        if (m_attrs.depth)
            ::glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, m_depthStencilBuffer);
        ::glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
#if PLATFORM(QT)
        } else {
            if (m_attrs.stencil) {
                ::glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, m_stencilBuffer);
                ::glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_STENCIL_INDEX8, width, height);
                ::glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, m_stencilBuffer);
            }
            if (m_attrs.depth) {
                ::glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, m_depthBuffer);
                ::glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT16, width, height);
                ::glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, m_depthBuffer);
            }
            ::glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
        }
#endif
    }

    if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT) {
        // FIXME: cleanup
        notImplemented();
    }
}

void GraphicsContext3D::resolveMultisamplingIfNecessary(const IntRect& rect)
{
#if PLATFORM(QT)
    Q_ASSERT(m_private);
    if (!m_attrs.antialias)
        return;

    // QTFIXME: Probably not needed, iOS uses following code successfully
    if (isGLES2Compliant()) {
        notImplemented();
        return;
    }
#endif

    TemporaryOpenGLSetting scopedScissor(GL_SCISSOR_TEST, GL_FALSE);
    TemporaryOpenGLSetting scopedDither(GL_DITHER, GL_FALSE);
    TemporaryOpenGLSetting scopedDepth(GL_DEPTH_TEST, GL_FALSE);
    TemporaryOpenGLSetting scopedStencil(GL_STENCIL_TEST, GL_FALSE);

    GLint boundFrameBuffer;
    ::glGetIntegerv(GL_FRAMEBUFFER_BINDING, &boundFrameBuffer);

    ::glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, m_multisampleFBO);
    ::glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, m_fbo);
#if PLATFORM(IOS)
    UNUSED_PARAM(rect);
    ::glResolveMultisampleFramebufferAPPLE();
    const GLenum discards[] = { GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT };
    ::glDiscardFramebufferEXT(GL_READ_FRAMEBUFFER_APPLE, 2, discards);
    ::glBindFramebuffer(GL_FRAMEBUFFER, boundFrameBuffer);
#else
    IntRect resolveRect = rect;
    if (rect.isEmpty())
        resolveRect = IntRect(0, 0, m_currentWidth, m_currentHeight);

    ::glBlitFramebufferEXT(resolveRect.x(), resolveRect.y(), resolveRect.maxX(), resolveRect.maxY(), resolveRect.x(), resolveRect.y(), resolveRect.maxX(), resolveRect.maxY(), GL_COLOR_BUFFER_BIT, GL_LINEAR);
#endif
}

void GraphicsContext3D::renderbufferStorage(GC3Denum target, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height)
{
    makeContextCurrent();
#if !PLATFORM(IOS)
#if PLATFORM(QT)
    if (!isGLES2Compliant()) {
#endif
    switch (internalformat) {
    case DEPTH_STENCIL:
        internalformat = GL_DEPTH24_STENCIL8_EXT;
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
#if PLATFORM(QT)
    }
#endif
#endif
    ::glRenderbufferStorageEXT(target, internalformat, width, height);
}

void GraphicsContext3D::getIntegerv(GC3Denum pname, GC3Dint* value)
{
    // Need to emulate MAX_FRAGMENT/VERTEX_UNIFORM_VECTORS and MAX_VARYING_VECTORS
    // because desktop GL's corresponding queries return the number of components
    // whereas GLES2 return the number of vectors (each vector has 4 components).
    // Therefore, the value returned by desktop GL needs to be divided by 4.
    makeContextCurrent();
#if PLATFORM(QT)
    if (isGLES2Compliant()) {
        ::glGetIntegerv(pname, value);
        return;
    }
#endif
    switch (pname) {
#if !PLATFORM(IOS)
    case MAX_FRAGMENT_UNIFORM_VECTORS:
        ::glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, value);
        *value /= 4;
        break;
    case MAX_VERTEX_UNIFORM_VECTORS:
        ::glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, value);
        *value /= 4;
        break;
    case MAX_VARYING_VECTORS:
        ::glGetIntegerv(GL_MAX_VARYING_FLOATS, value);
        *value /= 4;
        break;
#endif
    case MAX_TEXTURE_SIZE:
        ::glGetIntegerv(MAX_TEXTURE_SIZE, value);
        if (getExtensions()->requiresRestrictedMaximumTextureSize())
            *value = std::min(4096, *value);
        break;
    case MAX_CUBE_MAP_TEXTURE_SIZE:
        ::glGetIntegerv(MAX_CUBE_MAP_TEXTURE_SIZE, value);
        if (getExtensions()->requiresRestrictedMaximumTextureSize())
            *value = std::min(1024, *value);
        break;
    default:
        ::glGetIntegerv(pname, value);
    }
}

void GraphicsContext3D::getShaderPrecisionFormat(GC3Denum shaderType, GC3Denum precisionType, GC3Dint* range, GC3Dint* precision)
{
#if !PLATFORM(QT)
    UNUSED_PARAM(shaderType);
#endif
    ASSERT(range);
    ASSERT(precision);

    makeContextCurrent();

#if PLATFORM(QT)
    m_functions->glGetShaderPrecisionFormat(shaderType, precisionType, range, precision);
    return;
#endif

    switch (precisionType) {
    case GraphicsContext3D::LOW_INT:
    case GraphicsContext3D::MEDIUM_INT:
    case GraphicsContext3D::HIGH_INT:
        // These values are for a 32-bit twos-complement integer format.
        range[0] = 31;
        range[1] = 30;
        precision[0] = 0;
        break;
    case GraphicsContext3D::LOW_FLOAT:
    case GraphicsContext3D::MEDIUM_FLOAT:
    case GraphicsContext3D::HIGH_FLOAT:
        // These values are for an IEEE single-precision floating-point format.
        range[0] = 127;
        range[1] = 127;
        precision[0] = 23;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }
}

bool GraphicsContext3D::texImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height, GC3Dint border, GC3Denum format, GC3Denum type, const void* pixels)
{
    if (width && height && !pixels) {
        synthesizeGLError(INVALID_VALUE);
        return false;
    }

    GC3Denum openGLFormat = format;
    GC3Denum openGLInternalFormat = internalformat;
#if !PLATFORM(IOS)
#if PLATFORM(QT)
    if (!isGLES2Compliant()) {
#endif
    if (type == GL_FLOAT) {
        if (format == GL_RGBA)
            openGLInternalFormat = GL_RGBA32F_ARB;
        else if (format == GL_RGB)
            openGLInternalFormat = GL_RGB32F_ARB;
    } else if (type == HALF_FLOAT_OES) {
        if (format == GL_RGBA)
            openGLInternalFormat = GL_RGBA16F_ARB;
        else if (format == GL_RGB)
            openGLInternalFormat = GL_RGB16F_ARB;
        else if (format == GL_LUMINANCE)
            openGLInternalFormat = GL_LUMINANCE16F_ARB;
        else if (format == GL_ALPHA)
            openGLInternalFormat = GL_ALPHA16F_ARB;
        else if (format == GL_LUMINANCE_ALPHA)
            openGLInternalFormat = GL_LUMINANCE_ALPHA16F_ARB;
        type = GL_HALF_FLOAT_ARB;
    }

    ASSERT(format != Extensions3D::SRGB8_ALPHA8_EXT);
    if (format == Extensions3D::SRGB_ALPHA_EXT)
        openGLFormat = GL_RGBA;
    else if (format == Extensions3D::SRGB_EXT)
        openGLFormat = GL_RGB;
#if PLATFORM(QT)
    }
#endif
#endif
    texImage2DDirect(target, level, openGLInternalFormat, width, height, border, openGLFormat, type, pixels);
    return true;
}

void GraphicsContext3D::depthRange(GC3Dclampf zNear, GC3Dclampf zFar)
{
    makeContextCurrent();
#if PLATFORM(IOS)
    ::glDepthRangef(static_cast<float>(zNear), static_cast<float>(zFar));
#else
    ::glDepthRange(zNear, zFar);
#endif
}

void GraphicsContext3D::clearDepth(GC3Dclampf depth)
{
    makeContextCurrent();
#if PLATFORM(IOS)
    ::glClearDepthf(static_cast<float>(depth));
#else
    ::glClearDepth(depth);
#endif
}

Extensions3D* GraphicsContext3D::getExtensions()
{
    if (!m_extensions)
        m_extensions = std::make_unique<Extensions3DOpenGL>(this);
    return m_extensions.get();
}

void GraphicsContext3D::readPixels(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height, GC3Denum format, GC3Denum type, void* data)
{
    ASSERT(m_private);
    // FIXME: remove the two glFlush calls when the driver bug is fixed, i.e.,
    // all previous rendering calls should be done before reading pixels.
    makeContextCurrent();
    ::glFlush();
    if (m_attrs.antialias && m_state.boundFBO == m_multisampleFBO) {
        resolveMultisamplingIfNecessary(IntRect(x, y, width, height));
        ::glBindFramebufferEXT(GraphicsContext3D::FRAMEBUFFER, m_fbo);
        ::glFlush();
    }
    ::glReadPixels(x, y, width, height, format, type, data);
    if (m_attrs.antialias && m_state.boundFBO == m_multisampleFBO)
        ::glBindFramebufferEXT(GraphicsContext3D::FRAMEBUFFER, m_multisampleFBO);
}

}

#endif // ENABLE(GRAPHICS_CONTEXT_3D)
