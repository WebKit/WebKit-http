#include "pasteboard-wayland.h"

#include "display.h"
#include <cstdlib>
#include <fcntl.h>
#include <list>
#include <map>
#include <string>
#include <wayland-client.h>

namespace Wayland {

class Pasteboard {
public:
    Pasteboard();
    ~Pasteboard();

    struct wl_data_device* m_dataDevice;

    struct DataDeviceData {
    DataDeviceData() : data_offer(nullptr) {}
        wl_data_offer* data_offer;
        std::list<std::string> dataTypes;
    } m_dataDeviceData;

    struct DataSourceData {
        DataSourceData() : data_source(nullptr) {}
        wl_data_source* data_source;
        std::map<std::string, std::string> dataMap;
    } m_dataSourceData;
};

const struct wl_data_offer_listener g_dataOfferListener = {
    // offer
    [] (void* data, struct wl_data_offer* offer, const char* type)
    {
        auto& dataDeviceData = *static_cast<Pasteboard::DataDeviceData*>(data);
        assert(offer == dataDeviceData.data_offer);
        dataDeviceData.dataTypes.push_back(type);
    },
};

const struct wl_data_device_listener g_dataDeviceListener = {
    // data_offer
    [] (void* data, struct wl_data_device*, struct wl_data_offer* offer)
    {
        auto& dataDeviceData = *static_cast<Pasteboard::DataDeviceData*>(data);

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

const struct wl_data_source_listener g_dataSourceListener = {
    // target
    [] (void*, struct wl_data_source*, const char*) {},
    // send
    [] (void* data, struct wl_data_source* source, const char* mime_type, int32_t fd)
    {
        auto& dataSourceData = *static_cast<Pasteboard::DataSourceData*>(data);
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
            wl_display_roundtrip(Wayland::Display::singleton().display());
        } else
            return; // Eventually assert if we don't have a handler for a mimetype we are offering.
    },
    // cancelled
    [] (void* data, struct wl_data_source* source)
    {
        auto& dataSourceData = *static_cast<Pasteboard::DataSourceData*>(data);
        assert(dataSourceData.data_source == source);
        wl_data_source_destroy(source);
        dataSourceData.data_source = nullptr;
        dataSourceData.dataMap.clear();
    }
};

Pasteboard::Pasteboard()
{
    Wayland::Display& display = Wayland::Display::singleton();

    if (!display.interfaces().data_device_manager)
        return;

    m_dataDevice = wl_data_device_manager_get_data_device(display.interfaces().data_device_manager, display.interfaces().seat);
    wl_data_device_add_listener(m_dataDevice, &g_dataDeviceListener, &m_dataDeviceData);
}

Pasteboard::~Pasteboard()
{
    if (m_dataDeviceData.data_offer)
        wl_data_offer_destroy(m_dataDeviceData.data_offer);
    m_dataDeviceData.dataTypes.clear();

    wl_data_device_destroy(m_dataDevice);
}

} // namespace Wayland

extern "C" {

struct wpe_pasteboard_interface wayland_pasteboard_interface = {
    // initialize
    [](struct wpe_pasteboard*) -> void*
    {
        return new Wayland::Pasteboard;
    },
    // get_types
    [](void* data, struct wpe_pasteboard_string_vector* out_vector)
    {
        auto& pasteboard = *static_cast<Wayland::Pasteboard*>(data);
        uint64_t length = pasteboard.m_dataDeviceData.dataTypes.size();
        if (!length)
            return;

        out_vector->strings = static_cast<struct wpe_pasteboard_string*>(malloc(sizeof(struct wpe_pasteboard_string) * length));
        out_vector->length = length;
        memset(out_vector->strings, 0, out_vector->length);

        uint64_t i = 0;
        for (auto& entry : pasteboard.m_dataDeviceData.dataTypes)
            wpe_pasteboard_string_initialize(&out_vector->strings[i++], entry.data(), entry.length());
    },
    // get_string
    [](void* data, const char* type, struct wpe_pasteboard_string* out_string)
    {
        auto& pasteboard = *static_cast<Wayland::Pasteboard*>(data);
        if (!std::any_of(pasteboard.m_dataDeviceData.dataTypes.cbegin(), pasteboard.m_dataDeviceData.dataTypes.cend(),
                         [type](std::string str){ return str == type;}))
            return;

        int pipefd[2];
        // FIXME: Should probably handle this error somehow.
        if (pipe2(pipefd, O_CLOEXEC) == -1)
            return;
        wl_data_offer_receive(pasteboard.m_dataDeviceData.data_offer, type, pipefd[1]);
        close(pipefd[1]);

        wl_display_roundtrip(Wayland::Display::singleton().display());

        char buf[1024];
        std::string readString;
        ssize_t length;
        do {
            length = read(pipefd[0], buf, 1024);
            readString.append(buf, length);
        } while (length > 0);
        close(pipefd[0]);

        wpe_pasteboard_string_initialize(out_string, readString.c_str(), readString.length());
    },
    // write
    [](void* data, struct wpe_pasteboard_string_map* map)
    {
        auto& pasteboard = *static_cast<Wayland::Pasteboard*>(data);
        if (pasteboard.m_dataSourceData.data_source)
            wl_data_source_destroy(pasteboard.m_dataSourceData.data_source);

        pasteboard.m_dataSourceData.dataMap.clear();
        for (uint64_t i = 0; i < map->length; ++i) {
            auto& pair = map->pairs[i];
            pasteboard.m_dataSourceData.dataMap.insert(
                { std::string(pair.type.data, pair.type.length), std::string(pair.string.data, pair.string.length) });
        }

        Wayland::Display& display = Wayland::Display::singleton();
        pasteboard.m_dataSourceData.data_source = wl_data_device_manager_create_data_source(display.interfaces().data_device_manager);

        for (auto dataPair : pasteboard.m_dataSourceData.dataMap)
            wl_data_source_offer(pasteboard.m_dataSourceData.data_source, dataPair.first.c_str());

        wl_data_source_add_listener(pasteboard.m_dataSourceData.data_source, &Wayland::g_dataSourceListener, &pasteboard.m_dataSourceData);
        wl_data_device_set_selection(pasteboard.m_dataDevice, pasteboard.m_dataSourceData.data_source, display.singleton().serial());
    },
};

}
