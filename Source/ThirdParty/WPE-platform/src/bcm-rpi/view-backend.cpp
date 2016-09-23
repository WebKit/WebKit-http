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
#include "cursor-data.h"
#include "ipc.h"
#include "ipc-rpi.h"
#include <bcm_host.h>
#include <cstdio>
#include <memory>
#include <sys/eventfd.h>
#include <sys/time.h>

namespace BCMRPi {

struct ViewBackend;

class UpdateSource {
public:
    static GSourceFuncs sourceFuncs;

    GSource source;
    GPollFD pfd;
    ViewBackend* backend;
};

struct ViewBackend : public IPC::Host::Handler, public WPE::LibinputServer::Client {
    ViewBackend(struct wpe_view_backend*);
    virtual ~ViewBackend();

    void initialize();
    void initializeRenderingTarget();
    void initializeInput();

    int releaseClientFD();

    // IPC::Host::Handler
    void handleFd(int) override;
    void handleMessage(char*, size_t) override;

    void commitBuffer(uint32_t, uint32_t, uint32_t);
    void handleUpdate();

    // WPE::LibinputServer::Client
    void handleKeyboardEvent(struct wpe_input_keyboard_event*) override;
    void handlePointerEvent(struct wpe_input_pointer_event*) override;
    void handleAxisEvent(struct wpe_input_axis_event*) override;
    void handleTouchEvent(struct wpe_input_touch_event*) override;

    struct wpe_view_backend* backend;
    IPC::Host ipcHost;

    DISPMANX_DISPLAY_HANDLE_T displayHandle { DISPMANX_NO_HANDLE };
    DISPMANX_ELEMENT_HANDLE_T elementHandle { DISPMANX_NO_HANDLE };

    int updateFd { -1 };
    GSource* updateSource;

    uint32_t width { 0 };
    uint32_t height { 0 };

    struct Cursor : public WPE::LibinputServer::Client {
        Cursor(WPE::LibinputServer::Client&, DISPMANX_DISPLAY_HANDLE_T, uint32_t, uint32_t);
        ~Cursor();

        // WPE::LibinputServer::Client
        void handleKeyboardEvent(struct wpe_input_keyboard_event*) override;
        void handlePointerEvent(struct wpe_input_pointer_event*) override;
        void handleAxisEvent(struct wpe_input_axis_event*) override;
        void handleTouchEvent(struct wpe_input_touch_event*) override;

        static const uint32_t cursorWidth;
        static const uint32_t cursorHeight;

        WPE::LibinputServer::Client& targetClient;
        DISPMANX_ELEMENT_HANDLE_T cursorHandle;
        std::pair<uint32_t, uint32_t> position;
        std::pair<uint32_t, uint32_t> displaySize;
    };
    std::unique_ptr<Cursor> cursor;
};

ViewBackend::ViewBackend(struct wpe_view_backend* backend)
    : backend(backend)
{
    ipcHost.initialize(*this);

    bcm_host_init();
    displayHandle = vc_dispmanx_display_open(0);
    graphics_get_display_size(DISPMANX_ID_HDMI, &width, &height);

    updateFd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (updateFd == -1) {
        fprintf(stderr, "ViewBackend: failed to create the update eventfd\n");
        return;
    }

    updateSource = g_source_new(&UpdateSource::sourceFuncs, sizeof(UpdateSource));
    auto& source = *reinterpret_cast<UpdateSource*>(updateSource);
    source.backend = this;

    source.pfd.fd = updateFd;
    source.pfd.events = G_IO_IN | G_IO_ERR | G_IO_HUP;
    source.pfd.revents = 0;
    g_source_add_poll(updateSource, &source.pfd);

    g_source_set_name(updateSource, "[WPE] BCMRPi update");
    g_source_set_priority(updateSource, G_PRIORITY_HIGH + 30);
    g_source_set_can_recurse(updateSource, TRUE);
    g_source_attach(updateSource, g_main_context_get_thread_default());
}

ViewBackend::~ViewBackend()
{
    ipcHost.deinitialize();

    WPE::LibinputServer::singleton().setClient(nullptr);

    if (updateSource)
        g_source_destroy(updateSource);
    if (updateFd != -1)
        close(updateFd);

    vc_dispmanx_display_close(displayHandle);
    bcm_host_deinit();
}

void ViewBackend::initialize()
{
    initializeRenderingTarget();
    initializeInput();
}

void ViewBackend::initializeRenderingTarget()
{
    static VC_DISPMANX_ALPHA_T alpha = {
        static_cast<DISPMANX_FLAGS_ALPHA_T>(DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS),
        255, 0
    };

    if (elementHandle != DISPMANX_NO_HANDLE)
        return;

    DISPMANX_UPDATE_HANDLE_T updateHandle = vc_dispmanx_update_start(0);

    VC_RECT_T srcRect, destRect;
    vc_dispmanx_rect_set(&srcRect, 0, 0, width << 16, height << 16);
    vc_dispmanx_rect_set(&destRect, 0, 0, width, height);

    elementHandle = vc_dispmanx_element_add(updateHandle, displayHandle, 0,
        &destRect, DISPMANX_NO_HANDLE, &srcRect, DISPMANX_PROTECTION_NONE,
        &alpha, nullptr, DISPMANX_NO_ROTATE);

    vc_dispmanx_update_submit_sync(updateHandle);

    wpe_view_backend_dispatch_set_size(backend, width, height);
}

void ViewBackend::initializeInput()
{
    WPE::LibinputServer::Client* inputClient = this;

    if (std::getenv("WPE_BCMRPI_TOUCH"))
        WPE::LibinputServer::singleton().setHandleTouchEvents(true);

    if (std::getenv("WPE_BCMRPI_CURSOR")) {
        cursor.reset(new Cursor(*this, displayHandle, width, height));
        inputClient = cursor.get();
        WPE::LibinputServer::singleton().setHandlePointerEvents(true);
    }
    WPE::LibinputServer::singleton().setPointerBounds(width, height);

    WPE::LibinputServer::singleton().setClient(inputClient);
}

int ViewBackend::releaseClientFD()
{
    IPC::Message message;
    IPC::BCMRPi::TargetConstruction::construct(message, elementHandle, width, height);
    ipcHost.sendMessage(IPC::Message::data(message), IPC::Message::size);

    return ipcHost.releaseClientFD();
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
    case IPC::BCMRPi::BufferCommit::code:
    {
        auto& bufferCommit = IPC::BCMRPi::BufferCommit::cast(message);
        commitBuffer(bufferCommit.handle, bufferCommit.width, bufferCommit.height);
        break;
    }
    default:
        fprintf(stderr, "ViewBackend: unhandled message\n");
    }
}

void ViewBackend::commitBuffer(uint32_t handle, uint32_t width, uint32_t height)
{
    if (handle != elementHandle || width != this->width || height != this->height)
        return;

    DISPMANX_UPDATE_HANDLE_T updateHandle = vc_dispmanx_update_start(0);

    VC_RECT_T srcRect, destRect;
    vc_dispmanx_rect_set(&srcRect, 0, 0, width << 16, height << 16);
    vc_dispmanx_rect_set(&destRect, 0, 0, width, height);

    vc_dispmanx_element_change_attributes(updateHandle, elementHandle, 1 << 3 | 1 << 2, 0, 0, &destRect, &srcRect, 0, DISPMANX_NO_ROTATE);

    vc_dispmanx_update_submit(updateHandle,
        [](DISPMANX_UPDATE_HANDLE_T, void* data)
        {
            auto& backend = *static_cast<ViewBackend*>(data);

            struct timeval tv;
            gettimeofday(&tv, nullptr);
            uint64_t time = tv.tv_sec * 1000 + tv.tv_usec / 1000;

            ssize_t ret = write(backend.updateFd, &time, sizeof(time));
            if (ret != sizeof(time))
                fprintf(stderr, "ViewBackend: failed to write to the update eventfd\n");
        },
        this);
}

void ViewBackend::handleUpdate()
{
    uint64_t time;
    ssize_t ret = read(updateFd, &time, sizeof(time));
    if (ret != sizeof(time))
        return;

    IPC::Message message;
    IPC::BCMRPi::FrameComplete::construct(message);
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

ViewBackend::Cursor::Cursor(WPE::LibinputServer::Client& targetClient, DISPMANX_DISPLAY_HANDLE_T displayHandle, uint32_t displayWidth, uint32_t displayHeight)
    : targetClient(targetClient)
    , position({ 0, 0 })
    , displaySize({ displayWidth, displayHeight })
{
    static VC_DISPMANX_ALPHA_T alpha = {
        static_cast<DISPMANX_FLAGS_ALPHA_T>(DISPMANX_FLAGS_ALPHA_FROM_SOURCE),
        255, 0
    };

    DISPMANX_UPDATE_HANDLE_T updateHandle = vc_dispmanx_update_start(0);

    uint32_t imagePtr;
    VC_RECT_T rect;
    vc_dispmanx_rect_set(&rect, 0, 0, CursorData::width, CursorData::height);
    DISPMANX_RESOURCE_HANDLE_T pointerResource = vc_dispmanx_resource_create(VC_IMAGE_RGBA32, CursorData::width, CursorData::height, &imagePtr);
    vc_dispmanx_resource_write_data(pointerResource, VC_IMAGE_RGBA32, CursorData::width * 4, CursorData::data, &rect);

    VC_RECT_T srcRect, destRect;
    vc_dispmanx_rect_set(&srcRect, 0, 0, CursorData::width << 16, CursorData::height << 16);
    vc_dispmanx_rect_set(&destRect, position.first, position.second, cursorWidth, cursorHeight);

    cursorHandle = vc_dispmanx_element_add(updateHandle, displayHandle, 10,
        &destRect, pointerResource, &srcRect, DISPMANX_PROTECTION_NONE, &alpha,
        nullptr, DISPMANX_NO_ROTATE);

    vc_dispmanx_resource_delete(pointerResource);

    vc_dispmanx_update_submit_sync(updateHandle);
}

ViewBackend::Cursor::~Cursor()
{
    DISPMANX_UPDATE_HANDLE_T updateHandle = vc_dispmanx_update_start(0);
    vc_dispmanx_element_remove(updateHandle, cursorHandle);
    vc_dispmanx_update_submit_sync(updateHandle);
}

void ViewBackend::Cursor::handleKeyboardEvent(struct wpe_input_keyboard_event* event)
{
    targetClient.handleKeyboardEvent(event);
}

void ViewBackend::Cursor::handlePointerEvent(struct wpe_input_pointer_event* event)
{
    targetClient.handlePointerEvent(event);

    DISPMANX_UPDATE_HANDLE_T updateHandle = vc_dispmanx_update_start(0);

    VC_RECT_T destRect;
    vc_dispmanx_rect_set(&destRect, event->x, event->y,
        std::min<uint32_t>(cursorWidth, std::max<uint32_t>(0, displaySize.first - event->x)),
        std::min<uint32_t>(cursorHeight, std::max<uint32_t>(0, displaySize.second - event->y)));

    vc_dispmanx_element_change_attributes(updateHandle, cursorHandle, 1 << 2,
        0, 0, &destRect, nullptr, DISPMANX_NO_HANDLE, DISPMANX_NO_ROTATE);

    vc_dispmanx_update_submit_sync(updateHandle);
}

void ViewBackend::Cursor::handleAxisEvent(struct wpe_input_axis_event* event)
{
    targetClient.handleAxisEvent(event);
}

void ViewBackend::Cursor::handleTouchEvent(struct wpe_input_touch_event* event)
{
    targetClient.handleTouchEvent(event);
}

const uint32_t ViewBackend::Cursor::cursorWidth = 16;
const uint32_t ViewBackend::Cursor::cursorHeight = 16;

GSourceFuncs UpdateSource::sourceFuncs = {
    // prepare
    [](GSource*, gint* timeout) -> gboolean
    {
        *timeout = -1;
        return FALSE;
    },
    // check
    [](GSource* base) -> gboolean
    {
        auto& source = *reinterpret_cast<UpdateSource*>(base);
        return !!source.pfd.revents;
    },
    // dispatch
    [](GSource* base, GSourceFunc, gpointer) -> gboolean
    {
        auto& source = *reinterpret_cast<UpdateSource*>(base);

        if (source.pfd.revents & (G_IO_ERR | G_IO_HUP))
            return FALSE;

        if (source.pfd.revents & G_IO_IN)
            source.backend->handleUpdate();

        source.pfd.revents = 0;
        return TRUE;
    },
    nullptr, // finalize
    nullptr, // closure_callback
    nullptr, // closure_marshall
};

} // namespace BCMRPi

extern "C" {

struct wpe_view_backend_interface bcm_rpi_view_backend_interface = {
    // create
    [](void*, struct wpe_view_backend* backend) -> void*
    {
        return new BCMRPi::ViewBackend(backend);
    },
    // destroy
    [](void* data)
    {
        auto* backend = static_cast<BCMRPi::ViewBackend*>(data);
        delete backend;
    },
    // initialize
    [](void* data)
    {
        auto& backend = *static_cast<BCMRPi::ViewBackend*>(data);
        backend.initialize();
    },
    // get_renderer_host_fd
    [](void* data) -> int
    {
        auto& backend = *static_cast<BCMRPi::ViewBackend*>(data);
        return backend.releaseClientFD();
    },
};

}
