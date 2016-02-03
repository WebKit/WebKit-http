#include "Config.h"
#include "RenderingBackendBCMNexusBM.h"

#if WPE_BUFFER_MANAGEMENT(BCM_NEXUS)

#include <EGL/egl.h>

namespace WPE {

namespace Graphics {

RenderingBackendBCMNexusBM::RenderingBackendBCMNexusBM() = default;

RenderingBackendBCMNexusBM::~RenderingBackendBCMNexusBM() = default;

EGLNativeDisplayType RenderingBackendBCMNexusBM::nativeDisplay()
{
    return EGL_DEFAULT_DISPLAY;
}

std::unique_ptr<RenderingBackend::Surface> RenderingBackendBCMNexusBM::createSurface(uint32_t width, uint32_t height, uint32_t targetHandle, RenderingBackend::Surface::Client& client)
{
    return std::unique_ptr<RenderingBackendBCMNexusBM::Surface>(new RenderingBackendBCMNexusBM::Surface(*this, width, height, targetHandle, client));
}

std::unique_ptr<RenderingBackend::OffscreenSurface> RenderingBackendBCMNexusBM::createOffscreenSurface()
{
    return std::unique_ptr<RenderingBackendBCMNexusBM::OffscreenSurface>(new RenderingBackendBCMNexusBM::OffscreenSurface(*this));
}

RenderingBackendBCMNexusBM::Surface::Surface(const RenderingBackendBCMNexusBM&, uint32_t, uint32_t, uint32_t, Client&)
{
}

RenderingBackendBCMNexusBM::Surface::~Surface() = default;

EGLNativeWindowType RenderingBackendBCMNexusBM::Surface::nativeWindow()
{
    return nullptr;
}

void RenderingBackendBCMNexusBM::Surface::resize(uint32_t, uint32_t)
{
}

RenderingBackend::BufferExport RenderingBackendBCMNexusBM::Surface::lockFrontBuffer()
{
    return std::make_tuple(-1, nullptr, 0);
}

void RenderingBackendBCMNexusBM::Surface::releaseBuffer(uint32_t)
{
}

RenderingBackendBCMNexusBM::OffscreenSurface::OffscreenSurface(const RenderingBackendBCMNexusBM&)
{
}

RenderingBackendBCMNexusBM::OffscreenSurface::~OffscreenSurface() = default;

EGLNativeWindowType RenderingBackendBCMNexusBM::OffscreenSurface::nativeWindow()
{
    return nullptr;
}

} // namespace Graphics

} // namespace WPE

#endif // WPE_BUFFER_MANAGEMENT(BCM_NEXUS)
