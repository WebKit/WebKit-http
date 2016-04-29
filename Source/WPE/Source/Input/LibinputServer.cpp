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

#ifdef KEY_INPUT_HANDLING_VIRTUAL

#include <gluelogic/virtualkeyboard/VirtualKeyboard.h>

static const char * connectorName = "/tmp/keyhandler";

static void VirtualKeyboardCallback(actiontype type , unsigned int code)
{
   if (type != COMPLETED)
   {
      // RELEASED  = 0,
      // PRESSED   = 1,
      // REPEAT    = 2,
      // COMPLETED = 3
      LibinputServer::singleton().VirtualInput (type, code);
   }
}

void LibinputServer::VirtualInput (unsigned int type, unsigned int code)
{
    Input::KeyboardEvent::Raw rawEvent;

    rawEvent.time = time(nullptr);
    rawEvent.key = code;
    rawEvent.state = type;

    // printf ("Sending out key: %d, %d, %d\n", rawEvent.time, rawEvent.key, rawEvent.state);

    Input::KeyboardEventHandler::Result result = m_keyboardEventHandler->handleKeyboardEvent(rawEvent);
    m_client->handleKeyboardEvent({ rawEvent.time, std::get<0>(result), std::get<1>(result), !!rawEvent.state, std::get<2>(result) });
}

#endif

LibinputServer& LibinputServer::singleton()
{
    static LibinputServer server;
    return server;
}

LibinputServer::LibinputServer()
    : m_keyboardEventHandler(Input::KeyboardEventHandler::create())
    , m_keyboardEventRepeating(new Input::KeyboardEventRepeating(*this))
    , m_pointerCoords(0, 0)
    , m_pointerBounds(1, 1)
#ifdef KEY_INPUT_HANDLING_VIRTUAL
    , m_virtualkeyboard(nullptr)
#endif
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
    g_source_attach(baseSource, g_main_context_get_thread_default());

#ifdef KEY_INPUT_HANDLING_VIRTUAL
    const char listenerName[] = "wpe";
    m_virtualkeyboard = Construct(listenerName, connectorName, VirtualKeyboardCallback);
    if (m_virtualkeyboard == nullptr) {
      fprintf(stderr, "[LibinputServer] Initialization of virtual keyboard failed!!!\n");
    }
    else {
       fprintf(stderr, "[LibinputServer] Initialization of virtual keyboard and linux input system succeeded.\n");
    }
#else
    fprintf(stderr, "[LibinputServer] Initialization of linux input system succeeded.\n");
#endif

}

LibinputServer::~LibinputServer()
{
#ifdef KEY_INPUT_HANDLING_VIRTUAL
    if (m_virtualkeyboard != nullptr) {
       Destruct(m_virtualkeyboard);
    }
#endif
    libinput_unref(m_libinput);
    udev_unref(m_udev);
}

void LibinputServer::setClient(Input::Client* client)
{
    m_client = client;
}

void LibinputServer::handleKeyboardEvent(Input::KeyboardEvent&& actionEvent)
{
    if (m_client != nullptr)
    {
        // m_client->handleKeyboardEvent(actionEvent);
    }
}

void LibinputServer::setHandlePointerEvents(bool handle)
{
    m_handlePointerEvents = handle;
    fprintf(stderr, "[LibinputServer] %s pointer events.\n", handle ? "Enabling" : "Disabling");
}

void LibinputServer::setHandleTouchEvents(bool handle)
{
    m_handleTouchEvents = handle;
    fprintf(stderr, "[LibinputServer] %s handle events.\n", handle ? "Enabling" : "Disabling");
}

void LibinputServer::setPointerBounds(uint32_t width, uint32_t height)
{
    m_pointerBounds = { width, height };
}

void LibinputServer::processEvents()
{
    libinput_dispatch(m_libinput);

    while (auto* event = libinput_get_event(m_libinput)) {
        switch (libinput_event_get_type(event)) {
        case LIBINPUT_EVENT_TOUCH_DOWN: 
            if (m_handleTouchEvents)	
                handleTouchEvent(event, Input::TouchEvent::Type::Down);
            break;
        case LIBINPUT_EVENT_TOUCH_UP:	
            if (m_handleTouchEvents)	
                handleTouchEvent(event, Input::TouchEvent::Type::Up);    
            break;
        case LIBINPUT_EVENT_TOUCH_MOTION:
            if (m_handleTouchEvents)	
                handleTouchEvent(event, Input::TouchEvent::Type::Motion);    
            break;        
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
        case LIBINPUT_EVENT_POINTER_MOTION:
        {
            if (!m_handlePointerEvents)
                break;

            auto* pointerEvent = libinput_event_get_pointer_event(event);

            double dx = libinput_event_pointer_get_dx(pointerEvent);
            double dy = libinput_event_pointer_get_dy(pointerEvent);
            m_pointerCoords.first = std::min<int32_t>(std::max<uint32_t>(0, m_pointerCoords.first + dx), m_pointerBounds.first - 1);
            m_pointerCoords.second = std::min<int32_t>(std::max<uint32_t>(0, m_pointerCoords.second + dy), m_pointerBounds.second - 1);

            m_client->handlePointerEvent({ Input::PointerEvent::Motion, libinput_event_pointer_get_time(pointerEvent),
                m_pointerCoords.first, m_pointerCoords.second, 0, 0 });
            break;
        }
        case LIBINPUT_EVENT_POINTER_BUTTON:
        {
            if (!m_handlePointerEvents)
                break;

            auto* pointerEvent = libinput_event_get_pointer_event(event);
            m_client->handlePointerEvent({ Input::PointerEvent::Button, libinput_event_pointer_get_time(pointerEvent),
                m_pointerCoords.first, m_pointerCoords.second,
                libinput_event_pointer_get_button(pointerEvent), libinput_event_pointer_get_button_state(pointerEvent) });
            break;
        }
        case LIBINPUT_EVENT_POINTER_AXIS:
        {
            if (!m_handlePointerEvents)
                break;

            auto* pointerEvent = libinput_event_get_pointer_event(event);

            // Support only wheel events for now.
            if (libinput_event_pointer_get_axis_source(pointerEvent) != LIBINPUT_POINTER_AXIS_SOURCE_WHEEL)
                break;

            if (libinput_event_pointer_has_axis(pointerEvent, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL)) {
                auto axis = LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL;
                int32_t axisValue = libinput_event_pointer_get_axis_value(pointerEvent, axis);
                m_client->handleAxisEvent({ Input::AxisEvent::Motion, libinput_event_pointer_get_time(pointerEvent),
                    m_pointerCoords.first, m_pointerCoords.second,
                    axis, -axisValue });
            }

            if (libinput_event_pointer_has_axis(pointerEvent, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL)) {
                auto axis = LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL;
                int32_t axisValue = libinput_event_pointer_get_axis_value(pointerEvent, axis);
                m_client->handleAxisEvent({ Input::AxisEvent::Motion, libinput_event_pointer_get_time(pointerEvent),
                    m_pointerCoords.first, m_pointerCoords.second,
                    axis, axisValue });
            }

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

void LibinputServer::handleTouchEvent(struct libinput_event *event, Input::TouchEvent::Type type)
{
    auto* touchEvent = libinput_event_get_touch_event(event);
    uint32_t time = libinput_event_touch_get_time(touchEvent);
    int id = libinput_event_touch_get_slot(touchEvent);
    auto& targetPoint = m_touchEvents[id];
    int32_t x, y;
    
    if (type != Input::TouchEvent::Up) {
      x = libinput_event_touch_get_x_transformed(touchEvent, m_pointerBounds.first);
      y = libinput_event_touch_get_y_transformed(touchEvent, m_pointerBounds.second);
    } else {
      // libinput can't return pointer position on touch-up
      x = targetPoint.x;
      y = targetPoint.y;
    }
    targetPoint = Input::TouchEvent::Raw{ type, time, id, x, y };
    
    m_client->handleTouchEvent({
      m_touchEvents,
      type,
      id,
      time
    });
    
    if (type == Input::TouchEvent::Up) {
        targetPoint = Input::TouchEvent::Raw{ Input::TouchEvent::Null, 0, -1, -1, -1 };
    }
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
