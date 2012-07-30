/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef GestureEvent_h
#define GestureEvent_h

#if ENABLE(GESTURE_EVENTS)

#include "EventDispatcher.h"
#include "EventNames.h"
#include "Frame.h"
#include "FrameView.h"
#include "MouseRelatedEvent.h"
#include "PlatformEvent.h"
#include "PlatformGestureEvent.h"

namespace WebCore {

class GestureEvent : public MouseRelatedEvent {
public:
    virtual ~GestureEvent() { }

    static PassRefPtr<GestureEvent> create();
    static PassRefPtr<GestureEvent> create(PassRefPtr<AbstractView>, const PlatformGestureEvent&);

    void initGestureEvent(const AtomicString& type, PassRefPtr<AbstractView>, int screenX, int screenY, int clientX, int clientY, bool ctrlKey, bool altKey, bool shiftKey, bool metaKey, float deltaX, float deltaY);

    virtual const AtomicString& interfaceName() const;

    float deltaX() const { return m_deltaX; }
    float deltaY() const { return m_deltaY; }

private:
    GestureEvent();
    GestureEvent(const AtomicString& type, PassRefPtr<AbstractView>, int screenX, int screenY, int clientX, int clientY, bool ctrlKey, bool altKey, bool shiftKey, bool metaKey, float deltaX, float deltaY);

    float m_deltaX;
    float m_deltaY;
};

class GestureEventDispatchMediator : public EventDispatchMediator {
public:
    static PassRefPtr<GestureEventDispatchMediator> create(PassRefPtr<GestureEvent> gestureEvent)
    {
        return adoptRef(new GestureEventDispatchMediator(gestureEvent));
    }

private:
    explicit GestureEventDispatchMediator(PassRefPtr<GestureEvent>);

    GestureEvent* event() const;

    virtual bool dispatchEvent(EventDispatcher*) const;
};

} // namespace WebCore

#endif // ENABLE(GESTURE_EVENTS)

#endif // GestureEvent_h
