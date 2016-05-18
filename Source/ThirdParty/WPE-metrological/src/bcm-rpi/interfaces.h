#ifndef rpi_interfaces_h
#define rpi_interfaces_h

#include <wpe/renderer.h>
#include <wpe/view-backend.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct wpe_renderer_backend_egl_interface rpi_renderer_backend_egl_interface;
extern struct wpe_renderer_backend_egl_target_interface rpi_renderer_backend_egl_target_interface;
extern struct wpe_renderer_backend_egl_offscreen_target_interface rpi_renderer_backend_egl_offscreen_target_interface;

extern struct wpe_view_backend_interface rpi_view_backend_interface;

#ifdef __cplusplus
}
#endif

#endif // rpi_interfaces_h
