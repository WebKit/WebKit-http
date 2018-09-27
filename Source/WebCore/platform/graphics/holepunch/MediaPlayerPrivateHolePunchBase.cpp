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

#include "config.h"
#include "MediaPlayerPrivateHolePunchBase.h"

#if USE(COORDINATED_GRAPHICS_THREADED)
#include "IntRect.h"
#include "MediaPlayer.h"
#include "NotImplemented.h"
#include "TextureMapperGL.h"
#include "TextureMapperPlatformLayerBuffer.h"
#include <wtf/glib/GMutexLocker.h>


using namespace std;

namespace WebCore {

MediaPlayerPrivateHolePunchBase::MediaPlayerPrivateHolePunchBase(MediaPlayer* player)
    :m_player(player)
{
#if USE(COORDINATED_GRAPHICS_THREADED)
    m_platformLayerProxy = adoptRef(new TextureMapperPlatformLayerProxy());
    LockHolder locker(m_platformLayerProxy->lock());
    m_platformLayerProxy->pushNextBuffer(std::make_unique<TextureMapperPlatformLayerBuffer>(0, m_size, TextureMapperGL::ShouldOverwriteRect, GL_DONT_CARE));
#endif
}

MediaPlayerPrivateHolePunchBase::~MediaPlayerPrivateHolePunchBase()
{
#if USE(TEXTURE_MAPPER_GL)
    if (client())
        client()->platformLayerWillBeDestroyed();
#endif
    m_player = 0;
}

FloatSize MediaPlayerPrivateHolePunchBase::naturalSize() const
{
    return FloatSize();
}

MediaPlayer::NetworkState MediaPlayerPrivateHolePunchBase::networkState() const
{
    return MediaPlayer::Empty;
}

MediaPlayer::ReadyState MediaPlayerPrivateHolePunchBase::readyState() const
{
    return MediaPlayer::HaveNothing;
}

void MediaPlayerPrivateHolePunchBase::setSize(const IntSize& size)
{
    m_size = size;
}

void MediaPlayerPrivateHolePunchBase::swapBuffersIfNeeded()
{
    LockHolder locker(m_platformLayerProxy->lock());
    m_platformLayerProxy->pushNextBuffer(std::make_unique<TextureMapperPlatformLayerBuffer>(0, m_size, TextureMapperGL::ShouldOverwriteRect, GL_DONT_CARE));
}

}

#endif // USE(COORDINATED_GRAPHICS_THREADED)
