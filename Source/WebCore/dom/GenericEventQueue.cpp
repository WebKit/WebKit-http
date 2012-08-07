/*
 * Copyright (C) 2012 Victor Carbune (victor@rosedu.org)
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
 */

#include "config.h"

#include "GenericEventQueue.h"

#include "Event.h"

namespace WebCore {

PassOwnPtr<GenericEventQueue> GenericEventQueue::create(EventTarget* owner)
{
    return adoptPtr(new GenericEventQueue(owner));
}

GenericEventQueue::GenericEventQueue(EventTarget* owner)
    : m_owner(owner)
    , m_timer(this, &GenericEventQueue::timerFired)
    , m_isClosed(false)
{
}

GenericEventQueue::~GenericEventQueue()
{
}

bool GenericEventQueue::enqueueEvent(PassRefPtr<Event> event)
{
    if (m_isClosed)
        return false;

    ASSERT(event->target());
    if (event->target() == m_owner)
        event->setTarget(0);

    m_pendingEvents.append(event);

    if (!m_timer.isActive())
        m_timer.startOneShot(0);

    return true;
}

bool GenericEventQueue::cancelEvent(Event* event)
{
    bool found = m_pendingEvents.contains(event);

    if (found)
        m_pendingEvents.remove(m_pendingEvents.find(event));

    if (m_pendingEvents.isEmpty())
        m_timer.stop();

    return found;
}

void GenericEventQueue::timerFired(Timer<GenericEventQueue>*)
{
    ASSERT(!m_timer.isActive());
    ASSERT(!m_pendingEvents.isEmpty());

    Vector<RefPtr<Event> > pendingEvents;
    m_pendingEvents.swap(pendingEvents);

    RefPtr<EventTarget> protect(m_owner);
    for (unsigned i = 0; i < pendingEvents.size(); ++i) {
        EventTarget* target = pendingEvents[i]->target() ? pendingEvents[i]->target() : m_owner;
        target->dispatchEvent(pendingEvents[i].release());
    }
}

void GenericEventQueue::close()
{
    m_isClosed = true;

    m_timer.stop();
    m_pendingEvents.clear();
}

void GenericEventQueue::cancelAllEvents()
{
    m_timer.stop();
    m_pendingEvents.clear();
}

bool GenericEventQueue::hasPendingEvents() const
{
    return m_pendingEvents.size();
}

}
