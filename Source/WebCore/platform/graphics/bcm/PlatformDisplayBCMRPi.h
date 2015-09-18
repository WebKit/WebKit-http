#ifndef PlatformDisplayBCMRPi_h
#define PlatformDisplayBCMRPi_h

#if PLATFORM(BCM_RPI)

#include "PlatformDisplay.h"

#include "BCMRPiSurface.h"
#include <EGL/egl.h>

namespace WebCore {

class IntSize;

class PlatformDisplayBCMRPi final : public PlatformDisplay {
public:
    PlatformDisplayBCMRPi();
    virtual ~PlatformDisplayBCMRPi();

    std::unique_ptr<BCMRPiSurface> createSurface(const IntSize&);

    using BCMBufferExport = BCMRPiSurface::BCMBufferExport;

private:
    Type type() const override { return PlatformDisplay::Type::BCMRPi; }

    EGLConfig m_eglConfig;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_PLATFORM_DISPLAY(PlatformDisplayBCMRPi, BCMRPi)

#endif // PLATFORM(BCM_RPI)

#endif // PlatformDisplayBCMRPi_h
