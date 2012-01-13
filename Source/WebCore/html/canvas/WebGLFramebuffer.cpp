/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#if ENABLE(WEBGL)

#include "WebGLFramebuffer.h"

#include "WebGLContextGroup.h"
#include "WebGLRenderingContext.h"

namespace WebCore {

namespace {

    bool isAttachmentComplete(WebGLSharedObject* attachedObject, GC3Denum attachment)
    {
        ASSERT(attachedObject && attachedObject->object());
        ASSERT(attachedObject->isRenderbuffer());
        WebGLRenderbuffer* buffer = reinterpret_cast<WebGLRenderbuffer*>(attachedObject);
        switch (attachment) {
        case GraphicsContext3D::DEPTH_ATTACHMENT:
            if (buffer->getInternalFormat() != GraphicsContext3D::DEPTH_COMPONENT16)
                return false;
            break;
        case GraphicsContext3D::STENCIL_ATTACHMENT:
            if (buffer->getInternalFormat() != GraphicsContext3D::STENCIL_INDEX8)
                return false;
            break;
        case GraphicsContext3D::DEPTH_STENCIL_ATTACHMENT:
            if (buffer->getInternalFormat() != GraphicsContext3D::DEPTH_STENCIL)
                return false;
            break;
        default:
            ASSERT_NOT_REACHED();
            return false;
        }
        if (!buffer->getWidth() || !buffer->getHeight())
            return false;
        return true;
    }

    GC3Dsizei getImageWidth(WebGLSharedObject* attachedObject)
    {
        ASSERT(attachedObject && attachedObject->object());
        ASSERT(attachedObject->isRenderbuffer());
        WebGLRenderbuffer* buffer = reinterpret_cast<WebGLRenderbuffer*>(attachedObject);
        return buffer->getWidth();
    }

    GC3Dsizei getImageHeight(WebGLSharedObject* attachedObject)
    {
        ASSERT(attachedObject && attachedObject->object());
        ASSERT(attachedObject->isRenderbuffer());
        WebGLRenderbuffer* buffer = reinterpret_cast<WebGLRenderbuffer*>(attachedObject);
        return buffer->getHeight();
    }

    bool isUninitialized(WebGLSharedObject* attachedObject)
    {
        if (attachedObject && attachedObject->object() && attachedObject->isRenderbuffer()
            && !(reinterpret_cast<WebGLRenderbuffer*>(attachedObject))->isInitialized())
            return true;
        return false;
    }

    void setInitialized(WebGLSharedObject* attachedObject)
    {
        if (attachedObject && attachedObject->object() && attachedObject->isRenderbuffer())
            (reinterpret_cast<WebGLRenderbuffer*>(attachedObject))->setInitialized();
    }

    bool isValidRenderbuffer(WebGLSharedObject* attachedObject)
    {
        if (attachedObject && attachedObject->object() && attachedObject->isRenderbuffer()) {
            if (!(reinterpret_cast<WebGLRenderbuffer*>(attachedObject))->isValid())
                return false;
        }
        return true;
    }

} // anonymous namespace

PassRefPtr<WebGLFramebuffer> WebGLFramebuffer::create(WebGLRenderingContext* ctx)
{
    return adoptRef(new WebGLFramebuffer(ctx));
}

WebGLFramebuffer::WebGLFramebuffer(WebGLRenderingContext* ctx)
    : WebGLContextObject(ctx)
    , m_hasEverBeenBound(false)
    , m_texTarget(0)
    , m_texLevel(-1)
{
    setObject(ctx->graphicsContext3D()->createFramebuffer());
}

WebGLFramebuffer::~WebGLFramebuffer()
{
    deleteObject(0);
}

void WebGLFramebuffer::setAttachmentForBoundFramebuffer(GC3Denum attachment, GC3Denum texTarget, WebGLTexture* texture, GC3Dint level)
{
    ASSERT(isBound());
    if (!object())
        return;
    removeAttachmentFromBoundFramebuffer(attachment);
    if (texture && !texture->object())
        texture = 0;
    switch (attachment) {
    case GraphicsContext3D::COLOR_ATTACHMENT0:
        m_colorAttachment = texture;
        if (texture) {
            m_texTarget = texTarget;
            m_texLevel = level;
        }
        break;
    case GraphicsContext3D::DEPTH_ATTACHMENT:
        m_depthAttachment = texture;
        break;
    case GraphicsContext3D::STENCIL_ATTACHMENT:
        m_stencilAttachment = texture;
        break;
    case GraphicsContext3D::DEPTH_STENCIL_ATTACHMENT:
        m_depthStencilAttachment = texture;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }
    if (texture)
        texture->onAttached();
}

void WebGLFramebuffer::setAttachmentForBoundFramebuffer(GC3Denum attachment, WebGLRenderbuffer* renderbuffer)
{
    ASSERT(isBound());
    if (!object())
        return;
    removeAttachmentFromBoundFramebuffer(attachment);
    if (renderbuffer && !renderbuffer->object())
        renderbuffer = 0;
    switch (attachment) {
    case GraphicsContext3D::COLOR_ATTACHMENT0:
        m_colorAttachment = renderbuffer;
        break;
    case GraphicsContext3D::DEPTH_ATTACHMENT:
        m_depthAttachment = renderbuffer;
        break;
    case GraphicsContext3D::STENCIL_ATTACHMENT:
        m_stencilAttachment = renderbuffer;
        break;
    case GraphicsContext3D::DEPTH_STENCIL_ATTACHMENT:
        m_depthStencilAttachment = renderbuffer;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }
    if (renderbuffer)
        renderbuffer->onAttached();
}

WebGLSharedObject* WebGLFramebuffer::getAttachment(GC3Denum attachment) const
{
    if (!object())
        return 0;
    switch (attachment) {
    case GraphicsContext3D::COLOR_ATTACHMENT0:
        return m_colorAttachment.get();
    case GraphicsContext3D::DEPTH_ATTACHMENT:
        return m_depthAttachment.get();
    case GraphicsContext3D::STENCIL_ATTACHMENT:
        return m_stencilAttachment.get();
    case GraphicsContext3D::DEPTH_STENCIL_ATTACHMENT:
        return m_depthStencilAttachment.get();
    default:
        ASSERT_NOT_REACHED();
        return 0;
    }
}

void WebGLFramebuffer::removeAttachmentFromBoundFramebuffer(GC3Denum attachment)
{
    ASSERT(isBound());
    if (!object())
        return;

    GraphicsContext3D* context3d = context()->graphicsContext3D();
    switch (attachment) {
    case GraphicsContext3D::COLOR_ATTACHMENT0:
        if (m_colorAttachment) {
            m_colorAttachment->onDetached(context3d);
            m_colorAttachment = 0;
            m_texTarget = 0;
            m_texLevel = -1;
        }
        break;
    case GraphicsContext3D::DEPTH_ATTACHMENT:
        if (m_depthAttachment) {
            m_depthAttachment->onDetached(context3d);
            m_depthAttachment = 0;
        }
        break;
    case GraphicsContext3D::STENCIL_ATTACHMENT:
        if (m_stencilAttachment) {
            m_stencilAttachment->onDetached(context3d);
            m_stencilAttachment = 0;
        }
        break;
    case GraphicsContext3D::DEPTH_STENCIL_ATTACHMENT:
        if (m_depthStencilAttachment) {
            m_depthStencilAttachment->onDetached(context3d);
            m_depthStencilAttachment = 0;
        }
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }
}

void WebGLFramebuffer::removeAttachmentFromBoundFramebuffer(WebGLSharedObject* attachment)
{
    ASSERT(isBound());
    if (!object())
        return;
    if (!attachment)
        return;

    GraphicsContext3D* gc3d = context()->graphicsContext3D();

    if (attachment == m_colorAttachment.get()) {
        if (attachment->isRenderbuffer())
            gc3d->framebufferRenderbuffer(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::COLOR_ATTACHMENT0, GraphicsContext3D::RENDERBUFFER, 0);
        else
            gc3d->framebufferTexture2D(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::COLOR_ATTACHMENT0, m_texTarget, 0, m_texLevel);
        removeAttachmentFromBoundFramebuffer(GraphicsContext3D::COLOR_ATTACHMENT0);
    }
    if (attachment == m_depthAttachment.get()) {
        gc3d->framebufferRenderbuffer(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::DEPTH_ATTACHMENT, GraphicsContext3D::RENDERBUFFER, 0);
        removeAttachmentFromBoundFramebuffer(GraphicsContext3D::DEPTH_ATTACHMENT);
    }
    if (attachment == m_stencilAttachment.get()) {
        gc3d->framebufferRenderbuffer(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::STENCIL_ATTACHMENT, GraphicsContext3D::RENDERBUFFER, 0);
        removeAttachmentFromBoundFramebuffer(GraphicsContext3D::STENCIL_ATTACHMENT);
    }
    if (attachment == m_depthStencilAttachment.get()) {
        gc3d->framebufferRenderbuffer(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::DEPTH_ATTACHMENT, GraphicsContext3D::RENDERBUFFER, 0);
        gc3d->framebufferRenderbuffer(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::STENCIL_ATTACHMENT, GraphicsContext3D::RENDERBUFFER, 0);
        removeAttachmentFromBoundFramebuffer(GraphicsContext3D::DEPTH_STENCIL_ATTACHMENT);
    }
}

GC3Dsizei WebGLFramebuffer::getColorBufferWidth() const
{
    if (!object() || !isColorAttached())
        return 0;
    if (m_colorAttachment->isRenderbuffer())
        return (reinterpret_cast<WebGLRenderbuffer*>(m_colorAttachment.get()))->getWidth();
    if (m_colorAttachment->isTexture())
        return (reinterpret_cast<WebGLTexture*>(m_colorAttachment.get()))->getWidth(m_texTarget, m_texLevel);
    ASSERT_NOT_REACHED();
    return 0;
}

GC3Dsizei WebGLFramebuffer::getColorBufferHeight() const
{
    if (!object() || !isColorAttached())
        return 0;
    if (m_colorAttachment->isRenderbuffer())
        return (reinterpret_cast<WebGLRenderbuffer*>(m_colorAttachment.get()))->getHeight();
    if (m_colorAttachment->isTexture())
        return (reinterpret_cast<WebGLTexture*>(m_colorAttachment.get()))->getHeight(m_texTarget, m_texLevel);
    ASSERT_NOT_REACHED();
    return 0;
}

GC3Denum WebGLFramebuffer::getColorBufferFormat() const
{
    if (!object() || !isColorAttached())
        return 0;
    if (m_colorAttachment->isRenderbuffer()) {
        unsigned long format = (reinterpret_cast<WebGLRenderbuffer*>(m_colorAttachment.get()))->getInternalFormat();
        switch (format) {
        case GraphicsContext3D::RGBA4:
        case GraphicsContext3D::RGB5_A1:
            return GraphicsContext3D::RGBA;
        case GraphicsContext3D::RGB565:
            return GraphicsContext3D::RGB;
        }
        return 0;
    }
    if (m_colorAttachment->isTexture())
        return (reinterpret_cast<WebGLTexture*>(m_colorAttachment.get()))->getInternalFormat(m_texTarget, m_texLevel);
    ASSERT_NOT_REACHED();
    return 0;
}

GC3Denum WebGLFramebuffer::checkStatus() const
{
    unsigned int count = 0;
    GC3Dsizei width = 0, height = 0;
    if (isDepthAttached()) {
        if (!isAttachmentComplete(m_depthAttachment.get(), GraphicsContext3D::DEPTH_ATTACHMENT))
            return GraphicsContext3D::FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
        width = getImageWidth(m_depthAttachment.get());
        height = getImageHeight(m_depthAttachment.get());
        count++;
    }
    if (isStencilAttached()) {
        if (!isAttachmentComplete(m_stencilAttachment.get(), GraphicsContext3D::STENCIL_ATTACHMENT))
            return GraphicsContext3D::FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
        if (!count) {
            width = getImageWidth(m_stencilAttachment.get());
            height = getImageHeight(m_stencilAttachment.get());
        } else {
            if (width != getImageWidth(m_stencilAttachment.get()) || height != getImageHeight(m_stencilAttachment.get()))
                return GraphicsContext3D::FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
        }
        count++;
    }
    if (isDepthStencilAttached()) {
        if (!isAttachmentComplete(m_depthStencilAttachment.get(), GraphicsContext3D::DEPTH_STENCIL_ATTACHMENT))
            return GraphicsContext3D::FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
        if (!isValidRenderbuffer(m_depthStencilAttachment.get()))
            return GraphicsContext3D::FRAMEBUFFER_UNSUPPORTED;
        if (!count) {
            width = getImageWidth(m_depthStencilAttachment.get());
            height = getImageHeight(m_depthStencilAttachment.get());
        } else {
            if (width != getImageWidth(m_depthStencilAttachment.get()) || height != getImageHeight(m_depthStencilAttachment.get()))
                return GraphicsContext3D::FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
        }
        count++;
    }
    // WebGL specific: no conflicting DEPTH/STENCIL/DEPTH_STENCIL attachments.
    if (count > 1)
        return GraphicsContext3D::FRAMEBUFFER_UNSUPPORTED;
    if (isColorAttached()) {
        // FIXME: if color buffer is texture, is ALPHA, LUMINANCE or LUMINANCE_ALPHA valid?
        if (!getColorBufferFormat())
            return GraphicsContext3D::FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
        if (!count) {
            if (!getColorBufferWidth() || !getColorBufferHeight())
                return GraphicsContext3D::FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
        } else {
            if (width != getColorBufferWidth() || height != getColorBufferHeight())
                return GraphicsContext3D::FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
        }
    } else {
        if (!count)
            return GraphicsContext3D::FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
    }
    return GraphicsContext3D::FRAMEBUFFER_COMPLETE;
}

bool WebGLFramebuffer::onAccess(GraphicsContext3D* context3d, bool needToInitializeRenderbuffers)
{
    if (checkStatus() != GraphicsContext3D::FRAMEBUFFER_COMPLETE)
        return false;
    if (needToInitializeRenderbuffers)
        return initializeRenderbuffers(context3d);
    return true;
}

void WebGLFramebuffer::deleteObjectImpl(GraphicsContext3D* context3d, Platform3DObject object)
{
    if (m_colorAttachment)
        m_colorAttachment->onDetached(context3d);
    if (m_depthAttachment)
        m_depthAttachment->onDetached(context3d);
    if (m_stencilAttachment)
        m_stencilAttachment->onDetached(context3d);
    if (m_depthStencilAttachment)
        m_depthStencilAttachment->onDetached(context3d);
    context3d->deleteFramebuffer(object);
}

bool WebGLFramebuffer::initializeRenderbuffers(GraphicsContext3D* g3d)
{
    ASSERT(object());
    bool initColor = false, initDepth = false, initStencil = false;
    GC3Dbitfield mask = 0;
    if (isUninitialized(m_colorAttachment.get())) {
        initColor = true;
        mask |= GraphicsContext3D::COLOR_BUFFER_BIT;
    }
    if (isUninitialized(m_depthAttachment.get())) {
        initDepth = true;
        mask |= GraphicsContext3D::DEPTH_BUFFER_BIT;
    }
    if (isUninitialized(m_stencilAttachment.get())) {
        initStencil = true;
        mask |= GraphicsContext3D::STENCIL_BUFFER_BIT;
    }
    if (isUninitialized(m_depthStencilAttachment.get())) {
        initDepth = true;
        initStencil = true;
        mask |= (GraphicsContext3D::DEPTH_BUFFER_BIT | GraphicsContext3D::STENCIL_BUFFER_BIT);
    }
    if (!initColor && !initDepth && !initStencil)
        return true;

    // We only clear un-initialized renderbuffers when they are ready to be
    // read, i.e., when the framebuffer is complete.
    if (g3d->checkFramebufferStatus(GraphicsContext3D::FRAMEBUFFER) != GraphicsContext3D::FRAMEBUFFER_COMPLETE)
        return false;

    GC3Dfloat colorClearValue[] = {0, 0, 0, 0}, depthClearValue = 0;
    GC3Dint stencilClearValue = 0;
    GC3Dboolean colorMask[] = {0, 0, 0, 0}, depthMask = 0;
    GC3Duint stencilMask = 0xffffffff;
    GC3Dboolean isScissorEnabled = 0;
    GC3Dboolean isDitherEnabled = 0;
    if (initColor) {
        g3d->getFloatv(GraphicsContext3D::COLOR_CLEAR_VALUE, colorClearValue);
        g3d->getBooleanv(GraphicsContext3D::COLOR_WRITEMASK, colorMask);
        g3d->clearColor(0, 0, 0, 0);
        g3d->colorMask(true, true, true, true);
    }
    if (initDepth) {
        g3d->getFloatv(GraphicsContext3D::DEPTH_CLEAR_VALUE, &depthClearValue);
        g3d->getBooleanv(GraphicsContext3D::DEPTH_WRITEMASK, &depthMask);
        g3d->clearDepth(0);
        g3d->depthMask(true);
    }
    if (initStencil) {
        g3d->getIntegerv(GraphicsContext3D::STENCIL_CLEAR_VALUE, &stencilClearValue);
        g3d->getIntegerv(GraphicsContext3D::STENCIL_WRITEMASK, reinterpret_cast<GC3Dint*>(&stencilMask));
        g3d->clearStencil(0);
        g3d->stencilMask(0xffffffff);
    }
    isScissorEnabled = g3d->isEnabled(GraphicsContext3D::SCISSOR_TEST);
    g3d->disable(GraphicsContext3D::SCISSOR_TEST);
    isDitherEnabled = g3d->isEnabled(GraphicsContext3D::DITHER);
    g3d->disable(GraphicsContext3D::DITHER);

    g3d->clear(mask);

    if (initColor) {
        g3d->clearColor(colorClearValue[0], colorClearValue[1], colorClearValue[2], colorClearValue[3]);
        g3d->colorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
    }
    if (initDepth) {
        g3d->clearDepth(depthClearValue);
        g3d->depthMask(depthMask);
    }
    if (initStencil) {
        g3d->clearStencil(stencilClearValue);
        g3d->stencilMask(stencilMask);
    }
    if (isScissorEnabled)
        g3d->enable(GraphicsContext3D::SCISSOR_TEST);
    else
        g3d->disable(GraphicsContext3D::SCISSOR_TEST);
    if (isDitherEnabled)
        g3d->enable(GraphicsContext3D::DITHER);
    else
        g3d->disable(GraphicsContext3D::DITHER);

    if (initColor)
        setInitialized(m_colorAttachment.get());
    if (initDepth && initStencil && m_depthStencilAttachment)
        setInitialized(m_depthStencilAttachment.get());
    else {
        if (initDepth)
            setInitialized(m_depthAttachment.get());
        if (initStencil)
            setInitialized(m_stencilAttachment.get());
    }
    return true;
}

bool WebGLFramebuffer::isBound() const
{
    return (context()->m_framebufferBinding.get() == this);
}

}

#endif // ENABLE(WEBGL)
