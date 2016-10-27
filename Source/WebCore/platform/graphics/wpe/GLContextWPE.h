/*
 * Copyright (C) 2016 Igalia S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 */

#pragma once

#if PLATFORM(WPE)

#include "GLContext.h"

#include <EGL/egl.h>

namespace WebCore {

class GLContextWPE final : public GLContext {
public:
    static std::unique_ptr<GLContextWPE> createContext(PlatformDisplay&, EGLNativeWindowType, bool, std::unique_ptr<GLContext::Data>&& = nullptr);
    static std::unique_ptr<GLContextWPE> createOffscreenContext(PlatformDisplay&);
    static std::unique_ptr<GLContextWPE> createSharingContext(PlatformDisplay&);
    virtual ~GLContextWPE();

private:
    enum EGLSurfaceType { PbufferSurface, WindowSurface };
    GLContextWPE(PlatformDisplay&, EGLContext, EGLSurface, EGLSurfaceType);

    static std::unique_ptr<GLContextWPE> createWindowContext(EGLNativeWindowType, PlatformDisplay&, EGLContext sharingContext = EGL_NO_CONTEXT);
    static std::unique_ptr<GLContextWPE> createPbufferContext(PlatformDisplay&, EGLContext sharingContext = EGL_NO_CONTEXT);

    bool makeContextCurrent() override;
    void swapBuffers() override;
    void waitNative() override;
    bool canRenderToDefaultFramebuffer() override;
    IntSize defaultFrameBufferSize() override;
    void swapInterval(int) override;

    bool isEGLContext() const override;

#if USE(CAIRO)
    cairo_device_t* cairoDevice() override;
#endif

#if ENABLE(GRAPHICS_CONTEXT_3D)
    PlatformGraphicsContext3D platformContext() override;
#endif

    static bool getEGLConfig(EGLDisplay, EGLConfig*, EGLSurfaceType);

    EGLContext m_context { EGL_NO_CONTEXT };
    EGLSurface m_surface { EGL_NO_SURFACE };
    EGLSurfaceType m_type;

#if USE(CAIRO)
    cairo_device_t* m_cairoDevice { nullptr };
#endif
};

} // namespace WebCore

#endif // PLATFORM(WPE)
