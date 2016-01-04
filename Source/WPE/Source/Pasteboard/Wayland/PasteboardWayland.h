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

#ifndef WPE_Pasteboard_PasteboardWayland_h
#define WPE_Pasteboard_PasteboardWayland_h

#if WPE_BACKEND(WAYLAND)

#include <WPE/Pasteboard/Pasteboard.h>
#include <list>
#include <map>
#include <memory>
#include <vector>

struct wl_data_device;
struct wl_data_offer;
struct wl_data_source;

namespace WPE {

namespace Pasteboard {

class PasteboardWayland final : public Pasteboard {
public:
    std::vector<std::string> getTypes() override;
    std::string getString(const std::string) override;
    void write(std::map<std::string, std::string>&&) override;
    void write(std::string&&, std::string&&) override;

    PasteboardWayland();
    ~PasteboardWayland();

    struct DataDeviceData {
    DataDeviceData() : data_offer(nullptr) {}
        wl_data_offer* data_offer;
        std::list<std::string> dataTypes;
    };

    struct DataSourceData {
        DataSourceData() : data_source(nullptr) {}
        wl_data_source* data_source;
        std::map<std::string, std::string> dataMap;
    };

private:

    struct wl_data_device* m_dataDevice { nullptr };
    DataDeviceData m_dataDeviceData;
    DataSourceData m_dataSourceData;
};

} // namespace Pasteboard

} // namespace WPE

#endif //WPE_BACKEND(WAYLAND)

#endif // WPE_ViewBackend_PasteboardWayland_h
