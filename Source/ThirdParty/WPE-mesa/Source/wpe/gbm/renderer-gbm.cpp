#include "renderer-gbm.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <gbm.h>
#include <unistd.h>
#include <unordered_map>

#include <gio/gio.h>
#include <gio/gunixfdmessage.h>

#include "ipc-gbm.h"

#include <memory>

namespace GBM {

struct Backend {
    Backend()
    {
        fd = open("/dev/dri/renderD128", O_RDWR | O_CLOEXEC | O_NOCTTY | O_NONBLOCK);
        if (fd < 0)
            return;

        device = gbm_create_device(fd);
        if (!device) {
            close(fd);
            return;
        }
    }

    ~Backend()
    {
        if (device)
            gbm_device_destroy(device);

        if (fd >= 0)
            close(fd);
    }

    int fd { -1 };
    struct gbm_device* device;
};

struct EGLTarget {
    EGLTarget(struct wpe_renderer_backend_egl_target* target, int hostFd)
        : target(target)
    {
        if (hostFd < 0)
            return;

        socket = g_socket_new_from_fd(hostFd, 0);
        if (!socket)
            return;

        source = g_socket_create_source(socket, G_IO_IN, 0);
        g_source_set_callback(source, reinterpret_cast<GSourceFunc>(socketCallback), this, 0);
        g_source_attach(source, g_main_context_get_thread_default());
    }

    ~EGLTarget()
    {
        if (surface)
            gbm_surface_destroy(surface);

        if (source)
            g_source_destroy(source);
        if (socket)
            g_object_unref(socket);
    }

    static gboolean socketCallback(GSocket* socket, GIOCondition condition, gpointer data)
    {
        auto* target = reinterpret_cast<EGLTarget*>(data);

        if (condition & G_IO_IN) {
            gchar buff[MESSAGE_SIZE];
            gssize len = g_socket_receive(socket, buff, sizeof(buff), 0, 0);

            if (len == MESSAGE_SIZE) {
                auto* message = reinterpret_cast<struct ipc_gbm_message*>(&buff);
                uint64_t messageCode = message->message_code;

                switch (messageCode) {
                case 23:
                {
                    wpe_renderer_backend_egl_target_dispatch_frame_complete(target->target);
                    break;
                }
                case 16:
                {
                    auto* rbMessage = reinterpret_cast<struct release_buffer*>(std::addressof(message->data));
                    target->releaseBuffer(rbMessage->handle);
                    break;
                }
                default:
                    fprintf(stderr, "renderer-gbm: invalid message\n");
                    return FALSE;
                }
            }
        }

        return TRUE;
    }

    void releaseBuffer(uint32_t handle)
    {
        auto it = lockedBuffers.find(handle);
        assert(it != lockedBuffers.end());

        struct gbm_bo* bo = it->second;
        if (bo)
            gbm_surface_release_buffer(surface, bo);
    }

    struct wpe_renderer_backend_egl_target* target;
    GSocket* socket;
    GSource* source;

    struct gbm_surface* surface;
    uint32_t width { 0 };
    uint32_t height { 0 };
    std::unordered_map<uint32_t, struct gbm_bo*> lockedBuffers;
};

struct EGLOffscreenTarget {
    ~EGLOffscreenTarget()
    {
        if (surface)
            gbm_surface_destroy(surface);
    }

    struct gbm_surface* surface;
};

static void destroyBOData(struct gbm_bo*, void* data)
{
    if (data) {
        auto* boData = static_cast<struct buffer_commit*>(data);
        delete boData;
    }
}

} // namespace GBM

extern "C" {

struct wpe_renderer_backend_egl_interface gbm_renderer_backend_egl_interface = {
    // create
    []() -> void*
    {
        return new GBM::Backend;
    },
    // destroy
    [](void* data)
    {
        auto* backend = static_cast<GBM::Backend*>(data);
        delete backend;
    },
    // get_native_display
    [](void* data) -> EGLNativeDisplayType
    {
        auto* backend = static_cast<GBM::Backend*>(data);
        return backend->device;
    },
};

struct wpe_renderer_backend_egl_target_interface gbm_renderer_backend_egl_target_interface = {
    // create
    [](struct wpe_renderer_backend_egl_target* target, int hostFd) -> void*
    {
        return new GBM::EGLTarget(target, hostFd);
    },
    // destroy
    [](void* data)
    {
        auto* target = static_cast<GBM::EGLTarget*>(data);
        delete target;
    },
    // initialize
    [](void* data, void* backend_data, uint32_t width, uint32_t height)
    {
        auto* target = static_cast<GBM::EGLTarget*>(data);
        auto* backend = static_cast<GBM::Backend*>(backend_data);

        target->surface = gbm_surface_create(backend->device, width, height, GBM_FORMAT_ARGB8888, 0);
        target->width = width;
        target->height = height;
    },
    // get_native_window
    [](void* data) -> EGLNativeWindowType
    {
        auto* target = static_cast<GBM::EGLTarget*>(data);
        return target->surface;
    },
    // resize
    [](void* data, uint32_t width, uint32_t height)
    {
        auto* target = static_cast<GBM::EGLTarget*>(data);
        target->width = width;
        target->height = height;
    },
    // frame_rendered
    [](void* data)
    {
        auto* target = static_cast<GBM::EGLTarget*>(data);

        struct gbm_bo* bo = gbm_surface_lock_front_buffer(target->surface);
        assert(bo);

        uint32_t handle = gbm_bo_get_handle(bo).u32;
#ifndef NDEBUG
        auto result =
#endif
            target->lockedBuffers.insert({ handle, bo });
        assert(result.second);

        struct ipc_gbm_message messageData = { };
        messageData.message_code = 42;

        auto* commitData = reinterpret_cast<struct buffer_commit*>(std::addressof(messageData.data));

        auto* boData = static_cast<struct buffer_commit*>(gbm_bo_get_user_data(bo));
        if (!boData) {
            GSocketControlMessage* fdMessage = g_unix_fd_message_new();
            if (!g_unix_fd_message_append_fd(G_UNIX_FD_MESSAGE(fdMessage), gbm_bo_get_fd(bo), 0)) {
                fprintf(stderr, "gbm_renderer: failed to attach gbm_bo fd to the IPC message\n");
                g_object_unref(fdMessage);
                return;
            }

            if (g_socket_send_message(target->socket, 0, 0, 0, &fdMessage, 1, 0, 0, 0) == -1) {
                fprintf(stderr, "gbm_renderer: failed to send gbm_bo fd via the IPC message\n");
                g_object_unref(fdMessage);
                return;
            }

            g_object_unref(fdMessage);

            boData = new struct buffer_commit;
            *boData = { handle, target->width, target->height, gbm_bo_get_stride(bo), gbm_bo_get_format(bo), 0 };
            gbm_bo_set_user_data(bo, boData, &GBM::destroyBOData);
        }

        memcpy(commitData, boData, sizeof(struct buffer_commit));
        g_socket_send(target->socket, reinterpret_cast<char*>(&messageData), sizeof(messageData), 0, 0);
    },
};

struct wpe_renderer_backend_egl_offscreen_target_interface gbm_renderer_backend_egl_offscreen_target_interface = {
    // create
    []() -> void*
    {
        return new GBM::EGLOffscreenTarget;
    },
    // destroy
    [](void* data)
    {
        auto* target = static_cast<GBM::EGLOffscreenTarget*>(data);
        delete target;
    },
    // initialize
    [](void* data, void* backend_data)
    {
        auto* target = static_cast<GBM::EGLOffscreenTarget*>(data);
        auto* backend = static_cast<GBM::Backend*>(backend_data);

        target->surface = gbm_surface_create(backend->device, 1, 1, GBM_FORMAT_ARGB8888, 0);
    },
    // get_native_window
    [](void* data) -> EGLNativeWindowType
    {
        auto* target = static_cast<GBM::EGLOffscreenTarget*>(data);
        return target->surface;
    },
};

}
