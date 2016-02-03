#include "Config.h"
#include "BufferFactoryBCMNexus.h"

#if WPE_BUFFER_MANAGEMENT(BCM_NEXUS)

#include "BufferDataBCMNexus.h"
#include "WaylandDisplay.h"

#define ALIGN_UP(x, y)  ((x + (y)-1) & ~((y)-1))

namespace WPE {

namespace Graphics {

BufferFactoryBCMNexus::~BufferFactoryBCMNexus() = default;

uint32_t BufferFactoryBCMNexus::constructRenderingTarget(uint32_t, uint32_t)
{
    return 0;
}

std::pair<bool, std::pair<uint32_t, struct wl_buffer*>> BufferFactoryBCMNexus::createBuffer(int, const uint8_t*, size_t)
{
    return { false, { 0, nullptr } };
}

void BufferFactoryBCMNexus::destroyBuffer(uint32_t, struct wl_buffer*)
{
}

} // namespace Graphics

} // namespace WPE

#endif // WPE_BUFFER_MANAGEMENT(BCM_NEXUS)
