#include <wpe/loader.h>

#include <cstdio>
#include <cstring>

extern "C" {

__attribute__((visibility("default")))
struct wpe_loader_interface _wpe_loader_interface = {
    [](const char* object_name) -> void* {
        return nullptr;
    },
};

}
