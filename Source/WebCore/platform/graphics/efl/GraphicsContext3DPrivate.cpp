/*
    Copyright (C) 2012 Samsung Electronics
    Copyright (C) 2012 Intel Corporation.

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

#include "config.h"
#include "GraphicsContext3DPrivate.h"

#if USE(3D_GRAPHICS) || USE(ACCELERATED_COMPOSITING)

#include "HostWindow.h"
#include "NotImplemented.h"
#include "PlatformContextCairo.h"

#if USE(OPENGL_ES_2)
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#else
#include "OpenGLShims.h"
#endif

namespace WebCore {

GraphicsContext3DPrivate::GraphicsContext3DPrivate(GraphicsContext3D* context, HostWindow* hostWindow, GraphicsContext3D::RenderStyle renderStyle)
    : m_context(context)
    , m_hostWindow(hostWindow)
    , m_pendingSurfaceResize(false)
{
    if (m_hostWindow && m_hostWindow->platformPageClient()) {
        // FIXME: Implement this code path for WebKit1.
        // Get Evas object from platformPageClient and set EvasGL related members.
        return;
    }

    m_platformContext = GLPlatformContext::createContext(renderStyle);
    if (!m_platformContext)
        return;

    if (renderStyle == GraphicsContext3D::RenderOffscreen) {
#if USE(GRAPHICS_SURFACE)
        m_platformSurface = GLPlatformSurface::createTransportSurface();
#else
        m_platformSurface = GLPlatformSurface::createOffscreenSurface();
#endif
        if (!m_platformSurface) {
            m_platformContext = nullptr;
            return;
        }

        if (!m_platformContext->initialize(m_platformSurface.get()) || !m_platformContext->makeCurrent(m_platformSurface.get())) {
            releaseResources();
            m_platformContext = nullptr;
            m_platformSurface = nullptr;
#if USE(GRAPHICS_SURFACE)
        } else
            m_surfaceHandle = GraphicsSurfaceToken(m_platformSurface->handle());
#else
        }
#endif
    }
}

GraphicsContext3DPrivate::~GraphicsContext3DPrivate()
{
}

void GraphicsContext3DPrivate::releaseResources()
{
    // Release the current context and drawable only after destroying any associated gl resources.
    if (m_platformContext)
        m_platformContext->destroy();

    if (m_platformSurface)
        m_platformSurface->destroy();

    if (m_platformContext)
        m_platformContext->releaseCurrent();
}

bool GraphicsContext3DPrivate::createSurface(PageClientEfl*, bool)
{
    notImplemented();
    return false;
}

void GraphicsContext3DPrivate::setContextLostCallback(PassOwnPtr<GraphicsContext3D::ContextLostCallback> callBack)
{
    m_contextLostCallback = callBack;
}

PlatformGraphicsContext3D GraphicsContext3DPrivate::platformGraphicsContext3D() const
{
    return m_platformContext->handle();
}

bool GraphicsContext3DPrivate::makeContextCurrent()
{
    bool success = m_platformContext->makeCurrent(m_platformSurface.get());

    if (!m_platformContext->isValid()) {
        // FIXME: Restore context
        if (m_contextLostCallback)
            m_contextLostCallback->onContextLost();

        success = false;
    }

    return success;
}

#if USE(TEXTURE_MAPPER_GL)
void GraphicsContext3DPrivate::paintToTextureMapper(TextureMapper*, const FloatRect& /* target */, const TransformationMatrix&, float /* opacity */, BitmapTexture* /* mask */)
{
    notImplemented();
}
#endif

#if USE(GRAPHICS_SURFACE)
void GraphicsContext3DPrivate::createGraphicsSurfaces(const IntSize&)
{
    m_pendingSurfaceResize = true;
}

uint32_t GraphicsContext3DPrivate::copyToGraphicsSurface()
{
    if (!m_platformContext || !makeContextCurrent())
        return 0;

    if (m_pendingSurfaceResize) {
        m_pendingSurfaceResize = false;
        m_platformSurface->setGeometry(IntRect(0, 0, m_context->m_currentWidth, m_context->m_currentHeight));
    }

    if (m_context->m_attrs.antialias) {
#if !USE(OPENGL_ES_2)
        glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, m_context->m_multisampleFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER_EXT, m_context->m_fbo);
        glBlitFramebuffer(0, 0, m_context->m_currentWidth, m_context->m_currentHeight, 0, 0, m_context->m_currentWidth, m_context->m_currentHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
#endif
    }

    m_platformSurface->updateContents(m_context->m_texture, IntRect(0, 0, m_context->m_currentWidth, m_context->m_currentHeight), m_context->m_boundFBO);
    return 0;
}

GraphicsSurfaceToken GraphicsContext3DPrivate::graphicsSurfaceToken() const
{
    return m_surfaceHandle;
}
#endif

} // namespace WebCore

#endif // USE(3D_GRAPHICS) || USE(ACCELERATED_COMPOSITING)
