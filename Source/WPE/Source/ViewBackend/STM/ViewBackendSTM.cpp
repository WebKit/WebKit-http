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

} // namespace ViewBackend

} // namespace WPE

#endif // WPE_BACKEND(STM)
