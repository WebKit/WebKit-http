#include "pasteboard-private.h"

extern "C" {

struct wpe_pasteboard_interface noop_pasteboard_interface = {
    // initialize
    [](struct wpe_pasteboard*) -> void* { return nullptr; },
    // get_types
    [](void* data, struct wpe_pasteboard_string_vector* out_vector) { },
    // get_string
    [](void* data, const char* type, struct wpe_pasteboard_string* out_string) { },
    // write
    [](void* data, struct wpe_pasteboard_string_map* map) { },
};

}
