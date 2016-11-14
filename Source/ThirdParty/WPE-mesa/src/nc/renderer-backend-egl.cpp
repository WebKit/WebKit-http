/*
 * Copyright (C) 2015, 2016 Igalia S.L.
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

#include "nested-compositor.h"

#include <assert.h>
#include <cstdio>
#include <cstring>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <glib.h>
#include <wayland-client.h>
#include <wayland-egl.h>

namespace NC {

struct ClientSource {
    static GSourceFuncs sourceFuncs;

    GSource source;
    GPollFD pfd;
    struct wl_display* display;
};

GSourceFuncs ClientSource::sourceFuncs = {
    // prepare
    [](GSource* base, gint* timeout) -> gboolean
    {
        auto& source = *reinterpret_cast<ClientSource*>(base);
        *timeout = -1;
        wl_display_dispatch_pending(source.display);
        wl_display_flush(source.display);
        return FALSE;
    },
    // check
    [](GSource* base) -> gboolean
    {
        auto& source = *reinterpret_cast<ClientSource*>(base);
        return !!source.pfd.revents;
    },
    // dispatch
    [](GSource* base, GSourceFunc, gpointer) -> gboolean
    {
        auto& source = *reinterpret_cast<ClientSource*>(base);

        if (source.pfd.revents & G_IO_IN)
            wl_display_dispatch(source.display);

        if (source.pfd.revents & (G_IO_ERR | G_IO_HUP))
            return FALSE;

        source.pfd.revents = 0;
        return TRUE;
    },
    nullptr, // finalize
    nullptr, // closure_callback
    nullptr, // closure_marshall
};

class Backend {
public:
    Backend(int);
    ~Backend();

    struct wl_display* display() const { return m_display; }

    struct wl_surface* createSurface() const;

    void initialize();

private:
    static const struct wl_registry_listener s_registryListener;

    struct wl_display* m_display;
    struct wl_registry* m_registry;
    struct wl_compositor* m_compositor;

    GSource* m_source {nullptr};
};

struct EGLTarget {
    EGLTarget(struct wpe_renderer_backend_egl_target* target)
        : target(target)
    {
    }

    ~EGLTarget()
    {
        if (surface)
            wl_surface_destroy(surface);

        if (egl_window)
            wl_egl_window_destroy(egl_window);
    }

    struct wpe_renderer_backend_egl_target* target;

    struct wl_surface* surface {NULL};
    struct wl_egl_window* egl_window {NULL};
};

struct EGLOffscreenTarget {
    EGLOffscreenTarget()
    {
    }

    ~EGLOffscreenTarget()
    {
    }
};

Backend::Backend(int hostFd)
{
    m_display = wl_display_connect_to_fd(hostFd);
    m_registry = wl_display_get_registry(m_display);

    wl_registry_add_listener(m_registry, &s_registryListener, this);
    wl_display_roundtrip(m_display);
}

Backend::~Backend()
{
    if (m_source)
        g_source_unref(m_source);
}

struct wl_surface* Backend::createSurface() const
{
    return wl_compositor_create_surface(m_compositor);
}

void Backend::initialize()
{
    assert(!m_source);

    m_source = g_source_new(&ClientSource::sourceFuncs, sizeof(ClientSource));
    auto& source = *reinterpret_cast<ClientSource*>(m_source);
    source.pfd.fd = wl_display_get_fd(m_display);
    source.pfd.events = G_IO_IN | G_IO_ERR | G_IO_HUP;
    source.pfd.revents = 0;
    source.display = m_display;

    g_source_add_poll(m_source, &source.pfd);
    g_source_set_name(m_source, "NS::ClientSource");
    g_source_set_priority(m_source, G_PRIORITY_DEFAULT);
    g_source_set_can_recurse(m_source, TRUE);
    g_source_attach(m_source, g_main_context_get_thread_default());
}

const struct wl_registry_listener Backend::s_registryListener = {
    // global
    [](void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t)
    {
        auto& backend = *reinterpret_cast<Backend*>(data);

        if (!std::strcmp(interface, "wl_compositor"))
            backend.m_compositor = static_cast<struct wl_compositor*>(wl_registry_bind(registry, name, &wl_compositor_interface, 1));
    },
    // global_remove
    [](void*, struct wl_registry*, uint32_t)
    {
    },
};

} // namespace NC

extern "C" {

struct wpe_renderer_backend_egl_interface nc_renderer_backend_egl_interface = {
    // create
    [](int host_fd) -> void*
    {
        return new NC::Backend(host_fd);
    },
    // destroy
    [](void* data)
    {
        auto* backend = static_cast<NC::Backend*>(data);
        delete backend;
    },
    // get_native_display
    [](void* data) -> EGLNativeDisplayType
    {
        auto& backend = *static_cast<NC::Backend*>(data);
        return (EGLNativeDisplayType)backend.display();
    },
};

struct wpe_renderer_backend_egl_target_interface nc_renderer_backend_egl_target_interface = {
    // create
    [](struct wpe_renderer_backend_egl_target* target, int hostFd) -> void*
    {
        return new NC::EGLTarget(target);
    },
    // destroy
    [](void* data)
    {
        auto* target = static_cast<NC::EGLTarget*>(data);
        delete target;
    },
    // initialize
    [](void* data, void* backend_data, uint32_t width, uint32_t height)
    {
        auto& target = *static_cast<NC::EGLTarget*>(data);
        auto& backend = *static_cast<NC::Backend*>(backend_data);

        backend.initialize();

        target.surface = backend.createSurface();
        target.egl_window = wl_egl_window_create(target.surface,
                width, height);
    },
    // get_native_window
    [](void* data) -> EGLNativeWindowType
    {
        auto& target = *static_cast<NC::EGLTarget*>(data);
        return (EGLNativeWindowType)target.egl_window;
    },
    // resize
    [](void* data, uint32_t width, uint32_t height)
    {
        auto& target = *static_cast<NC::EGLTarget*>(data);
        if (target.egl_window)
            wl_egl_window_resize(target.egl_window, width, height, 0, 0);
    },
    // frame_will_render
    [](void* data)
    {
    },
    // frame_rendered
    [](void* data)
    {
        auto& target = *static_cast<NC::EGLTarget*>(data);
        wpe_renderer_backend_egl_target_dispatch_frame_complete(target.target);
    },
};

struct wpe_renderer_backend_egl_offscreen_target_interface nc_renderer_backend_egl_offscreen_target_interface = {
    // create
    []() -> void*
    {
        return new NC::EGLOffscreenTarget();
    },
    // destroy
    [](void* data)
    {
        auto* target = static_cast<NC::EGLOffscreenTarget*>(data);
        delete target;
    },
    // initialize
    [](void* data, void* backend_data, EGLDisplay display)
    {
    },
    // get_native_window
    [](void* data) -> EGLNativeWindowType
    {
        return (EGLNativeWindowType)nullptr;
    },
};

}
