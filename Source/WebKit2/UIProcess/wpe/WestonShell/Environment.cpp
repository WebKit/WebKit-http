#include "config.h"
#include "Environment.h"

#include <cstdlib>

namespace WPE {

class Output {
public:
    Output(struct weston_output*, struct weston_view*);

private:
    struct wl_list m_link;

    struct weston_output* m_output;
    struct wl_listener m_outputDestroyed;

    struct weston_view* m_baseView;

    static void outputDestroyed(struct wl_listener*, void* data);

    friend class Environment;
};

Output::Output(struct weston_output* output, struct weston_view* baseView)
    : m_output(output)
    , m_baseView(baseView)
{
    m_outputDestroyed.notify = outputDestroyed;
    wl_signal_add(&output->destroy_signal, &m_outputDestroyed);
}

void Output::outputDestroyed(struct wl_listener*, void*)
{
}

class Surface {
public:
    Surface(struct weston_surface*);

private:
    struct weston_surface* m_surface;
    struct weston_view* m_view;

    static void configure(struct weston_surface*, int32_t sx, int32_t sy);

    friend class Environment;
};

Surface::Surface(struct weston_surface* surface)
    : m_surface(surface)
    , m_view(weston_view_create(m_surface))
{
    surface->configure = configure;
    surface->configure_private = this;
}

void Surface::configure(struct weston_surface* surface, int32_t sx, int32_t sy)
{
    auto* shellSurface = static_cast<Surface*>(surface->configure_private);
    weston_view_set_position(shellSurface->m_view, sx, sy);
}

Environment::Environment(struct weston_compositor* compositor)
    : m_compositor(compositor)
    , m_outputSize(WKSizeMake(800, 600))
    , m_cursorView(nullptr)
{
    weston_layer_init(&m_layer, &m_compositor->cursor_layer.link);

    wl_list_init(&m_outputList);
    m_outputCreatedListener.notify = outputCreated;
    wl_signal_add(&m_compositor->output_created_signal, &m_outputCreatedListener);

    struct weston_output* output;
    wl_list_for_each(output, &m_compositor->output_list, link)
        createOutput(output);

    if (std::getenv("WPE_SHELL_MOUSE_CURSOR"))
        createCursor();

    wl_global_create(m_compositor->wl_display,
        &wl_wpe_interface, wl_wpe_interface.version, this,
        [](struct wl_client* client, void* data, uint32_t version, uint32_t id) {
            ASSERT(version == wl_wpe_interface.version);
            auto* environment = static_cast<Environment*>(data);
            struct wl_resource* resource = wl_resource_create(client, &wl_wpe_interface, version, id);
            wl_resource_set_implementation(resource, &m_wpeInterface, environment, nullptr);
        });
}

void Environment::updateCursorPosition(int x, int y)
{
    if (!m_cursorView)
        return;

    weston_view_set_position(m_cursorView, x - c_cursorOffset, y - c_cursorOffset);
    weston_compositor_schedule_repaint(m_compositor);
}

const struct wl_wpe_interface Environment::m_wpeInterface = {
    // register_surface
    [](struct wl_client*, struct wl_resource* shellResource, struct wl_resource* surfaceResource)
    {
        auto* shell = static_cast<Environment*>(wl_resource_get_user_data(shellResource));
        auto* surface = static_cast<struct weston_surface*>(wl_resource_get_user_data(surfaceResource));
        shell->registerSurface(surface);
    },
};

void Environment::createOutput(struct weston_output* output)
{
    m_outputSize = WKSizeMake(output->width, output->height);

    struct weston_surface* surface = weston_surface_create(m_compositor);
    weston_surface_set_color(surface, 0.0f, 0.75f, 0.0f, 1.0f);
	pixman_region32_fini(&surface->opaque);
	pixman_region32_init_rect(&surface->opaque, 0, 0, output->width, output->height);
	pixman_region32_fini(&surface->input);
	pixman_region32_init_rect(&surface->input, 0, 0, output->width, output->height);
    weston_surface_set_size(surface, output->width, output->height);

    struct weston_view* baseView = weston_view_create(surface);
    weston_view_set_position(baseView, output->x, output->y);
    weston_layer_entry_insert(&m_layer.view_list, &baseView->layer_link);

    auto* shellOutput = new Output(output, baseView);
    wl_list_insert(&m_outputList, &shellOutput->m_link);
}

void Environment::outputCreated(struct wl_listener*, void*)
{
}

void Environment::createCursor()
{
    struct weston_surface* surface = weston_surface_create(m_compositor);
    weston_surface_set_color(surface, 1.0f, 0.0f, 0.0f, 1.0f);
    pixman_region32_fini(&surface->opaque);
    pixman_region32_init_rect(&surface->opaque, 0, 0, c_cursorSize, c_cursorSize);
    weston_surface_set_size(surface, c_cursorSize, c_cursorSize);

    m_cursorView = weston_view_create(surface);
    weston_view_set_position(m_cursorView, -c_cursorOffset, -c_cursorOffset);
    weston_layer_entry_insert(&m_compositor->cursor_layer.view_list, &m_cursorView->layer_link);
}

void Environment::registerSurface(struct weston_surface* surface)
{
    auto* shellSurface = new Surface(surface);
    weston_layer_entry_insert(&m_layer.view_list, &shellSurface->m_view->layer_link);

    weston_compositor_schedule_repaint(m_compositor);
}

} // namespace WPE
