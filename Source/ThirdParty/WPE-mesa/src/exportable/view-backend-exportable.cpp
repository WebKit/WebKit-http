#include <wpe-mesa/view-backend-exportable.h>

#include "ipc.h"
#include "ipc-gbm.h"
#include <cstdio>
#include <memory>

namespace Exportable {

class ViewBackend;

class ClientBundle {
public:
    struct wpe_mesa_view_backend_exportable_client* client;
    void* data;
    ViewBackend* viewBackend;
};

class ViewBackend : public IPC::Host::Handler {
public:
    ViewBackend(ClientBundle*, struct wpe_view_backend* backend);
    virtual ~ViewBackend();

    void initialize();

    IPC::Host& ipcHost() { return m_renderer.ipcHost; }

private:
    // IPC::Host::Handler
    void handleFd(int) override;
    void handleMessage(char*, size_t) override;

    ClientBundle* m_clientBundle;
    struct wpe_view_backend* m_backend;

    struct {
        IPC::Host ipcHost;
        int pendingBufferFd { -1 };
    } m_renderer;
};

ViewBackend::ViewBackend(ClientBundle* clientBundle, struct wpe_view_backend* backend)
    : m_clientBundle(clientBundle)
    , m_backend(backend)
{
    m_clientBundle->viewBackend = this;
    m_renderer.ipcHost.initialize(*this);
}

ViewBackend::~ViewBackend()
{
    m_backend = nullptr;

    m_renderer.ipcHost.deinitialize();

    if (m_renderer.pendingBufferFd != -1)
        close(m_renderer.pendingBufferFd);
    m_renderer.pendingBufferFd = -1;
}

void ViewBackend::initialize()
{
    wpe_view_backend_dispatch_set_size(m_backend, 800, 600);
}

void ViewBackend::handleFd(int fd)
{
    if (m_renderer.pendingBufferFd != -1)
        close(m_renderer.pendingBufferFd);
    m_renderer.pendingBufferFd = fd;
}

void ViewBackend::handleMessage(char* data, size_t size)
{
    if (size != MESSAGE_SIZE)
        return;

    auto* message = reinterpret_cast<struct ipc_gbm_message*>(data);
    uint64_t messageCode = message->message_code;
    if (messageCode != 42)
        return;

    auto* commit = reinterpret_cast<struct buffer_commit*>(std::addressof(message->data));

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
        return backend.ipcHost().releaseClientFD();
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
    struct ipc_gbm_message message = { 0, };
    message.message_code = 23;
    exportable->clientBundle->viewBackend->ipcHost().send(reinterpret_cast<char*>(&message), sizeof(message));
}

__attribute__((visibility("default")))
void
wpe_mesa_view_backend_exportable_dispatch_release_buffer(struct wpe_mesa_view_backend_exportable* exportable, uint32_t handle)
{
    struct ipc_gbm_message message = { 0, };
    message.message_code = 16;

    auto* releaseBuffer = reinterpret_cast<struct release_buffer*>(std::addressof(message.data));
    releaseBuffer->handle = handle;

    exportable->clientBundle->viewBackend->ipcHost().send(reinterpret_cast<char*>(&message), sizeof(message));
}

}
