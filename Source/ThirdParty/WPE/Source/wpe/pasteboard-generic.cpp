#include "pasteboard-private.h"

#include <cstdlib>
#include <map>
#include <string>

namespace Generic {
using Pasteboard = std::map<std::string, std::string>;
}

extern "C" {

const struct wpe_pasteboard_interface generic_pasteboard_interface = {
    // initialize
    [](struct wpe_pasteboard*) -> void*
    {
        return new Generic::Pasteboard;
    },
    // get_types
    [](void* data, struct wpe_pasteboard_string_vector* out_vector)
    {
        auto& pasteboard = *static_cast<Generic::Pasteboard*>(data);
        uint64_t length = pasteboard.size();
        if (!length)
            return;

        out_vector->strings = static_cast<struct wpe_pasteboard_string*>(malloc(sizeof(struct wpe_pasteboard_string) * length));
        out_vector->length = length;
        memset(out_vector->strings, 0, out_vector->length);

        uint64_t i = 0;
        for (auto& entry : pasteboard)
            wpe_pasteboard_string_initialize(&out_vector->strings[i++], entry.first.c_str(), entry.first.length());
    },
    // get_string
    [](void* data, const char* type, struct wpe_pasteboard_string* out_string)
    {
        auto& pasteboard = *static_cast<Generic::Pasteboard*>(data);

        std::string typeString(type);
        auto it = pasteboard.find(typeString);
        if (it == pasteboard.end())
            return;

        wpe_pasteboard_string_initialize(out_string, it->second.c_str(), it->second.length());
    },
    // write
    [](void* data, struct wpe_pasteboard_string_map* map)
    {
        auto& pasteboard = *static_cast<Generic::Pasteboard*>(data);

        pasteboard.clear();
        for (uint64_t i = 0; i < map->length; ++i) {
            auto& pair = map->pairs[i];
            pasteboard.insert({ std::string(pair.type.data, pair.type.length), std::string(pair.string.data, pair.string.length) });
        }
    },
};

}
