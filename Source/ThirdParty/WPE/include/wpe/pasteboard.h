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
