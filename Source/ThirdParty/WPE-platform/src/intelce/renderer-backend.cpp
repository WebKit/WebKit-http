/*
 * Copyright (C) 2015, 2016 Igalia S.L.
 * Copyright (C) 2015, 2016 Metrological
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

#include <wpe/renderer-backend-egl.h>

#include "ipc.h"
#include "ipc-intelce.h"
#include <EGL/egl.h>
#include <libgdl.h>

namespace IntelCE {

struct EGLTarget : public IPC::Client::Handler {
    EGLTarget(struct wpe_renderer_backend_egl_target*, int);
    virtual ~EGLTarget();

    // IPC::Client::Handler
    void handleMessage(char* data, size_t size) override;

    struct wpe_renderer_backend_egl_target* target;
    IPC::Client ipcClient;

    uint32_t width { 0 };
    uint32_t height { 0 };
};

EGLTarget::EGLTarget(struct wpe_renderer_backend_egl_target* target, int hostFd)
    : target(target)
{
    ipcClient.initialize(*this, hostFd);
}

EGLTarget::~EGLTarget()
{
    ipcClient.deinitialize();
}

void EGLTarget::handleMessage(char* data, size_t size)
{
    if (size != IPC::Message::size)
        return;

    auto& message = IPC::Message::cast(data);
    switch (message.messageCode) {
    case IPC::IntelCE::FrameComplete::code:
    {
        wpe_renderer_backend_egl_target_dispatch_frame_complete(target);
        break;
    }
    default:
        fprintf(stderr, "EGLTarget: unhandled message\n");
    };
}

} // namespace IntelCE

extern "C" {

struct wpe_renderer_backend_egl_interface intelce_renderer_backend_egl_interface = {
    // create
    [](int) -> void*
    {
        return nullptr;
    },
    // destroy
    [](void* data)
    {
    },
    // get_native_display
    [](void* data) -> EGLNativeDisplayType
    {
        return EGL_DEFAULT_DISPLAY;
    },
};

struct wpe_renderer_backend_egl_target_interface intelce_renderer_backend_egl_target_interface = {
    // create
    [](struct wpe_renderer_backend_egl_target* target, int host_fd) -> void*
    {
        return new IntelCE::EGLTarget(target, host_fd);
    },
    // destroy
    [](void* data)
    {
        auto* target = static_cast<IntelCE::EGLTarget*>(data);
        delete target;
    },
    // initialize
    [](void* data, void* backend_data, uint32_t width, uint32_t height)
    {
        auto& target = *static_cast<IntelCE::EGLTarget*>(data);
        target.width = width;
        target.height = height;
    },
    // get_native_window
    [](void* data) -> EGLNativeWindowType
    {
        return reinterpret_cast<EGLNativeWindowType>(GDL_PLANE_ID_UPP_D);
    },
    // resize
    [](void* data, uint32_t width, uint32_t height)
    {
    },
    // frame_will_render
    [](void* data)
    {
    },
    // frame_rendered
    [](void* data)
    {
        auto& target = *static_cast<IntelCE::EGLTarget*>(data);

        IPC::Message message;
        IPC::IntelCE::BufferCommit::construct(message, target.width, target.height);
        target.ipcClient.sendMessage(IPC::Message::data(message), IPC::Message::size);
    },
};

struct wpe_renderer_backend_egl_offscreen_target_interface intelce_renderer_backend_egl_offscreen_target_interface = {
    // create
    []() -> void*
    {
        return nullptr;
    },
    // destroy
    [](void* data)
    {
    },
    // initialize
    [](void* data, void* backend_data)
    {
    },
    // get_native_window
    [](void* data) -> EGLNativeWindowType
    {
        return nullptr;
    },
};

}
