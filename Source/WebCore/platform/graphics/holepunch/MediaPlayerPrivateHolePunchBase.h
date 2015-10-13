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

#ifndef MediaPlayerPrivateHolePunchBase_h
#define MediaPlayerPrivateHolePunchBase_h

#include "MediaPlayerPrivate.h"

#if USE(COORDINATED_GRAPHICS_THREADED)
#include "TextureMapperPlatformLayerProxy.h"
#endif

namespace WebCore {

class IntSize;

class MediaPlayerPrivateHolePunchBase : public MediaPlayerPrivateInterface
#if USE(COORDINATED_GRAPHICS_THREADED)
    , public TextureMapperPlatformLayerProxyProvider
#endif
{

public:
    MediaPlayerPrivateHolePunchBase(MediaPlayer*);
    virtual ~MediaPlayerPrivateHolePunchBase();

    virtual FloatSize naturalSize() const override;

    virtual void setVolume(float) override { };
    virtual float volume() const override { return 0; };

    virtual bool supportsMuting() const override { return true; }
    virtual void setMuted(bool) override { };

    virtual MediaPlayer::NetworkState networkState() const override;
    virtual MediaPlayer::ReadyState readyState() const override;

    virtual void setVisible(bool) override { }
    virtual void setSize(const IntSize&) override;

    virtual void paint(GraphicsContext&, const FloatRect&) override { };

#if USE(COORDINATED_GRAPHICS_THREADED)
    virtual PlatformLayer* platformLayer() const override { return const_cast<MediaPlayerPrivateHolePunchBase*>(this); }
    virtual bool supportsAcceleratedRendering() const override { return true; }
    virtual RefPtr<TextureMapperPlatformLayerProxy> proxy() const override { return m_platformLayerProxy.copyRef(); }
    virtual void swapBuffersIfNeeded() override { };
#endif

private:
    MediaPlayer* m_player;
    IntSize m_size;
#if USE(COORDINATED_GRAPHICS_THREADED)
    RefPtr<TextureMapperPlatformLayerProxy> m_platformLayerProxy;
#endif

};
}

#endif
