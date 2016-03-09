/*
 * Copyright (C) 2014 Igalia S.L.
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

#ifndef WPE_Input_Handling_h
#define WPE_Input_Handling_h

#include <WPE/WPE.h>

#include "Events.h"
#include <array>
#include <memory>

namespace WPE {

namespace Input {

class KeyboardEventHandler;
class KeyboardEventRepeating;

class Client {
public:
    virtual ~Client() = default;

    virtual void handleKeyboardEvent(KeyboardEvent&&) = 0;
    virtual void handlePointerEvent(PointerEvent&&) = 0;
    virtual void handleAxisEvent(AxisEvent&&) = 0;
    virtual void handleTouchEvent(TouchEvent&&) = 0;
};

class Server {
public:
    static WPE_EXPORT Server& singleton();

    WPE_EXPORT void setTarget(Client*);

    WPE_EXPORT void serveKeyboardEvent(KeyboardEvent::Raw&&);
    WPE_EXPORT void servePointerEvent(PointerEvent::Raw&&);
    WPE_EXPORT void serveAxisEvent(AxisEvent::Raw&&);
    WPE_EXPORT void serveTouchEvent(TouchEvent::Raw&&);

private:
    Server();

    Client* m_target { nullptr };

    // FIXME: Merge the two classes?
    std::unique_ptr<KeyboardEventHandler> m_keyboardEventHandler;
    std::unique_ptr<KeyboardEventRepeating> m_keyboardEventRepeating;

    struct Pointer {
        uint32_t x;
        uint32_t y;
    } m_pointer { 0, 0 };

    void dispatchTouchEvent(int id);
    std::array<TouchEvent::Raw, 10> m_touchEvents;
};

} // namespace WPE

} // namespace WPE

#endif // WPE_Input_Handling_h
