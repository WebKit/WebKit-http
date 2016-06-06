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

#ifndef WPE_Graphics_Wayland_BufferFactory_h
#define WPE_Graphics_Wayland_BufferFactory_h

#include <memory>
#include <utility>

struct wl_buffer;

namespace WPE {

namespace Graphics {

class BufferFactory {
public:
    static std::unique_ptr<BufferFactory> create();

    virtual std::pair<bool, std::pair<uint32_t, uint32_t>> preferredSize() = 0;
    virtual std::pair<const uint8_t*, size_t> authenticate() = 0;
    virtual uint32_t constructRenderingTarget(uint32_t, uint32_t) = 0;
    virtual std::pair<bool, std::pair<uint32_t, struct wl_buffer*>> createBuffer(int, const uint8_t*, size_t) = 0;
    virtual void destroyBuffer(uint32_t, struct wl_buffer*) = 0;
};

} // namespace Graphics

} // namespace WPE

#endif // WPE_Graphics_Wayland_BufferFactory_h
