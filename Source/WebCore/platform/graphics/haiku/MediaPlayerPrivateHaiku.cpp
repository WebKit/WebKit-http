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

#include "NotImplemented.h"
#include "wtf/text/CString.h"
#include <media/MediaDefs.h>
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
{
}

MediaPlayerPrivate::~MediaPlayerPrivate()
{
    delete m_urlRequest;
}

IntSize MediaPlayerPrivate::naturalSize() const
{
    notImplemented();
    return IntSize(0,0);
}

bool MediaPlayerPrivate::hasAudio() const
{
    notImplemented();
    return false;
}

bool MediaPlayerPrivate::hasVideo() const
{
    notImplemented();
    return false;
}

void MediaPlayerPrivate::load(const String& url)
{
    m_urlRequest = BUrlProtocolRoster::MakeRequest(BUrl(url.utf8().data()), this);
    if (m_urlRequest)
        m_urlRequest->Run();
}

void MediaPlayerPrivate::cancelLoad()
{
    m_urlRequest->Stop();
    delete m_urlRequest;
    m_urlRequest = nullptr;
}

void MediaPlayerPrivate::play()
{
    notImplemented();
}

void MediaPlayerPrivate::pause()
{
    notImplemented();
}

bool MediaPlayerPrivate::paused() const
{
    notImplemented();
    return false;
}

bool MediaPlayerPrivate::seeking() const
{
    notImplemented();
    return false;
}
 
MediaPlayer::NetworkState MediaPlayerPrivate::networkState() const
{
    notImplemented();
    return MediaPlayer::Loaded;
}

MediaPlayer::ReadyState MediaPlayerPrivate::readyState() const
{
    notImplemented();
    return MediaPlayer::HaveNothing;
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
}

bool MediaPlayerPrivate::didLoadingProgress() const
{
    bool progress = m_didReceiveData;
    m_didReceiveData = false;
    return progress;
}

void MediaPlayerPrivate::setVisible(bool)
{
    notImplemented();
}

void MediaPlayerPrivate::setSize(const IntSize&)
{
    notImplemented();
}

void MediaPlayerPrivate::paint(GraphicsContext*, const IntRect&)
{
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

    printf("UNSUPPORTED TYPE: %s\n", parameters.type.utf8().data());
    return MediaPlayer::IsNotSupported;
}

}

#endif
