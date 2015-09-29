#include "config.h"
#include "BCMRPiSurface.h"

#if PLATFORM(BCM_RPI)

#include "IntSize.h"
#include <EGL/eglext.h>

namespace WebCore {

BCMRPiSurface::BCMRPiSurface(EGLDisplay eglDisplay, EGLConfig eglConfig, const IntSize& size)
    : m_eglDisplay(eglDisplay)
{
    int width = size.width();
    int height = size.height();
    fprintf(stderr, "BCMRPiSurface: ctor for %p, EGL display %p config %p, size (%d,%d)\n",
        this, eglDisplay, eglConfig, width, height);

    static const EGLint contextAttributes[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    m_eglContext = eglCreateContext(eglDisplay, eglConfig, GLContext::sharingContext()->platformContext(), contextAttributes);
    if (!m_eglContext) {
        fprintf(stderr, "BCMRPiSurface: failed to create EGLContext\n");
        return;
    }

    EGLint pixelFormat = EGL_PIXEL_FORMAT_ARGB_8888_BRCM;
    EGLint renderableType;
    eglGetConfigAttrib(eglDisplay, eglConfig, EGL_RENDERABLE_TYPE, &renderableType);
    if (renderableType & EGL_OPENGL_ES_BIT)
        pixelFormat |= EGL_PIXEL_FORMAT_RENDER_GLES_BRCM | EGL_PIXEL_FORMAT_GLES_TEXTURE_BRCM;
    if (renderableType & EGL_OPENGL_ES2_BIT)
        pixelFormat |= EGL_PIXEL_FORMAT_RENDER_GLES2_BRCM | EGL_PIXEL_FORMAT_GLES2_TEXTURE_BRCM;
    if (renderableType & EGL_OPENVG_BIT)
        pixelFormat |= EGL_PIXEL_FORMAT_RENDER_VG_BRCM | EGL_PIXEL_FORMAT_VG_IMAGE_BRCM;
    if (renderableType & EGL_OPENGL_BIT)
        pixelFormat |= EGL_PIXEL_FORMAT_RENDER_GL_BRCM;

    static const EGLint pixmapAttributes[] = {
        EGL_VG_COLORSPACE, EGL_VG_COLORSPACE_sRGB,
        EGL_VG_ALPHA_FORMAT, EGL_VG_ALPHA_FORMAT_NONPRE,
        EGL_NONE
    };

    for (unsigned i = 0; i < 2; ++i) {
        EGLint image[5] = { 0, 0, width, height, pixelFormat };

        eglCreateGlobalImageBRCM(width, height, pixelFormat, nullptr, width * 4, image);
        m_eglSurfaces[i] = eglCreatePixmapSurface(eglDisplay, eglConfig,
            (EGLNativePixmapType)image, pixmapAttributes);

        auto* globalImage = &m_globalImages[i * 5];
        globalImage[0] = static_cast<uint32_t>(image[0]);
        globalImage[1] = static_cast<uint32_t>(image[1]);
        globalImage[2] = static_cast<uint32_t>(width);
        globalImage[3] = static_cast<uint32_t>(height);
        globalImage[4] = static_cast<uint32_t>(pixelFormat);
    }

    fprintf(stderr, "global image #1: [0] = %d, [1] = %d\n", m_globalImages[0], m_globalImages[1]);
    fprintf(stderr, "global image #2: [0] = %d, [1] = %d\n", m_globalImages[5], m_globalImages[6]);
    fprintf(stderr, "\tEGL surfaces: #1 %p, #2 %p\n", m_eglSurfaces[0], m_eglSurfaces[1]);
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
    uint32_t i = m_currentSurface;
    m_currentSurface = (i + 1) % 2;
    auto* image = &m_globalImages[i * 5];
    return BCMBufferExport{ image[0], image[1], image[2], image[3], image[4] };
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
    uint32_t i = m_surface.m_currentSurface;
    return eglMakeCurrent(m_surface.m_eglDisplay, m_surface.m_eglSurfaces[i], m_surface.m_eglSurfaces[i], m_surface.m_eglContext);
}

void BCMRPiSurface::GLContextBCMRPi::swapBuffers()
{
    glFinish();
    eglFlushBRCM();
    uint32_t i = m_surface.m_currentSurface;
    eglSwapBuffers(m_surface.m_eglDisplay, m_surface.m_eglSurfaces[i]);
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
