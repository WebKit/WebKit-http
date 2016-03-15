#include "view-backend-drm.h"

#include "gbm-connection.h"
#include <cstdio>
#include <errno.h>
#include <fcntl.h>
#include <gbm.h>
#include <gio/gio.h>
#include <unordered_map>
#include <utility>
#include <xf86drm.h>
#include <xf86drmMode.h>

namespace DRM {

template<typename T>
struct Cleanup {
    ~Cleanup()
    {
        if (valid)
            cleanup();
    }

    T cleanup;
    bool valid;
};

template<typename T>
auto defer(T&& cleanup) -> Cleanup<T>
{
    return Cleanup<T>{ std::forward<T>(cleanup), true };
}

class EventSource {
public:
    static GSourceFuncs sourceFuncs;

    GSource source;
    GPollFD pfd;

    int drmFD;
    drmEventContext eventContext;
};

GSourceFuncs EventSource::sourceFuncs = {
    nullptr, // prepare
    // check
    [](GSource* base) -> gboolean
    {
        auto* source = reinterpret_cast<EventSource*>(base);
        return !!source->pfd.revents;
    },
    // dispatch
    [](GSource* base, GSourceFunc, gpointer) -> gboolean
    {
        auto* source = reinterpret_cast<EventSource*>(base);

        if (source->pfd.revents & G_IO_IN)
            drmHandleEvent(source->drmFD, &source->eventContext);

        if (source->pfd.revents & (G_IO_ERR | G_IO_HUP))
            return FALSE;

        source->pfd.revents = 0;
        return TRUE;
    },
    nullptr, // finalize
    nullptr, // closure_callback
    nullptr, // closure_marshall
};

class ViewBackend;

struct PageFlipHandlerData {
    ViewBackend* backend;
    std::pair<bool, uint32_t> nextFB;
    std::pair<bool, uint32_t> lockedFB;
};

class ViewBackend : public GBM::Host::Client {
public:
    ViewBackend(struct wpe_view_backend*);
    virtual ~ViewBackend();

    void importBufferFd(int) override;
    void commitBuffer(struct buffer_commit*) override;

    struct wpe_view_backend* backend;

    struct {
        int fd { -1 };
        drmModeModeInfo* mode { nullptr };
        std::pair<uint16_t, uint16_t> size;
        uint32_t crtcId { 0 };
        uint32_t connectorId { 0 };
    } m_drm;

    struct {
        int fd { -1 };
        struct gbm_device* device;
    } m_gbm;

    struct {
        GSource* source;
        std::unordered_map<uint32_t, std::pair<struct gbm_bo*, uint32_t>> fbMap;
        PageFlipHandlerData pageFlipData;
    } m_display;

    struct {
        GBM::Host gbmHost;
        int pendingBufferFd { -1 };
    } m_renderer;
};

static void pageFlipHandler(int, unsigned, unsigned, unsigned, void* data)
{
    auto& handlerData = *static_cast<PageFlipHandlerData*>(data);
    if (!handlerData.backend)
        return;

    handlerData.backend->m_renderer.gbmHost.frameComplete();

    auto bufferToRelease = handlerData.lockedFB;
    handlerData.lockedFB = handlerData.nextFB;
    handlerData.nextFB = { false, 0 };

    if (bufferToRelease.first)
        handlerData.backend->m_renderer.gbmHost.releaseBuffer(bufferToRelease.second);
}

ViewBackend::ViewBackend(struct wpe_view_backend* backend)
    : backend(backend)
{
    decltype(m_drm) drm;
    auto drmCleanup = defer(
        [&drm] {
            if (drm.mode)
                drmModeFreeModeInfo(drm.mode);
            if (drm.fd >= 0)
                close(drm.fd);
        });

    // FIXME: This path should be retrieved via udev.
    drm.fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if (drm.fd < 0) {
        fprintf(stderr, "ViewBackend: couldn't connect DRM\n");
        return;
    }

    drm_magic_t magic;
    if (drmGetMagic(drm.fd, &magic) || drmAuthMagic(drm.fd, magic))
        return;

    decltype(m_gbm) gbm;
    auto gbmCleanup = defer(
        [&gbm] {
            if (gbm.device)
                gbm_device_destroy(gbm.device);
            if (gbm.fd >= 0)
                close(gbm.fd);
        });

    gbm.device = gbm_create_device(drm.fd);
    if (!gbm.device)
        return;

    drmModeRes* resources = drmModeGetResources(drm.fd);
    if (!resources)
        return;
    auto drmResourcesCleanup = defer([&resources] { drmModeFreeResources(resources); });

    drmModeConnector* connector = nullptr;
    for (int i = 0; i < resources->count_connectors; ++i) {
        connector = drmModeGetConnector(drm.fd, resources->connectors[i]);
        if (connector->connection == DRM_MODE_CONNECTED)
            break;

        drmModeFreeConnector(connector);
    }

    if (!connector)
        return;
    auto drmConnectorCleanup = defer([&connector] { drmModeFreeConnector(connector); });

    int area = 0;
    for (int i = 0; i < connector->count_modes; ++i) {
        drmModeModeInfo* currentMode = &connector->modes[i];
        int modeArea = currentMode->hdisplay * currentMode->vdisplay;
        if (modeArea > area) {
            drm.mode = currentMode;
            drm.size = { currentMode->hdisplay, currentMode->vdisplay };
            area = modeArea;
        }
    }

    if (!drm.mode)
        return;

    drmModeEncoder* encoder = nullptr;
    for (int i = 0; i < resources->count_encoders; ++i) {
        encoder = drmModeGetEncoder(drm.fd, resources->encoders[i]);
        if (encoder->encoder_id == connector->encoder_id)
            break;

        drmModeFreeEncoder(encoder);
    }

    if (!encoder)
        return;
    auto drmEncoderCleanup = defer([&encoder] { drmModeFreeEncoder(encoder); });

    drm.crtcId = encoder->crtc_id;
    drm.connectorId = connector->connector_id;

    m_drm = drm;
    drmCleanup.valid = false;
    m_gbm = gbm;
    gbmCleanup.valid = false;
    fprintf(stderr, "ViewBackend: successfully initialized DRM.\n");

    m_display.source = g_source_new(&DRM::EventSource::sourceFuncs, sizeof(DRM::EventSource));
    auto* displaySource = reinterpret_cast<DRM::EventSource*>(m_display.source);
    displaySource->drmFD = m_drm.fd;
    displaySource->eventContext = {
        DRM_EVENT_CONTEXT_VERSION,
        nullptr,
        &pageFlipHandler
    };

    displaySource->pfd.fd = m_drm.fd;
    displaySource->pfd.events = G_IO_IN | G_IO_ERR | G_IO_HUP;
    displaySource->pfd.revents = 0;
    g_source_add_poll(m_display.source, &displaySource->pfd);

    g_source_set_name(m_display.source, "[WPE] DRM");
    g_source_set_priority(m_display.source, G_PRIORITY_HIGH + 30);
    g_source_set_can_recurse(m_display.source, TRUE);
    g_source_attach(m_display.source, g_main_context_get_thread_default());

    m_display.pageFlipData.backend = this;

    m_renderer.gbmHost.initialize(*this);
}

ViewBackend::~ViewBackend()
{
    m_renderer.gbmHost.deinitialize();

    m_display.fbMap = { };
    if (m_display.source)
        g_source_unref(m_display.source);
    m_display.source = nullptr;
    m_display.pageFlipData = { nullptr, { }, { } };

    if (m_gbm.device)
        gbm_device_destroy(m_gbm.device);
    m_gbm = { };

    if (m_drm.mode)
        drmModeFreeModeInfo(m_drm.mode);
    if (m_drm.fd >= 0)
        close(m_drm.fd);
    m_drm = { };
}

void ViewBackend::importBufferFd(int fd)
{
    if (m_renderer.pendingBufferFd != -1)
        close(m_renderer.pendingBufferFd);
    m_renderer.pendingBufferFd = fd;
}

void ViewBackend::commitBuffer(struct buffer_commit* commit)
{
    uint32_t fbID = 0;

    if (m_renderer.pendingBufferFd >= 0) {
        int fd = m_renderer.pendingBufferFd;
        m_renderer.pendingBufferFd = -1;

        assert(m_display.fbMap.find(commit->handle) == m_display.fbMap.end());

        struct gbm_import_fd_data fdData = { fd, commit->width, commit->height, commit->stride, commit->format };
        struct gbm_bo* bo = gbm_bo_import(m_gbm.device, GBM_BO_IMPORT_FD, &fdData, GBM_BO_USE_SCANOUT);
        uint32_t primeHandle = gbm_bo_get_handle(bo).u32;

        int ret = drmModeAddFB(m_drm.fd, gbm_bo_get_width(bo), gbm_bo_get_height(bo),
            24, 32, gbm_bo_get_stride(bo), primeHandle, &fbID);
        if (ret) {
            fprintf(stderr, "ViewBackend: failed to add FB: %s, fbID %d\n", strerror(errno), fbID);
            return;
        }

        m_display.fbMap.insert({ commit->handle, { bo, fbID } });
        m_display.pageFlipData.nextFB = { true, commit->handle };
    } else {
        auto it = m_display.fbMap.find(commit->handle);
        assert(it != m_fbMap.end());

        fbID = it->second.second;
        m_display.pageFlipData.nextFB = { true, commit->handle };
    }

    int ret = drmModePageFlip(m_drm.fd, m_drm.crtcId, fbID, DRM_MODE_PAGE_FLIP_EVENT, &m_display.pageFlipData);
    if (ret)
        fprintf(stderr, "ViewBackend: failed to queue page flip\n");
}

} // namespace DRM

extern "C" {

struct wpe_view_backend_interface drm_view_backend_interface = {
    // create
    [](struct wpe_view_backend* backend) -> void*
    {
        fprintf(stderr, "drm_view_backend_interface::create()\n");

        return new DRM::ViewBackend(backend);
    },
    // destroy
    [](void* data)
    {
        fprintf(stderr, "drm_view_backend_interface::destroy()\n");

        auto* backend = static_cast<DRM::ViewBackend*>(data);
        delete backend;
    },
    // initialize
    [](void* data)
    {
        fprintf(stderr, "drm_view_backend_interface::initialize()\n");

        auto* backend = static_cast<DRM::ViewBackend*>(data);
        wpe_view_backend_dispatch_set_size(backend->backend,
            backend->m_drm.size.first, backend->m_drm.size.second);
    },
    // get_renderer_host_fd
    [](void* data) -> int
    {
        fprintf(stderr, "drm_view_backend_interface::get_renderer_host_fd()\n");

        auto* backend = static_cast<DRM::ViewBackend*>(data);
        return backend->m_renderer.gbmHost.releaseClientFD();
    },
};

}
