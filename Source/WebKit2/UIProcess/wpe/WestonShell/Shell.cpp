/*
 * Copyright (C) 2014 Igalia S.L.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "Shell.h"

#include <WebKit/WKContextConfigurationRef.h>
#include <WebKit/WKContext.h>
#include <WebKit/WKPageGroup.h>
#include <WebKit/WKPage.h>
#include <WebKit/WKRetainPtr.h>
#include <WebKit/WKString.h>
#include <WebKit/WKURL.h>
#include <WebKit/WKView.h>

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

    friend class Shell;
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

    friend class Shell;
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
    auto shellSurface = static_cast<Surface*>(surface->configure_private);
    weston_view_set_position(shellSurface->m_view, sx, sy);
}


Shell::Shell(struct weston_compositor* compositor)
    : m_compositor(compositor)
    , m_outputSize(WKSizeMake(800, 600))
{
    weston_layer_init(&m_layer, &m_compositor->cursor_layer.link);

    wl_list_init(&m_outputList);
    m_outputCreatedListener.notify = outputCreated;
    wl_signal_add(&m_compositor->output_created_signal, &m_outputCreatedListener);

    struct weston_output* output;
    wl_list_for_each(output, &m_compositor->output_list, link)
        createOutput(output);

    wl_global_create(m_compositor->wl_display,
        &wl_wpe_interface, wl_wpe_interface.version, this,
        [](struct wl_client* client, void* data, uint32_t version, uint32_t id) {
            ASSERT(version == wl_wpe_interface.version);
            auto shell = static_cast<Shell*>(data);
            struct wl_resource* resource = wl_resource_create(client, &wl_wpe_interface, version, id);
            wl_resource_set_implementation(resource, &m_wpeInterface, shell, nullptr);
        });

    weston_compositor_set_default_pointer_grab(m_compositor, &m_pgInterface);
    weston_compositor_set_default_keyboard_grab(m_compositor, &m_kgInterface);
}

void Shell::createOutput(struct weston_output* output)
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
    wl_list_insert(&m_layer.view_list, &baseView->layer_link);

    auto shellOutput = new Output(output, baseView);
    wl_list_insert(&m_outputList, &shellOutput->m_link);
}

void Shell::registerSurface(struct weston_surface* surface)
{
    auto shellSurface = new Surface(surface);
    wl_list_insert(&m_layer.view_list, &shellSurface->m_view->layer_link);

    weston_compositor_schedule_repaint(m_compositor);
}

void Shell::outputCreated(struct wl_listener*, void*)
{
}

const struct wl_wpe_interface Shell::m_wpeInterface = {
    // register_surface
    [](struct wl_client*, struct wl_resource* shellResource, struct wl_resource* surfaceResource)
    {
        auto shell = static_cast<Shell*>(wl_resource_get_user_data(shellResource));
        auto surface = static_cast<struct weston_surface*>(wl_resource_get_user_data(surfaceResource));
        shell->registerSurface(surface);
    },
};

const struct weston_pointer_grab_interface Shell::m_pgInterface = {
    // focus
    [](struct weston_pointer_grab*) { },

    // motion
    [](struct weston_pointer_grab*, uint32_t, wl_fixed_t, wl_fixed_t)
    {
        // struct weston_pointer* pointer = grab->pointer;
        // weston_pointer_move(pointer, x, y);
        // X coordinate: wl_fixed_to_int(pointer->x);
        // Y coordinate: wl_fixed_to_int(pointer->y);
    },

    // button
    [](struct weston_pointer_grab*, uint32_t, uint32_t, uint32_t) { },

    // cancel
    [](struct weston_pointer_grab*) { }
};

const struct weston_keyboard_grab_interface Shell::m_kgInterface = {
    // key
    [](struct weston_keyboard_grab*, uint32_t time, uint32_t key, uint32_t state) { },

    // modifiers
    [](struct weston_keyboard_grab*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) { },

    // cancel
    [](struct weston_keyboard_grab*) { }
};

Shell* Shell::m_instance = nullptr;

gpointer Shell::launchWPE(gpointer data)
{
    Shell::m_instance = static_cast<Shell*>(data);

    GMainContext* threadContext = g_main_context_new();
    GMainLoop* threadLoop = g_main_loop_new(threadContext, FALSE);

    g_main_context_push_thread_default(threadContext);

    auto pageGroupIdentifier = adoptWK(WKStringCreateWithUTF8CString("WPEPageGroup"));
    auto pageGroup = adoptWK(WKPageGroupCreateWithIdentifier(pageGroupIdentifier.get()));

    auto contextConfiguration = adoptWK(WKContextConfigurationCreate());
    auto context = adoptWK(WKContextCreateWithConfiguration(contextConfiguration.get()));

    auto view = adoptWK(WKViewCreate(context.get(), pageGroup.get()));
    WKViewResize(view.get(), Shell::instance().m_outputSize);

    const char* url = g_getenv("WPE_SHELL_URL");
    if (!url)
        url = "https://www.youtube.com/tv/";
    auto shellURL = adoptWK(WKURLCreateWithUTF8CString(url));
    WKPageLoadURL(WKViewGetPage(view.get()), shellURL.get());

    g_main_loop_run(threadLoop);
    return nullptr;
}

} // namespace WPE
