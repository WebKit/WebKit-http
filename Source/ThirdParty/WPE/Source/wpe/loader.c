#include "loader-private.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

static void* s_impl_library = 0;
static struct wpe_loader_interface* s_impl_loader = 0;

void
load_impl_library()
{
    static const char library_name[] = "libWPE-mesa.so";

    s_impl_library = dlopen(library_name, RTLD_NOW);
    if (!s_impl_library) {
        fprintf(stderr, "wpe: could not load the impl library: %s\n", dlerror());
        abort();
    }

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
