/*
 * Copyright (C) 2006-2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebInputEventFactory.h"

#include "KeyboardCodes.h"
#include "KeyCodeConversion.h"

#include "WebInputEvent.h"

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <wtf/Assertions.h>

namespace {

// For click count tracking.
static int gNumClicks = 0;
static GdkWindow* gLastClickEventWindow = 0;
static gint gLastClickTime = 0;
static gint gLastClickX = 0;
static gint gLastClickY = 0;
static WebKit::WebMouseEvent::Button gLastClickButton = WebKit::WebMouseEvent::ButtonNone;

bool shouldForgetPreviousClick(GdkWindow* window, gint time, gint x, gint y)
{
    static GtkSettings* settings = gtk_settings_get_default();

    if (window != gLastClickEventWindow)
      return true;

    gint doubleClickTime = 250;
    gint doubleClickDistance = 5;
    g_object_get(G_OBJECT(settings),
                 "gtk-double-click-time", &doubleClickTime,
                 "gtk-double-click-distance", &doubleClickDistance, NULL);
    return (time - gLastClickTime) > doubleClickTime
           || abs(x - gLastClickX) > doubleClickDistance
           || abs(y - gLastClickY) > doubleClickDistance;
}

void resetClickCountState()
{
    gNumClicks = 0;
    gLastClickEventWindow = 0;
    gLastClickTime = 0;
    gLastClickX = 0;
    gLastClickY = 0;
    gLastClickButton = WebKit::WebMouseEvent::ButtonNone;
}

bool isKeyPadKeyval(guint keyval)
{
    // Keypad keyvals all fall into one range.
    return keyval >= GDK_KP_Space && keyval <= GDK_KP_9;
}

}  // namespace

namespace WebKit {

static double gdkEventTimeToWebEventTime(guint32 time)
{
    // Convert from time in ms to time in sec.
    return time / 1000.0;
}

static int gdkStateToWebEventModifiers(guint state)
{
    int modifiers = 0;
    if (state & GDK_SHIFT_MASK)
        modifiers |= WebInputEvent::ShiftKey;
    if (state & GDK_CONTROL_MASK)
        modifiers |= WebInputEvent::ControlKey;
    if (state & GDK_MOD1_MASK)
        modifiers |= WebInputEvent::AltKey;
    if (state & GDK_META_MASK)
        modifiers |= WebInputEvent::MetaKey;
    if (state & GDK_BUTTON1_MASK)
        modifiers |= WebInputEvent::LeftButtonDown;
    if (state & GDK_BUTTON2_MASK)
        modifiers |= WebInputEvent::MiddleButtonDown;
    if (state & GDK_BUTTON3_MASK)
        modifiers |= WebInputEvent::RightButtonDown;
    if (state & GDK_LOCK_MASK)
        modifiers |= WebInputEvent::CapsLockOn;
    if (state & GDK_MOD2_MASK)
        modifiers |= WebInputEvent::NumLockOn;
    return modifiers;
}

static int gdkEventToWindowsKeyCode(const GdkEventKey* event)
{
    static const unsigned int hardwareCodeToGDKKeyval[] = {
        0,                 // 0x00:
        0,                 // 0x01:
        0,                 // 0x02:
        0,                 // 0x03:
        0,                 // 0x04:
        0,                 // 0x05:
        0,                 // 0x06:
        0,                 // 0x07:
        0,                 // 0x08:
        0,                 // 0x09: GDK_Escape
        GDK_1,             // 0x0A: GDK_1
        GDK_2,             // 0x0B: GDK_2
        GDK_3,             // 0x0C: GDK_3
        GDK_4,             // 0x0D: GDK_4
        GDK_5,             // 0x0E: GDK_5
        GDK_6,             // 0x0F: GDK_6
        GDK_7,             // 0x10: GDK_7
        GDK_8,             // 0x11: GDK_8
        GDK_9,             // 0x12: GDK_9
        GDK_0,             // 0x13: GDK_0
        GDK_minus,         // 0x14: GDK_minus
        GDK_equal,         // 0x15: GDK_equal
        0,                 // 0x16: GDK_BackSpace
        0,                 // 0x17: GDK_Tab
        GDK_q,             // 0x18: GDK_q
        GDK_w,             // 0x19: GDK_w
        GDK_e,             // 0x1A: GDK_e
        GDK_r,             // 0x1B: GDK_r
        GDK_t,             // 0x1C: GDK_t
        GDK_y,             // 0x1D: GDK_y
        GDK_u,             // 0x1E: GDK_u
        GDK_i,             // 0x1F: GDK_i
        GDK_o,             // 0x20: GDK_o
        GDK_p,             // 0x21: GDK_p
        GDK_bracketleft,   // 0x22: GDK_bracketleft
        GDK_bracketright,  // 0x23: GDK_bracketright
        0,                 // 0x24: GDK_Return
        0,                 // 0x25: GDK_Control_L
        GDK_a,             // 0x26: GDK_a
        GDK_s,             // 0x27: GDK_s
        GDK_d,             // 0x28: GDK_d
        GDK_f,             // 0x29: GDK_f
        GDK_g,             // 0x2A: GDK_g
        GDK_h,             // 0x2B: GDK_h
        GDK_j,             // 0x2C: GDK_j
        GDK_k,             // 0x2D: GDK_k
        GDK_l,             // 0x2E: GDK_l
        GDK_semicolon,     // 0x2F: GDK_semicolon
        GDK_apostrophe,    // 0x30: GDK_apostrophe
        GDK_grave,         // 0x31: GDK_grave
        0,                 // 0x32: GDK_Shift_L
        GDK_backslash,     // 0x33: GDK_backslash
        GDK_z,             // 0x34: GDK_z
        GDK_x,             // 0x35: GDK_x
        GDK_c,             // 0x36: GDK_c
        GDK_v,             // 0x37: GDK_v
        GDK_b,             // 0x38: GDK_b
        GDK_n,             // 0x39: GDK_n
        GDK_m,             // 0x3A: GDK_m
        GDK_comma,         // 0x3B: GDK_comma
        GDK_period,        // 0x3C: GDK_period
        GDK_slash,         // 0x3D: GDK_slash
        0,                 // 0x3E: GDK_Shift_R
        0,                 // 0x3F:
        0,                 // 0x40:
        0,                 // 0x41:
        0,                 // 0x42:
        0,                 // 0x43:
        0,                 // 0x44:
        0,                 // 0x45:
        0,                 // 0x46:
        0,                 // 0x47:
        0,                 // 0x48:
        0,                 // 0x49:
        0,                 // 0x4A:
        0,                 // 0x4B:
        0,                 // 0x4C:
        0,                 // 0x4D:
        0,                 // 0x4E:
        0,                 // 0x4F:
        0,                 // 0x50:
        0,                 // 0x51:
        0,                 // 0x52:
        0,                 // 0x53:
        0,                 // 0x54:
        0,                 // 0x55:
        0,                 // 0x56:
        0,                 // 0x57:
        0,                 // 0x58:
        0,                 // 0x59:
        0,                 // 0x5A:
        0,                 // 0x5B:
        0,                 // 0x5C:
        0,                 // 0x5D:
        0,                 // 0x5E:
        0,                 // 0x5F:
        0,                 // 0x60:
        0,                 // 0x61:
        0,                 // 0x62:
        0,                 // 0x63:
        0,                 // 0x64:
        0,                 // 0x65:
        0,                 // 0x66:
        0,                 // 0x67:
        0,                 // 0x68:
        0,                 // 0x69:
        0,                 // 0x6A:
        0,                 // 0x6B:
        0,                 // 0x6C:
        0,                 // 0x6D:
        0,                 // 0x6E:
        0,                 // 0x6F:
        0,                 // 0x70:
        0,                 // 0x71:
        0,                 // 0x72:
        GDK_Super_L,       // 0x73: GDK_Super_L
        GDK_Super_R,       // 0x74: GDK_Super_R
    };

    // |windowsKeyCode| has to include a valid virtual-key code even when we
    // use non-US layouts, e.g. even when we type an 'A' key of a US keyboard
    // on the Hebrew layout, |windowsKeyCode| should be VK_A.
    // On the other hand, |event->keyval| value depends on the current
    // GdkKeymap object, i.e. when we type an 'A' key of a US keyboard on
    // the Hebrew layout, |event->keyval| becomes GDK_hebrew_shin and this
    // WebCore::windowsKeyCodeForKeyEvent() call returns 0.
    // To improve compatibilty with Windows, we use |event->hardware_keycode|
    // for retrieving its Windows key-code for the keys when the
    // WebCore::windowsKeyCodeForEvent() call returns 0.
    // We shouldn't use |event->hardware_keycode| for keys that GdkKeymap
    // objects cannot change because |event->hardware_keycode| doesn't change
    // even when we change the layout options, e.g. when we swap a control
    // key and a caps-lock key, GTK doesn't swap their
    // |event->hardware_keycode| values but swap their |event->keyval| values.
    int windowsKeyCode = WebCore::windowsKeyCodeForKeyEvent(event->keyval);
    if (windowsKeyCode)
        return windowsKeyCode;

    const int tableSize = sizeof(hardwareCodeToGDKKeyval) / sizeof(hardwareCodeToGDKKeyval[0]);
    if (event->hardware_keycode < tableSize) {
        int keyval = hardwareCodeToGDKKeyval[event->hardware_keycode];
        if (keyval)
            return WebCore::windowsKeyCodeForKeyEvent(keyval);
    }

    // This key is one that keyboard-layout drivers cannot change.
    // Use |event->keyval| to retrieve its |windowsKeyCode| value.
    return WebCore::windowsKeyCodeForKeyEvent(event->keyval);
}

// Normalizes event->state to make it Windows/Mac compatible. Since the way
// of setting modifier mask on X is very different than Windows/Mac as shown
// in http://crbug.com/127142#c8, the normalization is necessary.
static guint normalizeEventState(const GdkEventKey* event)
{
    guint mask = 0;
    switch (gdkEventToWindowsKeyCode(event)) {
    case WebCore::VKEY_CONTROL:
    case WebCore::VKEY_LCONTROL:
    case WebCore::VKEY_RCONTROL:
        mask = GDK_CONTROL_MASK;
        break;
    case WebCore::VKEY_SHIFT:
    case WebCore::VKEY_LSHIFT:
    case WebCore::VKEY_RSHIFT:
        mask = GDK_SHIFT_MASK;
        break;
    case WebCore::VKEY_MENU:
    case WebCore::VKEY_LMENU:
    case WebCore::VKEY_RMENU:
        mask = GDK_MOD1_MASK;
        break;
    case WebCore::VKEY_CAPITAL:
        mask = GDK_LOCK_MASK;
        break;
    default:
        return event->state;
    }
    if (event->type == GDK_KEY_PRESS)
        return event->state | mask;
    return event->state & ~mask;
}

// Gets the corresponding control character of a specified key code. See:
// http://en.wikipedia.org/wiki/Control_characters
// We emulate Windows behavior here.
static WebUChar getControlCharacter(int windowsKeyCode, bool shift)
{
    if (windowsKeyCode >= WebCore::VKEY_A && windowsKeyCode <= WebCore::VKEY_Z) {
        // ctrl-A ~ ctrl-Z map to \x01 ~ \x1A
        return windowsKeyCode - WebCore::VKEY_A + 1;
    }
    if (shift) {
        // following graphics chars require shift key to input.
        switch (windowsKeyCode) {
        // ctrl-@ maps to \x00 (Null byte)
        case WebCore::VKEY_2:
            return 0;
        // ctrl-^ maps to \x1E (Record separator, Information separator two)
        case WebCore::VKEY_6:
            return 0x1E;
        // ctrl-_ maps to \x1F (Unit separator, Information separator one)
        case WebCore::VKEY_OEM_MINUS:
            return 0x1F;
        // Returns 0 for all other keys to avoid inputting unexpected chars.
        default:
            return 0;
        }
    } else {
        switch (windowsKeyCode) {
        // ctrl-[ maps to \x1B (Escape)
        case WebCore::VKEY_OEM_4:
            return 0x1B;
        // ctrl-\ maps to \x1C (File separator, Information separator four)
        case WebCore::VKEY_OEM_5:
            return 0x1C;
        // ctrl-] maps to \x1D (Group separator, Information separator three)
        case WebCore::VKEY_OEM_6:
            return 0x1D;
        // ctrl-Enter maps to \x0A (Line feed)
        case WebCore::VKEY_RETURN:
            return 0x0A;
        // Returns 0 for all other keys to avoid inputting unexpected chars.
        default:
            return 0;
        }
    }
}

// WebKeyboardEvent -----------------------------------------------------------

WebKeyboardEvent WebInputEventFactory::keyboardEvent(const GdkEventKey* event)
{
    WebKeyboardEvent result;

    result.timeStampSeconds = gdkEventTimeToWebEventTime(event->time);
    result.modifiers = gdkStateToWebEventModifiers(normalizeEventState(event));

    switch (event->type) {
    case GDK_KEY_RELEASE:
        result.type = WebInputEvent::KeyUp;
        break;
    case GDK_KEY_PRESS:
        result.type = WebInputEvent::RawKeyDown;
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    // According to MSDN:
    // http://msdn.microsoft.com/en-us/library/ms646286(VS.85).aspx
    // Key events with Alt modifier and F10 are system key events.
    // We just emulate this behavior. It's necessary to prevent webkit from
    // processing keypress event generated by alt-d, etc.
    // F10 is not special on Linux, so don't treat it as system key.
    if (result.modifiers & WebInputEvent::AltKey)
        result.isSystemKey = true;

    // The key code tells us which physical key was pressed (for example, the
    // A key went down or up).  It does not determine whether A should be lower
    // or upper case.  This is what text does, which should be the keyval.
    int windowsKeyCode = gdkEventToWindowsKeyCode(event);
    result.windowsKeyCode = WebKeyboardEvent::windowsKeyCodeWithoutLocation(windowsKeyCode);
    result.modifiers |= WebKeyboardEvent::locationModifiersFromWindowsKeyCode(windowsKeyCode);
    result.nativeKeyCode = event->hardware_keycode;

    if (result.windowsKeyCode == WebCore::VKEY_RETURN)
        // We need to treat the enter key as a key press of character \r.  This
        // is apparently just how webkit handles it and what it expects.
        result.unmodifiedText[0] = '\r';
    else
        // FIXME: fix for non BMP chars
        result.unmodifiedText[0] =
            static_cast<WebUChar>(gdk_keyval_to_unicode(event->keyval));

    // If ctrl key is pressed down, then control character shall be input.
    if (result.modifiers & WebInputEvent::ControlKey)
        result.text[0] = getControlCharacter(
            result.windowsKeyCode, result.modifiers & WebInputEvent::ShiftKey);
    else
        result.text[0] = result.unmodifiedText[0];

    result.setKeyIdentifierFromWindowsKeyCode();

    // FIXME: Do we need to set IsAutoRepeat?
    if (isKeyPadKeyval(event->keyval))
        result.modifiers |= WebInputEvent::IsKeyPad;

    return result;
}

WebKeyboardEvent WebInputEventFactory::keyboardEvent(wchar_t character, int state, double timeStampSeconds)
{
    // keyboardEvent(const GdkEventKey*) depends on the GdkEventKey object and
    // it is hard to use/ it from signal handlers which don't use GdkEventKey
    // objects (e.g. GtkIMContext signal handlers.) For such handlers, this
    // function creates a WebInputEvent::Char event without using a
    // GdkEventKey object.
    WebKeyboardEvent result;
    result.type = WebKit::WebInputEvent::Char;
    result.timeStampSeconds = timeStampSeconds;
    result.modifiers = gdkStateToWebEventModifiers(state);
    result.windowsKeyCode = character;
    result.nativeKeyCode = character;
    result.text[0] = character;
    result.unmodifiedText[0] = character;

    // According to MSDN:
    // http://msdn.microsoft.com/en-us/library/ms646286(VS.85).aspx
    // Key events with Alt modifier and F10 are system key events.
    // We just emulate this behavior. It's necessary to prevent webkit from
    // processing keypress event generated by alt-d, etc.
    // F10 is not special on Linux, so don't treat it as system key.
    if (result.modifiers & WebInputEvent::AltKey)
        result.isSystemKey = true;

    return result;
}

// WebMouseEvent --------------------------------------------------------------

WebMouseEvent WebInputEventFactory::mouseEvent(const GdkEventButton* event)
{
    WebMouseEvent result;

    result.timeStampSeconds = gdkEventTimeToWebEventTime(event->time);

    result.modifiers = gdkStateToWebEventModifiers(event->state);
    result.x = static_cast<int>(event->x);
    result.y = static_cast<int>(event->y);
    result.windowX = result.x;
    result.windowY = result.y;
    result.globalX = static_cast<int>(event->x_root);
    result.globalY = static_cast<int>(event->y_root);
    result.clickCount = 0;

    switch (event->type) {
    case GDK_BUTTON_PRESS:
        result.type = WebInputEvent::MouseDown;
        break;
    case GDK_BUTTON_RELEASE:
        result.type = WebInputEvent::MouseUp;
        break;
    case GDK_3BUTTON_PRESS:
    case GDK_2BUTTON_PRESS:
    default:
        ASSERT_NOT_REACHED();
    };

    result.button = WebMouseEvent::ButtonNone;
    if (event->button == 1)
        result.button = WebMouseEvent::ButtonLeft;
    else if (event->button == 2)
        result.button = WebMouseEvent::ButtonMiddle;
    else if (event->button == 3)
        result.button = WebMouseEvent::ButtonRight;

    if (result.type == WebInputEvent::MouseDown) {
        bool forgetPreviousClick = shouldForgetPreviousClick(event->window, event->time, event->x, event->y);

        if (!forgetPreviousClick && result.button == gLastClickButton)
            ++gNumClicks;
        else {
            gNumClicks = 1;

            gLastClickEventWindow = event->window;
            gLastClickX = event->x;
            gLastClickY = event->y;
            gLastClickButton = result.button;
        }
        gLastClickTime = event->time;
    }
    result.clickCount = gNumClicks;

    return result;
}

WebMouseEvent WebInputEventFactory::mouseEvent(const GdkEventMotion* event)
{
    WebMouseEvent result;

    result.timeStampSeconds = gdkEventTimeToWebEventTime(event->time);
    result.modifiers = gdkStateToWebEventModifiers(event->state);
    result.x = static_cast<int>(event->x);
    result.y = static_cast<int>(event->y);
    result.windowX = result.x;
    result.windowY = result.y;
    result.globalX = static_cast<int>(event->x_root);
    result.globalY = static_cast<int>(event->y_root);

    switch (event->type) {
    case GDK_MOTION_NOTIFY:
        result.type = WebInputEvent::MouseMove;
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    result.button = WebMouseEvent::ButtonNone;
    if (event->state & GDK_BUTTON1_MASK)
        result.button = WebMouseEvent::ButtonLeft;
    else if (event->state & GDK_BUTTON2_MASK)
        result.button = WebMouseEvent::ButtonMiddle;
    else if (event->state & GDK_BUTTON3_MASK)
        result.button = WebMouseEvent::ButtonRight;

    if (shouldForgetPreviousClick(event->window, event->time, event->x, event->y))
        resetClickCountState();

    return result;
}

WebMouseEvent WebInputEventFactory::mouseEvent(const GdkEventCrossing* event)
{
    WebMouseEvent result;

    result.timeStampSeconds = gdkEventTimeToWebEventTime(event->time);
    result.modifiers = gdkStateToWebEventModifiers(event->state);
    result.x = static_cast<int>(event->x);
    result.y = static_cast<int>(event->y);
    result.windowX = result.x;
    result.windowY = result.y;
    result.globalX = static_cast<int>(event->x_root);
    result.globalY = static_cast<int>(event->y_root);

    switch (event->type) {
    case GDK_ENTER_NOTIFY:
    case GDK_LEAVE_NOTIFY:
        // Note that if we sent MouseEnter or MouseLeave to WebKit, it
        // wouldn't work - they don't result in the proper JavaScript events.
        // MouseMove does the right thing.
        result.type = WebInputEvent::MouseMove;
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    result.button = WebMouseEvent::ButtonNone;
    if (event->state & GDK_BUTTON1_MASK)
        result.button = WebMouseEvent::ButtonLeft;
    else if (event->state & GDK_BUTTON2_MASK)
        result.button = WebMouseEvent::ButtonMiddle;
    else if (event->state & GDK_BUTTON3_MASK)
        result.button = WebMouseEvent::ButtonRight;

    if (shouldForgetPreviousClick(event->window, event->time, event->x, event->y))
        resetClickCountState();

    return result;
}

// WebMouseWheelEvent ---------------------------------------------------------

WebMouseWheelEvent WebInputEventFactory::mouseWheelEvent(const GdkEventScroll* event)
{
    WebMouseWheelEvent result;

    result.type = WebInputEvent::MouseWheel;
    result.button = WebMouseEvent::ButtonNone;

    result.timeStampSeconds = gdkEventTimeToWebEventTime(event->time);
    result.modifiers = gdkStateToWebEventModifiers(event->state);
    result.x = static_cast<int>(event->x);
    result.y = static_cast<int>(event->y);
    result.windowX = result.x;
    result.windowY = result.y;
    result.globalX = static_cast<int>(event->x_root);
    result.globalY = static_cast<int>(event->y_root);

    // How much should we scroll per mouse wheel event?
    // - Windows uses 3 lines by default and obeys a system setting.
    // - Mozilla has a pref that lets you either use the "system" number of lines
    //   to scroll, or lets the user override it.
    //   For the "system" number of lines, it appears they've hardcoded 3.
    //   See case NS_MOUSE_SCROLL in content/events/src/nsEventStateManager.cpp
    //   and InitMouseScrollEvent in widget/src/gtk2/nsCommonWidget.cpp .
    // - Gtk makes the scroll amount a function of the size of the scroll bar,
    //   which is not available to us here.
    // Instead, we pick a number that empirically matches Firefox's behavior.
    static const float scrollbarPixelsPerTick = 160.0f / 3.0f;

    switch (event->direction) {
    case GDK_SCROLL_UP:
        result.deltaY = scrollbarPixelsPerTick;
        result.wheelTicksY = 1;
        break;
    case GDK_SCROLL_DOWN:
        result.deltaY = -scrollbarPixelsPerTick;
        result.wheelTicksY = -1;
        break;
    case GDK_SCROLL_LEFT:
        result.deltaX = scrollbarPixelsPerTick;
        result.wheelTicksX = 1;
        break;
    case GDK_SCROLL_RIGHT:
        result.deltaX = -scrollbarPixelsPerTick;
        result.wheelTicksX = -1;
        break;
    }

    return result;
}

} // namespace WebKit
