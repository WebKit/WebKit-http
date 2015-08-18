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
#include "PlatformDisplayGBM.h"

#if PLATFORM(GBM)

#include "GBMSurface.h"
#include "IntSize.h"
#include <fcntl.h>
#include <xf86drm.h>

#include <cstdio>

namespace WebCore {

PlatformDisplayGBM::PlatformDisplayGBM()
{
    fprintf(stderr, "PlatformDisplayGBM: ctor\n");

    m_gbm.fd = open("/dev/dri/renderD128", O_RDWR | O_CLOEXEC | O_NOCTTY | O_NONBLOCK);
    if (m_gbm.fd < 0) {
        fprintf(stderr, "PlatformDisplayGBM: cannot open the render node\n");
        return;
    }

    m_gbm.device = gbm_create_device(m_gbm.fd);
    if (!m_gbm.device) {
        fprintf(stderr, "PlatformDisplayGBM: cannot create the GBM device\n");
        return;
    }

    m_eglDisplay = eglGetDisplay(m_gbm.device);
    if (m_eglDisplay == EGL_NO_DISPLAY) {
        fprintf(stderr, "PlatformDisplayGBM: cannot create the EGL display\n");
        return;
    }

    PlatformDisplay::initializeEGLDisplay();

    static const EGLint configAttributes[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 1,
        EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE, 1,
        EGL_ALPHA_SIZE, 1,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    EGLint numberOfConfigs = 0;
    if (!eglChooseConfig(m_eglDisplay, configAttributes, &m_eglConfig, 1, &numberOfConfigs) || numberOfConfigs != 1) {
        fprintf(stderr, "PlatformDisplayGBM: cannot find the desired EGLConfig\n");
        return;
    }
}

PlatformDisplayGBM::~PlatformDisplayGBM()
{
    fprintf(stderr, "PlatformDisplayGBM: dtor\n");
    if (m_gbm.device)
        gbm_device_destroy(m_gbm.device);

    if (m_gbm.fd >= 0)
        close(m_gbm.fd);
}

std::unique_ptr<GBMSurface> PlatformDisplayGBM::createSurface(const IntSize& size)
{
    struct gbm_surface* surface = gbm_surface_create(m_gbm.device, size.width(), size.height(), GBM_FORMAT_ARGB8888, 0);
    return std::make_unique<GBMSurface>(surface);
}

int PlatformDisplayGBM::lockFrontBuffer(GBMSurface& surface)
{
    struct gbm_bo* bo = gbm_surface_lock_front_buffer(surface.native());
    if (!bo) {
        fprintf(stderr, "PlatformDisplayGBM: no front bo for surface %p\n", surface.native());
        return -1;
    }

    int fd;
    int r = drmPrimeHandleToFD(m_gbm.fd, gbm_bo_get_handle(bo).u32, 0, &fd);
    if (r < 0) {
        fprintf(stderr, "PlatformDisplayGBM: couldn't get prime fd for bo %p\n", bo);
        return -1;
    }

    fprintf(stderr, "PlatformDisplayGBM: locked front bo %p, handle %u, related prime fd %d\n",
        bo, gbm_bo_get_handle(bo).u32, fd);
    return fd;
}

} // namespace WebCore

#endif // PLATFORM(GBM)
