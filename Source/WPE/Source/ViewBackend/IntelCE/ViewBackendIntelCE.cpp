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
#include "ViewBackendIntelCE.h"

#if WPE_BACKEND(INTEL_CE)

#include "LibinputServer.h"
#include <cstdio>
#include <libgdl.h>

// Plane size and position
#define ORIGIN_X 0
#define ORIGIN_Y 0
#define WIDTH 1280
#define HEIGHT 720
#define ASPECT ((GLfloat)WIDTH / (GLfloat)HEIGHT)

// Initializes a plane for the graphics to be rendered to
static gdl_ret_t setup_plane(gdl_plane_id_t plane) {
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
        if (dstRect.height != HEIGHT) {
            useScaler = GDL_TRUE;
        }
    }

    srcRect.origin.x = 0;
    srcRect.origin.y = 0;
    srcRect.width = WIDTH;
    srcRect.height = HEIGHT;

    rc = gdl_plane_reset(plane);
    if (GDL_SUCCESS == rc) {
        rc = gdl_plane_config_begin(plane);
    }

    if (GDL_SUCCESS == rc) {
        rc = gdl_plane_set_attr(GDL_PLANE_SRC_COLOR_SPACE, &colorSpace);
    }

    if (GDL_SUCCESS == rc) {
        rc = gdl_plane_set_attr(GDL_PLANE_SCALE, &useScaler);
    }

    if (GDL_SUCCESS == rc) {
        rc = gdl_plane_set_attr(GDL_PLANE_PIXEL_FORMAT, &pixelFormat);
    }

    if (GDL_SUCCESS == rc) {
        rc = gdl_plane_set_attr(GDL_PLANE_DST_RECT, &dstRect);
    }

    if (GDL_SUCCESS == rc) {
        rc = gdl_plane_set_attr(GDL_PLANE_SRC_RECT, &srcRect);
    }

    if (GDL_SUCCESS == rc) {
        rc = gdl_plane_config_end(GDL_FALSE);
    } else {
        gdl_plane_config_end(GDL_TRUE);
    }

    if (GDL_SUCCESS != rc) {
        fprintf(stderr, "GDL configuration failed! GDL error code is 0x%x\n",
                rc);
    }

    return rc;
}

namespace WPE {

namespace ViewBackend {

ViewBackendIntelCE::ViewBackendIntelCE()
    : m_width(WIDTH)
    , m_height(HEIGHT)
{
    gdl_plane_id_t plane = GDL_PLANE_ID_UPP_D;
    gdl_init(0);
    setup_plane(plane);
}

ViewBackendIntelCE::~ViewBackendIntelCE()
{
    LibinputServer::singleton().setClient(nullptr);
}

void ViewBackendIntelCE::setClient(Client* client)
{
    m_client = client;
    if (m_client)
        m_client->setSize(m_width, m_height);
}

uint32_t ViewBackendIntelCE::constructRenderingTarget(uint32_t, uint32_t)
{
    return 0;
}

void ViewBackendIntelCE::commitBuffer(int, const uint8_t*, size_t)
{
    if (m_client)
        m_client->frameComplete();
}

void ViewBackendIntelCE::destroyBuffer(uint32_t)
{
}

void ViewBackendIntelCE::setInputClient(Input::Client* client)
{
    LibinputServer::singleton().setClient(client);
}

} // namespace ViewBackend

} // namespace WPE

#endif // WPE_BACKEND(INTEL_CE)
