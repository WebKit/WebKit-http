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

#include "config.h"
#include "MediaPlayerPrivateHaiku.h"

#if ENABLE(VIDEO)

#include "GraphicsContext.h"
#include "NotImplemented.h"
#include "wtf/text/CString.h"

#include <Bitmap.h>
#include <DataIO.h>
#include <MediaDefs.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <SoundPlayer.h>
#include <UrlProtocolRoster.h>
#include <UrlRequest.h>
#include <View.h>

namespace WebCore {

PassOwnPtr<MediaPlayerPrivateInterface> MediaPlayerPrivate::create(MediaPlayer* player)
{
    return adoptPtr(new MediaPlayerPrivate(player));
}

void MediaPlayerPrivate::registerMediaEngine(MediaEngineRegistrar registrar)
{
    registrar(create, getSupportedTypes, supportsType, 0, 0, 0, 0);
}

MediaPlayerPrivate::MediaPlayerPrivate(MediaPlayer* player)
    : m_didReceiveData(false)
    , m_urlRequest(nullptr)
    , m_cache(new BMallocIO())
    , m_writePos(0)
    , m_mediaFile(nullptr)
    , m_audioTrack(nullptr)
    , m_videoTrack(nullptr)
    , m_soundPlayer(nullptr)
    , m_player(player)
    , m_networkState(MediaPlayer::Empty)
    , m_readyState(MediaPlayer::HaveNothing)
    , m_volume(1.0)
    , m_paused(true)
{
}

MediaPlayerPrivate::~MediaPlayerPrivate()
{
    delete m_urlRequest;
    delete m_soundPlayer;
    delete m_cache;
    if (m_mediaFile) {
        m_mediaFile->ReleaseTrack(m_audioTrack);
        m_mediaFile->ReleaseTrack(m_videoTrack);
    }
    delete m_mediaFile;
}

void MediaPlayerPrivate::load(const String& url)
{
    // Cleanup from previous request (can this even happen?)
    if (m_soundPlayer)
        m_soundPlayer->Stop();
    delete m_soundPlayer;

    delete m_urlRequest;
    m_urlRequest = nullptr;
    if (m_mediaFile) {
        m_mediaFile->ReleaseTrack(m_audioTrack);
        m_mediaFile->ReleaseTrack(m_videoTrack);
        m_audioTrack = nullptr;
        m_videoTrack = nullptr;
    }
    delete m_mediaFile;
    m_mediaFile = nullptr;
    
    m_writePos = 0;
    m_cache->SetSize(0);
    m_cache->Seek(0, SEEK_SET);

    m_urlRequest = BUrlProtocolRoster::MakeRequest(BUrl(url.utf8().data()),
        new BUrlProtocolDispatchingListener(this));

    if (m_urlRequest)
        m_networkState = MediaPlayer::Idle;
    else
        m_networkState = MediaPlayer::FormatError;
    m_player->networkStateChanged();

    m_readyState = MediaPlayer::HaveNothing;
    m_player->readyStateChanged();
}

void MediaPlayerPrivate::cancelLoad()
{
    m_urlRequest->Stop();
    delete m_urlRequest;
    m_urlRequest = nullptr;
}

void MediaPlayerPrivate::prepareToPlay()
{
    if (m_urlRequest) {
        m_urlRequest->Run();

        m_networkState = MediaPlayer::Loading;
    } else
        m_networkState = MediaPlayer::NetworkError;

    m_readyState = MediaPlayer::HaveNothing;

    m_player->networkStateChanged();
    m_player->readyStateChanged();
}

void MediaPlayerPrivate::playCallback(void* cookie, void* buffer,
    size_t size, const media_raw_audio_format& format)
{
    MediaPlayerPrivate* player = (MediaPlayerPrivate*)cookie;

	int64 size64;
	if (player->m_audioTrack->ReadFrames(buffer, &size64) != B_OK)
    {
        player->pause();
        return;
    }

    if (player->m_videoTrack) {
        if (player->m_videoTrack->CurrentTime() 
            < player->m_audioTrack->CurrentTime())
        {
            // Decode a video frame and show it on screen
            int64 count;
            player->m_videoTrack->ReadFrames(player->m_frameBuffer->Bits(),
                &count);
            BMessenger(player).SendMessage('rfsh');
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

IntSize MediaPlayerPrivate::naturalSize() const
{
    notImplemented();
    return IntSize(0,0);
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
    // TODO handle the case where there is a video, but no audio track.
    if (!m_audioTrack)
        return 0;

    return m_audioTrack->CurrentTime() / 1000000.f;
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
    auto timeRanges = PlatformTimeRanges::create();
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

void MediaPlayerPrivate::paint(GraphicsContext* context, const IntRect& r)
{
    if (context->paintingDisabled())
        return;

    if (!m_player->visible())
        return;

    context->platformContext()->DrawBitmap(m_frameBuffer, r);
}

// #pragma mark - BUrlProtocolListener

void MediaPlayerPrivate::DataReceived(BUrlRequest*, const char* data, ssize_t size)
{
    // Don't use Write(), because the media decoder may use the seek position.
    m_writePos += m_cache->WriteAt(m_writePos, data, size);
}

void MediaPlayerPrivate::DownloadProgress(BUrlRequest*, ssize_t currentSize,
    ssize_t totalSize)
{
    m_cache->SetSize(totalSize);
    m_didReceiveData = true;

    if (currentSize >= std::min((ssize_t)256*1024, totalSize)) {
        IdentifyTracks();
        // TODO be smarter here. We know the media play time, and we can
        // estimate the download end time. We should not start playing until
        // we think the media will finish playing after the download is done.
        if (m_readyState != MediaPlayer::HaveFutureData) {
            m_readyState = MediaPlayer::HaveFutureData;
            m_player->readyStateChanged();
        }
    }
}

void MediaPlayerPrivate::RequestCompleted(BUrlRequest*, bool success)
{
    if(success) {
        m_networkState = MediaPlayer::Loaded;
        m_readyState = MediaPlayer::HaveEnoughData;
        m_player->readyStateChanged();
    } else
        m_networkState = MediaPlayer::NetworkError;

    m_player->networkStateChanged();
}

void MediaPlayerPrivate::MessageReceived(BMessage* message)
{
    if (message->what == 'rfsh')
    {
        m_player->repaint();
        return;
    }
    BUrlProtocolAsynchronousListener::MessageReceived(message);
}

// #pragma mark - private methods

void MediaPlayerPrivate::IdentifyTracks()
{
    // Check if we already did this.
    if (m_mediaFile)
        return;

    m_mediaFile = new BMediaFile(m_cache);
    if (m_mediaFile->InitCheck() == B_OK) {
        for (int i = m_mediaFile->CountTracks() - 1; i >= 0; i--)
        {
            BMediaTrack* track = m_mediaFile->TrackAt(i);

            media_format format;
            track->DecodedFormat(&format);

            if (format.IsVideo()) {
                m_videoTrack = track;

                m_frameBuffer = new BBitmap(
                    BRect(0, 0, format.Width() - 1, format.Height() - 1),
                    format.ColorSpace());

                if (m_audioTrack)
                    break;
            }

            if (format.IsAudio()) {
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

        m_player->characteristicChanged();

        m_readyState = MediaPlayer::HaveMetadata;
        m_player->readyStateChanged();
    } else {
        // Not enough data to decode the header yet. Try again later!
        delete m_mediaFile;
        m_mediaFile = nullptr;
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
    if (mimeTypeCache().contains(parameters.type))
        return parameters.codecs.isEmpty() ? MediaPlayer::MayBeSupported : MediaPlayer::IsSupported;

    return MediaPlayer::IsNotSupported;
}

}

#endif
