#include "Config.h"
#include "RenderingBackendBCMRPiBM.h"

#if WPE_BUFFER_MANAGEMENT(BCM_RPI)

#include <EGL/egl.h>

namespace WPE {

namespace Graphics {

RenderingBackendBCMRPiBM::RenderingBackendBCMRPiBM() = default;

RenderingBackendBCMRPiBM::~RenderingBackendBCMRPiBM() = default;

EGLNativeDisplayType RenderingBackendBCMRPiBM::nativeDisplay()
{
    return EGL_DEFAULT_DISPLAY;
}

std::unique_ptr<RenderingBackend::Surface> RenderingBackendBCMRPiBM::createSurface(uint32_t width, uint32_t height, uint32_t targetHandle, RenderingBackend::Surface::Client& client)
{
    return std::unique_ptr<RenderingBackendBCMRPiBM::Surface>(new RenderingBackendBCMRPiBM::Surface(*this, width, height, targetHandle, client));
}

std::unique_ptr<RenderingBackend::OffscreenSurface> RenderingBackendBCMRPiBM::createOffscreenSurface()
{
    return std::unique_ptr<RenderingBackendBCMRPiBM::OffscreenSurface>(new RenderingBackendBCMRPiBM::OffscreenSurface(*this));
}

RenderingBackendBCMRPiBM::Surface::Surface(const RenderingBackendBCMRPiBM&, uint32_t, uint32_t, uint32_t, Client&)
{
}

RenderingBackendBCMRPiBM::Surface::~Surface() = default;

EGLNativeWindowType RenderingBackendBCMRPiBM::Surface::nativeWindow()
{
    return nullptr;
}

void RenderingBackendBCMRPiBM::Surface::resize(uint32_t, uint32_t)
{
}

RenderingBackend::BufferExport RenderingBackendBCMRPiBM::Surface::lockFrontBuffer()
{
    return std::make_tuple(-1, nullptr, 0);
}

void RenderingBackendBCMRPiBM::Surface::releaseBuffer(uint32_t)
{
}

RenderingBackendBCMRPiBM::OffscreenSurface::OffscreenSurface(const RenderingBackendBCMRPiBM&)
{
}

RenderingBackendBCMRPiBM::OffscreenSurface::~OffscreenSurface() = default;

EGLNativeWindowType RenderingBackendBCMRPiBM::OffscreenSurface::nativeWindow()
{
    return nullptr;
}

} // namespace Graphics

} // namespace WPE

#endif // WPE_BUFFER_MANAGEMENT(BCM_RPI)
