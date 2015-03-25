/*
 * Copyright (C) 2015 Igalia S.L.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "KeyInputHandlerXKB.h"

namespace WPE {

std::unique_ptr<KeyInputHandler> KeyInputHandler::create()
{
    return std::make_unique<KeyInputHandlerXKB>();
}

KeyInputHandlerXKB::KeyInputHandlerXKB()
{
    struct xkb_context* context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    struct xkb_rule_names names = { "evdev", "pc105", "us", "", "" };

    m_xkbKeymap = xkb_keymap_new_from_names(context, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
    m_xkbState = xkb_state_new(m_xkbKeymap);

    m_modifiers.ctrl = xkb_keymap_mod_get_index(m_xkbKeymap, XKB_MOD_NAME_CTRL);
    m_modifiers.shift = xkb_keymap_mod_get_index(m_xkbKeymap, XKB_MOD_NAME_SHIFT);
    m_modifiers.effective = 0;

    xkb_context_unref(context);
}

KeyInputHandlerXKB::~KeyInputHandlerXKB()
{
    xkb_keymap_unref(m_xkbKeymap);
    xkb_state_unref(m_xkbState);
}

KeyInputHandler::HandlingResult KeyInputHandlerXKB::handleKeyInputEvent(const KeyboardEvent::Raw& event)
{
    // Keycode system starts at 8. Go figure.
    int key = event.key + 8;

    uint8_t keyModifiers = 0;
    if (m_modifiers.effective & (1 << m_modifiers.ctrl))
        keyModifiers |= KeyboardEvent::Control;
    if (m_modifiers.effective & (1 << m_modifiers.shift))
        keyModifiers |= KeyboardEvent::Shift;

    HandlingResult result = {
        xkb_state_key_get_one_sym(m_xkbState, key),
        xkb_state_key_get_utf32(m_xkbState, key),
        keyModifiers
    };

    xkb_state_update_key(m_xkbState, key, event.state ? XKB_KEY_DOWN : XKB_KEY_UP);
    m_modifiers.effective = xkb_state_serialize_mods(m_xkbState,
        static_cast<xkb_state_component>(XKB_STATE_MODS_EFFECTIVE | XKB_STATE_LAYOUT_EFFECTIVE));

    return result;
}

} // namespace WPE
