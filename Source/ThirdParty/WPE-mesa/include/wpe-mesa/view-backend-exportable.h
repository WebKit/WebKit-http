#ifndef wpe_mesa_view_backend_exportable_h
#define wpe_mesa_view_backend_exportable_h

#ifdef __cplusplus
extern "C" {
#endif

#include <wpe/view-backend.h>

struct wpe_mesa_view_backend_exportable;

struct wpe_mesa_view_backend_exportable_client {
    void (*export_dma_buf)(void*, int32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
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
