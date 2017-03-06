#ifndef WPE_ViewBackend_WesterosViewbackendInput_h
#define WPE_ViewBackend_WesterosViewbackendInput_h

#include <glib.h>
#include <utility>
#include <xkbcommon/xkbcommon-compose.h>
#include <xkbcommon/xkbcommon.h>
#include <wayland-client.h>
#include <westeros-compositor.h>

struct wpe_view_backend;

namespace Westeros {

class WesterosViewbackendInput {
public:
    WesterosViewbackendInput(struct wpe_view_backend*);
    virtual ~WesterosViewbackendInput();
    void initializeNestedInputHandler(WstCompositor *compositor);
    void deinitialize() { m_viewbackend = nullptr; }

    static void keyboardHandleKeyMap( void *userData, uint32_t format, int fd, uint32_t size );
    static void keyboardHandleEnter( void *userData, struct wl_array *keys );
    static void keyboardHandleLeave( void *userData );
    static void keyboardHandleKey( void *userData, uint32_t time, uint32_t key, uint32_t state );
    static void keyboardHandleModifiers( void *userData, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group );
    static void keyboardHandleRepeatInfo( void *userData, int32_t rate, int32_t delay );
    static void pointerHandleEnter( void *userData, wl_fixed_t sx, wl_fixed_t sy );
    static void pointerHandleLeave( void *userData );
    static void pointerHandleMotion( void *userData, uint32_t time, wl_fixed_t sx, wl_fixed_t sy );
    static void pointerHandleButton( void *userData, uint32_t time, uint32_t button, uint32_t state );
    static void pointerHandleAxis( void *userData, uint32_t time, uint32_t axis, wl_fixed_t value );
    static void handleKeyEvent(void* userData, uint32_t key, uint32_t state, uint32_t time);
    static gboolean repeatRateTimeout(void* userData);
    static gboolean repeatDelayTimeout(void* userData);

    struct HandlerData {
        struct {
            std::pair<int, int> coords;
        } pointer { { 0, 0 } };

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
    };

private:
    void clearDataArrays();

    WstCompositor* m_compositor;
    struct wpe_view_backend* m_viewbackend;
    HandlerData m_handlerData;
    GPtrArray* m_keyEventDataArray;
    GPtrArray* m_motionEventDataArray;
    GPtrArray* m_buttonEventDataArray;
    GPtrArray* m_axisEventDataArray;
};

} // namespace Westeros

#endif // WPE_ViewBackend_WesterosViewbackendInput_h
