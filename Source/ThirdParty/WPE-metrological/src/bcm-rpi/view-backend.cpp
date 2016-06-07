#include <wpe/view-backend.h>

#include "LibinputServer.h"
#include "ipc.h"
#include "ipc-rpi.h"
#include <bcm_host.h>
#include <cstdio>
#include <sys/eventfd.h>
#include <sys/time.h>

namespace BCMRPi {

struct ViewBackend;

class UpdateSource {
public:
    static GSourceFuncs sourceFuncs;

    GSource source;
    GPollFD pfd;
    ViewBackend* backend;
};

struct ViewBackend : public IPC::Host::Handler {
    ViewBackend(struct wpe_view_backend*);
    virtual ~ViewBackend();

    void initialize();
    void initializeRenderingTarget();
    void initializeInput();

    // IPC::Host::Handler
    void handleFd(int) override;
    void handleMessage(char*, size_t) override;

    void commitBuffer(uint32_t, uint32_t, uint32_t);
    void handleUpdate();

    struct wpe_view_backend* backend;
    IPC::Host ipcHost;

    DISPMANX_DISPLAY_HANDLE_T displayHandle { DISPMANX_NO_HANDLE };
    DISPMANX_ELEMENT_HANDLE_T elementHandle { DISPMANX_NO_HANDLE };

    int updateFd { -1 };
    GSource* updateSource;

    uint32_t width { 0 };
    uint32_t height { 0 };
};

ViewBackend::ViewBackend(struct wpe_view_backend* backend)
    : backend(backend)
{
    ipcHost.initialize(*this);

    bcm_host_init();
    displayHandle = vc_dispmanx_display_open(0);
    graphics_get_display_size(DISPMANX_ID_HDMI, &width, &height);

    updateFd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (updateFd == -1) {
        fprintf(stderr, "ViewBackend: failed to create the update eventfd\n");
        return;
    }

    updateSource = g_source_new(&UpdateSource::sourceFuncs, sizeof(UpdateSource));
    auto& source = *reinterpret_cast<UpdateSource*>(updateSource);
    source.backend = this;

    source.pfd.fd = updateFd;
    source.pfd.events = G_IO_IN | G_IO_ERR | G_IO_HUP;
    source.pfd.revents = 0;
    g_source_add_poll(updateSource, &source.pfd);

    g_source_set_name(updateSource, "[WPE] BCMRPi update");
    g_source_set_priority(updateSource, G_PRIORITY_HIGH + 30);
    g_source_set_can_recurse(updateSource, TRUE);
    g_source_attach(updateSource, g_main_context_get_thread_default());
}

ViewBackend::~ViewBackend()
{
    ipcHost.deinitialize();

    if (updateSource)
        g_source_destroy(updateSource);
    if (updateFd != -1)
        close(updateFd);

    vc_dispmanx_display_close(displayHandle);
    bcm_host_deinit();
}

void ViewBackend::initialize()
{
    initializeRenderingTarget();
    initializeInput();
}

void ViewBackend::initializeRenderingTarget()
{
    static VC_DISPMANX_ALPHA_T alpha = {
        static_cast<DISPMANX_FLAGS_ALPHA_T>(DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS),
        255, 0
    };

    if (elementHandle != DISPMANX_NO_HANDLE)
        return;

    DISPMANX_UPDATE_HANDLE_T updateHandle = vc_dispmanx_update_start(0);

    VC_RECT_T srcRect, destRect;
    vc_dispmanx_rect_set(&srcRect, 0, 0, width << 16, height << 16);
    vc_dispmanx_rect_set(&destRect, 0, 0, width, height);

    elementHandle = vc_dispmanx_element_add(updateHandle, displayHandle, 0,
        &destRect, DISPMANX_NO_HANDLE, &srcRect, DISPMANX_PROTECTION_NONE,
        &alpha, nullptr, DISPMANX_NO_ROTATE);

    vc_dispmanx_update_submit_sync(updateHandle);

    wpe_view_backend_dispatch_set_size(backend, width, height);

    IPC::BCMRPi::Message message;
    IPC::BCMRPi::TargetConstruction::construct(message, elementHandle, width, height);
    ipcHost.send(IPC::BCMRPi::messageData(message), IPC::BCMRPi::messageSize);
}

void ViewBackend::initializeInput()
{
    if (std::getenv("WPE_BCMRPI_TOUCH"))
        WPE::LibinputServer::singleton().setHandleTouchEvents(true);

#if 0
    if (std::getenv("WPE_BCMRPI_CURSOR")) {
        m_cursor.reset(new Cursor(client, m_displayHandle, m_width, m_height));
        client = m_cursor.get();
        WPE::LibinputServer::singleton().setHandlePointerEvents(true);
    }
#endif
    WPE::LibinputServer::singleton().setPointerBounds(width, height);

    WPE::LibinputServer::singleton().setBackend(backend);
}

void ViewBackend::handleFd(int)
{
}

void ViewBackend::handleMessage(char* data, size_t size)
{
    if (size != IPC::BCMRPi::messageSize)
        return;

    auto& message = IPC::BCMRPi::asMessage(data);
    switch (message.messageCode) {
    case IPC::BCMRPi::BufferCommit::code:
    {
        auto& bufferCommit = IPC::BCMRPi::BufferCommit::cast(message);
        commitBuffer(bufferCommit.handle, bufferCommit.width, bufferCommit.height);
        break;
    }
    default:
        fprintf(stderr, "ViewBackend: unhandled message\n");
    }
}

void ViewBackend::commitBuffer(uint32_t handle, uint32_t width, uint32_t height)
{
    if (handle != elementHandle || width != this->width || height != this->height)
        return;

    DISPMANX_UPDATE_HANDLE_T updateHandle = vc_dispmanx_update_start(0);

    VC_RECT_T srcRect, destRect;
    vc_dispmanx_rect_set(&srcRect, 0, 0, width << 16, height << 16);
    vc_dispmanx_rect_set(&destRect, 0, 0, width, height);

    vc_dispmanx_element_change_attributes(updateHandle, elementHandle, 1 << 3 | 1 << 2, 0, 0, &destRect, &srcRect, 0, DISPMANX_NO_ROTATE);

    vc_dispmanx_update_submit(updateHandle,
        [](DISPMANX_UPDATE_HANDLE_T, void* data)
        {
            auto& backend = *static_cast<ViewBackend*>(data);

            struct timeval tv;
            gettimeofday(&tv, nullptr);
            uint64_t time = tv.tv_sec * 1000 + tv.tv_usec / 1000;

            ssize_t ret = write(backend.updateFd, &time, sizeof(time));
            if (ret != sizeof(time))
                fprintf(stderr, "ViewBackend: failed to write to the update eventfd\n");
        },
        this);
}

void ViewBackend::handleUpdate()
{
    uint64_t time;
    ssize_t ret = read(updateFd, &time, sizeof(time));
    if (ret != sizeof(time))
        return;

    IPC::BCMRPi::Message message;
    IPC::BCMRPi::FrameComplete::construct(message);
    ipcHost.send(IPC::BCMRPi::messageData(message), IPC::BCMRPi::messageSize);
}

GSourceFuncs UpdateSource::sourceFuncs = {
    // prepare
    [](GSource*, gint* timeout) -> gboolean
    {
        *timeout = -1;
        return FALSE;
    },
    // check
    [](GSource* base) -> gboolean
    {
        auto& source = *reinterpret_cast<UpdateSource*>(base);
        return !!source.pfd.revents;
    },
    // dispatch
    [](GSource* base, GSourceFunc, gpointer) -> gboolean
    {
        auto& source = *reinterpret_cast<UpdateSource*>(base);

        if (source.pfd.revents & (G_IO_ERR | G_IO_HUP))
            return FALSE;

        if (source.pfd.revents & G_IO_IN)
            source.backend->handleUpdate();

        source.pfd.revents = 0;
        return TRUE;
    },
    nullptr, // finalize
    nullptr, // closure_callback
    nullptr, // closure_marshall
};

} // namespace BCMRPi

extern "C" {

struct wpe_view_backend_interface bcm_rpi_view_backend_interface = {
    // create
    [](void*, struct wpe_view_backend* backend) -> void*
    {
        return new BCMRPi::ViewBackend(backend);
    },
    // destroy
    [](void* data)
    {
        auto* backend = static_cast<BCMRPi::ViewBackend*>(data);
        delete backend;
    },
    // initialize
    [](void* data)
    {
        auto& backend = *static_cast<BCMRPi::ViewBackend*>(data);
        backend.initialize();
    },
    // get_renderer_host_fd
    [](void* data) -> int
    {
        auto& backend = *static_cast<BCMRPi::ViewBackend*>(data);
        return backend.ipcHost.releaseClientFD();
    },
};

}
