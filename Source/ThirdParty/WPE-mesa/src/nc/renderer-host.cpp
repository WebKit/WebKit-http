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
#include "nc-renderer-host.h"

#include <cstdio>
#include <glib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <wayland-server-core.h>

namespace NC {

GSourceFuncs RendererHost::HostSource::sourceFuncs = {
    // prepare
    [](GSource* base, gint* timeout) -> gboolean
    {
        auto& source = *reinterpret_cast<HostSource*>(base);

        *timeout = -1;

        wl_display_flush_clients(source.display);
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


RendererHost::RendererHost()
{
    m_display = wl_display_create();
}

RendererHost::~RendererHost()
{
    if (m_source)
        g_source_unref(m_source);

    if (m_display)
        wl_display_destroy(m_display);
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

void RendererHost::initialize()
{
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
}


RendererHost& RendererHost::singleton()
{
    static RendererHost* host = nullptr;

    if (host)
        return *host;

    host = new RendererHost();
    return *host;
}

} // namespace NC

extern "C" {

struct wpe_renderer_host_interface nc_renderer_host_interface = {
    // create
    []() -> void*
    {
        return nullptr;
    },
    // destroy
    [](void* data)
    {
    },
    // create_client
    [](void* data) -> int
    {
        return NC::RendererHost::singleton().createClient();
    },
};

}
