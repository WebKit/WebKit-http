/*
 * Copyright (C) 2015 Igalia S.L.
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Config.h"
#include "RenderingBackendGBM.h"

#if WPE_BACKEND(DRM) || WPE_BACKEND(WAYLAND)

#include "BufferDataGBM.h"
#include <cassert>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

namespace WPE {

namespace Graphics {

RenderingBackendGBM::RenderingBackendGBM()
{
    // FIXME: This path should be retrieved with udev.
    m_gbm.fd = open("/dev/dri/renderD128", O_RDWR | O_CLOEXEC | O_NOCTTY | O_NONBLOCK);
    if (m_gbm.fd < 0) {
        fprintf(stderr, "RenderingBackendGBM: cannot open the render node.\n");
        return;
    }

    m_gbm.device = gbm_create_device(m_gbm.fd);
    if (!m_gbm.device) {
        fprintf(stderr, "RenderingBackendGBM: cannot create the GBM device.\n");
        return;
    }
}

RenderingBackendGBM::~RenderingBackendGBM()
{
    if (m_gbm.device)
        gbm_device_destroy(m_gbm.device);
    if (m_gbm.fd >= 0)
        close(m_gbm.fd);
    m_gbm = { };
}

EGLNativeDisplayType RenderingBackendGBM::nativeDisplay()
{
    return m_gbm.device;
}

std::unique_ptr<RenderingBackend::Surface> RenderingBackendGBM::createSurface(uint32_t width, uint32_t height, uint32_t targetHandle, RenderingBackend::Surface::Client& client)
{
    return std::unique_ptr<RenderingBackendGBM::Surface>(new RenderingBackendGBM::Surface(*this, width, height, targetHandle, client));
}

std::unique_ptr<RenderingBackend::OffscreenSurface> RenderingBackendGBM::createOffscreenSurface()
{
    return std::unique_ptr<RenderingBackendGBM::OffscreenSurface>(new RenderingBackendGBM::OffscreenSurface(*this));
}

RenderingBackendGBM::Surface::Surface(const RenderingBackendGBM& renderingBackend, uint32_t width, uint32_t height, uint32_t targetHandle, RenderingBackendGBM::Surface::Client& client)
    : m_client(client)
{
    m_size = { width, height };
    auto boSize = m_size;
    if (!targetHandle)
        boSize = { 2048, 2048 };

    m_surface = gbm_surface_create(renderingBackend.m_gbm.device, boSize.first, boSize.second, GBM_FORMAT_ARGB8888, 0);
}

RenderingBackendGBM::Surface::~Surface()
{
    for (auto& it : m_lockedBuffers)
        m_client.destroyBuffer(it.first);

    if (m_surface)
        gbm_surface_destroy(m_surface);
}

EGLNativeWindowType RenderingBackendGBM::Surface::nativeWindow()
{
    return m_surface;
}

void RenderingBackendGBM::Surface::resize(uint32_t width, uint32_t height)
{
    m_size = { width, height };
}

static void destroyBOData(struct gbm_bo*, void* data)
{
    if (!data)
        return;

    auto* bufferData = static_cast<BufferDataGBM*>(data);
    delete bufferData;
}

RenderingBackend::BufferExport RenderingBackendGBM::Surface::lockFrontBuffer()
{
    struct gbm_bo* bo = gbm_surface_lock_front_buffer(m_surface);
    assert(bo);

    uint32_t handle = gbm_bo_get_handle(bo).u32;
#ifndef NDEBUG
    auto result =
#endif
        m_lockedBuffers.insert({ handle, bo });
    assert(result.second);

    auto* data = static_cast<BufferDataGBM*>(gbm_bo_get_user_data(bo));
    if (data) {
        std::pair<uint32_t, uint32_t> storedSize{ data->width, data->height };
        if (m_size == storedSize)
            return RenderingBackend::BufferExport{ -1, reinterpret_cast<const uint8_t*>(data), sizeof(BufferDataGBM) };

        delete data;
    }

    data = new BufferDataGBM{ handle, m_size.first, m_size.second, gbm_bo_get_stride(bo), gbm_bo_get_format(bo), BufferDataGBM::magicValue };
    gbm_bo_set_user_data(bo, data, &destroyBOData);

    return RenderingBackend::BufferExport{ gbm_bo_get_fd(bo), reinterpret_cast<const uint8_t*>(data), sizeof(BufferDataGBM) };
}

void RenderingBackendGBM::Surface::releaseBuffer(uint32_t handle)
{
    auto it = m_lockedBuffers.find(handle);
    assert(it != m_lockedBuffers.end());

    struct gbm_bo* bo = it->second;
    if (bo)
        gbm_surface_release_buffer(m_surface, bo);
    m_lockedBuffers.erase(it);
}

RenderingBackendGBM::OffscreenSurface::OffscreenSurface(const RenderingBackendGBM& renderingBackend)
{
    m_surface = gbm_surface_create(renderingBackend.m_gbm.device, 1, 1, GBM_FORMAT_ARGB8888, 0);
}

RenderingBackendGBM::OffscreenSurface::~OffscreenSurface()
{
    gbm_surface_destroy(m_surface);
}

EGLNativeWindowType RenderingBackendGBM::OffscreenSurface::nativeWindow()
{
    return m_surface;
}

} // namespace Graphics

} // namespace WPE

#endif // WPE_BACKEND(DRM) || WPE_BACKEND(WAYLAND)
