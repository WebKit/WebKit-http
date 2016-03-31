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

#include "Config.h"
#include "RenderingBackendIntelCE.h"

#if WPE_BACKEND(INTEL_CE)

#include <EGL/egl.h>
#include <libgdl.h>

namespace WPE {

namespace Graphics {

RenderingBackendIntelCE::RenderingBackendIntelCE() = default;

RenderingBackendIntelCE::~RenderingBackendIntelCE() = default;

EGLNativeDisplayType RenderingBackendIntelCE::nativeDisplay()
{
    return EGL_DEFAULT_DISPLAY;
}

std::unique_ptr<RenderingBackend::Surface> RenderingBackendIntelCE::createSurface(uint32_t width, uint32_t height, uint32_t targetHandle, RenderingBackend::Surface::Client& client)
{
    return std::unique_ptr<RenderingBackendIntelCE::Surface>(new RenderingBackendIntelCE::Surface(*this, width, height, targetHandle, client));
}

std::unique_ptr<RenderingBackend::OffscreenSurface> RenderingBackendIntelCE::createOffscreenSurface()
{
    return std::unique_ptr<RenderingBackendIntelCE::OffscreenSurface>(new RenderingBackendIntelCE::OffscreenSurface(*this));
}

RenderingBackendIntelCE::Surface::Surface(const RenderingBackendIntelCE&, uint32_t width, uint32_t height, uint32_t targetHandle, RenderingBackendIntelCE::Surface::Client&)
{
    m_bufferData.handle = targetHandle;
    m_bufferData.width = width;
    m_bufferData.height = height;
    m_bufferData.magic = BufferDataIntelCE::magicValue;
}

RenderingBackendIntelCE::Surface::~Surface() = default;

EGLNativeWindowType RenderingBackendIntelCE::Surface::nativeWindow()
{
    return (EGLNativeWindowType)GDL_PLANE_ID_UPP_D;
}

void RenderingBackendIntelCE::Surface::resize(uint32_t, uint32_t)
{
}

RenderingBackend::BufferExport RenderingBackendIntelCE::Surface::lockFrontBuffer()
{
    return std::make_tuple(-1, reinterpret_cast<uint8_t*>(&m_bufferData), sizeof(BufferDataIntelCE));
}

void RenderingBackendIntelCE::Surface::releaseBuffer(uint32_t)
{
}

RenderingBackendIntelCE::OffscreenSurface::OffscreenSurface(const RenderingBackendIntelCE&)
{
}

RenderingBackendIntelCE::OffscreenSurface::~OffscreenSurface() = default;

EGLNativeWindowType RenderingBackendIntelCE::OffscreenSurface::nativeWindow()
{
    // This will in turn create a pbuffer-based GL context.
    return nullptr;
}

} // namespace Graphics

} // namespace WPE

#endif // WPE_BACKEND(INTEL_CE)
