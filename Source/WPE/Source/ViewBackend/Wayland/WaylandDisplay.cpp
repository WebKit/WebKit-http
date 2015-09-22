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

#include "WaylandDisplay.h"

#include "xdg-shell-client-protocol.h"
#include "wayland-drm-client-protocol.h"
#include <cstring>
#include <glib.h>
#include <wayland-client.h>

namespace WPE {

namespace ViewBackend {

class EventSource {
public:
    static GSourceFuncs sourceFuncs;

    GSource source;
    GPollFD pfd;
    struct wl_display* display;
};

GSourceFuncs EventSource::sourceFuncs = {
    // prepare
    [](GSource* base, gint* timeout) -> gboolean
    {
        auto* source = reinterpret_cast<EventSource*>(base);
        struct wl_display* display = source->display;

        *timeout = -1;

        wl_display_flush(display);
        wl_display_dispatch_pending(display);

        return FALSE;
    },
    // check
    [](GSource* base) -> gboolean
    {
        auto* source = reinterpret_cast<EventSource*>(base);
        return !!source->pfd.revents;
    },
    // dispatch
    [](GSource* base, GSourceFunc, gpointer) -> gboolean
    {
        auto* source = reinterpret_cast<EventSource*>(base);
        struct wl_display* display = source->display;

        if (source->pfd.revents & G_IO_IN)
            wl_display_dispatch(display);

        if (source->pfd.revents & (G_IO_ERR | G_IO_HUP))
            return FALSE;

        source->pfd.revents = 0;
        return TRUE;
    },
    nullptr, // finalize
    nullptr, // closure_callback
    nullptr, // closure_marshall
};

static const struct xdg_shell_listener g_xdgShellListener = {
    // ping
    [](void*, struct xdg_shell* shell, uint32_t serial)
    {
        xdg_shell_pong(shell, serial);
    },
};

const struct wl_registry_listener g_registryListener = {
    // global
    [](void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t)
    {
        auto& interfaces = *static_cast<WaylandDisplay::Interfaces*>(data);

        if (!std::strcmp(interface, "wl_compositor"))
            interfaces.compositor = static_cast<struct wl_compositor*>(wl_registry_bind(registry, name, &wl_compositor_interface, 1));

        if (!std::strcmp(interface, "wl_drm"))
            interfaces.drm = static_cast<struct wl_drm*>(wl_registry_bind(registry, name, &wl_drm_interface, 2));

        if (!std::strcmp(interface, "wl_seat"))
            interfaces.seat = static_cast<struct wl_seat*>(wl_registry_bind(registry, name, &wl_seat_interface, 1));

        if (!std::strcmp(interface, "xdg_shell")) {
            interfaces.xdg = static_cast<struct xdg_shell*>(wl_registry_bind(registry, name, &xdg_shell_interface, 1)); 
            xdg_shell_add_listener(interfaces.xdg, &g_xdgShellListener, nullptr);
            xdg_shell_use_unstable_version(interfaces.xdg, 5);
        }
    },
    // global_remove
    [](void*, struct wl_registry*, uint32_t) { },
};

const WaylandDisplay& WaylandDisplay::singleton()
{
    static WaylandDisplay display;
    return display;
}

WaylandDisplay::WaylandDisplay()
{
    m_display = wl_display_connect(nullptr);
    m_registry = wl_display_get_registry(m_display);

    wl_registry_add_listener(m_registry, &g_registryListener, &interfaces);
    wl_display_roundtrip(m_display);

    GSource* baseSource = g_source_new(&EventSource::sourceFuncs, sizeof(EventSource));
    auto* source = reinterpret_cast<EventSource*>(baseSource);
    source->display = m_display;

    source->pfd.fd = wl_display_get_fd(m_display);
    source->pfd.events = G_IO_IN | G_IO_ERR | G_IO_HUP;
    source->pfd.revents = 0;
    g_source_add_poll(baseSource, &source->pfd);

    g_source_set_name(baseSource, "[WebKit] WaylandDisplay");
    g_source_set_priority(baseSource, G_PRIORITY_HIGH + 30);
    g_source_set_can_recurse(baseSource, TRUE);
    g_source_attach(baseSource, g_main_context_get_thread_default());
}

WaylandDisplay::~WaylandDisplay()
{
    if (interfaces.compositor)
        wl_compositor_destroy(interfaces.compositor);
    if (interfaces.drm)
        wl_drm_destroy(interfaces.drm);
    if (interfaces.seat)
        wl_seat_destroy(interfaces.seat);
    if (interfaces.xdg)
        xdg_shell_destroy(interfaces.xdg);

    if (m_registry)
        wl_registry_destroy(m_registry);
    if (m_display)
        wl_display_disconnect(m_display);
}

} // namespace ViewBackend

} // namespace WPE
