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
#include "ipc-bcmnexuswl.h"
#include <EGL/egl.h>
#include <cstring>
#include <refsw/nexus_config.h>
#include <refsw/nexus_platform.h>
#include <refsw/nexus_display.h>
#include <refsw/nexus_core_utils.h>
#include <refsw/default_nexus.h>
#include <refsw/nxclient.h>
#include <condition_variable>
#include <mutex>

namespace BCMNexusWL {

struct Backend {
    static Backend* s_singleton;

    Backend();
    ~Backend();

    bool authenticated() const { return m_authenticated; }
    void authenticate(uint32_t, uint32_t, uint8_t*);
    void waitForAuthenticationIfNeeded();

    NXPL_PlatformHandle m_nxplHandle { nullptr };
    NxClient_AllocResults m_allocResults;
    std::string m_authenticationData { };

    std::mutex m_authenticationMutex;
    std::condition_variable m_authenticationCondition;
    bool m_authenticated { false };
};

Backend* Backend::s_singleton { nullptr };

Backend::Backend()
{
    NEXUS_DisplayHandle displayHandle(nullptr);
    NxClient_AllocSettings allocSettings;
    NxClient_JoinSettings joinSettings;
    NxClient_GetDefaultJoinSettings(&joinSettings);

    strcpy(joinSettings.name, "wpe");

    NEXUS_Error rc = NxClient_Join(&joinSettings);
    BDBG_ASSERT(!rc);

    NxClient_GetDefaultAllocSettings(&allocSettings);
    allocSettings.surfaceClient = 1;
    rc = NxClient_Alloc(&allocSettings, &m_allocResults);
    BDBG_ASSERT(!rc);

    NXPL_RegisterNexusDisplayPlatform(&m_nxplHandle, displayHandle);

    s_singleton = this;
}

Backend::~Backend()
{
    NXPL_UnregisterNexusDisplayPlatform(m_nxplHandle);
    s_singleton = nullptr;
}

void Backend::waitForAuthenticationIfNeeded()
{
    std::unique_lock<std::mutex> locker(m_authenticationMutex);
    while (!m_authenticated)
        m_authenticationCondition.wait(locker);
}

void Backend::authenticate(uint32_t authDataSize, uint32_t chunkSize, uint8_t* authData)
{
    m_authenticationData.append(reinterpret_cast<const char*>(authData), chunkSize);
    if (m_authenticationData.length() != authDataSize)
        return;

    // Unregister to register again
    NXPL_UnregisterNexusDisplayPlatform(m_nxplHandle);
    NxClient_Free(&m_allocResults);
    NxClient_Uninit();

    NEXUS_Certificate certificate;
    BKNI_Memcpy(certificate.data, m_authenticationData.data(), m_authenticationData.length());
    certificate.length = m_authenticationData.length();

    NEXUS_ClientAuthenticationSettings authSettings;
    NEXUS_Platform_GetDefaultClientAuthenticationSettings(&authSettings);
    authSettings.certificate = certificate;

    if (NEXUS_Platform_AuthenticatedJoin(&authSettings))
        fprintf(stderr, "Backend: failed to join\n");

    NXPL_RegisterNexusDisplayPlatform(&m_nxplHandle, nullptr);

    std::unique_lock<std::mutex> locker(m_authenticationMutex);
    m_authenticated = true;
    m_authenticationCondition.notify_one();
}

struct EGLTarget : public IPC::Client::Handler {
    EGLTarget(struct wpe_renderer_backend_egl_target*, int);
    virtual ~EGLTarget();

    void initialize(Backend&);
    // IPC::Client::Handler
    void handleMessage(char* data, size_t size) override;

    void constructTarget(uint32_t, uint32_t, uint32_t);

    struct wpe_renderer_backend_egl_target* target;
    IPC::Client ipcClient;

    void* m_nativeWindow { nullptr };
    Backend* m_backend { nullptr };
    uint32_t m_width { 0 };
    uint32_t m_height { 0 };
};

EGLTarget::EGLTarget(struct wpe_renderer_backend_egl_target* target, int hostFd)
    : target(target)
{
    ipcClient.initialize(*this, hostFd);
}

EGLTarget::~EGLTarget()
{
    ipcClient.deinitialize();
    NXPL_DestroyNativeWindow(m_nativeWindow);
}

void EGLTarget::initialize(Backend& backend)
{
    m_backend = &backend;

    // Wait for the TargetConstruction message from wpe_view_backend.
    while (!m_nativeWindow)
        ipcClient.readSynchronously();
}

void EGLTarget::handleMessage(char* data, size_t size)
{
    if (size != IPC::Message::size)
        return;

    auto& message = IPC::Message::cast(data);
    switch (message.messageCode) {
    case IPC::BCMNexusWL::TargetConstruction::code:
    {
        auto& targetConstruction = IPC::BCMNexusWL::TargetConstruction::cast(message);
        constructTarget(targetConstruction.handle, targetConstruction.width, targetConstruction.height);
        break;
    }
    case IPC::BCMNexusWL::Authentication::code:
    {
        auto& authentication = IPC::BCMNexusWL::Authentication::cast(message);
        m_backend->authenticate(authentication.authDataSize, authentication.chunkSize, authentication.data);
        break;
    }
    case IPC::BCMNexusWL::FrameComplete::code:
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
    NXPL_NativeWindowInfo windowInfo;
    windowInfo.x = 0;
    windowInfo.y = 0;
    windowInfo.width = width;
    windowInfo.height = height;
    windowInfo.stretch = false;
    windowInfo.zOrder = 0;
    windowInfo.clientID = handle;
    m_nativeWindow = NXPL_CreateNativeWindow(&windowInfo);

    m_width = width;
    m_height = height;
}

} // namespace BCMNexusWL

extern "C" {

struct wpe_renderer_backend_egl_interface bcm_nexus_wayland_renderer_backend_egl_interface = {
    // create
    [](int) -> void*
    {
        return new BCMNexusWL::Backend;
    },
    // destroy
    [](void* data)
    {
        auto* backend = static_cast<BCMNexusWL::Backend*>(data);
        delete backend;
    },
    // get_native_display
    [](void* data) -> EGLNativeDisplayType
    {
        return EGL_DEFAULT_DISPLAY;
    },
};

struct wpe_renderer_backend_egl_target_interface bcm_nexus_wayland_renderer_backend_egl_target_interface = {
    // create
    [](struct wpe_renderer_backend_egl_target* target, int host_fd) -> void*
    {
        return new BCMNexusWL::EGLTarget(target, host_fd);
    },
    // destroy
    [](void* data)
    {
        auto* target = static_cast<BCMNexusWL::EGLTarget*>(data);
        delete target;
    },
    // initialize
    [](void* data, void* backend_data, uint32_t width, uint32_t height)
    {
        auto& target = *static_cast<BCMNexusWL::EGLTarget*>(data);
        auto& backend = *static_cast<BCMNexusWL::Backend*>(backend_data);
        target.initialize(backend);
    },
    // get_native_window
    [](void* data) -> EGLNativeWindowType
    {
        auto& target = *static_cast<BCMNexusWL::EGLTarget*>(data);
        return target.m_nativeWindow;
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
        auto& target = *static_cast<BCMNexusWL::EGLTarget*>(data);

        IPC::Message message;
        IPC::BCMNexusWL::BufferCommit::construct(message, target.m_width, target.m_height);
        target.ipcClient.sendMessage(IPC::Message::data(message), IPC::Message::size);
    },
};

struct wpe_renderer_backend_egl_offscreen_target_interface bcm_nexus_wayland_renderer_backend_egl_offscreen_target_interface = {
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
        auto& backend = *static_cast<BCMNexusWL::Backend*>(backend_data);

        // This can be called from the main thread before the Backend had the chance to authenticate. As no context
        // can be created before the Backend is authenticated, make the caller wait here until that happens.
        backend.waitForAuthenticationIfNeeded();
    },
    // get_native_window
    [](void* data) -> EGLNativeWindowType
    {
        return nullptr;
    },
};

}
