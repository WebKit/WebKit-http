#include "Config.h"
#include "RenderingBackendBCMNexusBM.h"

#if WPE_BUFFER_MANAGEMENT(BCM_NEXUS)

#include <EGL/egl.h>
#include <refsw/nexus_config.h>
#include <refsw/nexus_platform.h>
#include <refsw/nexus_display.h>
#include <refsw/nexus_core_utils.h>
#include <refsw/default_nexus.h>
#include <refsw/nxclient.h>
#include <cstring>

namespace WPE {

namespace Graphics {

RenderingBackendBCMNexusBM::RenderingBackendBCMNexusBM(const uint8_t* data, size_t size)
    : m_nxplHandle(nullptr)
{
    NEXUS_Certificate certificate;
    BKNI_Memcpy(certificate.data, data, size);
    certificate.length = size;

    NEXUS_ClientAuthenticationSettings authSettings;
    NEXUS_Platform_GetDefaultClientAuthenticationSettings(&authSettings);
    authSettings.certificate = certificate;

    if (NEXUS_Platform_AuthenticatedJoin(&authSettings))
        fprintf(stderr, "RenderingBackendBCMNexusBM: failed to join\n");

    NXPL_RegisterNexusDisplayPlatform(&m_nxplHandle, nullptr);
}

RenderingBackendBCMNexusBM::~RenderingBackendBCMNexusBM()
{
    NXPL_UnregisterNexusDisplayPlatform(m_nxplHandle);
}

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

RenderingBackendBCMNexusBM::Surface::Surface(const RenderingBackendBCMNexusBM&, uint32_t width, uint32_t height, uint32_t targetHandle, Client&)
{
    NXPL_NativeWindowInfo windowInfo;
    windowInfo.x = 0;
    windowInfo.y = 0;
    windowInfo.width = width;
    windowInfo.height = height;
    windowInfo.stretch = false;
    windowInfo.zOrder = 0;
    windowInfo.clientID = targetHandle;
    m_nativeWindow = NXPL_CreateNativeWindow(&windowInfo);
}

RenderingBackendBCMNexusBM::Surface::~Surface()
{
    NXPL_DestroyNativeWindow(m_nativeWindow);
}

EGLNativeWindowType RenderingBackendBCMNexusBM::Surface::nativeWindow()
{
    return m_nativeWindow;
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
