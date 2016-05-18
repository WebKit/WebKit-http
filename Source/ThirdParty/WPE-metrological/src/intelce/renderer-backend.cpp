#include <wpe/renderer-backend.h>

extern "C" {

struct wpe_renderer_backend_egl_interface intelce_renderer_backend_egl_interface = {
    // create
    []() -> void*
    {
        return nullptr;
    },
    // destroy
    [](void* data)
    {
    },
    // get_native_display
    [](void* data) -> EGLNativeDisplayType
    {
    },
};

struct wpe_renderer_backend_egl_target_interface intelce_renderer_backend_egl_target_interface = {
    // create
    [](struct wpe_renderer_backend_egl_target* target, int host_fd) -> void*
    {
        return nullptr;
    },
    // destroy
    [](void* data)
    {
    },
    // initialize
    [](void* data, void* backend_data, uint32_t width, uint32_t height)
    {
    },
    // get_native_window
    [](void* data) -> EGLNativeWindowType
    {
    },
    // resize
    [](void* data, uint32_t width, uint32_t height)
    {
    },
    // frame_rendered
    [](void* data)
    {
    },
};

struct wpe_renderer_backend_egl_offscreen_target_interface intelce_renderer_backend_egl_offscreen_target_interface = {
    // create
    []() -> void*
    {
        return nullptr;
    },
    // destroy
    [](void* data)
    {
    },
    // initialize
    [](void* data, void* backend_data)
    {
    },
    // get_native_window
    [](void* data) -> EGLNativeWindowType
    {
        return nullptr;
    },
};

}
