#include "Config.h"

#if WPE_BACKEND(WESTEROS)

#include "ViewBackendWesteros.h"
#include "WesterosViewbackendInput.h"
#include <stdlib.h>

namespace WPE {

namespace ViewBackend {


ViewBackendWesteros::ViewBackendWesteros()
 : m_client(nullptr)
 , m_input_handler(nullptr)
{
    m_compositor = WstCompositorCreate();
    if (!m_compositor)
        return;

    m_input_handler = new WesterosViewbackendInput();
    const char* nestedTargetDisplay = std::getenv("WAYLAND_DISPLAY");
    if (nestedTargetDisplay) {
        fprintf(stderr, "ViewBackendWesteros: running as the nested compositor\n");
        WstCompositorSetIsNested(m_compositor, true);
        WstCompositorSetIsRepeater( m_compositor, true);
        WstCompositorSetNestedDisplayName( m_compositor, nestedTargetDisplay);
        //Register for all the necessary callback before starting the compositor
        m_input_handler->initializeNestedInputHandler(m_compositor, this);
        const char * nestedDisplayName = WstCompositorGetDisplayName(m_compositor);
        setenv("WPE_WESTEROS_NESTED_DISPLAY", nestedDisplayName, 1);
    }

    if (!WstCompositorStart(m_compositor))
    {
        fprintf(stderr, "ViewBackendWesteros: failed to start the compositor: %s\n",
            WstCompositorGetLastErrorDetail(m_compositor));
        WstCompositorDestroy(m_compositor);
        m_compositor = nullptr;
    }
}

ViewBackendWesteros::~ViewBackendWesteros()
{
    if(m_input_handler)
        m_input_handler->unregisterInputClient();
    m_client = nullptr;

    if (!m_compositor)
        return;

    WstCompositorStop(m_compositor);
    WstCompositorDestroy(m_compositor);
    m_compositor = nullptr;
    if(m_input_handler)
        delete m_input_handler;
}

void ViewBackendWesteros::setClient(Client* client)
{
    m_client = client;
    if(m_client && m_compositor) {
        uint32_t width, height;
        WstCompositorGetNestedSize( m_compositor, &width, &height );
        m_client->setSize(width, height);
    }
}

uint32_t ViewBackendWesteros::constructRenderingTarget(uint32_t, uint32_t)
{
    return 0;
}

void ViewBackendWesteros::commitBuffer(int, const uint8_t*, size_t)
{
    if (m_client)
        m_client->frameComplete();
}

void ViewBackendWesteros::destroyBuffer(uint32_t)
{
}

void ViewBackendWesteros::setInputClient(Input::Client* client)
{
    if(m_input_handler)
        m_input_handler->registerInputClient(client);
}

} // namespace ViewBackend

} // namespace WPE

#endif // WPE_BACKEND(WESTEROS)
