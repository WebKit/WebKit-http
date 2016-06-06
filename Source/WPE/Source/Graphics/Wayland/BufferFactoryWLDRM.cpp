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
#include "BufferFactoryWLDRM.h"

#if WPE_BUFFER_MANAGEMENT(GBM)

#include "BufferDataGBM.h"
#include "WaylandDisplay.h"
#include "wayland-drm-client-protocol.h"

namespace WPE {

namespace Graphics {

BufferFactoryWLDRM::~BufferFactoryWLDRM() = default;

uint32_t BufferFactoryWLDRM::constructRenderingTarget(uint32_t, uint32_t)
{
    // This is for now meaningless for this factory.
    return 0;
}

std::pair<bool, std::pair<uint32_t, struct wl_buffer*>> BufferFactoryWLDRM::createBuffer(int fd, const uint8_t* data, size_t size)
{
    std::pair<bool, std::pair<uint32_t, struct wl_buffer*>> result = { false, { 0, nullptr } };

    if (!data || size != sizeof(BufferDataGBM))
        return result;

    auto& bufferData = *reinterpret_cast<const BufferDataGBM*>(data);
    if (bufferData.magic != BufferDataGBM::magicValue)
        return result;

    result = { true, { bufferData.handle, nullptr } };

    if (fd >= 0)
        result.second.second = wl_drm_create_prime_buffer(ViewBackend::WaylandDisplay::singleton().interfaces().drm,
            fd, bufferData.width, bufferData.height,
            WL_DRM_FORMAT_ARGB8888, 0, bufferData.stride, 0, 0, 0, 0);
    
    return result;
}

void BufferFactoryWLDRM::destroyBuffer(uint32_t, struct wl_buffer*)
{
}

} // namespace Graphics

} // namespace WPE

#endif // WPE_BUFFER_MANAGEMENT(GBM)
