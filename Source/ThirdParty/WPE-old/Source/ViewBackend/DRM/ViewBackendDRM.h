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

#ifndef WPE_ViewBackend_ViewBackendDRM_h
#define WPE_ViewBackend_ViewBackendDRM_h

#if WPE_BACKEND(DRM)

#include <WPE/ViewBackend/ViewBackend.h>

#include <unordered_map>
#include <utility>

struct gbm_bo;
struct gbm_device;
typedef struct _GSource GSource;
typedef struct _drmModeModeInfo drmModeModeInfo;

namespace WPE {

namespace ViewBackend {

class Client;

class ViewBackendDRM final : public ViewBackend {
public:
    ViewBackendDRM();
    virtual ~ViewBackendDRM();

    void setClient(Client*) override;
    uint32_t constructRenderingTarget(uint32_t, uint32_t) override;
    void commitBuffer(int, const uint8_t* data, size_t size) override;
    void destroyBuffer(uint32_t handle) override;

    struct PageFlipHandlerData {
        Client* client;
        std::pair<bool, uint32_t> nextFB;
        std::pair<bool, uint32_t> lockedFB;
    };

private:
    struct {
        int fd { -1 };
        drmModeModeInfo* mode { nullptr };
        std::pair<uint16_t, uint16_t> size;
        uint32_t crtcId { 0 };
        uint32_t connectorId { 0 };
    } m_drm;

    struct {
        int fd { -1 };
        struct gbm_device* device;
    } m_gbm;

    PageFlipHandlerData m_pageFlipData;

    GSource* m_eventSource;
    std::unordered_map<uint32_t, std::pair<struct gbm_bo*, uint32_t>> m_fbMap;
};

} // namespace ViewBackend

} // namespace WPE

#endif // WPE_BACKEND(DRM)

#endif // WPE_ViewBackend_ViewBackendDRM_h
