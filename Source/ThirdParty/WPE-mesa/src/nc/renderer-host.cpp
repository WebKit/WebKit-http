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

#include <cstdio>
#include <glib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <wayland-client.h>
#include <wayland-server.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace NC {

struct HostSource {
    static GSourceFuncs sourceFuncs;

    GSource source;
    GPollFD pfd;
    struct wl_display* display;
};

GSourceFuncs HostSource::sourceFuncs = {
    // prepare
    [](GSource*, gint* timeout) -> gboolean
    {
        *timeout = -1;
        return FALSE;
    },
    // check
    [](GSource* base) -> gboolean
    {
        auto& source = *reinterpret_cast<HostSource*>(base);
        return !!source.pfd.revents;
    },
    // dispatch
    [](GSource* base, GSourceFunc, gpointer) -> gboolean
    {
        auto& source = *reinterpret_cast<HostSource*>(base);

        if (source.pfd.revents & G_IO_IN) {
            struct wl_event_loop* eventLoop = wl_display_get_event_loop(source.display);
            wl_event_loop_dispatch(eventLoop, -1);
            wl_display_flush_clients(source.display);
        }

        if (source.pfd.revents & (G_IO_ERR | G_IO_HUP))
            return FALSE;

        source.pfd.revents = 0;
        return TRUE;
    },
    nullptr, // finalize
    nullptr, // closure_callback
    nullptr, // closure_marshall
};

class RendererHost {
public:
    RendererHost();
    ~RendererHost();

    int createClient();

private:
    static const struct wl_compositor_interface s_compositorInterface;

    struct wl_display* m_display;
    GSource* m_source;
};

RendererHost::RendererHost()
{
    m_display = wl_display_create();

    m_source = g_source_new(&HostSource::sourceFuncs, sizeof(HostSource));
    auto& source = *reinterpret_cast<HostSource*>(m_source);

    struct wl_event_loop* eventLoop = wl_display_get_event_loop(m_display);
    source.pfd.fd = wl_event_loop_get_fd(eventLoop);
    source.pfd.events = G_IO_IN | G_IO_ERR | G_IO_HUP;
    source.pfd.revents = 0;
    source.display = m_display;

    g_source_add_poll(m_source, &source.pfd);
    g_source_set_name(m_source, "NC::HostSource");
    g_source_set_priority(m_source, G_PRIORITY_DEFAULT);
    g_source_set_can_recurse(m_source, TRUE);
    g_source_attach(m_source, g_main_context_get_thread_default());

    wl_global_create(m_display, &wl_compositor_interface, 1, this,
        [](struct wl_client* client, void* data, uint32_t, uint32_t id)
        {
            struct wl_resource* resource = wl_resource_create(client, &wl_compositor_interface, 1, id);
            wl_resource_set_implementation(resource, &s_compositorInterface, data, nullptr);
        });
}

RendererHost::~RendererHost()
{
    if (m_source)
        g_source_unref(m_source);
}

int RendererHost::createClient()
{
    int pair[2];
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, pair) < 0)
        return -1;

    int clientFd = dup(pair[1]);
    close(pair[1]);

    wl_client_create(m_display, pair[0]);

    return clientFd;
}

const struct wl_compositor_interface RendererHost::s_compositorInterface = {
    // create_surface
    [](struct wl_client*, struct wl_resource*, uint32_t)
    {
    },
    // create_region
    [](struct wl_client*, struct wl_resource*, uint32_t) { },
};

} // namespace NC

extern "C" {

struct wpe_renderer_host_interface nc_renderer_host_interface = {
    // create
    []() -> void*
    {
        return new NC::RendererHost;
    },
    // destroy
    [](void* data)
    {
        auto* rendererHost = static_cast<NC::RendererHost*>(data);
        delete rendererHost;
    },
    // create_client
    [](void* data) -> int
    {
        auto& rendererHost = *static_cast<NC::RendererHost*>(data);
        return rendererHost.createClient();
    },
};

}
