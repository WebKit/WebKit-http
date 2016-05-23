#include "Config.h"
#include "ViewBackendSTM.h"
#include <WPE/Input/Handling.h>

#if WPE_BACKEND(STM)

namespace WPE {

namespace ViewBackend {

ViewBackendSTM::ViewBackendSTM()
{
}

ViewBackendSTM::~ViewBackendSTM()
{
}

void ViewBackendSTM::setClient(Client* client)
{
    m_client = client;
}

uint32_t ViewBackendSTM::constructRenderingTarget(uint32_t, uint32_t)
{
    return 0;
}

void ViewBackendSTM::commitBuffer(int, const uint8_t*, size_t)
{
    if (m_client)
        m_client->frameComplete();
}

void ViewBackendSTM::destroyBuffer(uint32_t)
{
}

void ViewBackendSTM::setInputClient(Input::Client* client)
{
    m_input_client = client;
}

void ViewBackendSTM::handleKeyboardEvent(const Input::KeyboardEvent& event)
{
    if(m_input_client)
        m_input_client->handleKeyboardEvent({event.time, event.keyCode, event.unicode, event.pressed, event.modifiers});
}

void ViewBackendSTM::handlePointerEvent(const Input::PointerEvent& event)
{
    if(m_input_client)
        m_input_client->handlePointerEvent({event.type, event.time, event.x, event.y, event.button, event.state});
}

void ViewBackendSTM::handleAxisEvent(const Input::AxisEvent& event)
{
    if(m_input_client)
        m_input_client->handleAxisEvent({event.type, event.time, event.x, event.y, event.axis, event.value});
}

} // namespace ViewBackend

} // namespace WPE

#endif // WPE_BACKEND(STM)
