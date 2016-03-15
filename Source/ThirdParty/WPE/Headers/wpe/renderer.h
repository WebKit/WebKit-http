#ifndef wpe_renderer_h
#define wpe_renderer_h

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __GBM__
#define __GBM__
#endif

#include <EGL/eglplatform.h>

struct wpe_renderer_backend_egl;
struct wpe_renderer_backend_egl_target;
struct wpe_renderer_backend_egl_offscreen_target;

struct wpe_renderer_backend_egl_interface {
    void* (*create)();
    void (*destroy)(void*);

    EGLNativeDisplayType (*get_native_display)(void*);
};

struct wpe_renderer_backend_egl_target_interface {
    void* (*create)(struct wpe_renderer_backend_egl_target*, int);
    void (*destroy)(void*);

    void (*initialize)(void*, void*, uint32_t, uint32_t);
    EGLNativeWindowType (*get_native_window)(void*);
    void (*resize)(void*, uint32_t, uint32_t);
    void (*frame_rendered)(void*);
};

struct wpe_renderer_backend_egl_target_client {
    void (*frame_complete)(void*);
};

struct wpe_renderer_backend_egl_offscreen_target_interface {
    void* (*create)();
    void (*destroy)(void*);

    void (*initialize)(void*, void*);
    EGLNativeWindowType (*get_native_window)(void*);
};


struct wpe_renderer_backend_egl*
wpe_renderer_backend_egl_create();

void
wpe_renderer_backend_egl_destroy(struct wpe_renderer_backend_egl*);

EGLNativeDisplayType
wpe_renderer_backend_egl_get_native_display(struct wpe_renderer_backend_egl*);


struct wpe_renderer_backend_egl_target*
wpe_renderer_backend_egl_target_create(int);

void
wpe_renderer_backend_egl_target_destroy(struct wpe_renderer_backend_egl_target*);

void
wpe_renderer_backend_egl_target_set_client(struct wpe_renderer_backend_egl_target*, struct wpe_renderer_backend_egl_target_client*, void*);

void
wpe_renderer_backend_egl_target_initialize(struct wpe_renderer_backend_egl_target*, struct wpe_renderer_backend_egl*, uint32_t, uint32_t);

EGLNativeWindowType
wpe_renderer_backend_egl_target_get_native_window(struct wpe_renderer_backend_egl_target*);

void
wpe_renderer_backend_egl_target_resize(struct wpe_renderer_backend_egl_target*, uint32_t, uint32_t);

void
wpe_renderer_backend_egl_target_frame_rendered(struct wpe_renderer_backend_egl_target*);

struct wpe_renderer_backend_egl_offscreen_target*
wpe_renderer_backend_egl_offscreen_target_create();

void
wpe_renderer_backend_egl_offscreen_target_destroy(struct wpe_renderer_backend_egl_offscreen_target*);

void
wpe_renderer_backend_egl_offscreen_target_initialize(struct wpe_renderer_backend_egl_offscreen_target*, struct wpe_renderer_backend_egl*);

EGLNativeWindowType
wpe_renderer_backend_egl_offscreen_target_get_native_window(struct wpe_renderer_backend_egl_offscreen_target*);

#ifdef __cplusplus
}
#endif

#endif // wpe_renderer_h
