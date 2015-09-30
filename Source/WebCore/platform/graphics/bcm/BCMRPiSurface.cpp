#include "config.h"
#include "BCMRPiSurface.h"

#if PLATFORM(BCM_RPI)

#include "GLContextEGL.h"
#include "IntSize.h"

namespace WebCore {

BCMRPiSurface::BCMRPiSurface(const IntSize& size, uint32_t elementHandle)
{
    fprintf(stderr, "BCMRPiSurface: ctor for %p, size (%d,%d), elementHandle %u\n",
        this, size.width(), size.height(), elementHandle);

    m_nativeWindow.element = elementHandle;
    m_nativeWindow.width = size.width();
    m_nativeWindow.height = size.height();

    fprintf(stderr, "BCMRPiSurface: all ok :|\n");
}

std::unique_ptr<GLContext> BCMRPiSurface::createGLContext()
{
    return GLContextEGL::createWindowContext(&m_nativeWindow, GLContext::sharingContext());
}

BCMRPiSurface::BCMBufferExport BCMRPiSurface::lockFrontBuffer()
{
    return BCMBufferExport{ m_nativeWindow.element, m_nativeWindow.width, m_nativeWindow.height };
}

void BCMRPiSurface::releaseBuffer(uint32_t)
{
}

} // namespace WebCore

#endif // PLATFORM(BCM_RPI)
