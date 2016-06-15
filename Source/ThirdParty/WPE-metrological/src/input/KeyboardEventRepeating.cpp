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

#include "KeyboardEventRepeating.h"

namespace WPE {

namespace Input {

KeyboardEventRepeating::KeyboardEventRepeating(Client& client)
    : m_client(client)
{
    m_source = g_source_new(&sourceFuncs, sizeof(GSource));
    g_source_set_name(m_source, "[WPE] KeyboardEventRepeating");
    g_source_set_priority(m_source, G_PRIORITY_DEFAULT_IDLE);
    g_source_set_callback(m_source, nullptr, this, nullptr);
    g_source_attach(m_source, g_main_context_get_thread_default());
}

KeyboardEventRepeating::~KeyboardEventRepeating()
{
    g_source_destroy(m_source);
    g_source_unref(m_source);
}

void KeyboardEventRepeating::schedule(struct wpe_input_keyboard_event* event)
{
    if (!!m_event.time && m_event.time == event->time && m_event.keyCode == event->keyCode)
        return;

    m_event = *event;
    g_source_set_ready_time(m_source, g_get_monotonic_time() + s_startupDelay);
}

void KeyboardEventRepeating::cancel()
{
    if (!!m_event.time)
        g_source_set_ready_time(m_source, -1);
    m_event = { 0, 0, 0 };
}

void KeyboardEventRepeating::dispatch()
{
    if (!m_event.time) {
        g_source_set_ready_time(m_source, -1);
        return;
    }

    m_client.dispatchKeyboardEvent(&m_event);
    g_source_set_ready_time(m_source, g_get_monotonic_time() + s_repeatDelay);
}

GSourceFuncs KeyboardEventRepeating::sourceFuncs = {
    nullptr, // prepare
    nullptr, // check
    // dispatch
    [](GSource*, GSourceFunc, gpointer data) -> gboolean {
        auto& repeating = *reinterpret_cast<KeyboardEventRepeating*>(data);
        repeating.dispatch();
        return G_SOURCE_CONTINUE;
    },
    nullptr, // finalize
    nullptr, // closure_callback
    nullptr, // closure_marshall
};

} // namespace Input

} // namespace WPE
