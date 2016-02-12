/*
 * Copyright (C) 2015 Igalia S.L.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PlatformDisplayWPE.h"

#if PLATFORM(WPE)

#include "GLContextEGL.h"
#include "IntSize.h"

namespace WebCore {

static std::pair<const uint8_t*, size_t>* s_initializationData = nullptr;

void PlatformDisplayWPE::initialize(std::pair<const uint8_t*, size_t> data)
{
    auto initializationData = std::make_unique<std::pair<const uint8_t*, size_t>>(data);
    s_initializationData = initializationData.get();

    // This will construct the PlatformDisplayWPE object using the initialization data.
    PlatformDisplayWPE::sharedDisplay();

    s_initializationData = nullptr;
}

PlatformDisplayWPE::PlatformDisplayWPE()
{
    std::pair<const uint8_t*, size_t> initializationData = { nullptr, 0 };
    if (s_initializationData)
        initializationData = *s_initializationData;
    m_backend = WPE::Graphics::RenderingBackend::create(initializationData.first, initializationData.second);

    m_eglDisplay = eglGetDisplay(m_backend->nativeDisplay());
    if (m_eglDisplay == EGL_NO_DISPLAY) {
        fprintf(stderr, "PlatformDisplayWPE: could not create the EGL display.\n");
        return;
    }

    PlatformDisplay::initializeEGLDisplay();
}

PlatformDisplayWPE::~PlatformDisplayWPE() = default;

std::unique_ptr<PlatformDisplayWPE::Surface> PlatformDisplayWPE::createSurface(const IntSize& size, uint32_t targetHandle, PlatformDisplayWPE::Surface::Client& client)
{
    return std::make_unique<Surface>(*this, size, targetHandle, client);
}

std::unique_ptr<GLContextEGL> PlatformDisplayWPE::createOffscreenContext(GLContext* sharingContext)
{
    struct OffscreenContextData final : public GLContext::Data {
    public:
        virtual ~OffscreenContextData() = default;

        std::unique_ptr<WPE::Graphics::RenderingBackend::OffscreenSurface> surface;
    };

    auto contextData = std::make_unique<OffscreenContextData>();
    contextData->surface = m_backend->createOffscreenSurface();

    EGLNativeWindowType nativeWindow = contextData->surface->nativeWindow();
    return GLContextEGL::createContext(nativeWindow, sharingContext, WTFMove(contextData));
}

PlatformDisplayWPE::Surface::Surface(const PlatformDisplayWPE& display, const IntSize& size, uint32_t targetHandle, Client& client)
{
    m_backend = display.m_backend->createSurface(std::max(0, size.width()), std::max(0, size.height()), targetHandle, client);
}

void PlatformDisplayWPE::Surface::resize(const IntSize& size)
{
    m_backend->resize(std::max(0, size.width()), std::max(0, size.height()));
}

std::unique_ptr<GLContextEGL> PlatformDisplayWPE::Surface::createGLContext() const
{
    return GLContextEGL::createWindowContext(m_backend->nativeWindow(), GLContext::sharingContext());
}

PlatformDisplayWPE::BufferExport PlatformDisplayWPE::Surface::lockFrontBuffer()
{
    return m_backend->lockFrontBuffer();
}

void PlatformDisplayWPE::Surface::releaseBuffer(uint32_t handle)
{
    m_backend->releaseBuffer(handle);
}

} // namespace WebCore

#endif // PLATFORM(WPE)
