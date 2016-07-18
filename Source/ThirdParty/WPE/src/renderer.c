/*
 * Copyright (C) 2015, 2016 Igalia S.L.
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

#include <wpe/renderer-backend-egl.h>

#include "loader-private.h"
#include "renderer-private.h"
#include <stdlib.h>


__attribute__((visibility("default")))
struct wpe_renderer_backend_egl*
wpe_renderer_backend_egl_create(int host_fd)
{
    struct wpe_renderer_backend_egl* backend = malloc(sizeof(struct wpe_renderer_backend_egl));
    if (!backend)
        return 0;

    backend->interface = wpe_load_object("_wpe_renderer_backend_egl_interface");
    backend->interface_data = backend->interface->create(host_fd);

    return backend;
}

__attribute__((visibility("default")))
void
wpe_renderer_backend_egl_destroy(struct wpe_renderer_backend_egl* backend)
{
    backend->interface->destroy(backend->interface_data);
    backend->interface_data = 0;

    free(backend);
}

__attribute__((visibility("default")))
EGLNativeDisplayType
wpe_renderer_backend_egl_get_native_display(struct wpe_renderer_backend_egl* backend)
{
    return backend->interface->get_native_display(backend->interface_data);
}

__attribute__((visibility("default")))
struct wpe_renderer_backend_egl_target*
wpe_renderer_backend_egl_target_create(int host_fd)
{
    struct wpe_renderer_backend_egl_target* target = malloc(sizeof(struct wpe_renderer_backend_egl_target));
    if (!target)
        return 0;

    target->interface = wpe_load_object("_wpe_renderer_backend_egl_target_interface");
    target->interface_data = target->interface->create(target, host_fd);

    return target;
}

__attribute__((visibility("default")))
void
wpe_renderer_backend_egl_target_destroy(struct wpe_renderer_backend_egl_target* target)
{
    target->interface->destroy(target->interface_data);
    target->interface_data = 0;

    target->client = 0;
    target->client_data = 0;

    free(target);
}

__attribute__((visibility("default")))
void
wpe_renderer_backend_egl_target_set_client(struct wpe_renderer_backend_egl_target* target, struct wpe_renderer_backend_egl_target_client* client, void* client_data)
{
    target->client = client;
    target->client_data = client_data;
}

__attribute__((visibility("default")))
void
wpe_renderer_backend_egl_target_initialize(struct wpe_renderer_backend_egl_target* target, struct wpe_renderer_backend_egl* backend, uint32_t width, uint32_t height)
{
    target->interface->initialize(target->interface_data, backend->interface_data, width, height);
}

__attribute__((visibility("default")))
EGLNativeWindowType
wpe_renderer_backend_egl_target_get_native_window(struct wpe_renderer_backend_egl_target* target)
{
    return target->interface->get_native_window(target->interface_data);
}

__attribute__((visibility("default")))
void
wpe_renderer_backend_egl_target_resize(struct wpe_renderer_backend_egl_target* target, uint32_t width, uint32_t height)
{
    target->interface->resize(target->interface_data, width, height);
}

__attribute__((visibility("default")))
void
wpe_renderer_backend_egl_target_frame_will_render(struct wpe_renderer_backend_egl_target* target)
{
    target->interface->frame_will_render(target->interface_data);
}

__attribute__((visibility("default")))
void
wpe_renderer_backend_egl_target_frame_rendered(struct wpe_renderer_backend_egl_target* target)
{
    target->interface->frame_rendered(target->interface_data);
}

__attribute__((visibility("default")))
struct wpe_renderer_backend_egl_offscreen_target*
wpe_renderer_backend_egl_offscreen_target_create()
{
    struct wpe_renderer_backend_egl_offscreen_target* target = malloc(sizeof(struct wpe_renderer_backend_egl_offscreen_target));
    if (!target)
        return 0;

    target->interface = wpe_load_object("_wpe_renderer_backend_egl_offscreen_target_interface");
    target->interface_data = target->interface->create();

    return target;
}

__attribute__((visibility("default")))
void
wpe_renderer_backend_egl_offscreen_target_destroy(struct wpe_renderer_backend_egl_offscreen_target* target)
{
    target->interface->destroy(target->interface_data);
    target->interface_data = 0;

    free(target);
}

__attribute__((visibility("default")))
void
wpe_renderer_backend_egl_offscreen_target_initialize(struct wpe_renderer_backend_egl_offscreen_target* target, struct wpe_renderer_backend_egl* backend)
{
    target->interface->initialize(target->interface_data, backend->interface_data);
}

__attribute__((visibility("default")))
EGLNativeWindowType
wpe_renderer_backend_egl_offscreen_target_get_native_window(struct wpe_renderer_backend_egl_offscreen_target* target)
{
    return target->interface->get_native_window(target->interface_data);
}

__attribute__((visibility("default")))
void
wpe_renderer_backend_egl_target_dispatch_frame_complete(struct wpe_renderer_backend_egl_target* target)
{
    if (target->client)
        target->client->frame_complete(target->client_data);
}
