#include "Config.h"

#if WPE_BACKEND(WESTEROS)

#include "RenderingBackendWesteros.h"
#include <EGL/egl.h>
#include <cstring>
#include <array>
#include <unordered_map>
#include <utility>
#include <glib.h>

namespace WPE {

namespace Graphics {

typedef struct _GSource GSource;

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

static wl_display* g_WlDisplay = nullptr;

static struct wl_registry_listener registryListener = {

    RenderingBackendWesteros::globalCallback,
    RenderingBackendWesteros::globalRemoveCallback
};

void RenderingBackendWesteros::globalCallback(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t)
{
    auto& backend = *static_cast<RenderingBackendWesteros*>(data);

    if (!std::strcmp(interface, "wl_compositor"))
        backend.m_compositor = static_cast<struct wl_compositor*>(wl_registry_bind(registry, name, &wl_compositor_interface, 1));
}

void RenderingBackendWesteros::globalRemoveCallback(void*, struct wl_registry*, uint32_t)
{
    // FIXME: if this can happen without the UI Process getting shut down
    // we should probably destroy our cached display instance.
}

RenderingBackendWesteros::RenderingBackendWesteros()
 : m_display(nullptr)
 , m_registry(nullptr)
 , m_compositor(nullptr)
{
    m_display = wl_display_connect(nullptr);
    g_WlDisplay = m_display;

    if(!m_display)
        return;

    m_registry = wl_display_get_registry(m_display);
    wl_registry_add_listener(m_registry, &registryListener, this);
    wl_display_roundtrip(m_display);

    m_eventSource = g_source_new(&EventSource::sourceFuncs, sizeof(EventSource));
    auto* source = reinterpret_cast<EventSource*>(m_eventSource);
    source->display = m_display;

    source->pfd.fd = wl_display_get_fd(m_display);
    source->pfd.events = G_IO_IN | G_IO_ERR | G_IO_HUP;
    source->pfd.revents = 0;
    g_source_add_poll(m_eventSource, &source->pfd);

    g_source_set_name(m_eventSource, "[WPE] WaylandDisplay");
    g_source_set_priority(m_eventSource, G_PRIORITY_HIGH + 30);
    g_source_set_can_recurse(m_eventSource, TRUE);
    g_source_attach(m_eventSource, g_main_context_get_thread_default());
}

RenderingBackendWesteros::~RenderingBackendWesteros()
{
    if (m_eventSource)
        g_source_unref(m_eventSource);
    m_eventSource = nullptr;

    g_WlDisplay = nullptr;

    if (m_compositor)
        wl_compositor_destroy(m_compositor);
    if (m_registry)
        wl_registry_destroy(m_registry);
    if (m_display)
        wl_display_disconnect(m_display);
}

EGLNativeDisplayType RenderingBackendWesteros::nativeDisplay()
{
    return m_display;
}

std::unique_ptr<RenderingBackend::Surface> RenderingBackendWesteros::createSurface(uint32_t width, uint32_t height, uint32_t targetHandle, RenderingBackend::Surface::Client& client)
{
    return std::unique_ptr<RenderingBackendWesteros::Surface>(new RenderingBackendWesteros::Surface(*this, width, height, targetHandle, client));
}

std::unique_ptr<RenderingBackend::OffscreenSurface> RenderingBackendWesteros::createOffscreenSurface()
{
    return std::unique_ptr<RenderingBackendWesteros::OffscreenSurface>(new RenderingBackendWesteros::OffscreenSurface(*this));
}

RenderingBackendWesteros::Surface::Surface(const RenderingBackendWesteros& backend, uint32_t width, uint32_t height, uint32_t, RenderingBackendWesteros::Surface::Client&)
{
    m_surface = wl_compositor_create_surface(backend.m_compositor);

    if (m_surface)
        m_window = wl_egl_window_create(m_surface, width, height);
}

RenderingBackendWesteros::Surface::~Surface()
{
    if (m_window)
        wl_egl_window_destroy(m_window);
    if (m_surface)
        wl_surface_destroy(m_surface);
}

EGLNativeWindowType RenderingBackendWesteros::Surface::nativeWindow()
{
    return m_window;
}

void RenderingBackendWesteros::Surface::resize(uint32_t width, uint32_t height)
{
    if(m_window) {
        wl_egl_window_resize(m_window, width, height, 0, 0);
        if(g_WlDisplay)
            wl_display_flush(g_WlDisplay);
    }
}

RenderingBackend::BufferExport RenderingBackendWesteros::Surface::lockFrontBuffer()
{
    // This needs to be called for every eglSwapBuffers for the events to get flushed.
    if(g_WlDisplay)
        wl_display_flush(g_WlDisplay);
    return std::make_tuple(-1, nullptr, 0);
}

void RenderingBackendWesteros::Surface::releaseBuffer(uint32_t)
{
}

RenderingBackendWesteros::OffscreenSurface::OffscreenSurface(const RenderingBackendWesteros&)
{
}

RenderingBackendWesteros::OffscreenSurface::~OffscreenSurface() = default;

EGLNativeWindowType RenderingBackendWesteros::OffscreenSurface::nativeWindow()
{
    return nullptr;
}

} // namespace Graphics

} // namespace WPE

#endif // WPE_BACKEND(WESTEROS);
