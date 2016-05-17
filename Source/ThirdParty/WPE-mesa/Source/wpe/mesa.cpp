#include <wpe/loader.h>

#include "pasteboard-wayland.h"
#include "renderer-gbm.h"
#include "view-backend-drm.h"
#include "view-backend-wayland.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {

static bool under_wayland = !!std::getenv("WAYLAND_DISPLAY");

struct wpe_loader_interface _wpe_loader_interface = {
    [](const char* object_name) -> void* {
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

        if (!std::strcmp(object_name, "_wpe_pasteboard_interface") && under_wayland)
            return &wayland_pasteboard_interface;

        return nullptr;
    },
};

}
