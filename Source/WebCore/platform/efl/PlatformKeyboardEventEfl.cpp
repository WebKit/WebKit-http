/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2008 Diego Hidalgo C. Gonzalez
 * Copyright (C) 2009-2010 ProFUSION embedded systems
 * Copyright (C) 2009-2010 Samsung Electronics
 *
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PlatformKeyboardEvent.h"

#include "EflKeyboardUtilities.h"
#include "NotImplemented.h"
#include "TextEncoding.h"
#include <Evas.h>
#include <stdio.h>

namespace WebCore {

PlatformKeyboardEvent::PlatformKeyboardEvent(const Evas_Event_Key_Down* event)
    : m_type(KeyDown)
    , m_text(String::fromUTF8(event->string))
    , m_unmodifiedText(String::fromUTF8(event->string))
    , m_shiftKey(evas_key_modifier_is_set(event->modifiers, "Shift"))
    , m_ctrlKey(evas_key_modifier_is_set(event->modifiers, "Control"))
    , m_altKey(evas_key_modifier_is_set(event->modifiers, "Alt"))
    , m_metaKey(evas_key_modifier_is_set(event->modifiers, "Meta"))
{
    String keyName = String(event->key);
    m_keyIdentifier = keyIdentifierForEvasKeyName(keyName);
    m_windowsVirtualKeyCode = windowsKeyCodeForEvasKeyName(keyName);

    // FIXME:
    m_isKeypad = false;
    m_autoRepeat = false;
}

PlatformKeyboardEvent::PlatformKeyboardEvent(const Evas_Event_Key_Up* event)
    : m_type(KeyUp)
    , m_text(String::fromUTF8(event->string))
    , m_shiftKey(evas_key_modifier_is_set(event->modifiers, "Shift"))
    , m_ctrlKey(evas_key_modifier_is_set(event->modifiers, "Control"))
    , m_altKey(evas_key_modifier_is_set(event->modifiers, "Alt"))
    , m_metaKey(evas_key_modifier_is_set(event->modifiers, "Meta"))
{
    String keyName = String(event->key);
    m_keyIdentifier = keyIdentifierForEvasKeyName(keyName);
    m_windowsVirtualKeyCode = windowsKeyCodeForEvasKeyName(keyName);

    // FIXME:
    m_isKeypad = false;
    m_autoRepeat = false;
}

void PlatformKeyboardEvent::disambiguateKeyDownEvent(Type type, bool)
{
    ASSERT(m_type == KeyDown);
    m_type = type;

    if (type == RawKeyDown) {
        m_text = String();
        m_unmodifiedText = String();
    } else {
        m_keyIdentifier = String();
        m_windowsVirtualKeyCode = 0;
    }
}

bool PlatformKeyboardEvent::currentCapsLockState()
{
    notImplemented();
    return false;
}

void PlatformKeyboardEvent::getCurrentModifierState(bool& shiftKey, bool& ctrlKey, bool& altKey, bool& metaKey)
{
    notImplemented();
    shiftKey = false;
    ctrlKey = false;
    altKey = false;
    metaKey = false;
}

}
