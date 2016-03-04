#ifndef wpe_view_backend_h
#define wpe_view_backend_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

struct wpe_view_backend;

struct wpe_view_backend_interface {
    void* (*create)(struct wpe_view_backend*);
    void (*destroy)(void*);

    void (*initialize)(void*);
    int (*get_renderer_host_fd)(void*);
};

struct wpe_view_backend_client {
    void (*set_size)(void*, uint32_t, uint32_t);
};

struct wpe_view_backend_input_client {
    void (*dummy)();
};

struct wpe_view_backend*
wpe_view_backend_create();

void
wpe_view_backend_destroy(struct wpe_view_backend*);

void 
wpe_view_backend_set_backend_client(struct wpe_view_backend*, struct wpe_view_backend_client*, void*);

void
wpe_view_backend_set_input_client(struct wpe_view_backend*, struct wpe_view_backend_input_client*, void*);

void
wpe_view_backend_initialize(struct wpe_view_backend*);

int
wpe_view_backend_get_renderer_host_fd(struct wpe_view_backend*);

#ifdef __cplusplus
}
#endif

#endif // wpe_view_backend_h
