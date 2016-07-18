/*
 * Copyright (C) 2016 Garmin Ltd.
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

#include <wpe-mesa/view-backend-exportable.h>

#include "nested-compositor.h"

#include "nc-renderer-host.h"
#include <assert.h>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>
#include <wayland-client.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <xf86drm.h>

#include "wayland-drm-server-protocol.h"

namespace NC {
namespace Exportable {

template<class P, class M>
P* container_of(M* ptr, const M P::*member) {
    return reinterpret_cast<P*>(reinterpret_cast<char*>(ptr) -
            (size_t)&(reinterpret_cast<P*>(0)->*member));
}

class ViewBackend;

class ClientBundle {
public:
    struct wpe_mesa_view_backend_exportable_client* client;
    void* data;
    ViewBackend* viewBackend;
};

class Buffer {
public:
    Buffer(struct wl_resource*,
            struct wpe_mesa_view_backend_exportable_dma_buf_egl_image_data&);

    ~Buffer();

    struct wpe_mesa_view_backend_exportable_dma_buf_egl_image_data&
    getImageData() { return m_imageData; }

    void sendRelease() const;

private:
    struct wl_resource* m_resource;
    struct wpe_mesa_view_backend_exportable_dma_buf_egl_image_data m_imageData;
    struct wl_listener m_buffer_destroy_listener;

    static void onBufferDestroyed(struct wl_listener*, void*);
};

class ViewBackend {
public:
    ViewBackend(ClientBundle*, struct wpe_view_backend* backend);
    virtual ~ViewBackend();

    void initialize();

    void sendFrameCompleteCallback();

    Buffer* getBuffer(uint32_t);
private:
    struct {
        std::string dev_path;
        int fd {-1};
    } m_drm;

    struct {
        struct {
            Buffer* buffer {nullptr};
            struct wl_resource* callback {nullptr};
        } pending;

        struct {
            struct wl_resource* callback {nullptr};
        } current;

        struct wl_global* compositor;
        struct wl_global* drm;
    } m_server;

    ClientBundle* m_clientBundle;
    struct wpe_view_backend* m_backend;

    std::unordered_map<uint32_t, Buffer*> m_bufferMap;
    uint32_t m_handle {1};

    static const struct wl_drm_interface g_drmImplementation;
    static const struct wl_surface_interface g_surfaceImplementation;
    static const struct wl_compositor_interface g_compositorImplementation;
    static const struct wl_buffer_interface g_drmBufferImplementation;

    static void bindDrm(struct wl_client*, void*, uint32_t, uint32_t);
    static void bindCompositor(struct wl_client*, void*, uint32_t, uint32_t);

    static void destroyBuffer(struct wl_resource*);
    static void destroyDrm(struct wl_resource*);
    static void destroyCallback(struct wl_resource*);

};

Buffer::Buffer(struct wl_resource* resource,
        struct wpe_mesa_view_backend_exportable_dma_buf_egl_image_data& imageData)
    : m_resource(resource)
    , m_imageData(imageData)
{
    memset(&m_buffer_destroy_listener, 0, sizeof(m_buffer_destroy_listener));
    m_buffer_destroy_listener.notify = onBufferDestroyed;

    wl_resource_add_destroy_listener(resource, &m_buffer_destroy_listener);
}

Buffer::~Buffer()
{
    if (m_imageData.fd >= 0)
        close(m_imageData.fd);

    wl_list_remove(&m_buffer_destroy_listener.link);
}

void Buffer::sendRelease() const
{
    if (m_resource)
        wl_buffer_send_release(m_resource);
}

void Buffer::onBufferDestroyed(struct wl_listener* listener, void* data)
{
    auto* buffer = container_of(listener, &Buffer::m_buffer_destroy_listener);
    buffer->m_resource = nullptr;
    wl_list_remove(&buffer->m_buffer_destroy_listener.link);
}

ViewBackend::ViewBackend(ClientBundle* clientBundle, struct wpe_view_backend* backend)
    : m_clientBundle(clientBundle)
    , m_backend(backend)
{
    m_clientBundle->viewBackend = this;

    // TODO: We need to get this via some other method
    m_drm.dev_path = "/dev/dri/renderD128";
    m_drm.fd = open(m_drm.dev_path.c_str(), O_CLOEXEC | O_RDWR);

    if (m_drm.fd < 0 ) {
        fprintf(stderr, "Cannot open %s: %s\n", m_drm.dev_path.c_str(), strerror(errno));
        return;
    }

    auto& server = NC::RendererHost::singleton();
    server.initialize();

    m_server.compositor = wl_global_create(server.display(), &wl_compositor_interface, 3, this, bindCompositor);
    m_server.drm = wl_global_create(server.display(), &wl_drm_interface, 2, this, bindDrm);
}

ViewBackend::~ViewBackend()
{
    if (m_drm.fd >= 0 )
        close(m_drm.fd);

    m_backend = nullptr;
}

void ViewBackend::initialize()
{
    wpe_view_backend_dispatch_set_size(m_backend, 800, 600);
}

void ViewBackend::destroyCallback(struct wl_resource* resource)
{
    auto& backend = *static_cast<ViewBackend*>(wl_resource_get_user_data(resource));
    if (backend.m_server.current.callback == resource)
        backend.m_server.current.callback = nullptr;

    if (backend.m_server.pending.callback == resource)
        backend.m_server.pending.callback = nullptr;
}

const struct wl_surface_interface ViewBackend::g_surfaceImplementation = {
    // destroy
    [](struct wl_client*, struct wl_resource* resource)
    {
    },
    // attach
    [](struct wl_client*, struct wl_resource* resource, struct wl_resource* buffer_resource, int32_t x,
            int32_t y)
    {
        auto& backend = *static_cast<ViewBackend*>(wl_resource_get_user_data(resource));

        backend.m_server.pending.buffer = nullptr;
        if (buffer_resource)
            backend.m_server.pending.buffer = static_cast<Buffer*>(wl_resource_get_user_data(buffer_resource));
    },
    // damage
    [](struct wl_client*, struct wl_resource* resource, int32_t x, int32_t y, int32_t width, int32_t height)
    {
    },
    // frame
    [](struct wl_client* client, struct wl_resource* resource, uint32_t id)
    {
        auto& backend = *static_cast<ViewBackend*>(wl_resource_get_user_data(resource));

        struct wl_resource* callback_resource = wl_resource_create(client, &wl_callback_interface,
                1, id);

        if (!callback_resource) {
            wl_resource_post_no_memory(resource);
            return;
        }

        wl_resource_set_implementation(callback_resource, nullptr, &backend, destroyCallback);

        backend.m_server.pending.callback = callback_resource;
    },
    // set_opaque_region
    [](struct wl_client*, struct wl_resource*, struct wl_resource*)
    {
        assert(0);
    },
    // set_input_region
    [](struct wl_client*, struct wl_resource*, struct wl_resource*)
    {
        assert(0);
    },
    // commit
    [](struct wl_client*, struct wl_resource* resource)
    {
        auto& backend = *static_cast<ViewBackend*>(wl_resource_get_user_data(resource));

        auto* buffer = backend.m_server.pending.buffer;
        auto* callback = backend.m_server.pending.callback;

        backend.m_server.pending.buffer = nullptr;
        backend.m_server.pending.callback = nullptr;

        if (buffer) {
            backend.m_server.current.callback = callback;

            backend.m_clientBundle->client->export_dma_buf_egl_image(backend.m_clientBundle->data,
                    &buffer->getImageData());
        }
    },
    // set_buffer_transform
    [](struct wl_client*, struct wl_resource*, int32_t)
    {
        assert(0);
    },
    // set_buffer_scale
    [](struct wl_client*, struct wl_resource*, int32_t)
    {
        assert(0);
    }
};

const struct wl_compositor_interface ViewBackend::g_compositorImplementation = {
    // create_surface
    [](struct wl_client* client, struct wl_resource* resource, uint32_t id)
    {
        auto& backend = *static_cast<ViewBackend*>(wl_resource_get_user_data(resource));

        struct wl_resource* surface_resource = wl_resource_create(client, &wl_surface_interface,
                3, id);

        if (!surface_resource) {
            wl_resource_post_no_memory(resource);
            return;
        }

        wl_resource_set_implementation(surface_resource, &ViewBackend::g_surfaceImplementation,
                &backend, nullptr);
    },
    // create_region
    [](struct wl_client* client, struct wl_resource* resource, uint32_t id)
    {
        assert(0);
    },
};

void ViewBackend::bindCompositor(struct wl_client* client, void* data, uint32_t version, uint32_t id)
{
    auto& backend = *static_cast<ViewBackend*>(data);
    struct wl_resource* resource = wl_resource_create(client, &wl_compositor_interface,
        version, id);

    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }

    wl_resource_set_implementation(resource, &g_compositorImplementation, &backend, nullptr);
}

const struct wl_buffer_interface ViewBackend::g_drmBufferImplementation = {
    // destroy
    [] (struct wl_client*, struct wl_resource* resource)
    {
    },
};

const struct wl_drm_interface ViewBackend::g_drmImplementation = {
    // authenticate
    [](struct wl_client* client, struct wl_resource* resource, uint32_t id)
    {
        // Since we are using a render node, there isn't really any authentication,
        // just report success
        wl_drm_send_authenticated(resource);
    },
    // create_buffer
	[](struct wl_client *client, struct wl_resource *resource, uint32_t id, uint32_t name,
            int32_t width, int32_t height, uint32_t stride, uint32_t format)
    {
        wl_resource_post_error(resource, WL_DRM_ERROR_INVALID_NAME,
                "non-Prime buffers are not supported on render nodes");
    },
    // create_planar_buffer
    [](struct wl_client *client, struct wl_resource *resource, uint32_t id, uint32_t name,
            int32_t width, int32_t height, uint32_t format, int32_t offset0, int32_t stride0,
            int32_t offset1, int32_t stride1, int32_t offset2, int32_t stride2)
    {
        wl_resource_post_error(resource, WL_DRM_ERROR_INVALID_NAME,
                "non-Prime buffers are not supported on render nodes");
    },
	// create_prime_buffer
    [](struct wl_client *client, struct wl_resource *resource, uint32_t id, int32_t fd,
            int32_t width, int32_t height, uint32_t format, int32_t offset0, int32_t stride0,
            int32_t offset1, int32_t stride1, int32_t offset2, int32_t stride2)
    {
        auto& backend = *static_cast<ViewBackend*>(wl_resource_get_user_data(resource));

        struct wl_resource* buffer_resource = wl_resource_create(client, &wl_buffer_interface,
                1, id);

        if (!buffer_resource) {
            wl_client_post_no_memory(client);
            return;
        }

        struct wpe_mesa_view_backend_exportable_dma_buf_egl_image_data imageData;
        imageData.fd = fd;
        imageData.width = width;
        imageData.height = height;
        imageData.format = format;
        imageData.stride = stride0;
        imageData.handle = backend.m_handle;

        auto* buffer = new Buffer(buffer_resource, imageData);

        backend.m_bufferMap.insert({imageData.handle, buffer});
        backend.m_handle++;

        wl_resource_set_implementation(buffer_resource, &g_drmBufferImplementation, buffer, nullptr);
    },
};

void ViewBackend::bindDrm(struct wl_client* client, void* data, uint32_t version, uint32_t id)
{
    auto& backend = *static_cast<ViewBackend*>(data);
    struct wl_resource* resource = wl_resource_create(client, &wl_drm_interface,
        version, id);

    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }

    wl_resource_set_implementation(resource, &g_drmImplementation, &backend, nullptr);
    wl_drm_send_device(resource, backend.m_drm.dev_path.c_str());
    wl_drm_send_format(resource, WL_DRM_FORMAT_XRGB8888);
    wl_drm_send_format(resource, WL_DRM_FORMAT_ARGB8888);
    wl_drm_send_capabilities(resource, WL_DRM_CAPABILITY_PRIME);
}

void ViewBackend::sendFrameCompleteCallback()
{
    if (m_server.current.callback) {
        wl_callback_send_done(m_server.current.callback, 0);
        m_server.current.callback = nullptr;
    }
}

Buffer* ViewBackend::getBuffer(uint32_t handle)
{
    auto i = m_bufferMap.find(handle);
    if (i == m_bufferMap.end())
        return nullptr;
    return i->second;
}
} // namespace Exportable
} // namespace NC

extern "C" {

struct wpe_mesa_view_backend_exportable {
    NC::Exportable::ClientBundle* clientBundle;
    struct wpe_view_backend* backend;
};

static struct wpe_view_backend_interface exportable_view_backend_interface = {
    // create
    [](void* data, struct wpe_view_backend* backend) -> void*
    {
        auto* clientBundle = static_cast<NC::Exportable::ClientBundle*>(data);
        return new NC::Exportable::ViewBackend(clientBundle, backend);
    },
    // destroy
    [](void* data)
    {
        auto* backend = static_cast<NC::Exportable::ViewBackend*>(data);
        delete backend;
    },
    // initialize
    [](void* data)
    {
        auto& backend = *static_cast<NC::Exportable::ViewBackend*>(data);
        backend.initialize();
    },
    // get_renderer_host_fd
    [](void* data) -> int
    {
        return -1;
    },
};

__attribute__((visibility("default")))
struct wpe_mesa_view_backend_exportable*
wpe_mesa_view_backend_exportable_create(struct wpe_mesa_view_backend_exportable_client* client, void* client_data)
{
    auto* clientBundle = new NC::Exportable::ClientBundle{ client, client_data, nullptr };
    struct wpe_view_backend* backend = wpe_view_backend_create_with_backend_interface(&exportable_view_backend_interface, clientBundle);

    auto* exportable = new struct wpe_mesa_view_backend_exportable;
    exportable->clientBundle = clientBundle;
    exportable->backend = backend;

    return exportable;
}

__attribute__((visibility("default")))
void
wpe_mesa_view_backend_exportable_destroy(struct wpe_mesa_view_backend_exportable* exportable)
{
    wpe_view_backend_destroy(exportable->backend);
    delete exportable->clientBundle;
    delete exportable;
}

__attribute__((visibility("default")))
struct wpe_view_backend*
wpe_mesa_view_backend_exportable_get_view_backend(struct wpe_mesa_view_backend_exportable* exportable)
{
    return exportable->backend;
}

__attribute__((visibility("default")))
void
wpe_mesa_view_backend_exportable_dispatch_frame_complete(struct wpe_mesa_view_backend_exportable* exportable)
{
    auto* backend = exportable->clientBundle->viewBackend;
    backend->sendFrameCompleteCallback();
}

__attribute__((visibility("default")))
void
wpe_mesa_view_backend_exportable_dispatch_release_buffer(struct wpe_mesa_view_backend_exportable* exportable, uint32_t handle)
{
    auto* backend = exportable->clientBundle->viewBackend;
    auto* buffer = backend->getBuffer(handle);

    if (buffer)
        buffer->sendRelease();
}

}

