#ifndef WPE_Graphics_Wayland_BufferFactoryBCMNexus_h
#define WPE_Graphics_Wayland_BufferFactoryBCMNexus_h

#if WPE_BUFFER_MANAGEMENT(BCM_NEXUS)

#include "BufferFactory.h"

#include <unordered_map>

namespace WPE {

namespace Graphics {

class BufferFactoryBCMNexus : public BufferFactory {
public:
    virtual ~BufferFactoryBCMNexus();

    uint32_t constructRenderingTarget(uint32_t, uint32_t) override;
    std::pair<bool, std::pair<uint32_t, struct wl_buffer*>> createBuffer(int, const uint8_t*, size_t) override;
    void destroyBuffer(uint32_t, struct wl_buffer*) override;
};

} // namespace Graphics

} // namespace WPE

#endif // WPE_BUFFER_MANAGEMENT(BCMNexus)

#endif // WPE_Graphics_Wayland_BufferFactoryWLBCMNexus_h
