#ifndef bcm_nexus_wayland_interfaces_h
#define bcm_nexus_wayland_interfaces_h

#include <wpe/renderer-backend-egl.h>
#include <wpe/view-backend.h>

#ifdef __cplusplus
extern "C" {
#endif


extern struct wpe_renderer_backend_egl_interface bcm_nexus_wayland_renderer_backend_egl_interface;
extern struct wpe_renderer_backend_egl_target_interface bcm_nexus_wayland_renderer_backend_egl_target_interface;
extern struct wpe_renderer_backend_egl_offscreen_target_interface bcm_nexus_wayland_renderer_backend_egl_offscreen_target_interface;

extern struct wpe_view_backend_interface bcm_nexus_wayland_view_backend_interface;

#ifdef __cplusplus
}
#endif

#endif // bcm_nexus_wayland_interfaces_h
