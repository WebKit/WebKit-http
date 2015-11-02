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
#include <cstdio>
#include <refsw/nexus_config.h>
#include <refsw/nexus_platform.h>
#include <refsw/nexus_display.h>
#include <refsw/nexus_core_utils.h>
#include <refsw/default_nexus.h>

namespace WebCore {

static void hotplugCallback(void *pParam, int iParam)
{
   NEXUS_HdmiOutputStatus status;
   NEXUS_HdmiOutputHandle hdmi = (NEXUS_HdmiOutputHandle)pParam;
   NEXUS_DisplayHandle display = (NEXUS_DisplayHandle)iParam;

   NEXUS_HdmiOutput_GetStatus(hdmi, &status);

   fprintf (stdout, "HDMI hotplug event: %s", status.connected?"connected":"not connected");

   /* the app can choose to switch to the preferred format, but it's not required. */
   if (status.connected)
   {
      NEXUS_DisplaySettings displaySettings;
      NEXUS_Display_GetSettings(display, &displaySettings);

      fprintf (stdout, "Switching to preferred format %d", status.preferredVideoFormat);

      displaySettings.format = status.preferredVideoFormat;

      NEXUS_Display_SetSettings(display, &displaySettings);
   }
}

PlatformDisplayBCMNexus::PlatformDisplayBCMNexus()
{
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

    NEXUS_PlatformConfiguration platformConfiguration;
    NEXUS_Platform_GetConfiguration(&platformConfiguration);
    if (platformConfiguration.outputs.hdmi[0]) {
        NEXUS_Display_AddOutput(displayHandle, NEXUS_HdmiOutput_GetVideoConnector(platformConfiguration.outputs.hdmi[0]));

        NEXUS_HdmiOutputSettings hdmiSettings;
        NEXUS_HdmiOutput_GetSettings(platformConfiguration.outputs.hdmi[0], &hdmiSettings);
        hdmiSettings.hotplugCallback.callback = hotplugCallback;
        hdmiSettings.hotplugCallback.context = platformConfiguration.outputs.hdmi[0];
        hdmiSettings.hotplugCallback.param = (int)displayHandle;

        NEXUS_HdmiOutput_SetSettings(platformConfiguration.outputs.hdmi[0], &hdmiSettings);
        hotplugCallback(platformConfiguration.outputs.hdmi[0], (int)displayHandle);
    }

    NEXUS_VideoFormatInfo videoFormatInfo;
    NEXUS_Display_GetSettings(displayHandle, &displaySettings);
    NEXUS_VideoFormat_GetInfo(displaySettings.format, &videoFormatInfo);
    fprintf(stderr, "PlatformDisplayBCMNexus: video format (%d,%d)\n",
        videoFormatInfo.width, videoFormatInfo.height);
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
