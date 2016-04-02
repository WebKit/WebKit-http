/*
 * Copyright (C) 2015 Igalia S.L.
 * Copyright (C) 2015 Metrological
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

#ifndef LibinputServer_h
#define LibinputServer_h

#include "KeyboardEventRepeating.h"
#include <glib.h>
#include <libinput.h>
#include <libudev.h>
#include <memory>
#include <WPE/Input/Events.h>

namespace WPE {

namespace Input {
class Client;
class KeyboardEventHandler;
}

class LibinputServer : public Input::KeyboardEventRepeating::Client {
public:
    static LibinputServer& singleton();

    void setClient(Input::Client* client);
    void setHandlePointerEvents(bool handle);
    void setHandleTouchEvents(bool handle);
    void setPointerBounds(uint32_t, uint32_t);
    void handleKeyboardEvent(Input::KeyboardEvent&&);
private:
    LibinputServer();
    ~LibinputServer();

    void processEvents();

    // Input::KeyboardEventRepeating::Client
    void dispatchKeyboardEvent(const Input::KeyboardEvent::Raw&) override;

    struct udev* m_udev;
    struct libinput* m_libinput;
    Input::Client* m_client;
    std::unique_ptr<Input::KeyboardEventHandler> m_keyboardEventHandler;
    std::unique_ptr<Input::KeyboardEventRepeating> m_keyboardEventRepeating;

    bool m_handlePointerEvents { false };
    std::pair<int32_t, int32_t> m_pointerCoords;
    std::pair<uint32_t, uint32_t> m_pointerBounds;

    bool m_handleTouchEvents { false };
    std::array<Input::TouchEvent::Raw, 10> m_touchEvents;
    void handleTouchEvent(struct libinput_event *event, Input::TouchEvent::Type type);

#ifdef KEY_INPUT_HANDLING_VIRTUAL
    void* m_virtualkeyboard;
#endif

    class EventSource {
    public:
        static GSourceFuncs s_sourceFuncs;

        GSource source;
        GPollFD pfd;
        LibinputServer* server;
    };
};

} // namespace WPE

#endif // LibinputServer_h
