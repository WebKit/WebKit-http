/*
 * Copyright (C) 2015 Igalia S.L.
 * Copyright (C) 2015 Metrological
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Config.h"
#include "RenderingBackendBCMNexus.h"

#if WPE_BACKEND(BCM_NEXUS)

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <refsw/nexus_config.h>
#include <refsw/nexus_platform.h>
#include <refsw/nexus_display.h>
#include <refsw/nexus_core_utils.h>
#include <refsw/default_nexus.h>
#include <refsw/nxclient.h>
#include <tuple>
#include <EGL/egl.h>

namespace WPE {

namespace Graphics {

RenderingBackendBCMNexus::RenderingBackendBCMNexus() : m_nxplHandle (NULL)
{
    NEXUS_DisplayHandle displayHandle (NULL);
    NxClient_AllocSettings allocSettings;
    NxClient_JoinSettings joinSettings;
    NxClient_GetDefaultJoinSettings( &joinSettings );

    strcpy( joinSettings.name, "wpe" );

    NEXUS_Error rc = NxClient_Join( &joinSettings );
    BDBG_ASSERT(!rc);

    NxClient_GetDefaultAllocSettings(&allocSettings);
    allocSettings.surfaceClient = 1;
    rc = NxClient_Alloc(&allocSettings, &m_AllocResults);
    BDBG_ASSERT(!rc);

    NXPL_RegisterNexusDisplayPlatform(&m_nxplHandle, displayHandle);
}

RenderingBackendBCMNexus::~RenderingBackendBCMNexus()
{
    NXPL_UnregisterNexusDisplayPlatform(m_nxplHandle);
    NxClient_Free(&m_AllocResults);
    NxClient_Uninit();
}

EGLNativeDisplayType RenderingBackendBCMNexus::nativeDisplay()
{
    return EGL_DEFAULT_DISPLAY;
}

std::unique_ptr<RenderingBackend::Surface> RenderingBackendBCMNexus::createSurface(uint32_t width, uint32_t height, uint32_t /* targetHandle*/, RenderingBackend::Surface::Client& client)
{
    return std::unique_ptr<RenderingBackendBCMNexus::Surface>(new RenderingBackendBCMNexus::Surface(*this, width, height, m_AllocResults.surfaceClient[0].id, client));
}

std::unique_ptr<RenderingBackend::OffscreenSurface> RenderingBackendBCMNexus::createOffscreenSurface()
{
    return std::unique_ptr<RenderingBackendBCMNexus::OffscreenSurface>(new RenderingBackendBCMNexus::OffscreenSurface(*this));
}

RenderingBackendBCMNexus::Surface::Surface(const RenderingBackendBCMNexus&, uint32_t width, uint32_t height, uint32_t /* targetHandle */, RenderingBackend::Surface::Client&)
{
    NXPL_NativeWindowInfo windowInfo;
    windowInfo.x = 0;
    windowInfo.y = 0;
    windowInfo.width = width;
    windowInfo.height = height;
    windowInfo.stretch = false;
    // windowInfo.clientID = targetHandle;
    windowInfo.clientID = 0; // For now we only accept 0. See Mail David Montgomery
    m_nativeWindow = NXPL_CreateNativeWindow(&windowInfo);
}

RenderingBackendBCMNexus::Surface::~Surface()
{
    NEXUS_SurfaceClient_Release (reinterpret_cast<NEXUS_SurfaceClient*>(m_nativeWindow));
}

EGLNativeWindowType RenderingBackendBCMNexus::Surface::nativeWindow()
{
    return m_nativeWindow;
}

void RenderingBackendBCMNexus::Surface::resize(uint32_t, uint32_t)
{
}

RenderingBackend::BufferExport RenderingBackendBCMNexus::Surface::lockFrontBuffer()
{
    return std::make_tuple( -1, reinterpret_cast<uint8_t*>(&m_bufferData), sizeof(BufferDataBCMNexus) );
}

void RenderingBackendBCMNexus::Surface::releaseBuffer(uint32_t)
{
}

RenderingBackendBCMNexus::OffscreenSurface::OffscreenSurface(const RenderingBackendBCMNexus&)
{
}

RenderingBackendBCMNexus::OffscreenSurface::~OffscreenSurface() = default;

EGLNativeWindowType RenderingBackendBCMNexus::OffscreenSurface::nativeWindow()
{
    return nullptr;
}

} // namespace Graphics

} // namespace WPE

#endif // WPE_BACKEND(BCM_NEXUS)
