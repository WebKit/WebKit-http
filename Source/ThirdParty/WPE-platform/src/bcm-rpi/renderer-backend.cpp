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
#include "ipc-rpi.h"
#include <EGL/egl.h>

#include <cstdio>

namespace BCMRPi {

struct EGLTarget : public IPC::Client::Handler {
    EGLTarget(struct wpe_renderer_backend_egl_target*, int);
    virtual ~EGLTarget();

    // IPC::Client::Handler
    void handleMessage(char* data, size_t size) override;

    void constructTarget(uint32_t, uint32_t, uint32_t);

    struct wpe_renderer_backend_egl_target* target;
    IPC::Client ipcClient;

    EGL_DISPMANX_WINDOW_T nativeWindow { 0, };
};

EGLTarget::EGLTarget(struct wpe_renderer_backend_egl_target* target, int hostFd)
    : target(target)
{
    ipcClient.initialize(*this, hostFd);

    // Wait for the TargetConstruction message from wpe_view_backend.
    while (!nativeWindow.element)
        ipcClient.readSynchronously();
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
    case IPC::BCMRPi::TargetConstruction::code:
    {
        auto& targetConstruction = IPC::BCMRPi::TargetConstruction::cast(message);
        constructTarget(targetConstruction.handle, targetConstruction.width, targetConstruction.height);
        break;
    }
    case IPC::BCMRPi::FrameComplete::code:
    {
        wpe_renderer_backend_egl_target_dispatch_frame_complete(target);
        break;
    }
    default:
        fprintf(stderr, "EGLTarget: unhandled message\n");
    };
}

void EGLTarget::constructTarget(uint32_t handle, uint32_t width, uint32_t height)
{
    if (nativeWindow.element)
        return;

    nativeWindow.element = handle;
    nativeWindow.width = width;
    nativeWindow.height = height;
}

} // namespace BCMRPi

extern "C" {

struct wpe_renderer_backend_egl_interface bcm_rpi_renderer_backend_egl_interface = {
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

struct wpe_renderer_backend_egl_target_interface bcm_rpi_renderer_backend_egl_target_interface = {
    // create
    [](struct wpe_renderer_backend_egl_target* target, int host_fd) -> void*
    {
        return new BCMRPi::EGLTarget(target, host_fd);
    },
    // destroy
    [](void* data)
    {
        auto* target = static_cast<BCMRPi::EGLTarget*>(data);
        delete target;
    },
    // initialize
    [](void* data, void* backend_data, uint32_t width, uint32_t height)
    {
    },
    // get_native_window
    [](void* data) -> EGLNativeWindowType
    {
        auto& target = *static_cast<BCMRPi::EGLTarget*>(data);
        return &target.nativeWindow;
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
        auto& target = *static_cast<BCMRPi::EGLTarget*>(data);

        IPC::Message message;
        IPC::BCMRPi::BufferCommit::construct(message, target.nativeWindow.element,
            target.nativeWindow.width, target.nativeWindow.height);
        target.ipcClient.sendMessage(IPC::Message::data(message), IPC::Message::size);
    },
};

struct wpe_renderer_backend_egl_offscreen_target_interface bcm_rpi_renderer_backend_egl_offscreen_target_interface = {
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
