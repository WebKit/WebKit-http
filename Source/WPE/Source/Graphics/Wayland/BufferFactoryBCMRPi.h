#ifndef WPE_Graphics_Wayland_BufferFactoryBCMRPi_h
#define WPE_Graphics_Wayland_BufferFactoryBCMRPi_h

#if WPE_BUFFER_MANAGEMENT(BCM_RPI)

#include "BufferFactory.h"

#include <unordered_map>

namespace WPE {

namespace Graphics {

class BufferFactoryBCMRPi : public BufferFactory {
public:
    virtual ~BufferFactoryBCMRPi();

    std::pair<bool, std::pair<uint32_t, uint32_t>> preferredSize() override { return { false, { 0, 0 } }; };
    std::pair<const uint8_t*, size_t> authenticate() override { return { nullptr, 0 }; };
    uint32_t constructRenderingTarget(uint32_t, uint32_t) override;
    std::pair<bool, std::pair<uint32_t, struct wl_buffer*>> createBuffer(int, const uint8_t*, size_t) override;
    void destroyBuffer(uint32_t, struct wl_buffer*) override;
};

} // namespace Graphics

} // namespace WPE

#endif // WPE_BUFFER_MANAGEMENT(BCMRPi)

#endif // WPE_Graphics_Wayland_BufferFactoryWLBCMRPi_h
