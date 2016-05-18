#include <wpe/view-backend.h>

extern "C" {

struct wpe_view_backend_interface bcm_nexus_view_backend_interface = {
    // create
    [](struct wpe_view_backend* backend) -> void*
    {
        return nullptr;
    },
    // destroy
    [](void* data)
    {
    },
    // initialize
    [](void* data)
    {
    },
    // get_renderer_host_fd
    [](void* data) -> int
    {
        return -1;
    },
};

}
