/*
 * Copyright (C) 2016 Igalia S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 */

#include "config.h"
#include "GLContextWPE.h"

#include "PlatformDisplayWPE.h"

#if USE(CAIRO)
#include <cairo.h>
#if ENABLE(ACCELERATED_2D_CANVAS)
#include <cairo-gl.h>
#endif
#endif

namespace WebCore {

static const EGLint gContextAttributes[] = {
#if USE(OPENGL_ES_2)
    EGL_CONTEXT_CLIENT_VERSION, 2,
#endif
    EGL_NONE
};

#if USE(OPENGL_ES_2)
static const EGLenum gEGLAPIVersion = EGL_OPENGL_ES_API;
#else
static const EGLenum gEGLAPIVersion = EGL_OPENGL_API;
#endif


std::unique_ptr<GLContextWPE> GLContextWPE::createContext(PlatformDisplay& platformDisplay, EGLNativeWindowType window, bool isSharing, std::unique_ptr<GLContext::Data>&& contextData)
{
    if (platformDisplay.eglDisplay() == EGL_NO_DISPLAY)
        return nullptr;

    if (eglBindAPI(gEGLAPIVersion) == EGL_FALSE)
        return nullptr;

    EGLContext eglSharingContext = EGL_NO_CONTEXT;
    if (!isSharing)
        eglSharingContext = static_cast<GLContextWPE*>(platformDisplay.sharingGLContext())->m_context;

    auto context = window ? createWindowContext(window, platformDisplay, eglSharingContext) : nullptr;
    if (!context)
        context = createPbufferContext(platformDisplay, eglSharingContext);

    if (context)
        context->m_contextData = WTFMove(contextData);
    return context;
}

std::unique_ptr<GLContextWPE> GLContextWPE::createOffscreenContext(PlatformDisplay& platformDisplay)
{
    return downcast<PlatformDisplayWPE>(platformDisplay).createOffscreenContext(platformDisplay, false);
}

std::unique_ptr<GLContextWPE> GLContextWPE::createSharingContext(PlatformDisplay& platformDisplay)
{
    return downcast<PlatformDisplayWPE>(platformDisplay).createOffscreenContext(platformDisplay, true);
}

GLContextWPE::GLContextWPE(PlatformDisplay& display, EGLContext context, EGLSurface surface, EGLSurfaceType type)
    : GLContext(display)
    , m_context(context)
    , m_surface(surface)
    , m_type(type)
{
}

GLContextWPE::~GLContextWPE()
{
#if USE(CAIRO)
    if (m_cairoDevice)
        cairo_device_destroy(m_cairoDevice);
#endif

    EGLDisplay display = m_display.eglDisplay();
    if (m_context) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroyContext(display, m_context);
    }

    if (m_surface)
        eglDestroySurface(display, m_surface);
}

std::unique_ptr<GLContextWPE> GLContextWPE::createWindowContext(EGLNativeWindowType window, PlatformDisplay& platformDisplay, EGLContext sharingContext)
{
    EGLDisplay display = platformDisplay.eglDisplay();
    EGLConfig config;
    if (!getEGLConfig(display, &config, WindowSurface))
        return nullptr;

    EGLContext context = eglCreateContext(display, config, sharingContext, gContextAttributes);
    if (context == EGL_NO_CONTEXT)
        return nullptr;

    EGLSurface surface = eglCreateWindowSurface(display, config, window, 0);
    if (surface == EGL_NO_SURFACE) {
        eglDestroyContext(display, context);
        return nullptr;
    }

    return std::unique_ptr<GLContextWPE>(new GLContextWPE(platformDisplay, context, surface, WindowSurface));
}

std::unique_ptr<GLContextWPE> GLContextWPE::createPbufferContext(PlatformDisplay& platformDisplay, EGLContext sharingContext)
{
    EGLDisplay display = platformDisplay.eglDisplay();
    EGLConfig config;
    if (!getEGLConfig(display, &config, PbufferSurface))
        return nullptr;

    EGLContext context = eglCreateContext(display, config, sharingContext, gContextAttributes);
    if (context == EGL_NO_CONTEXT)
        return nullptr;

    static const int pbufferAttributes[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
    EGLSurface surface = eglCreatePbufferSurface(display, config, pbufferAttributes);
    if (surface == EGL_NO_SURFACE) {
        eglDestroyContext(display, context);
        return nullptr;
    }

    return std::unique_ptr<GLContextWPE>(new GLContextWPE(platformDisplay, context, surface, PbufferSurface));
}

bool GLContextWPE::makeContextCurrent()
{
    ASSERT(m_context);

    GLContext::makeContextCurrent();
    if (eglGetCurrentContext() == m_context)
        return true;

    return eglMakeCurrent(m_display.eglDisplay(), m_surface, m_surface, m_context);
}

void GLContextWPE::swapBuffers()
{
    ASSERT(m_surface);
    eglSwapBuffers(m_display.eglDisplay(), m_surface);
}

void GLContextWPE::waitNative()
{
    eglWaitNative(EGL_CORE_NATIVE_ENGINE);
}

bool GLContextWPE::canRenderToDefaultFramebuffer()
{
    return m_type == WindowSurface;
}

IntSize GLContextWPE::defaultFrameBufferSize()
{
    if (!canRenderToDefaultFramebuffer())
        return IntSize();

    EGLDisplay display = m_display.eglDisplay();
    EGLint width, height;
    if (!eglQuerySurface(display, m_surface, EGL_WIDTH, &width)
        || !eglQuerySurface(display, m_surface, EGL_HEIGHT, &height))
        return IntSize();

    return IntSize(width, height);
}

void GLContextWPE::swapInterval(int interval)
{
    ASSERT(m_surface);
    eglSwapInterval(m_display.eglDisplay(), interval);
}

bool GLContextWPE::isEGLContext() const
{
    return true;
}

#if USE(CAIRO)
cairo_device_t* GLContextWPE::cairoDevice()
{
    if (m_cairoDevice)
        return m_cairoDevice;

#if ENABLE(ACCELERATED_2D_CANVAS)
    m_cairoDevice = cairo_egl_device_create_for_egl_surface(m_display.eglDisplay(), m_context, m_surface);
#endif

    return m_cairoDevice;
}
#endif

#if ENABLE(GRAPHICS_CONTEXT_3D)
PlatformGraphicsContext3D GLContextWPE::platformContext()
{
    return m_context;
}
#endif

bool GLContextWPE::getEGLConfig(EGLDisplay display, EGLConfig* config, EGLSurfaceType surfaceType)
{
    EGLint attributeList[] = {
#if USE(OPENGL_ES_2)
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
#else
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
#endif
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_STENCIL_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_SURFACE_TYPE, EGL_NONE,
        EGL_NONE
    };

    switch (surfaceType) {
    case GLContextWPE::PbufferSurface:
        attributeList[13] = EGL_PBUFFER_BIT;
        break;
    case GLContextWPE::WindowSurface:
        attributeList[13] = EGL_WINDOW_BIT;
        break;
    }

    EGLint numberConfigsReturned;
    return eglChooseConfig(display, attributeList, config, 1, &numberConfigsReturned) && numberConfigsReturned;
}

} // namespace WebCore
