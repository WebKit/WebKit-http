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

#include <WPE/ViewBackend/ViewBackend.h>

#include <cstring>
#include <cstdlib>

#if WPE_PLATFORM_BCM_RPI
#include "ViewBackendBCMRPi.h"
#endif
#if WPE_PLATFORM_DRM
#include "ViewBackendDRM.h"
#endif
#if WPE_PLATFORM_WAYLAND
#include "ViewBackendWayland.h"
#endif

namespace WPE {

namespace ViewBackend {

std::unique_ptr<ViewBackend> ViewBackend::create()
{
    auto* backendEnv = std::getenv("WPE_BACKEND");

#if WPE_PLATFORM_WAYLAND
    if (std::getenv("WAYLAND_DISPLAY") || (backendEnv && !std::strcmp(backendEnv, "wayland")))
        return std::unique_ptr<ViewBackendWayland>(new ViewBackendWayland);
#endif

#if WPE_PLATFORM_DRM
    if (backendEnv && !std::strcmp(backendEnv, "drm"))
        return std::unique_ptr<ViewBackendDRM>(new ViewBackendDRM);
#endif

#if WPE_PLATFORM_BCM_RPI
    if (!backendEnv || !std::strcmp(backendEnv, "rpi"))
        return std::unique_ptr<ViewBackendBCMRPi>(new ViewBackendBCMRPi);
#endif

    return nullptr;
}

void ViewBackend::setClient(Client*)
{
}

void ViewBackend::commitPrimeBuffer(int, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)
{
}

void ViewBackend::destroyPrimeBuffer(uint32_t)
{
}

uint32_t ViewBackend::createBCMElement(int32_t, int32_t)
{
    return 0;
}

void ViewBackend::commitBCMBuffer(uint32_t, uint32_t, uint32_t)
{
}

void ViewBackend::setInputClient(Input::Client*)
{
}

} // namespace ViewBackend

} // namespace WPE
