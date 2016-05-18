#ifndef intelce_interfaces_h
#define intelce_interfaces_h

#include <wpe/renderer-backend.h>
#include <wpe/view-backend.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct wpe_renderer_backend_egl_interface intelce_renderer_backend_egl_interface;
extern struct wpe_renderer_backend_egl_target_interface intelce_renderer_backend_egl_target_interface;
extern struct wpe_renderer_backend_egl_offscreen_target_interface intelce_renderer_backend_egl_offscreen_target_interface;

extern struct wpe_view_backend_interface intelce_view_backend_interface;

#ifdef __cplusplus
}
#endif

#endif // intelce_interfaces_h
