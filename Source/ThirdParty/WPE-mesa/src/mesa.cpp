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

#include <wpe/loader.h>

#include "input-libxkbcommon.h"
#include "nested-compositor.h"
#include "pasteboard-wayland.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if 0
#include "renderer-gbm.h"
#include "view-backend-drm.h"
#include "view-backend-wayland.h"
#endif

extern "C" {

static bool under_wayland = !!std::getenv("WAYLAND_DISPLAY");

__attribute__((visibility("default")))
struct wpe_loader_interface _wpe_loader_interface = {
    [](const char* object_name) -> void* {
        if (!std::strcmp(object_name, "_wpe_renderer_backend_egl_interface"))
            return &nc_renderer_backend_egl_interface;
        if (!std::strcmp(object_name, "_wpe_renderer_backend_egl_target_interface"))
            return &nc_renderer_backend_egl_target_interface;
        if (!std::strcmp(object_name, "_wpe_renderer_backend_egl_offscreen_target_interface"))
            return &nc_renderer_backend_egl_offscreen_target_interface;
        if (!std::strcmp(object_name, "_wpe_renderer_host_interface"))
            return &nc_renderer_host_interface;
        if (!std::strcmp(object_name, "_wpe_view_backend_interface"))
            return &nc_view_backend_interface;

#if 0
        if (!std::strcmp(object_name, "_wpe_view_backend_interface")) {
            if (under_wayland)
                return reinterpret_cast<void*>(&wayland_view_backend_interface);
            return reinterpret_cast<void*>(&drm_view_backend_interface);
        }

        if (!std::strcmp(object_name, "_wpe_renderer_backend_egl_interface"))
            return &gbm_renderer_backend_egl_interface;
        if (!std::strcmp(object_name, "_wpe_renderer_backend_egl_target_interface"))
            return &gbm_renderer_backend_egl_target_interface;
        if (!std::strcmp(object_name, "_wpe_renderer_backend_egl_offscreen_target_interface"))
            return &gbm_renderer_backend_egl_offscreen_target_interface;
#endif

        if (!std::strcmp(object_name, "_wpe_pasteboard_interface") && under_wayland)
            return &wayland_pasteboard_interface;

        if (!std::strcmp(object_name, "_wpe_input_key_mapper_interface"))
            return &libxkbcommon_input_key_mapper_interface;

        return nullptr;
    },
};

}
