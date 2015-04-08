/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 * Copyright (C) 2013 Orange
 * Copyright (C) 2014 Sebastian Dröge <sebastian@centricular.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SourceBufferPrivateGStreamer.h"

#if ENABLE(MEDIA_SOURCE) && USE(GSTREAMER)

#include "ContentType.h"
#include "MediaSample.h"
#include "MediaSourceGStreamer.h"
#include "NotImplemented.h"
#include "WebKitMediaSourceGStreamer.h"

namespace WebCore {

PassRefPtr<SourceBufferPrivateGStreamer> SourceBufferPrivateGStreamer::create(MediaSourceGStreamer* mediaSource, PassRefPtr<MediaSourceClientGStreamer> client, const ContentType& contentType)
{
    return adoptRef(new SourceBufferPrivateGStreamer(mediaSource, client, contentType));
}

SourceBufferPrivateGStreamer::SourceBufferPrivateGStreamer(MediaSourceGStreamer* mediaSource, PassRefPtr<MediaSourceClientGStreamer> client, const ContentType& contentType)
    : SourceBufferPrivate()
    , m_mediaSource(mediaSource)
    , m_type(contentType)
    , m_client(client)
    , m_readyState(MediaPlayer::HaveNothing)
{
}

SourceBufferPrivateGStreamer::~SourceBufferPrivateGStreamer()
{
}

void SourceBufferPrivateGStreamer::setClient(SourceBufferPrivateClient* client)
{
    m_sourceBufferPrivateClient = client;
}

void SourceBufferPrivateGStreamer::append(const unsigned char* data, unsigned length)
{
    ASSERT(m_mediaSource);
    ASSERT(m_sourceBufferPrivateClient);

    if (!m_client->append(this, data, length)) {
        if (m_sourceBufferPrivateClient)
            m_sourceBufferPrivateClient->sourceBufferPrivateAppendComplete(this, SourceBufferPrivateClient::ReadStreamFailed);
    }
}

void SourceBufferPrivateGStreamer::abort()
{
    notImplemented();
}

void SourceBufferPrivateGStreamer::removedFromMediaSource()
{
    if (!m_mediaSource || !m_client)
        return;

    m_mediaSource->removeSourceBuffer(this);
    m_client->removedFromMediaSource(this);
}

#if !ENABLE(VIDEO_TRACK)
PassRefPtr<TimeRanges> SourceBufferPrivateGStreamer::buffered() {
  if (!m_mediaSource)
      return TimeRanges::create();
  // TODO: Pass some id to buffered() to get only the ranges of this SourceBuffer
  return m_mediaSource->buffered().get();
}
#endif

MediaPlayer::ReadyState SourceBufferPrivateGStreamer::readyState() const
{
    return m_readyState;
}

void SourceBufferPrivateGStreamer::setReadyState(MediaPlayer::ReadyState state)
{
    m_readyState = state;
}

// TODO: Implement these
void SourceBufferPrivateGStreamer::flushAndEnqueueNonDisplayingSamples(Vector<RefPtr<MediaSample> >, AtomicString)
{
    notImplemented();
}

void SourceBufferPrivateGStreamer::enqueueSample(PassRefPtr<MediaSample>, AtomicString)
{
    notImplemented();
}

bool SourceBufferPrivateGStreamer::isReadyForMoreSamples(AtomicString)
{
    notImplemented();

    return false;
}

void SourceBufferPrivateGStreamer::setActive(bool isActive)
{
    if (m_mediaSource)
        m_mediaSource->sourceBufferPrivateDidChangeActiveState(this, isActive);
}

void SourceBufferPrivateGStreamer::stopAskingForMoreSamples(AtomicString)
{
    notImplemented();
}

void SourceBufferPrivateGStreamer::notifyClientWhenReadyForMoreSamples(AtomicString)
{
    notImplemented();
}

#if ENABLE(VIDEO_TRACK)
void SourceBufferPrivateGStreamer::didReceiveInitializationSegment(const SourceBufferPrivateClient::InitializationSegment& initializationSegment)
{
    if (m_sourceBufferPrivateClient)
        m_sourceBufferPrivateClient->sourceBufferPrivateDidReceiveInitializationSegment(this, initializationSegment);
}

void SourceBufferPrivateGStreamer::didReceiveSample(PassRefPtr<MediaSample> sample)
{
    if (m_sourceBufferPrivateClient)
        m_sourceBufferPrivateClient->sourceBufferPrivateDidReceiveSample(this, sample);
}

void SourceBufferPrivateGStreamer::didReceiveAllPendingSamples()
{
    if (m_sourceBufferPrivateClient) {
        m_sourceBufferPrivateClient->sourceBufferPrivateAppendComplete(this, SourceBufferPrivateClient::AppendSucceeded);
    }
}
#endif

}
#endif
