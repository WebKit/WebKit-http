#ifndef PlatformDisplayBCMRPi_h
#define PlatformDisplayBCMRPi_h

#if PLATFORM(BCM_RPI)

#include "PlatformDisplay.h"

#include "BCMRPiSurface.h"

namespace WebCore {

class BCMRPiSurface;
class IntSize;

class PlatformDisplayBCMRPi final : public PlatformDisplay {
public:
    PlatformDisplayBCMRPi();
    virtual ~PlatformDisplayBCMRPi() = default;

    std::unique_ptr<BCMRPiSurface> createSurface(const IntSize&, uint32_t elementHandle);

    using BCMBufferExport = BCMRPiSurface::BCMBufferExport;

private:
    Type type() const override { return PlatformDisplay::Type::BCMRPi; }
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_PLATFORM_DISPLAY(PlatformDisplayBCMRPi, BCMRPi)

#endif // PLATFORM(BCM_RPI)

#endif // PlatformDisplayBCMRPi_h
