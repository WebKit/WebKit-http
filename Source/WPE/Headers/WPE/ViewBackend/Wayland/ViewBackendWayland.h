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

#ifndef WPE_ViewBackend_ViewBackendWayland_h
#define WPE_ViewBackend_ViewBackendWayland_h

#include <WPE/ViewBackend/ViewBackend.h>

#include <unordered_map>
#include <utility>

struct wl_buffer;
struct wl_callback;
struct wl_keyboard;
struct wl_pointer;
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
    ~ViewBackendWayland();

    void setClient(Client* client) override;
    void commitPrimeFD(int fd, uint32_t handle, uint32_t width, uint32_t height, uint32_t stride, uint32_t format) override;

    void setInputClient(Input::Client*) override;

    struct SeatData {
        Input::Client* client;
        std::pair<int, int> pointerCoords;
        struct wl_pointer* pointer;
        struct wl_keyboard* keyboard;
    };

    struct BufferListenerData {
        Client* client;
        std::unordered_map<uint32_t, struct wl_buffer*> map;
    };

    struct CallbackListenerData {
        Client* client;
        struct wl_callback* frameCallback;
    };

private:
    const WaylandDisplay& m_display;

    SeatData m_seatData;
    BufferListenerData m_bufferData;
    CallbackListenerData m_callbackData;

    struct wl_surface* m_surface;
    struct xdg_surface* m_xdgSurface;
};

} // namespace ViewBackend

} // namespace WPE

#endif // WPE_ViewBackend_ViewBackendWayland_h
