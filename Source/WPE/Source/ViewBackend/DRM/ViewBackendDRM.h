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

#ifndef WPE_ViewBackend_ViewBackendDRM_h
#define WPE_ViewBackend_ViewBackendDRM_h

#if WPE_BACKEND(DRM)

#include <WPE/ViewBackend/ViewBackend.h>

#include <unordered_map>
#include <utility>

typedef struct _drmModeModeInfo drmModeModeInfo;
struct gbm_bo;
struct gbm_device;

namespace WPE {

namespace ViewBackend {

class Client;

class ViewBackendDRM final : public ViewBackend {
public:
    ViewBackendDRM();
    ~ViewBackendDRM();


    void setClient(Client*) override;
    void commitPrimeBuffer(int fd, uint32_t handle, uint32_t width, uint32_t height, uint32_t stride, uint32_t format) override;
    void destroyPrimeBuffer(uint32_t handle) override;

    struct PageFlipHandlerData {
        Client* client;
        bool bufferLocked { false };
        uint32_t lockedBufferHandle { 0 };
    };

private:
    struct {
        int fd;
        struct gbm_device* device;
    } m_gbm;

    struct {
        int fd;
        drmModeModeInfo* mode;
        uint32_t crtcId;
        uint32_t connectorId;
    } m_drm;

    PageFlipHandlerData m_pageFlipData;

    std::unordered_map<uint32_t, std::pair<struct gbm_bo*, uint32_t>> m_fbMap;
};

} // namespace ViewBackend

} // namespace WPE

#endif // WPE_BACKEND(DRM)

#endif // WPE_ViewBackend_ViewBackendDRM_h
