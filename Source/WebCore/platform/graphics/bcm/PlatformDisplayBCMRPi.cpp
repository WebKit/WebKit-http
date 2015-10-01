#include "config.h"
#include "PlatformDisplayBCMRPi.h"

#if PLATFORM(BCM_RPI)

#include <EGL/egl.h>
#include "IntSize.h"

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
}

std::unique_ptr<BCMRPiSurface> PlatformDisplayBCMRPi::createSurface(const IntSize& size, uint32_t elementHandle)
{
    return std::make_unique<BCMRPiSurface>(size, elementHandle);
}

} // namespace WebCore

#endif // PLATFORM(BCM_RPI)
