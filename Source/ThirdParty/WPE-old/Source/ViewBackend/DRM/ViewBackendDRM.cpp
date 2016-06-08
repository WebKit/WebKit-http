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
#include "ViewBackendDRM.h"

#if WPE_BACKEND(DRM)

#include "BufferDataGBM.h"
#include <WPE/ViewBackend/ViewBackend.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <gbm.h>
#include <glib.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <unistd.h>

#if WPE_BACKEND(DRM_TEGRA)
#include <tegra_drm.h>
#include <sys/ioctl.h>
#endif

namespace WPE {

namespace ViewBackend {

namespace DRM {

class EventSource {
public:
    static GSourceFuncs sourceFuncs;

    GSource source;
    GPollFD pfd;

    int drmFD;
    drmEventContext eventContext;
};

GSourceFuncs EventSource::sourceFuncs = {
    nullptr, // prepare
    // check
    [](GSource* base) -> gboolean
    {
        auto* source = reinterpret_cast<EventSource*>(base);
        return !!source->pfd.revents;
    },
    // dispatch
    [](GSource* base, GSourceFunc, gpointer) -> gboolean
    {
        auto* source = reinterpret_cast<EventSource*>(base);

        if (source->pfd.revents & G_IO_IN)
            drmHandleEvent(source->drmFD, &source->eventContext);

        if (source->pfd.revents & (G_IO_ERR | G_IO_HUP))
            return FALSE;

        source->pfd.revents = 0;
        return TRUE;
    },
    nullptr, // finalize
    nullptr, // closure_callback
    nullptr, // closure_marshall
};

} // namespace DRM

static void pageFlipHandler(int, unsigned, unsigned, unsigned, void* data)
{
    auto& handlerData = *static_cast<ViewBackendDRM::PageFlipHandlerData*>(data);
    if (handlerData.client) {
        handlerData.client->frameComplete();

        auto bufferToRelease = handlerData.lockedFB;
        handlerData.lockedFB = handlerData.nextFB;
        handlerData.nextFB = { false, 0 };

        if (bufferToRelease.first)
            handlerData.client->releaseBuffer(bufferToRelease.second);
    }
}

template<typename T>
struct Cleanup {
    ~Cleanup()
    {
        if (valid)
            cleanup();
    }

    T cleanup;
    bool valid;
};

template<typename T>
auto defer(T&& cleanup) -> Cleanup<T>
{
    return Cleanup<T>{ std::forward<T>(cleanup), true };
}

ViewBackendDRM::ViewBackendDRM()
    : m_pageFlipData{ nullptr, { }, { } }
{
    decltype(m_drm) drm;
    auto drmCleanup = defer(
        [&drm] {
            if (drm.mode)
                drmModeFreeModeInfo(drm.mode);
            if (drm.fd >= 0)
                close(drm.fd);
        });


    // FIXME: This path should be retrieved via udev.
    drm.fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if (drm.fd < 0) {
        fprintf(stderr, "ViewBackendDRM: couldn't connect DRM.\n");
        return;
    }

    drm_magic_t magic;
    if (drmGetMagic(drm.fd, &magic) || drmAuthMagic(drm.fd, magic))
        return;

    decltype(m_gbm) gbm;
    auto gbmCleanup = defer(
        [&gbm] {
            if (gbm.device)
                gbm_device_destroy(gbm.device);
            if (gbm.fd >= 0)
                close(gbm.fd);
        });

    int gbmFd = drm.fd;
#if WPE_BACKEND(DRM_TEGRA)
    // FIXME: This path should be retrieved via udev.
    gbm.fd = open("/dev/dri/card1", O_RDWR | O_CLOEXEC);
    if (gbm.fd < 0) {
        fprintf(stderr, "ViewBackendDRM: couldn't open the GBM rendering node.\n");
        return;
    }
    gbmFd = gbm.fd;
#endif

    gbm.device = gbm_create_device(gbmFd);
    if (!gbm.device)
        return;

    drmModeRes* resources = drmModeGetResources(drm.fd);
    if (!resources)
        return;
    auto drmResourcesCleanup = defer([&resources] { drmModeFreeResources(resources); });

    drmModeConnector* connector = nullptr;
    for (int i = 0; i < resources->count_connectors; ++i) {
        connector = drmModeGetConnector(drm.fd, resources->connectors[i]);
        if (connector->connection == DRM_MODE_CONNECTED)
            break;

        drmModeFreeConnector(connector);
    }

    if (!connector)
        return;
    auto drmConnectorCleanup = defer([&connector] { drmModeFreeConnector(connector); });

    int area = 0;
    for (int i = 0; i < connector->count_modes; ++i) {
        drmModeModeInfo* currentMode = &connector->modes[i];
        int modeArea = currentMode->hdisplay * currentMode->vdisplay;
        if (modeArea > area) {
            drm.mode = currentMode;
            drm.size = { currentMode->hdisplay, currentMode->vdisplay };
            area = modeArea;
        }
    }

    if (!drm.mode)
        return;

    drmModeEncoder* encoder = nullptr;
    for (int i = 0; i < resources->count_encoders; ++i) {
        encoder = drmModeGetEncoder(drm.fd, resources->encoders[i]);
        if (encoder->encoder_id == connector->encoder_id)
            break;

        drmModeFreeEncoder(encoder);
    }

    if (!encoder)
        return;
    auto drmEncoderCleanup = defer([&encoder] { drmModeFreeEncoder(encoder); });

    drm.crtcId = encoder->crtc_id;
    drm.connectorId = connector->connector_id;

    m_drm = drm;
    drmCleanup.valid = false;
    m_gbm = gbm;
    gbmCleanup.valid = false;
    fprintf(stderr, "ViewBackendDRM: successfully initialized DRM.\n");

    m_eventSource = g_source_new(&DRM::EventSource::sourceFuncs, sizeof(DRM::EventSource));
    auto* source = reinterpret_cast<DRM::EventSource*>(m_eventSource);
    source->drmFD = m_drm.fd;
    source->eventContext = {
        DRM_EVENT_CONTEXT_VERSION,
        nullptr,
        &pageFlipHandler
    };

    source->pfd.fd = m_drm.fd;
    source->pfd.events = G_IO_IN | G_IO_ERR | G_IO_HUP;
    source->pfd.revents = 0;
    g_source_add_poll(m_eventSource, &source->pfd);

    g_source_set_name(m_eventSource, "[WPE] DRM");
    g_source_set_priority(m_eventSource, G_PRIORITY_HIGH + 30);
    g_source_set_can_recurse(m_eventSource, TRUE);
    g_source_attach(m_eventSource, g_main_context_get_thread_default());
}

ViewBackendDRM::~ViewBackendDRM()
{
    m_fbMap = { };

    if (m_eventSource)
        g_source_unref(m_eventSource);
    m_eventSource = nullptr;

    m_pageFlipData = { nullptr, { }, { } };

    if (m_gbm.device)
        gbm_device_destroy(m_gbm.device);
    m_gbm = { };

    if (m_drm.mode)
        drmModeFreeModeInfo(m_drm.mode);
    if (m_drm.fd >= 0)
        close(m_drm.fd);
    m_drm = { };
}

void ViewBackendDRM::setClient(Client* client)
{
    m_pageFlipData.client = client;
    client->setSize(m_drm.size.first, m_drm.size.second);
}

uint32_t ViewBackendDRM::constructRenderingTarget(uint32_t, uint32_t)
{
    // This is for now meaningless for this ViewBackend.
    return 1 << 0;
}

void ViewBackendDRM::commitBuffer(int fd, const uint8_t* data, size_t size)
{
    if (!data || size != sizeof(Graphics::BufferDataGBM)) {
        fprintf(stderr, "ViewBackendDRM: failed to validate the committed buffer\n");
        return;
    }

    auto& bufferData = *reinterpret_cast<const Graphics::BufferDataGBM*>(data);
    if (bufferData.magic != Graphics::BufferDataGBM::magicValue) {
        fprintf(stderr, "ViewBackendDRM: failed to validate the committed buffer\n");
        return;
    }

    uint32_t fbID = 0;

    if (fd >= 0) {
        assert(m_fbMap.find(bufferData.handle) == m_fbMap.end());
        struct gbm_import_fd_data fdData = { fd, bufferData.width, bufferData.height, bufferData.stride, bufferData.format };
        struct gbm_bo* bo = gbm_bo_import(m_gbm.device, GBM_BO_IMPORT_FD, &fdData, GBM_BO_USE_SCANOUT);
        uint32_t primeHandle = gbm_bo_get_handle(bo).u32;

#if WPE_BACKEND(DRM_TEGRA)
        {
            int primeFd;

            int ret = drmPrimeHandleToFD(m_gbm.fd, primeHandle, 0, &primeFd);
            if (ret) {
                fprintf(stderr, "ViewBackendDRM: failed to export the PRIME handle from the GBM device.\n");
                return;
            }

            ret = drmPrimeFDToHandle(m_drm.fd, primeFd, &primeHandle);
            if (ret) {
                fprintf(stderr, "ViewBackendDRM: failed to import the PRIME fd into the DRM device.\n");
                return;
            }

            close(primeFd);

            {
                struct drm_tegra_gem_set_tiling args;
                memset(&args, 0, sizeof(args));
                args.handle = primeHandle;
                args.mode = DRM_TEGRA_GEM_TILING_MODE_BLOCK;
                args.value = 4;

                int ret = ioctl(m_drm.fd, DRM_IOCTL_TEGRA_GEM_SET_TILING, &args);
                if (ret) {
                    fprintf(stderr, "ViewBackendDRM: failed to set tiling parameters for the Tegra GEM.\n");
                    return;
                }
            }
        }
#endif

        int ret = drmModeAddFB(m_drm.fd, gbm_bo_get_width(bo), gbm_bo_get_height(bo),
            24, 32, gbm_bo_get_stride(bo), primeHandle, &fbID);
        if (ret) {
            fprintf(stderr, "ViewBackendDRM: failed to add FB: %s, fbID %d\n", strerror(errno), fbID);
            return;
        }

        m_fbMap.insert({ bufferData.handle, { bo, fbID } });
        m_pageFlipData.nextFB = { true, bufferData.handle };
    } else {
        auto it = m_fbMap.find(bufferData.handle);
        assert(it != m_fbMap.end());

        fbID = it->second.second;
        m_pageFlipData.nextFB = { true, bufferData.handle };
    }

    int ret = drmModePageFlip(m_drm.fd, m_drm.crtcId, fbID, DRM_MODE_PAGE_FLIP_EVENT, &m_pageFlipData);
    if (ret)
        fprintf(stderr, "ViewBackendDRM: failed to queue page flip\n");
}

void ViewBackendDRM::destroyBuffer(uint32_t handle)
{
    auto it = m_fbMap.find(handle);
    assert(it != m_fbMap.end());

    drmModeRmFB(m_drm.fd, it->second.second);
    m_fbMap.erase(it);
}

} // namespace ViewBackend

} // namespace WPE

#endif // WPE_BACKEND(DRM)
