#ifndef WPE_Graphics_RenderingBackendSTM_h
#define WPE_Graphics_RenderingBackendSTM_h

#include <WPE/Graphics/RenderingBackend.h>
#include <wayland-egl.h>
#include <wayland-client.h>
#include <glib.h>
#include <WPE/Input/Events.h>
#include <xkbcommon/xkbcommon-compose.h>
#include <xkbcommon/xkbcommon.h>

struct wl_egl_window;
struct wl_surface;

namespace WPE {

namespace Graphics {

class RenderingBackendSTM final : public RenderingBackend {
public:
    using Client = WPE::Graphics::RenderingBackend::Surface::Client;
    class Surface final : public RenderingBackend::Surface {
    public:
        Surface(const RenderingBackendSTM&, uint32_t, uint32_t, uint32_t, Client&);
        WPE_EXPORT virtual ~Surface();

        EGLNativeWindowType nativeWindow() override;
        void resize(uint32_t, uint32_t) override;

        BufferExport lockFrontBuffer() override;
        void releaseBuffer(uint32_t) override;

        Client& getClient() {return m_client;}
        wl_surface* getWlSurface() {return m_surface;}
    private:
        struct wl_surface* m_surface;
        struct wl_egl_window* m_window;
        Client& m_client;
    };

    class OffscreenSurface final : public RenderingBackend::OffscreenSurface {
    public:
        OffscreenSurface(const RenderingBackendSTM&);
        virtual ~OffscreenSurface();

        EGLNativeWindowType nativeWindow() override;
    };


    struct SeatData {
        struct {
            struct wl_pointer* object;
            struct wl_surface* surface;
            std::pair<int, int> coords;
        } pointer { nullptr, nullptr, { 0, 0 } };
        struct {
            struct wl_keyboard* object;
            struct wl_surface* surface;
        } keyboard { nullptr, nullptr};
        struct {
            struct wl_touch* object;
            std::array<struct wl_surface*, 10> targets;
            std::array<Input::TouchEvent::Raw, 10> touchPoints;
        } touch { nullptr, { }, { } };

        struct {
            struct xkb_context* context;
            struct xkb_keymap* keymap;
            struct xkb_state* state;
            struct {
                xkb_mod_index_t control;
                xkb_mod_index_t alt;
                xkb_mod_index_t shift;
            } indexes;
            uint8_t modifiers;
            struct xkb_compose_table* composeTable;
            struct xkb_compose_state* composeState;
        } xkb { nullptr, nullptr, nullptr, { 0, 0, 0 }, 0, nullptr, nullptr };

        struct {
            int32_t rate;
            int32_t delay;
        } repeatInfo { 0, 0 };

        struct {
            uint32_t key;
            uint32_t time;
            uint32_t state;
            uint32_t eventSource;
        } repeatData { 0, 0, 0, 0 };

        uint32_t serial;
    };

    setInputSurface(Surface* surface) { input_surface = surface; }
    SeatData& getSeatData() { return m_seatData; }
    wl_surface* getWlSurface() { return input_surface->getWlSurface(); }
    Client& getClient() { return input_surface->getClient(); }
    void initializeSeatData();
    RenderingBackendSTM();
    virtual ~RenderingBackendSTM();

    EGLNativeDisplayType nativeDisplay() override;
    std::unique_ptr<RenderingBackend::Surface> createSurface(uint32_t, uint32_t, uint32_t, RenderingBackend::Surface::Client&) override;
    std::unique_ptr<RenderingBackend::OffscreenSurface> createOffscreenSurface() override;

private:
    static const struct wl_registry_listener m_registryListener;
    static const struct wl_seat_listener m_seatListener;
    static void globalCallback(void* data, struct wl_registry*, uint32_t name, const char* interface, uint32_t version);
    static void globalRemoveCallback(void* data, struct wl_registry*, uint32_t name);
    static void globalSeatCapabilities(void* data, struct wl_seat* seat, uint32_t capabilities);
    static void globalSeatName(void* data, struct wl_seat* seat, const char* name);

    Surface* input_surface;
    struct wl_display* m_display;
    struct wl_registry* m_registry;
    struct wl_compositor* m_compositor;
    struct wl_shell* m_shell;
    struct wl_seat* m_seat;
    struct wl_pointer* m_pointer;
    struct wl_keyboard* m_keyboard;
    struct wl_touch* m_touch;
    bool m_seatDataInitialized;
    SeatData m_seatData;
    GSource* m_eventSource;
};

} // namespace Graphics

} // namespace WPE
#endif
