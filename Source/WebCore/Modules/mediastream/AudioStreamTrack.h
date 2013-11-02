/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef AudioStreamTrack_h
#define AudioStreamTrack_h

#if ENABLE(MEDIA_STREAM)

#include "MediaStreamTrack.h"
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

namespace WebCore {

class MediaStreamSource;
class ScriptExecutionContext;

class AudioStreamTrack FINAL : public MediaStreamTrack {
public:
    static RefPtr<AudioStreamTrack> create(ScriptExecutionContext&, const Dictionary&);
    static RefPtr<AudioStreamTrack> create(ScriptExecutionContext&, MediaStreamTrackPrivate&);
    static RefPtr<AudioStreamTrack> create(MediaStreamTrack&);

    virtual ~AudioStreamTrack() { }

    virtual const AtomicString& kind() const OVERRIDE;

private:
    AudioStreamTrack(ScriptExecutionContext&, MediaStreamTrackPrivate&, const Dictionary*);
    explicit AudioStreamTrack(MediaStreamTrack&);
};

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

#endif // AudioStreamTrack_h
