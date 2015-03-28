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

#ifndef WPEInputHandler_h
#define WPEInputHandler_h

#include "APIObject.h"
#include "KeyInputHandler.h"
#include <array>
#include <wtf/Vector.h>
#include <WPE/Input/Events.h>

namespace WKWPE {

class View;

class InputHandler : public API::ObjectImpl<API::Object::Type::InputHandler> {
public:
    static InputHandler* create(View& view)
    {
        return new InputHandler(view);
    }

    void handleKeyboardKey(WPE::Input::KeyboardEvent::Raw);

    void handlePointerEvent(WPE::Input::PointerEvent::Raw);

    void handleAxisEvent(WPE::Input::AxisEvent::Raw);

    void handleTouchDown(WPE::Input::TouchEvent::Raw);
    void handleTouchUp(WPE::Input::TouchEvent::Raw);
    void handleTouchMotion(WPE::Input::TouchEvent::Raw);

private:
    InputHandler(View&);

    View& m_view;
    std::unique_ptr<WPE::KeyInputHandler> m_keyInputHandler;

    struct Pointer {
        uint32_t x;
        uint32_t y;
    } m_pointer;

    void dispatchTouchEvent(int id);
    std::array<WPE::Input::TouchEvent::Raw, 10> m_touchEvents;
};

} // namespace WPE

#endif // WPEInputHandler_h
