/*
 * Copyright (C) 2015, 2016 Igalia S.L.
 * Copyright (C) 2015, 2016 Metrological
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if defined(KEY_INPUT_HANDLING_LINUX_INPUT) && KEY_INPUT_HANDLING_LINUX_INPUT

#include "KeyboardEventHandler.h"

#include <linux/input.h>
#include <mutex>

namespace WPE {

namespace Input {

class KeyboardEventHandlerLinuxInput final : public KeyboardEventHandler {
public:
    KeyboardEventHandlerLinuxInput() = default;
    virtual ~KeyboardEventHandlerLinuxInput() = default;

    Result handleKeyboardEvent(struct wpe_input_keyboard_event*) override;

private:
    struct Modifiers {
        bool ctrl;
        bool shift;
        bool alt;
        bool meta;
    } m_modifiers { false, false, false, false };
};

std::unique_ptr<KeyboardEventHandler> KeyboardEventHandler::create()
{
    return std::unique_ptr<KeyboardEventHandlerLinuxInput>(new KeyboardEventHandlerLinuxInput);
}

static uint32_t keyCodeToUTF32(uint32_t key, bool state)
{
    struct Entry {
        uint32_t lowercase;
        uint32_t uppercase;
    };
    static Entry table[KEY_MAX + 1] = { { 0, 0 } };
    std::once_flag flag;
    std::call_once(flag, [] {
        table[ KEY_ESC        ] /* 1   */ = { 0x001B                                     , 0 };
        table[ KEY_1          ] /* 2   */ = { 0x0031 /* U+0031 1 Digit One            */ , 0 };
        table[ KEY_2          ] /* 3   */ = { 0x0032 /* U+0032 2 Digit Two            */ , 0 };
        table[ KEY_3          ] /* 4   */ = { 0x0033 /* U+0033 3 Digit Three          */ , 0 };
        table[ KEY_4          ] /* 5   */ = { 0x0034 /* U+0034 4 Digit Four           */ , 0 };
        table[ KEY_5          ] /* 6   */ = { 0x0035 /* U+0035 5 Digit Five           */ , 0 };
        table[ KEY_6          ] /* 7   */ = { 0x0036 /* U+0036 6 Digit Six            */ , 0 };
        table[ KEY_7          ] /* 8   */ = { 0x0037 /* U+0037 7 Digit Seven          */ , 0 };
        table[ KEY_8          ] /* 9   */ = { 0x0038 /* U+0038 8 Digit Eight          */ , 0 };
        table[ KEY_9          ] /* 10  */ = { 0x0039 /* U+0039 9 Digit Nine           */ , 0 };
        table[ KEY_0          ] /* 11  */ = { 0x0030 /* U+0030 0 Digit Zero           */ , 0 };
        table[ KEY_MINUS      ] /* 12  */ = { 0x002D /* U+002D - Hyphen-minus         */ , 0 };
        table[ KEY_EQUAL      ] /* 13  */ = { 0x003D /* U+003D = Equal sign           */ , 0 };
        table[ KEY_BACKSPACE  ] /* 14  */ = { 0x0008                                     , 0 };
        table[ KEY_TAB        ] /* 15  */ = { 0x0009 /* U+0009 Horizontal tab HT      */ , 0 };
        table[ KEY_Q          ] /* 16  */ = { 0x0071 /* U+0071 q Latin Small Letter Q */ , 0x0051 /* U+0051 Q Latin Capital letter Q */ };
        table[ KEY_W          ] /* 17  */ = { 0x0077 /* U+0077 w Latin Small Letter W */ , 0x0057 /* U+0057 W Latin Capital letter W */ };
        table[ KEY_E          ] /* 18  */ = { 0x0065 /* U+0065 e Latin Small Letter E */ , 0x0045 /* U+0045 E Latin Capital letter E */ };
        table[ KEY_R          ] /* 19  */ = { 0x0072 /* U+0072 r Latin Small Letter R */ , 0x0052 /* U+0052 R Latin Capital letter R */ };
        table[ KEY_T          ] /* 20  */ = { 0x0074 /* U+0074 t Latin Small Letter T */ , 0x0054 /* U+0054 T Latin Capital letter T */ };
        table[ KEY_Y          ] /* 21  */ = { 0x0079 /* U+0079 y Latin Small Letter Y */ , 0x0059 /* U+0059 Y Latin Capital letter Y */ };
        table[ KEY_U          ] /* 22  */ = { 0x0075 /* U+0075 u Latin Small Letter U */ , 0x0055 /* U+0055 U Latin Capital letter U */ };
        table[ KEY_I          ] /* 23  */ = { 0x0069 /* U+0069 i Latin Small Letter I */ , 0x0049 /* U+0049 I Latin Capital letter I */ };
        table[ KEY_O          ] /* 24  */ = { 0x006F /* U+006F o Latin Small Letter O */ , 0x004F /* U+004F O Latin Capital letter O */ };
        table[ KEY_P          ] /* 25  */ = { 0x0070 /* U+0070 p Latin Small Letter P */ , 0x0050 /* U+0050 P Latin Capital letter P */ };
        table[ KEY_LEFTBRACE  ] /* 26  */ = { 0x007B /* U+007B Left Curly Bracket     */ , 0 };
        table[ KEY_RIGHTBRACE ] /* 27  */ = { 0x007D /* U+007D } Right Curly Bracket  */ , 0 };

        table[ KEY_A          ] /* 30  */ = { 0x0061 /* U+0061 a Latin Small Letter A */ , 0x0041 /* U+0041 A Latin Capital letter A */ };
        table[ KEY_S          ] /* 31  */ = { 0x0073 /* U+0073 s Latin Small Letter S */ , 0x0053 /* U+0053 S Latin Capital letter S */ };
        table[ KEY_D          ] /* 32  */ = { 0x0064 /* U+0064 d Latin Small Letter D */ , 0x0044 /* U+0044 D Latin Capital letter D */ };
        table[ KEY_F          ] /* 33  */ = { 0x0066 /* U+0066 f Latin Small Letter F */ , 0x0046 /* U+0046 F Latin Capital letter F */ };
        table[ KEY_G          ] /* 34  */ = { 0x0067 /* U+0067 g Latin Small Letter G */ , 0x0047 /* U+0047 G Latin Capital letter G */ };
        table[ KEY_H          ] /* 35  */ = { 0x0068 /* U+0068 h Latin Small Letter H */ , 0x0048 /* U+0048 H Latin Capital letter H */ };
        table[ KEY_J          ] /* 36  */ = { 0x006A /* U+006A j Latin Small Letter J */ , 0x004A /* U+004A J Latin Capital letter J */ };
        table[ KEY_K          ] /* 37  */ = { 0x006B /* U+006B k Latin Small Letter K */ , 0x004B /* U+004B K Latin Capital letter K */ };
        table[ KEY_L          ] /* 38  */ = { 0x006C /* U+006C l Latin Small Letter L */ , 0x004C /* U+004C L Latin Capital letter L */ };
        table[ KEY_SEMICOLON  ] /* 39  */ = { 0x003B /* U+003B ; Semicolon            */ , 0 };
        table[ KEY_APOSTROPHE ] /* 40  */ = { 0x0027 /* U+0027 ' Apostrophe           */ , 0 };
        table[ KEY_GRAVE      ] /* 41  */ = { 0x0060 /* U+0060 ` Grave accent         */ , 0 };

        table[ KEY_BACKSLASH  ] /* 43  */ = { 0x005C /* U+005C \ Backslash            */ , 0 };
        table[ KEY_Z          ] /* 44  */ = { 0x007A /* U+007A z Latin Small Letter Z */ , 0x005A /* U+005A Z Latin Capital letter Z */ };
        table[ KEY_X          ] /* 45  */ = { 0x0078 /* U+0078 x Latin Small Letter X */ , 0x0058 /* U+0058 X Latin Capital letter X */ };
        table[ KEY_C          ] /* 46  */ = { 0x0063 /* U+0063 c Latin Small Letter C */ , 0x0043 /* U+0043 C Latin Capital letter C */ };
        table[ KEY_V          ] /* 47  */ = { 0x0076 /* U+0076 v Latin Small Letter V */ , 0x0056 /* U+0056 V Latin Capital letter V */ };
        table[ KEY_B          ] /* 48  */ = { 0x0062 /* U+0062 b Latin Small Letter B */ , 0x0042 /* U+0042 B Latin Capital letter B */ };
        table[ KEY_N          ] /* 49  */ = { 0x006E /* U+006E n Latin Small Letter N */ , 0x004E /* U+004E N Latin Capital letter N */ };
        table[ KEY_M          ] /* 50  */ = { 0x006D /* U+006D m Latin Small Letter M */ , 0x004D /* U+004D M Latin Capital letter M */ };
        table[ KEY_COMMA      ] /* 51  */ = { 0x002C /* U+002C , 0, Comma             */ , 0 };
        table[ KEY_DOT        ] /* 52  */ = { 0x002E /* U+002E . Full stop            */ , 0 };
        table[ KEY_SLASH      ] /* 53  */ = { 0x002F /* U+002F / Slash                */ , 0 };

        table[ KEY_SPACE      ] /* 57  */ = { 0x0020 /* U+0020 Space SP               */ , 0 };
    });

    auto& entry = table[key];
    return state ? entry.uppercase : entry.lowercase;
};

KeyboardEventHandler::Result KeyboardEventHandlerLinuxInput::handleKeyboardEvent(struct wpe_input_keyboard_event* event)
{
    switch (event->keyCode) {
    case KEY_LEFTCTRL:
    case KEY_RIGHTCTRL:
        m_modifiers.ctrl = event->pressed;
        break;
    case KEY_LEFTSHIFT:
    case KEY_RIGHTSHIFT:
        m_modifiers.shift = event->pressed;
        break;
    case KEY_LEFTALT:
    case KEY_RIGHTALT:
        m_modifiers.alt = event->pressed;
        break;
    case KEY_LEFTMETA:
    case KEY_RIGHTMETA:
        m_modifiers.meta = event->pressed;
        break;
    default:
        break;
    }

    uint8_t keyModifiers = 0;
    if (m_modifiers.ctrl)
        keyModifiers |= wpe_input_keyboard_modifier_control;
    if (m_modifiers.shift)
        keyModifiers |= wpe_input_keyboard_modifier_shift;
    if (m_modifiers.alt)
        keyModifiers |= wpe_input_keyboard_modifier_alt;
    if (m_modifiers.meta)
        keyModifiers |= wpe_input_keyboard_modifier_meta;

    // TODO: Range check.

    return Result{ event->keyCode, keyCodeToUTF32(event->keyCode, m_modifiers.shift), keyModifiers };
}

} // namespace Input

} // namespace WPE

#endif // defined(KEY_INPUT_HANDLING_LINUX_INPUT) && KEY_INPUT_HANDLING_LINUX_INPUT
