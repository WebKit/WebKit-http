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
#include <cassert>
#include <cstdio>
#include <errno.h>
#include <fcntl.h>
#include <memory>
#include <sys/mman.h>

#include <sys/types.h>
#include <gbm.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <array>
#include <vector>

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


ViewBackendWayland::ViewBackendWayland()
    : m_display(WaylandDisplay::singleton())
{
    fprintf(stderr, "ViewBackendWayland::ViewBackendWayland()\n");
    m_surface = wl_compositor_create_surface(WaylandDisplay::singleton().compositor());

    m_xdgSurface = xdg_shell_get_xdg_surface(m_display.xdg(), m_surface);
    xdg_surface_add_listener(m_xdgSurface, &g_xdgSurfaceListener, this);
    xdg_surface_set_title(m_xdgSurface, "WPE");
}

void ViewBackendWayland::commitPrimeFD(int fd, uint32_t handle, uint32_t width, uint32_t height, uint32_t stride, uint32_t)
{
    struct wl_buffer* buffer = nullptr;
    if (fd >= 0) {
        assert(m_bufferMap.find(handle) == m_bufferMap.end());
        buffer = wl_drm_create_prime_buffer(m_display.drm(), fd, width, height, WL_DRM_FORMAT_ARGB8888, 0, stride, 0, 0, 0, 0);
        wl_buffer_add_listener(buffer, &m_bufferListener, new BufferListenerData{ *this, handle });
        m_bufferMap.insert({ handle, buffer });
    } else {
        auto it = m_bufferMap.find(handle);
        assert(it != m_bufferMap.end());
        buffer = it->second;
    }

    if (!buffer) {
        fprintf(stderr, "ViewBackendWayland: failed to create/find a buffer for PRIME handle %u\n", handle);
        return;
    }

    struct wl_callback* callback = wl_surface_frame(m_surface);
    wl_callback_add_listener(callback, &m_callbackListener, this);

    wl_surface_attach(m_surface, buffer, 0, 0);
    wl_surface_damage(m_surface, 0, 0, INT32_MAX, INT32_MAX);
    wl_surface_commit(m_surface);
    wl_display_flush(m_display.display());
}

const struct wl_buffer_listener ViewBackendWayland::m_bufferListener = {
    // release
    [](void* data, struct wl_buffer*)
    {
        auto& listenerData = *static_cast<BufferListenerData*>(data);
        if (listenerData.first.m_client)
            listenerData.first.m_client->releaseBuffer(listenerData.second);
    },
};

const struct wl_callback_listener ViewBackendWayland::m_callbackListener = {
    // frame
    [](void* data, struct wl_callback* callback, uint32_t)
    {
        auto& backend = *static_cast<ViewBackendWayland*>(data);
        if (backend.m_client)
            backend.m_client->frameComplete();

        wl_callback_destroy(callback);
    },
};

} // namespace ViewBackend

} // namespace WPE
