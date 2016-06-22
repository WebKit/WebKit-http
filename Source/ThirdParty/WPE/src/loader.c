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

#include "loader-private.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

static void* s_impl_library = 0;
static struct wpe_loader_interface* s_impl_loader = 0;

void
load_impl_library()
{
#ifdef WPE_BACKEND
    s_impl_library = dlopen(WPE_BACKEND, RTLD_NOW);
    if (!s_impl_library) {
        fprintf(stderr, "wpe: could not load compile-time defined WPE_BACKEND: %s\n", dlerror());
        abort();
    }
#else
    // FIXME:
    // 1. should use a more generic name, usable via a symbolic link to the
    //    platform-specific libraries,
    static const char library_name[] = "libWPE-mesa.so";

    // Get the impl library from an environment variable.
    char* env_library_name = getenv("WPE_BACKEND_LIBRARY");
    if (env_library_name) {
        s_impl_library = dlopen(env_library_name, RTLD_NOW);
        if (!s_impl_library) {
            fprintf(stderr, "wpe: could not load specified WPE_BACKEND_LIBRARY: %s\n", dlerror());
            abort();
        }
    } else {
        // Load libWPE-mesa.so by defauly.
        s_impl_library = dlopen(library_name, RTLD_NOW);
        if (!s_impl_library) {
            fprintf(stderr, "wpe: could not load the impl library: %s\n", dlerror());
            abort();
        }
    }
#endif

    s_impl_loader = dlsym(s_impl_library, "_wpe_loader_interface");
}

void*
wpe_load_object(const char* object_name)
{
    if (!s_impl_library)
        load_impl_library();

    if (s_impl_loader)
        return s_impl_loader->load_object(object_name);

    void* object = dlsym(s_impl_library, object_name);
    if (!object)
        fprintf(stderr, "wpe_load_object: failed to load object with name '%s'\n", object_name);

    return object;
}
