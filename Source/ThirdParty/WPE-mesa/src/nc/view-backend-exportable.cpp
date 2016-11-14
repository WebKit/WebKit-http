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

#error This file shouldn't be built for now -- do we need it?

#include <wpe-mesa/view-backend-exportable.h>

#include "nested-compositor.h"

#include "nc-renderer-host.h"
#include "nc-view-display.h"
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
#include <EGL/egl.h>
#include <EGL/eglext.h>

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
    EGLDisplay display;
};

class ViewBackend: public NC::ViewDisplay::Client {
public:
    class Buffer {
    public:
        Buffer(struct wl_resource*, ViewBackend*);
        ~Buffer();

        void sendRelease() const;
        struct wl_resource* getResource() const { return m_resource; }

    private:
        struct wl_resource* m_resource;
        ViewBackend* m_backend;
        struct wl_listener m_buffer_destroy_listener;

        static void onBufferDestroyed(struct wl_listener*, void*);
    };

    ViewBackend(ClientBundle*, struct wpe_view_backend* backend);
    virtual ~ViewBackend();

    virtual void OnSurfaceCommit(NC::ViewDisplay::Surface const&, NC::ViewDisplay::Buffer const*,
            NC::ViewDisplay::FrameCallback const*, NC::ViewDisplay::Damage const*) override;

    void initialize();

    void sendFrameCompleteCallback();

    Buffer* getBuffer(uint32_t);
    Buffer* getBuffer(struct wl_resource*);
    Buffer* addBuffer(struct wl_resource*);

private:
    struct {
        EGLDisplay display;
        bool waylandBound {false};

        PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR;
        PFNEGLBINDWAYLANDDISPLAYWL eglBindWaylandDisplayWL;
        PFNEGLUNBINDWAYLANDDISPLAYWL eglUnbindWaylandDisplayWL;
        PFNEGLQUERYWAYLANDBUFFERWL eglQueryWaylandBufferWL;
    } m_egl;

    struct wl_resource *m_callback {nullptr};

    NC::ViewDisplay m_viewDisplay;

    ClientBundle* m_clientBundle;
    struct wpe_view_backend* m_backend;

    std::unordered_map<uint32_t, Buffer*> m_bufferMap;

    static void destroyBuffer(struct wl_resource*);

};

ViewBackend::Buffer::Buffer(struct wl_resource* resource, ViewBackend* backend)
    : m_resource(resource)
    , m_backend(backend)
{
    memset(&m_buffer_destroy_listener, 0, sizeof(m_buffer_destroy_listener));
    m_buffer_destroy_listener.notify = onBufferDestroyed;

    wl_resource_add_destroy_listener(resource, &m_buffer_destroy_listener);

    backend->m_bufferMap.insert({wl_resource_get_id(resource), this});
}

ViewBackend::Buffer::~Buffer()
{
    wl_list_remove(&m_buffer_destroy_listener.link);
    m_backend->m_bufferMap.erase(wl_resource_get_id(m_resource));
}

void ViewBackend::Buffer::sendRelease() const
{
    wl_buffer_send_release(m_resource);
}

void ViewBackend::Buffer::onBufferDestroyed(struct wl_listener* listener, void* data)
{
    auto* buffer = container_of(listener, &ViewBackend::Buffer::m_buffer_destroy_listener);
    delete buffer;
}

template<class T>
static void bindEGLproc(T& p, char const* name)
{
    p = reinterpret_cast<T>(eglGetProcAddress(name));
    if (!p) {
        fprintf(stderr, "Cannot bind %s\n", name);
        abort();
    }
}

ViewBackend::ViewBackend(ClientBundle* clientBundle, struct wpe_view_backend* backend)
    : m_clientBundle(clientBundle)
    , m_backend(backend)
    , m_viewDisplay(this)
{
    m_clientBundle->viewBackend = this;
    m_egl.display = m_clientBundle->display;

    auto& server = NC::RendererHost::singleton();
    server.initialize();

    m_viewDisplay.initialize(server.display());

    bindEGLproc(m_egl.eglCreateImageKHR, "eglCreateImageKHR");
    bindEGLproc(m_egl.eglBindWaylandDisplayWL, "eglBindWaylandDisplayWL");
    bindEGLproc(m_egl.eglUnbindWaylandDisplayWL, "eglUnbindWaylandDisplayWL");
    bindEGLproc(m_egl.eglQueryWaylandBufferWL, "eglQueryWaylandBufferWL");

    m_egl.waylandBound = m_egl.eglBindWaylandDisplayWL(m_egl.display, server.display());
    if (!m_egl.waylandBound) {
        fprintf(stderr, "Cannot bind Wayland Display to EGL Display: %d\n", eglGetError());
        return;
    }
}

ViewBackend::~ViewBackend()
{
    m_backend = nullptr;

    if (m_egl.waylandBound)
        m_egl.eglUnbindWaylandDisplayWL(m_egl.display, NC::RendererHost::singleton().display());
}

void ViewBackend::OnSurfaceCommit(NC::ViewDisplay::Surface const&, NC::ViewDisplay::Buffer const* buffer,
        NC::ViewDisplay::FrameCallback const* frameCallback, NC::ViewDisplay::Damage const*)
{
    if (buffer) {
        if (frameCallback) {
            m_callback = frameCallback->resource();

            wl_resource_set_implementation(frameCallback->resource(), nullptr, this,
                    [](struct wl_resource* resource)
                    {
                        auto& backend = *static_cast<ViewBackend*>(wl_resource_get_user_data(resource));
                        backend.m_callback = nullptr;
                    });
        }

        auto* resource = buffer->resource();

        addBuffer(resource);

        struct wpe_mesa_view_backend_exportable_egl_image_data data;

        data.handle = wl_resource_get_id(resource);
        data.image = m_egl.eglCreateImageKHR(m_egl.display, EGL_NO_CONTEXT,
                EGL_WAYLAND_BUFFER_WL, (EGLClientBuffer)resource, nullptr);

        m_egl.eglQueryWaylandBufferWL(m_egl.display, resource, EGL_WIDTH, &data.width);
        m_egl.eglQueryWaylandBufferWL(m_egl.display, resource, EGL_HEIGHT, &data.height);

        m_clientBundle->client->export_egl_image(m_clientBundle->data, &data);
    }
}

void ViewBackend::initialize()
{
    wpe_view_backend_dispatch_set_size(m_backend, 800, 600);
}

void ViewBackend::sendFrameCompleteCallback()
{
    if (m_callback) {
        wl_callback_send_done(m_callback, 0);
        m_callback = nullptr;
    }
}

ViewBackend::Buffer* ViewBackend::getBuffer(uint32_t handle)
{
    auto i = m_bufferMap.find(handle);
    if (i == m_bufferMap.end())
        return nullptr;
    return i->second;
}

ViewBackend::Buffer* ViewBackend::getBuffer(struct wl_resource* resource)
{
    return getBuffer(wl_resource_get_id(resource));
}

ViewBackend::Buffer* ViewBackend::addBuffer(struct wl_resource* resource)
{
    auto* buffer = getBuffer(resource);
    if (buffer)
        return buffer;

    buffer = new Buffer(resource, this);

    return buffer;
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
wpe_mesa_view_backend_exportable_create(EGLDisplay display, struct wpe_mesa_view_backend_exportable_client* client,
        void* client_data)
{
    auto* clientBundle = new NC::Exportable::ClientBundle{ client, client_data, nullptr };
    clientBundle->display = display;

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

