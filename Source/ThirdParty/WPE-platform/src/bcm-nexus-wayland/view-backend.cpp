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

#include "display.h"
#include "ipc.h"
#include "ipc-bcmnexuswl.h"
#include "xdg-shell-client-protocol.h"
#include "nsc-client-protocol.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <unistd.h>

namespace BCMNexusWL {

class ViewBackend : public IPC::Host::Handler {
public:
    ViewBackend(struct wpe_view_backend*);
    virtual ~ViewBackend();

    void initialize();

    // IPC::Host::Handler
    void handleFd(int) override { };
    void handleMessage(char*, size_t) override;
    void commitBuffer(uint32_t, uint32_t);

    struct wpe_view_backend* backend() { return m_backend; }
    IPC::Host& ipcHost() { return m_ipcHost; }

    struct CallbackListenerData {
        IPC::Host* ipcHost;
        struct wl_callback* frameCallback;
        struct wpe_view_backend* backend;
    };

    struct NSCData {
        uint32_t clientID;
        std::string authenticationData;
        uint32_t width;
        uint32_t height;
    };

private:
    Wayland::Display& m_display;
    struct wpe_view_backend* m_backend;

    struct wl_surface* m_surface;
    struct xdg_surface* m_xdgSurface;

    CallbackListenerData m_callbackData { nullptr, nullptr, nullptr};
    NSCData m_nscData { 0, std::string{ }, 0, 0 };
    struct wl_buffer* m_buffer;

    IPC::Host m_ipcHost;
};

static const struct xdg_surface_listener g_xdgSurfaceListener = {
    // configure
    [](void*, struct xdg_surface* surface, int32_t, int32_t, struct wl_array*, uint32_t serial)
    {
        xdg_surface_ack_configure(surface, serial);
    },
    // delete
    [](void*, struct xdg_surface*) { },
};

const struct wl_callback_listener g_callbackListener = {
    // frame
    [](void* data, struct wl_callback* callback, uint32_t)
    {
        auto& callbackData = *static_cast<ViewBackend::CallbackListenerData*>(data);

        if (callbackData.ipcHost) {
            IPC::Message message;
            IPC::BCMNexusWL::FrameComplete::construct(message);
            callbackData.ipcHost->sendMessage(IPC::Message::data(message), IPC::Message::size);
        }

        wpe_view_backend_dispatch_frame_displayed(callbackData.backend);

        callbackData.frameCallback = nullptr;
        wl_callback_destroy(callback);
    },
};

static const struct wl_nsc_listener g_nscListener = {
    // handle_standby_status
    [](void*, struct wl_nsc*, struct wl_array*) { },
    // connectID
    [](void*, struct wl_nsc*, uint32_t) { },
    // composition
    [](void*, struct wl_nsc*, struct wl_array*) { },
    // authenticated
    [](void* data, struct wl_nsc*, const char* certData, uint32_t certLength)
    {
        auto& nscData = *static_cast<ViewBackend::NSCData*>(data);
        nscData.authenticationData = { certData, certLength };
    },
    // clientID_created
    [](void* data, struct wl_nsc*, struct wl_array* clientIDArray)
    {
        auto& nscData = *static_cast<ViewBackend::NSCData*>(data);
        nscData.clientID = static_cast<uint32_t*>(clientIDArray->data)[0];
    },
    // display_geometry
    [](void* data, struct wl_nsc*, uint32_t width, uint32_t height)
    {
        auto& nscData = *static_cast<ViewBackend::NSCData*>(data);
        nscData.width = width;
        nscData.height = height;
    },
    // audiosettings
    [](void*, struct wl_nsc*, struct wl_array*) { },
    // displaystatus
    [](void*, struct wl_nsc*, struct wl_array*) { },
    // picturequalitysettings
    [](void*, struct wl_nsc*, struct wl_array*) { },
    // displaysettings
    [](void*, struct wl_nsc*, struct wl_array*) { },
};

ViewBackend::ViewBackend(struct wpe_view_backend* backend)
    : m_display(Wayland::Display::singleton())
    , m_backend(backend)
{
    m_ipcHost.initialize(*this);

    m_surface = wl_compositor_create_surface(m_display.interfaces().compositor);

    if (m_display.interfaces().xdg) {
        m_xdgSurface = xdg_shell_get_xdg_surface(m_display.interfaces().xdg, m_surface);
        xdg_surface_add_listener(m_xdgSurface, &g_xdgSurfaceListener, nullptr);
        xdg_surface_set_title(m_xdgSurface, "WPE");
    }

    wl_nsc_add_listener(m_display.interfaces().nsc, &g_nscListener, &m_nscData);

    m_callbackData.ipcHost = &m_ipcHost;
    m_callbackData.backend = m_backend;
}

ViewBackend::~ViewBackend()
{
    m_backend = nullptr;

    m_ipcHost.deinitialize();

    m_display.unregisterInputClient(m_surface);

    if (m_callbackData.frameCallback)
        wl_callback_destroy(m_callbackData.frameCallback);
    m_callbackData = { nullptr, nullptr, nullptr };

    m_nscData = { 0, std::string{ }, 0, 0 };

    if (m_xdgSurface)
        xdg_surface_destroy(m_xdgSurface);
    m_xdgSurface = nullptr;
    if (m_surface)
        wl_surface_destroy(m_surface);
    m_surface = nullptr;
}

void ViewBackend::initialize()
{
    m_display.registerInputClient(m_surface, m_backend);

    wl_nsc_get_display_geometry(m_display.interfaces().nsc);
    wl_display_roundtrip(m_display.display());

    wpe_view_backend_dispatch_set_size(m_backend, m_nscData.width, m_nscData.height);

    wl_nsc_authenticate(m_display.interfaces().nsc);
    wl_display_roundtrip(m_display.display());

    IPC::Message message;

    size_t transferredData = 0;
    size_t totalDataSize = m_nscData.authenticationData.length();
    const uint8_t* data = reinterpret_cast<const uint8_t*>(m_nscData.authenticationData.data());
    while (transferredData < totalDataSize) {
        size_t dataSize;
        if (totalDataSize - transferredData > IPC::BCMNexusWL::Authentication::maxChunkSize)
            dataSize = IPC::BCMNexusWL::Authentication::maxChunkSize;
        else
            dataSize = totalDataSize - transferredData;

        IPC::BCMNexusWL::Authentication::construct(message, totalDataSize, dataSize, data + transferredData);
        m_ipcHost.sendMessage(IPC::Message::data(message), IPC::Message::size);

        transferredData += dataSize;
    }

    wl_nsc_request_clientID(m_display.interfaces().nsc, WL_NSC_CLIENT_SURFACE);
    wl_display_roundtrip(m_display.display());

    wl_nsc_create_window(m_display.interfaces().nsc, m_nscData.clientID, 0, m_nscData.width, m_nscData.height);
    wl_display_roundtrip(m_display.display());

    IPC::BCMNexusWL::TargetConstruction::construct(message, m_nscData.clientID, m_nscData.width, m_nscData.height);
    m_ipcHost.sendMessage(IPC::Message::data(message), IPC::Message::size);

}

void ViewBackend::handleMessage(char* data, size_t size)
{
    if (size != IPC::Message::size)
        return;

    auto& message = IPC::Message::cast(data);
    switch (message.messageCode) {
    case IPC::BCMNexusWL::BufferCommit::code:
    {
        auto& bufferCommit = IPC::BCMNexusWL::BufferCommit::cast(message);
        commitBuffer(bufferCommit.width, bufferCommit.height);
        break;
    }
    default:
        fprintf(stderr, "ViewBackend: unhandled message\n");
    }
}

void ViewBackend::commitBuffer(uint32_t width, uint32_t height)
{
    if (width != m_nscData.width || height != m_nscData.height)
        return;

    if (!m_buffer) {
        m_buffer = wl_nsc_create_buffer(m_display.interfaces().nsc, m_nscData.clientID, m_nscData.width, m_nscData.height);
        wl_display_roundtrip(m_display.display());
    }

    m_callbackData.frameCallback = wl_surface_frame(m_surface);
    wl_callback_add_listener(m_callbackData.frameCallback, &g_callbackListener, &m_callbackData);

    wl_surface_attach(m_surface, m_buffer, 0, 0);
    wl_surface_damage(m_surface, 0, 0, INT32_MAX, INT32_MAX);
    wl_surface_commit(m_surface);
    wl_display_flush(m_display.display());
}

} // namespace BCMNexusWL

extern "C" {

struct wpe_view_backend_interface bcm_nexus_wayland_view_backend_interface = {
    // create
    [](void*, struct wpe_view_backend* backend) -> void*
    {
        return new BCMNexusWL::ViewBackend(backend);
    },
    // destroy
    [](void* data)
    {
        auto* backend = static_cast<BCMNexusWL::ViewBackend*>(data);
        delete backend;
    },
    // initialize
    [](void* data)
    {
        auto& backend = *static_cast<BCMNexusWL::ViewBackend*>(data);
        backend.initialize();
    },
    // get_renderer_host_fd
    [](void* data) -> int
    {
        auto& backend = *static_cast<BCMNexusWL::ViewBackend*>(data);
        return backend.ipcHost().releaseClientFD();
    },
};

}
