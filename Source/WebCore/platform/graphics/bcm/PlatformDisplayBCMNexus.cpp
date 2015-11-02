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

#include "config.h"
#include "PlatformDisplayBCMNexus.h"

#if PLATFORM(BCM_NEXUS)

#include "IntSize.h"
#include <EGL/egl.h>
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

namespace WebCore {

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

PlatformDisplayBCMNexus::PlatformDisplayBCMNexus()
{
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
        fprintf(stderr, "PlatformDisplayBCMNexus: Failed to initialize.\n");
        return;
    }

    NEXUS_DisplaySettings displaySettings;
    NEXUS_Display_GetDefaultSettings(&displaySettings);
    NEXUS_DisplayHandle displayHandle = NEXUS_Display_Open(0, &displaySettings);
    if (!displayHandle) {
        fprintf(stderr, "PlatformDisplayBCMNexus: Failed to open the display.\n");
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
                fprintf(stderr, "PlatformDisplayBCMNexus: Requested format '%s' is not supported, issues possible.\n",
                    std::get<0>(selectedFormat));
                displaySettings.format = hdmiStatus.preferredVideoFormat;
            } else
                displaySettings.format = std::get<1>(selectedFormat);
        }
    }

    NEXUS_Display_SetSettings(displayHandle, &displaySettings);
    NXPL_RegisterNexusDisplayPlatform(&m_nxplHandle, displayHandle);

    m_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (m_eglDisplay == EGL_NO_DISPLAY) {
        fprintf(stderr, "PlatformDisplayBCMNexus: could not create the EGL display\n");
        return;
    }

    PlatformDisplay::initializeEGLDisplay();
}

PlatformDisplayBCMNexus::~PlatformDisplayBCMNexus()
{
    if (m_nxplHandle)
        NXPL_UnregisterNexusDisplayPlatform(m_nxplHandle);
    m_nxplHandle = nullptr;
}

std::unique_ptr<BCMNexusSurface> PlatformDisplayBCMNexus::createSurface(const IntSize& size, uintptr_t handle)
{
    return std::make_unique<BCMNexusSurface>(size, handle);
}

} // namespace WebCore

#endif // PLATFORM(BCM_NEXUS)
