
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <WebKit/WKContext.h>
#include <WebKit/WKFramePolicyListener.h>
#include <WebKit/WKPage.h>
#include <WebKit/WKPageGroup.h>
#include <WebKit/WKPageConfigurationRef.h>
#include <WebKit/WKRetainPtr.h>
#include <WebKit/WKString.h>
#include <WebKit/WKURL.h>
#include <WebKit/WKView.h>

#include <cassert>
#include <cstdio>
#include <glib.h>
#include <unordered_map>
#include <wpe-mesa/view-backend-exportable.h>

class EGLConnection {
public:
    EGLConnection();

    bool initialized() const { return m_initialized; }
    EGLDisplay eglDisplay() const { return m_eglDisplay; }

    bool makeCurrent();

    PFNEGLCREATEIMAGEKHRPROC createImage;
    PFNEGLDESTROYIMAGEKHRPROC destroyImage;

private:
    EGLDisplay m_eglDisplay;
    EGLConfig m_eglConfig;
    EGLContext m_eglContext;
    bool m_initialized { false };
};

EGLConnection::EGLConnection()
{
    m_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (m_eglDisplay == EGL_NO_DISPLAY)
        return;

    if (!eglInitialize(m_eglDisplay, nullptr, nullptr))
        return;

    if (!eglBindAPI(EGL_OPENGL_ES_API))
        return;

    static const EGLint configAttributes[13] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 1,
        EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE, 1,
        EGL_ALPHA_SIZE, 1,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    EGLint numConfigs;
    EGLBoolean ret = eglChooseConfig(m_eglDisplay, configAttributes, &m_eglConfig, 1, &numConfigs);
    if (!ret || !numConfigs)
        return;

    static const EGLint contextAttributes[3] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig, EGL_NO_CONTEXT, contextAttributes);
    if (m_eglContext == EGL_NO_CONTEXT)
        return;

    if (!eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, m_eglContext))
        return;

    createImage = reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(eglGetProcAddress("eglCreateImageKHR"));
    destroyImage = reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(eglGetProcAddress("eglDestroyImageKHR"));

    fprintf(stderr, "EGLConnection: initialized!\n");
    m_initialized = true;
}

bool EGLConnection::makeCurrent()
{
    return eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, m_eglContext);
}

class View {
public:
    View(EGLConnection&);

    void load(const char*);

private:
    static struct wpe_mesa_view_backend_exportable_client s_exportableClient;
    static WKPageNavigationClientV0 s_pageNavigationClient;

    EGLConnection& m_eglConnection;

    WKContextRef m_context;
    WKPageConfigurationRef m_pageConfiguration;

    struct wpe_mesa_view_backend_exportable* m_exportable;
    WKViewRef m_view;

    std::unordered_map<uint32_t, std::pair<int32_t, EGLImageKHR>> m_imageMap;
    std::pair<uint32_t, EGLImageKHR> m_lockedImage { 0, nullptr };
};

View::View(EGLConnection& eglConnection)
    : m_eglConnection(eglConnection)
{
    m_context = WKContextCreate();
    m_pageConfiguration = WKPageConfigurationCreate();
    {
        auto pageGroupIdentifier = adoptWK(WKStringCreateWithUTF8CString("WPEPageGroup"));
        auto pageGroup = adoptWK(WKPageGroupCreateWithIdentifier(pageGroupIdentifier.get()));
        WKPageConfigurationSetContext(m_pageConfiguration, m_context);
        WKPageConfigurationSetPageGroup(m_pageConfiguration, pageGroup.get());
    }

    m_exportable = wpe_mesa_view_backend_exportable_create(&s_exportableClient, this);

    auto* backend = wpe_mesa_view_backend_exportable_get_view_backend(m_exportable);
    m_view = WKViewCreateWithViewBackend(backend, m_pageConfiguration);

    WKPageSetPageNavigationClient(WKViewGetPage(m_view), &s_pageNavigationClient.base);
}

void View::load(const char* url)
{
    auto shellURL = adoptWK(WKURLCreateWithUTF8CString(url));
    WKPageLoadURL(WKViewGetPage(m_view), shellURL.get());
}

struct wpe_mesa_view_backend_exportable_client View::s_exportableClient = {
    // export_dma_buf
    [](void* data, struct wpe_mesa_view_backend_exportable_dma_buf_egl_image_data* imageData)
    {
        auto& view = *static_cast<View*>(data);
        fprintf(stderr, "View::s_exportableClient::export_dma_buf() fd %d handle %u (%u,%u) stride %u format %u\n",
            imageData->fd, imageData->handle, imageData->width, imageData->height, imageData->stride, imageData->format);

        auto it = view.m_imageMap.end();
        if (imageData->fd >= 0) {
            assert(view.m_imageMap.find(imageData->handle) == view.m_imageMap.end());

            it = view.m_imageMap.insert({ imageData->handle, { imageData->fd, nullptr }}).first;
        } else {
            assert(view.m_imageMap.find(imageData->handle) != view.m_imageMap.end());
            it = view.m_imageMap.find(imageData->handle);
        }

        assert(it != view.m_imageMap.end());
        uint32_t handle = it->first;
        int32_t fd = it->second.first;

        view.m_eglConnection.makeCurrent();

        EGLint attributes[] = {
            EGL_WIDTH, static_cast<EGLint>(imageData->width),
            EGL_HEIGHT, static_cast<EGLint>(imageData->height),
            EGL_LINUX_DRM_FOURCC_EXT, static_cast<EGLint>(imageData->format),
            EGL_DMA_BUF_PLANE0_FD_EXT, fd,
            EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
            EGL_DMA_BUF_PLANE0_PITCH_EXT, static_cast<EGLint>(imageData->stride),
            EGL_NONE,
        };
        EGLImageKHR image = view.m_eglConnection.createImage(view.m_eglConnection.eglDisplay(), EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr, attributes);
        fprintf(stderr, "\timage %p\n", image);

        wpe_mesa_view_backend_exportable_dispatch_frame_complete(view.m_exportable);
        if (view.m_lockedImage.first) {
            wpe_mesa_view_backend_exportable_dispatch_release_buffer(view.m_exportable, view.m_lockedImage.first);
            view.m_eglConnection.destroyImage(view.m_eglConnection.eglDisplay(), view.m_lockedImage.second);
        }
        view.m_lockedImage = { imageData->handle, image };
    },
};

WKPageNavigationClientV0 View::s_pageNavigationClient = {
    { 0, nullptr },
    // decidePolicyForNavigationAction
    [](WKPageRef, WKNavigationActionRef, WKFramePolicyListenerRef listener, WKTypeRef, const void*) {
        WKFramePolicyListenerUse(listener);
    },
    // decidePolicyForNavigationResponse
    [](WKPageRef, WKNavigationResponseRef, WKFramePolicyListenerRef listener, WKTypeRef, const void*) {
        WKFramePolicyListenerUse(listener);
    },
    nullptr, // decidePolicyForPluginLoad
    nullptr, // didStartProvisionalNavigation
    nullptr, // didReceiveServerRedirectForProvisionalNavigation
    nullptr, // didFailProvisionalNavigation
    nullptr, // didCommitNavigation
    nullptr, // didFinishNavigation
    nullptr, // didFailNavigation
    nullptr, // didFailProvisionalLoadInSubframe
    nullptr, // didFinishDocumentLoad
    nullptr, // didSameDocumentNavigation
    nullptr, // renderingProgressDidChange
    nullptr, // canAuthenticateAgainstProtectionSpace
    nullptr, // didReceiveAuthenticationChallenge
    nullptr, // webProcessDidCrash
    nullptr, // copyWebCryptoMasterKey
    nullptr, // didBeginNavigationGesture
    nullptr, // willEndNavigationGesture
    nullptr, // didEndNavigationGesture
    nullptr, // didRemoveNavigationGestureSnapshot
};

int main(int argc, char* argv[])
{
    EGLConnection eglConnection;
    if (!eglConnection.initialized())
        return 1;

    GMainLoop* loop = g_main_loop_new(g_main_context_default(), FALSE);

    View view(eglConnection);
    view.load("http://www.webkit.org/blog-files/3d-transforms/poster-circle.html");

    g_main_loop_run(loop);

    g_main_loop_unref(loop);
    return 0;
}
