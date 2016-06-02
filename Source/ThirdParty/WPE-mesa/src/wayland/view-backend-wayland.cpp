#include <wpe/view-backend.h>

#include "display.h"
#include "gbm-connection.h"

#include "ivi-application-client-protocol.h"
#include "xdg-shell-client-protocol.h"
#include "wayland-drm-client-protocol.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <unistd.h>
#include <unordered_map>

namespace Wayland {

class ViewBackend : public GBM::Host::Client {
public:
    ViewBackend(struct wpe_view_backend*);
    virtual ~ViewBackend();

    void initialize();

    void importBufferFd(int) override;
    void commitBuffer(struct buffer_commit*) override;

    struct wpe_view_backend* backend() { return m_backend; }
    GBM::Host& gbmHost() { return m_renderer.gbmHost; }

    struct BufferListenerData {
        GBM::Host* gbmHost;
        std::unordered_map<uint32_t, struct wl_buffer*> map;
    };

    struct CallbackListenerData {
        GBM::Host* gbmHost;
        struct wl_callback* frameCallback;
    };

    struct ResizingData {
        struct wpe_view_backend* backend;
        uint32_t width;
        uint32_t height;
    };

private:
    Display& m_display;
    struct wpe_view_backend* m_backend;

    struct wl_surface* m_surface;
    struct xdg_surface* m_xdgSurface;
    struct ivi_surface* m_iviSurface;

    BufferListenerData m_bufferData { nullptr, decltype(m_bufferData.map){ } };
    CallbackListenerData m_callbackData { nullptr, nullptr };
    ResizingData m_resizingData { nullptr, 0, 0 };

    struct {
        GBM::Host gbmHost;
        int pendingBufferFd { -1 };
    } m_renderer;
};

static const struct xdg_surface_listener g_xdgSurfaceListener = {
    // configure
    [](void*, struct xdg_surface* surface, int32_t, int32_t, struct wl_array*, uint32_t serial)
    {
        xdg_surface_ack_configure(surface, serial);
    },
    // delete
    [](void*, struct xdg_surface*) { },
};

static const struct ivi_surface_listener g_iviSurfaceListener = {
    // configure
    [](void* data, struct ivi_surface*, int32_t width, int32_t height)
    {
        struct wpe_view_backend* backend = static_cast<ViewBackend::ResizingData*>(data)->backend;
        wpe_view_backend_dispatch_set_size(backend, std::max(0, width), std::max(0, height));
    },
};

const struct wl_buffer_listener g_bufferListener = {
    // release
    [](void* data, struct wl_buffer* buffer)
    {
        auto& bufferData = *static_cast<ViewBackend::BufferListenerData*>(data);
        auto& bufferMap = bufferData.map;

        auto it = std::find_if(bufferMap.begin(), bufferMap.end(),
            [buffer] (const std::pair<uint32_t, struct wl_buffer*>& entry) { return entry.second == buffer; });

        if (it == bufferMap.end())
            return;

        if (bufferData.gbmHost)
            bufferData.gbmHost->releaseBuffer(it->first);
    },
};

const struct wl_callback_listener g_callbackListener = {
    // frame
    [](void* data, struct wl_callback* callback, uint32_t)
    {
        auto& callbackData = *static_cast<ViewBackend::CallbackListenerData*>(data);
        if (callbackData.gbmHost)
            callbackData.gbmHost->frameComplete();
        callbackData.frameCallback = nullptr;
        wl_callback_destroy(callback);
    },
};

ViewBackend::ViewBackend(struct wpe_view_backend* backend)
    : m_display(Display::singleton())
    , m_backend(backend)
{
    m_renderer.gbmHost.initialize(*this);

    m_surface = wl_compositor_create_surface(m_display.interfaces().compositor);

    if (m_display.interfaces().xdg) {
        m_xdgSurface = xdg_shell_get_xdg_surface(m_display.interfaces().xdg, m_surface);
        xdg_surface_add_listener(m_xdgSurface, &g_xdgSurfaceListener, nullptr);
        xdg_surface_set_title(m_xdgSurface, "WPE");
    }

    if (m_display.interfaces().ivi_application) {
        m_iviSurface = ivi_application_surface_create(m_display.interfaces().ivi_application,
            4200 + getpid(), // a unique identifier
            m_surface);
        ivi_surface_add_listener(m_iviSurface, &g_iviSurfaceListener, &m_resizingData);
    }

    // Ensure the Pasteboard singleton is constructed early.
    // FIXME:
    // Pasteboard::Pasteboard::singleton();

    m_bufferData.gbmHost = &m_renderer.gbmHost;
    m_callbackData.gbmHost = &m_renderer.gbmHost;
    m_resizingData.backend = m_backend;
}

ViewBackend::~ViewBackend()
{
    m_backend = nullptr;

    m_renderer.gbmHost.deinitialize();

    m_display.unregisterInputClient(m_surface);

    m_bufferData = { nullptr, decltype(m_bufferData.map){ } };

    if (m_callbackData.frameCallback)
        wl_callback_destroy(m_callbackData.frameCallback);
    m_callbackData = { nullptr, nullptr };

    m_resizingData = { nullptr, 0, 0 };

    if (m_iviSurface)
        ivi_surface_destroy(m_iviSurface);
    m_iviSurface = nullptr;
    if (m_xdgSurface)
        xdg_surface_destroy(m_xdgSurface);
    m_xdgSurface = nullptr;
    if (m_surface)
        wl_surface_destroy(m_surface);
    m_surface = nullptr;
}

void ViewBackend::initialize()
{
    m_display.registerInputClient(m_surface, m_backend);
}

void ViewBackend::importBufferFd(int fd)
{
    if (m_renderer.pendingBufferFd != -1)
        close(m_renderer.pendingBufferFd);
    m_renderer.pendingBufferFd = fd;
}

void ViewBackend::commitBuffer(struct buffer_commit* commit)
{
    struct wl_buffer* buffer = nullptr;
    auto& bufferMap = m_bufferData.map;
    auto it = bufferMap.find(commit->handle);

    if (m_renderer.pendingBufferFd >= 0) {
        int fd = m_renderer.pendingBufferFd;
        m_renderer.pendingBufferFd = -1;

        buffer = wl_drm_create_prime_buffer(m_display.interfaces().drm, fd, commit->width, commit->height, WL_DRM_FORMAT_ARGB8888, 0, commit->stride, 0, 0, 0, 0);
        wl_buffer_add_listener(buffer, &g_bufferListener, &m_bufferData);

        if (it != bufferMap.end()) {
            wl_buffer_destroy(it->second);
            it->second = buffer;
        } else
            bufferMap.insert({ commit->handle, buffer });
    } else {
        assert(it != bufferMap.end());
        buffer = it->second;
    }

    if (!buffer) {
        fprintf(stderr, "ViewBackendWayland: failed to create/find a buffer for PRIME handle %u\n", commit->handle);
        return;
    }

    m_callbackData.frameCallback = wl_surface_frame(m_surface);
    wl_callback_add_listener(m_callbackData.frameCallback, &g_callbackListener, &m_callbackData);

    wl_surface_attach(m_surface, buffer, 0, 0);
    wl_surface_damage(m_surface, 0, 0, INT32_MAX, INT32_MAX);
    wl_surface_commit(m_surface);
    wl_display_flush(m_display.display());
}

} // namespace Wayland

extern "C" {

struct wpe_view_backend_interface wayland_view_backend_interface = {
    // create
    [](void*, struct wpe_view_backend* backend) -> void*
    {
        return new Wayland::ViewBackend(backend);
    },
    // destroy
    [](void* data)
    {
        auto* backend = static_cast<Wayland::ViewBackend*>(data);
        delete backend;
    },
    // initialize
    [](void* data) {
        auto* backend = static_cast<Wayland::ViewBackend*>(data);
        backend->initialize();
    },
    // get_renderer_host_fd
    [](void* data) -> int
    {
        auto* backend = static_cast<Wayland::ViewBackend*>(data);
        return backend->gbmHost().releaseClientFD();
    },
};

}
