#include "config.h"
#include "PlatformDisplayBCMRPi.h"

#if PLATFORM(BCM_RPI)

#include "IntSize.h"
#include <EGL/eglext.h>

namespace WebCore {

PlatformDisplayBCMRPi::PlatformDisplayBCMRPi()
{
    fprintf(stderr, "PlatformDisplayBCMRPi: ctor\n");

    m_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (m_eglDisplay == EGL_NO_DISPLAY) {
        fprintf(stderr, "PlatformDisplayBCMRPi: could not create the EGL display\n");
        return;
    }

    PlatformDisplay::initializeEGLDisplay();
    fprintf(stderr, "\tm_eglDisplay %p\n", m_eglDisplay);

    static const EGLint configAttributes[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE, EGL_PIXMAP_BIT,
        EGL_NONE
    };

    EGLint numberOfConfigs;
    if (!eglChooseConfig(m_eglDisplay, configAttributes, &m_eglConfig, 1, &numberOfConfigs) || numberOfConfigs != 1) {
        fprintf(stderr, "\tPlatformDisplayBCMRPi: could not get the desired EGL config\n");
        return;
    }
}

PlatformDisplayBCMRPi::~PlatformDisplayBCMRPi()
{
    fprintf(stderr, "PlatformDisplayBCMRPi: dtor\n");
}

std::unique_ptr<BCMRPiSurface> PlatformDisplayBCMRPi::createSurface(const IntSize& size)
{
    fprintf(stderr, "PlatformDisplayBCMRPi::%s(): size (%d,%d)\n", __func__, size.width(), size.height());
    return std::make_unique<BCMRPiSurface>(m_eglDisplay, m_eglConfig, size);
}

} // namespace WebCore

#endif // PLATFORM(BCM_RPI)
