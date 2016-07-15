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

#ifndef wpe_view_backend_h
#define wpe_view_backend_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

struct wpe_view_backend;

struct wpe_input_axis_event;
struct wpe_input_keyboard_event;
struct wpe_input_pointer_event;
struct wpe_input_touch_event;

struct wpe_view_backend_client;
struct wpe_view_backend_input_client;

struct wpe_view_backend_interface {
    void* (*create)(void*, struct wpe_view_backend*);
    void (*destroy)(void*);

    void (*initialize)(void*);
    int (*get_renderer_host_fd)(void*);
};


struct wpe_view_backend*
wpe_view_backend_create();

struct wpe_view_backend*
wpe_view_backend_create_with_backend_interface(struct wpe_view_backend_interface*, void*);

void
wpe_view_backend_destroy(struct wpe_view_backend*);

void 
wpe_view_backend_set_backend_client(struct wpe_view_backend*, struct wpe_view_backend_client*, void*);

void
wpe_view_backend_set_input_client(struct wpe_view_backend*, struct wpe_view_backend_input_client*, void*);

void
wpe_view_backend_initialize(struct wpe_view_backend*);

int
wpe_view_backend_get_renderer_host_fd(struct wpe_view_backend*);


struct wpe_view_backend_client {
    void (*set_size)(void*, uint32_t, uint32_t);
    void (*frame_displayed)(void*);
};

void
wpe_view_backend_dispatch_set_size(struct wpe_view_backend*, uint32_t, uint32_t);

void
wpe_view_backend_dispatch_frame_displayed(struct wpe_view_backend*);


struct wpe_view_backend_input_client {
    void (*handle_keyboard_event)(void*, struct wpe_input_keyboard_event*);
    void (*handle_pointer_event)(void*, struct wpe_input_pointer_event*);
    void (*handle_axis_event)(void*, struct wpe_input_axis_event*);
    void (*handle_touch_event)(void*, struct wpe_input_touch_event*);
};

void
wpe_view_backend_dispatch_keyboard_event(struct wpe_view_backend*, struct wpe_input_keyboard_event*);

void
wpe_view_backend_dispatch_pointer_event(struct wpe_view_backend*, struct wpe_input_pointer_event*);

void
wpe_view_backend_dispatch_axis_event(struct wpe_view_backend*, struct wpe_input_axis_event*);

void
wpe_view_backend_dispatch_touch_event(struct wpe_view_backend*, struct wpe_input_touch_event*);

#ifdef __cplusplus
}
#endif

#endif // wpe_view_backend_h
