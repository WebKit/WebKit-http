#ifndef wpe_view_backend_wayland_display_h
#define wpe_view_backend_wayland_display_h

#include <array>
#include <unordered_map>
#include <utility>
#include <wpe/input.h>
#include <xkbcommon/xkbcommon-compose.h>
#include <xkbcommon/xkbcommon.h>

struct wpe_view_backend;

struct ivi_application;
struct wl_compositor;
struct wl_data_device_manager;
struct wl_display;
struct wl_drm;
struct wl_keyboard;
struct wl_pointer;
struct wl_registry;
struct wl_seat;
struct wl_surface;
struct wl_touch;
struct xdg_shell;

typedef struct _GSource GSource;

namespace Wayland {

class Display {
public:
    static Display& singleton();

    struct wl_display* display() const { return m_display; }

    uint32_t serial() const { return m_seatData.serial; }

    struct Interfaces {
        struct wl_compositor* compositor;
        struct wl_data_device_manager* data_device_manager;
        struct wl_drm* drm;
        struct wl_seat* seat;
        struct xdg_shell* xdg;
        struct ivi_application* ivi_application;
    };
    const Interfaces& interfaces() const { return m_interfaces; }

    struct SeatData {
        std::unordered_map<struct wl_surface*, struct wpe_view_backend*> inputClients;

        struct {
            struct wl_pointer* object;
            std::pair<struct wl_surface*, struct wpe_view_backend*> target;
            std::pair<int, int> coords;
        } pointer { nullptr, { }, { 0, 0 } };
        struct {
            struct wl_keyboard* object;
            std::pair<struct wl_surface*, struct wpe_view_backend*> target;
        } keyboard { nullptr, { } };
        struct {
            struct wl_touch* object;
            std::array<std::pair<struct wl_surface*, struct wpe_view_backend*>, 10> targets;
            std::array<struct wpe_input_touch_event_raw, 10> touchPoints;
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

    void registerInputClient(struct wl_surface*, struct wpe_view_backend*);
    void unregisterInputClient(struct wl_surface*);

private:
    Display();
    ~Display();

    struct wl_display* m_display;
    struct wl_registry* m_registry;
    Interfaces m_interfaces;

    SeatData m_seatData;

    GSource* m_eventSource;
};

} // namespace Wayland

#endif // wpe_view_backend_wayland_display_h
