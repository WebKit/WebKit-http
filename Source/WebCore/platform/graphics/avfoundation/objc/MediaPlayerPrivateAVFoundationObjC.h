/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef MediaPlayerPrivateAVFoundationObjC_h
#define MediaPlayerPrivateAVFoundationObjC_h

#if ENABLE(VIDEO) && USE(AVFOUNDATION)

#include "CachedResourceClient.h"
#include "MediaPlayerPrivateAVFoundation.h"
#include <wtf/HashMap.h>

OBJC_CLASS AVURLAsset;
OBJC_CLASS AVPlayer;
OBJC_CLASS AVPlayerItem;
OBJC_CLASS AVPlayerItemVideoOutput;
OBJC_CLASS AVPlayerLayer;
OBJC_CLASS AVAssetImageGenerator;
OBJC_CLASS WebCoreAVFMovieObserver;

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
OBJC_CLASS WebCoreAVFLoaderDelegate;
OBJC_CLASS AVAssetResourceLoadingRequest;
#endif

#ifndef __OBJC__
typedef struct objc_object *id;
#endif

typedef struct CGImage *CGImageRef;
typedef struct __CVBuffer *CVPixelBufferRef;

namespace WebCore {

class WebCoreAVFResourceLoader;

class MediaPlayerPrivateAVFoundationObjC : public MediaPlayerPrivateAVFoundation {
public:
    ~MediaPlayerPrivateAVFoundationObjC();

    static void registerMediaEngine(MediaEngineRegistrar);

    void setAsset(id);
    virtual void tracksChanged();

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    bool shouldWaitForLoadingOfResource(AVAssetResourceLoadingRequest*);
    void didCancelLoadingRequest(AVAssetResourceLoadingRequest*);
#endif

private:
    MediaPlayerPrivateAVFoundationObjC(MediaPlayer*);

    // engine support
    static PassOwnPtr<MediaPlayerPrivateInterface> create(MediaPlayer*);
    static void getSupportedTypes(HashSet<String>& types);
    static MediaPlayer::SupportsType supportsType(const String& type, const String& codecs, const KURL&);
#if ENABLE(ENCRYPTED_MEDIA)
    static MediaPlayer::SupportsType extendedSupportsType(const String& type, const String& codecs, const String& keySystem, const KURL&);
#endif

    static bool isAvailable();

    virtual void cancelLoad();

    virtual PlatformMedia platformMedia() const;

    virtual void platformSetVisible(bool);
    virtual void platformPlay();
    virtual void platformPause();
    virtual float currentTime() const;
    virtual void setVolume(float);
    virtual void setClosedCaptionsVisible(bool);
    virtual void paint(GraphicsContext*, const IntRect&);
    virtual void paintCurrentFrameInContext(GraphicsContext*, const IntRect&);
    virtual PlatformLayer* platformLayer() const;
    virtual bool supportsAcceleratedRendering() const { return true; }
    virtual float mediaTimeForTimeValue(float) const;

    virtual void createAVPlayer();
    virtual void createAVPlayerItem();
    virtual void createAVAssetForURL(const String& url);
    virtual MediaPlayerPrivateAVFoundation::ItemStatus playerItemStatus() const;
    virtual MediaPlayerPrivateAVFoundation::AssetStatus assetStatus() const;

    virtual void checkPlayability();
    virtual void updateRate();
    virtual float rate() const;
    virtual void seekToTime(float time);
    virtual unsigned totalBytes() const;
    virtual PassRefPtr<TimeRanges> platformBufferedTimeRanges() const;
    virtual float platformMaxTimeSeekable() const;
    virtual float platformDuration() const;
    virtual float platformMaxTimeLoaded() const;
    virtual void beginLoadingMetadata();
    virtual void sizeChanged();

    virtual bool hasAvailableVideoFrame() const;

    virtual void createContextVideoRenderer();
    virtual void destroyContextVideoRenderer();

    virtual void createVideoLayer();
    virtual void destroyVideoLayer();

    virtual bool hasContextRenderer() const;
    virtual bool hasLayerRenderer() const;

    virtual bool hasSingleSecurityOrigin() const;

#if __MAC_OS_X_VERSION_MIN_REQUIRED < 1080
    void createImageGenerator();
    void destroyImageGenerator();
    RetainPtr<CGImageRef> createImageForTimeInRect(float, const IntRect&);
    void paintWithImageGenerator(GraphicsContext*, const IntRect&);
#else
    void createVideoOutput();
    void destroyVideoOutput();
    RetainPtr<CVPixelBufferRef> createPixelBuffer();
    void paintWithVideoOutput(GraphicsContext*, const IntRect&);
#endif

#if ENABLE(ENCRYPTED_MEDIA)
    virtual MediaPlayer::MediaKeyException addKey(const String&, const unsigned char*, unsigned, const unsigned char*, unsigned, const String&);
    virtual MediaPlayer::MediaKeyException generateKeyRequest(const String&, const unsigned char*, unsigned);
    virtual MediaPlayer::MediaKeyException cancelKeyRequest(const String&, const String&);
#endif

    RetainPtr<AVURLAsset> m_avAsset;
    RetainPtr<AVPlayer> m_avPlayer;
    RetainPtr<AVPlayerItem> m_avPlayerItem;
    RetainPtr<AVPlayerLayer> m_videoLayer;
    RetainPtr<WebCoreAVFMovieObserver> m_objcObserver;
    RetainPtr<id> m_timeObserver;
    bool m_videoFrameHasDrawn;
    bool m_haveCheckedPlayability;

#if __MAC_OS_X_VERSION_MIN_REQUIRED < 1080
    RetainPtr<AVAssetImageGenerator> m_imageGenerator;
#else
    RetainPtr<AVPlayerItemVideoOutput> m_videoOutput;
    RetainPtr<CVPixelBufferRef> m_lastImage;
#endif

#if ENABLE(ENCRYPTED_MEDIA) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    RetainPtr<WebCoreAVFLoaderDelegate> m_loaderDelegate;
    HashMap<String, RetainPtr<AVAssetResourceLoadingRequest> > m_keyURIToRequestMap;
    HashMap<String, RetainPtr<AVAssetResourceLoadingRequest> > m_sessionIDToRequestMap;
#endif

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    friend class WebCoreAVFResourceLoader;
    OwnPtr<WebCoreAVFResourceLoader> m_resourceLoader;
#endif
};

}

#endif
#endif
