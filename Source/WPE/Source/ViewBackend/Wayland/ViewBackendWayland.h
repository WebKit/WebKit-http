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

#ifndef WPE_ViewBackend_ViewBackendWayland_h
#define WPE_ViewBackend_ViewBackendWayland_h

#if WPE_BACKEND(WAYLAND)

#include <WPE/ViewBackend/ViewBackend.h>
#include <unordered_map>

struct ivi_surface;
struct wl_buffer;
struct wl_callback;
struct wl_surface;
struct xdg_surface;

namespace WPE {

namespace Input {
class Client;
}

namespace ViewBackend {

class Client;
class WaylandDisplay;

class ViewBackendWayland final : public ViewBackend {
public:
    ViewBackendWayland();
    virtual ~ViewBackendWayland();

    void setClient(Client* client) override;
    uint32_t constructRenderingTarget(uint32_t, uint32_t) override;
    void commitBuffer(int, const uint8_t* data, size_t size) override;
    void destroyBuffer(uint32_t handle) override;

    void setInputClient(Input::Client*) override;

    struct BufferListenerData {
        Client* client;
        std::unordered_map<uint32_t, struct wl_buffer*> map;
    };

    struct CallbackListenerData {
        Client* client;
        struct wl_callback* frameCallback;
    };

    struct ResizingData {
        Client* client;
        uint32_t width;
        uint32_t height;
    };

private:
    WaylandDisplay& m_display;

    struct wl_surface* m_surface { nullptr };
    struct xdg_surface* m_xdgSurface { nullptr };
    struct ivi_surface* m_iviSurface { nullptr };

    BufferListenerData m_bufferData { nullptr, decltype(m_bufferData.map){ } };
    CallbackListenerData m_callbackData { nullptr, nullptr };
    ResizingData m_resizingData { nullptr, 0, 0 };
};

} // namespace ViewBackend

} // namespace WPE

#endif // WPE_BACKEND(WAYLAND)

#endif // WPE_ViewBackend_ViewBackendWayland_h
