#include "Config.h"
#include "RenderingBackendSTM.h"

#if WPE_BACKEND(STM)

#include <EGL/egl.h>
#include <cstring>

#include <cassert>
#include <cstring>
#include <glib.h>
#include <linux/input.h>
#include <locale.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>
#include <WPE/Input/Events.h>

namespace WPE {

namespace Graphics {

typedef struct _GSource GSource;

class EventSource {
public:
    static GSourceFuncs sourceFuncs1;

    GSource source;
    GPollFD pfd;
    struct wl_display* display;
};

GSourceFuncs EventSource::sourceFuncs1 = {
    // prepare
    [](GSource* base, gint* timeout) -> gboolean
    {
        auto* source = reinterpret_cast<EventSource*>(base);
        struct wl_display* display = source->display;

        *timeout = -1;
        wl_display_flush(display);
        wl_display_dispatch_pending(display);

        return FALSE;
    },
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
        struct wl_display* display = source->display;

        if (source->pfd.revents & G_IO_IN)
            wl_display_dispatch(display);

        if (source->pfd.revents & (G_IO_ERR | G_IO_HUP))
            return FALSE;

        source->pfd.revents = 0;
        return TRUE;
    },
    nullptr, // finalize
    nullptr, // closure_callback
    nullptr, // closure_marshall
};


//For sending pong in response to ping from server
static void
handle_ping(void *data, struct wl_shell_surface *shell_surface,
                                                       uint32_t serial)
{
       wl_shell_surface_pong(shell_surface, serial);
}

static void
handle_configure(void *data, struct wl_shell_surface *shell_surface,
                uint32_t edges, int32_t width, int32_t height)
{
}

static void
handle_popup_done(void *data, struct wl_shell_surface *shell_surface)
{
}

static const struct wl_shell_surface_listener shell_surface_listener = {
       handle_ping,
       handle_configure,
       handle_popup_done
};
//For sending pong in response to ping from server


const struct wl_registry_listener RenderingBackendSTM::m_registryListener = {
    RenderingBackendSTM::globalCallback,
    RenderingBackendSTM::globalRemoveCallback
};

const struct wl_seat_listener RenderingBackendSTM::m_seatListener = {
    RenderingBackendSTM::globalSeatCapabilities,
    RenderingBackendSTM::globalSeatName
};

static const struct wl_pointer_listener g_pointerListener = {
    // enter
    [](void* data, struct wl_pointer*, uint32_t serial, struct wl_surface* surface, wl_fixed_t, wl_fixed_t)
    {
        auto& backend = *static_cast<RenderingBackendSTM*>(data);
        auto& seatData = backend.getSeatData();
        seatData.serial = serial;
        seatData.pointer.surface = surface;
    },
    // leave
    [](void* data, struct wl_pointer*, uint32_t serial, struct wl_surface* surface)
    {
        auto& backend = *static_cast<RenderingBackendSTM*>(data);
        auto& seatData = backend.getSeatData();
        seatData.serial = serial;
        seatData.pointer.surface = nullptr;
    },
    // motion
    [](void* data, struct wl_pointer*, uint32_t time, wl_fixed_t fixedX, wl_fixed_t fixedY)
    {
        auto& backend = *static_cast<RenderingBackendSTM*>(data);
        auto& seatData = backend.getSeatData();
        auto x = wl_fixed_to_int(fixedX);
        auto y = wl_fixed_to_int(fixedY);

        auto& pointer = seatData.pointer;
        pointer.coords = { x, y };
        if (pointer.surface == backend.getWlSurface())
        {
            WPE::Input::PointerEvent event{Input::PointerEvent::Motion, time, x, y, 0, 0};
/* FIXME Input handling
            backend.getClient().handlePointerEvent(event);
*/
        }
    },
    // button
    [](void* data, struct wl_pointer*, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
    {
        auto& backend = *static_cast<RenderingBackendSTM*>(data);
        auto& seatData = backend.getSeatData();

        seatData.serial = serial;

        if (button >= BTN_MOUSE)
            button = button - BTN_MOUSE + 1;
        else
            button = 0;

        auto& pointer = seatData.pointer;
        auto& coords = pointer.coords;
        if (pointer.surface == backend.getWlSurface())
        {
            WPE::Input::PointerEvent event{Input::PointerEvent::Button, time, coords.first, coords.second, button, state};
/* FIXME  Input handling
            backend.getClient().handlePointerEvent(event);
*/
        }
    },
    // axis
    [](void* data, struct wl_pointer*, uint32_t time, uint32_t axis, wl_fixed_t value)
    {
        auto& backend = *static_cast<RenderingBackendSTM*>(data);
        auto& seatData = backend.getSeatData();
        auto& pointer = seatData.pointer;
        auto& coords = pointer.coords;
        if (pointer.surface == backend.getWlSurface())
        {
            WPE::Input::AxisEvent event{Input::AxisEvent::Motion, time, coords.first, coords.second, axis, -wl_fixed_to_int(value)};
/* FIXME Input handling
            backend.getClient().handleAxisEvent(event);
*/
        }
    },
};

static void
handleKeyEvent(void* data, uint32_t key, uint32_t state, uint32_t time)
{
    auto& backend = *static_cast<RenderingBackendSTM*>(data);
    auto& seatData = backend.getSeatData();
    auto& xkb = seatData.xkb;
    uint32_t keysym = xkb_state_key_get_one_sym(xkb.state, key);
    uint32_t unicode = xkb_state_key_get_utf32(xkb.state, key);

    if (xkb.composeState
        && state == WL_KEYBOARD_KEY_STATE_PRESSED
        && xkb_compose_state_feed(xkb.composeState, keysym) == XKB_COMPOSE_FEED_ACCEPTED
        && xkb_compose_state_get_status(xkb.composeState) == XKB_COMPOSE_COMPOSED)
    {
        keysym = xkb_compose_state_get_one_sym(xkb.composeState);
        unicode = xkb_keysym_to_utf32(keysym);
    }

    if (seatData.keyboard.surface == backend.getWlSurface())
    {
        WPE::Input::KeyboardEvent event{time, keysym, unicode, !!state, xkb.modifiers};
/* FIXME Input handling
        backend.getClient().handleKeyboardEvent(event);
*/
    }
}

static gboolean
repeatRateTimeout(void* data)
{
    auto& backend = *static_cast<RenderingBackendSTM*>(data);
    auto& seatData = backend.getSeatData();
    handleKeyEvent(data, seatData.repeatData.key, seatData.repeatData.state, seatData.repeatData.time);
    return G_SOURCE_CONTINUE;
}

static gboolean
repeatDelayTimeout(void* data)
{
    auto& backend = *static_cast<RenderingBackendSTM*>(data);
    auto& seatData = backend.getSeatData();
    handleKeyEvent(data, seatData.repeatData.key, seatData.repeatData.state, seatData.repeatData.time);
    seatData.repeatData.eventSource = g_timeout_add(seatData.repeatInfo.rate, static_cast<GSourceFunc>(repeatRateTimeout), data);
    return G_SOURCE_REMOVE;
}


static const struct wl_keyboard_listener g_keyboardListener = {
    // keymap
    [](void* data, struct wl_keyboard*, uint32_t format, int fd, uint32_t size)
    {
        if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
            close(fd);
            return;
        }

        void* mapping = mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
        if (mapping == MAP_FAILED) {
            close(fd);
            return;
        }

        auto& backend = *static_cast<RenderingBackendSTM*>(data);
        auto& seatData = backend.getSeatData();
        auto& xkb = seatData.xkb;
        xkb.keymap = xkb_keymap_new_from_string(xkb.context, static_cast<char*>(mapping),
            XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
        munmap(mapping, size);
        close(fd);

        if (!xkb.keymap)
            return;

        xkb.state = xkb_state_new(xkb.keymap);
        if (!xkb.state)
            return;

        xkb.indexes.control = xkb_keymap_mod_get_index(xkb.keymap, XKB_MOD_NAME_CTRL);
        xkb.indexes.alt = xkb_keymap_mod_get_index(xkb.keymap, XKB_MOD_NAME_ALT);
        xkb.indexes.shift = xkb_keymap_mod_get_index(xkb.keymap, XKB_MOD_NAME_SHIFT);
    },
    // enter
    [](void* data, struct wl_keyboard*, uint32_t serial, struct wl_surface* surface, struct wl_array*)
    {
        auto& backend = *static_cast<RenderingBackendSTM*>(data);
        auto& seatData = backend.getSeatData();
        seatData.serial = serial;
        seatData.keyboard.surface = surface;
    },
    // leave
    [](void* data, struct wl_keyboard*, uint32_t serial, struct wl_surface* surface)
    {
        auto& backend = *static_cast<RenderingBackendSTM*>(data);
        auto& seatData = backend.getSeatData();
        seatData.serial = serial;
        seatData.keyboard.surface = nullptr;
    },
    // key
    [](void* data, struct wl_keyboard*, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
    {
        // IDK.
        key += 8;
        auto& backend = *static_cast<RenderingBackendSTM*>(data);
        auto& seatData = backend.getSeatData();

        seatData.serial = serial;
        handleKeyEvent(data, key, state, time);

        if (!seatData.repeatInfo.rate)
            return;

        if (state == WL_KEYBOARD_KEY_STATE_RELEASED
            && seatData.repeatData.key == key) {
            if (seatData.repeatData.eventSource)
                g_source_remove(seatData.repeatData.eventSource);
            seatData.repeatData = { 0, 0, 0, 0 };
        } else if (state == WL_KEYBOARD_KEY_STATE_PRESSED
            && xkb_keymap_key_repeats(seatData.xkb.keymap, key)) {

            if (seatData.repeatData.eventSource)
                g_source_remove(seatData.repeatData.eventSource);

            seatData.repeatData = { key, time, state, g_timeout_add(seatData.repeatInfo.delay, static_cast<GSourceFunc>(repeatDelayTimeout), data) };
        }
    },
    // modifiers
    [](void* data, struct wl_keyboard*, uint32_t serial, uint32_t depressedMods, uint32_t latchedMods, uint32_t lockedMods, uint32_t group)
    {
        auto& backend = *static_cast<RenderingBackendSTM*>(data);
        auto& seatData = backend.getSeatData();
        seatData.serial = serial;
        auto& xkb = seatData.xkb;
        xkb_state_update_mask(xkb.state, depressedMods, latchedMods, lockedMods, 0, 0, group);

        auto& modifiers = xkb.modifiers;
        modifiers = 0;
        auto component = static_cast<xkb_state_component>(XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED);
        if (xkb_state_mod_index_is_active(xkb.state, xkb.indexes.control, component))
            modifiers |= Input::KeyboardEvent::Control;
        if (xkb_state_mod_index_is_active(xkb.state, xkb.indexes.alt, component))
            modifiers |= Input::KeyboardEvent::Alt;
        if (xkb_state_mod_index_is_active(xkb.state, xkb.indexes.shift, component))
            modifiers |= Input::KeyboardEvent::Shift;
    },
    // repeat_info
    [](void* data, struct wl_keyboard*, int32_t rate, int32_t delay)
    {
        auto& backend = *static_cast<RenderingBackendSTM*>(data);
        auto& seatData = backend.getSeatData();
        auto& repeatInfo = seatData.repeatInfo;
        repeatInfo = { rate, delay };

        // A rate of zero disables any repeating.
        if (!rate) {
            auto& repeatData = seatData.repeatData;
            if (repeatData.eventSource) {
                g_source_remove(repeatData.eventSource);
                repeatData = { 0, 0, 0, 0 };
            }
        }
    },
};

static const struct wl_touch_listener g_touchListener = {
    // down
    [](void* data, struct wl_touch*, uint32_t serial, uint32_t time, struct wl_surface* surface, int32_t id, wl_fixed_t x, wl_fixed_t y)
    {
/*
        auto& seatData = *static_cast<RenderingBackendSTM::SeatData*>(data);
        seatData.serial = serial;

        int32_t arraySize = std::tuple_size<decltype(seatData.touch.targets)>::value;
        if (id < 0 || id >= arraySize)
            return;

        auto& target = seatData.touch.targets[id];
        assert(!target.first && !target.second);

        auto it = seatData.inputClients.find(surface);
        if (it == seatData.inputClients.end())
            return;

        target = { surface, it->second };

        auto& touchPoints = seatData.touch.touchPoints;
        touchPoints[id] = { Input::TouchEvent::Down, time, id, wl_fixed_to_int(x), wl_fixed_to_int(y) };
        target.second->handleTouchEvent({ touchPoints, Input::TouchEvent::Down, id, time });
*/
    },
    // up
    [](void* data, struct wl_touch*, uint32_t serial, uint32_t time, int32_t id)
    {
/*
        auto& seatData = *static_cast<RenderingBackendSTM::SeatData*>(data);
        seatData.serial = serial;

        int32_t arraySize = std::tuple_size<decltype(seatData.touch.targets)>::value;
        if (id < 0 || id >= arraySize)
            return;

        auto& target = seatData.touch.targets[id];
        assert(target.first && target.second);

        auto& touchPoints = seatData.touch.touchPoints;
        auto& point = touchPoints[id];
        point = { Input::TouchEvent::Up, time, id, point.x, point.y };
        target.second->handleTouchEvent({ touchPoints, Input::TouchEvent::Up, id, time });

        point = { Input::TouchEvent::Null, 0, 0, 0, 0 };
        target = { nullptr, nullptr };
*/
    },
    // motion
    [](void* data, struct wl_touch*, uint32_t time, int32_t id, wl_fixed_t x, wl_fixed_t y)
    {
/*
        auto& seatData = *static_cast<RenderingBackendSTM::SeatData*>(data);

        int32_t arraySize = std::tuple_size<decltype(seatData.touch.targets)>::value;
        if (id < 0 || id >= arraySize)
            return;

        auto& target = seatData.touch.targets[id];
        assert(target.first && target.second);

        auto& touchPoints = seatData.touch.touchPoints;
        touchPoints[id] = { Input::TouchEvent::Motion, time, id, wl_fixed_to_int(x), wl_fixed_to_int(y) };
        target.second->handleTouchEvent({ touchPoints, Input::TouchEvent::Motion, id, time });
*/
    },
    // frame
    [](void*, struct wl_touch*)
    {
        // FIXME: Dispatching events via frame() would avoid dispatching events
        // for every single event that's encapsulated in a frame with multiple
        // other events.
    },
    // cancel
    [](void*, struct wl_touch*) { },
};

void RenderingBackendSTM::globalSeatCapabilities(void* data, struct wl_seat* seat, uint32_t capabilities)
{
    auto& backend = *static_cast<RenderingBackendSTM*>(data);
    auto& seatData = backend.getSeatData();

    // WL_SEAT_CAPABILITY_POINTER
    const bool hasPointerCap = capabilities & WL_SEAT_CAPABILITY_POINTER;
    if (hasPointerCap && !seatData.pointer.object) {
        seatData.pointer.object = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(seatData.pointer.object, &g_pointerListener, data);
    }
    if (!hasPointerCap && seatData.pointer.object) {
        wl_pointer_destroy(seatData.pointer.object);
        seatData.pointer.object = nullptr;
    }

    // WL_SEAT_CAPABILITY_KEYBOARD
    const bool hasKeyboardCap = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;
    if (hasKeyboardCap && !seatData.keyboard.object) {
        seatData.keyboard.object = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(seatData.keyboard.object, &g_keyboardListener, data);
    }
    if (!hasKeyboardCap && seatData.keyboard.object) {
        wl_keyboard_destroy(seatData.keyboard.object);
        seatData.keyboard.object = nullptr;
    }

    // WL_SEAT_CAPABILITY_TOUCH
    const bool hasTouchCap = capabilities & WL_SEAT_CAPABILITY_TOUCH;
    if (hasTouchCap && !seatData.touch.object) {
        seatData.touch.object = wl_seat_get_touch(seat);
        wl_touch_add_listener(seatData.touch.object, &g_touchListener, data);
    }
    if (!hasTouchCap && seatData.touch.object) {
        wl_touch_destroy(seatData.touch.object);
        seatData.touch.object = nullptr;
    }
}

void RenderingBackendSTM::globalSeatName(void* data, struct wl_seat* seat, const char* name)
{
}

void RenderingBackendSTM::globalCallback(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t)
{
    auto backend = static_cast<RenderingBackendSTM*>(data);
    if (!std::strcmp(interface, "wl_compositor"))
        backend->m_compositor = static_cast<struct wl_compositor*>(wl_registry_bind(registry, name, &wl_compositor_interface, 1));
    if (!std::strcmp(interface, "wl_seat")) {
        backend->m_seat = static_cast<struct wl_seat*>(wl_registry_bind(registry, name, &wl_seat_interface, 4));
        backend->initializeSeatData();
    }
    if (!std::strcmp(interface, "wl_shell"))
        backend->m_shell = static_cast<struct wl_shell*>(wl_registry_bind(registry, name, &wl_shell_interface, 1));
}

void RenderingBackendSTM::initializeSeatData()
{
    // This should be inside a lock as it can cause a race condition
    if(m_seat && input_surface && !m_seatDataInitialized) {

        m_seatDataInitialized = true;
        wl_seat_add_listener(m_seat, &m_seatListener, this);

        m_seatData.xkb.context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        m_seatData.xkb.composeTable = xkb_compose_table_new_from_locale(m_seatData.xkb.context, setlocale(LC_CTYPE, nullptr), XKB_COMPOSE_COMPILE_NO_FLAGS);
        if (m_seatData.xkb.composeTable)
            m_seatData.xkb.composeState = xkb_compose_state_new(m_seatData.xkb.composeTable, XKB_COMPOSE_STATE_NO_FLAGS);
    }
}

void RenderingBackendSTM::globalRemoveCallback(void*, struct wl_registry*, uint32_t)
{
    // FIXME: if this can happen without the UI Process getting shut down
    // we should probably destroy our cached display instance.
}

static wl_display* g_wldisplay = nullptr;
RenderingBackendSTM::RenderingBackendSTM()
    : input_surface(nullptr)
    , m_seatDataInitialized(false)
{
    m_display = wl_display_connect(nullptr);

    g_wldisplay = m_display;
    m_registry = wl_display_get_registry(m_display);
    wl_registry_add_listener(m_registry, &m_registryListener, this);
    wl_display_roundtrip(m_display);

    m_eventSource = g_source_new(&EventSource::sourceFuncs1, sizeof(EventSource));
    auto* source = reinterpret_cast<EventSource*>(m_eventSource);
    source->display = m_display;

    source->pfd.fd = wl_display_get_fd(m_display);
    source->pfd.events = G_IO_IN | G_IO_ERR | G_IO_HUP;
    source->pfd.revents = 0;
    g_source_add_poll(m_eventSource, &source->pfd);

    g_source_set_name(m_eventSource, "[WPE] PlatformDisplayWPE");
    g_source_set_priority(m_eventSource, G_PRIORITY_HIGH + 30);
    g_source_set_can_recurse(m_eventSource, TRUE);
    g_source_attach(m_eventSource, g_main_context_get_thread_default());
}

RenderingBackendSTM::~RenderingBackendSTM()
{
    if (m_eventSource)
        g_source_unref(m_eventSource);
    m_eventSource = nullptr;

    if (m_seatData.pointer.object)
        wl_pointer_destroy(m_seatData.pointer.object);
    if (m_seatData.keyboard.object)
        wl_keyboard_destroy(m_seatData.keyboard.object);
    if (m_seatData.touch.object)
        wl_touch_destroy(m_seatData.touch.object);
    if (m_seatData.xkb.context)
        xkb_context_unref(m_seatData.xkb.context);
    if (m_seatData.xkb.keymap)
        xkb_keymap_unref(m_seatData.xkb.keymap);
    if (m_seatData.xkb.state)
        xkb_state_unref(m_seatData.xkb.state);
    if (m_seatData.xkb.composeTable)
        xkb_compose_table_unref(m_seatData.xkb.composeTable);
    if (m_seatData.xkb.composeState)
        xkb_compose_state_unref(m_seatData.xkb.composeState);
    if (m_seatData.repeatData.eventSource)
        g_source_remove(m_seatData.repeatData.eventSource);
    m_seatData = SeatData{ };


    if (m_compositor)
        wl_compositor_destroy(m_compositor);
    if (m_seat)
        wl_seat_destroy(m_seat);
    if (m_registry)
        wl_registry_destroy(m_registry);
    g_wldisplay = nullptr;
    if (m_display)
        wl_display_disconnect(m_display);

    m_eventSource = nullptr;
    m_compositor = nullptr;
    m_seat = nullptr;
    m_registry = nullptr;
    m_display = nullptr;
}

EGLNativeDisplayType RenderingBackendSTM::nativeDisplay()
{
    return m_display;
}

std::unique_ptr<RenderingBackend::Surface> RenderingBackendSTM::createSurface(uint32_t width, uint32_t height, uint32_t targetHandle, RenderingBackend::Surface::Client& client)
{
    return std::unique_ptr<RenderingBackendSTM::Surface>(new RenderingBackendSTM::Surface(*this, width, height, targetHandle, client));
}

std::unique_ptr<RenderingBackend::OffscreenSurface> RenderingBackendSTM::createOffscreenSurface()
{
    return std::unique_ptr<RenderingBackendSTM::OffscreenSurface>(new RenderingBackendSTM::OffscreenSurface(*this));
}

RenderingBackendSTM::Surface::Surface(const RenderingBackendSTM& backend, uint32_t width, uint32_t height, uint32_t, RenderingBackendSTM::Surface::Client& client)
    : m_client(client)
{
    m_surface = wl_compositor_create_surface(backend.m_compositor);

    struct wl_shell_surface *shell_surface;
    shell_surface = wl_shell_get_shell_surface(backend.m_shell, m_surface);

    if (shell_surface)
            wl_shell_surface_add_listener(shell_surface,
                            &shell_surface_listener, NULL);
    wl_shell_surface_set_toplevel(shell_surface);

    struct wl_region *region;
    region = wl_compositor_create_region(backend.m_compositor);
    wl_region_add(region, 0, 0,
                   width,
                   height);
    wl_surface_set_opaque_region(m_surface, region);



    if (m_surface) {
        backend.setInputSurface(this);
        backend.initializeSeatData();
        m_window = wl_egl_window_create(m_surface, width, height);
    }
}

RenderingBackendSTM::Surface::~Surface()
{
    if (m_window)
        wl_egl_window_destroy(m_window);
    if (m_surface)
        wl_surface_destroy(m_surface);
}

EGLNativeWindowType RenderingBackendSTM::Surface::nativeWindow()
{
    return m_window;
}

void RenderingBackendSTM::Surface::resize(uint32_t, uint32_t)
{
}

RenderingBackend::BufferExport RenderingBackendSTM::Surface::lockFrontBuffer()
{
    if(g_wldisplay)
        wl_display_flush(g_wldisplay);
    return std::make_tuple(-1, nullptr, 0);
}

void RenderingBackendSTM::Surface::releaseBuffer(uint32_t)
{
}

RenderingBackendSTM::OffscreenSurface::OffscreenSurface(const RenderingBackendSTM&)
{
}

RenderingBackendSTM::OffscreenSurface::~OffscreenSurface() = default;

EGLNativeWindowType RenderingBackendSTM::OffscreenSurface::nativeWindow()
{
    return nullptr;
}

} // namespace Graphics

} // namespace WPE
#endif // WPE_BACKEND(STM);
