#include "Config.h"

#if WPE_BACKEND(WESTEROS)

#include "ViewBackendWesteros.h"
#include "WesterosViewbackendInput.h"
#include "WesterosViewbackendOutput.h"
#include <stdlib.h>

namespace WPE {

namespace ViewBackend {


ViewBackendWesteros::ViewBackendWesteros()
 : m_client(nullptr)
 , m_input_handler(nullptr)
 , m_output_handler(nullptr)
{
    m_compositor = WstCompositorCreate();
    if (!m_compositor)
        return;

    m_input_handler = new WesterosViewbackendInput();
    m_output_handler = new WesterosViewbackendOutput();
    const char* nestedTargetDisplay = std::getenv("WAYLAND_DISPLAY");
    if (nestedTargetDisplay) {
        fprintf(stderr, "ViewBackendWesteros: running as the nested compositor\n");
        WstCompositorSetIsNested(m_compositor, true);
        WstCompositorSetIsRepeater( m_compositor, true);
        WstCompositorSetNestedDisplayName( m_compositor, nestedTargetDisplay);
        //Register for all the necessary callback before starting the compositor
        m_input_handler->initializeNestedInputHandler(m_compositor, this);
        m_output_handler->initializeNestedOutputHandler(m_compositor, this);
        const char * nestedDisplayName = WstCompositorGetDisplayName(m_compositor);
        setenv("WAYLAND_DISPLAY", nestedDisplayName, 1);
    }

    if (!WstCompositorStart(m_compositor))
    {
        fprintf(stderr, "ViewBackendWesteros: failed to start the compositor: %s\n",
            WstCompositorGetLastErrorDetail(m_compositor));
        WstCompositorDestroy(m_compositor);
        m_compositor = nullptr;
        delete m_input_handler;
        delete m_output_handler;
        m_input_handler = nullptr;
        m_output_handler = nullptr;
    }
}

ViewBackendWesteros::~ViewBackendWesteros()
{
    if(m_input_handler)
        m_input_handler->unregisterInputClient();
    if(m_output_handler)
        m_output_handler->unregisterClient();
    m_client = nullptr;

    if (m_compositor) {
        WstCompositorStop(m_compositor);
        WstCompositorDestroy(m_compositor);
        m_compositor = nullptr;
    }
    if(m_input_handler)
        delete m_input_handler;
    if(m_output_handler)
        delete m_output_handler;
}

void ViewBackendWesteros::setClient(Client* client)
{
    m_client = client;
    if(m_output_handler) {
        m_output_handler->registerClient(m_client);
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
