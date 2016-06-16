#ifndef WPE_Graphics_RenderingBackendBCMRPiBM_h
#define WPE_Graphics_RenderingBackendBCMRPiBM_h

#if WPE_BUFFER_MANAGEMENT(BCM_RPI)

#include <WPE/Graphics/RenderingBackend.h>

namespace WPE {

namespace Graphics {

class RenderingBackendBCMRPiBM final : public RenderingBackend {
public:
    class Surface final : public RenderingBackend::Surface {
    public:
        Surface(const RenderingBackendBCMRPiBM&, uint32_t, uint32_t, uint32_t, Client&);
        WPE_EXPORT virtual ~Surface();

        EGLNativeWindowType nativeWindow() override;
        void resize(uint32_t, uint32_t) override;

        BufferExport lockFrontBuffer() override;
        void releaseBuffer(uint32_t) override;
    };

    class OffscreenSurface final : public RenderingBackend::OffscreenSurface {
    public:
        OffscreenSurface(const RenderingBackendBCMRPiBM&);
        virtual ~OffscreenSurface();

        EGLNativeWindowType nativeWindow() override;
    };

    RenderingBackendBCMRPiBM();
    virtual ~RenderingBackendBCMRPiBM();

    EGLNativeDisplayType nativeDisplay() override;
    std::unique_ptr<RenderingBackend::Surface> createSurface(uint32_t, uint32_t, uint32_t, RenderingBackend::Surface::Client&) override;
    std::unique_ptr<RenderingBackend::OffscreenSurface> createOffscreenSurface() override;
};

} // Graphics

} // WPE

#endif // WPE_BUFFER_MANAGEMENT(BCM_RPI)

#endif // WPE_Graphics_RenderingBackendBCMRPiBM_h
