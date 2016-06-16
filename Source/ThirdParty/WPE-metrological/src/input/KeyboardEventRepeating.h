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

#ifndef WPE_Input_KeyboardEventRepeating_h
#define WPE_Input_KeyboardEventRepeating_h

#include <glib.h>
#include <wpe/input.h>

namespace WPE {

namespace Input {

class KeyboardEventRepeating {
public:
    class Client {
    public:
        virtual void dispatchKeyboardEvent(struct wpe_input_keyboard_event*) = 0;
    };

    KeyboardEventRepeating(Client&);
    ~KeyboardEventRepeating();

    void schedule(struct wpe_input_keyboard_event*);
    void cancel();

private:
    static const unsigned s_startupDelay { 500000 };
    static const unsigned s_repeatDelay { 100000 };

    void dispatch();
    static GSourceFuncs sourceFuncs;

    Client& m_client;
    GSource* m_source;
    struct wpe_input_keyboard_event m_event { 0, };
};

} // namespace Input

} // namespace WPE

#endif // WPE_Input_KeyboardEventRepeating_h
