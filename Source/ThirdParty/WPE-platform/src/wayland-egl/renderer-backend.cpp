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

#include <wpe/renderer-backend-egl.h>

#include "display.h"
#include "ipc.h"
#include "ipc-waylandegl.h"
#include <wayland-client-protocol.h>
#include <wayland-egl.h>

namespace WaylandEGL {

struct Backend {
    Backend();
    ~Backend();

    Wayland::Display& display;
};

Backend::Backend()
    : display(Wayland::Display::singleton())
{
}

Backend::~Backend()
{
}

struct EGLTarget : public IPC::Client::Handler {
    EGLTarget(struct wpe_renderer_backend_egl_target*, int);
    virtual ~EGLTarget();

    void initialize(Backend& backend, uint32_t width, uint32_t height);
    // IPC::Client::Handler
    void handleMessage(char* data, size_t size) override;

    struct wpe_renderer_backend_egl_target* target;
    IPC::Client ipcClient;

    struct wl_surface* m_surface { nullptr };
    struct wl_shell_surface *m_shellSurface { nullptr };
    struct wl_egl_window* m_window { nullptr };
    Backend* m_backend { nullptr };
};

static void
handle_ping(void *data, struct wl_shell_surface *shell_surface,
                                                       uint32_t serial)
{
       wl_shell_surface_pong(shell_surface, serial);
}

static void
handle_configure(void *data, struct wl_shell_surface *shell_surface,
                uint32_t edges, int32_t width, int32_t height)
{
}

static void
handle_popup_done(void *data, struct wl_shell_surface *shell_surface)
{
}

static const struct wl_shell_surface_listener shell_surface_listener = {
       handle_ping,
       handle_configure,
       handle_popup_done
};

EGLTarget::EGLTarget(struct wpe_renderer_backend_egl_target* target, int hostFd)
    : target(target)
{
    ipcClient.initialize(*this, hostFd);
    Wayland::EventDispatcher::singleton().setIPC( ipcClient );
}

void EGLTarget::initialize(Backend& backend, uint32_t width, uint32_t height)
{
    m_backend = &backend;
    m_surface = wl_compositor_create_surface(m_backend->display.interfaces().compositor);

    if (!m_surface) {
        fprintf(stderr, "EGLTarget: unable to create wayland surface\n");
        return;
    }

    if (m_backend->display.interfaces().shell) {
        m_shellSurface = wl_shell_get_shell_surface(m_backend->display.interfaces().shell, m_surface);
        if (m_shellSurface) {
            wl_shell_surface_add_listener(m_shellSurface,
                                          &shell_surface_listener, NULL);
            // wl_shell_surface_set_toplevel(m_shellSurface);
            wl_shell_surface_set_fullscreen(m_shellSurface, WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT, 0, NULL);
        }
    }
    struct wl_region *region;
    region = wl_compositor_create_region(m_backend->display.interfaces().compositor);
    wl_region_add(region, 0, 0,
                   width,
                   height);
    wl_surface_set_opaque_region(m_surface, region);

    m_window = wl_egl_window_create(m_surface, width, height);
}

EGLTarget::~EGLTarget()
{
    ipcClient.deinitialize();

    if (m_window)
        wl_egl_window_destroy(m_window);
    m_window = nullptr;
    if (m_shellSurface)
        wl_shell_surface_destroy(m_shellSurface);
    m_shellSurface = nullptr;
    if (m_surface)
        wl_surface_destroy(m_surface);
    m_surface = nullptr;
}

void EGLTarget::handleMessage(char* data, size_t size)
{
    if (size != IPC::Message::size)
        return;

    auto& message = IPC::Message::cast(data);
    switch (message.messageCode) {
    case IPC::WaylandEGL::FrameComplete::code:
    {
        wpe_renderer_backend_egl_target_dispatch_frame_complete(target);
        break;
    }
    default:
        fprintf(stderr, "EGLTarget: unhandled message\n");
    };
}

} // namespace WaylandEGL

extern "C" {

struct wpe_renderer_backend_egl_interface wayland_egl_renderer_backend_egl_interface = {
    // create
    [](int) -> void*
    {
        return new WaylandEGL::Backend;
    },
    // destroy
    [](void* data)
    {
        auto* backend = static_cast<WaylandEGL::Backend*>(data);
        delete backend;
    },
    // get_native_display
    [](void* data) -> EGLNativeDisplayType
    {
        auto& backend = *static_cast<WaylandEGL::Backend*>(data);
        return backend.display.display();
    },
};

struct wpe_renderer_backend_egl_target_interface wayland_egl_renderer_backend_egl_target_interface = {
    // create
    [](struct wpe_renderer_backend_egl_target* target, int host_fd) -> void*
    {
        return new WaylandEGL::EGLTarget(target, host_fd);
    },
    // destroy
    [](void* data)
    {
        auto* target = static_cast<WaylandEGL::EGLTarget*>(data);
        delete target;
    },
    // initialize
    [](void* data, void* backend_data, uint32_t width, uint32_t height)
    {
        auto& target = *static_cast<WaylandEGL::EGLTarget*>(data);
        auto& backend = *static_cast<WaylandEGL::Backend*>(backend_data);
        target.initialize(backend, width, height);
    },
    // get_native_window
    [](void* data) -> EGLNativeWindowType
    {
        auto& target = *static_cast<WaylandEGL::EGLTarget*>(data);
        return target.m_window;
    },
    // resize
    [](void* data, uint32_t width, uint32_t height)
    {
    },
    // frame_will_render
    [](void* data)
    {
    },
    // frame_rendered
    [](void* data)
    {
        auto& target = *static_cast<WaylandEGL::EGLTarget*>(data);

        wl_display *display = target.m_backend->display.display();
        if(display)
            wl_display_flush(display);

        IPC::Message message;
        IPC::WaylandEGL::BufferCommit::construct(message);
        target.ipcClient.sendMessage(IPC::Message::data(message), IPC::Message::size);
    },
};

struct wpe_renderer_backend_egl_offscreen_target_interface wayland_egl_renderer_backend_egl_offscreen_target_interface = {
    // create
    []() -> void*
    {
        return nullptr;
    },
    // destroy
    [](void* data)
    {
    },
    // initialize
    [](void* data, void* backend_data)
    {
    },
    // get_native_window
    [](void* data) -> EGLNativeWindowType
    {
        return nullptr;
    },
};

}
