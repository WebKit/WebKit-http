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

#ifndef PlatformDisplayWPE_h
#define PlatformDisplayWPE_h

#if PLATFORM(WPE)

#if PLATFORM(GBM)
#include <gbm.h>
#endif

#include <WPE/Graphics/RenderingBackend.h>
#include "PlatformDisplay.h"

namespace WebCore {

class GLContext;
class GLContextEGL;
class IntSize;

class PlatformDisplayWPE final : public PlatformDisplay {
public:
    PlatformDisplayWPE();
    virtual ~PlatformDisplayWPE();

    using BufferExport = WPE::Graphics::RenderingBackend::BufferExport;

    class Surface {
    public:
        using Client = WPE::Graphics::RenderingBackend::Surface::Client;

        Surface(const PlatformDisplayWPE&, const IntSize&, Client&);

        void resize(const IntSize&);
        std::unique_ptr<GLContextEGL> createGLContext() const;

        BufferExport lockFrontBuffer();
        void releaseBuffer(uint32_t);

    private:
        std::unique_ptr<WPE::Graphics::RenderingBackend::Surface> m_backend;
    };

    std::unique_ptr<Surface> createSurface(const IntSize&, Surface::Client&);

    std::unique_ptr<GLContextEGL> createOffscreenContext(GLContext*);

private:
    Type type() const override { return PlatformDisplay::Type::WPE; }

    std::unique_ptr<WPE::Graphics::RenderingBackend> m_backend;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_PLATFORM_DISPLAY(PlatformDisplayWPE, WPE)

#endif // PLATFORM(WPE)

#endif // PlatformDisplayWPE_h
