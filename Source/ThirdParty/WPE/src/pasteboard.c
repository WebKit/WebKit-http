#include "pasteboard-private.h"

#include "loader-private.h"
#include <stdlib.h>
#include <string.h>

__attribute__((visibility("default")))
void
wpe_pasteboard_string_initialize(struct wpe_pasteboard_string* string, const char* in_string, uint64_t in_length)
{
    if (string->data)
        return;

    string->data = malloc(sizeof(char*) * in_length);
    string->length = in_length;
    memcpy(string->data, in_string, in_length);
}

__attribute__((visibility("default")))
void
wpe_pasteboard_string_free(struct wpe_pasteboard_string* string)
{
    if (string->data)
        free((void*)string->data);
    string->data = 0;
    string->length = 0;
}

__attribute__((visibility("default")))
void
wpe_pasteboard_string_vector_free(struct wpe_pasteboard_string_vector* vector)
{
    if (vector->strings) {
        for (uint64_t i = 0; i < vector->length; ++i)
            wpe_pasteboard_string_free(&vector->strings[i]);
        free(vector->strings);
    }
    vector->strings = 0;
    vector->length = 0;
}

__attribute__((visibility("default")))
struct wpe_pasteboard*
wpe_pasteboard_get_singleton()
{
    static struct wpe_pasteboard* s_pasteboard = 0;
    if (!s_pasteboard) {
        s_pasteboard = malloc(sizeof(struct wpe_pasteboard));
        s_pasteboard->interface = wpe_load_object("_wpe_pasteboard_interface");
        if (!s_pasteboard->interface)
            s_pasteboard->interface = &noop_pasteboard_interface;
        s_pasteboard->interface_data = s_pasteboard->interface->initialize(s_pasteboard);
    }

    return s_pasteboard;
}

__attribute__((visibility("default")))
void
wpe_pasteboard_get_types(struct wpe_pasteboard* pasteboard, struct wpe_pasteboard_string_vector* out_types)
{
    pasteboard->interface->get_types(pasteboard->interface_data, out_types);
}

__attribute__((visibility("default")))
void
wpe_pasteboard_get_string(struct wpe_pasteboard* pasteboard, const char* type, struct wpe_pasteboard_string* out_string)
{
    pasteboard->interface->get_string(pasteboard->interface_data, type, out_string);
}

__attribute__((visibility("default")))
void
wpe_pasteboard_write(struct wpe_pasteboard* pasteboard, struct wpe_pasteboard_string_map* map)
{
    pasteboard->interface->write(pasteboard->interface_data, map);
}
