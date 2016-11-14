#include <WebKit/WKContext.h>
#include <WebKit/WKFramePolicyListener.h>
#include <WebKit/WKPageGroup.h>
#include <WebKit/WKPageConfigurationRef.h>
#include <WebKit/WKPage.h>
#include <WebKit/WKRetainPtr.h>
#include <WebKit/WKString.h>
#include <WebKit/WKURL.h>
#include <WebKit/WKView.h>
#include <glib.h>
#include <wpe-mesa/view-backend-exportable-dma-buf.h>

#include "xdg-shell-client-protocol.h"
#include <cassert>
#include <cstring>
#include <unordered_map>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

static WKPageNavigationClientV0 createPageNavigationClient()
{
    WKPageNavigationClientV0 navigationClient = {
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
    return navigationClient;
}

struct Embedder {
    struct wl_display* display;
    struct wl_registry* registry;

    struct wl_compositor* compositor;
    struct xdg_shell* xdgShell;

    EGLDisplay eglDisplay;
    EGLConfig eglConfig;
    EGLContext eglContext;

    PFNEGLCREATEIMAGEKHRPROC createImage;
    PFNEGLDESTROYIMAGEKHRPROC destroyImage;
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC imageTargetTexture2DOES;

    struct wl_surface* surface;
    struct wl_egl_window* nativeWindow;
    EGLSurface eglSurface;
    struct xdg_surface* xdgSurface;

    static const struct wl_registry_listener registryListener;
    static const struct xdg_shell_listener xdgShellListener;
    static const struct wl_callback_listener frameListener;

    static const EGLint contextAttributes[];
    static const EGLint configAttributes[];

    GLuint program, fragmentShader, vertexShader;
    GLuint rotation, textureUniform;
    GLuint texture;
    float angle { 0 };

    static void createProgram(Embedder& e);
    static void createTexture(Embedder& e);
    static void render(Embedder&);

    static const char* vertexShaderSource;
    static const char* fragmentShaderSource;

    std::unordered_map<uint32_t, std::pair<int32_t, EGLImageKHR>> imageMap;
    std::pair<uint32_t, EGLImageKHR> pendingImage { 0, nullptr };
    std::pair<uint32_t, EGLImageKHR> lockedImage { 0, nullptr };

    GSource* displaySource;

    WKContextRef context;
    WKPageConfigurationRef pageConfiguration;

    struct wpe_mesa_view_backend_exportable_dma_buf* exportableBackend;
    WKViewRef view;

    static struct wpe_mesa_view_backend_exportable_dma_buf_client exportableClient;
};

const struct wl_registry_listener Embedder::registryListener = {
    // global
    [] (void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t)
    {
        auto& e = *static_cast<Embedder*>(data);

        if (!std::strcmp(interface, "wl_compositor"))
            e.compositor = static_cast<struct wl_compositor*>(wl_registry_bind(registry, name, &wl_compositor_interface, 1));
        else if (!std::strcmp(interface, "xdg_shell")) {
            e.xdgShell = static_cast<struct xdg_shell*>(wl_registry_bind(registry, name, &xdg_shell_interface, 1));
            xdg_shell_add_listener(e.xdgShell, &Embedder::xdgShellListener, nullptr);
            xdg_shell_use_unstable_version(e.xdgShell, 5);
        }
    },
    // global_remove
    [] (void*, struct wl_registry*, uint32_t) { },
};

const struct xdg_shell_listener Embedder::xdgShellListener = {
    // ping
    [] (void*, struct xdg_shell* shell, uint32_t serial)
    {
        xdg_shell_pong(shell, serial);
    },
};

const struct wl_callback_listener Embedder::frameListener = {
    // frame
    [] (void* data, struct wl_callback* callback, uint32_t time)
    {
        if (callback)
            wl_callback_destroy(callback);

        auto& e = *static_cast<Embedder*>(data);

        if (e.pendingImage.first)
            wpe_mesa_view_backend_exportable_dma_buf_dispatch_frame_complete(e.exportableBackend);
        if (e.lockedImage.first) {
            wpe_mesa_view_backend_exportable_dma_buf_dispatch_release_buffer(e.exportableBackend, e.lockedImage.first);
            e.lockedImage = { 0, nullptr };
        }

        Embedder::render(e);
    },
};

const EGLint Embedder::contextAttributes[3] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
};
const EGLint Embedder::configAttributes[13] = {
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RED_SIZE, 1,
    EGL_GREEN_SIZE, 1,
    EGL_BLUE_SIZE, 1,
    EGL_ALPHA_SIZE, 1,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
};

void Embedder::createProgram(Embedder& e)
{
    e.vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(e.vertexShader, 1, static_cast<const char**>(&Embedder::vertexShaderSource), nullptr);
    glCompileShader(e.vertexShader);

    e.fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(e.fragmentShader, 1, static_cast<const char**>(&Embedder::fragmentShaderSource), nullptr);
    glCompileShader(e.fragmentShader);

    e.program = glCreateProgram();
    glAttachShader(e.program, e.fragmentShader);
    glAttachShader(e.program, e.vertexShader);
    glLinkProgram(e.program);

    GLint status;
    glGetProgramiv(e.program, GL_LINK_STATUS, &status);

    glBindAttribLocation(e.program, 0, "pos");
    glBindAttribLocation(e.program, 1, "texture");
    e.rotation = glGetUniformLocation(e.program, "rotation");
    e.textureUniform = glGetUniformLocation(e.program, "u_texture");
}

void Embedder::createTexture(Embedder& e)
{
    glGenTextures(1, &e.texture);
    glBindTexture(GL_TEXTURE_2D, e.texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 800, 600, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
}

void Embedder::render(Embedder& e)
{
    eglMakeCurrent(e.eglDisplay, e.eglSurface, e.eglSurface, e.eglContext);

    glClearColor(0.125, 0.125, 0.125, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(e.program);

    static const GLfloat vertices[4][2] = {
        { -1.0,  1.0 },
        {  1.0,  1.0 },
        { -1.0, -1.0 },
        {  1.0, -1.0 },
    };

    static const GLfloat texturePos[4][2] = {
        {  0,  0 },
        {  1,  0 },
        {  0,  1 },
        {  1,  1 },
    };

    static GLfloat rotation[4][4] = {
        { 1, 0, 0, 0 },
        { 0, 1, 0, 0 },
        { 0, 0, 1, 0 },
        { 0, 0, 0, 1 },
    };

    e.angle += 0.02f;
    rotation[0][0] =  cos(e.angle);

    glUniformMatrix4fv(e.rotation, 1, GL_FALSE, reinterpret_cast<GLfloat*>(rotation));

    if (e.pendingImage.first) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, e.texture);
        e.imageTargetTexture2DOES(GL_TEXTURE_2D, e.pendingImage.second);
        glUniform1i(e.textureUniform, 0);

        e.lockedImage = e.pendingImage;
        e.pendingImage = { 0, nullptr };
    }

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, texturePos);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    struct wl_callback* callback = wl_surface_frame(e.surface);
    wl_callback_add_listener(callback, &Embedder::frameListener, &e);

    eglSwapBuffers(e.eglDisplay, e.eglSurface);
}

const char* Embedder::vertexShaderSource =
	"uniform mat4 rotation;\n"
	"attribute vec2 pos;\n"
	"attribute vec2 texture;\n"
	"varying vec2 v_texture;\n"
	"void main() {\n"
	"  v_texture = texture;\n"
	"  gl_Position = rotation * vec4(pos, 0, 1);\n"
	"}\n";

const char* Embedder::fragmentShaderSource =
	"precision mediump float;\n"
    "uniform sampler2D u_texture;\n"
	"varying vec2 v_texture;\n"
	"void main() {\n"
	"  gl_FragColor = texture2D(u_texture, v_texture);\n"
	"}\n";

struct EventSource {
    GSource source;
    GPollFD pfd;
    struct wl_display* display;

    static GSourceFuncs sourceFuncs;
};

GSourceFuncs EventSource::sourceFuncs = {
    // prepare
    [](GSource* base, gint* timeout) -> gboolean
    {
        auto& source = *reinterpret_cast<EventSource*>(base);

        *timeout = -1;

        wl_display_flush(source.display);
        wl_display_dispatch_pending(source.display);

        return FALSE;
    },
    // check
    [](GSource* base) -> gboolean
    {
        auto& source = *reinterpret_cast<EventSource*>(base);
        return !!source.pfd.revents;
    },
    // dispatch
    [](GSource* base, GSourceFunc, gpointer) -> gboolean
    {
        auto& source = *reinterpret_cast<EventSource*>(base);

        if (source.pfd.revents & G_IO_IN)
            wl_display_dispatch(source.display);

        if (source.pfd.revents & (G_IO_ERR | G_IO_HUP))
            return FALSE;

        source.pfd.revents = 0;
        return TRUE;
    },
    nullptr, // finalize
    nullptr, // closure_callback
    nullptr, // closure_marshall
};

struct wpe_mesa_view_backend_exportable_dma_buf_client Embedder::exportableClient = {
    // export_dma_buf
    [](void* data, struct wpe_mesa_view_backend_exportable_dma_buf_data* imageData)
    {
        auto& e = *static_cast<Embedder*>(data);

        auto it = e.imageMap.end();
        if (imageData->fd >= 0) {
            assert(e.imageMap.find(imageData->handle) == e.imageMap.end());

            it = e.imageMap.insert({ imageData->handle, { imageData->fd, nullptr }}).first;
        } else {
            assert(e.imageMap.find(imageData->handle) != e.imageMap.end());
            it = e.imageMap.find(imageData->handle);
        }

        assert(it != e.imageMap.end());
        uint32_t handle = it->first;
        int32_t fd = it->second.first;

        eglMakeCurrent(e.eglDisplay, e.eglSurface, e.eglSurface, e.eglContext);

        EGLint attributes[] = {
            EGL_WIDTH, static_cast<EGLint>(imageData->width),
            EGL_HEIGHT, static_cast<EGLint>(imageData->height),
            EGL_LINUX_DRM_FOURCC_EXT, static_cast<EGLint>(imageData->format),
            EGL_DMA_BUF_PLANE0_FD_EXT, fd,
            EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
            EGL_DMA_BUF_PLANE0_PITCH_EXT, static_cast<EGLint>(imageData->stride),
            EGL_NONE,
        };
        EGLImageKHR image = e.createImage(e.eglDisplay, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr, attributes);

        e.pendingImage = { handle, image };
    }
};

int main(int argc, char* argv[])
{
    Embedder e;
    e.display = wl_display_connect(nullptr);
    e.registry = wl_display_get_registry(e.display);

    wl_registry_add_listener(e.registry, &Embedder::registryListener, &e);
    wl_display_roundtrip(e.display);

    e.eglDisplay = eglGetDisplay(e.display);
    eglInitialize(e.eglDisplay, nullptr, nullptr);
    eglBindAPI(EGL_OPENGL_ES_API);

    EGLint numConfigs;
    eglChooseConfig(e.eglDisplay, Embedder::configAttributes, &e.eglConfig, 1, &numConfigs);

    e.eglContext = eglCreateContext(e.eglDisplay, e.eglConfig, EGL_NO_CONTEXT, Embedder::contextAttributes);
    e.createImage = reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(eglGetProcAddress("eglCreateImageKHR"));
    e.destroyImage = reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(eglGetProcAddress("eglDestroyImageKHR"));
    e.imageTargetTexture2DOES = reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(eglGetProcAddress("glEGLImageTargetTexture2DOES"));

    e.surface = wl_compositor_create_surface(e.compositor);
    e.nativeWindow = wl_egl_window_create(e.surface, 800, 600);
    e.eglSurface = eglCreateWindowSurface(e.eglDisplay, e.eglConfig, e.nativeWindow, nullptr);
    e.xdgSurface = xdg_shell_get_xdg_surface(e.xdgShell, e.surface);

    eglMakeCurrent(e.eglDisplay, e.eglSurface, e.eglSurface, e.eglContext);

    Embedder::createProgram(e);
    Embedder::createTexture(e);

    GMainLoop* loop = g_main_loop_new(nullptr, FALSE);

    e.displaySource = g_source_new(&EventSource::sourceFuncs, sizeof(EventSource));
    auto& source = *reinterpret_cast<EventSource*>(e.displaySource);
    source.display = e.display;

    source.pfd.fd = wl_display_get_fd(source.display);
    source.pfd.events = G_IO_IN | G_IO_ERR | G_IO_HUP;
    source.pfd.revents = 0;
    g_source_add_poll(e.displaySource, &source.pfd);

    g_source_set_can_recurse(e.displaySource, TRUE);
    g_source_attach(e.displaySource, nullptr);

    e.context = WKContextCreate();
    e.pageConfiguration = WKPageConfigurationCreate();
    {
        auto pageGroupIdentifier = adoptWK(WKStringCreateWithUTF8CString("WPEPageGroup"));
        auto pageGroup = adoptWK(WKPageGroupCreateWithIdentifier(pageGroupIdentifier.get()));
        WKPageConfigurationSetContext(e.pageConfiguration, e.context);
        WKPageConfigurationSetPageGroup(e.pageConfiguration, pageGroup.get());
    }

    e.exportableBackend = wpe_mesa_view_backend_exportable_dma_buf_create(&Embedder::exportableClient, &e);
    e.view = WKViewCreateWithViewBackend(wpe_mesa_view_backend_exportable_dma_buf_get_view_backend(e.exportableBackend), e.pageConfiguration);
    auto pageNavigationClient = createPageNavigationClient();
    WKPageSetPageNavigationClient(WKViewGetPage(e.view), &pageNavigationClient.base);

    const char* url = "http://www.webkit.org/blog-files/3d-transforms/poster-circle.html";
    if (argc > 1)
        url = argv[1];

    {
        auto shellURL = adoptWK(WKURLCreateWithUTF8CString(url));
        WKPageLoadURL(WKViewGetPage(e.view), shellURL.get());
    }

    g_idle_add(
        [](gpointer data) {
            auto& e = *static_cast<Embedder*>(data);
            Embedder::render(e);
            return FALSE;
        }, &e);

    g_main_loop_run(loop);

    g_main_loop_unref(loop);

    return 0;
}
