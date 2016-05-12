#include <wpe/view-backend.h>

#include "loader-private.h"
#include "view-backend-private.h"
#include <stdlib.h>


struct wpe_view_backend*
wpe_view_backend_create()
{
    struct wpe_view_backend_interface* backend_interface = wpe_load_object("_wpe_view_backend_interface");
    return wpe_view_backend_create_with_backend_interface(backend_interface);
}

struct wpe_view_backend*
wpe_view_backend_create_with_backend_interface(struct wpe_view_backend_interface* backend_interface)
{
    struct wpe_view_backend* backend = malloc(sizeof(struct wpe_view_backend));
    if (!backend)
        return 0;

    backend->interface = backend_interface;
    backend->interface_data = backend->interface->create(backend);

    return backend;
}

void
wpe_view_backend_destroy(struct wpe_view_backend* backend)
{
    backend->interface->destroy(backend->interface_data);
    backend->interface_data = 0;

    backend->backend_client = 0;
    backend->backend_client_data = 0;

    backend->input_client = 0;
    backend->input_client_data = 0;

    free(backend);
}

void
wpe_view_backend_set_backend_client(struct wpe_view_backend* backend, struct wpe_view_backend_client* client, void* client_data)
{
    backend->backend_client = client;
    backend->backend_client_data = client_data;
}

void
wpe_view_backend_set_input_client(struct wpe_view_backend* backend, struct wpe_view_backend_input_client* client, void* client_data)
{
    backend->input_client = client;
    backend->input_client_data = client_data;
}

void
wpe_view_backend_initialize(struct wpe_view_backend* backend)
{
    backend->interface->initialize(backend->interface_data);
}

int
wpe_view_backend_get_renderer_host_fd(struct wpe_view_backend* backend)
{
    return backend->interface->get_renderer_host_fd(backend->interface_data);
}

void
wpe_view_backend_dispatch_set_size(struct wpe_view_backend* backend, uint32_t width, uint32_t height)
{
    if (backend->backend_client)
        backend->backend_client->set_size(backend->backend_client_data, width, height);
}

void
wpe_view_backend_dispatch_keyboard_event(struct wpe_view_backend* backend, struct wpe_input_keyboard_event* event)
{
    if (backend->input_client)
        backend->input_client->handle_keyboard_event(backend->input_client_data, event);
}

void
wpe_view_backend_dispatch_pointer_event(struct wpe_view_backend* backend, struct wpe_input_pointer_event* event)
{
    if (backend->input_client)
        backend->input_client->handle_pointer_event(backend->input_client_data, event);
}

void
wpe_view_backend_dispatch_axis_event(struct wpe_view_backend* backend, struct wpe_input_axis_event* event)
{
    if (backend->input_client)
        backend->input_client->handle_axis_event(backend->input_client_data, event);
}

void
wpe_view_backend_dispatch_touch_event(struct wpe_view_backend* backend, struct wpe_input_touch_event* event)
{
    if (backend->input_client)
        backend->input_client->handle_touch_event(backend->input_client_data, event);
}
