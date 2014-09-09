/*
 * Copyright (C) 2011, 2013-2014 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef MediaPlayerPrivateAVFoundationCF_h
#define MediaPlayerPrivateAVFoundationCF_h

#if PLATFORM(WIN) && ENABLE(VIDEO) && USE(AVFOUNDATION)

#include "MediaPlayerPrivateAVFoundation.h"

#if HAVE(AVFOUNDATION_LOADER_DELEGATE) || HAVE(ENCRYPTED_MEDIA_V2)
typedef struct OpaqueAVCFAssetResourceLoadingRequest* AVCFAssetResourceLoadingRequestRef;
#endif

namespace WebCore {

class AVFWrapper;
class WebCoreAVCFResourceLoader;

class MediaPlayerPrivateAVFoundationCF : public MediaPlayerPrivateAVFoundation {
public:
    virtual ~MediaPlayerPrivateAVFoundationCF();

    virtual void tracksChanged() override;

#if HAVE(AVFOUNDATION_LOADER_DELEGATE)
    bool shouldWaitForLoadingOfResource(AVCFAssetResourceLoadingRequestRef);
    void didCancelLoadingRequest(AVCFAssetResourceLoadingRequestRef);
    void didStopLoadingRequest(AVCFAssetResourceLoadingRequestRef);
#endif

#if ENABLE(ENCRYPTED_MEDIA_V2)
    RetainPtr<AVCFAssetResourceLoadingRequestRef> takeRequestForKeyURI(const String&);
#endif

    static void registerMediaEngine(MediaEngineRegistrar);

private:
    MediaPlayerPrivateAVFoundationCF(MediaPlayer*);

    // Engine support
    static PassOwnPtr<MediaPlayerPrivateInterface> create(MediaPlayer*);
    static void getSupportedTypes(HashSet<String>& types);
    static MediaPlayer::SupportsType supportsType(const MediaEngineSupportParameters&);
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
    virtual void seekToTime(double time, double negativeTolerance, double positiveTolerance);
    virtual unsigned long long totalBytes() const;
    virtual std::unique_ptr<PlatformTimeRanges> platformBufferedTimeRanges() const;
    virtual double platformMinTimeSeekable() const;
    virtual double platformMaxTimeSeekable() const;
    virtual float platformDuration() const;
    virtual float platformMaxTimeLoaded() const;
    virtual void beginLoadingMetadata();
    virtual void sizeChanged();
    virtual bool requiresImmediateCompositing() const override;

    virtual bool hasAvailableVideoFrame() const;

    virtual void createContextVideoRenderer();
    virtual void destroyContextVideoRenderer();

    virtual void createVideoLayer();
    virtual void destroyVideoLayer();

    virtual bool hasContextRenderer() const;
    virtual bool hasLayerRenderer() const;

    virtual void updateVideoLayerGravity() override;

    virtual void contentsNeedsDisplay();

#if ENABLE(ENCRYPTED_MEDIA_V2)
    virtual std::unique_ptr<CDMSession> createSession(const String&) override;
#endif

    virtual String languageOfPrimaryAudioTrack() const override;

#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)
    void processMediaSelectionOptions();
#endif

    virtual void setCurrentTextTrack(InbandTextTrackPrivateAVF*) override;
    virtual InbandTextTrackPrivateAVF* currentTextTrack() const override;

#if !HAVE(AVFOUNDATION_LEGIBLE_OUTPUT_SUPPORT)
    void processLegacyClosedCaptionsTracks();
#endif

    friend class AVFWrapper;
    AVFWrapper* m_avfWrapper;

    mutable String m_languageOfPrimaryAudioTrack;

#if HAVE(AVFOUNDATION_LOADER_DELEGATE)
    friend class WebCoreAVCFResourceLoader;
    HashMap<RetainPtr<AVCFAssetResourceLoadingRequestRef>, RefPtr<WebCoreAVCFResourceLoader>> m_resourceLoaderMap;
#endif

    bool m_videoFrameHasDrawn;
};

}

#endif // PLATFORM(WIN) && ENABLE(VIDEO) && USE(AVFOUNDATION)
#endif // MediaPlayerPrivateAVFoundationCF_h
