/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 *           (C) 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
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
 *
 */

#include "config.h"
#include "EventTarget.h"

#include "Event.h"
#include "EventException.h"
#include <wtf/MainThread.h>
#include <wtf/StdLibExtras.h>
#include <wtf/Vector.h>

using namespace WTF;

namespace WebCore {

#ifndef NDEBUG
static int gEventDispatchForbidden = 0;

void forbidEventDispatch()
{
    if (!isMainThread())
        return;
    ++gEventDispatchForbidden;
}

void allowEventDispatch()
{
    if (!isMainThread())
        return;
    if (gEventDispatchForbidden > 0)
        --gEventDispatchForbidden;
}

bool eventDispatchForbidden()
{
    if (!isMainThread())
        return false;
    return gEventDispatchForbidden > 0;
}
#endif // NDEBUG

EventTargetData::EventTargetData()
{
}

EventTargetData::~EventTargetData()
{
}

EventTarget::~EventTarget()
{
}

Node* EventTarget::toNode()
{
    return 0;
}

DOMWindow* EventTarget::toDOMWindow()
{
    return 0;
}

bool EventTarget::addEventListener(const AtomicString& eventType, PassRefPtr<EventListener> listener, bool useCapture)
{
    EventTargetData* d = ensureEventTargetData();
    return d->eventListenerMap.add(eventType, listener, useCapture);
}

bool EventTarget::removeEventListener(const AtomicString& eventType, EventListener* listener, bool useCapture)
{
    EventTargetData* d = eventTargetData();
    if (!d)
        return false;

    size_t indexOfRemovedListener;

    if (!d->eventListenerMap.remove(eventType, listener, useCapture, indexOfRemovedListener))
        return false;

    // Notify firing events planning to invoke the listener at 'index' that
    // they have one less listener to invoke.
    for (size_t i = 0; i < d->firingEventIterators.size(); ++i) {
        if (eventType != d->firingEventIterators[i].eventType)
            continue;

        if (indexOfRemovedListener >= d->firingEventIterators[i].end)
            continue;

        --d->firingEventIterators[i].end;
        if (indexOfRemovedListener <= d->firingEventIterators[i].iterator)
            --d->firingEventIterators[i].iterator;
    }

    return true;
}

bool EventTarget::setAttributeEventListener(const AtomicString& eventType, PassRefPtr<EventListener> listener)
{
    clearAttributeEventListener(eventType);
    if (!listener)
        return false;
    return addEventListener(eventType, listener, false);
}

EventListener* EventTarget::getAttributeEventListener(const AtomicString& eventType)
{
    const EventListenerVector& entry = getEventListeners(eventType);
    for (size_t i = 0; i < entry.size(); ++i) {
        if (entry[i].listener->isAttribute())
            return entry[i].listener.get();
    }
    return 0;
}

bool EventTarget::clearAttributeEventListener(const AtomicString& eventType)
{
    EventListener* listener = getAttributeEventListener(eventType);
    if (!listener)
        return false;
    return removeEventListener(eventType, listener, false);
}

bool EventTarget::dispatchEvent(PassRefPtr<Event> event, ExceptionCode& ec)
{
    if (!event || event->type().isEmpty()) {
        ec = EventException::UNSPECIFIED_EVENT_TYPE_ERR;
        return false;
    }

    if (event->isBeingDispatched()) {
        ec = EventException::DISPATCH_REQUEST_ERR;
        return false;
    }

    if (!scriptExecutionContext())
        return false;

    return dispatchEvent(event);
}

bool EventTarget::dispatchEvent(PassRefPtr<Event> event)
{
    event->setTarget(this);
    event->setCurrentTarget(this);
    event->setEventPhase(Event::AT_TARGET);
    bool defaultPrevented = fireEventListeners(event.get());
    event->setEventPhase(0);
    return defaultPrevented;
}

void EventTarget::uncaughtExceptionInEventHandler()
{
}

bool EventTarget::fireEventListeners(Event* event)
{
    ASSERT(!eventDispatchForbidden());
    ASSERT(event && !event->type().isEmpty());

    EventTargetData* d = eventTargetData();
    if (!d)
        return true;

    EventListenerVector* listenerVector = d->eventListenerMap.find(event->type());

    if (listenerVector)
        fireEventListeners(event, d, *listenerVector);
    
    return !event->defaultPrevented();
}
        
void EventTarget::fireEventListeners(Event* event, EventTargetData* d, EventListenerVector& entry)
{
    RefPtr<EventTarget> protect = this;

    // Fire all listeners registered for this event. Don't fire listeners removed
    // during event dispatch. Also, don't fire event listeners added during event
    // dispatch. Conveniently, all new event listeners will be added after 'end',
    // so iterating to 'end' naturally excludes new event listeners.

    size_t i = 0;
    size_t end = entry.size();
    d->firingEventIterators.append(FiringEventIterator(event->type(), i, end));
    for ( ; i < end; ++i) {
        RegisteredEventListener& registeredListener = entry[i];
        if (event->eventPhase() == Event::CAPTURING_PHASE && !registeredListener.useCapture)
            continue;
        if (event->eventPhase() == Event::BUBBLING_PHASE && registeredListener.useCapture)
            continue;

        // If stopImmediatePropagation has been called, we just break out immediately, without
        // handling any more events on this target.
        if (event->immediatePropagationStopped())
            break;

        // To match Mozilla, the AT_TARGET phase fires both capturing and bubbling
        // event listeners, even though that violates some versions of the DOM spec.
        registeredListener.listener->handleEvent(scriptExecutionContext(), event);
    }
    d->firingEventIterators.removeLast();
}

const EventListenerVector& EventTarget::getEventListeners(const AtomicString& eventType)
{
    DEFINE_STATIC_LOCAL(EventListenerVector, emptyVector, ());

    EventTargetData* d = eventTargetData();
    if (!d)
        return emptyVector;

    EventListenerVector* listenerVector = d->eventListenerMap.find(eventType);
    if (!listenerVector)
        return emptyVector;

    return *listenerVector;
}

void EventTarget::removeAllEventListeners()
{
    EventTargetData* d = eventTargetData();
    if (!d)
        return;
    d->eventListenerMap.clear();

    // Notify firing events planning to invoke the listener at 'index' that
    // they have one less listener to invoke.
    for (size_t i = 0; i < d->firingEventIterators.size(); ++i) {
        d->firingEventIterators[i].iterator = 0;
        d->firingEventIterators[i].end = 0;
    }
}

} // namespace WebCore
