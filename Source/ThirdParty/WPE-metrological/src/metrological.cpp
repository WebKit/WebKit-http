#include <wpe/loader.h>

#include <cstdio>
#include <cstring>

#ifdef BACKEND_BCM_NEXUS
#include "bcm-nexus/interfaces.h"
#endif

#ifdef BACKEND_BCM_RPI
#include "bcm-rpi/interfaces.h"
#endif

#ifdef BACKEND_INTELCE
#include "intelce/interfaces.h"
#endif

#ifdef BACKEND_STM
#include "stm/interfaces.h"
#endif

#ifdef BACKEND_WESTEROS
#include "westeros/interfaces.h"
#endif

#ifdef KEY_INPUT_HANDLING_XKB
#include "input/XKB/input-libxkbcommon.h"
#endif

#ifdef KEY_INPUT_HANDLING_LINUX_INPUT
#include "input/LinuxInput/input-linuxinput.h"
#endif

extern "C" {

__attribute__((visibility("default")))
struct wpe_loader_interface _wpe_loader_interface = {
    [](const char* object_name) -> void* {

#ifdef BACKEND_BCM_NEXUS
        if (!std::strcmp(object_name, "_wpe_renderer_backend_egl_interface"))
            return &bcm_nexus_renderer_backend_egl_interface;
        if (!std::strcmp(object_name, "_wpe_renderer_backend_egl_target_interface"))
            return &bcm_nexus_renderer_backend_egl_target_interface;
        if (!std::strcmp(object_name, "_wpe_renderer_backend_egl_offscreen_target_interface"))
            return &bcm_nexus_renderer_backend_egl_offscreen_target_interface;

        if (!std::strcmp(object_name, "_wpe_view_backend_interface"))
            return &bcm_nexus_view_backend_interface;
#endif

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

#ifdef BACKEND_STM
        if (!std::strcmp(object_name, "_wpe_renderer_backend_egl_interface"))
            return &stm_renderer_backend_egl_interface;
        if (!std::strcmp(object_name, "_wpe_renderer_backend_egl_target_interface"))
            return &stm_renderer_backend_egl_target_interface;
        if (!std::strcmp(object_name, "_wpe_renderer_backend_egl_offscreen_target_interface"))
            return &stm_renderer_backend_egl_offscreen_target_interface;

        if (!std::strcmp(object_name, "_wpe_view_backend_interface"))
            return &stm_view_backend_interface;
#endif

#ifdef BACKEND_WESTEROS
        if (!std::strcmp(object_name, "_wpe_renderer_backend_egl_interface"))
            return &westeros_renderer_backend_egl_interface;
        if (!std::strcmp(object_name, "_wpe_renderer_backend_egl_target_interface"))
            return &westeros_renderer_backend_egl_target_interface;
        if (!std::strcmp(object_name, "_wpe_renderer_backend_egl_offscreen_target_interface"))
            return &westeros_renderer_backend_egl_offscreen_target_interface;

        if (!std::strcmp(object_name, "_wpe_view_backend_interface"))
            return &westeros_view_backend_interface;
#endif

#ifdef KEY_INPUT_HANDLING_XKB
        if (!std::strcmp(object_name, "_wpe_input_key_mapper_interface"))
            return &libxkbcommon_input_key_mapper_interface;
#endif

#ifdef KEY_INPUT_HANDLING_LINUX_INPUT
        if (!std::strcmp(object_name, "_wpe_input_key_mapper_interface"))
            return &linuxinput_input_key_mapper_interface;
#endif

        return nullptr;
    },
};

}
