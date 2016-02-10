#ifndef WPE_Graphics_RenderingBackendBCMRNexusBM_h
#define WPE_Graphics_RenderingBackendBCMNexusBM_h

#if WPE_BUFFER_MANAGEMENT(BCM_NEXUS)

#include <WPE/Graphics/RenderingBackend.h>

#include <refsw/nexus_config.h>
#include <refsw/nexus_platform.h>
#include <refsw/nexus_display.h>
#include <refsw/nexus_core_utils.h>
#include <refsw/default_nexus.h>
#include <refsw/nxclient.h>

namespace WPE {

namespace Graphics {

class RenderingBackendBCMNexusBM final : public RenderingBackend {
public:
    class Surface final : public RenderingBackend::Surface {
    public:
        Surface(const RenderingBackendBCMNexusBM&, uint32_t, uint32_t, uint32_t, Client&);
        WPE_EXPORT virtual ~Surface();

        EGLNativeWindowType nativeWindow() override;
        void resize(uint32_t, uint32_t) override;

        BufferExport lockFrontBuffer() override;
        void releaseBuffer(uint32_t) override;
    public:
        void* m_nativeWindow;
    };

    class OffscreenSurface final : public RenderingBackend::OffscreenSurface {
    public:
        OffscreenSurface(const RenderingBackendBCMNexusBM&);
        virtual ~OffscreenSurface();

        EGLNativeWindowType nativeWindow() override;
    };

    RenderingBackendBCMNexusBM(const uint8_t*, size_t);
    virtual ~RenderingBackendBCMNexusBM();

    EGLNativeDisplayType nativeDisplay() override;
    std::unique_ptr<RenderingBackend::Surface> createSurface(uint32_t, uint32_t, uint32_t, RenderingBackend::Surface::Client&) override;
    std::unique_ptr<RenderingBackend::OffscreenSurface> createOffscreenSurface() override;

private:
    NXPL_PlatformHandle m_nxplHandle;
};

} // Graphics

} // WPE

#endif // WPE_BUFFER_MANAGEMENT(BCM_NEXUS)

#endif // WPE_Graphics_RenderingBackendBCMNexusBM_h
