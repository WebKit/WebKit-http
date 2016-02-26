#ifndef WPE_Graphics_RenderingBackendWesteros_h
#define WPE_Graphics_RenderingBackendWesteros_h
#if WPE_BACKEND(WESTEROS)

#include <wayland-client.h>
#include <wayland-egl.h>
#include <WPE/Graphics/RenderingBackend.h>

typedef struct _GSource GSource;

namespace WPE {

namespace Graphics {

class RenderingBackendWesteros final : public RenderingBackend {
public:
    class Surface final : public RenderingBackend::Surface {
    public:
        Surface(const RenderingBackendWesteros&, uint32_t, uint32_t, uint32_t, Client&);
        WPE_EXPORT virtual ~Surface();

        EGLNativeWindowType nativeWindow() override;
        void resize(uint32_t, uint32_t) override;

        BufferExport lockFrontBuffer() override;
        void releaseBuffer(uint32_t) override;

    private:
        struct wl_surface* m_surface;
        struct wl_egl_window* m_window;
    };

    class OffscreenSurface final : public RenderingBackend::OffscreenSurface {
    public:
        OffscreenSurface(const RenderingBackendWesteros&);
        virtual ~OffscreenSurface();

        EGLNativeWindowType nativeWindow() override;
    };

    RenderingBackendWesteros();
    virtual ~RenderingBackendWesteros();

    EGLNativeDisplayType nativeDisplay() override;
    std::unique_ptr<RenderingBackend::Surface> createSurface(uint32_t, uint32_t, uint32_t, RenderingBackend::Surface::Client&) override;
    std::unique_ptr<RenderingBackend::OffscreenSurface> createOffscreenSurface() override;
    static void globalCallback(void* data, struct wl_registry*, uint32_t name, const char* interface, uint32_t version);
    static void globalRemoveCallback(void* data, struct wl_registry*, uint32_t name);

private:
    struct wl_display* m_display;
    struct wl_registry* m_registry;
    struct wl_compositor* m_compositor;
    GSource* m_eventSource;
};

} // namespace Graphics

} // namespace WPE

#endif // WPE_BACKEND(WESTEROS)
#endif // WPE_Graphics_RenderingBackendWesteros_h
