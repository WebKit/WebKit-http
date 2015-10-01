#include "Config.h"
#include "ViewBackendBCMRPi.h"

#if WPE_BACKEND(BCM_RPI)

#include "LibinputServer.h"

namespace WPE {

namespace ViewBackend {

ViewBackendBCMRPi::ViewBackendBCMRPi()
    : m_elementHandle(DISPMANX_NO_HANDLE)
    , m_width(0)
    , m_height(0)
{
    bcm_host_init();
    m_displayHandle = vc_dispmanx_display_open(0);
}

ViewBackendBCMRPi::~ViewBackendBCMRPi()
{
    LibinputServer::singleton().setClient(nullptr);
}

void ViewBackendBCMRPi::setClient(Client* client)
{
    m_client = client;
}

uint32_t ViewBackendBCMRPi::createBCMElement(int32_t width, int32_t height)
{
    static VC_DISPMANX_ALPHA_T alpha = {
        static_cast<DISPMANX_FLAGS_ALPHA_T>(DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS),
        255, 0
    };

    if (m_elementHandle != DISPMANX_NO_HANDLE)
        return 0;

    m_width = std::max(width, 0);
    m_height = std::max(height, 0);

    DISPMANX_UPDATE_HANDLE_T updateHandle = vc_dispmanx_update_start(0);

    VC_RECT_T srcRect, destRect;
    vc_dispmanx_rect_set(&srcRect, 0, 0, m_width << 16, m_height << 16);
    vc_dispmanx_rect_set(&destRect, 0, 0, m_width, m_height);

    m_elementHandle = vc_dispmanx_element_add(updateHandle, m_displayHandle, 0,
        &destRect, DISPMANX_NO_HANDLE, &srcRect, DISPMANX_PROTECTION_NONE,
        &alpha, nullptr, DISPMANX_NO_ROTATE);

    vc_dispmanx_update_submit_sync(updateHandle);
    return m_elementHandle;
}

void ViewBackendBCMRPi::commitBCMBuffer(uint32_t elementHandle, uint32_t width, uint32_t height)
{
    DISPMANX_UPDATE_HANDLE_T updateHandle = vc_dispmanx_update_start(0);

    VC_RECT_T destRect;
    vc_dispmanx_rect_set(&destRect, 0, 0, m_width, m_height);
    vc_dispmanx_element_modified(updateHandle, m_elementHandle, &destRect);

    vc_dispmanx_update_submit_sync(updateHandle);

    if (m_client)
        m_client->frameComplete();
}

void ViewBackendBCMRPi::setInputClient(Input::Client* client)
{
    LibinputServer::singleton().setClient(client);
}

} // namespace ViewBackend

} // namespace WPE

#endif // WPE_BACKEND(BCM_RPI)
