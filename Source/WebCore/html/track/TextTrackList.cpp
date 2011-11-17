/*
 * Copyright (C) 2011 Apple Inc.  All rights reserved.
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

#if ENABLE(VIDEO_TRACK)

#include "TextTrackList.h"

#include "EventNames.h"
#include "LoadableTextTrack.h"
#include "ScriptExecutionContext.h"
#include "TextTrack.h"
#include "TrackEvent.h"

using namespace WebCore;

TextTrackList::TextTrackList(HTMLMediaElement* owner, ScriptExecutionContext* context)
    : m_context(context)
    , m_owner(owner)
    , m_pendingEventTimer(this, &TextTrackList::asyncEventTimerFired)
    , m_dispatchingEvents(0)
{
    ASSERT(context->isDocument());
}

TextTrackList::~TextTrackList()
{
}

unsigned TextTrackList::length() const
{
    return m_addTrackTracks.size() + m_elementTracks.size();
}

TextTrack* TextTrackList::item(unsigned index)
{
    // 4.8.10.12.1 Text track model
    // The text tracks are sorted as follows:
    // 1. The text tracks corresponding to track element children of the media element, in tree order.
    // 2. Any text tracks added using the addTextTrack() method, in the order they were added, oldest first.
    // 3. Any media-resource-specific text tracks (text tracks corresponding to data in the media
    // resource), in the order defined by the media resource's format specification.

    if (index < m_elementTracks.size())
        return m_elementTracks[index].get();

    index -= m_elementTracks.size();
    if (index < m_addTrackTracks.size())
        return m_addTrackTracks[index].get();

    return 0;
}

void TextTrackList::append(PassRefPtr<TextTrack> track)
{
    RefPtr<TextTrack> trackRef = track;
    
    if (trackRef->trackType() == TextTrack::AddTrack)
        m_addTrackTracks.append(trackRef);
    else if (trackRef->trackType() == TextTrack::TrackElement) {
        // Insert tracks added for <track> element in tree order.
        size_t index = static_cast<LoadableTextTrack*>(trackRef.get())->trackElementIndex();
        m_elementTracks.insert(index, trackRef);
    } else
        ASSERT_NOT_REACHED();

    scheduleAddTrackEvent(trackRef);
}

void TextTrackList::remove(PassRefPtr<TextTrack> track)
{
    Vector<RefPtr<TextTrack> >* tracks = 0;

    if (track->trackType() == TextTrack::TrackElement)
        tracks = &m_elementTracks;
    else if (track->trackType() == TextTrack::AddTrack)
        tracks = &m_addTrackTracks;
    else
        ASSERT_NOT_REACHED();

    size_t index = tracks->find(track);
    if (index == notFound)
        return;
    tracks->remove(index);

}

const AtomicString& TextTrackList::interfaceName() const
{
    return eventNames().interfaceForTextTrackList;
}

void TextTrackList::scheduleAddTrackEvent(PassRefPtr<TextTrack> track)
{
    // 4.8.10.12.3 Sourcing out-of-band text tracks
    // 4.8.10.12.3 Sourcing out-of-band text tracks
    // 4.8.10.12.4 Text track API
    // ... then queue a task to fire an event with the name addtrack, that does not 
    // bubble and is not cancelable, and that uses the TrackEvent interface, with 
    // the track attribute initialized to the text track's TextTrack object, at 
    // the media element's textTracks attribute's TextTrackList object.

    RefPtr<TextTrack> trackRef = track;
    TrackEventInit initializer;
    initializer.track = trackRef;
    initializer.bubbles = false;
    initializer.cancelable = false;

    m_pendingEvents.append(TrackEvent::create(eventNames().addtrackEvent, initializer));
    if (!m_pendingEventTimer.isActive())
        m_pendingEventTimer.startOneShot(0);
}

void TextTrackList::asyncEventTimerFired(Timer<TextTrackList>*)
{
    Vector<RefPtr<Event> > pendingEvents;
    ExceptionCode ec = 0;
    
    ++m_dispatchingEvents;
    m_pendingEvents.swap(pendingEvents);
    size_t count = pendingEvents.size();
    for (size_t index = 0; index < count; ++index)
        dispatchEvent(pendingEvents[index].release(), ec);
    --m_dispatchingEvents;
}

#endif
