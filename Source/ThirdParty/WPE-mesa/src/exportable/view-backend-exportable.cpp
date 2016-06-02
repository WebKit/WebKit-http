#include <wpe-mesa/view-backend-exportable.h>

#include "gbm-connection.h"
#include <cstdio>

namespace Exportable {

class ViewBackend;

class ClientBundle {
public:
    struct wpe_mesa_view_backend_exportable_client* client;
    void* data;
    ViewBackend* viewBackend;
};

class ViewBackend : public GBM::Host::Client {
public:
    ViewBackend(ClientBundle*, struct wpe_view_backend* backend);
    virtual ~ViewBackend();

    void initialize();

    GBM::Host& gbmHost() { return m_renderer.gbmHost; }

private:
    // GBM::Host::Client
    void importBufferFd(int) override;
    void commitBuffer(struct buffer_commit*) override;

    ClientBundle* m_clientBundle;
    struct wpe_view_backend* m_backend;

    struct {
        GBM::Host gbmHost;
        int pendingBufferFd { -1 };
    } m_renderer;
};

ViewBackend::ViewBackend(ClientBundle* clientBundle, struct wpe_view_backend* backend)
    : m_clientBundle(clientBundle)
    , m_backend(backend)
{
    m_clientBundle->viewBackend = this;
    m_renderer.gbmHost.initialize(*this);
}

ViewBackend::~ViewBackend()
{
    m_backend = nullptr;

    m_renderer.gbmHost.deinitialize();

    if (m_renderer.pendingBufferFd != -1)
        close(m_renderer.pendingBufferFd);
    m_renderer.pendingBufferFd = -1;
}

void ViewBackend::initialize()
{
    wpe_view_backend_dispatch_set_size(m_backend, 800, 600);
}

void ViewBackend::importBufferFd(int fd)
{
    if (m_renderer.pendingBufferFd != -1)
        close(m_renderer.pendingBufferFd);
    m_renderer.pendingBufferFd = fd;
}

void ViewBackend::commitBuffer(struct buffer_commit* commit)
{
    m_clientBundle->client->export_dma_buf(m_clientBundle->data, m_renderer.pendingBufferFd,
        commit->handle, commit->width, commit->height, commit->stride, commit->format);

    m_renderer.pendingBufferFd = -1;
}

} // namespace Exportable

extern "C" {

struct wpe_mesa_view_backend_exportable {
    Exportable::ClientBundle* clientBundle;
    struct wpe_view_backend* backend;
};

struct wpe_view_backend_interface exportable_view_backend_interface = {
    // create
    [](void* data, struct wpe_view_backend* backend) -> void*
    {
        auto* clientBundle = static_cast<Exportable::ClientBundle*>(data);
        return new Exportable::ViewBackend(clientBundle, backend);
    },
    // destroy
    [](void* data)
    {
        auto* backend = static_cast<Exportable::ViewBackend*>(data);
        delete backend;
    },
    // initialize
    [](void* data)
    {
        auto& backend = *static_cast<Exportable::ViewBackend*>(data);
        backend.initialize();
    },
    // get_renderer_host_fd
    [](void* data) -> int
    {
        auto& backend = *static_cast<Exportable::ViewBackend*>(data);
        return backend.gbmHost().releaseClientFD();
    },
};

__attribute__((visibility("default")))
struct wpe_mesa_view_backend_exportable*
wpe_mesa_view_backend_exportable_create(struct wpe_mesa_view_backend_exportable_client* client, void* client_data)
{
    auto* clientBundle = new Exportable::ClientBundle{ client, client_data, nullptr };
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
    exportable->clientBundle->viewBackend->gbmHost().frameComplete();
}

__attribute__((visibility("default")))
void
wpe_mesa_view_backend_exportable_dispatch_release_buffer(struct wpe_mesa_view_backend_exportable* exportable, uint32_t handle)
{
    exportable->clientBundle->viewBackend->gbmHost().releaseBuffer(handle);
}


}
