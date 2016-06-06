#include "Config.h"
#include "BufferFactoryBCMNexus.h"

#if WPE_BUFFER_MANAGEMENT(BCM_NEXUS)

#include "BufferDataBCMNexus.h"
#include "WaylandDisplay.h"
#include "nsc-client-protocol.h"
#include <string.h>

#include <refsw/nexus_config.h>
#include <refsw/nexus_platform.h>
#include <refsw/nexus_display.h>
#include <refsw/nexus_core_utils.h>
#include <refsw/default_nexus.h>
#include <refsw/nxclient.h>

namespace WPE {

namespace Graphics {

static const struct wl_nsc_listener g_nscListener = {
    // handle_standby_status
    [](void*, struct wl_nsc*, struct wl_array*) { },
    // connectID
    [](void*, struct wl_nsc*, uint32_t) { },
    // composition
    [](void*, struct wl_nsc*, struct wl_array*) { },
    // authenticated
    [](void* data, struct wl_nsc*, const char* certData, uint32_t certLength)
    {
        auto& nscData = *static_cast<BufferFactoryBCMNexus::NSCData*>(data);
        nscData.authenticationData = { certData, certLength };
    },
    // clientID_created
    [](void* data, struct wl_nsc*, struct wl_array* clientIDArray)
    {
        auto& nscData = *static_cast<BufferFactoryBCMNexus::NSCData*>(data);
        nscData.clientID = static_cast<uint32_t*>(clientIDArray->data)[0];
    },
    // display_geometry
    [](void* data, struct wl_nsc*, uint32_t width, uint32_t height)
    {
        auto& nscData = *static_cast<BufferFactoryBCMNexus::NSCData*>(data);
        nscData.width = width;
        nscData.height = height;
    },
    // audiosettings
    [](void*, struct wl_nsc*, struct wl_array*) { },
    // displaystatus
    [](void*, struct wl_nsc*, struct wl_array*) { },
    // picturequalitysettings
    [](void*, struct wl_nsc*, struct wl_array*) { },
    // displaysettings
    [](void*, struct wl_nsc*, struct wl_array*) { },
};

BufferFactoryBCMNexus::BufferFactoryBCMNexus()
    : m_display(ViewBackend::WaylandDisplay::singleton())
{
    wl_nsc_add_listener(m_display.interfaces().nsc, &g_nscListener, &m_nscData);

    wl_nsc_get_display_geometry(m_display.interfaces().nsc);
    wl_display_roundtrip(m_display.display());
}

BufferFactoryBCMNexus::~BufferFactoryBCMNexus() = default;

std::pair<bool, std::pair<uint32_t, uint32_t>> BufferFactoryBCMNexus::preferredSize()
{
    return { true, { m_nscData.width, m_nscData.height } };
}

std::pair<const uint8_t*, size_t> BufferFactoryBCMNexus::authenticate()
{
    wl_nsc_authenticate(m_display.interfaces().nsc);
    wl_display_roundtrip(m_display.display());

    return { reinterpret_cast<const uint8_t*>(m_nscData.authenticationData.data()), m_nscData.authenticationData.size() };
}

uint32_t BufferFactoryBCMNexus::constructRenderingTarget(uint32_t width, uint32_t height)
{
    wl_nsc_request_clientID(m_display.interfaces().nsc, WL_NSC_CLIENT_SURFACE);
    wl_display_roundtrip(m_display.display());

    wl_nsc_create_window(m_display.interfaces().nsc, m_nscData.clientID, 0, width, height);
    wl_display_roundtrip(m_display.display());

    return m_nscData.clientID;
}

std::pair<bool, std::pair<uint32_t, struct wl_buffer*>> BufferFactoryBCMNexus::createBuffer(int, const uint8_t*, size_t)
{
    if (!m_nscData.buffer) {
        m_nscData.buffer = wl_nsc_create_buffer(m_display.interfaces().nsc, m_nscData.clientID, m_nscData.width, m_nscData.height);
        wl_display_roundtrip(m_display.display());

        return { true, { 0, m_nscData.buffer } };
    }

    return { true, { 0, nullptr } };
}

void BufferFactoryBCMNexus::destroyBuffer(uint32_t, struct wl_buffer*)
{
}

} // namespace Graphics

} // namespace WPE

#endif // WPE_BUFFER_MANAGEMENT(BCM_NEXUS)
