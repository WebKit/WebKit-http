#include <wpe/input.h>

#include "loader-private.h"
#include <stdlib.h>

struct wpe_input_key_mapper {
    struct wpe_input_key_mapper_interface* interface;
};

__attribute__((visibility("default")))
struct wpe_input_key_mapper*
wpe_input_key_mapper_get_singleton()
{
    static struct wpe_input_key_mapper* s_key_mapper = 0;
    if (!s_key_mapper) {
        s_key_mapper = malloc(sizeof(struct wpe_input_key_mapper));
        s_key_mapper->interface = wpe_load_object("_wpe_input_key_mapper_interface");
    }

    return s_key_mapper;
}

__attribute__((visibility("default")))
const char*
wpe_input_identifier_for_key_event(struct wpe_input_key_mapper* key_mapper, struct wpe_input_keyboard_event* event)
{
    return key_mapper->interface->identifier_for_key_event(event);
}

__attribute__((visibility("default")))
int
wpe_input_windows_key_code_for_key_event(struct wpe_input_key_mapper* key_mapper, struct wpe_input_keyboard_event* event)
{
    return key_mapper->interface->windows_key_code_for_key_event(event);
}

__attribute__((visibility("default")))
const char*
wpe_input_single_character_for_key_event(struct wpe_input_key_mapper* key_mapper, struct wpe_input_keyboard_event* event)
{
    return key_mapper->interface->single_character_for_key_event(event);

}
