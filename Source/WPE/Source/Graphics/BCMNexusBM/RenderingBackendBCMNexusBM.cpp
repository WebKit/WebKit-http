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

RenderingBackendBCMNexusBM::RenderingBackendBCMNexusBM() : m_nxplHandle (NULL)
{
    NEXUS_DisplayHandle displayHandle (NULL);
    NxClient_AllocSettings allocSettings;
    NxClient_JoinSettings joinSettings;
    NxClient_GetDefaultJoinSettings( &joinSettings );

    strcpy( joinSettings.name, "wpe" );

    NEXUS_Error rc = NxClient_Join( &joinSettings );
    BDBG_ASSERT(!rc);

    NxClient_GetDefaultAllocSettings(&allocSettings);
    allocSettings.surfaceClient = 1;
    rc = NxClient_Alloc(&allocSettings, &m_AllocResults);
    BDBG_ASSERT(!rc);

    NXPL_RegisterNexusDisplayPlatform(&m_nxplHandle, displayHandle);
}

RenderingBackendBCMNexusBM::~RenderingBackendBCMNexusBM()
{
    NXPL_UnregisterNexusDisplayPlatform(m_nxplHandle);
    NxClient_Free(&m_AllocResults);
    NxClient_Uninit();
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

RenderingBackendBCMNexusBM::Surface::Surface(const RenderingBackendBCMNexusBM&, uint32_t width, uint32_t height, uint32_t, Client&)
{
    printf("creating window with size %dx%d\n",width, height);
    NXPL_NativeWindowInfo windowInfo;
    windowInfo.x = 0;
    windowInfo.y = 0;
    windowInfo.width = width;
    windowInfo.height = height;
    windowInfo.stretch = false;
    windowInfo.clientID = 0;
    m_nativeWindow = NXPL_CreateNativeWindow(&windowInfo);
}

RenderingBackendBCMNexusBM::Surface::~Surface()
{
    NEXUS_SurfaceClient_Release (reinterpret_cast<NEXUS_SurfaceClient*>(m_nativeWindow));
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
