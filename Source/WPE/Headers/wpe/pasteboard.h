#ifndef wpe_pasteboard_h
#define wpe_pasteboard_h

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct wpe_pasteboard_string {
    char* data;
    uint64_t length;
};

struct wpe_pasteboard_string_vector {
    struct wpe_pasteboard_string* strings;
    uint64_t length;
};

struct wpe_pasteboard_string_pair {
    struct wpe_pasteboard_string type;
    struct wpe_pasteboard_string string;
};

struct wpe_pasteboard_string_map {
    struct wpe_pasteboard_string_pair* pairs;
    uint64_t length;
};

void
wpe_pasteboard_string_initialize(struct wpe_pasteboard_string*, const char*, uint64_t);

void
wpe_pasteboard_string_free(struct wpe_pasteboard_string*);

void
wpe_pasteboard_string_vector_free(struct wpe_pasteboard_string_vector*);


struct wpe_pasteboard;

struct wpe_pasteboard_interface {
    void* (*initialize)(struct wpe_pasteboard*);

    void (*get_types)(void*, struct wpe_pasteboard_string_vector*);
    void (*get_string)(void*, const char*, struct wpe_pasteboard_string*);
    void (*write)(void*, struct wpe_pasteboard_string_map*);
};

struct wpe_pasteboard*
wpe_pasteboard_get_singleton();

void
wpe_pasteboard_get_types(struct wpe_pasteboard*, struct wpe_pasteboard_string_vector*);

void
wpe_pasteboard_get_string(struct wpe_pasteboard*, const char*, struct wpe_pasteboard_string*);

void
wpe_pasteboard_write(struct wpe_pasteboard*, struct wpe_pasteboard_string_map*);

#ifdef __cplusplus
}
#endif

#endif // wpe_pasteboard_h
