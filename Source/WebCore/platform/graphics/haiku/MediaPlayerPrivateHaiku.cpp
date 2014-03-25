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
{
}

MediaPlayerPrivate::~MediaPlayerPrivate()
{
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
    notImplemented();
}

void MediaPlayerPrivate::cancelLoad()
{
    notImplemented();
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

bool MediaPlayerPrivate::didLoadingProgress() const
{
    notImplemented();
    return false;
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

void MediaPlayerPrivate::getSupportedTypes(HashSet<String>& types)
{
    notImplemented();
}

MediaPlayer::SupportsType MediaPlayerPrivate::supportsType(const MediaEngineSupportParameters& parameters)
{
    notImplemented();
    return MediaPlayer::IsNotSupported;
}

}

#endif
