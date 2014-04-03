/*
 * Copyright (C) 2014 Haiku, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * aint with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef MediaPlayerPrivateHaiku_h
#define MediaPlayerPrivateHaiku_h
#if ENABLE(VIDEO)

#include "MediaPlayerPrivate.h"

#include <UrlProtocolAsynchronousListener.h>

class BDataIO;
class BMediaFile;
class BMediaTrack;
class BUrlRequest;

namespace WebCore {

class MediaPlayerPrivate : public MediaPlayerPrivateInterface, BUrlProtocolAsynchronousListener {
    public:
        static void registerMediaEngine(MediaEngineRegistrar);

        ~MediaPlayerPrivate();

        void load(const String& url) override;
        void cancelLoad() override;

        void prepareToPlay() override;

        void play() override;
        void pause() override;

        IntSize naturalSize() const override;
        bool hasAudio() const override;
        bool hasVideo() const override;

        void setVisible(bool) override;

        float duration() const override;
        float currentTime() const override;

        bool seeking() const override;
        bool paused() const override;
        
        MediaPlayer::NetworkState networkState() const override;
        MediaPlayer::ReadyState readyState() const override;
        
        std::unique_ptr<PlatformTimeRanges> buffered() const override;
        bool didLoadingProgress() const override;

        void setSize(const IntSize&) override;

        void paint(GraphicsContext*, const IntRect&) override;

        // BUrlProtocolListener API
	    void DataReceived(BUrlRequest* caller, const char* data, ssize_t size) override;
        void DownloadProgress(BUrlRequest*, ssize_t, ssize_t) override;
        void RequestCompleted(BUrlRequest*, bool success) override;
    private:
        MediaPlayerPrivate(MediaPlayer*);
        
        void IdentifyTracks();

        // engine support
        static PassOwnPtr<MediaPlayerPrivateInterface> create(MediaPlayer*);
        static void getSupportedTypes(HashSet<String>& types);
        static MediaPlayer::SupportsType supportsType(const MediaEngineSupportParameters&);

        mutable bool m_didReceiveData;
        BUrlRequest* m_urlRequest;
        BPositionIO* m_cache;
        off_t m_writePos;
        BMediaFile* m_mediaFile;
        BMediaTrack* m_audioTrack;
        BMediaTrack* m_videoTrack;

        MediaPlayer* m_player;
        MediaPlayer::NetworkState m_networkState;
        MediaPlayer::ReadyState m_readyState;
};

}

#endif
#endif
