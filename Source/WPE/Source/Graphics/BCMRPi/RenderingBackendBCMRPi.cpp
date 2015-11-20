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
#include "RenderingBackendBCMRPi.h"

#if WPE_BACKEND(BCM_RPI)

#include <EGL/egl.h>

namespace WPE {

namespace Graphics {

RenderingBackendBCMRPi::RenderingBackendBCMRPi() = default;

RenderingBackendBCMRPi::~RenderingBackendBCMRPi() = default;

EGLNativeDisplayType RenderingBackendBCMRPi::nativeDisplay()
{
    return EGL_DEFAULT_DISPLAY;
}

std::unique_ptr<RenderingBackend::Surface> RenderingBackendBCMRPi::createSurface(uint32_t width, uint32_t height, uint32_t targetHandle, RenderingBackend::Surface::Client& client)
{
    return std::unique_ptr<RenderingBackendBCMRPi::Surface>(new RenderingBackendBCMRPi::Surface(*this, width, height, targetHandle, client));
}

std::unique_ptr<RenderingBackend::OffscreenSurface> RenderingBackendBCMRPi::createOffscreenSurface()
{
    return std::unique_ptr<RenderingBackendBCMRPi::OffscreenSurface>(new RenderingBackendBCMRPi::OffscreenSurface(*this));
}

RenderingBackendBCMRPi::Surface::Surface(const RenderingBackendBCMRPi&, uint32_t width, uint32_t height, uint32_t targetHandle, RenderingBackendBCMRPi::Surface::Client&)
{
    m_nativeWindow.element = targetHandle;
    m_nativeWindow.width = width;
    m_nativeWindow.height = height;

    m_bufferData.handle = targetHandle;
    m_bufferData.width = width;
    m_bufferData.height = height;
    m_bufferData.magic = BufferDataBCMRPi::magicValue;
}

RenderingBackendBCMRPi::Surface::~Surface() = default;

EGLNativeWindowType RenderingBackendBCMRPi::Surface::nativeWindow()
{
    return &m_nativeWindow;
}

void RenderingBackendBCMRPi::Surface::resize(uint32_t, uint32_t)
{
}

RenderingBackend::BufferExport RenderingBackendBCMRPi::Surface::lockFrontBuffer()
{
    return std::make_tuple(-1, reinterpret_cast<uint8_t*>(&m_bufferData), sizeof(BufferDataBCMRPi));
}

void RenderingBackendBCMRPi::Surface::releaseBuffer(uint32_t)
{
}

RenderingBackendBCMRPi::OffscreenSurface::OffscreenSurface(const RenderingBackendBCMRPi&)
{
}

RenderingBackendBCMRPi::OffscreenSurface::~OffscreenSurface() = default;

EGLNativeWindowType RenderingBackendBCMRPi::OffscreenSurface::nativeWindow()
{
    // This will in turn create a pbuffer-based GL context.
    return nullptr;
}

} // namespace Graphics

} // namespace WPE

#endif // WPE_BACKEND(BCM_RPI)
