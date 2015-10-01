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

#include "GBMSurface.h"
#include <tuple>
#include <wtf/HashMap.h>

struct gbm_bo;
struct gbm_device;

namespace WebCore {

class GLContext;
class GLContextEGL;
class IntSize;

class PlatformDisplayGBM final : public PlatformDisplay {
public:
    PlatformDisplayGBM();
    virtual ~PlatformDisplayGBM();

    std::unique_ptr<GBMSurface> createSurface(const IntSize&, GBMSurface::Client&);
    std::unique_ptr<GLContextEGL> createOffscreenContext(GLContext*);

    using GBMBufferExport = std::tuple<int, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, int>;
    bool hasFreeBuffers(GBMSurface&);
    GBMBufferExport lockFrontBuffer(GBMSurface&);
    void releaseBuffer(GBMSurface&, uint32_t);

private:
    Type type() const override { return PlatformDisplay::Type::GBM; }

    struct {
        int fd;
        struct gbm_device* device;
    } m_gbm;

    static void boDataDestroyCallback(struct gbm_bo*, void*);
    HashMap<uint32_t, struct gbm_bo*> m_lockedBuffers;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_PLATFORM_DISPLAY(PlatformDisplayGBM, GBM)

#endif // PLATFORM(GBM)

#endif // PlatformDisplayGBM_h
