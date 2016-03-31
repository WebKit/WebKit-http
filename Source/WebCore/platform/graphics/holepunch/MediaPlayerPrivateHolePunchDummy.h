/*
 * Copyright (C) 2015 Igalia S.L.
 * Copyright (C) 2015 Metrological
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MediaPlayerPrivateHolePunchDummy_h
#define MediaPlayerPrivateHolePunchDummy_h

#include "MediaPlayerPrivateHolePunchBase.h"

namespace WebCore {

class MediaPlayerPrivateHolePunchDummy : public MediaPlayerPrivateHolePunchBase {
public:
    explicit MediaPlayerPrivateHolePunchDummy(MediaPlayer*);
    ~MediaPlayerPrivateHolePunchDummy();

    static void registerMediaEngine(MediaEngineRegistrar);

    bool hasVideo() const override { return false; };
    bool hasAudio() const override { return false; };

    void load(const String&) override { };
#if ENABLE(MEDIA_SOURCE)
    void load(const String&, MediaSourcePrivateClient*) override { };
#endif
#if ENABLE(MEDIA_STREAM)
    void load(MediaStreamPrivate&) override { };
#endif
    void commitLoad() { };
    void cancelLoad() override { };

    void play() override { };
    void pause() override { };
    bool paused() const override { return false; };
    bool seeking() const override { return false; };
    std::unique_ptr<PlatformTimeRanges> buffered() const override { return std::make_unique<PlatformTimeRanges>(); };
    bool didLoadingProgress() const override { return false; };

private:
    static void getSupportedTypes(HashSet<String, ASCIICaseInsensitiveHash>&);
    static MediaPlayer::SupportsType supportsType(const MediaEngineSupportParameters&);
};
}
#endif
