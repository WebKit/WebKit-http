#include <wpe/loader.h>

#include "renderer-gbm.h"
#include "view-backend-drm.h"
#include "view-backend-wayland.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {

struct wpe_loader_interface _wpe_loader_interface = {
    [](const char* object_name) -> void* {
        if (!std::strcmp(object_name, "_wpe_view_backend_interface")) {
            if (std::getenv("WAYLAND_DISPLAY"))
                return reinterpret_cast<void*>(&wayland_view_backend_interface);
            return reinterpret_cast<void*>(&drm_view_backend_interface);
        }

        if (!std::strcmp(object_name, "_wpe_renderer_backend_egl_interface"))
            return &gbm_renderer_backend_egl_interface;
        if (!std::strcmp(object_name, "_wpe_renderer_backend_egl_target_interface"))
            return &gbm_renderer_backend_egl_target_interface;
        if (!std::strcmp(object_name, "_wpe_renderer_backend_egl_offscreen_target_interface"))
            return &gbm_renderer_backend_egl_offscreen_target_interface;

        return nullptr;
    },
};

}
