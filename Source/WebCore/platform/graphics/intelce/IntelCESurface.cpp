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
#include "IntelCESurface.h"

#if PLATFORM(INTEL_CE)

#include "GLContextEGL.h"
#include "IntSize.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

namespace WebCore {

IntelCESurface::IntelCESurface(const IntSize& size, uint32_t elementHandle)
{
    // FIXME: implement.
}

std::unique_ptr<GLContext> IntelCESurface::createGLContext()
{
    return GLContextEGL::createWindowContext((EGLNativeWindowType)0x07 /* GDL_PLANE_ID_UPP_C */, GLContext::sharingContext());
}

void IntelCESurface::resize(const IntSize& size)
{
    // FIXME: implement.
}

IntelCESurface::BufferExport IntelCESurface::lockFrontBuffer()
{
    // FIXME: implement.
    return BufferExport{ m_nativeWindow.element, m_nativeWindow.width, m_nativeWindow.height };
}

void IntelCESurface::releaseBuffer(uint32_t)
{
    // FIXME: implement.
}

} // namespace WebCore

#endif // PLATFORM(INTEL_CE)
