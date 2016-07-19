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

#ifndef wpe_mesa_view_backend_exportable_h
#define wpe_mesa_view_backend_exportable_h

#ifdef __cplusplus
extern "C" {
#endif

#include <wpe/view-backend.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

struct wpe_mesa_view_backend_exportable;

struct wpe_mesa_view_backend_exportable_egl_image_data {
    uint32_t handle;
    EGLImageKHR image;
    int32_t width;
    int32_t height;
};

struct wpe_mesa_view_backend_exportable_client {
    void (*export_egl_image)(void*, struct wpe_mesa_view_backend_exportable_egl_image_data*);
};

struct wpe_mesa_view_backend_exportable*
wpe_mesa_view_backend_exportable_create(EGLDisplay, struct wpe_mesa_view_backend_exportable_client*, void*);

void
wpe_mesa_view_backend_exportable_destroy(struct wpe_mesa_view_backend_exportable*);

struct wpe_view_backend*
wpe_mesa_view_backend_exportable_get_view_backend(struct wpe_mesa_view_backend_exportable*);

void
wpe_mesa_view_backend_exportable_dispatch_frame_complete(struct wpe_mesa_view_backend_exportable*);

void
wpe_mesa_view_backend_exportable_dispatch_release_buffer(struct wpe_mesa_view_backend_exportable*, uint32_t);

#ifdef __cplusplus
}
#endif

#endif // wpe_mesa_view_backend_exportable_h
