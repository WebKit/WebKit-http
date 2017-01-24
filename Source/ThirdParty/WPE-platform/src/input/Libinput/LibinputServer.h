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

#ifndef LibinputServer_h
#define LibinputServer_h

#include "KeyboardEventRepeating.h"
#include <glib.h>
#include <memory>
#include <wpe/input.h>
#ifndef KEY_INPUT_HANDLING_VIRTUAL
#include <libudev.h>
#include <libinput.h>
#else
#include <gluelogic/virtualkeyboard/VirtualKeyboard.h>
#endif

struct wpe_view_backend;

namespace WPE {

namespace Input {
class KeyboardEventHandler;
}

class LibinputServer : public Input::KeyboardEventRepeating::Client {
public:
    static LibinputServer& singleton();

    class Client {
    public:
        virtual void handleKeyboardEvent(struct wpe_input_keyboard_event*) = 0;
        virtual void handlePointerEvent(struct wpe_input_pointer_event*) = 0;
        virtual void handleAxisEvent(struct wpe_input_axis_event*) = 0;
        virtual void handleTouchEvent(struct wpe_input_touch_event*) = 0;
    };

    void setClient(Client*);
    void setHandlePointerEvents(bool handle);
    void setHandleTouchEvents(bool handle);
    void setPointerBounds(uint32_t, uint32_t);
    void handleKeyboardEvent(struct wpe_input_keyboard_event*);
private:
    LibinputServer();
    ~LibinputServer();


    // Input::KeyboardEventRepeating::Client
    void dispatchKeyboardEvent(struct wpe_input_keyboard_event*) override;

    Client* m_client;
    std::unique_ptr<Input::KeyboardEventHandler> m_keyboardEventHandler;
    std::unique_ptr<Input::KeyboardEventRepeating> m_keyboardEventRepeating;

    bool m_handlePointerEvents { false };
    std::pair<int32_t, int32_t> m_pointerCoords;
    std::pair<uint32_t, uint32_t> m_pointerBounds;

    bool m_handleTouchEvents { false };
    std::array<struct wpe_input_touch_event_raw, 10> m_touchEvents;

#ifdef KEY_INPUT_HANDLING_VIRTUAL
public:
    void VirtualInput (unsigned int type, unsigned int code);

private:
    void* m_virtualkeyboard;
#else
    void processEvents();
    void handleTouchEvent(struct libinput_event *event, enum wpe_input_touch_event_type type);

    struct udev* m_udev;
    struct libinput* m_libinput;

    class EventSource {
    public:
        static GSourceFuncs s_sourceFuncs;

        GSource source;
        GPollFD pfd;
        LibinputServer* server;
    };
#endif
};

} // namespace WPE

#endif // LibinputServer_h
