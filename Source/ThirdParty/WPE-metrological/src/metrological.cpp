#include <wpe/loader.h>

#include <cstdio>
#include <cstring>

#ifdef BACKEND_BCM_RPI
#include "bcm-rpi/interfaces.h"
#endif

#ifdef BACKEND_INTELCE
#include "intelce/interfaces.h"
#endif

extern "C" {

__attribute__((visibility("default")))
struct wpe_loader_interface _wpe_loader_interface = {
    [](const char* object_name) -> void* {

#ifdef BACKEND_BCM_RPI
        if (!std::strcmp(object_name, "_wpe_renderer_backend_egl_interface"))
            return &bcm_rpi_renderer_backend_egl_interface;
        if (!std::strcmp(object_name, "_wpe_renderer_backend_egl_target_interface"))
            return &bcm_rpi_renderer_backend_egl_target_interface;
        if (!std::strcmp(object_name, "_wpe_renderer_backend_egl_offscreen_target_interface"))
            return &bcm_rpi_renderer_backend_egl_offscreen_target_interface;

        if (!std::strcmp(object_name, "_wpe_view_backend_interface"))
            return &bcm_rpi_view_backend_interface;
#endif

#ifdef BACKEND_INTELCE
        if (!std::strcmp(object_name, "_wpe_renderer_backend_egl_interface"))
            return &intelce_renderer_backend_egl_interface;
        if (!std::strcmp(object_name, "_wpe_renderer_backend_egl_target_interface"))
            return &intelce_renderer_backend_egl_target_interface;
        if (!std::strcmp(object_name, "_wpe_renderer_backend_egl_offscreen_target_interface"))
            return &intelce_renderer_backend_egl_offscreen_target_interface;

        if (!std::strcmp(object_name, "_wpe_view_backend_interface"))
            return &intelce_view_backend_interface;
#endif

        return nullptr;
    },
};

}
