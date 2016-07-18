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

#ifndef wpe_renderer_h
#define wpe_renderer_h

#ifdef __cplusplus
extern "C" {
#endif

#include <EGL/eglplatform.h>

struct wpe_renderer_backend_egl;
struct wpe_renderer_backend_egl_target;
struct wpe_renderer_backend_egl_offscreen_target;

struct wpe_renderer_backend_egl_target_client;

struct wpe_renderer_backend_egl_interface {
    void* (*create)();
    void (*destroy)(void*);

    EGLNativeDisplayType (*get_native_display)(void*);
};

struct wpe_renderer_backend_egl_target_interface {
    void* (*create)(struct wpe_renderer_backend_egl_target*, int);
    void (*destroy)(void*);

    void (*initialize)(void*, void*, uint32_t, uint32_t);
    EGLNativeWindowType (*get_native_window)(void*);
    void (*resize)(void*, uint32_t, uint32_t);
    void (*frame_will_render)(void*);
    void (*frame_rendered)(void*);
};

struct wpe_renderer_backend_egl_offscreen_target_interface {
    void* (*create)();
    void (*destroy)(void*);

    void (*initialize)(void*, void*);
    EGLNativeWindowType (*get_native_window)(void*);
};


struct wpe_renderer_backend_egl*
wpe_renderer_backend_egl_create();

void
wpe_renderer_backend_egl_destroy(struct wpe_renderer_backend_egl*);

EGLNativeDisplayType
wpe_renderer_backend_egl_get_native_display(struct wpe_renderer_backend_egl*);


struct wpe_renderer_backend_egl_target*
wpe_renderer_backend_egl_target_create(int);

void
wpe_renderer_backend_egl_target_destroy(struct wpe_renderer_backend_egl_target*);

void
wpe_renderer_backend_egl_target_set_client(struct wpe_renderer_backend_egl_target*, struct wpe_renderer_backend_egl_target_client*, void*);

void
wpe_renderer_backend_egl_target_initialize(struct wpe_renderer_backend_egl_target*, struct wpe_renderer_backend_egl*, uint32_t, uint32_t);

EGLNativeWindowType
wpe_renderer_backend_egl_target_get_native_window(struct wpe_renderer_backend_egl_target*);

void
wpe_renderer_backend_egl_target_resize(struct wpe_renderer_backend_egl_target*, uint32_t, uint32_t);

void
wpe_renderer_backend_egl_target_frame_will_render(struct wpe_renderer_backend_egl_target*);

void
wpe_renderer_backend_egl_target_frame_rendered(struct wpe_renderer_backend_egl_target*);

struct wpe_renderer_backend_egl_offscreen_target*
wpe_renderer_backend_egl_offscreen_target_create();

void
wpe_renderer_backend_egl_offscreen_target_destroy(struct wpe_renderer_backend_egl_offscreen_target*);

void
wpe_renderer_backend_egl_offscreen_target_initialize(struct wpe_renderer_backend_egl_offscreen_target*, struct wpe_renderer_backend_egl*);

EGLNativeWindowType
wpe_renderer_backend_egl_offscreen_target_get_native_window(struct wpe_renderer_backend_egl_offscreen_target*);


struct wpe_renderer_backend_egl_target_client {
    void (*frame_complete)(void*);
};

void
wpe_renderer_backend_egl_target_dispatch_frame_complete(struct wpe_renderer_backend_egl_target*);


#ifdef __cplusplus
}
#endif

#endif // wpe_renderer_h
