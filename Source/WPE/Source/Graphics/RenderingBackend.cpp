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

#include "Config.h"

#include "RenderingBackendBCMNexus.h"
#include "RenderingBackendBCMRPi.h"
#include "RenderingBackendIntelCE.h"
#include "RenderingBackendSTM.h"
#include <cstdio>

#if WPE_BUFFER_MANAGEMENT(GBM)
#include "RenderingBackendGBM.h"
#endif

#if WPE_BUFFER_MANAGEMENT(BCM_RPI)
#include "RenderingBackendBCMRPiBM.h"
#endif

#if WPE_BUFFER_MANAGEMENT(BCM_NEXUS)
#include "RenderingBackendBCMNexusBM.h"
#endif

#if WPE_BACKEND(WESTEROS)
#include "RenderingBackendWesteros.h"
#endif

#if WPE_BACKEND(STM)
#include "RenderingBackendSTM.h"
#endif

namespace WPE {

namespace Graphics {

std::unique_ptr<RenderingBackend> RenderingBackend::create(const uint8_t* data, size_t size)
{
#if WPE_BUFFER_MANAGEMENT(BCM_NEXUS)
    return std::unique_ptr<RenderingBackendBCMNexusBM>(new RenderingBackendBCMNexusBM(data, size));
#else
    (void)data;
    (void)size;
#endif

#if WPE_BUFFER_MANAGEMENT(GBM)
    return std::unique_ptr<RenderingBackendGBM>(new RenderingBackendGBM);
#endif

#if WPE_BUFFER_MANAGEMENT(BCM_RPI)
    return std::unique_ptr<RenderingBackendBCMRPiBM>(new RenderingBackendBCMRPiBM);
#endif

#if WPE_BACKEND(BCM_NEXUS)
    return std::unique_ptr<RenderingBackendBCMNexus>(new RenderingBackendBCMNexus);
#endif

#if WPE_BACKEND(BCM_RPI)
    return std::unique_ptr<RenderingBackendBCMRPi>(new RenderingBackendBCMRPi);
#endif

#if WPE_BACKEND(INTEL_CE)
    return std::unique_ptr<RenderingBackendIntelCE>(new RenderingBackendIntelCE);
#endif

#if WPE_BACKEND(WESTEROS)
    return std::unique_ptr<RenderingBackendWesteros>(new RenderingBackendWesteros);
#endif

#if WPE_BACKEND(STM)
    return std::unique_ptr<RenderingBackendSTM>(new RenderingBackendSTM);
#endif

    fprintf(stderr, "RenderingBackend: no usable backend found, will crash.\n");
    return nullptr;
}

RenderingBackend::~RenderingBackend() = default;

RenderingBackend::Surface::~Surface() = default;

RenderingBackend::OffscreenSurface::~OffscreenSurface() = default;

} // namespace Graphics

} // namespace WPE
