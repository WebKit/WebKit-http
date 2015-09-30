#ifndef BCMRPiSurface_h
#define BCMRPiSurface_h

#if PLATFORM(BCM_RPI)

#include <EGL/egl.h>
#include <tuple>

namespace WebCore {

class GLContext;
class IntSize;

class BCMRPiSurface {
public:
    BCMRPiSurface(const IntSize&, uint32_t);

    std::unique_ptr<GLContext> createGLContext();

    using BCMBufferExport = std::tuple<uint32_t, uint32_t, uint32_t>;
    BCMBufferExport lockFrontBuffer();
    void releaseBuffer(uint32_t);

private:
    EGL_DISPMANX_WINDOW_T m_nativeWindow;
};

} // namespace WebCore

#endif // PLATFORM(BCM_RPI)

#endif // BCMRPiSurface_h
