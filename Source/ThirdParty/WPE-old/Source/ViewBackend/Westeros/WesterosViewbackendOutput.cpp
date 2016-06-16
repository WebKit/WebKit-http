#include "Config.h"

#if WPE_BACKEND(WESTEROS)

#include "ViewBackendWesteros.h"
#include "WesterosViewbackendOutput.h"

#include <cstring>
#include <cassert>
#include <cstring>
#include <linux/input.h>
#include <locale.h>
#include <sys/mman.h>
#include <unistd.h>

namespace WPE {

namespace ViewBackend {

static WstOutputNestedListener output_listener = {
    WesterosViewbackendOutput::handleGeometryCallback,
    WesterosViewbackendOutput::handleModeCallback,
    WesterosViewbackendOutput::handleDoneCallback,
    WesterosViewbackendOutput::handleScaleCallback
};

void WesterosViewbackendOutput::handleGeometryCallback( void *userData, int32_t x, int32_t y, int32_t mmWidth, int32_t mmHeight,
                                                 int32_t subPixel, const char *make, const char *model, int32_t transform )
{
}

void WesterosViewbackendOutput::handleModeCallback( void *userData, uint32_t flags, int32_t width, int32_t height, int32_t refreshRate )
{
    auto& backend_output = *static_cast<WesterosViewbackendOutput*>(userData);
    if (flags == WesterosViewbackendModeCurrent) {
        backend_output.m_width = width;
        backend_output.m_height = height;
        if(backend_output.m_client)
        {
            backend_output.m_client->setSize(backend_output.m_width, backend_output.m_height);
        }
    }
}

void WesterosViewbackendOutput::handleDoneCallback( void *UserData )
{
}

void WesterosViewbackendOutput::handleScaleCallback( void *UserData, int32_t scale )
{
}

WesterosViewbackendOutput::WesterosViewbackendOutput()
 : m_compositor(nullptr)
 , m_viewbackend(nullptr)
 , m_client(nullptr)
 , m_width(0)
 , m_height(0)
{
}

WesterosViewbackendOutput::~WesterosViewbackendOutput()
{
    m_compositor = nullptr;
    m_viewbackend = nullptr;
    m_client = nullptr;
}

void WesterosViewbackendOutput::registerClient(Client* client)
{
    m_client = client;
    if (m_client)
    {
        // If we get registerClient call before handleModeCallback,
        // it will endup setting size to (0,0) and then again back to correct size in handleModeCallback.
        m_client->setSize(m_width, m_height);
    }
}

void WesterosViewbackendOutput::initializeNestedOutputHandler(WstCompositor *compositor, ViewBackendWesteros *backend)
{
    m_compositor = compositor;
    m_viewbackend = backend;

    if (m_compositor && m_viewbackend) {
        if (!WstCompositorSetOutputNestedListener( m_compositor, &output_listener, this )) {
            fprintf(stderr, "ViewBackendWesteros: failed to set output listener: %s\n",
                WstCompositorGetLastErrorDetail(m_compositor));
        }
    }
}

} // namespace ViewBackend

} // namespace WPE

#endif // WPE_BACKEND(WESTEROS)
