/*
 * Copyright (C) 2015, 2016 Igalia S.L.
 * Copyright (C) 2015, 2016 Metrological
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

#define WL_EGL_PLATFORM 1

#include <wpe/renderer-backend-egl.h>

#include <cstring>
#include <glib.h>
#include <wayland-client.h>
#include <wayland-egl.h>

#if defined(WPE_BACKEND_MESA)
#include <gbm.h>
#include <fcntl.h>
#include <unistd.h>

#define DEFAULT_CARD "/dev/dri/card0"
#define DEFAULT_MODE_WIDTH (1280)
#define DEFAULT_MODE_HEIGHT (720)

namespace GBM{

struct EGLOffscreenTarget {
    ~EGLOffscreenTarget()
    {
        if (surface)
            gbm_surface_destroy(surface);
    }

    struct gbm_surface* surface;

    void initialize(struct gbm_device* device, uint32_t width, uint32_t height)
    {
       if (device)
           surface = gbm_surface_create(device, width, height, GBM_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
       else{
           printf("EGLOffscreenTarget: gbm device is null\n");
           return;
       }

       if (!surface)
           printf("EGLOffscreenTarget: gbm surface created failed\n");
    }
};

struct Backend {
    Backend()
    {
        fd = open(DEFAULT_CARD, O_RDWR | O_CLOEXEC | O_NOCTTY | O_NONBLOCK);
        if (fd < 0){
            printf("Backend: DRM device open failure\n");
            return;
        }

        device = gbm_create_device(fd);
        if (!device) {
            printf("Backend: gbm device not created\n");
            close(fd);
            return;
        }
    }

    ~Backend()
    {
        if (device)
            gbm_device_destroy(device);

        if (fd >= 0)
            close(fd);
    }

    int fd { -1 };
    struct gbm_device* device;
};
}
#endif

namespace Westeros {

class EventSource {
public:
    static GSourceFuncs sourceFuncs;

    GSource source;
    GPollFD pfd;
    struct wl_display* display;
};

GSourceFuncs EventSource::sourceFuncs = {
    // prepare
    [](GSource* base, gint* timeout) -> gboolean
    {
        auto* source = reinterpret_cast<EventSource*>(base);
        struct wl_display* display = source->display;

        *timeout = -1;
        wl_display_flush(display);
        wl_display_dispatch_pending(display);

        return FALSE;
    },
    // check
    [](GSource* base) -> gboolean
    {
        auto* source = reinterpret_cast<EventSource*>(base);
        return !!source->pfd.revents;
    },
    // dispatch
    [](GSource* base, GSourceFunc, gpointer) -> gboolean
    {
        auto* source = reinterpret_cast<EventSource*>(base);
        struct wl_display* display = source->display;

        if (source->pfd.revents & G_IO_IN)
            wl_display_dispatch(display);

        if (source->pfd.revents & (G_IO_ERR | G_IO_HUP))
            return FALSE;

        source->pfd.revents = 0;
        return TRUE;
    },
    nullptr, // finalize
    nullptr, // closure_callback
    nullptr, // closure_marshall
};

class Backend {
public:
    Backend();
    ~Backend();

    struct wl_display* display() const { return m_display; }
    struct wl_compositor* compositor() const { return m_compositor; }

    void initialize();

private:
    static struct wl_registry_listener s_registryListener;

    struct wl_display* m_display { nullptr };
    struct wl_registry* m_registry { nullptr };
    struct wl_compositor* m_compositor { nullptr };
    GSource* m_eventSource { nullptr };
};

Backend::Backend()
{
    m_display = wl_display_connect(nullptr);
    if (!m_display)
        return;

    m_registry = wl_display_get_registry(m_display);
    wl_registry_add_listener(m_registry, &s_registryListener, this);
    wl_display_roundtrip(m_display);
}

Backend::~Backend()
{
    if (m_eventSource)
        g_source_destroy(m_eventSource);

    if (m_compositor)
        wl_compositor_destroy(m_compositor);
    if (m_registry)
        wl_registry_destroy(m_registry);
    if (m_display)
        wl_display_disconnect(m_display);
}

void Backend::initialize()
{
    if (m_eventSource != nullptr)
        return;

    m_eventSource = g_source_new(&EventSource::sourceFuncs, sizeof(EventSource));
    auto& source = *reinterpret_cast<EventSource*>(m_eventSource);
    source.display = m_display;

    source.pfd.fd = wl_display_get_fd(m_display);
    source.pfd.events = G_IO_IN | G_IO_ERR | G_IO_HUP;
    source.pfd.revents = 0;
    g_source_add_poll(m_eventSource, &source.pfd);

    g_source_set_name(m_eventSource, "[WPE] WaylandDisplay");
    g_source_set_priority(m_eventSource, G_PRIORITY_HIGH + 30);
    g_source_set_can_recurse(m_eventSource, TRUE);

    g_source_attach(m_eventSource, g_main_context_get_thread_default());
}

struct wl_registry_listener Backend::s_registryListener = {
    // global
    [](void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t)
    {
        auto& backend = *static_cast<Backend*>(data);

        if (!std::strcmp(interface, "wl_compositor"))
            backend.m_compositor = static_cast<struct wl_compositor*>(wl_registry_bind(registry, name, &wl_compositor_interface, 1));
    },
    // global_remove
    [](void*, struct wl_registry*, uint32_t)
    {
        // FIXME: if this can happen without the UI Process getting shut down
        // we should probably destroy our cached display instance.
    },
};

class EGLTarget {
public:
    EGLTarget(struct wpe_renderer_backend_egl_target*);
    virtual ~EGLTarget();

    void initialize(const Backend&, uint32_t, uint32_t);

    struct wl_egl_window* nativeWindow() const { return m_window; }

    void resize(uint32_t width, uint32_t height);
    void frameWillRender();
    void frameRendered();

private:
    static const struct wl_callback_listener s_frameListener;

    struct wpe_renderer_backend_egl_target* m_target;

    const Backend* m_backend;
    struct wl_surface* m_surface;
    struct wl_egl_window* m_window;
};

EGLTarget::EGLTarget(struct wpe_renderer_backend_egl_target* target)
    : m_target(target)
{
}

EGLTarget::~EGLTarget()
{
    if (m_window)
        wl_egl_window_destroy(m_window);
    if (m_surface)
        wl_surface_destroy(m_surface);
}

void EGLTarget::initialize(const Backend& backend, uint32_t width, uint32_t height)
{
    const_cast<Backend&>(backend).initialize();
    m_backend = &backend;
    m_surface = wl_compositor_create_surface(backend.compositor());
    if (m_surface)
        m_window = wl_egl_window_create(m_surface, width, height);
}

void EGLTarget::resize(uint32_t width, uint32_t height)
{
    if (m_window) {
        wl_egl_window_resize(m_window, width, height, 0, 0);
        if (m_backend && m_backend->display())
            wl_display_flush(m_backend->display());
    }
}

void EGLTarget::frameWillRender()
{
    struct wl_callback* frameCallback = wl_surface_frame(m_surface);
    wl_callback_add_listener(frameCallback, &s_frameListener, m_target);
}

void EGLTarget::frameRendered()
{
    if (m_backend && m_backend->display())
        wl_display_flush(m_backend->display());
}

const struct wl_callback_listener EGLTarget::s_frameListener = {
    // frame
    [](void* data, struct wl_callback* callback, uint32_t)
    {
        wl_callback_destroy(callback);

        auto* target = static_cast<struct wpe_renderer_backend_egl_target*>(data);
        wpe_renderer_backend_egl_target_dispatch_frame_complete(target);
    },
};

} // namespace Westeros

extern "C" {

struct wpe_renderer_backend_egl_interface westeros_renderer_backend_egl_interface = {
    // create
    [](int) -> void*
    {
        return new Westeros::Backend;
    },
    // destroy
    [](void* data)
    {
        auto* backend = static_cast<Westeros::Backend*>(data);
        delete backend;
    },
    // get_native_display
    [](void* data) -> EGLNativeDisplayType
    {
        auto& backend = *static_cast<Westeros::Backend*>(data);
        return backend.display();
    },
};

struct wpe_renderer_backend_egl_target_interface westeros_renderer_backend_egl_target_interface = {
    // create
    [](struct wpe_renderer_backend_egl_target* target, int) -> void*
    {
        return new Westeros::EGLTarget(target);
    },
    // destroy
    [](void* data)
    {
        auto* target = static_cast<Westeros::EGLTarget*>(data);
        delete target;
    },
    // initialize
    [](void* data, void* backend_data, uint32_t width, uint32_t height)
    {
        auto& target = *static_cast<Westeros::EGLTarget*>(data);
        const auto& backend = *static_cast<Westeros::Backend*>(backend_data);
        target.initialize(backend, width, height);
    },
    // get_native_window
    [](void* data) -> EGLNativeWindowType
    {
        auto& target = *static_cast<Westeros::EGLTarget*>(data);
        return (EGLNativeWindowType)target.nativeWindow();
    },
    // resize
    [](void* data, uint32_t width, uint32_t height)
    {
        auto& target = *static_cast<Westeros::EGLTarget*>(data);
        target.resize(width, height);
    },
    // frame_will_render
    [](void* data)
    {
        auto& target = *static_cast<Westeros::EGLTarget*>(data);
        target.frameWillRender();
    },
    // frame_rendered
    [](void* data)
    {
        auto& target = *static_cast<Westeros::EGLTarget*>(data);
        target.frameRendered();
    },
};

struct wpe_renderer_backend_egl_offscreen_target_interface westeros_renderer_backend_egl_offscreen_target_interface = {
    // create
    []() -> void*
    {
#if defined(WPE_BACKEND_MESA)        
        return new GBM::EGLOffscreenTarget;
#else
        return nullptr;
#endif        
    },
    // destroy
    [](void* data)
    {
#if defined(WPE_BACKEND_MESA)        
        auto* target = static_cast<GBM::EGLOffscreenTarget*>(data);
        delete target;
#endif        
    },
    // initialize
    [](void* data, void* backend_data)
    {
#if defined(WPE_BACKEND_MESA)        
        auto* backend = new GBM::Backend;
        auto* target = static_cast<GBM::EGLOffscreenTarget*>(data);
        target->initialize(backend->device, DEFAULT_MODE_WIDTH, DEFAULT_MODE_HEIGHT);
#endif        
    },
    // get_native_window
    [](void* data) -> EGLNativeWindowType
    {
#if defined(WPE_BACKEND_MESA)        
        auto* target = static_cast<GBM::EGLOffscreenTarget*>(data);
        printf("westeros_renderer_backend_egl_offscreen_target_interface: native window %x\n", target->surface);`        
        return (EGLNativeWindowType)target->surface;
#else
        return (EGLNativeWindowType)nullptr;
#endif        
    },
};

}
