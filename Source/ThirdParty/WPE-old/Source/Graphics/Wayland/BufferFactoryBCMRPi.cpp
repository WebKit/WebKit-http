#include "Config.h"
#include "BufferFactoryBCMRPi.h"

#if WPE_BUFFER_MANAGEMENT(BCM_RPI)

#include "BufferDataBCMRPi.h"
#include "WaylandDisplay.h"

#define ALIGN_UP(x, y)  ((x + (y)-1) & ~((y)-1))

namespace WPE {

namespace Graphics {

BufferFactoryBCMRPi::~BufferFactoryBCMRPi() = default;

uint32_t BufferFactoryBCMRPi::constructRenderingTarget(uint32_t, uint32_t)
{
    return 0;
}

std::pair<bool, std::pair<uint32_t, struct wl_buffer*>> BufferFactoryBCMRPi::createBuffer(int, const uint8_t*, size_t)
{
    return { false, { 0, nullptr } };
}

void BufferFactoryBCMRPi::destroyBuffer(uint32_t, struct wl_buffer*)
{
}

} // namespace Graphics

} // namespace WPE

#endif // WPE_BUFFER_MANAGEMENT(BCM_RPI)
