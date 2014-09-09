/*
 * Copyright (C) 2007-2014 Apple Inc. All rights reserved.
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

#ifndef MediaPlayer_h
#define MediaPlayer_h

#if ENABLE(VIDEO)
#include "GraphicsTypes3D.h"

#include "AudioTrackPrivate.h"
#include "CDMSession.h"
#include "InbandTextTrackPrivate.h"
#include "IntRect.h"
#include "URL.h"
#include "LayoutRect.h"
#include "MediaSession.h"
#include "NativeImagePtr.h"
#include "PlatformLayer.h"
#include "Timer.h"
#include "VideoTrackPrivate.h"
#include <runtime/Uint8Array.h>
#include <wtf/Forward.h>
#include <wtf/HashSet.h>
#include <wtf/MediaTime.h>
#include <wtf/Noncopyable.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/StringHash.h>

#if ENABLE(AVF_CAPTIONS)
#include "PlatformTextTrack.h"
#endif

#if USE(PLATFORM_TEXT_TRACK_MENU)
#include "PlatformTextTrackMenu.h"
#endif

OBJC_CLASS AVAsset;
OBJC_CLASS AVPlayer;
OBJC_CLASS NSArray;
OBJC_CLASS QTMovie;

class AVCFPlayer;
class QTMovieGWorld;
class QTMovieVisualContext;

namespace WebCore {

class AudioSourceProvider;
class AuthenticationChallenge;
class Document;
#if ENABLE(MEDIA_SOURCE)
class MediaSourcePrivateClient;
#endif
class MediaPlayerPrivateInterface;
class TextTrackRepresentation;
struct Cookie;

// Structure that will hold every native
// types supported by the current media player.
// We have to do that has multiple media players
// backend can live at runtime.
struct PlatformMedia {
    enum {
        None,
        QTMovieType,
        QTMovieGWorldType,
        QTMovieVisualContextType,
        ChromiumMediaPlayerType,
        QtMediaPlayerType,
        AVFoundationMediaPlayerType,
        AVFoundationCFMediaPlayerType,
        AVFoundationAssetType,
    } type;

    union {
        QTMovie* qtMovie;
        QTMovieGWorld* qtMovieGWorld;
        QTMovieVisualContext* qtMovieVisualContext;
        MediaPlayerPrivateInterface* chromiumMediaPlayer;
        MediaPlayerPrivateInterface* qtMediaPlayer;
        AVPlayer* avfMediaPlayer;
        AVCFPlayer* avcfMediaPlayer;
        AVAsset* avfAsset;
    } media;
};

struct MediaEngineSupportParameters {
    String type;
    String codecs;
    URL url;
#if ENABLE(ENCRYPTED_MEDIA)
    String keySystem;
#endif
#if ENABLE(MEDIA_SOURCE)
    bool isMediaSource;
#endif

    MediaEngineSupportParameters()
#if ENABLE(MEDIA_SOURCE)
        : isMediaSource(false)
#endif
    {
    }
};

extern const PlatformMedia NoPlatformMedia;

class CachedResourceLoader;
class ContentType;
class FrameView;
class GraphicsContext;
class GraphicsContext3D;
class HostWindow;
class IntRect;
class IntSize;
class MediaPlayer;
struct MediaPlayerFactory;
class PlatformTimeRanges;

#if PLATFORM(WIN) && USE(AVFOUNDATION)
struct GraphicsDeviceAdapter;
#endif

class MediaPlayerClient {
public:
    enum CORSMode { Unspecified, Anonymous, UseCredentials };

    virtual ~MediaPlayerClient() { }

    // Get the document which the media player is owned by
    virtual Document* mediaPlayerOwningDocument() { return 0; }

    // the network state has changed
    virtual void mediaPlayerNetworkStateChanged(MediaPlayer*) { }

    // the ready state has changed
    virtual void mediaPlayerReadyStateChanged(MediaPlayer*) { }

    // the volume state has changed
    virtual void mediaPlayerVolumeChanged(MediaPlayer*) { }

    // the mute state has changed
    virtual void mediaPlayerMuteChanged(MediaPlayer*) { }

    // time has jumped, eg. not as a result of normal playback
    virtual void mediaPlayerTimeChanged(MediaPlayer*) { }

    // the media file duration has changed, or is now known
    virtual void mediaPlayerDurationChanged(MediaPlayer*) { }

    // the playback rate has changed
    virtual void mediaPlayerRateChanged(MediaPlayer*) { }

    // the play/pause status changed
    virtual void mediaPlayerPlaybackStateChanged(MediaPlayer*) { }

    // The MediaPlayer has found potentially problematic media content.
    // This is used internally to trigger swapping from a <video>
    // element to an <embed> in standalone documents
    virtual void mediaPlayerSawUnsupportedTracks(MediaPlayer*) { }

    // The MediaPlayer could not discover an engine which supports the requested resource.
    virtual void mediaPlayerResourceNotSupported(MediaPlayer*) { }

// Presentation-related methods
    // a new frame of video is available
    virtual void mediaPlayerRepaint(MediaPlayer*) { }

    // the movie size has changed
    virtual void mediaPlayerSizeChanged(MediaPlayer*) { }

    virtual void mediaPlayerEngineUpdated(MediaPlayer*) { }

    // The first frame of video is available to render. A media engine need only make this callback if the
    // first frame is not available immediately when prepareForRendering is called.
    virtual void mediaPlayerFirstVideoFrameAvailable(MediaPlayer*) { }

    // A characteristic of the media file, eg. video, audio, closed captions, etc, has changed.
    virtual void mediaPlayerCharacteristicChanged(MediaPlayer*) { }
    
    // whether the rendering system can accelerate the display of this MediaPlayer.
    virtual bool mediaPlayerRenderingCanBeAccelerated(MediaPlayer*) { return false; }

    // called when the media player's rendering mode changed, which indicates a change in the
    // availability of the platformLayer().
    virtual void mediaPlayerRenderingModeChanged(MediaPlayer*) { }

#if PLATFORM(WIN) && USE(AVFOUNDATION)
    virtual GraphicsDeviceAdapter* mediaPlayerGraphicsDeviceAdapter(const MediaPlayer*) const { return 0; }
#endif

#if ENABLE(ENCRYPTED_MEDIA)
    enum MediaKeyErrorCode { UnknownError = 1, ClientError, ServiceError, OutputError, HardwareChangeError, DomainError };
    virtual void mediaPlayerKeyAdded(MediaPlayer*, const String& /* keySystem */, const String& /* sessionId */) { }
    virtual void mediaPlayerKeyError(MediaPlayer*, const String& /* keySystem */, const String& /* sessionId */, MediaKeyErrorCode, unsigned short /* systemCode */) { }
    virtual void mediaPlayerKeyMessage(MediaPlayer*, const String& /* keySystem */, const String& /* sessionId */, const unsigned char* /* message */, unsigned /* messageLength */, const URL& /* defaultURL */) { }
    virtual bool mediaPlayerKeyNeeded(MediaPlayer*, const String& /* keySystem */, const String& /* sessionId */, const unsigned char* /* initData */, unsigned /* initDataLength */) { return false; }
#endif

#if ENABLE(ENCRYPTED_MEDIA_V2)
    virtual bool mediaPlayerKeyNeeded(MediaPlayer*, Uint8Array*) { return false; }
#endif
    
#if ENABLE(IOS_AIRPLAY)
    virtual void mediaPlayerCurrentPlaybackTargetIsWirelessChanged(MediaPlayer*) { };
    virtual void mediaPlayerPlaybackTargetAvailabilityChanged(MediaPlayer*) { };
#endif

    virtual String mediaPlayerReferrer() const { return String(); }
    virtual String mediaPlayerUserAgent() const { return String(); }
    virtual CORSMode mediaPlayerCORSMode() const { return Unspecified; }
    virtual void mediaPlayerEnterFullscreen() { }
    virtual void mediaPlayerExitFullscreen() { }
    virtual bool mediaPlayerIsFullscreen() const { return false; }
    virtual bool mediaPlayerIsFullscreenPermitted() const { return false; }
    virtual bool mediaPlayerIsVideo() const { return false; }
    virtual LayoutRect mediaPlayerContentBoxRect() const { return LayoutRect(); }
    virtual void mediaPlayerSetSize(const IntSize&) { }
    virtual void mediaPlayerPause() { }
    virtual void mediaPlayerPlay() { }
    virtual bool mediaPlayerPlatformVolumeConfigurationRequired() const { return false; }
    virtual bool mediaPlayerIsPaused() const { return true; }
    virtual bool mediaPlayerIsLooping() const { return false; }
    virtual HostWindow* mediaPlayerHostWindow() { return 0; }
    virtual IntRect mediaPlayerWindowClipRect() { return IntRect(); }
    virtual CachedResourceLoader* mediaPlayerCachedResourceLoader() { return 0; }
    virtual bool doesHaveAttribute(const AtomicString&, AtomicString* = 0) const { return false; }

#if ENABLE(VIDEO_TRACK)
    virtual void mediaPlayerDidAddAudioTrack(PassRefPtr<AudioTrackPrivate>) { }
    virtual void mediaPlayerDidAddTextTrack(PassRefPtr<InbandTextTrackPrivate>) { }
    virtual void mediaPlayerDidAddVideoTrack(PassRefPtr<VideoTrackPrivate>) { }
    virtual void mediaPlayerDidRemoveAudioTrack(PassRefPtr<AudioTrackPrivate>) { }
    virtual void mediaPlayerDidRemoveTextTrack(PassRefPtr<InbandTextTrackPrivate>) { }
    virtual void mediaPlayerDidRemoveVideoTrack(PassRefPtr<VideoTrackPrivate>) { }

    virtual void textTrackRepresentationBoundsChanged(const IntRect&) { }
#if ENABLE(AVF_CAPTIONS)
    virtual Vector<RefPtr<PlatformTextTrack>> outOfBandTrackSources() { return Vector<RefPtr<PlatformTextTrack>>(); }
#endif
#endif

#if PLATFORM(IOS)
    virtual String mediaPlayerNetworkInterfaceName() const { return String(); }
    virtual bool mediaPlayerGetRawCookies(const URL&, Vector<Cookie>&) const { return false; }
#endif
    
    virtual bool mediaPlayerShouldWaitForResponseToAuthenticationChallenge(const AuthenticationChallenge&) { return false; }
    virtual void mediaPlayerHandlePlaybackCommand(MediaSession::RemoteControlCommandType) { }

    virtual String mediaPlayerSourceApplicationIdentifier() const { return emptyString(); }
};

class MediaPlayerSupportsTypeClient {
public:
    virtual ~MediaPlayerSupportsTypeClient() { }

    virtual bool mediaPlayerNeedsSiteSpecificHacks() const { return false; }
    virtual String mediaPlayerDocumentHost() const { return String(); }
};

class MediaPlayer {
    WTF_MAKE_NONCOPYABLE(MediaPlayer); WTF_MAKE_FAST_ALLOCATED;
public:

    static PassOwnPtr<MediaPlayer> create(MediaPlayerClient* client)
    {
        return adoptPtr(new MediaPlayer(client));
    }
    virtual ~MediaPlayer();

    // Media engine support.
    enum SupportsType { IsNotSupported, IsSupported, MayBeSupported };
    static MediaPlayer::SupportsType supportsType(const MediaEngineSupportParameters&, const MediaPlayerSupportsTypeClient*);
    static void getSupportedTypes(HashSet<String>&);
    static bool isAvailable();
    static void getSitesInMediaCache(Vector<String>&);
    static void clearMediaCache();
    static void clearMediaCacheForSite(const String&);
    static bool supportsKeySystem(const String& keySystem, const String& mimeType);

    bool supportsFullscreen() const;
    bool supportsSave() const;
    bool supportsScanning() const;
    bool requiresImmediateCompositing() const;
    bool doesHaveAttribute(const AtomicString&, AtomicString* value = nullptr) const;
    PlatformMedia platformMedia() const;
    PlatformLayer* platformLayer() const;
#if PLATFORM(IOS)
    void setVideoFullscreenLayer(PlatformLayer*);
    void setVideoFullscreenFrame(FloatRect);
    enum VideoGravity { VideoGravityResize, VideoGravityResizeAspect, VideoGravityResizeAspectFill };
    void setVideoFullscreenGravity(VideoGravity);

    NSArray *timedMetadata() const;
    String accessLog() const;
    String errorLog() const;
#endif

    IntSize naturalSize();
    bool hasVideo() const;
    bool hasAudio() const;

    void setFrameView(FrameView* frameView) { m_frameView = frameView; }
    FrameView* frameView() { return m_frameView; }
    bool inMediaDocument();

    IntSize size() const { return m_size; }
    void setSize(const IntSize& size);

    bool load(const URL&, const ContentType&, const String& keySystem);
#if ENABLE(MEDIA_SOURCE)
    bool load(const URL&, const ContentType&, MediaSourcePrivateClient*);
#endif
    void cancelLoad();

    bool visible() const;
    void setVisible(bool);

    void prepareToPlay();
    void play();
    void pause();
    void setShouldBufferData(bool);

#if ENABLE(ENCRYPTED_MEDIA) || ENABLE(ENCRYPTED_MEDIA_V2)
    // Represents synchronous exceptions that can be thrown from the Encrypted Media methods.
    // This is different from the asynchronous MediaKeyError.
    enum MediaKeyException { NoError, InvalidPlayerState, KeySystemNotSupported };
#endif

#if ENABLE(ENCRYPTED_MEDIA)
    MediaKeyException generateKeyRequest(const String& keySystem, const unsigned char* initData, unsigned initDataLength);
    MediaKeyException addKey(const String& keySystem, const unsigned char* key, unsigned keyLength, const unsigned char* initData, unsigned initDataLength, const String& sessionId);
    MediaKeyException cancelKeyRequest(const String& keySystem, const String& sessionId);
#endif

#if ENABLE(ENCRYPTED_MEDIA_V2)
    std::unique_ptr<CDMSession> createSession(const String& keySystem);
    void setCDMSession(CDMSession*);
#endif

    bool paused() const;
    bool seeking() const;

    static double invalidTime() { return -1.0;}
    MediaTime duration() const;
    MediaTime currentTime() const;
    void seek(const MediaTime&);
    void seekWithTolerance(const MediaTime&, const MediaTime& negativeTolerance, const MediaTime& positiveTolerance);

    MediaTime startTime() const;
    MediaTime initialTime() const;

    double rate() const;
    void setRate(double);

    bool preservesPitch() const;
    void setPreservesPitch(bool);

    std::unique_ptr<PlatformTimeRanges> buffered();
    std::unique_ptr<PlatformTimeRanges> seekable();
    MediaTime minTimeSeekable();
    MediaTime maxTimeSeekable();

    bool didLoadingProgress();

    double volume() const;
    void setVolume(double);
    bool platformVolumeConfigurationRequired() const { return m_mediaPlayerClient->mediaPlayerPlatformVolumeConfigurationRequired(); }

    bool muted() const;
    void setMuted(bool);

    bool hasClosedCaptions() const;
    void setClosedCaptionsVisible(bool closedCaptionsVisible);

    bool autoplay() const;
    void setAutoplay(bool);

    void paint(GraphicsContext*, const IntRect&);
    void paintCurrentFrameInContext(GraphicsContext*, const IntRect&);

    // copyVideoTextureToPlatformTexture() is used to do the GPU-GPU textures copy without a readback to system memory.
    // The first five parameters denote the corresponding GraphicsContext, destination texture, requested level, requested type and the required internalFormat for destination texture.
    // The last two parameters premultiplyAlpha and flipY denote whether addtional premultiplyAlpha and flip operation are required during the copy.
    // It returns true on success and false on failure.

    // In the GPU-GPU textures copy, the source texture(Video texture) should have valid target, internalFormat and size, etc.
    // The destination texture may need to be resized to to the dimensions of the source texture or re-defined to the required internalFormat.
    // The current restrictions require that format shoud be RGB or RGBA, type should be UNSIGNED_BYTE and level should be 0. It may be lifted in the future.

    // Each platform port can have its own implementation on this function. The default implementation for it is a single "return false" in MediaPlayerPrivate.h.
    // In chromium, the implementation is based on GL_CHROMIUM_copy_texture extension which is documented at
    // http://src.chromium.org/viewvc/chrome/trunk/src/gpu/GLES2/extensions/CHROMIUM/CHROMIUM_copy_texture.txt and implemented at
    // http://src.chromium.org/viewvc/chrome/trunk/src/gpu/command_buffer/service/gles2_cmd_copy_texture_chromium.cc via shaders.
    bool copyVideoTextureToPlatformTexture(GraphicsContext3D*, Platform3DObject texture, GC3Dint level, GC3Denum type, GC3Denum internalFormat, bool premultiplyAlpha, bool flipY);

    PassNativeImagePtr nativeImageForCurrentTime();

    enum NetworkState { Empty, Idle, Loading, Loaded, FormatError, NetworkError, DecodeError };
    NetworkState networkState();

    enum ReadyState  { HaveNothing, HaveMetadata, HaveCurrentData, HaveFutureData, HaveEnoughData };
    ReadyState readyState();

    enum MovieLoadType { Unknown, Download, StoredStream, LiveStream };
    MovieLoadType movieLoadType() const;

    enum Preload { None, MetaData, Auto };
    Preload preload() const;
    void setPreload(Preload);

    void networkStateChanged();
    void readyStateChanged();
    void volumeChanged(double);
    void muteChanged(bool);
    void timeChanged();
    void sizeChanged();
    void rateChanged();
    void playbackStateChanged();
    void durationChanged();
    void firstVideoFrameAvailable();
    void characteristicChanged();

    void repaint();

    MediaPlayerClient* mediaPlayerClient() const { return m_mediaPlayerClient; }
    void clearMediaPlayerClient() { m_mediaPlayerClient = 0; }

    bool hasAvailableVideoFrame() const;
    void prepareForRendering();

    bool canLoadPoster() const;
    void setPoster(const String&);

#if USE(NATIVE_FULLSCREEN_VIDEO)
    void enterFullscreen();
    void exitFullscreen();
#endif

#if ENABLE(IOS_AIRPLAY)
    bool isCurrentPlaybackTargetWireless() const;

    enum WirelessPlaybackTargetType { TargetTypeNone, TargetTypeAirPlay, TargetTypeTVOut };
    WirelessPlaybackTargetType wirelessPlaybackTargetType() const;

    String wirelessPlaybackTargetName() const;

    void showPlaybackTargetPicker();

    bool hasWirelessPlaybackTargets() const;

    bool wirelessVideoPlaybackDisabled() const;
    void setWirelessVideoPlaybackDisabled(bool);

    void currentPlaybackTargetIsWirelessChanged();
    void playbackTargetAvailabilityChanged();

    void setHasPlaybackTargetAvailabilityListeners(bool);
#endif

    double minFastReverseRate() const;
    double maxFastForwardRate() const;

#if USE(NATIVE_FULLSCREEN_VIDEO)
    bool canEnterFullscreen() const;
#endif

    // whether accelerated rendering is supported by the media engine for the current media.
    bool supportsAcceleratedRendering() const;
    // called when the rendering system flips the into or out of accelerated rendering mode.
    void acceleratedRenderingStateChanged();

    bool shouldMaintainAspectRatio() const;
    void setShouldMaintainAspectRatio(bool);

#if PLATFORM(WIN) && USE(AVFOUNDATION)
    GraphicsDeviceAdapter* graphicsDeviceAdapter() const;
#endif

    bool hasSingleSecurityOrigin() const;

    bool didPassCORSAccessCheck() const;

    MediaTime mediaTimeForTimeValue(const MediaTime&) const;

    double maximumDurationToCacheMediaTime() const;

    unsigned decodedFrameCount() const;
    unsigned droppedFrameCount() const;
    unsigned audioDecodedByteCount() const;
    unsigned videoDecodedByteCount() const;

    void setPrivateBrowsingMode(bool);

#if ENABLE(WEB_AUDIO)
    AudioSourceProvider* audioSourceProvider();
#endif

#if ENABLE(ENCRYPTED_MEDIA)
    void keyAdded(const String& keySystem, const String& sessionId);
    void keyError(const String& keySystem, const String& sessionId, MediaPlayerClient::MediaKeyErrorCode, unsigned short systemCode);
    void keyMessage(const String& keySystem, const String& sessionId, const unsigned char* message, unsigned messageLength, const URL& defaultURL);
    bool keyNeeded(const String& keySystem, const String& sessionId, const unsigned char* initData, unsigned initDataLength);
#endif

#if ENABLE(ENCRYPTED_MEDIA_V2)
    bool keyNeeded(Uint8Array* initData);
#endif

    String referrer() const;
    String userAgent() const;

    String engineDescription() const;

    CachedResourceLoader* cachedResourceLoader();

#if ENABLE(VIDEO_TRACK)
    void addAudioTrack(PassRefPtr<AudioTrackPrivate>);
    void addTextTrack(PassRefPtr<InbandTextTrackPrivate>);
    void addVideoTrack(PassRefPtr<VideoTrackPrivate>);
    void removeAudioTrack(PassRefPtr<AudioTrackPrivate>);
    void removeTextTrack(PassRefPtr<InbandTextTrackPrivate>);
    void removeVideoTrack(PassRefPtr<VideoTrackPrivate>);

    bool requiresTextTrackRepresentation() const;
    void setTextTrackRepresentation(TextTrackRepresentation*);
    void syncTextTrackBounds();
#if ENABLE(AVF_CAPTIONS)
    void notifyTrackModeChanged();
    Vector<RefPtr<PlatformTextTrack>> outOfBandTrackSources();
#endif
#endif

#if PLATFORM(IOS)
    String mediaPlayerNetworkInterfaceName() const;
    bool getRawCookies(const URL&, Vector<Cookie>&) const;
#endif

    static void resetMediaEngines();

#if USE(PLATFORM_TEXT_TRACK_MENU)
    bool implementsTextTrackControls() const;
    PassRefPtr<PlatformTextTrackMenuInterface> textTrackMenu();
#endif

#if USE(GSTREAMER)
    void simulateAudioInterruption();
#endif

    String languageOfPrimaryAudioTrack() const;

    size_t extraMemoryCost() const;

    unsigned long long fileSize() const;

#if ENABLE(MEDIA_SOURCE)
    unsigned long totalVideoFrames();
    unsigned long droppedVideoFrames();
    unsigned long corruptedVideoFrames();
    MediaTime totalFrameDelay();
#endif

    bool shouldWaitForResponseToAuthenticationChallenge(const AuthenticationChallenge&);
    void handlePlaybackCommand(MediaSession::RemoteControlCommandType);
    String sourceApplicationIdentifier() const;

private:
    MediaPlayer(MediaPlayerClient*);
    MediaPlayerFactory* nextBestMediaEngine(MediaPlayerFactory*) const;
    void loadWithNextMediaEngine(MediaPlayerFactory*);
    void reloadTimerFired(Timer<MediaPlayer>&);

    static void initializeMediaEngines();

    MediaPlayerClient* m_mediaPlayerClient;
    Timer<MediaPlayer> m_reloadTimer;
    OwnPtr<MediaPlayerPrivateInterface> m_private;
    MediaPlayerFactory* m_currentMediaEngine;
    URL m_url;
    String m_contentMIMEType;
    String m_contentTypeCodecs;
    String m_keySystem;
    FrameView* m_frameView;
    IntSize m_size;
    Preload m_preload;
    bool m_visible;
    double m_rate;
    double m_volume;
    bool m_muted;
    bool m_preservesPitch;
    bool m_privateBrowsing;
    bool m_shouldPrepareToRender;
    bool m_contentMIMETypeWasInferredFromExtension;

#if ENABLE(MEDIA_SOURCE)
    RefPtr<MediaSourcePrivateClient> m_mediaSource;
#endif
};

typedef PassOwnPtr<MediaPlayerPrivateInterface> (*CreateMediaEnginePlayer)(MediaPlayer*);
typedef void (*MediaEngineSupportedTypes)(HashSet<String>& types);
typedef MediaPlayer::SupportsType (*MediaEngineSupportsType)(const MediaEngineSupportParameters& parameters);
typedef void (*MediaEngineGetSitesInMediaCache)(Vector<String>&);
typedef void (*MediaEngineClearMediaCache)();
typedef void (*MediaEngineClearMediaCacheForSite)(const String&);
typedef bool (*MediaEngineSupportsKeySystem)(const String& keySystem, const String& mimeType);

typedef void (*MediaEngineRegistrar)(CreateMediaEnginePlayer, MediaEngineSupportedTypes, MediaEngineSupportsType,
    MediaEngineGetSitesInMediaCache, MediaEngineClearMediaCache, MediaEngineClearMediaCacheForSite, MediaEngineSupportsKeySystem);
typedef void (*MediaEngineRegister)(MediaEngineRegistrar);

class MediaPlayerFactorySupport {
public:
    WEBCORE_EXPORT static void callRegisterMediaEngine(MediaEngineRegister);
};

}

#endif // ENABLE(VIDEO)

#endif
