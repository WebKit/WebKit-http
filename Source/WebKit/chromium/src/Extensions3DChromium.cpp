/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#include "config.h"

#if ENABLE(WEBGL)

#include "Extensions3DChromium.h"

#include "GraphicsContext3D.h"
#include "GraphicsContext3DInternal.h"

namespace WebCore {

Extensions3DChromium::Extensions3DChromium(GraphicsContext3DInternal* internal)
    : m_internal(internal)
{
}

Extensions3DChromium::~Extensions3DChromium()
{
}

bool Extensions3DChromium::supports(const String& name)
{
    return m_internal->supportsExtension(name);
}

void Extensions3DChromium::ensureEnabled(const String& name)
{
#ifndef NDEBUG
    bool result =
#endif
        m_internal->ensureExtensionEnabled(name);
    ASSERT(result);
}

bool Extensions3DChromium::isEnabled(const String& name)
{
    return m_internal->isExtensionEnabled(name);
}

int Extensions3DChromium::getGraphicsResetStatusARB()
{
    return static_cast<int>(m_internal->getGraphicsResetStatusARB());
}

void Extensions3DChromium::blitFramebuffer(long srcX0, long srcY0, long srcX1, long srcY1, long dstX0, long dstY0, long dstX1, long dstY1, unsigned long mask, unsigned long filter)
{
    m_internal->blitFramebufferCHROMIUM(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

void Extensions3DChromium::renderbufferStorageMultisample(unsigned long target, unsigned long samples, unsigned long internalformat, unsigned long width, unsigned long height)
{
    m_internal->renderbufferStorageMultisampleCHROMIUM(target, samples, internalformat, width, height);
}

void* Extensions3DChromium::mapBufferSubDataCHROMIUM(unsigned target, int offset, int size, unsigned access)
{
    return m_internal->mapBufferSubDataCHROMIUM(target, offset, size, access);
}

void Extensions3DChromium::unmapBufferSubDataCHROMIUM(const void* data)
{
    m_internal->unmapBufferSubDataCHROMIUM(data);
}

void* Extensions3DChromium::mapTexSubImage2DCHROMIUM(unsigned target, int level, int xoffset, int yoffset, int width, int height, unsigned format, unsigned type, unsigned access)
{
    return m_internal->mapTexSubImage2DCHROMIUM(target, level, xoffset, yoffset, width, height, format, type, access);
}

void Extensions3DChromium::unmapTexSubImage2DCHROMIUM(const void* data)
{
    m_internal->unmapTexSubImage2DCHROMIUM(data);
}

Platform3DObject Extensions3DChromium::createVertexArrayOES()
{
    return 0;
}

void Extensions3DChromium::deleteVertexArrayOES(Platform3DObject)
{
}

GC3Dboolean Extensions3DChromium::isVertexArrayOES(Platform3DObject)
{
    return 0;
}

void Extensions3DChromium::bindVertexArrayOES(Platform3DObject)
{
}

void Extensions3DChromium::setSwapBuffersCompleteCallbackCHROMIUM(PassOwnPtr<SwapBuffersCompleteCallbackCHROMIUM> callback)
{
    m_internal->setSwapBuffersCompleteCallbackCHROMIUM(callback);
}

void Extensions3DChromium::rateLimitOffscreenContextCHROMIUM()
{
    m_internal->rateLimitOffscreenContextCHROMIUM();
}

void Extensions3DChromium::paintFramebufferToCanvas(int framebuffer, int width, int height, bool premultiplyAlpha, ImageBuffer* imageBuffer)
{
    m_internal->paintFramebufferToCanvas(framebuffer, width, height, premultiplyAlpha, imageBuffer);
}

} // namespace WebCore

#endif // ENABLE(WEBGL)
