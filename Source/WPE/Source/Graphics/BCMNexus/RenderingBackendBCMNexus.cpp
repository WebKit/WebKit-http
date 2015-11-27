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
#include <tuple>
#include <EGL/egl.h>

#ifdef NEXUS_MULTIPROCESS_ACCESS
#include <refsw/nxclient.h>
#endif

namespace WPE {

namespace Graphics {

#ifndef NEXUS_MULTIPROCESS_ACCESS

using FormatTuple = std::tuple<const char*, NEXUS_VideoFormat>;
static const std::array<FormatTuple, 9> s_formatTable = {
   FormatTuple{ "720p", NEXUS_VideoFormat_e720p },
   FormatTuple{ "1080i", NEXUS_VideoFormat_e1080i },
   FormatTuple{ "480p", NEXUS_VideoFormat_e480p },
   FormatTuple{ "480i", NEXUS_VideoFormat_eNtsc },
   FormatTuple{ "720p50Hz", NEXUS_VideoFormat_e720p50hz },
   FormatTuple{ "1080p24Hz", NEXUS_VideoFormat_e1080p24hz },
   FormatTuple{ "1080i50Hz", NEXUS_VideoFormat_e1080i50hz },
   FormatTuple{ "1080p50Hz", NEXUS_VideoFormat_e1080p50hz },
   FormatTuple{ "1080p60Hz", NEXUS_VideoFormat_e1080p60hz },
};

#endif

RenderingBackendBCMNexus::RenderingBackendBCMNexus() : m_nxplHandle (NULL)
{

#ifndef NEXUS_MULTIPROCESS_ACCESS

    const char* format = std::getenv("WPE_NEXUS_FORMAT");
    if (!format)
        format = "720p";
    auto it = std::find_if(s_formatTable.cbegin(), s_formatTable.cend(), [format](const FormatTuple& t) { return !std::strcmp(std::get<0>(t), format); });
    assert(it != s_formatTable.end());
    auto& selectedFormat = *it;

    NEXUS_PlatformSettings platformSettings;
    NEXUS_Platform_GetDefaultSettings(&platformSettings);
    platformSettings.openFrontend = false;

    NEXUS_Error err = NEXUS_Platform_Init(&platformSettings);
    if (err != NEXUS_SUCCESS) {
        fprintf(stderr, "RenderingBackendBCMNexus: Failed to initialize.\n");
        return;
    }

    NEXUS_DisplaySettings displaySettings;
    NEXUS_Display_GetDefaultSettings(&displaySettings);
    NEXUS_DisplayHandle displayHandle = NEXUS_Display_Open(0, &displaySettings);
    if (!displayHandle) {
        fprintf(stderr, "RenderingBackendBCMNexus: Failed to open the display.\n");
        return;
    }

    NEXUS_GraphicsSettings graphicsSettings;
    NEXUS_Display_GetGraphicsSettings(displayHandle, &graphicsSettings);

    graphicsSettings.horizontalFilter = NEXUS_GraphicsFilterCoeffs_eBilinear;
    graphicsSettings.verticalFilter = NEXUS_GraphicsFilterCoeffs_eBilinear;
    NEXUS_Display_SetGraphicsSettings(displayHandle, &graphicsSettings);

    NEXUS_Display_GetSettings(displayHandle, &displaySettings);

    NEXUS_PlatformConfiguration platformConfiguration;
    NEXUS_Platform_GetConfiguration(&platformConfiguration);
    if (platformConfiguration.outputs.hdmi[0]) {
        NEXUS_Display_AddOutput(displayHandle, NEXUS_HdmiOutput_GetVideoConnector(platformConfiguration.outputs.hdmi[0]));

        NEXUS_HdmiOutputStatus hdmiStatus;
        NEXUS_HdmiOutput_GetStatus(platformConfiguration.outputs.hdmi[0], &hdmiStatus);
        if (hdmiStatus.connected) {
            if (!hdmiStatus.videoFormatSupported[std::get<1>(selectedFormat)]) {
                fprintf(stderr, "RenderingBackendBCMNexus: Requested format '%s' is not supported, issues possible.\n",
                    std::get<0>(selectedFormat));
                displaySettings.format = hdmiStatus.preferredVideoFormat;
            } else
                displaySettings.format = std::get<1>(selectedFormat);
        }
    }

    NEXUS_Display_SetSettings(displayHandle, &displaySettings);

#else
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

#endif

    NXPL_RegisterNexusDisplayPlatform(&m_nxplHandle, displayHandle);
}

RenderingBackendBCMNexus::~RenderingBackendBCMNexus()
{
    NXPL_UnregisterNexusDisplayPlatform(m_nxplHandle);

#ifdef NEXUS_MULTIPROCESS_ACCESS

    NxClient_Free(&m_AllocResults);
    NxClient_Uninit();

#endif
}

EGLNativeDisplayType RenderingBackendBCMNexus::nativeDisplay()
{
    return EGL_DEFAULT_DISPLAY;
}

std::unique_ptr<RenderingBackend::Surface> RenderingBackendBCMNexus::createSurface(uint32_t width, uint32_t height, uint32_t targetHandle, RenderingBackend::Surface::Client& client)
{

#ifdef NEXUS_MULTIPROCESS_ACCESS
    return std::unique_ptr<RenderingBackendBCMNexus::Surface>(new RenderingBackendBCMNexus::Surface(*this, width, height, m_AllocResults.surfaceClient[0].id, client));
#else
    return std::unique_ptr<RenderingBackendBCMNexus::Surface>(new RenderingBackendBCMNexus::Surface(*this, width, height, targetHandle, client));
#endif
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
