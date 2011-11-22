/*
 * Copyright (C) 2011 Ericsson AB. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Ericsson nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
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

#ifndef MediaStreamCenter_h
#define MediaStreamCenter_h

#if ENABLE(MEDIA_STREAM)

#include "MediaStreamSource.h"
#include <wtf/OwnPtr.h>

namespace WebCore {

class MediaStreamDescriptor;

class MediaStreamSourcesQueryClient : public RefCounted<MediaStreamSourcesQueryClient> {
public:
    virtual ~MediaStreamSourcesQueryClient() { }

    virtual bool audio() const = 0;
    virtual bool video() const = 0;

    virtual void mediaStreamSourcesQueryCompleted(const MediaStreamSourceVector&) = 0;
};

class MediaStreamCenter {
    WTF_MAKE_NONCOPYABLE(MediaStreamCenter);
    WTF_MAKE_FAST_ALLOCATED;
public:
    ~MediaStreamCenter();

    static MediaStreamCenter& instance();

    void queryMediaStreamSources(PassRefPtr<MediaStreamSourcesQueryClient>);

    // FIXME: add a way to mute a MediaStreamSource from the WebKit API layer

    // Calls from the DOM objects to notify the platform
    void didSetMediaStreamTrackEnabled(MediaStreamDescriptor*, unsigned componentIndex);
    void didStopLocalMediaStream(MediaStreamDescriptor*);

    // Calls from the platform to update the DOM objects
    void endLocalMediaStream(MediaStreamDescriptor*);

private:
    MediaStreamCenter();
};

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

#endif // MediaStreamCenter_h
