
#include <cairo.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
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
#include <wpe-mesa/view-backend-exportable-dma-buf.h>

class GLContext {
public:
    GLContext();

    bool initialized() const { return m_initialized; }
    EGLDisplay eglDisplay() const { return m_eglDisplay; }

    bool makeCurrent();
    bool readImage(EGLImageKHR, uint32_t, uint32_t, uint8_t*);

    PFNEGLCREATEIMAGEKHRPROC createImage;
    PFNEGLDESTROYIMAGEKHRPROC destroyImage;
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC imageTargetTexture2DOES;

private:
    EGLDisplay m_eglDisplay;
    EGLConfig m_eglConfig;
    EGLContext m_eglContext;
    bool m_initialized { false };
};

GLContext::GLContext()
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
    imageTargetTexture2DOES = reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(eglGetProcAddress("glEGLImageTargetTexture2DOES"));

    fprintf(stderr, "GLContext: initialized!\n");
    m_initialized = true;
}

bool GLContext::makeCurrent()
{
    return eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, m_eglContext);
}

bool GLContext::readImage(EGLImageKHR image, uint32_t width, uint32_t height, uint8_t* buffer)
{
    makeCurrent();

    GLuint imageTexture;
    glGenTextures(1, &imageTexture);
    glBindTexture(GL_TEXTURE_2D, imageTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, nullptr);

    imageTargetTexture2DOES(GL_TEXTURE_2D, image);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLuint imageFramebuffer;
    glGenFramebuffers(1, &imageFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, imageFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, imageTexture, 0);

    glFlush();
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &imageFramebuffer);
        glDeleteTextures(1, &imageTexture);
        return false;
    }

    glReadPixels(0, 0, width, height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, buffer);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &imageFramebuffer);
    glDeleteTextures(1, &imageTexture);
    return true;
}

class View {
public:
    View(GLContext&);

    void load(const char*);
    void snapshot();

private:
    static struct wpe_mesa_view_backend_exportable_dma_buf_client s_exportableClient;
    static WKPageNavigationClientV0 s_pageNavigationClient;

    GLContext& m_glContext;

    WKContextRef m_context;
    WKPageConfigurationRef m_pageConfiguration;

    struct wpe_mesa_view_backend_exportable_dma_buf* m_exportable;
    WKViewRef m_view;

    std::unordered_map<uint32_t, std::pair<int32_t, EGLImageKHR>> m_imageMap;
    std::pair<uint32_t, std::tuple<EGLImageKHR, uint32_t, uint32_t>> m_pendingImage { };
    std::pair<uint32_t, std::tuple<EGLImageKHR, uint32_t, uint32_t>> m_lockedImage { };

    void performUpdate();
    GSource* m_updateSource;
    gint64 m_frameRate { G_USEC_PER_SEC / 60 };
};

View::View(GLContext& glContext)
    : m_glContext(glContext)
{
    m_context = WKContextCreate();
    m_pageConfiguration = WKPageConfigurationCreate();
    {
        auto pageGroupIdentifier = adoptWK(WKStringCreateWithUTF8CString("WPEPageGroup"));
        auto pageGroup = adoptWK(WKPageGroupCreateWithIdentifier(pageGroupIdentifier.get()));
        WKPageConfigurationSetContext(m_pageConfiguration, m_context);
        WKPageConfigurationSetPageGroup(m_pageConfiguration, pageGroup.get());
    }

    m_exportable = wpe_mesa_view_backend_exportable_dma_buf_create(&s_exportableClient, this);

    auto* backend = wpe_mesa_view_backend_exportable_dma_buf_get_view_backend(m_exportable);
    m_view = WKViewCreateWithViewBackend(backend, m_pageConfiguration);

    WKPageSetPageNavigationClient(WKViewGetPage(m_view), &s_pageNavigationClient.base);

    m_updateSource = g_timeout_source_new(m_frameRate / 1000);
    g_source_set_callback(m_updateSource,
        [](gpointer data) -> gboolean {
            auto& view = *static_cast<View*>(data);
            view.performUpdate();
            return TRUE;
        }, this, nullptr);
    g_source_attach(m_updateSource, g_main_context_default());
}

void View::load(const char* url)
{
    auto shellURL = adoptWK(WKURLCreateWithUTF8CString(url));
    WKPageLoadURL(WKViewGetPage(m_view), shellURL.get());
}

void View::snapshot()
{
    EGLImageKHR image = std::get<0>(m_lockedImage.second);
    if (!image)
        return;

    uint32_t width = std::get<1>(m_lockedImage.second);
    uint32_t height = std::get<2>(m_lockedImage.second);

    uint8_t* buffer = new uint8_t[4 * width * height];
    if (!m_glContext.readImage(image, width, height, buffer)) {
        fprintf(stderr, "View::snapshot(): failed\n");
        delete[] buffer;
        return;
    }

    cairo_surface_t* imageSurface = cairo_image_surface_create_for_data(buffer,
        CAIRO_FORMAT_ARGB32, width, height, width * 4);
    cairo_surface_write_to_png(imageSurface, "/tmp/wpe-snapshot.png");
    cairo_surface_destroy(imageSurface);

    delete[] buffer;
}

void View::performUpdate()
{
    if (!m_pendingImage.first)
        return;

    wpe_mesa_view_backend_exportable_dma_buf_dispatch_frame_complete(m_exportable);
    if (m_lockedImage.first) {
        wpe_mesa_view_backend_exportable_dma_buf_dispatch_release_buffer(m_exportable, m_lockedImage.first);
        m_glContext.destroyImage(m_glContext.eglDisplay(), std::get<0>(m_lockedImage.second));
    }

    m_lockedImage = m_pendingImage;
    m_pendingImage = std::pair<uint32_t, std::tuple<EGLImageKHR, uint32_t, uint32_t>> { };
}

struct wpe_mesa_view_backend_exportable_dma_buf_client View::s_exportableClient = {
    // export_dma_buf
    [](void* data, struct wpe_mesa_view_backend_exportable_dma_buf_data* imageData)
    {
        auto& view = *static_cast<View*>(data);

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

        view.m_glContext.makeCurrent();

        EGLint attributes[] = {
            EGL_WIDTH, static_cast<EGLint>(imageData->width),
            EGL_HEIGHT, static_cast<EGLint>(imageData->height),
            EGL_LINUX_DRM_FOURCC_EXT, static_cast<EGLint>(imageData->format),
            EGL_DMA_BUF_PLANE0_FD_EXT, fd,
            EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
            EGL_DMA_BUF_PLANE0_PITCH_EXT, static_cast<EGLint>(imageData->stride),
            EGL_NONE,
        };
        EGLImageKHR image = view.m_glContext.createImage(view.m_glContext.eglDisplay(), EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr, attributes);
        view.m_pendingImage = { imageData->handle, std::make_tuple(image, imageData->width, imageData->height) };
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
    GLContext glContext;
    if (!glContext.initialized())
        return 1;

    GMainLoop* loop = g_main_loop_new(g_main_context_default(), FALSE);

    View view(glContext);
    view.load("http://www.webkit.org/blog-files/3d-transforms/poster-circle.html");

    g_timeout_add(10 * 1000,
        [](gpointer data) -> gboolean {
            auto& view = *static_cast<View*>(data);
            view.snapshot();
            return TRUE;
        }, &view);

    g_main_loop_run(loop);

    g_main_loop_unref(loop);
    return 0;
}
