/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 * Copyright (C) 2013-2014 Apple Inc. All rights reserved.
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
#ifndef SourceBufferPrivate_h
#define SourceBufferPrivate_h

#if ENABLE(MEDIA_SOURCE)

#include "MediaPlayer.h"
#include "TimeRanges.h"
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

namespace WebCore {

class MediaSample;
class SourceBufferPrivateClient;
class TimeRanges;

class SourceBufferPrivate : public RefCounted<SourceBufferPrivate> {
public:
    virtual ~SourceBufferPrivate() { }

    virtual void setClient(SourceBufferPrivateClient*) = 0;

    virtual void append(const unsigned char* data, unsigned length) = 0;
    virtual void abort() = 0;
    virtual void removedFromMediaSource() = 0;

    virtual MediaPlayer::ReadyState readyState() const = 0;
    virtual void setReadyState(MediaPlayer::ReadyState) = 0;

    virtual void flushAndEnqueueNonDisplayingSamples(Vector<RefPtr<MediaSample>>, AtomicString) { }
    virtual void enqueueSample(PassRefPtr<MediaSample>, AtomicString) { }
    virtual bool isReadyForMoreSamples(AtomicString) { return false; }
    virtual void setActive(bool) { }
    virtual void stopAskingForMoreSamples(AtomicString) { }
    virtual void notifyClientWhenReadyForMoreSamples(AtomicString) { }
};

}

#endif
#endif
