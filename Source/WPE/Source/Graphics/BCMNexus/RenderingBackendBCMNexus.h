/*
 * Copyright (C) 2015 Igalia S.L.
 * Copyright (C) 2015 Metrological
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WPE_Graphics_RenderingBackendBCMNexus_h
#define WPE_Graphics_RenderingBackendBCMNexus_h

#if WPE_BACKEND(BCM_NEXUS)

#include "BufferDataBCMNexus.h"
#include <WPE/Graphics/RenderingBackend.h>

#include <refsw/nexus_config.h>
#include <refsw/nexus_platform.h>
#include <refsw/nexus_display.h>
#include <refsw/nexus_core_utils.h>
#include <refsw/default_nexus.h>
#include <refsw/nxclient.h>

typedef void* NXPL_PlatformHandle;

namespace WPE {

namespace Graphics {

class RenderingBackendBCMNexus final : public RenderingBackend {
public:
    class Surface final : public RenderingBackend::Surface {
    public:
        Surface(const RenderingBackendBCMNexus&, uint32_t, uint32_t, uint32_t,  Client&);
        virtual ~Surface();

        EGLNativeWindowType nativeWindow() override;
        void resize(uint32_t, uint32_t) override;

        BufferExport lockFrontBuffer() override;
        void releaseBuffer(uint32_t) override;

    public:
        void* m_nativeWindow;
        BufferDataBCMNexus m_bufferData;
    };

    class OffscreenSurface final : public RenderingBackend::OffscreenSurface {
    public:
        OffscreenSurface(const RenderingBackendBCMNexus&);
        virtual ~OffscreenSurface();

        EGLNativeWindowType nativeWindow() override;
    };

    RenderingBackendBCMNexus();
    virtual ~RenderingBackendBCMNexus();

    EGLNativeDisplayType nativeDisplay() override;
    std::unique_ptr<RenderingBackend::Surface> createSurface(uint32_t, uint32_t, uint32_t, RenderingBackend::Surface::Client&) override;
    std::unique_ptr<RenderingBackend::OffscreenSurface> createOffscreenSurface() override;

private:
    NXPL_PlatformHandle m_nxplHandle;
    NxClient_AllocResults m_AllocResults;
    NEXUS_SurfaceClientHandle m_Client;
};

} // namespace Graphics

} // namespace WPE

#endif // WPE_BACKEND(BCM_NEXUS)

#endif // WPE_Graphics_RenderingBackendBCMNexus_h
