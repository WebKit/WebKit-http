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

#include "pasteboard-private.h"

#include <cstdlib>
#include <cstring>
#include <map>
#include <string>

namespace Generic {
using Pasteboard = std::map<std::string, std::string>;
}

extern "C" {

struct wpe_pasteboard_interface generic_pasteboard_interface = {
    // initialize
    [](struct wpe_pasteboard*) -> void*
    {
        return new Generic::Pasteboard;
    },
    // get_types
    [](void* data, struct wpe_pasteboard_string_vector* out_vector)
    {
        auto& pasteboard = *static_cast<Generic::Pasteboard*>(data);
        uint64_t length = pasteboard.size();
        if (!length)
            return;

        out_vector->strings = static_cast<struct wpe_pasteboard_string*>(malloc(sizeof(struct wpe_pasteboard_string) * length));
        out_vector->length = length;
        memset(out_vector->strings, 0, out_vector->length);

        uint64_t i = 0;
        for (auto& entry : pasteboard)
            wpe_pasteboard_string_initialize(&out_vector->strings[i++], entry.first.c_str(), entry.first.length());
    },
    // get_string
    [](void* data, const char* type, struct wpe_pasteboard_string* out_string)
    {
        auto& pasteboard = *static_cast<Generic::Pasteboard*>(data);

        std::string typeString(type);
        auto it = pasteboard.find(typeString);
        if (it == pasteboard.end())
            return;

        wpe_pasteboard_string_initialize(out_string, it->second.c_str(), it->second.length());
    },
    // write
    [](void* data, struct wpe_pasteboard_string_map* map)
    {
        auto& pasteboard = *static_cast<Generic::Pasteboard*>(data);

        pasteboard.clear();
        for (uint64_t i = 0; i < map->length; ++i) {
            auto& pair = map->pairs[i];
            pasteboard.insert({ std::string(pair.type.data, pair.type.length), std::string(pair.string.data, pair.string.length) });
        }
    },
};

}
