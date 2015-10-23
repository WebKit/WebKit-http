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
#include "ViewBackendNEXUS.h"

#if WPE_BACKEND(NEXUS)

#include "LibinputServer.h"
#include <refsw/nexus_config.h>
#include <refsw/nexus_platform.h>
#include <refsw/nexus_display.h>
#include <refsw/nexus_core_utils.h>
#include <refsw/default_nexus.h>

#ifdef NEXUS_SURFACE_COMPOSITION
#include <refsw/refsw_session_simple_client.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

static unsigned int         gs_requested_screen_width  = static_cast<unsigned int>(~0);
static unsigned int         gs_requested_screen_height = static_cast<unsigned int>(~0);
static NEXUS_DisplayHandle  gs_nexus_display = 0;
static NEXUS_SurfaceClient* gs_native_window = 0;
static NXPL_PlatformHandle  nxpl_handle = 0;

#ifndef NEXUS_SURFACE_COMPOSITION

static NEXUS_VideoFormat gs_requested_video_format  = static_cast<NEXUS_VideoFormat>(~0);

typedef struct {
  NEXUS_VideoFormat format;
  const char* name;
} formatTable;

static formatTable gs_possible_formats[] = {

  { NEXUS_VideoFormat_e1080i,     "1080i" },
  { NEXUS_VideoFormat_e720p,      "720p" },
  { NEXUS_VideoFormat_e480p,      "480p" },
  { NEXUS_VideoFormat_eNtsc,      "480i" },
  { NEXUS_VideoFormat_e720p50hz,  "720p50Hz" },
  { NEXUS_VideoFormat_e1080p24hz, "1080p24Hz" },
  { NEXUS_VideoFormat_e1080i50hz, "1080i50Hz" },
  { NEXUS_VideoFormat_e1080p50hz, "1080p50Hz" },
  { NEXUS_VideoFormat_e1080p60hz, "1080p60Hz" },
  { NEXUS_VideoFormat_ePal,       "576i" },
  { NEXUS_VideoFormat_e576p,      "576p" },

  /* END of ARRAY */
  { static_cast<NEXUS_VideoFormat>(~0), NULL }
};

#if NEXUS_NUM_HDMI_OUTPUTS && !NEXUS_DTV_PLATFORM

static void hotplug_callback(void *pParam, int iParam)
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

      /* See if the format is supprted */
      if ( (gs_requested_video_format != static_cast<NEXUS_VideoFormat>(~0)) && (status.videoFormatSupported[gs_requested_video_format]) )
      {
        displaySettings.format = gs_requested_video_format;
      }
      else
      {
        displaySettings.format = status.preferredVideoFormat;
      }

      NEXUS_Display_SetSettings(display, &displaySettings);
   }
}

#endif

void InitHDMIOutput(NEXUS_DisplayHandle display)
{

#if NEXUS_NUM_HDMI_OUTPUTS && !NEXUS_DTV_PLATFORM

#ifndef NEXUS_SURFACE_COMPOSITION
  unsigned int index = 0;
  const char* environment = getenv ( "QT_VIDEO_FORMAT" );

  if ( (environment != NULL) && (environment[0] != '\0') )
  {
    fprintf (stdout, "Defined QT_VIDEO_FORMAT: <%s>\n", environment);
    // See if we can find it.
    while ( (gs_possible_formats[index].name != NULL) && (strcmp(gs_possible_formats[index].name, environment) != 0) )
    {
      index++;
    }
    if (gs_possible_formats[index].name != NULL)
    {
      gs_requested_video_format = gs_possible_formats[index].format;
    }
  }

  environment = getenv( "QT_QPA_EGLFS_PHYSICAL_WIDTH" );

  if ( (environment != NULL) && (environment[0] != '\0') )
  {
    fprintf (stdout, "Defined QT_QPA_EGLFS_PHYSICAL_WIDTH: <%s>\n", environment);
    gs_requested_screen_width = ::atoi(environment);
  }

  environment = getenv( "QT_QPA_EGLFS_PHYSICAL_HEIGHT" );

  if ( (environment != NULL) && (environment[0] != '\0') )
  {
    fprintf (stdout, "Defined QT_QPA_EGLFS_PHYSICAL_HEIGHT: <%s>\n", environment);
    gs_requested_screen_height = ::atoi(environment);
  }
#endif

   NEXUS_HdmiOutputSettings      hdmiSettings;
   NEXUS_PlatformConfiguration   platform_config;

   NEXUS_Platform_GetConfiguration(&platform_config);

   if (platform_config.outputs.hdmi[0])
   {
      NEXUS_Display_AddOutput(display, NEXUS_HdmiOutput_GetVideoConnector(platform_config.outputs.hdmi[0]));

      /* Install hotplug callback -- video only for now */
      NEXUS_HdmiOutput_GetSettings(platform_config.outputs.hdmi[0], &hdmiSettings);

      hdmiSettings.hotplugCallback.callback = hotplug_callback;
      hdmiSettings.hotplugCallback.context = platform_config.outputs.hdmi[0];
      hdmiSettings.hotplugCallback.param = (int)display;

      NEXUS_HdmiOutput_SetSettings(platform_config.outputs.hdmi[0], &hdmiSettings);

      /* Force a hotplug to switch to a supported format if necessary */
      hotplug_callback(platform_config.outputs.hdmi[0], (int)display);
   }

#else

   UNUSED(display);

#endif

}

#endif

bool InitPlatform ( void )
{
   bool succeeded = true;
   NEXUS_Error err;

#ifndef NEXUS_SURFACE_COMPOSITION

   NEXUS_PlatformSettings platform_settings;

   fprintf (stdout, "Initialise Nexus Platform. GetDDefaultSettings().\n");

   /* Initialise the Nexus platform */
   NEXUS_Platform_GetDefaultSettings(&platform_settings);
   platform_settings.openFrontend = false;

   fprintf (stdout, "Initialise Nexus Platform. Init().\n");
   /* Initialise the Nexus platform */
   err = NEXUS_Platform_Init(&platform_settings);

   fprintf (stdout, "Initialisation done.\n");
   if (err)
   {
      fprintf (stderr, "Err: NEXUS_Platform_Init() failed");
      succeeded = false;
   }
   else
   {
      NEXUS_DisplayHandle    display = NULL;
      NEXUS_DisplaySettings  display_settings;

      NEXUS_Display_GetDefaultSettings(&display_settings);

      display = NEXUS_Display_Open(0, &display_settings);

      if (display == NULL)
      {
         fprintf (stderr, "Err: NEXUS_Display_Open() failed");
         succeeded = false;
      }
      else
      {
          NEXUS_VideoFormatInfo   video_format_info;
          NEXUS_GraphicsSettings  graphics_settings;
          NEXUS_Display_GetGraphicsSettings(display, &graphics_settings);

          graphics_settings.horizontalFilter = NEXUS_GraphicsFilterCoeffs_eBilinear;
          graphics_settings.verticalFilter = NEXUS_GraphicsFilterCoeffs_eBilinear;

          NEXUS_Display_SetGraphicsSettings(display, &graphics_settings);

          fprintf (stdout, "Init HDMI.\n");
          InitHDMIOutput(display);

          NEXUS_Display_GetSettings(display, &display_settings);
          NEXUS_VideoFormat_GetInfo(display_settings.format, &video_format_info);

          gs_nexus_display = display;

          if (gs_requested_screen_width == static_cast<unsigned int>(~0))
          {
            gs_requested_screen_width = video_format_info.width;
            fprintf (stdout, "Screen width not defined, using display width: %d\n", gs_requested_screen_width);
          }
          else if (gs_requested_screen_width == 0)
          {
            gs_requested_screen_width = 1280;
            fprintf (stdout, "Screen width defined as 0, using hard coded width: %d\n", gs_requested_screen_width);
          }


          if (gs_requested_screen_height == static_cast<unsigned int>(~0))
          {
            gs_requested_screen_height = video_format_info.height ;
            fprintf (stdout, "Screen height not defined, using display size: %d\n", gs_requested_screen_height);
          }
          else if (gs_requested_screen_height == 0)
          {
            gs_requested_screen_height = 720;
            fprintf (stdout, "Screen height defined as 0, using hard coded height: %d\n", gs_requested_screen_width);
          }
      }
   }

#else

   NEXUS_ClientAuthenticationSettings authSettings;

   simple_client_init("xre", &authSettings);

   err = NEXUS_Platform_AuthenticatedJoin(&authSettings);

   if ( err )
   {
      fprintf (stderr, "Error : NEXUS_Platform_AuthenticatedJoin() : %x ",  err );
      succeeded = false;
   }
   else
   {
      gs_nexus_display = 0;
      gs_requested_screen_width = 1280;
      gs_requested_screen_height = 720;
   }

#endif

   if (succeeded == true)
   {
       fprintf (stdout, "Register display.\n");
       NXPL_RegisterNexusDisplayPlatform ( &nxpl_handle, gs_nexus_display );
   }

   return succeeded;
}


void DeInitPlatform ( void )
{
   if ( gs_nexus_display != 0 )
   {
       NXPL_UnregisterNexusDisplayPlatform ( nxpl_handle );
       NEXUS_SurfaceClient_Release ( gs_native_window );
   }

   NEXUS_Platform_Uninit ();
}

#ifdef __cplusplus
}
#endif

namespace WPE {

namespace ViewBackend {

ViewBackendNEXUS::ViewBackendNEXUS()
    : m_client(nullptr)
    , m_width(0)
    , m_height(0)
{
    InitPlatform();
}

ViewBackendNEXUS::~ViewBackendNEXUS()
{
    LibinputServer::singleton().setClient(nullptr);

    DeInitPlatform();
}

void ViewBackendNEXUS::setClient(Client* client)
{
    assert ((client != nullptr) ^ (m_client != nullptr));
    m_client = client;
}

uint32_t ViewBackendNEXUS::createBCMElement(int32_t width, int32_t height)
{
   NXPL_NativeWindowInfo win_info;

   win_info.x        = 0;
   win_info.y        = 0;
   win_info.width    = gs_requested_screen_width; // Or do we wanr the given sizes ?? width
   win_info.height   = gs_requested_screen_height; // Or do we wanr the given sizes ?? height 
   win_info.stretch  = true;
   win_info.clientID = 0; //FIXME hardcoding

   fprintf (stdout, "Creating native window. Width (%d) x Height (%d)\n", gs_requested_screen_width, gs_requested_screen_height);
   gs_native_window = static_cast<NEXUS_SurfaceClient*> (NXPL_CreateNativeWindow ( &win_info ));

   return ( uint32_t ) gs_native_window;
}

void ViewBackendNEXUS::commitBCMBuffer(uint32_t elementHandle, uint32_t width, uint32_t height)
{
    // I assume we need to do an EGL Swap buffer here on the elementHandle (which is actually or NEXUS_SurfaceClient handle
    // but we need to check !!

    if (m_client)
        m_client->frameComplete();
}

void ViewBackendNEXUS::setInputClient(Input::Client* client)
{
    LibinputServer::singleton().setClient(client);
}

} // namespace ViewBackend

} // namespace WPE

#endif // WPE_BACKEND(BCM_RPI)
