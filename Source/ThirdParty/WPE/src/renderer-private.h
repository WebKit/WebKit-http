#ifndef wpe_renderer_private_h
#define wpe_renderer_private_h

#ifdef __cplusplus
extern "C" {
#endif

struct wpe_renderer_backend_egl {
    const struct wpe_renderer_backend_egl_interface* interface;
    void* interface_data;
};

struct wpe_renderer_backend_egl_target {
    const struct wpe_renderer_backend_egl_target_interface* interface;
    void* interface_data;

    struct wpe_renderer_backend_egl_target_client* client;
    void* client_data;
};

struct wpe_renderer_backend_egl_offscreen_target {
    const struct wpe_renderer_backend_egl_offscreen_target_interface* interface;
    void* interface_data;
};

#ifdef __cplusplus
}
#endif

#endif // wpe_renderer_private_h
