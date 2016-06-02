#ifndef wpe_view_backend_private_h
#define wpe_view_backend_private_h

#include <wpe/view-backend.h>

#ifdef __cplusplus
extern "C" {
#endif

struct wpe_view_backend {
    const struct wpe_view_backend_interface* interface;
    void* interface_data;

    struct wpe_view_backend_client* backend_client;
    void* backend_client_data;

    struct wpe_view_backend_input_client* input_client;
    void* input_client_data;
};

#ifdef __cplusplus
}
#endif

#endif // wpe_view_backend_private_h
