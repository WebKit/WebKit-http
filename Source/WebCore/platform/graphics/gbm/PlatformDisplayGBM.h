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

#ifndef PlatformDisplayGBM_h
#define PlatformDisplayGBM_h

#if PLATFORM(GBM)

#include "PlatformDisplay.h"

#include <gbm.h>
#include <EGL/egl.h>

struct gbm_device;

namespace WebCore {

class GBMSurface;
class GLContext;
class GLContextEGL;
class IntSize;

class PlatformDisplayGBM final : public PlatformDisplay {
public:
    PlatformDisplayGBM();
    virtual ~PlatformDisplayGBM();

    std::unique_ptr<GBMSurface> createSurface(const IntSize&);
    std::unique_ptr<GLContextEGL> createOffscreenContext(GLContext*);

    int lockFrontBuffer(GBMSurface&);

private:
    Type type() const override { return PlatformDisplay::Type::GBM; }

    struct {
        int fd;
        struct gbm_device* device;
    } m_gbm;

    EGLConfig m_eglConfig;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_PLATFORM_DISPLAY(PlatformDisplayGBM, GBM)

#endif // PLATFORM(GBM)

#endif // PlatformDisplayGBM_h
