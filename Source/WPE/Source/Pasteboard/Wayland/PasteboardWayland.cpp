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
#include "PasteboardWayland.h"

#if WPE_BACKEND(WAYLAND)

#include "WaylandDisplay.h"
#include <algorithm>
#include <glib.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <unistd.h>
#include <wayland-client.h>

namespace WPE {

namespace Pasteboard {

const struct wl_data_offer_listener g_dataOfferListener = {
    // offer
    [] (void* data, struct wl_data_offer* offer, const char* type)
    {
        auto& dataDeviceData = *static_cast<PasteboardWayland::DataDeviceData*>(data);
        assert(offer == dataDeviceData.data_offer);
        dataDeviceData.dataTypes.push_back(type);
    },
};

const struct wl_data_device_listener g_dataDeviceListener = {
    // data_offer
    [] (void* data, struct wl_data_device*, struct wl_data_offer* offer)
    {
        auto& dataDeviceData = *static_cast<PasteboardWayland::DataDeviceData*>(data);

        dataDeviceData.dataTypes.clear();
        if (dataDeviceData.data_offer)
            wl_data_offer_destroy(dataDeviceData.data_offer);
        dataDeviceData.data_offer = offer;
        wl_data_offer_add_listener(offer, &g_dataOfferListener, data);
    },
    // enter
    [] (void*, struct wl_data_device*, uint32_t, struct wl_surface*, wl_fixed_t, wl_fixed_t, struct wl_data_offer*) {},
    // leave
    [] (void*, struct wl_data_device*) {},
    // motion
    [] (void*, struct wl_data_device*, uint32_t, wl_fixed_t, wl_fixed_t) {},
    // drop
    [] (void*, struct wl_data_device*) {},
    // selection
    [] (void*, struct wl_data_device*, wl_data_offer*) {},
};

PasteboardWayland::PasteboardWayland()
{
    ViewBackend::WaylandDisplay& display = ViewBackend::WaylandDisplay::singleton();

    if (!display.interfaces().data_device_manager)
        return;

    m_dataDevice = wl_data_device_manager_get_data_device(display.interfaces().data_device_manager, display.interfaces().seat);
    wl_data_device_add_listener(m_dataDevice, &g_dataDeviceListener, &m_dataDeviceData);
}

PasteboardWayland::~PasteboardWayland()
{
    if (m_dataDeviceData.data_offer)
        wl_data_offer_destroy(m_dataDeviceData.data_offer);
    m_dataDeviceData.dataTypes.clear();

    wl_data_device_destroy(m_dataDevice);
}

std::vector<std::string> PasteboardWayland::getTypes()
{
    std::vector<std::string> types;
    for (auto dataType: m_dataDeviceData.dataTypes)
        types.push_back(dataType);
    return types;
}

std::string PasteboardWayland::getString(const std::string pasteboardType)
{
    if (!std::any_of(m_dataDeviceData.dataTypes.cbegin(), m_dataDeviceData.dataTypes.cend(),
                     [pasteboardType](std::string str){ return str == pasteboardType;}))
        return std::string();

    int pipefd[2];
    // FIXME: Should probably handle this error somehow.
    if (pipe2(pipefd, O_CLOEXEC) == -1)
        return std::string();
    wl_data_offer_receive(m_dataDeviceData.data_offer, pasteboardType.c_str(), pipefd[1]);
    close(pipefd[1]);

    wl_display_roundtrip(ViewBackend::WaylandDisplay::singleton().display());

    char buf[1024];
    std::string readString;
    ssize_t length;
    do {
        length = read(pipefd[0], buf, 1024);
        readString.append(buf, length);
    } while (length > 0);
    close(pipefd[0]);

    return readString;
}

const struct wl_data_source_listener g_dataSourceListener = {
    // target
    [] (void*, struct wl_data_source*, const char*) {},
    // send
    [] (void* data, struct wl_data_source* source, const char* mime_type, int32_t fd)
    {
        auto& dataSourceData = *static_cast<PasteboardWayland::DataSourceData*>(data);
        assert(dataSourceData.data_source == source);
        assert(!dataSourceData.dataMap.count(mime_type));

        if (strncmp(mime_type, "text/", 5) == 0) {
            std::string stringToSend = dataSourceData.dataMap[mime_type];
            const char* p = stringToSend.data();
            ssize_t length = stringToSend.size();
            ssize_t written = 0;
            while (length > 0 && written != -1) {
                written = write(fd, p, length);
                p += written;
                length -= written;
            }
            close(fd);
            wl_display_roundtrip(ViewBackend::WaylandDisplay::singleton().display());
        } else
            return; // Eventually assert if we don't have a handler for a mimetype we are offering.
    },
    // cancelled
    [] (void* data, struct wl_data_source* source)
    {
        auto& dataSourceData = *static_cast<PasteboardWayland::DataSourceData*>(data);
        assert(dataSourceData.data_source == source);
        wl_data_source_destroy(source);
        dataSourceData.data_source = nullptr;
        dataSourceData.dataMap.clear();
    }
};

void PasteboardWayland::write(std::map<std::string, std::string>&& dataMap)
{
    if (m_dataSourceData.data_source)
        wl_data_source_destroy(m_dataSourceData.data_source);

    m_dataSourceData.dataMap = dataMap;

    ViewBackend::WaylandDisplay& display = ViewBackend::WaylandDisplay::singleton();
    m_dataSourceData.data_source = wl_data_device_manager_create_data_source(display.interfaces().data_device_manager);

    for (auto dataPair : m_dataSourceData.dataMap)
        wl_data_source_offer(m_dataSourceData.data_source, dataPair.first.c_str());

    wl_data_source_add_listener(m_dataSourceData.data_source, &g_dataSourceListener, &m_dataSourceData);
    wl_data_device_set_selection(m_dataDevice, m_dataSourceData.data_source, display.singleton().serial());
}

void PasteboardWayland::write(std::string&& pasteboardType, std::string&& stringToWrite)
{
    std::map<std::string, std::string> dataMap;
    dataMap[pasteboardType] = stringToWrite;
    write(std::move(dataMap));
}

} // namespace Pasteboard

} // namespace WPE

#endif // WPE_BACKEND(WAYLAND)
