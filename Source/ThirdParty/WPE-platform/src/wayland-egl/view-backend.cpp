/*
 * Copyright (C) 2015, 2016 Igalia S.L.
 * Copyright (C) 2015, 2016 Metrological
 * Copyright (C) 2016 SoftAtHome
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

#include <wpe/input.h>
#include <wpe/view-backend.h>
#include "display.h"
#include "ipc.h"
#include "ipc-waylandegl.h"

#define WIDTH 1280
#define HEIGHT 720

namespace WaylandEGL {

struct ViewBackend;

struct ViewBackend : public IPC::Host::Handler {
    ViewBackend(struct wpe_view_backend*);
    virtual ~ViewBackend();

    // IPC::Host::Handler
    void handleFd(int) override { };
    void handleMessage(char*, size_t) override;

    void ackBufferCommit();
    void initialize();

    struct wpe_view_backend* backend;
    IPC::Host ipcHost;
};

ViewBackend::ViewBackend(struct wpe_view_backend* backend)
    : backend(backend)
{
    ipcHost.initialize(*this);
}

ViewBackend::~ViewBackend()
{
    ipcHost.deinitialize();
}

void ViewBackend::handleMessage(char* data, size_t size)
{
    if (size != IPC::Message::size)
        return;

    auto& message = IPC::Message::cast(data);
    switch (message.messageCode) {
    case Wayland::EventDispatcher::MsgType::AXIS:
    {
        struct wpe_input_axis_event * event = reinterpret_cast<wpe_input_axis_event*>(std::addressof(message.messageData));
        wpe_view_backend_dispatch_axis_event(backend, event);
        break;
    }
    case Wayland::EventDispatcher::MsgType::POINTER:
    {
        struct wpe_input_pointer_event * event = reinterpret_cast<wpe_input_pointer_event*>(std::addressof(message.messageData));
        wpe_view_backend_dispatch_pointer_event(backend, event);
        break;
    }
    case Wayland::EventDispatcher::MsgType::TOUCH:
    {
        struct wpe_input_touch_event * event = reinterpret_cast<wpe_input_touch_event*>(std::addressof(message.messageData));
        wpe_view_backend_dispatch_touch_event(backend, event);
        break;
    }
    case Wayland::EventDispatcher::MsgType::KEYBOARD:
    {
        struct wpe_input_keyboard_event * event = reinterpret_cast<wpe_input_keyboard_event*>(std::addressof(message.messageData));
        wpe_view_backend_dispatch_keyboard_event(backend, event);
        break;
    }
    case IPC::WaylandEGL::BufferCommit::code:
    {
        ackBufferCommit();
        break;
    }
    default:
        fprintf(stderr, "ViewBackend: unhandled message\n");
    }
}

void ViewBackend::initialize()
{
    wpe_view_backend_dispatch_set_size( backend, WIDTH, HEIGHT );
}

void ViewBackend::ackBufferCommit()
{
    IPC::Message message;
    IPC::WaylandEGL::FrameComplete::construct(message);
    ipcHost.sendMessage(IPC::Message::data(message), IPC::Message::size);

    wpe_view_backend_dispatch_frame_displayed(backend);
}

} // namespace WaylandEGL

extern "C" {

struct wpe_view_backend_interface wayland_egl_view_backend_interface = {
    // create
    [](void*, struct wpe_view_backend* backend) -> void*
    {
        return new WaylandEGL::ViewBackend(backend);
    },
    // destroy
    [](void* data)
    {
        auto* backend = static_cast<WaylandEGL::ViewBackend*>(data);
        delete backend;
    },
    // initialize
    [](void* data)
    {
        auto& backend = *static_cast<WaylandEGL::ViewBackend*>(data);
        backend.initialize();
    },
    // get_renderer_host_fd
    [](void* data) -> int
    {
        auto& backend = *static_cast<WaylandEGL::ViewBackend*>(data);
        return backend.ipcHost.releaseClientFD();
    },
};

}
