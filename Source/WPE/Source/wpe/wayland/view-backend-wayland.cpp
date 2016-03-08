#include "view-backend-wayland.h"

#include "display.h"
#include "view-backend-private.h"

#include "ivi-application-client-protocol.h"
#include "xdg-shell-client-protocol.h"
#include "wayland-drm-client-protocol.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <unistd.h>
#include <unordered_map>

namespace Wayland {

class ViewBackend {
public:
    ViewBackend(struct wpe_view_backend*);
    ~ViewBackend();

    struct wpe_view_backend* backend() { return m_backend; }

    struct BufferListenerData {
        void* client;
        std::unordered_map<uint32_t, struct wl_buffer*> map;
    };

    struct CallbackListenerData {
        void* client;
        struct wl_callback* frameCallback;
    };

    struct ResizingData {
        void* client;
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
        // FIXME:
        // auto& resizingData = *static_cast<ViewBackend::ResizingData*>(data);
        // if (resizingData.client)
        //     resizingData.client->setSize(std::max(0, width), std::max(0, height));
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

        // FIXME:
        // if (bufferData.client)
        //     bufferData.client->releaseBuffer(it->first);
    },
};

const struct wl_callback_listener g_callbackListener = {
    // frame
    [](void* data, struct wl_callback* callback, uint32_t)
    {
        auto& callbackData = *static_cast<ViewBackend::CallbackListenerData*>(data);
        // FIXME:
        // if (callbackData.client)
        //     callbackData.client->frameComplete();
        callbackData.frameCallback = nullptr;
        wl_callback_destroy(callback);
    },
};

ViewBackend::ViewBackend(struct wpe_view_backend* backend)
    : m_display(Display::singleton())
    , m_backend(backend)
{
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
}

ViewBackend::~ViewBackend()
{
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

} // namespace Wayland

extern "C" {

const struct wpe_view_backend_interface wayland_view_backend_interface = {
    // create
    [](struct wpe_view_backend* backend) -> void*
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
        auto* backend = static_cast<Wayland::ViewBackend*>(data)->backend();
        if (backend->backend_client)
            backend->backend_client->set_size(backend->backend_client_data, 800, 600);
    },
    // get_renderer_host_fd
    [](void*) -> int
    {
        return -1;
    },
};

}
