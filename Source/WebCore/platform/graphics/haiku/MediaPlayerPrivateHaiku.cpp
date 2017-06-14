/*
 * Copyright (C) 2014-2016 Haiku, Inc.
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

#include "config.h"
#include "MediaPlayerPrivateHaiku.h"

#if ENABLE(VIDEO)

#include "GraphicsContext.h"
#include "NotImplemented.h"
#include "wtf/text/CString.h"

#include <Bitmap.h>
#include <DataIO.h>
#include <HttpRequest.h>
#include <MediaDefs.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <SoundPlayer.h>
#include <UrlProtocolRoster.h>
#include <UrlRequest.h>
#include <View.h>

namespace WebCore {

void MediaPlayerPrivate::registerMediaEngine(MediaEngineRegistrar registrar)
{
    registrar([](MediaPlayer* player) { return std::make_unique<MediaPlayerPrivate>(player); },
        getSupportedTypes, supportsType, 0, 0, 0, 0);
}

MediaPlayerPrivate::MediaPlayerPrivate(MediaPlayer* player)
    : m_didReceiveData(false)
    , m_mediaFile(nullptr)
    , m_audioTrack(nullptr)
    , m_videoTrack(nullptr)
    , m_soundPlayer(nullptr)
    , m_frameBuffer(nullptr)
    , m_player(player)
    , m_networkState(MediaPlayer::Empty)
    , m_readyState(MediaPlayer::HaveNothing)
    , m_volume(1.0)
    , m_currentTime(0.f)
    , m_paused(true)
{
}

MediaPlayerPrivate::~MediaPlayerPrivate()
{
    cancelLoad();

    delete m_soundPlayer;
    delete m_mediaFile;
    delete m_frameBuffer;
}

void MediaPlayerPrivate::load(const String& url)
{
    // Cleanup from previous request (can this even happen?)
    if (m_soundPlayer)
        m_soundPlayer->Stop();
    delete m_soundPlayer;

    cancelLoad();

    // Deleting the BMediaFile release the tracks
    m_audioTrack = nullptr;
    m_videoTrack = nullptr;
    delete m_mediaFile;
    m_mediaFile = nullptr;

    // TODO we need more detailed info from the BMediaFile to accurately report
    // the m_readyState and the m_networkState to WebKit. The API will need to
    // be extended on Haiku side to query the internal state (and probably
    // BMediaFile should not block until data is ready?)
    IdentifyTracks(url);
    if (m_mediaFile && m_mediaFile->InitCheck() == B_OK) {
        m_player->characteristicChanged();
        m_player->durationChanged();
        m_player->sizeChanged();
        m_player->firstVideoFrameAvailable();

        //m_readyState = MediaPlayer::HaveMetadata;
        //m_readyState = MediaPlayer::HaveFutureData;
        m_readyState = MediaPlayer::HaveEnoughData;
        m_networkState = MediaPlayer::Loaded; // Loading;
    } else {
        m_networkState = MediaPlayer::FormatError;
        m_readyState = MediaPlayer::HaveMetadata;
    }
    m_player->networkStateChanged();
    m_player->readyStateChanged();
}

void MediaPlayerPrivate::cancelLoad()
{
    delete m_mediaFile;
    m_mediaFile = nullptr;
}

void MediaPlayerPrivate::prepareToPlay()
{
    // TODO should we seek the tracks to 0? reset m_currentTime? other things?
}

void MediaPlayerPrivate::playCallback(void* cookie, void* buffer,
    size_t /*size*/, const media_raw_audio_format& /*format*/)
{
    MediaPlayerPrivate* player = (MediaPlayerPrivate*)cookie;

    // TODO handle the case where there is a video, but no audio track.
    player->m_currentTime = player->m_audioTrack->CurrentTime() / 1000000.f;

    int64 size64;
    if (player->m_audioTrack->ReadFrames(buffer, &size64) != B_OK)
    {
        // Notify that we're done playing...
        player->m_currentTime = player->m_audioTrack->Duration() / 1000000.f;
        player->m_soundPlayer->Stop();
        callOnMainThread([player] {player->m_player->timeChanged();});
    }

    if (player->m_videoTrack) {
        if (player->m_videoTrack->CurrentTime() 
            < player->m_audioTrack->CurrentTime())
        {
            // Decode a video frame and show it on screen
            int64 count;
            player->m_videoTrack->ReadFrames(player->m_frameBuffer->Bits(),
                &count);

            callOnMainThread([player] { player->m_player->repaint(); });
        }
    }
}

void MediaPlayerPrivate::play()
{
    if (m_soundPlayer)
        m_soundPlayer->Start();
    m_paused = false;
}

void MediaPlayerPrivate::pause()
{
    if (m_soundPlayer)
        m_soundPlayer->Stop();
    m_paused = true;
}

FloatSize MediaPlayerPrivate::naturalSize() const
{
    if (!m_frameBuffer)
        return FloatSize(0,0);

    BRect r(m_frameBuffer->Bounds());
    return FloatSize(r.Width() + 1, r.Height() + 1);
}

bool MediaPlayerPrivate::hasAudio() const
{
    return m_audioTrack;
}

bool MediaPlayerPrivate::hasVideo() const
{
    return m_videoTrack;
}

void MediaPlayerPrivate::setVisible(bool)
{
    notImplemented();
}

float MediaPlayerPrivate::duration() const
{
    // TODO handle the case where there is a video, but no audio track.
    if (!m_audioTrack)
        return 0;
    return m_audioTrack->Duration() / 1000000.f;
}

float MediaPlayerPrivate::currentTime() const
{
    return m_currentTime;
}

void MediaPlayerPrivate::seek(float time)
{
    // TODO we should make sure the cache is ready to serve this. The idea is:
    // * Seek the tracks using SeekToTime
    // * The decoder will try to read "somewhere" in the cache. The Read call
    // should block, and check if the data is already downloaded
    // * If not, it should wait for it (and a sufficient buffer)
    //
    // Generally, we shouldn't let the reads to the cache return uninitialized
    // data. Note that we will probably need HTTP range requests support.

    bigtime_t newTime = (bigtime_t)(time * 1000000);
    // Usually, seeking the video is rounded to the nearest keyframe. This
    // modifies newTime, and we pass the adjusted value to the audio track, to
    // keep them in sync
    if (m_videoTrack)
        m_videoTrack->SeekToTime(&newTime);
    if (m_audioTrack)
        m_audioTrack->SeekToTime(&newTime);
}

bool MediaPlayerPrivate::seeking() const
{
    notImplemented();
    return false;
}
 
bool MediaPlayerPrivate::paused() const
{
    return m_paused;
}

void MediaPlayerPrivate::setVolume(float volume)
{
    m_volume = volume;
    if (m_soundPlayer)
        m_soundPlayer->SetVolume(volume);
}

MediaPlayer::NetworkState MediaPlayerPrivate::networkState() const
{
    return m_networkState;
}

MediaPlayer::ReadyState MediaPlayerPrivate::readyState() const
{
    return m_readyState;
}

std::unique_ptr<PlatformTimeRanges> MediaPlayerPrivate::buffered() const
{
    notImplemented();
    auto timeRanges = std::make_unique<PlatformTimeRanges>();
    return timeRanges;
}

bool MediaPlayerPrivate::didLoadingProgress() const
{
    bool progress = m_didReceiveData;
    m_didReceiveData = false;
    return progress;
}

void MediaPlayerPrivate::setSize(const IntSize&)
{
    notImplemented();
}

void MediaPlayerPrivate::paint(GraphicsContext& context, const FloatRect& r)
{
    if (context.paintingDisabled())
        return;

    if (!m_player->visible())
        return;

    if (m_frameBuffer) {
        BView* target = context.platformContext();
        target->SetDrawingMode(B_OP_COPY);
        target->DrawBitmap(m_frameBuffer, r);
    }
}

// #pragma mark - private methods

void MediaPlayerPrivate::IdentifyTracks(const String& url)
{
    // TODO something here is blocking inside the Media Kit. We should rework
    // things so that this is run in a separate thread (Media Thread?). This
    // would avoid the annoying freeze whenever a media is being loaded.
    m_mediaFile = new BMediaFile(BUrl(url.utf8().data()));

    if (m_mediaFile->InitCheck() == B_OK) {
        for (int i = m_mediaFile->CountTracks() - 1; i >= 0; i--)
        {
            BMediaTrack* track = m_mediaFile->TrackAt(i);

            media_format format;
            track->DecodedFormat(&format);

            if (format.IsVideo()) {
                if (m_videoTrack)
                    continue;
                m_videoTrack = track;

                m_frameBuffer = new BBitmap(
                    BRect(0, 0, format.Width() - 1, format.Height() - 1),
                    B_RGB32);

                if (m_audioTrack)
                    break;
            }

            if (format.IsAudio()) {
                if (m_audioTrack)
                    continue;
                m_audioTrack = track;

                m_soundPlayer = new BSoundPlayer(&format.u.raw_audio,
                    "HTML5 Audio", playCallback, NULL, this);
                m_soundPlayer->SetVolume(m_volume);
                if (!m_paused)
                    m_soundPlayer->Start();

                if (m_videoTrack)
                    break;
            }
        }
    }
}

// #pragma mark - static methods

static HashSet<String> mimeTypeCache()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(HashSet<String>, cache, ());
    static bool typeListInitialized = false;

    if (typeListInitialized)
        return cache;

    int32 cookie = 0;
    media_file_format mfi;

    // Add the types the Haiku Media Kit add-ons advertise support for
    while(get_next_file_format(&cookie, &mfi) == B_OK) {
        cache.add(String(mfi.mime_type));
    }

    typeListInitialized = true;
    return cache;
}

void MediaPlayerPrivate::getSupportedTypes(HashSet<String>& types)
{
    types = mimeTypeCache();
}

MediaPlayer::SupportsType MediaPlayerPrivate::supportsType(const MediaEngineSupportParameters& parameters)
{
    if (parameters.type.isNull() || parameters.type.isEmpty())
        return MediaPlayer::IsNotSupported;

    // spec says we should not return "probably" if the codecs string is empty
    if (mimeTypeCache().contains(parameters.type)) {
        return parameters.codecs.isEmpty() ? MediaPlayer::MayBeSupported : MediaPlayer::IsSupported;
    }

    return MediaPlayer::IsNotSupported;
}

}

#endif
