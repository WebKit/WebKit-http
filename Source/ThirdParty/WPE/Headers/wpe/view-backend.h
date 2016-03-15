#ifndef wpe_view_backend_h
#define wpe_view_backend_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

struct wpe_view_backend;

struct wpe_input_axis_event;
struct wpe_input_keyboard_event;
struct wpe_input_pointer_event;
struct wpe_input_touch_event;

struct wpe_view_backend_client;
struct wpe_view_backend_input_client;

struct wpe_view_backend_interface {
    void* (*create)(struct wpe_view_backend*);
    void (*destroy)(void*);

    void (*initialize)(void*);
    int (*get_renderer_host_fd)(void*);
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


struct wpe_view_backend_client {
    void (*set_size)(void*, uint32_t, uint32_t);
};

void
wpe_view_backend_dispatch_set_size(struct wpe_view_backend*, uint32_t, uint32_t);


struct wpe_view_backend_input_client {
    void (*handle_keyboard_event)(void*, struct wpe_input_keyboard_event*);
    void (*handle_pointer_event)(void*, struct wpe_input_pointer_event*);
    void (*handle_axis_event)(void*, struct wpe_input_axis_event*);
    void (*handle_touch_event)(void*, struct wpe_input_touch_event*);
};

void
wpe_view_backend_dispatch_keyboard_event(struct wpe_view_backend*, struct wpe_input_keyboard_event*);

void
wpe_view_backend_dispatch_pointer_event(struct wpe_view_backend*, struct wpe_input_pointer_event*);

void
wpe_view_backend_dispatch_axis_event(struct wpe_view_backend*, struct wpe_input_axis_event*);

void
wpe_view_backend_dispatch_touch_event(struct wpe_view_backend*, struct wpe_input_touch_event*);

#ifdef __cplusplus
}
#endif

#endif // wpe_view_backend_h
