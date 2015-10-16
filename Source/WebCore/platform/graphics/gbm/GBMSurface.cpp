/*
 * Copyright (C) 2015 Igalia S.L.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "GBMSurface.h"

#if PLATFORM(GBM)

#include "GLContextEGL.h"
#include "PlatformDisplayGBM.h"
#include <gbm.h>
#include <unistd.h>

namespace WebCore {

std::unique_ptr<GBMSurface> GBMSurface::create(const IntSize& size, Client& client)
{
    struct gbm_surface* surface = gbm_surface_create(downcast<PlatformDisplayGBM>(PlatformDisplay::sharedDisplay()).native(), 2048, 2048, GBM_FORMAT_ARGB8888, 0);
    return std::make_unique<GBMSurface>(surface, size, client);
}

GBMSurface::GBMSurface(struct gbm_surface* surface, const IntSize& size, Client& client)
    : m_surface(surface)
    , m_size(size)
    , m_client(client)
{
}

GBMSurface::~GBMSurface()
{
    if (m_surface)
        gbm_surface_destroy(m_surface);
    m_surface = nullptr;
}

void GBMSurface::resize(const IntSize& size)
{
    m_size = size;
}

std::unique_ptr<GLContextEGL> GBMSurface::createGLContext() const
{
    return GLContextEGL::createWindowContext(m_surface, GLContext::sharingContext());
}

GBMSurface::GBMBufferExport GBMSurface::lockFrontBuffer()
{
    struct gbm_bo* bo = gbm_surface_lock_front_buffer(m_surface);
    ASSERT(bo);

    uint32_t handle = gbm_bo_get_handle(bo).u32;
    auto result = m_lockedBuffers.add(handle, bo);
    ASSERT_UNUSED(result, result.isNewEntry);

    auto* data = static_cast<GBMBufferExport*>(gbm_bo_get_user_data(bo));
    if (data) {
        auto& bufferExport = *data;
        IntSize boSize(std::get<2>(bufferExport), std::get<3>(bufferExport));
        if (m_size == boSize)
            return bufferExport;

        delete data;
    }

    int fd = gbm_bo_get_fd(bo);
    data = new GBMBufferExport{ -1, handle, m_size.width(), m_size.height(), gbm_bo_get_stride(bo), gbm_bo_get_format(bo), fd };
    gbm_bo_set_user_data(bo, data, &boDataDestroyCallback);

    auto bufferExport = *data;
    std::get<0>(bufferExport) = dup(fd);
    return bufferExport;
}

void GBMSurface::releaseBuffer(uint32_t handle)
{
    ASSERT(m_lockedBuffers.contains(handle));
    struct gbm_bo* bo = m_lockedBuffers.take(handle);
    if (bo)
        gbm_surface_release_buffer(m_surface, bo);
}

void GBMSurface::boDataDestroyCallback(struct gbm_bo*, void* data)
{
    if (!data)
        return;

    auto* bufferExport = static_cast<GBMBufferExport*>(data);
    close(std::get<6>(*bufferExport));
    delete bufferExport;
}

} // namespace WebCore

#endif // PLATFORM(GBM)
