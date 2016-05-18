#include <wpe/loader.h>

#include <cstdio>
#include <cstring>

#ifdef BACKEND_BCM_RPI
#include "rpi/interfaces.h"
#endif

extern "C" {

__attribute__((visibility("default")))
struct wpe_loader_interface _wpe_loader_interface = {
    [](const char* object_name) -> void* {

#ifdef BACKEND_BCM_RPI
        if (!std::strcmp(object_name, "_wpe_renderer_backend_egl_interface"))
            return &rpi_renderer_backend_egl_interface;
        if (!std::strcmp(object_name, "_wpe_renderer_backend_egl_target_interface"))
            return &rpi_renderer_backend_egl_target_interface;
        if (!std::strcmp(object_name, "_wpe_renderer_backend_egl_offscreen_target_interface"))
            return &rpi_renderer_backend_egl_offscreen_target_interface;

        if (!std::strcmp(object_name, "_wpe_view_backend_interface"))
            return &rpi_view_backend_interface;
#endif

        return nullptr;
    },
};

}
