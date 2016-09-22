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

#include <wpe/view-backend.h>

#include "Libinput/LibinputServer.h"
#include "ipc.h"
#include "ipc-intelce.h"
#include <cstdio>
#include <libgdl.h>

namespace IntelCE {

// Plane size and position
#define ORIGIN_X 0
#define ORIGIN_Y 0
#define WIDTH 1280
#define HEIGHT 720
#define ASPECT ((GLfloat)WIDTH / (GLfloat)HEIGHT)

// Initializes a plane for the graphics to be rendered to
static gdl_ret_t setup_plane(gdl_plane_id_t plane) {
    //int alphaGlobal = 255; // [0..255], 0 for transparent, 255 for opaque.
    //gdl_boolean_t usePremultAlpha = GDL_TRUE;
    gdl_boolean_t useScaler = GDL_FALSE;
    gdl_pixel_format_t pixelFormat = GDL_PF_ARGB_32;
    gdl_color_space_t colorSpace = GDL_COLOR_SPACE_RGB;
    gdl_rectangle_t srcRect;
    gdl_rectangle_t dstRect;
    gdl_ret_t rc = GDL_SUCCESS;

    dstRect.origin.x = ORIGIN_X;
    dstRect.origin.y = ORIGIN_Y;
    dstRect.width = WIDTH;
    dstRect.height = HEIGHT;

    gdl_display_info_t displayInfo;
    rc = gdl_get_display_info(GDL_DISPLAY_ID_0, &displayInfo);
    if (GDL_SUCCESS == rc) {
        dstRect.width = displayInfo.tvmode.width;
        dstRect.height = displayInfo.tvmode.height;
        if (dstRect.height != HEIGHT)
            useScaler = GDL_TRUE;
    }

    srcRect.origin.x = 0;
    srcRect.origin.y = 0;
    srcRect.width = WIDTH;
    srcRect.height = HEIGHT;

    rc = gdl_plane_reset(plane);
    if (GDL_SUCCESS == rc)
        rc = gdl_plane_config_begin(plane);

    //if (GDL_SUCCESS == rc)
    //    rc = gdl_plane_set_attr(GDL_PLANE_ALPHA_GLOBAL, &alphaGlobal);

    //if (GDL_SUCCESS == rc)
    //    rc = gdl_plane_set_attr(GDL_PLANE_ALPHA_PREMULT, &usePremultAlpha);

    if (GDL_SUCCESS == rc)
        rc = gdl_plane_set_attr(GDL_PLANE_SRC_COLOR_SPACE, &colorSpace);

    if (GDL_SUCCESS == rc)
        rc = gdl_plane_set_attr(GDL_PLANE_SCALE, &useScaler);

    if (GDL_SUCCESS == rc)
        rc = gdl_plane_set_attr(GDL_PLANE_PIXEL_FORMAT, &pixelFormat);

    if (GDL_SUCCESS == rc)
        rc = gdl_plane_set_attr(GDL_PLANE_DST_RECT, &dstRect);

    if (GDL_SUCCESS == rc)
        rc = gdl_plane_set_attr(GDL_PLANE_SRC_RECT, &srcRect);

    if (GDL_SUCCESS == rc)
        rc = gdl_plane_config_end(GDL_FALSE);
    else
        gdl_plane_config_end(GDL_TRUE);

    if (GDL_SUCCESS != rc)
        fprintf(stderr, "GDL configuration failed! GDL error code is 0x%x\n",
                rc);

    return rc;
}

struct ViewBackend : public IPC::Host::Handler, public WPE::LibinputServer::Client {
    ViewBackend(struct wpe_view_backend*);
    virtual ~ViewBackend();

    void initialize();

    // IPC::Host::Handler
    void handleFd(int) override;
    void handleMessage(char*, size_t) override;

    void commitBuffer(uint32_t, uint32_t);

    // WPE::LibinputServer::Client
    void handleKeyboardEvent(struct wpe_input_keyboard_event*) override;
    void handlePointerEvent(struct wpe_input_pointer_event*) override;
    void handleAxisEvent(struct wpe_input_axis_event*) override;
    void handleTouchEvent(struct wpe_input_touch_event*) override;

    struct wpe_view_backend* backend;
    IPC::Host ipcHost;

    uint32_t width { WIDTH };
    uint32_t height { HEIGHT };
};

ViewBackend::ViewBackend(struct wpe_view_backend* backend)
    : backend(backend)
{
    ipcHost.initialize(*this);
}

ViewBackend::~ViewBackend()
{
    ipcHost.deinitialize();

    WPE::LibinputServer::singleton().setClient(nullptr);
}

void ViewBackend::initialize()
{
    gdl_plane_id_t plane = GDL_PLANE_ID_UPP_D;
    gdl_init(0);
    setup_plane(plane);

    wpe_view_backend_dispatch_set_size(backend, width, height);

    WPE::LibinputServer::singleton().setClient(this);
}

void ViewBackend::handleFd(int)
{
}

void ViewBackend::handleMessage(char* data, size_t size)
{
    if (size != IPC::Message::size)
        return;

    auto& message = IPC::Message::cast(data);
    switch (message.messageCode) {
    case IPC::IntelCE::BufferCommit::code:
    {
        auto& bufferCommit = IPC::IntelCE::BufferCommit::cast(message);
        commitBuffer(bufferCommit.width, bufferCommit.height);
        break;
    }
    default:
        fprintf(stderr, "ViewBackend: unhandled message\n");
    };
}

void ViewBackend::commitBuffer(uint32_t width, uint32_t height)
{
    if (width != this->width || height != this->height)
        return;

    IPC::Message message;
    IPC::IntelCE::FrameComplete::construct(message);
    ipcHost.sendMessage(IPC::Message::data(message), IPC::Message::size);

    wpe_view_backend_dispatch_frame_displayed(backend);
}

void ViewBackend::handleKeyboardEvent(struct wpe_input_keyboard_event* event)
{
    wpe_view_backend_dispatch_keyboard_event(backend, event);
}

void ViewBackend::handlePointerEvent(struct wpe_input_pointer_event* event)
{
    wpe_view_backend_dispatch_pointer_event(backend, event);
}

void ViewBackend::handleAxisEvent(struct wpe_input_axis_event* event)
{
    wpe_view_backend_dispatch_axis_event(backend, event);
}

void ViewBackend::handleTouchEvent(struct wpe_input_touch_event* event)
{
    wpe_view_backend_dispatch_touch_event(backend, event);
}

} // namespace IntelCE

extern "C" {

struct wpe_view_backend_interface intelce_view_backend_interface = {
    // create
    [](void*, struct wpe_view_backend* backend) -> void*
    {
        return new IntelCE::ViewBackend(backend);
    },
    // destroy
    [](void* data)
    {
        auto* backend = static_cast<IntelCE::ViewBackend*>(data);
        delete backend;
    },
    // initialize
    [](void* data)
    {
        auto& backend = *static_cast<IntelCE::ViewBackend*>(data);
        backend.initialize();
    },
    // get_renderer_host_fd
    [](void* data) -> int
    {
        auto& backend = *static_cast<IntelCE::ViewBackend*>(data);
        return backend.ipcHost.releaseClientFD();
    },
};

}
