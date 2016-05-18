#ifndef westeros_interfaces_h
#define westeros_interfaces_h

#include <wpe/renderer-backend.h>
#include <wpe/view-backend.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct wpe_renderer_backend_egl_interface westeros_renderer_backend_egl_interface;
extern struct wpe_renderer_backend_egl_target_interface westeros_renderer_backend_egl_target_interface;
extern struct wpe_renderer_backend_egl_offscreen_target_interface westeros_renderer_backend_egl_offscreen_target_interface;

extern struct wpe_view_backend_interface westeros_view_backend_interface;

#ifdef __cplusplus
}
#endif

#endif // westeros_interfaces_h
