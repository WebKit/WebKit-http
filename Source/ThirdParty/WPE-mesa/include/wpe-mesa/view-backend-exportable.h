#ifndef wpe_mesa_view_backend_exportable_h
#define wpe_mesa_view_backend_exportable_h

#ifdef __cplusplus
extern "C" {
#endif

#include <wpe/view-backend.h>

struct wpe_mesa_view_backend_exportable;

struct wpe_mesa_view_backend_exportable_dma_buf_egl_image_data {
    int32_t fd;
    uint32_t handle;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t format;
};

struct wpe_mesa_view_backend_exportable_client {
    void (*export_dma_buf_egl_image)(void*, struct wpe_mesa_view_backend_exportable_dma_buf_egl_image_data*);
};

struct wpe_mesa_view_backend_exportable*
wpe_mesa_view_backend_exportable_create(struct wpe_mesa_view_backend_exportable_client*, void*);

void
wpe_mesa_view_backend_exportable_destroy(struct wpe_mesa_view_backend_exportable*);

struct wpe_view_backend*
wpe_mesa_view_backend_exportable_get_view_backend(struct wpe_mesa_view_backend_exportable*);

void
wpe_mesa_view_backend_exportable_dispatch_frame_complete(struct wpe_mesa_view_backend_exportable*);

void
wpe_mesa_view_backend_exportable_dispatch_release_buffer(struct wpe_mesa_view_backend_exportable*, uint32_t);

#ifdef __cplusplus
}
#endif

#endif // wpe_mesa_view_backend_exportable_h
