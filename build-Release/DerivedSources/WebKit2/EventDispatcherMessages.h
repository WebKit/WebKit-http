/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "ArgumentCoders.h"

namespace WebKit {
    class WebGestureEvent;
    class WebTouchEvent;
    class WebWheelEvent;
}

namespace Messages {
namespace EventDispatcher {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("EventDispatcher");
}

class WheelEvent {
public:
    typedef std::tuple<uint64_t, const WebKit::WebWheelEvent&, bool, bool, bool, bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("WheelEvent"); }
    static const bool isSync = false;

    WheelEvent(uint64_t pageID, const WebKit::WebWheelEvent& event, bool canRubberBandAtLeft, bool canRubberBandAtRight, bool canRubberBandAtTop, bool canRubberBandAtBottom)
        : m_arguments(pageID, event, canRubberBandAtLeft, canRubberBandAtRight, canRubberBandAtTop, canRubberBandAtBottom)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

#if ENABLE(IOS_TOUCH_EVENTS)
class TouchEvent {
public:
    typedef std::tuple<uint64_t, const WebKit::WebTouchEvent&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("TouchEvent"); }
    static const bool isSync = false;

    TouchEvent(uint64_t pageID, const WebKit::WebTouchEvent& event)
        : m_arguments(pageID, event)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if ENABLE(MAC_GESTURE_EVENTS)
class GestureEvent {
public:
    typedef std::tuple<uint64_t, const WebKit::WebGestureEvent&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GestureEvent"); }
    static const bool isSync = false;

    GestureEvent(uint64_t pageID, const WebKit::WebGestureEvent& event)
        : m_arguments(pageID, event)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

} // namespace EventDispatcher
} // namespace Messages
