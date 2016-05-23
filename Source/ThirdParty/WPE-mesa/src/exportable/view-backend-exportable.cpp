#include <wpe-mesa/view-backend-exportable.h>

#include "gbm-connection.h"
#include <cstdio>

namespace Exportable {

class ViewBackend : public GBM::Host::Client {
public:
    ViewBackend(struct wpe_view_backend* backend);
    virtual ~ViewBackend();

    void initialize();

    GBM::Host& gbmHost() { return m_renderer.gbmHost; }

private:
    // GBM::Host::Client
    void importBufferFd(int) override;
    void commitBuffer(struct buffer_commit*) override;

    struct wpe_view_backend* m_backend;

    struct {
        GBM::Host gbmHost;
        int pendingBufferFd { -1 };
    } m_renderer;
};

ViewBackend::ViewBackend(struct wpe_view_backend* backend)
    : m_backend(backend)
{
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
}

void ViewBackend::importBufferFd(int fd)
{
    if (m_renderer.pendingBufferFd != -1)
        close(m_renderer.pendingBufferFd);
    m_renderer.pendingBufferFd = fd;
}

void ViewBackend::commitBuffer(struct buffer_commit*)
{
}

} // namespace Exportable

extern "C" {

struct wpe_view_backend_interface exportable_view_backend_interface = {
    // create
    [](void*, struct wpe_view_backend* backend) -> void*
    {
        fprintf(stderr, "exportable_view_backend_interface::create()\n");

        return new Exportable::ViewBackend(backend);
    },
    // destroy
    [](void* data)
    {
        fprintf(stderr, "exportable_view_backend_interface::destroy()\n");

        auto* backend = static_cast<Exportable::ViewBackend*>(data);
        delete backend;
    },
    // initialize
    [](void* data)
    {
        fprintf(stderr, "exportable_view_backend_interface::initialize()\n");

        auto& backend = *static_cast<Exportable::ViewBackend*>(data);
        backend.initialize();
    },
    // get_renderer_host_fd
    [](void* data) -> int
    {
        fprintf(stderr, "exportable_view_backend_interface::get_renderer_host_fd()\n");

        auto& backend = *static_cast<Exportable::ViewBackend*>(data);
        return backend.gbmHost().releaseClientFD();
    },
};

__attribute__((visibility("default")))
struct wpe_view_backend*
wpe_mesa_view_backend_exportable_create()
{
    return wpe_view_backend_create_with_backend_interface(&exportable_view_backend_interface, 0);
}

}
