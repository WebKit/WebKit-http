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
#include "BCMNexusSurface.h"

#if PLATFORM(BCM_NEXUS)

#include "GLContextEGL.h"
#include <refsw/nexus_config.h>
#include <refsw/nexus_platform.h>
#include <refsw/nexus_display.h>
#include <refsw/default_nexus.h>

namespace WebCore {

BCMNexusSurface::BCMNexusSurface(const IntSize& size, uintptr_t clientID)
    : m_width(std::max(0, size.width()))
    , m_height(std::max(0, size.height()))
{
    NXPL_NativeWindowInfo windowInfo;
    windowInfo.x = 0;
    windowInfo.y = 0;
    windowInfo.width = m_width;
    windowInfo.height = m_height;
    windowInfo.stretch = false;
    windowInfo.clientID = clientID;
    m_nativeWindow = NXPL_CreateNativeWindow(&windowInfo);
}

std::unique_ptr<GLContext> BCMNexusSurface::createGLContext()
{
    return GLContextEGL::createWindowContext(m_nativeWindow, GLContext::sharingContext());
}

BCMNexusSurface::BufferExport BCMNexusSurface::lockFrontBuffer()
{
    return BufferExport{ (uintptr_t)m_nativeWindow, m_width, m_height };
}

} // namespace WebCore

#endif
