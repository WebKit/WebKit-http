#include "Config.h"

#if WPE_BACKEND(WESTEROS)

#include "ViewBackendWesteros.h"
#include "WesterosViewbackendInput.h"

#include <cstring>
#include <cassert>
#include <cstring>
#include <linux/input.h>
#include <locale.h>
#include <sys/mman.h>
#include <unistd.h>
#include <WPE/Input/Events.h>
#include <WPE/Input/Handling.h>

namespace WPE {

namespace ViewBackend {


static WstKeyboardNestedListener keyboard_listener = {
    WesterosViewbackendInput::keyboardHandleKeyMap,
    WesterosViewbackendInput::keyboardHandleEnter,
    WesterosViewbackendInput::keyboardHandleLeave,
    WesterosViewbackendInput::keyboardHandleKey,
    WesterosViewbackendInput::keyboardHandleModifiers,
    WesterosViewbackendInput::keyboardHandleRepeatInfo
};

static WstPointerNestedListener pointer_listener = {
    WesterosViewbackendInput::pointerHandleEnter,
    WesterosViewbackendInput::pointerHandleLeave,
    WesterosViewbackendInput::pointerHandleMotion,
    WesterosViewbackendInput::pointerHandleButton,
    WesterosViewbackendInput::pointerHandleAxis
};

void WesterosViewbackendInput::keyboardHandleKeyMap( void *userData, uint32_t format, int fd, uint32_t size )
{
    auto& backend_input = *static_cast<WesterosViewbackendInput*>(userData);
    auto& handlerData = backend_input.m_handlerData;

    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        close(fd);
        return;
    }

    void* mapping = mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
    if (mapping == MAP_FAILED) {
        close(fd);
        return;
    }

    auto& xkb = handlerData.xkb;
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
}

void WesterosViewbackendInput::keyboardHandleEnter( void *userData, struct wl_array *keys )
{
}

void WesterosViewbackendInput::keyboardHandleLeave( void *userData )
{
}

void WesterosViewbackendInput::keyboardHandleKey( void *userData, uint32_t time, uint32_t key, uint32_t state )
{
    auto& backend_input = *static_cast<WesterosViewbackendInput*>(userData);
    auto& handlerData = backend_input.m_handlerData;

    // IDK.
    key += 8;

    handleKeyEvent(userData, key, state, time);

    if (!handlerData.repeatInfo.rate)
        return;

    if (state == WL_KEYBOARD_KEY_STATE_RELEASED
        && handlerData.repeatData.key == key) {
        if (handlerData.repeatData.eventSource)
            g_source_remove(handlerData.repeatData.eventSource);
        handlerData.repeatData = { 0, 0, 0, 0 };
    } else if (state == WL_KEYBOARD_KEY_STATE_PRESSED
        && xkb_keymap_key_repeats(handlerData.xkb.keymap, key)) {

        if (handlerData.repeatData.eventSource)
            g_source_remove(handlerData.repeatData.eventSource);

        handlerData.repeatData = { key, time, state, g_timeout_add(handlerData.repeatInfo.delay, static_cast<GSourceFunc>(repeatDelayTimeout), userData) };
    }
}

void WesterosViewbackendInput::keyboardHandleModifiers( void *userData, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group )
{
    auto& backend_input = *static_cast<WesterosViewbackendInput*>(userData);
    auto& handlerData = backend_input.m_handlerData;

    auto& xkb = handlerData.xkb;
    xkb_state_update_mask(xkb.state, mods_depressed, mods_latched, mods_locked, 0, 0, group);

    auto& modifiers = xkb.modifiers;
    modifiers = 0;
    auto component = static_cast<xkb_state_component>(XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED);
    if (xkb_state_mod_index_is_active(xkb.state, xkb.indexes.control, component))
        modifiers |= Input::KeyboardEvent::Control;
    if (xkb_state_mod_index_is_active(xkb.state, xkb.indexes.alt, component))
        modifiers |= Input::KeyboardEvent::Alt;
    if (xkb_state_mod_index_is_active(xkb.state, xkb.indexes.shift, component))
        modifiers |= Input::KeyboardEvent::Shift;
}

void WesterosViewbackendInput::keyboardHandleRepeatInfo( void *userData, int32_t rate, int32_t delay )
{
    auto& backend_input = *static_cast<WesterosViewbackendInput*>(userData);
    auto& handlerData = backend_input.m_handlerData;

    auto& repeatInfo = handlerData.repeatInfo;
    repeatInfo = { rate, delay };

    // A rate of zero disables any repeating.
    if (!rate) {
        auto& repeatData = handlerData.repeatData;
        if (repeatData.eventSource) {
            g_source_remove(repeatData.eventSource);
            repeatData = { 0, 0, 0, 0 };
        }
    }
}

void WesterosViewbackendInput::handleKeyEvent(void* userData, uint32_t key, uint32_t state, uint32_t time)
{
    auto& backend_input = *static_cast<WesterosViewbackendInput*>(userData);
    auto& handlerData = backend_input.m_handlerData;

    auto& xkb = handlerData.xkb;
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

    backend_input.m_client->handleKeyboardEvent({time, keysym, unicode, !!state, xkb.modifiers});
}

gboolean WesterosViewbackendInput::repeatRateTimeout(void* userData)
{
    auto& backend_input = *static_cast<WesterosViewbackendInput*>(userData);
    auto& handlerData = backend_input.m_handlerData;

    handleKeyEvent(userData, handlerData.repeatData.key, handlerData.repeatData.state, handlerData.repeatData.time);
    return G_SOURCE_CONTINUE;
}

gboolean WesterosViewbackendInput::repeatDelayTimeout(void* userData)
{
    auto& backend_input = *static_cast<WesterosViewbackendInput*>(userData);
    auto& handlerData = backend_input.m_handlerData;

    handleKeyEvent(userData, handlerData.repeatData.key, handlerData.repeatData.state, handlerData.repeatData.time);
    handlerData.repeatData.eventSource = g_timeout_add(handlerData.repeatInfo.rate, static_cast<GSourceFunc>(repeatRateTimeout), userData);
    return G_SOURCE_REMOVE;
}

void WesterosViewbackendInput::pointerHandleEnter( void *userData, wl_fixed_t sx, wl_fixed_t sy )
{
}

void WesterosViewbackendInput::pointerHandleLeave( void *userData )
{
}

void WesterosViewbackendInput::pointerHandleMotion( void *userData, uint32_t time, wl_fixed_t sx, wl_fixed_t sy )
{
    auto& backend_input = *static_cast<WesterosViewbackendInput*>(userData);
    auto& handlerData = backend_input.m_handlerData;

    auto x = wl_fixed_to_int(sx);
    auto y = wl_fixed_to_int(sy);

    auto& pointer = handlerData.pointer;
    pointer.coords = { x, y };

    backend_input.m_client->handlePointerEvent({ Input::PointerEvent::Motion, time, x, y, 0, 0 });
}

void WesterosViewbackendInput::pointerHandleButton( void *userData, uint32_t time, uint32_t button, uint32_t state )
{
    auto& backend_input = *static_cast<WesterosViewbackendInput*>(userData);
    auto& handlerData = backend_input.m_handlerData;

    if (button >= BTN_MOUSE)
            button = button - BTN_MOUSE + 1;
        else
            button = 0;

    auto& pointer = handlerData.pointer;
    auto& coords = pointer.coords;

    backend_input.m_client->handlePointerEvent({ Input::PointerEvent::Button, time, coords.first, coords.second, button, state });
}

void WesterosViewbackendInput::pointerHandleAxis( void *userData, uint32_t time, uint32_t axis, wl_fixed_t value )
{
    auto& backend_input = *static_cast<WesterosViewbackendInput*>(userData);
    auto& handlerData = backend_input.m_handlerData;

    auto& pointer = handlerData.pointer;
    auto& coords = pointer.coords;

    backend_input.m_client->handleAxisEvent({ Input::AxisEvent::Motion, time, coords.first, coords.second, axis, -wl_fixed_to_int(value) });
}

WesterosViewbackendInput::WesterosViewbackendInput()
 : m_compositor(nullptr)
 , m_viewbackend(nullptr)
 , m_client(nullptr)
{
    m_handlerData.xkb.context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    m_handlerData.xkb.composeTable = xkb_compose_table_new_from_locale(m_handlerData.xkb.context, setlocale(LC_CTYPE, nullptr), XKB_COMPOSE_COMPILE_NO_FLAGS);
    if (m_handlerData.xkb.composeTable)
        m_handlerData.xkb.composeState = xkb_compose_state_new(m_handlerData.xkb.composeTable, XKB_COMPOSE_STATE_NO_FLAGS);
}

WesterosViewbackendInput::~WesterosViewbackendInput()
{
    m_compositor = nullptr;
    m_viewbackend = nullptr;
    m_client = nullptr;

    if (m_handlerData.xkb.context)
        xkb_context_unref(m_handlerData.xkb.context);
    if (m_handlerData.xkb.keymap)
        xkb_keymap_unref(m_handlerData.xkb.keymap);
    if (m_handlerData.xkb.state)
        xkb_state_unref(m_handlerData.xkb.state);
    if (m_handlerData.xkb.composeTable)
        xkb_compose_table_unref(m_handlerData.xkb.composeTable);
    if (m_handlerData.xkb.composeState)
        xkb_compose_state_unref(m_handlerData.xkb.composeState);
    if (m_handlerData.repeatData.eventSource)
        g_source_remove(m_handlerData.repeatData.eventSource);
}

void WesterosViewbackendInput::initializeNestedInputHandler(WstCompositor *compositor, ViewBackendWesteros *backend)
{
    m_compositor = compositor;
    m_viewbackend = backend;

    if (m_compositor && m_viewbackend)
    {
        if (!WstCompositorSetKeyboardNestedListener( m_compositor, &keyboard_listener, this )) {
            fprintf(stderr, "ViewBackendWesteros: failed to set keyboard nested listener: %s\n",
                WstCompositorGetLastErrorDetail(m_compositor));
        }
        if (!WstCompositorSetPointerNestedListener( m_compositor, &pointer_listener, this )) {
            fprintf(stderr, "WesterosViewbackendInput: failed to set pointer nested listener: %s\n",
                WstCompositorGetLastErrorDetail(m_compositor));
        }
    }
}

} // namespace ViewBackend

} // namespace WPE

#endif // WPE_BACKEND(WESTEROS)
