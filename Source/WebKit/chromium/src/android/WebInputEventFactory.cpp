/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#include "KeyCodeConversion.h"
#include "KeyboardCodes.h"
#include "WebInputEvent.h"
#include <wtf/Assertions.h>

namespace WebKit {

WebKeyboardEvent WebInputEventFactory::keyboardEvent(WebInputEvent::Type type,
                                                     int modifiers,
                                                     double timeStampSeconds,
                                                     int keycode,
                                                     WebUChar unicodeCharacter,
                                                     bool isSystemKey)
{
    WebKeyboardEvent result;

    result.type = type;
    result.modifiers = modifiers;
    result.timeStampSeconds = timeStampSeconds;
    result.windowsKeyCode = WebCore::windowsKeyCodeForKeyEvent(keycode);
    result.nativeKeyCode = keycode;
    result.unmodifiedText[0] = unicodeCharacter;
    if (result.windowsKeyCode == WebCore::VKEY_RETURN) {
        // This is the same behavior as GTK:
        // We need to treat the enter key as a key press of character \r. This
        // is apparently just how webkit handles it and what it expects.
        result.unmodifiedText[0] = '\r';
    }
    result.text[0] = result.unmodifiedText[0];
    result.setKeyIdentifierFromWindowsKeyCode();
    result.isSystemKey = isSystemKey;

    return result;
}

WebMouseEvent WebInputEventFactory::mouseEvent(int x, int y,
                                               int windowX, int windowY,
                                               MouseEventType type,
                                               double timeStampSeconds,
                                               WebMouseEvent::Button button)
{
    WebMouseEvent result;

    result.x = x;
    result.y = y;
    result.windowX = windowX;
    result.windowY = windowY;
    // FIXME: We need to decide what to use for the globalX/Y.
    result.globalX = windowX;
    result.globalY = windowY;
    result.timeStampSeconds = timeStampSeconds;
    result.clickCount = 1;

    switch (type) {
    case MouseEventTypeDown:
        result.type = WebInputEvent::MouseDown;
        result.button = button;
        break;
    case MouseEventTypeUp:
        result.type = WebInputEvent::MouseUp;
        result.button = button;
        break;
    case MouseEventTypeMove:
        result.type = WebInputEvent::MouseMove;
        result.button = WebMouseEvent::ButtonNone;
        break;
    };

    return result;
}

} // namespace WebKit
