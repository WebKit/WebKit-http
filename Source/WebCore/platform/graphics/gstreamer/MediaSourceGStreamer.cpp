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
#include "MediaSourceGStreamer.h"

#if ENABLE(MEDIA_SOURCE) && USE(GSTREAMER)

#include "ContentType.h"
#include "MediaPlayerPrivateGStreamer.h"
#include "NotImplemented.h"
#include "SourceBufferPrivateGStreamer.h"
#include "WebKitMediaSourceGStreamer.h"
#include <wtf/gobject/GRefPtr.h>
#include <wtf/PassRefPtr.h>
#include "NotImplemented.h"
#include "TimeRanges.h"

namespace WebCore {

void MediaSourceGStreamer::open(MediaSourcePrivateClient* mediaSource, WebKitMediaSrc* src, MediaPlayerPrivateGStreamer* playerPrivate)
{
    ASSERT(mediaSource);
    RefPtr<MediaSourceGStreamer> mediaSourcePrivate = adoptRef(new MediaSourceGStreamer(mediaSource, src));
    mediaSourcePrivate->m_playerPrivate = playerPrivate;
    mediaSource->setPrivateAndOpen(mediaSourcePrivate.releaseNonNull());
}

MediaSourceGStreamer::MediaSourceGStreamer(MediaSourcePrivateClient* mediaSource, WebKitMediaSrc* src)
    : MediaSourcePrivate()
    , m_mediaSource(mediaSource)
{
    m_client = MediaSourceClientGStreamer::create(src);
}

MediaSourceGStreamer::~MediaSourceGStreamer()
{
    for (HashSet<SourceBufferPrivateGStreamer*>::iterator it = m_sourceBuffers.begin(), end = m_sourceBuffers.end(); it != end; ++it)
        (*it)->clearMediaSource();
}

MediaSourceGStreamer::AddStatus MediaSourceGStreamer::addSourceBuffer(const ContentType& contentType, RefPtr<SourceBufferPrivate>& sourceBufferPrivate)
{
    // MediaEngineSupportParameters parameters;
    // parameters.isMediaSource = true;
    // parameters.type = contentType.type();
    // parameters.codecs = contentType.parameter(ASCIILiteral("codecs"));
    // if (MediaPlayerPrivateGStreamer::supportsType(parameters) == MediaPlayer::IsNotSupported)
    //     return NotSupported;

    RefPtr<SourceBufferPrivateGStreamer> buffer = SourceBufferPrivateGStreamer::create(this, m_client, contentType);
    m_sourceBuffers.add(buffer.get());
    sourceBufferPrivate = buffer;
    return m_client->addSourceBuffer(buffer, contentType);
}

void MediaSourceGStreamer::removeSourceBuffer(SourceBufferPrivate* buffer)
{
    SourceBufferPrivateGStreamer* sourceBufferPrivateGStreamer = reinterpret_cast<SourceBufferPrivateGStreamer*>(buffer);
    ASSERT(m_sourceBuffers.contains(sourceBufferPrivateGStreamer));

    sourceBufferPrivateGStreamer->clearMediaSource();
    m_sourceBuffers.remove(sourceBufferPrivateGStreamer);
    m_activeSourceBuffers.remove(sourceBufferPrivateGStreamer);
}

void MediaSourceGStreamer::durationChanged()
{
    m_client->durationChanged(m_mediaSource->duration());
}

void MediaSourceGStreamer::markEndOfStream(EndOfStreamStatus status)
{
    m_client->markEndOfStream(status);
}

void MediaSourceGStreamer::unmarkEndOfStream()
{
    notImplemented();
}

MediaPlayer::ReadyState MediaSourceGStreamer::readyState() const
{
    return m_playerPrivate->readyState();
}

void MediaSourceGStreamer::setReadyState(MediaPlayer::ReadyState state)
{
    m_playerPrivate->setReadyState(state);
}

void MediaSourceGStreamer::waitForSeekCompleted()
{
    notImplemented();
}

void MediaSourceGStreamer::seekCompleted()
{
    notImplemented();
}

void MediaSourceGStreamer::sourceBufferPrivateDidChangeActiveState(SourceBufferPrivateGStreamer* buffer, bool isActive)
{
    if (isActive && !m_activeSourceBuffers.contains(buffer))
        m_activeSourceBuffers.add(buffer);

    if (!isActive)
        m_activeSourceBuffers.remove(buffer);
}

std::unique_ptr<PlatformTimeRanges> MediaSourceGStreamer::buffered() {
    return m_mediaSource->buffered();
}

}
#endif
