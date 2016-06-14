/*
 * Copyright (C) 2015, 2016 Igalia S.L.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
