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

#include "LibinputServer.h"

#include "KeyboardEventHandler.h"
#include "KeyboardEventRepeating.h"
#include <WPE/Input/Handling.h>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

namespace WPE {

struct libinput_interface g_interface = {
    // open_restricted
    [](const char* path, int flags, void*)
    {
        return open(path, flags);
    },
    [](int fd, void*)
    {
        close(fd);
    }
};

LibinputServer& LibinputServer::singleton()
{
    static LibinputServer server;
    return server;
}

LibinputServer::LibinputServer()
    : m_keyboardEventHandler(Input::KeyboardEventHandler::create())
    , m_keyboardEventRepeating(new Input::KeyboardEventRepeating(*this))
{
    m_udev = udev_new();
    if (!m_udev)
        return;

    m_libinput = libinput_udev_create_context(&g_interface, nullptr, m_udev);
    if (!m_libinput)
        return;

    int ret = libinput_udev_assign_seat(m_libinput, "seat0");
    if (ret)
        return;

    GSource* baseSource = g_source_new(&EventSource::s_sourceFuncs, sizeof(EventSource));
    auto* source = reinterpret_cast<EventSource*>(baseSource);
    source->pfd.fd = libinput_get_fd(m_libinput);
    source->pfd.events = G_IO_IN | G_IO_ERR | G_IO_HUP;
    source->pfd.revents = 0;
    g_source_add_poll(baseSource, &source->pfd);
    source->server = this;

    g_source_set_name(baseSource, "[WPE] libinput");
    g_source_set_priority(baseSource, G_PRIORITY_DEFAULT);
    g_source_attach(baseSource, nullptr);

    fprintf(stderr, "[LibinputServer] Initialization succeeded.\n");
}

LibinputServer::~LibinputServer()
{
    libinput_unref(m_libinput);
    udev_unref(m_udev);
}

void LibinputServer::setClient(Input::Client* client)
{
    m_client = client;
}

void LibinputServer::processEvents()
{
    libinput_dispatch(m_libinput);

    while (auto* event = libinput_get_event(m_libinput)) {
        switch (libinput_event_get_type(event)) {
        case LIBINPUT_EVENT_KEYBOARD_KEY:
        {
            auto* keyEvent = libinput_event_get_keyboard_event(event);

            Input::KeyboardEvent::Raw rawEvent{
                libinput_event_keyboard_get_time(keyEvent),
                libinput_event_keyboard_get_key(keyEvent),
                libinput_event_keyboard_get_key_state(keyEvent)
            };

            Input::KeyboardEventHandler::Result result = m_keyboardEventHandler->handleKeyboardEvent(rawEvent);
            m_client->handleKeyboardEvent({ rawEvent.time, std::get<0>(result), std::get<1>(result), !!rawEvent.state, std::get<2>(result) });

            if (!!rawEvent.state)
                m_keyboardEventRepeating->schedule(rawEvent);
            else
                m_keyboardEventRepeating->cancel();

            break;
        }
        default:
            break;
        }

        libinput_event_destroy(event);
    }
}

void LibinputServer::dispatchKeyboardEvent(const Input::KeyboardEvent::Raw& event)
{
    Input::KeyboardEventHandler::Result result = m_keyboardEventHandler->handleKeyboardEvent(event);
    m_client->handleKeyboardEvent({ event.time, std::get<0>(result), std::get<1>(result), !!event.state, std::get<2>(result) });
}

GSourceFuncs LibinputServer::EventSource::s_sourceFuncs = {
    nullptr, // prepare
    // check
    [](GSource* base) -> gboolean
    {
        auto* source = reinterpret_cast<EventSource*>(base);
        return !!source->pfd.revents;
    },
    // dispatch
    [](GSource* base, GSourceFunc, gpointer) -> gboolean
    {
        auto* source = reinterpret_cast<EventSource*>(base);

        if (source->pfd.revents & (G_IO_ERR | G_IO_HUP))
            return FALSE;

        if (source->pfd.revents & G_IO_IN)
            source->server->processEvents();
        source->pfd.revents = 0;
        return TRUE;
    },
    nullptr, // finalize
    nullptr, // closure_callback
    nullptr, // closure_marshall
};

} // namespace WPE
