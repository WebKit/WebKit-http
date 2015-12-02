/*
 * Copyright (C) 2015 Igalia S.L.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WPE_Graphics_RenderingBackendGBM_h
#define WPE_Graphics_RenderingBackendGBM_h

#if WPE_BACKEND(DRM) || WPE_BACKEND(WAYLAND)

// Included first to set up the EGL platform.
#include <gbm.h>

#include <WPE/Graphics/RenderingBackend.h>
#include <unordered_map>

namespace WPE {

namespace Graphics {

class RenderingBackendGBM final : public RenderingBackend {
public:
    class Surface final : public RenderingBackend::Surface {
    public:
        Surface(const RenderingBackendGBM&, uint32_t, uint32_t, uint32_t, Client&);
        WPE_EXPORT virtual ~Surface();

        EGLNativeWindowType nativeWindow() override;
        void resize(uint32_t, uint32_t) override;

        BufferExport lockFrontBuffer() override;
        void releaseBuffer(uint32_t) override;

    private:
        Client& m_client;
        struct gbm_surface* m_surface;
        std::pair<uint32_t, uint32_t> m_size;
        std::unordered_map<uint32_t, struct gbm_bo*> m_lockedBuffers;
    };

    class OffscreenSurface final : public RenderingBackend::OffscreenSurface {
    public:
        OffscreenSurface(const RenderingBackendGBM&);
        virtual ~OffscreenSurface();

        EGLNativeWindowType nativeWindow() override;

    private:
        struct gbm_surface* m_surface;
    };

    RenderingBackendGBM();
    virtual ~RenderingBackendGBM();

    EGLNativeDisplayType nativeDisplay() override;
    std::unique_ptr<RenderingBackend::Surface> createSurface(uint32_t, uint32_t, uint32_t, RenderingBackend::Surface::Client&) override;
    std::unique_ptr<RenderingBackend::OffscreenSurface> createOffscreenSurface() override;

private:
    friend class Surface;
    friend class OffscreenSurface;

    struct {
        int fd { -1 };
        struct gbm_device* device;
    } m_gbm;
};

} // Graphics

} // WPE

#endif // WPE_BACKEND(DRM) || WPE_BACKEND(WAYLAND)

#endif // WPE_Graphics_RenderingBackendGBM_h
