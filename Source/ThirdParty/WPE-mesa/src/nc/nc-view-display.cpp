/*
 * Copyright (C) 2016 Garmin Ltd.
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

#include "nc-view-display.h"

#include "nc-renderer-host.h"
#include <cstring>
#include <cstdio>
#include <wayland-server.h>

namespace NC {

template<class P, class M>
P* container_of(M* ptr, const M P::*member) {
    return reinterpret_cast<P*>(reinterpret_cast<char*>(ptr) -
            (size_t)&(reinterpret_cast<P*>(0)->*member));
}

template<class C>
static C const* getResource(C const* r)
{
    if (r && r->resource()) {
        return r;
    }
    return nullptr;
}

ViewDisplay::ViewDisplay(Client* client)
{
    setClient(client);
}

void ViewDisplay::initialize(struct wl_display* display)
{
    static const struct wl_surface_interface surfaceInterface[] = {
        // destroy
        [](struct wl_client*, struct wl_resource* resource)
        {
        },
        // attach
        [](struct wl_client*, struct wl_resource* resource, struct wl_resource* buffer_resource, int32_t x,
                int32_t y)
        {
            auto& view = *static_cast<ViewDisplay*>(wl_resource_get_user_data(resource));

            if (view.m_pending.buffer)
                delete view.m_pending.buffer;

            view.m_pending.buffer = new Buffer(buffer_resource, x, y);

            if (view.m_client)
                view.m_client->OnSurfaceAttach(*view.m_surface, getResource(view.m_pending.buffer));
        },
        // damage
        [](struct wl_client*, struct wl_resource* resource, int32_t x, int32_t y, int32_t width, int32_t height)
        {
            auto& view = *static_cast<ViewDisplay*>(wl_resource_get_user_data(resource));

            if (view.m_pending.damage)
                view.m_pending.damage->expand(x, y, width, height);
            else
                view.m_pending.damage = new Damage(x, y, width, height);

            if (view.m_client)
                view.m_client->OnSurfaceDamage(*view.m_surface, view.m_pending.damage);
        },
        // frame
        [](struct wl_client* client, struct wl_resource* resource, uint32_t id)
        {
            auto& view = *static_cast<ViewDisplay*>(wl_resource_get_user_data(resource));

            struct wl_resource* callback_resource = wl_resource_create(client, &wl_callback_interface,
                    1, id);

            if (!callback_resource) {
                wl_resource_post_no_memory(resource);
                return;
            }

            if (view.m_pending.frameCallback)
                delete view.m_pending.frameCallback;

            view.m_pending.frameCallback = new FrameCallback(callback_resource);

            wl_resource_set_implementation(callback_resource, nullptr, &view,
                    [](struct wl_resource* resource)
                    {
                        auto& view = *static_cast<ViewDisplay*>(wl_resource_get_user_data(resource));
                        delete view.m_pending.frameCallback;
                        view.m_pending.frameCallback = nullptr;
                    });

            if (view.m_client)
                view.m_client->OnSurfaceFrame(*view.m_surface, getResource(view.m_pending.frameCallback));
        },
        // set_opaque_region
        [](struct wl_client*, struct wl_resource* resource, struct wl_resource*)
        {
            unsupportedOperation(resource);
        },
        // set_input_region
        [](struct wl_client*, struct wl_resource* resource, struct wl_resource*)
        {
            unsupportedOperation(resource);
        },
        // commit
        [](struct wl_client*, struct wl_resource* resource)
        {
            auto& view = *static_cast<ViewDisplay*>(wl_resource_get_user_data(resource));

            if (view.m_client) {
                view.m_client->OnSurfaceCommit(*view.m_surface, getResource(view.m_pending.buffer),
                        getResource(view.m_pending.frameCallback), view.m_pending.damage);
            }

            view.clearPending();
        },
        // set_buffer_transform
        [](struct wl_client*, struct wl_resource* resource, int32_t)
        {
            unsupportedOperation(resource);
        },
        // set_buffer_scale
        [](struct wl_client*, struct wl_resource* resource, int32_t)
        {
            unsupportedOperation(resource);
        }
    };

    static const struct wl_compositor_interface compositorInterface[] = {
        // create_surface
        [](struct wl_client* client, struct wl_resource* resource, uint32_t id)
        {
            auto& view = *static_cast<ViewDisplay*>(wl_resource_get_user_data(resource));

            if( view.m_surface ) {
                fprintf(stderr, "Surface already created\n");
                return;
            }

            struct wl_resource* surface_resource = wl_resource_create(client, &wl_surface_interface, 1, id);

            if (!surface_resource) {
                wl_client_post_no_memory(client);
                return;
            }

            view.m_surface = new Surface(surface_resource);

            wl_resource_set_implementation(surface_resource, &surfaceInterface, &view,
                    [](struct wl_resource* resource)
                    {
                        auto& view = *static_cast<ViewDisplay*>(wl_resource_get_user_data(resource));
                        view.m_surface = nullptr;
                        view.clearPending();
                    });
        },
        // create_region
        [](struct wl_client*, struct wl_resource* resource, uint32_t)
        {
            unsupportedOperation(resource);
        },
    };

    m_compositor_global = wl_global_create(display, &wl_compositor_interface, 3, this,
            [](struct wl_client* client, void* data, uint32_t version, uint32_t id)
            {
                auto& view = *static_cast<ViewDisplay*>(data);

                view.m_compositor = wl_resource_create(client, &wl_compositor_interface, version, id);

                if (!view.m_compositor) {
                    wl_client_post_no_memory(client);
                    return;
                }

                wl_resource_set_implementation(view.m_compositor, &compositorInterface, &view,
                    nullptr);
            });
}

ViewDisplay::~ViewDisplay()
{
    clearPending();

    if (m_surface)
        delete m_surface;

    if (m_compositor_global)
        wl_global_destroy(m_compositor_global);
}

void ViewDisplay::setClient(ViewDisplay::Client* client)
{
    m_client = client;
}

void ViewDisplay::clearPending()
{
    if (m_pending.buffer) {
        delete m_pending.buffer;
        m_pending.buffer = nullptr;
    }

    if (m_pending.damage) {
        delete m_pending.damage;
        m_pending.damage = nullptr;
    }

    if (m_pending.frameCallback) {
        delete m_pending.frameCallback;
        m_pending.frameCallback = nullptr;
    }
}

void ViewDisplay::unsupportedOperation(struct wl_resource* resource)
{
    fprintf(stderr, "Unsupported Wayland operation\n");
    wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_METHOD, "Unsupported method");
}

ViewDisplay::Resource::Resource(struct wl_resource* resource)
    : m_resource(resource)
{
    memset(&m_listener, 0, sizeof(m_listener));

    m_listener.notify = destroyListener;

    if (resource)
        wl_resource_add_destroy_listener(resource, &m_listener);
}

ViewDisplay::Resource::~Resource()
{
    if (m_resource)
        wl_list_remove(&m_listener.link);
}

void ViewDisplay::Resource::destroyListener(struct wl_listener* listener, void*)
{
    auto& r = *container_of(listener, &ViewDisplay::Resource::m_listener);

    r.m_resource = nullptr;
    wl_list_remove(&r.m_listener.link);
}

void ViewDisplay::Damage::expand(int32_t x, int32_t y, int32_t w, int32_t h)
{
    if (x < m_x) {
        m_width += m_x - x;
        m_x = x;
    }

    if (y < m_y) {
        m_height += m_y - y;
        m_y = y;
    }

    if (w > m_width)
        m_width = w;

    if (h > m_height)
        m_height = h;
}

}; // namespace NC
