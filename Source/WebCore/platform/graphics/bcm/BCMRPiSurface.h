#ifndef BCMRPiSurface_h
#define BCMRPiSurface_h

#if PLATFORM(BCM_RPI)

#include "GLContext.h"
#include <EGL/egl.h>
#include <tuple>

namespace WebCore {

class IntSize;

class BCMRPiSurface {
public:
    BCMRPiSurface(EGLDisplay, EGLConfig, const IntSize&);
    ~BCMRPiSurface();

    std::unique_ptr<GLContext> createGLContext();

    using BCMBufferExport = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>;
    BCMBufferExport lockFrontBuffer();

private:
    class GLContextBCMRPi : public GLContext {
    public:
        GLContextBCMRPi(BCMRPiSurface&);
        virtual ~GLContextBCMRPi() = default;

        bool makeContextCurrent() override;
        void swapBuffers() override;
        void waitNative() override;
        bool canRenderToDefaultFramebuffer() override;
        IntSize defaultFrameBufferSize() override;

        bool isEGLContext() const override { return true; }

#if USE(CAIRO)
        cairo_device_t* cairoDevice() override { return nullptr; }
#endif

#if ENABLE(GRAPHICS_CONTEXT_3D)
        PlatformGraphicsContext3D platformContext() override;
#endif

    private:
        BCMRPiSurface& m_surface;
    };

    EGLDisplay m_eglDisplay;
    EGLContext m_eglContext;
    std::array<uint32_t, 10> m_globalImages;
    std::array<EGLSurface, 2> m_eglSurfaces;
    uint32_t m_currentSurface { 0 };
};

} // namespace WebCore

#endif // PLATFORM(BCM_RPI)

#endif // BCMRPiSurface_h
