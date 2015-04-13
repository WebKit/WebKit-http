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

#ifndef SourceBufferPrivateGStreamer_h
#define SourceBufferPrivateGStreamer_h

#if ENABLE(MEDIA_SOURCE) && USE(GSTREAMER)

#include "ContentType.h"
#include "SourceBufferPrivate.h"
#include "SourceBufferPrivateClient.h"
#include "WebKitMediaSourceGStreamer.h"

namespace WebCore {

class MediaSourceGStreamer;

class SourceBufferPrivateGStreamer final : public SourceBufferPrivate {

public:
    static PassRefPtr<SourceBufferPrivateGStreamer> create(MediaSourceGStreamer*, PassRefPtr<MediaSourceClientGStreamer>, const ContentType&);
    virtual ~SourceBufferPrivateGStreamer();

    void clearMediaSource() { m_mediaSource = 0; }

    virtual void setClient(SourceBufferPrivateClient*) override;
    virtual void append(const unsigned char* data, unsigned length) override;
    virtual void abort() override;
    virtual void removedFromMediaSource() override;
#if !ENABLE(VIDEO_TRACK)
    virtual PassRefPtr<TimeRanges> buffered() override;
#endif
    virtual MediaPlayer::ReadyState readyState() const override;
    virtual void setReadyState(MediaPlayer::ReadyState) override;

    virtual void flushAndEnqueueNonDisplayingSamples(Vector<RefPtr<MediaSample> >, AtomicString) override;
    virtual void enqueueSample(PassRefPtr<MediaSample>, AtomicString) override;
    virtual bool isReadyForMoreSamples(AtomicString) override;
    virtual void setActive(bool) override;
    virtual void stopAskingForMoreSamples(AtomicString) override;
    virtual void notifyClientWhenReadyForMoreSamples(AtomicString) override;
    virtual bool isAborted() { return m_aborted; }
    virtual void resetAborted() { m_aborted = false; }

private:
    SourceBufferPrivateGStreamer(MediaSourceGStreamer*, PassRefPtr<MediaSourceClientGStreamer>, const ContentType&);
    friend class MediaSourceClientGStreamer;

#if ENABLE(VIDEO_TRACK)
    void didReceiveInitializationSegment(const SourceBufferPrivateClient::InitializationSegment&);
    void didReceiveSample(PassRefPtr<MediaSample>);
    void didReceiveAllPendingSamples();
#endif

    MediaSourceGStreamer* m_mediaSource;
    ContentType m_type;
    RefPtr<MediaSourceClientGStreamer> m_client;
    SourceBufferPrivateClient* m_sourceBufferPrivateClient;
    bool m_aborted;
};

}

#endif
#endif
