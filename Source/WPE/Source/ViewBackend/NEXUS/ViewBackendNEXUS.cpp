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

#if WPE_BACKEND(BCM_NEXUS)

#include "LibinputServer.h"
#include <cassert>

namespace WPE {

namespace ViewBackend {

ViewBackendNexus::ViewBackendNexus()
    : m_client(nullptr)
    , m_width(1280)
    , m_height(720)
{
}

ViewBackendNexus::~ViewBackendNexus()
{
    LibinputServer::singleton().setClient(nullptr);
}

void ViewBackendNexus::setClient(Client* client)
{
    assert ((client != nullptr) ^ (m_client != nullptr));
    m_client = client;
}

uint32_t ViewBackendNexus::createBCMNexusElement(int32_t, int32_t)
{
    // Hard-code to returning the 0 client ID.
    return 0;
}

void ViewBackendNexus::commitBCMNexusBuffer(uint32_t, uint32_t, uint32_t)
{
    // Just a pass-through for now -- immediately return a frame completion signal.
    if (m_client)
        m_client->frameComplete();
}

void ViewBackendNexus::setInputClient(Input::Client* client)
{
    LibinputServer::singleton().setClient(client);
}

} // namespace ViewBackend

} // namespace WPE

#endif // WPE_BACKEND(BCM_RPI)
