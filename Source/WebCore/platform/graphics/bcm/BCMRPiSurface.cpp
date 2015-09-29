#include "config.h"
#include "BCMRPiSurface.h"

#if PLATFORM(BCM_RPI)

#include "IntSize.h"
#include <EGL/eglext.h>

namespace WebCore {

uint32_t BCMRPiSurface::s_imageHandle = 0;

BCMRPiSurface::BCMRPiSurface(EGLDisplay eglDisplay, EGLConfig eglConfig, const IntSize& size)
    : m_eglDisplay(eglDisplay)
    , m_size(size)
{
    fprintf(stderr, "BCMRPiSurface: ctor for %p, EGL display %p config %p, size (%d,%d)\n",
        this, eglDisplay, eglConfig, m_size.width(), m_size.height());

    static const EGLint contextAttributes[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    m_eglContext = eglCreateContext(eglDisplay, eglConfig, GLContext::sharingContext()->platformContext(), contextAttributes);
    if (!m_eglContext) {
        fprintf(stderr, "BCMRPiSurface: failed to create EGLContext\n");
        return;
    }

    const EGLint pbufferAttributes[] = {
        EGL_WIDTH, m_size.width(),
        EGL_HEIGHT, m_size.height(),
        EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGBA,
        EGL_TEXTURE_TARGET, EGL_TEXTURE_2D,
        EGL_NONE
    };
    m_eglSurface = eglCreatePbufferSurface(eglDisplay, eglConfig, pbufferAttributes);
    fprintf(stderr, "m_eglSurface %p error 0x%x\n", m_eglSurface, eglGetError());
    eglMakeCurrent(eglDisplay, m_eglSurface, m_eglSurface, m_eglContext);

    for (auto& image : m_images) {
        auto& texture = image.first;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_size.width(), m_size.height(), 0,
            GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        image.second = eglCreateImageKHR(m_eglDisplay, m_eglContext, EGL_GL_TEXTURE_2D_KHR, (EGLClientBuffer)texture, nullptr);
    }

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_images[0].first, 0);
}

BCMRPiSurface::~BCMRPiSurface()
{
}

std::unique_ptr<GLContext> BCMRPiSurface::createGLContext()
{
    return std::make_unique<GLContextBCMRPi>(*this);
}

BCMRPiSurface::BCMBufferExport BCMRPiSurface::lockFrontBuffer()
{
    uint32_t cb = m_cb;
    m_cb = (m_cb + 1) % 2;
    return BCMBufferExport{ m_images[cb].first, *((uint32_t*)&m_images[cb].second), m_size.width(), m_size.height(), 0 };
}

void BCMRPiSurface::releaseBuffer(uint32_t)
{
}

BCMRPiSurface::GLContextBCMRPi::GLContextBCMRPi(BCMRPiSurface& surface)
    : m_surface(surface)
{
}

bool BCMRPiSurface::GLContextBCMRPi::makeContextCurrent()
{
    GLContext::makeContextCurrent();
    bool r = eglMakeCurrent(m_surface.m_eglDisplay, m_surface.m_eglSurface, m_surface.m_eglSurface, m_surface.m_eglContext);
    if (!r)
        return false;

    glBindFramebuffer(GL_FRAMEBUFFER, m_surface.m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_surface.m_images[m_surface.m_cb].first, 0);
    return true;
}

void BCMRPiSurface::GLContextBCMRPi::swapBuffers()
{
    // FIXME:
    glFinish();

    eglSwapBuffers(m_surface.m_eglDisplay, m_surface.m_eglSurface);
}

void BCMRPiSurface::GLContextBCMRPi::waitNative()
{
    eglWaitNative(EGL_CORE_NATIVE_ENGINE);
}

bool BCMRPiSurface::GLContextBCMRPi::canRenderToDefaultFramebuffer()
{
    return true;
}

IntSize BCMRPiSurface::GLContextBCMRPi::defaultFrameBufferSize()
{
    return { 1280, 720 };
}

PlatformGraphicsContext3D BCMRPiSurface::GLContextBCMRPi::platformContext()
{
    return m_surface.m_eglContext;
}

} // namespace WebCore

#endif // PLATFORM(BCM_RPI)
