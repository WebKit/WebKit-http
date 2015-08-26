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

#include <WPE/ViewBackend/Wayland/ViewBackendWayland.h>

#include "WaylandDisplay.h"
#include "xdg-shell-client-protocol.h"
#include "wayland-drm-client-protocol.h"
#include <WPE/ViewBackend/ViewBackend.h>
#include <algorithm>
#include <cassert>
#include <cstdio>

namespace WPE {

namespace ViewBackend {

static const struct xdg_surface_listener g_xdgSurfaceListener = {
    // configure
    [](void*, struct xdg_surface* surface, int32_t, int32_t, struct wl_array*, uint32_t serial)
    {
        xdg_surface_ack_configure(surface, serial);
    },
    // delete
    [](void*, struct xdg_surface*) { },
};

const struct wl_buffer_listener g_bufferListener = {
    // release
    [](void* data, struct wl_buffer* buffer)
    {
        auto& bufferData = *static_cast<ViewBackendWayland::BufferListenerData*>(data);
        auto& bufferMap = bufferData.map;

        auto it = std::find_if(bufferMap.begin(), bufferMap.end(),
            [buffer] (const std::pair<uint32_t, struct wl_buffer*>& entry) { return entry.second == buffer; });

        if (it == bufferMap.end())
            return;

        if (bufferData.client)
            bufferData.client->releaseBuffer(it->first);
    },
};

const struct wl_callback_listener g_callbackListener = {
    // frame
    [](void* data, struct wl_callback* callback, uint32_t)
    {
        auto& callbackData = *static_cast<ViewBackendWayland::CallbackListenerData*>(data);
        if (callbackData.client)
            callbackData.client->frameComplete();
        callbackData.frameCallback = nullptr;
        wl_callback_destroy(callback);
    },
};

ViewBackendWayland::ViewBackendWayland()
    : m_display(WaylandDisplay::singleton())
{
    m_surface = wl_compositor_create_surface(m_display.interfaces.compositor);

    m_xdgSurface = xdg_shell_get_xdg_surface(m_display.interfaces.xdg, m_surface);
    xdg_surface_add_listener(m_xdgSurface, &g_xdgSurfaceListener, this);
    xdg_surface_set_title(m_xdgSurface, "WPE");
}

ViewBackendWayland::~ViewBackendWayland()
{
    if (m_callbackData.frameCallback)
        wl_callback_destroy(m_callbackData.frameCallback);
}

void ViewBackendWayland::setClient(Client* client)
{
    m_bufferData.client = client;
    m_callbackData.client = client;
}

void ViewBackendWayland::commitPrimeFD(int fd, uint32_t handle, uint32_t width, uint32_t height, uint32_t stride, uint32_t)
{
    struct wl_buffer* buffer = nullptr;
    auto& bufferMap = m_bufferData.map;
    if (fd >= 0) {
        assert(bufferMap.find(handle) == bufferMap.end());
        buffer = wl_drm_create_prime_buffer(m_display.interfaces.drm, fd, width, height, WL_DRM_FORMAT_ARGB8888, 0, stride, 0, 0, 0, 0);
        wl_buffer_add_listener(buffer, &g_bufferListener, &m_bufferData);
        bufferMap.insert({ handle, buffer });
    } else {
        auto it = bufferMap.find(handle);
        assert(it != bufferMap.end());
        buffer = it->second;
    }

    if (!buffer) {
        fprintf(stderr, "ViewBackendWayland: failed to create/find a buffer for PRIME handle %u\n", handle);
        return;
    }

    m_callbackData.frameCallback = wl_surface_frame(m_surface);
    wl_callback_add_listener(m_callbackData.frameCallback, &g_callbackListener, &m_callbackData);

    wl_surface_attach(m_surface, buffer, 0, 0);
    wl_surface_damage(m_surface, 0, 0, INT32_MAX, INT32_MAX);
    wl_surface_commit(m_surface);
    wl_display_flush(m_display.display());
}

} // namespace ViewBackend

} // namespace WPE
