/*
 * Copyright (C) 2014 Igalia S.L.
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

#ifndef WPE_Input_Handling_h
#define WPE_Input_Handling_h

#include <WPE/Input/Events.h>
#include <array>

namespace WPE {

namespace Input {

class KeyboardEventHandler;

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
    static Server& singleton();

    void setTarget(Client* target) { m_target = target; }

    void serveKeyboardEvent(KeyboardEvent::Raw&&);
    void servePointerEvent(PointerEvent::Raw&&);
    void serveAxisEvent(AxisEvent::Raw&&);
    void serveTouchEvent(TouchEvent::Raw&&);

private:
    Server();

    Client* m_target { nullptr };

    std::unique_ptr<KeyboardEventHandler> m_keyboardEventHandler;

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
