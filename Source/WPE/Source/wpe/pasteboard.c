#include "pasteboard-private.h"

#include <stdlib.h>

void
wpe_pasteboard_string_vector_free(struct wpe_pasteboard_string_vector* vector)
{
    if (vector->strings)
        free(vector->strings);
    vector->strings = 0;
    vector->length = 0;
}

struct wpe_pasteboard*
wpe_pasteboard_get_singleton()
{
    static struct wpe_pasteboard* s_pasteboard = 0;
    if (!s_pasteboard) {
        s_pasteboard = malloc(sizeof(struct wpe_pasteboard));
        s_pasteboard->interface = &generic_pasteboard_interface;
        s_pasteboard->interface_data = s_pasteboard->interface->initialize(s_pasteboard);
    }

    return s_pasteboard;
}

void
wpe_pasteboard_get_types(struct wpe_pasteboard* pasteboard, struct wpe_pasteboard_string_vector* out_types)
{
    pasteboard->interface->get_types(pasteboard->interface_data, out_types);
}

void
wpe_pasteboard_get_string(struct wpe_pasteboard* pasteboard, const char* type, struct wpe_pasteboard_string* out_string)
{
    pasteboard->interface->get_string(pasteboard->interface_data, type, out_string);
}

void
wpe_pasteboard_write(struct wpe_pasteboard* pasteboard, struct wpe_pasteboard_string_map* map)
{
    pasteboard->interface->write(pasteboard->interface_data, map);
}
