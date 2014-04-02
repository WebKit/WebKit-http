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

#include <DataIO.h>
#include <MediaDefs.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <UrlProtocolRoster.h>
#include <UrlRequest.h>

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
    , m_mediaFile(nullptr)
    , m_player(player)
    , m_networkState(MediaPlayer::Empty)
    , m_readyState(MediaPlayer::HaveNothing)
{
}

MediaPlayerPrivate::~MediaPlayerPrivate()
{
    delete m_urlRequest;
    delete m_cache;
    delete m_mediaFile;
}

void MediaPlayerPrivate::load(const String& url)
{
    // Cleanup from previous request (can this even happen?)
    delete m_urlRequest;
    m_urlRequest = nullptr;
    delete m_mediaFile;
    m_mediaFile = nullptr;
    m_cache->SetSize(0);
    m_cache->Seek(0, SEEK_SET);

    m_urlRequest = BUrlProtocolRoster::MakeRequest(BUrl(url.utf8().data()),
        new BUrlProtocolDispatchingListener(this));

    if (m_urlRequest)
        m_networkState = MediaPlayer::Idle;
    else
        m_networkState = MediaPlayer::FormatError;
    m_player->networkStateChanged();
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

void MediaPlayerPrivate::play()
{
    notImplemented();
}

void MediaPlayerPrivate::pause()
{
    notImplemented();
}

IntSize MediaPlayerPrivate::naturalSize() const
{
    notImplemented();
    return IntSize(0,0);
}

bool MediaPlayerPrivate::hasAudio() const
{
    if(!m_mediaFile)
        return false;

    int id = -1;
    for(int i = m_mediaFile->CountTracks() - 1; i >= 0; i--)
    {
        BMediaTrack* track = m_mediaFile->TrackAt(i);

        media_format format;
        track->DecodedFormat(&format);

        if (format.IsAudio()) {
            id = i;

            // TODO we may want to keep the track around for decoding...
        }

        m_mediaFile->ReleaseTrack(track);
    }

    return id != -1;
}

bool MediaPlayerPrivate::hasVideo() const
{
    if(!m_mediaFile)
        return false;

    int id = -1;
    for(int i = m_mediaFile->CountTracks() - 1; i >= 0; i--)
    {
        BMediaTrack* track = m_mediaFile->TrackAt(i);

        media_format format;
        track->DecodedFormat(&format);

        if (format.IsVideo()) {
            id = i;

            // TODO we may want to keep the track around for decoding...
        }

        m_mediaFile->ReleaseTrack(track);
    }

    return id != -1;
}

void MediaPlayerPrivate::setVisible(bool)
{
    notImplemented();
}

float MediaPlayerPrivate::duration() const
{
    if (!m_mediaFile)
        return 0;

    BMediaTrack* track = m_mediaFile->TrackAt(0);
    if (!track)
        return 0;

    return track->Duration() / 1000000.f;

    m_mediaFile->ReleaseTrack(track);
}

float MediaPlayerPrivate::currentTime() const
{
    if (!m_mediaFile)
        return 0;

    BMediaTrack* track = m_mediaFile->TrackAt(0);
    if (!track)
        return 0;

    return track->CurrentTime() / 1000000.f;

    m_mediaFile->ReleaseTrack(track);
}

bool MediaPlayerPrivate::seeking() const
{
    notImplemented();
    return false;
}
 
bool MediaPlayerPrivate::paused() const
{
    notImplemented();
    return true;
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

void MediaPlayerPrivate::DataReceived(BUrlRequest*, const char* data, ssize_t size)
{
    m_didReceiveData = true;
    // Don't use Write(), because the media decoder may use the seek position.
    off_t position;
    m_cache->GetSize(&position);
    m_cache->WriteAt(position, data, size);

    if (!m_mediaFile) {
        m_mediaFile = new BMediaFile(m_cache);
        if (m_mediaFile->InitCheck() == B_OK) {
            m_player->characteristicChanged();

            m_readyState = MediaPlayer::HaveMetadata;
            m_player->readyStateChanged();
        } else {
            // Not enough data to decode the header yet. Try again later!
            delete m_mediaFile;
            m_mediaFile = nullptr;
        }
    } else {
        // TODO check if that's true - hard to tell?
        // we need HaveFutureData so the playing starts, but later on if
        // the buffer gets empty we can go back to HaveCurrentData.
        m_readyState = MediaPlayer::HaveFutureData;
        m_player->readyStateChanged();
    }
}

void MediaPlayerPrivate::RequestCompleted(BUrlRequest*, bool success)
{
    if(success) {
puts("success!");
        m_networkState = MediaPlayer::Loaded;
        m_readyState = MediaPlayer::HaveEnoughData;
        m_player->readyStateChanged();
    } else
        m_networkState = MediaPlayer::NetworkError;

    m_player->networkStateChanged();
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

    notImplemented();
}

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
