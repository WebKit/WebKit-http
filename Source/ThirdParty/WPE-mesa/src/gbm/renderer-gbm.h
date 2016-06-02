#ifndef wpe_renderer_gbm_h
#define wpe_renderer_gbm_h

#include <wpe/renderer-backend-egl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct wpe_renderer_backend_egl_interface gbm_renderer_backend_egl_interface;
extern struct wpe_renderer_backend_egl_target_interface gbm_renderer_backend_egl_target_interface;
extern struct wpe_renderer_backend_egl_offscreen_target_interface gbm_renderer_backend_egl_offscreen_target_interface;

#ifdef __cplusplus
}
#endif

#endif // wpe_renderer_gbm_h
