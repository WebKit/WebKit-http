/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2011, 2012 Ericsson AB. All rights reserved.
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "MediaStream.h"

#if ENABLE(MEDIA_STREAM)

#include "AudioStreamTrack.h"
#include "Event.h"
#include "ExceptionCode.h"
#include "MediaStreamCenter.h"
#include "MediaStreamRegistry.h"
#include "MediaStreamSource.h"
#include "MediaStreamTrackEvent.h"
#include "VideoStreamTrack.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {

PassRefPtr<MediaStream> MediaStream::create(ScriptExecutionContext& context)
{
    return MediaStream::create(context, MediaStreamDescriptor::create(Vector<RefPtr<MediaStreamSource>>(), Vector<RefPtr<MediaStreamSource>>()));
}

PassRefPtr<MediaStream> MediaStream::create(ScriptExecutionContext& context, PassRefPtr<MediaStream> stream)
{
    ASSERT(stream);

    Vector<RefPtr<MediaStreamTrackPrivate>> audioTracks;
    Vector<RefPtr<MediaStreamTrackPrivate>> videoTracks;

    for (size_t i = 0; i < stream->m_audioTracks.size(); ++i)
        audioTracks.append(&stream->m_audioTracks[i]->privateTrack());

    for (size_t i = 0; i < stream->m_videoTracks.size(); ++i)
        videoTracks.append(&stream->m_videoTracks[i]->privateTrack());

    return MediaStream::create(context, MediaStreamDescriptor::create(audioTracks, videoTracks));
}

PassRefPtr<MediaStream> MediaStream::create(ScriptExecutionContext& context, const Vector<RefPtr<MediaStreamTrack>>& tracks)
{
    Vector<RefPtr<MediaStreamTrackPrivate>> audioTracks;
    Vector<RefPtr<MediaStreamTrackPrivate>> videoTracks;

    for (size_t i = 0; i < tracks.size(); ++i) {
        if (tracks[i]->kind() == "audio")
            audioTracks.append(&tracks[i]->privateTrack());
        else
            videoTracks.append(&tracks[i]->privateTrack());
    }

    return MediaStream::create(context, MediaStreamDescriptor::create(audioTracks, videoTracks));
}

PassRefPtr<MediaStream> MediaStream::create(ScriptExecutionContext& context, PassRefPtr<MediaStreamDescriptor> streamDescriptor)
{
    return adoptRef(new MediaStream(context, streamDescriptor));
}

MediaStream::MediaStream(ScriptExecutionContext& context, PassRefPtr<MediaStreamDescriptor> streamDescriptor)
    : ContextDestructionObserver(&context)
    , m_descriptor(streamDescriptor)
    , m_scheduledEventTimer(this, &MediaStream::scheduledEventTimerFired)
{
    ASSERT(m_descriptor);
    m_descriptor->setClient(this);

    RefPtr<MediaStreamTrack> track;
    size_t numberOfAudioTracks = m_descriptor->numberOfAudioTracks();
    m_audioTracks.reserveCapacity(numberOfAudioTracks);
    for (size_t i = 0; i < numberOfAudioTracks; i++) {
        track = AudioStreamTrack::create(context, *m_descriptor->audioTracks(i));
        track->addObserver(this);
        m_audioTracks.append(track.release());
    }

    size_t numberOfVideoTracks = m_descriptor->numberOfVideoTracks();
    m_videoTracks.reserveCapacity(numberOfVideoTracks);
    for (size_t i = 0; i < numberOfVideoTracks; i++) {
        track = VideoStreamTrack::create(context, *m_descriptor->videoTracks(i));
        track->addObserver(this);
        m_videoTracks.append(track.release());
    }
}

MediaStream::~MediaStream()
{
    m_descriptor->setClient(0);
}

bool MediaStream::ended() const
{
    return m_descriptor->ended();
}

void MediaStream::setEnded()
{
    if (ended())
        return;
    m_descriptor->setEnded();
}

PassRefPtr<MediaStream> MediaStream::clone()
{
    Vector<RefPtr<MediaStreamTrack>> trackSet;

    cloneMediaStreamTrackVector(trackSet, getAudioTracks());
    cloneMediaStreamTrackVector(trackSet, getVideoTracks());
    return MediaStream::create(*scriptExecutionContext(), trackSet);
}

void MediaStream::cloneMediaStreamTrackVector(Vector<RefPtr<MediaStreamTrack>>& destination, const Vector<RefPtr<MediaStreamTrack>>& source)
{
    for (unsigned i = 0; i < source.size(); i++)
        destination.append(source[i]->clone());
}

void MediaStream::addTrack(PassRefPtr<MediaStreamTrack> prpTrack, ExceptionCode& ec)
{
    if (ended()) {
        ec = INVALID_STATE_ERR;
        return;
    }

    if (!prpTrack) {
        ec = TYPE_MISMATCH_ERR;
        return;
    }

    addTrack(prpTrack);
}

bool MediaStream::addTrack(PassRefPtr<MediaStreamTrack> prpTrack)
{
    // This is a common part used by addTrack called by JavaScript
    // and addRemoteTrack and only addRemoteTrack must fire addtrack event
    RefPtr<MediaStreamTrack> track = prpTrack;
    if (getTrackById(track->id()))
        return false;

    Vector<RefPtr<MediaStreamTrack>>* tracks = trackVectorForType(track->source()->type());

    tracks->append(track);
    track->addObserver(this);
    m_descriptor->addTrack(&track->privateTrack());
    return true;
}

void MediaStream::removeTrack(PassRefPtr<MediaStreamTrack> prpTrack, ExceptionCode& ec)
{
    if (ended()) {
        ec = INVALID_STATE_ERR;
        return;
    }

    if (!prpTrack) {
        ec = TYPE_MISMATCH_ERR;
        return;
    }

    removeTrack(prpTrack);
}

bool MediaStream::removeTrack(PassRefPtr<MediaStreamTrack> prpTrack)
{
    // This is a common part used by removeTrack called by JavaScript
    // and removeRemoteTrack and only removeRemoteTrack must fire removetrack event
    RefPtr<MediaStreamTrack> track = prpTrack;
    Vector<RefPtr<MediaStreamTrack>>* tracks = trackVectorForType(track->source()->type());

    size_t pos = tracks->find(track);
    if (pos == notFound)
        return false;

    tracks->remove(pos);
    m_descriptor->removeTrack(&track->privateTrack());
    // There can be other tracks using the same source in the same MediaStream,
    // like when MediaStreamTrack::clone() is called, for instance.
    // Spec says that a source can be shared, so we must assure that there is no
    // other track using it.
    if (!haveTrackWithSource(track->source()))
        m_descriptor->removeSource(track->source());

    track->removeObserver(this);
    if (!m_audioTracks.size() && !m_videoTracks.size())
        setEnded();

    return true;
}

bool MediaStream::haveTrackWithSource(PassRefPtr<MediaStreamSource> source)
{
    if (source->type() == MediaStreamSource::Audio) {
        for (auto iter = m_audioTracks.begin(); iter != m_audioTracks.end(); ++iter) {
            if ((*iter)->source() == source.get())
                return true;
        }
        return false;
    }

    for (auto iter = m_videoTracks.begin(); iter != m_videoTracks.end(); ++iter) {
        if ((*iter)->source() == source.get())
            return true;
    }

    return false;
}

MediaStreamTrack* MediaStream::getTrackById(String id)
{
    for (auto iter = m_audioTracks.begin(); iter != m_audioTracks.end(); ++iter) {
        if ((*iter)->id() == id)
            return (*iter).get();
    }

    for (auto iter = m_videoTracks.begin(); iter != m_videoTracks.end(); ++iter) {
        if ((*iter)->id() == id)
            return (*iter).get();
    }

    return 0;
}

void MediaStream::trackDidEnd()
{
    for (size_t i = 0; i < m_audioTracks.size(); ++i)
        if (!m_audioTracks[i]->ended())
            return;
    
    for (size_t i = 0; i < m_videoTracks.size(); ++i)
        if (!m_videoTracks[i]->ended())
            return;
    
    setEnded();
}

void MediaStream::streamDidEnd()
{
    if (ended())
        return;

    scheduleDispatchEvent(Event::create(eventNames().endedEvent, false, false));
}

void MediaStream::contextDestroyed()
{
    ContextDestructionObserver::contextDestroyed();
}

void MediaStream::addRemoteSource(MediaStreamSource* source)
{
    ASSERT(source);
    addRemoteTrack(MediaStreamTrackPrivate::create(source).get());
}

void MediaStream::removeRemoteSource(MediaStreamSource* source)
{
    ASSERT(source);
    if (ended())
        return;

    Vector<RefPtr<MediaStreamTrack>>* tracks = trackVectorForType(source->type());

    for (int i = tracks->size() - 1; i >= 0; --i) {
        if ((*tracks)[i]->source() != source)
            continue;

        RefPtr<MediaStreamTrack> track = (*tracks)[i];
        track->removeObserver(this);
        tracks->remove(i);
        m_descriptor->removeTrack(&track->privateTrack());
        scheduleDispatchEvent(MediaStreamTrackEvent::create(eventNames().removetrackEvent, false, false, track.release()));
    }

    m_descriptor->removeSource(source);
}

void MediaStream::addRemoteTrack(MediaStreamTrackPrivate* privateTrack)
{
    ASSERT(privateTrack);
    if (ended())
        return;

    RefPtr<MediaStreamTrack> track;
    switch (privateTrack->type()) {
    case MediaStreamSource::Audio:
        track = AudioStreamTrack::create(*scriptExecutionContext(), *privateTrack);
        break;
    case MediaStreamSource::Video:
        track = VideoStreamTrack::create(*scriptExecutionContext(), *privateTrack);
        break;
    case MediaStreamSource::None:
        ASSERT_NOT_REACHED();
        break;
    }

    if (!track)
        return;

    if (addTrack(track))
        scheduleDispatchEvent(MediaStreamTrackEvent::create(eventNames().addtrackEvent, false, false, track));
}

void MediaStream::removeRemoteTrack(MediaStreamTrackPrivate* privateTrack)
{
    ASSERT(privateTrack);
    if (ended())
        return;

    RefPtr<MediaStreamTrack> track = getTrackById(privateTrack->id());
    if (removeTrack(track))
        scheduleDispatchEvent(MediaStreamTrackEvent::create(eventNames().removetrackEvent, false, false, track.release()));
}

void MediaStream::scheduleDispatchEvent(PassRefPtr<Event> event)
{
    m_scheduledEvents.append(event);

    if (!m_scheduledEventTimer.isActive())
        m_scheduledEventTimer.startOneShot(0);
}

void MediaStream::scheduledEventTimerFired(Timer<MediaStream>*)
{
    Vector<RefPtr<Event>> events;
    events.swap(m_scheduledEvents);

    Vector<RefPtr<Event>>::iterator it = events.begin();
    for (; it != events.end(); ++it)
        dispatchEvent((*it).release());

    events.clear();
}

URLRegistry& MediaStream::registry() const
{
    return MediaStreamRegistry::registry();
}

Vector<RefPtr<MediaStreamTrack>>* MediaStream::trackVectorForType(MediaStreamSource::Type type)
{
    switch (type) {
    case MediaStreamSource::Audio:
        return &m_audioTracks;
    case MediaStreamSource::Video:
        return &m_videoTracks;
    case MediaStreamSource::None:
        ASSERT_NOT_REACHED();
    }
    return nullptr;
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
