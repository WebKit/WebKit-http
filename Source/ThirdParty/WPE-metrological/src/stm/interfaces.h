#ifndef stm_interfaces_h
#define stm_interfaces_h

#include <wpe/renderer-backend-egl.h>
#include <wpe/view-backend.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct wpe_renderer_backend_egl_interface stm_renderer_backend_egl_interface;
extern struct wpe_renderer_backend_egl_target_interface stm_renderer_backend_egl_target_interface;
extern struct wpe_renderer_backend_egl_offscreen_target_interface stm_renderer_backend_egl_offscreen_target_interface;

extern struct wpe_view_backend_interface stm_view_backend_interface;

#ifdef __cplusplus
}
#endif

#endif // stm_interfaces_h
