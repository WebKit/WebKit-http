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
#include <WPE/Graphics/RenderingBackend.h>

#include "RenderingBackendBCMNexus.h"
#include "RenderingBackendBCMRPi.h"
#include "RenderingBackendGBM.h"

namespace WPE {

namespace Graphics {

std::unique_ptr<RenderingBackend> RenderingBackend::create()
{
#if WPE_BACKEND(DRM) || WPE_BACKEND(WAYLAND)
    return std::unique_ptr<RenderingBackendGBM>(new RenderingBackendGBM);
#endif

#if WPE_BACKEND(BCM_NEXUS)
    return std::unique_ptr<RenderingBackendBCMNexus>(new RenderingBackendBCMNexus);
#endif

#if WPE_BACKEND(BCM_RPI)
    return std::unique_ptr<RenderingBackendBCMRPi>(new RenderingBackendBCMRPi);
#endif

    return nullptr;
}

RenderingBackend::~RenderingBackend() = default;

RenderingBackend::Surface::~Surface() = default;

RenderingBackend::OffscreenSurface::~OffscreenSurface() = default;

} // namespace Graphics

} // namespace WPE
