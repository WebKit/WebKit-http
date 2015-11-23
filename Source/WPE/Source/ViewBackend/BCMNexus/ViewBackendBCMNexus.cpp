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
#include "ViewBackendBCMNexus.h"

#if WPE_BACKEND(BCM_NEXUS)

#include "LibinputServer.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <array>
#include <tuple>

namespace WPE {

namespace ViewBackend {

using FormatTuple = std::tuple<const char*, uint32_t, uint32_t>;
static const std::array<FormatTuple, 9> s_formatTable = {
   FormatTuple{ "1080i", 1920, 1080 },
   FormatTuple{ "720p", 1280, 720 },
   FormatTuple{ "480p", 640, 480 },
   FormatTuple{ "480i", 640, 480 },
   FormatTuple{ "720p50Hz", 1280, 720 },
   FormatTuple{ "1080p24Hz", 1920, 1080 },
   FormatTuple{ "1080i50Hz", 1920, 1080 },
   FormatTuple{ "1080p50Hz", 1920, 1080 },
   FormatTuple{ "1080p60Hz", 1920, 1080 },
};

ViewBackendBCMNexus::ViewBackendBCMNexus()
    : m_client(nullptr)
{
    const char* format = std::getenv("WPE_NEXUS_FORMAT");
    if (!format)
        format = "720p";
    auto it = std::find_if(s_formatTable.cbegin(), s_formatTable.cend(), [format](const FormatTuple& t) { return !std::strcmp(std::get<0>(t), format); });
    assert(it != s_formatTable.end());
    auto& selectedFormat = *it;

    m_width = std::get<1>(selectedFormat);
    m_height = std::get<2>(selectedFormat);
    fprintf(stderr, "ViewBackendBCMNexus: selected format '%s' (%d,%d)\n", std::get<0>(selectedFormat), m_width, m_height);
}

ViewBackendBCMNexus::~ViewBackendBCMNexus()
{
    LibinputServer::singleton().setClient(nullptr);
}

void ViewBackendBCMNexus::setClient(Client* client)
{
    assert ((client != nullptr) ^ (m_client != nullptr));
    m_client = client;
    m_client->setSize(m_width, m_height);
}

uint32_t ViewBackendBCMNexus::constructRenderingTarget(uint32_t width, uint32_t height)
{
    if (m_width != width || m_height != height)
        fprintf(stderr, "ViewBackendBCMNexus: mismatch in buffer parameters during construction.\n");

    // Hard-coded to returning the 0 client ID.
    return 0;
}

void ViewBackendBCMNexus::commitBuffer(int, const uint8_t*, size_t)
{
    // Just a pass-through for now -- immediately return a frame completion signal.
    if (m_client)
        m_client->frameComplete();
}

void ViewBackendBCMNexus::destroyBuffer(uint32_t)
{
}

void ViewBackendBCMNexus::setInputClient(Input::Client* client)
{
    LibinputServer::singleton().setClient(client);
}

} // namespace ViewBackend

} // namespace WPE

#endif // WPE_BACKEND(BCM_RPI)
