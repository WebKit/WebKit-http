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
#include "ViewBackendDRM.h"

#if WPE_BACKEND(DRM)

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

        if (handlerData.bufferLocked) {
            handlerData.bufferLocked = false;
            handlerData.client->releaseBuffer(handlerData.lockedBufferHandle);
        }
    }
}

ViewBackendDRM::ViewBackendDRM()
    : m_pageFlipData{ nullptr, false, 0 }
{
    m_drm.fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if (m_drm.fd < 0) {
        fprintf(stderr, "ViewBackendDRM: couldn't connect DRM\n");
        return;
    }

    drm_magic_t magic;
    if (drmGetMagic(m_drm.fd, &magic) || drmAuthMagic(m_drm.fd, magic)) {
        close(m_drm.fd);
        m_drm.fd = -1;
        return;
    }

    m_drm.gbmDevice = gbm_create_device(m_drm.fd);
    if (!m_drm.gbmDevice) {
        close(m_drm.fd);
        m_drm.fd = -1;
        return;
    }

    drmModeRes* resources = drmModeGetResources(m_drm.fd);
    if (!resources) {
        close(m_drm.fd);
        m_drm.fd = -1;
        return;
    }

    drmModeConnector* connector = nullptr;
    for (int i = 0; i < resources->count_connectors; ++i) {
        connector = drmModeGetConnector(m_drm.fd, resources->connectors[i]);
        if (connector->connection == DRM_MODE_CONNECTED)
            break;

        drmModeFreeConnector(connector);
    }

    if (!connector) {
        close(m_drm.fd);
        m_drm.fd = -1;
        return;
    }

    int area = 0;
    for (int i = 0; i < connector->count_modes; ++i) {
        drmModeModeInfo* currentMode = &connector->modes[i];
        int modeArea = currentMode->hdisplay * currentMode->vdisplay;
        if (modeArea > area) {
            m_drm.mode = currentMode;
            area = modeArea;
        }
    }

    if (!m_drm.mode) {
        close(m_drm.fd);
        m_drm.fd = -1;
        return;
    }

    drmModeEncoder* encoder = nullptr;
    for (int i = 0; i < resources->count_encoders; ++i) {
        encoder = drmModeGetEncoder(m_drm.fd, resources->encoders[i]);
        if (encoder->encoder_id == connector->encoder_id)
            break;

        drmModeFreeEncoder(encoder);
    }

    m_drm.crtcId = encoder->crtc_id;
    m_drm.connectorId = connector->connector_id;
    fprintf(stderr, "ViewBackendDRM: successfully initialized DRM\n");

    drmModeFreeEncoder(encoder);
    drmModeFreeConnector(connector);
    drmModeFreeResources(resources);

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

    m_pageFlipData = { nullptr, false, 0 };

    if (m_drm.mode)
        drmModeFreeModeInfo(m_drm.mode);
    if (m_drm.gbmDevice)
        gbm_device_destroy(m_drm.gbmDevice);
    if (m_drm.fd >= 0)
        close(m_drm.fd);
    m_drm = { -1, nullptr, nullptr, 0, 0 };
}

void ViewBackendDRM::setClient(Client* client)
{
    m_pageFlipData.client = client;
}

void ViewBackendDRM::commitPrimeBuffer(int fd, uint32_t handle, uint32_t width, uint32_t height, uint32_t stride, uint32_t format)
{
    uint32_t fbID = 0;

    if (fd >= 0) {
        assert(m_fbMap.find(handle) == m_fbMap.end());
        struct gbm_import_fd_data fdData = { fd, width, height, stride, format };
        struct gbm_bo* bo = gbm_bo_import(m_drm.gbmDevice, GBM_BO_IMPORT_FD, &fdData, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);

        int ret = drmModeAddFB(m_drm.fd, gbm_bo_get_width(bo), gbm_bo_get_height(bo),
            24, 32, gbm_bo_get_stride(bo), gbm_bo_get_handle(bo).u32, &fbID);
        if (ret) {
            fprintf(stderr, "ViewBackendDRM: failed to add FB: %s, fbID %d\n", strerror(errno), fbID);
            return;
        }
        m_fbMap.insert({ handle, { bo, fbID } });

        m_pageFlipData.bufferLocked = true;
        m_pageFlipData.lockedBufferHandle = handle;
    } else {
        auto it = m_fbMap.find(handle);
        assert(it != m_fbMap.end());
        fbID = it->second.second;

        m_pageFlipData.bufferLocked = true;
        m_pageFlipData.lockedBufferHandle = handle;
    }

    int ret = drmModePageFlip(m_drm.fd, m_drm.crtcId, fbID, DRM_MODE_PAGE_FLIP_EVENT, &m_pageFlipData);
    if (ret)
        fprintf(stderr, "ViewBackendDRM: failed to queue page flip\n");
}

void ViewBackendDRM::destroyPrimeBuffer(uint32_t handle)
{
    auto it = m_fbMap.find(handle);
    assert(it != m_fbMap.end());

    drmModeRmFB(m_drm.fd, it->second.second);
    m_fbMap.erase(it);
}

} // namespace ViewBackend

} // namespace WPE

#endif // WPE_BACKEND(DRM)
