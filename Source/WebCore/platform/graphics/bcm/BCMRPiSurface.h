#ifndef BCMRPiSurface_h
#define BCMRPiSurface_h

#if PLATFORM(BCM_RPI)

#include "GLContext.h"
#include "IntSize.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <tuple>

namespace WebCore {

class BCMRPiSurface {
public:
    BCMRPiSurface(EGLDisplay, EGLConfig, const IntSize&);
    ~BCMRPiSurface();

    std::unique_ptr<GLContext> createGLContext();

    using BCMBufferExport = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>;
    BCMBufferExport lockFrontBuffer();
    void releaseBuffer(uint32_t);

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

    static uint32_t s_imageHandle;
    IntSize m_size;
    EGLSurface m_eglSurface;
    uint32_t m_cb { 0 };
    std::array<std::pair<GLuint, EGLImageKHR>, 2> m_images;
    GLuint m_fbo { 0 };
};

} // namespace WebCore

#endif // PLATFORM(BCM_RPI)

#endif // BCMRPiSurface_h
