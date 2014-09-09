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

#include "config.h"

#if ENABLE(VIDEO)
#include "MediaPlayer.h"

#include "ContentType.h"
#include "Document.h"
#include "Frame.h"
#include "FrameView.h"
#include "IntRect.h"
#include "Logging.h"
#include "MIMETypeRegistry.h"
#include "MediaPlayerPrivate.h"
#include "PlatformTimeRanges.h"
#include "Settings.h"
#include <wtf/text/CString.h>

#if ENABLE(VIDEO_TRACK)
#include "InbandTextTrackPrivate.h"
#endif

#if ENABLE(MEDIA_SOURCE)
#include "MediaSourcePrivateClient.h"
#endif

#if USE(GSTREAMER)
#include "MediaPlayerPrivateGStreamer.h"
#define PlatformMediaEngineClassName MediaPlayerPrivateGStreamer
#endif

#if USE(MEDIA_FOUNDATION)
#include "MediaPlayerPrivateMediaFoundation.h"
#define PlatformMediaEngineClassName MediaPlayerPrivateMediaFoundation
#endif

#if PLATFORM(COCOA)
#include "MediaPlayerPrivateQTKit.h"
#if USE(AVFOUNDATION)
#include "MediaPlayerPrivateAVFoundationObjC.h"
#if ENABLE(MEDIA_SOURCE)
#include "MediaPlayerPrivateMediaSourceAVFObjC.h"
#endif
#endif
#elif OS(WINCE)
#include "MediaPlayerPrivateWinCE.h"
#define PlatformMediaEngineClassName MediaPlayerPrivate
#elif PLATFORM(WIN) && !USE(GSTREAMER)
#if USE(AVFOUNDATION)
#include "MediaPlayerPrivateAVFoundationCF.h"
#endif // USE(AVFOUNDATION)
#endif

namespace WebCore {

const PlatformMedia NoPlatformMedia = { PlatformMedia::None, {0} };

// a null player to make MediaPlayer logic simpler

class NullMediaPlayerPrivate : public MediaPlayerPrivateInterface {
public:
    NullMediaPlayerPrivate(MediaPlayer*) { }

    virtual void load(const String&) { }
#if ENABLE(MEDIA_SOURCE)
    virtual void load(const String&, MediaSourcePrivateClient*) { }
#endif
    virtual void cancelLoad() { }

    virtual void prepareToPlay() { }
    virtual void play() { }
    virtual void pause() { }    

    virtual PlatformMedia platformMedia() const { return NoPlatformMedia; }
    virtual PlatformLayer* platformLayer() const { return 0; }

    virtual IntSize naturalSize() const { return IntSize(0, 0); }

    virtual bool hasVideo() const { return false; }
    virtual bool hasAudio() const { return false; }

    virtual void setVisible(bool) { }

    virtual double durationDouble() const { return 0; }

    virtual double currentTimeDouble() const { return 0; }
    virtual void seekDouble(double) { }
    virtual bool seeking() const { return false; }

    virtual void setRateDouble(double) { }
    virtual void setPreservesPitch(bool) { }
    virtual bool paused() const { return false; }

    virtual void setVolumeDouble(double) { }

    virtual bool supportsMuting() const { return false; }
    virtual void setMuted(bool) { }

    virtual bool hasClosedCaptions() const { return false; }
    virtual void setClosedCaptionsVisible(bool) { };

    virtual MediaPlayer::NetworkState networkState() const { return MediaPlayer::Empty; }
    virtual MediaPlayer::ReadyState readyState() const { return MediaPlayer::HaveNothing; }

    virtual double maxTimeSeekableDouble() const { return 0; }
    virtual double minTimeSeekable() const { return 0; }
    virtual std::unique_ptr<PlatformTimeRanges> buffered() const { return PlatformTimeRanges::create(); }

    virtual unsigned totalBytes() const { return 0; }
    virtual bool didLoadingProgress() const { return false; }

    virtual void setSize(const IntSize&) { }

    virtual void paint(GraphicsContext*, const IntRect&) { }

    virtual bool canLoadPoster() const { return false; }
    virtual void setPoster(const String&) { }

    virtual bool hasSingleSecurityOrigin() const { return true; }

#if ENABLE(ENCRYPTED_MEDIA)
    virtual MediaPlayer::MediaKeyException generateKeyRequest(const String&, const unsigned char*, unsigned) override { return MediaPlayer::InvalidPlayerState; }
    virtual MediaPlayer::MediaKeyException addKey(const String&, const unsigned char*, unsigned, const unsigned char*, unsigned, const String&) override { return MediaPlayer::InvalidPlayerState; }
    virtual MediaPlayer::MediaKeyException cancelKeyRequest(const String&, const String&) override { return MediaPlayer::InvalidPlayerState; }
#endif
};

static PassOwnPtr<MediaPlayerPrivateInterface> createNullMediaPlayer(MediaPlayer* player) 
{ 
    return adoptPtr(new NullMediaPlayerPrivate(player)); 
}


// engine support

struct MediaPlayerFactory {
    WTF_MAKE_NONCOPYABLE(MediaPlayerFactory); WTF_MAKE_FAST_ALLOCATED;
public:
    MediaPlayerFactory(CreateMediaEnginePlayer constructor, MediaEngineSupportedTypes getSupportedTypes, MediaEngineSupportsType supportsTypeAndCodecs,
        MediaEngineGetSitesInMediaCache getSitesInMediaCache, MediaEngineClearMediaCache clearMediaCache, MediaEngineClearMediaCacheForSite clearMediaCacheForSite, MediaEngineSupportsKeySystem supportsKeySystem)
        : constructor(constructor)
        , getSupportedTypes(getSupportedTypes)
        , supportsTypeAndCodecs(supportsTypeAndCodecs)
        , getSitesInMediaCache(getSitesInMediaCache)
        , clearMediaCache(clearMediaCache)
        , clearMediaCacheForSite(clearMediaCacheForSite)
        , supportsKeySystem(supportsKeySystem)
    {
    }

    CreateMediaEnginePlayer constructor;
    MediaEngineSupportedTypes getSupportedTypes;
    MediaEngineSupportsType supportsTypeAndCodecs;
    MediaEngineGetSitesInMediaCache getSitesInMediaCache;
    MediaEngineClearMediaCache clearMediaCache;
    MediaEngineClearMediaCacheForSite clearMediaCacheForSite;
    MediaEngineSupportsKeySystem supportsKeySystem;
};

static void addMediaEngine(CreateMediaEnginePlayer, MediaEngineSupportedTypes, MediaEngineSupportsType, MediaEngineGetSitesInMediaCache, MediaEngineClearMediaCache, MediaEngineClearMediaCacheForSite, MediaEngineSupportsKeySystem);

static MediaPlayerFactory* bestMediaEngineForSupportParameters(const MediaEngineSupportParameters&, MediaPlayerFactory* current = 0);
static MediaPlayerFactory* nextMediaEngine(MediaPlayerFactory* current);

enum RequeryEngineOptions { DoNotResetEngines, ResetEngines };
static Vector<MediaPlayerFactory*>& installedMediaEngines(RequeryEngineOptions requeryFlags = DoNotResetEngines )
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Vector<MediaPlayerFactory*>, installedEngines, ());
    static bool enginesQueried = false;

    if (requeryFlags == ResetEngines) {
        installedEngines.clear();
        enginesQueried = false;
        return installedEngines;
    }

    if (enginesQueried)
        return installedEngines;

    enginesQueried = true;

#if USE(AVFOUNDATION)
    if (Settings::isAVFoundationEnabled()) {
#if PLATFORM(COCOA)
        MediaPlayerPrivateAVFoundationObjC::registerMediaEngine(addMediaEngine);
#if ENABLE(MEDIA_SOURCE)
        MediaPlayerPrivateMediaSourceAVFObjC::registerMediaEngine(addMediaEngine);
#endif
#elif PLATFORM(WIN)
        MediaPlayerPrivateAVFoundationCF::registerMediaEngine(addMediaEngine);
#endif
    }
#endif

#if PLATFORM(MAC)
    if (Settings::isQTKitEnabled())
        MediaPlayerPrivateQTKit::registerMediaEngine(addMediaEngine);
#endif

#if defined(PlatformMediaEngineClassName)
    PlatformMediaEngineClassName::registerMediaEngine(addMediaEngine);
#endif

    return installedEngines;
}

static void addMediaEngine(CreateMediaEnginePlayer constructor, MediaEngineSupportedTypes getSupportedTypes, MediaEngineSupportsType supportsType,
    MediaEngineGetSitesInMediaCache getSitesInMediaCache, MediaEngineClearMediaCache clearMediaCache, MediaEngineClearMediaCacheForSite clearMediaCacheForSite, MediaEngineSupportsKeySystem supportsKeySystem)
{
    ASSERT(constructor);
    ASSERT(getSupportedTypes);
    ASSERT(supportsType);

    installedMediaEngines().append(new MediaPlayerFactory(constructor, getSupportedTypes, supportsType, getSitesInMediaCache, clearMediaCache, clearMediaCacheForSite, supportsKeySystem));
}

static const AtomicString& applicationOctetStream()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(const AtomicString, applicationOctetStream, ("application/octet-stream", AtomicString::ConstructFromLiteral));
    return applicationOctetStream;
}

static const AtomicString& textPlain()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(const AtomicString, textPlain, ("text/plain", AtomicString::ConstructFromLiteral));
    return textPlain;
}

static const AtomicString& codecs()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(const AtomicString, codecs, ("codecs", AtomicString::ConstructFromLiteral));
    return codecs;
}

static MediaPlayerFactory* bestMediaEngineForSupportParameters(const MediaEngineSupportParameters& parameters, MediaPlayerFactory* current)
{
    if (parameters.type.isEmpty())
        return 0;

    Vector<MediaPlayerFactory*>& engines = installedMediaEngines();
    if (engines.isEmpty())
        return 0;

    // 4.8.10.3 MIME types - In the absence of a specification to the contrary, the MIME type "application/octet-stream" 
    // when used with parameters, e.g. "application/octet-stream;codecs=theora", is a type that the user agent knows 
    // it cannot render.
    if (parameters.type == applicationOctetStream()) {
        if (!parameters.codecs.isEmpty())
            return 0;
    }

    MediaPlayerFactory* engine = 0;
    MediaPlayer::SupportsType supported = MediaPlayer::IsNotSupported;
    unsigned count = engines.size();
    for (unsigned ndx = 0; ndx < count; ndx++) {
        if (current) {
            if (current == engines[ndx])
                current = 0;
            continue;
        }
        MediaPlayer::SupportsType engineSupport = engines[ndx]->supportsTypeAndCodecs(parameters);
        if (engineSupport > supported) {
            supported = engineSupport;
            engine = engines[ndx];
        }
    }

    return engine;
}

static MediaPlayerFactory* nextMediaEngine(MediaPlayerFactory* current)
{
    Vector<MediaPlayerFactory*>& engines = installedMediaEngines();
    if (engines.isEmpty())
        return 0;

    if (!current) 
        return engines.first();

    size_t currentIndex = engines.find(current);
    if (currentIndex == WTF::notFound || currentIndex + 1 >= engines.size()) 
        return 0;

    return engines[currentIndex + 1];
}

// media player

MediaPlayer::MediaPlayer(MediaPlayerClient* client)
    : m_mediaPlayerClient(client)
    , m_reloadTimer(this, &MediaPlayer::reloadTimerFired)
    , m_private(createNullMediaPlayer(this))
    , m_currentMediaEngine(0)
    , m_frameView(0)
    , m_preload(Auto)
    , m_visible(false)
    , m_rate(1.0f)
    , m_volume(1.0f)
    , m_muted(false)
    , m_preservesPitch(true)
    , m_privateBrowsing(false)
    , m_shouldPrepareToRender(false)
    , m_contentMIMETypeWasInferredFromExtension(false)
{
}

MediaPlayer::~MediaPlayer()
{
    m_mediaPlayerClient = 0;
}

bool MediaPlayer::load(const URL& url, const ContentType& contentType, const String& keySystem)
{
    m_contentMIMEType = contentType.type().lower();
    m_contentTypeCodecs = contentType.parameter(codecs());
    m_url = url;
    m_keySystem = keySystem.lower();
    m_contentMIMETypeWasInferredFromExtension = false;

#if ENABLE(MEDIA_SOURCE)
    m_mediaSource = 0;
#endif

    // If the MIME type is missing or is not meaningful, try to figure it out from the URL.
    if (m_contentMIMEType.isEmpty() || m_contentMIMEType == applicationOctetStream() || m_contentMIMEType == textPlain()) {
        if (m_url.protocolIsData())
            m_contentMIMEType = mimeTypeFromDataURL(m_url.string());
        else {
            String lastPathComponent = url.lastPathComponent();
            size_t pos = lastPathComponent.reverseFind('.');
            if (pos != notFound) {
                String extension = lastPathComponent.substring(pos + 1);
                String mediaType = MIMETypeRegistry::getMediaMIMETypeForExtension(extension);
                if (!mediaType.isEmpty()) {
                    m_contentMIMEType = mediaType;
                    m_contentMIMETypeWasInferredFromExtension = true;
                }
            }
        }
    }

    loadWithNextMediaEngine(0);
    return m_currentMediaEngine;
}

#if ENABLE(MEDIA_SOURCE)
bool MediaPlayer::load(const URL& url, const ContentType& contentType, MediaSourcePrivateClient* mediaSource)
{
    ASSERT(mediaSource);
    m_mediaSource = mediaSource;
    m_contentMIMEType = contentType.type().lower();
    m_contentTypeCodecs = contentType.parameter(codecs());
    m_url = url;
    m_keySystem = "";
    m_contentMIMETypeWasInferredFromExtension = false;
    loadWithNextMediaEngine(0);
    return m_currentMediaEngine;
}
#endif

MediaPlayerFactory* MediaPlayer::nextBestMediaEngine(MediaPlayerFactory* current) const
{
    MediaEngineSupportParameters parameters;
    parameters.type = m_contentMIMEType;
    parameters.codecs = m_contentTypeCodecs;
    parameters.url = m_url;
#if ENABLE(ENCRYPTED_MEDIA)
    parameters.keySystem = m_keySystem;
#endif
#if ENABLE(MEDIA_SOURCE)
    parameters.isMediaSource = !!m_mediaSource;
#endif

    return bestMediaEngineForSupportParameters(parameters, current);
}

void MediaPlayer::loadWithNextMediaEngine(MediaPlayerFactory* current)
{
    MediaPlayerFactory* engine = 0;

    if (!m_contentMIMEType.isEmpty())
        engine = nextBestMediaEngine(current);

    // If no MIME type is specified or the type was inferred from the file extension, just use the next engine.
    if (!engine && (m_contentMIMEType.isEmpty() || m_contentMIMETypeWasInferredFromExtension))
        engine = nextMediaEngine(current);

    // Don't delete and recreate the player unless it comes from a different engine.
    if (!engine) {
        LOG(Media, "MediaPlayer::loadWithNextMediaEngine - no media engine found for type \"%s\"", m_contentMIMEType.utf8().data());
        m_currentMediaEngine = engine;
        m_private = nullptr;
    } else if (m_currentMediaEngine != engine) {
        m_currentMediaEngine = engine;
        m_private = engine->constructor(this);
        if (m_mediaPlayerClient)
            m_mediaPlayerClient->mediaPlayerEngineUpdated(this);
        m_private->setPrivateBrowsingMode(m_privateBrowsing);
        m_private->setPreload(m_preload);
        m_private->setPreservesPitch(preservesPitch());
        if (m_shouldPrepareToRender)
            m_private->prepareForRendering();
    }

    if (m_private) {
#if ENABLE(MEDIA_SOURCE)
        if (m_mediaSource)
            m_private->load(m_url.string(), m_mediaSource.get());
        else
#endif
        m_private->load(m_url.string());
    } else {
        m_private = createNullMediaPlayer(this);
        if (m_mediaPlayerClient) {
            m_mediaPlayerClient->mediaPlayerEngineUpdated(this);
            m_mediaPlayerClient->mediaPlayerResourceNotSupported(this);
        }
    }
}

bool MediaPlayer::hasAvailableVideoFrame() const
{
    return m_private->hasAvailableVideoFrame();
}

void MediaPlayer::prepareForRendering()
{
    m_shouldPrepareToRender = true;
    m_private->prepareForRendering();
}

bool MediaPlayer::canLoadPoster() const
{
    return m_private->canLoadPoster();
}

void MediaPlayer::setPoster(const String& url)
{
    m_private->setPoster(url);
}    

void MediaPlayer::cancelLoad()
{
    m_private->cancelLoad();
}    

void MediaPlayer::prepareToPlay()
{
    m_private->prepareToPlay();
}

void MediaPlayer::play()
{
    m_private->play();
}

void MediaPlayer::pause()
{
    m_private->pause();
}

void MediaPlayer::setShouldBufferData(bool shouldBuffer)
{
    m_private->setShouldBufferData(shouldBuffer);
}

#if ENABLE(ENCRYPTED_MEDIA)
MediaPlayer::MediaKeyException MediaPlayer::generateKeyRequest(const String& keySystem, const unsigned char* initData, unsigned initDataLength)
{
    return m_private->generateKeyRequest(keySystem.lower(), initData, initDataLength);
}

MediaPlayer::MediaKeyException MediaPlayer::addKey(const String& keySystem, const unsigned char* key, unsigned keyLength, const unsigned char* initData, unsigned initDataLength, const String& sessionId)
{
    return m_private->addKey(keySystem.lower(), key, keyLength, initData, initDataLength, sessionId);
}

MediaPlayer::MediaKeyException MediaPlayer::cancelKeyRequest(const String& keySystem, const String& sessionId)
{
    return m_private->cancelKeyRequest(keySystem.lower(), sessionId);
}
#endif

#if ENABLE(ENCRYPTED_MEDIA_V2)
std::unique_ptr<CDMSession> MediaPlayer::createSession(const String& keySystem)
{
    return m_private->createSession(keySystem);
}

void MediaPlayer::setCDMSession(CDMSession* session)
{
    m_private->setCDMSession(session);
}
#endif
    
MediaTime MediaPlayer::duration() const
{
    return m_private->durationMediaTime();
}

MediaTime MediaPlayer::startTime() const
{
    return m_private->startTime();
}

MediaTime MediaPlayer::initialTime() const
{
    return m_private->initialTime();
}

MediaTime MediaPlayer::currentTime() const
{
    return m_private->currentMediaTime();
}

void MediaPlayer::seekWithTolerance(const MediaTime& time, const MediaTime& negativeTolerance, const MediaTime& positiveTolerance)
{
    m_private->seekWithTolerance(time, negativeTolerance, positiveTolerance);
}

void MediaPlayer::seek(const MediaTime& time)
{
    m_private->seek(time);
}

bool MediaPlayer::paused() const
{
    return m_private->paused();
}

bool MediaPlayer::seeking() const
{
    return m_private->seeking();
}

bool MediaPlayer::supportsFullscreen() const
{
    return m_private->supportsFullscreen();
}

bool MediaPlayer::supportsSave() const
{
    return m_private->supportsSave();
}

bool MediaPlayer::supportsScanning() const
{
    return m_private->supportsScanning();
}

bool MediaPlayer::requiresImmediateCompositing() const
{
    return m_private->requiresImmediateCompositing();
}

IntSize MediaPlayer::naturalSize()
{
    return m_private->naturalSize();
}

bool MediaPlayer::hasVideo() const
{
    return m_private->hasVideo();
}

bool MediaPlayer::hasAudio() const
{
    return m_private->hasAudio();
}

bool MediaPlayer::inMediaDocument()
{
    if (!m_frameView)
        return false;
    Document* document = m_frameView->frame().document();
    return document && document->isMediaDocument();
}

PlatformMedia MediaPlayer::platformMedia() const
{
    return m_private->platformMedia();
}

PlatformLayer* MediaPlayer::platformLayer() const
{
    return m_private->platformLayer();
}
    
#if PLATFORM(IOS)
void MediaPlayer::setVideoFullscreenLayer(PlatformLayer* layer)
{
    m_private->setVideoFullscreenLayer(layer);
}

void MediaPlayer::setVideoFullscreenFrame(FloatRect frame)
{
    m_private->setVideoFullscreenFrame(frame);
}

void MediaPlayer::setVideoFullscreenGravity(MediaPlayer::VideoGravity gravity)
{
    m_private->setVideoFullscreenGravity(gravity);
}

NSArray* MediaPlayer::timedMetadata() const
{
    return m_private->timedMetadata();
}

String MediaPlayer::accessLog() const
{
    return m_private->accessLog();
}

String MediaPlayer::errorLog() const
{
    return m_private->errorLog();
}
#endif

MediaPlayer::NetworkState MediaPlayer::networkState()
{
    return m_private->networkState();
}

MediaPlayer::ReadyState MediaPlayer::readyState()
{
    return m_private->readyState();
}

double MediaPlayer::volume() const
{
    return m_volume;
}

void MediaPlayer::setVolume(double volume)
{
    m_volume = volume;

    if (m_private->supportsMuting() || !m_muted)
        m_private->setVolumeDouble(volume);
}

bool MediaPlayer::muted() const
{
    return m_muted;
}

void MediaPlayer::setMuted(bool muted)
{
    m_muted = muted;

    if (m_private->supportsMuting())
        m_private->setMuted(muted);
    else
        m_private->setVolume(muted ? 0 : m_volume);
}

bool MediaPlayer::hasClosedCaptions() const
{
    return m_private->hasClosedCaptions();
}

void MediaPlayer::setClosedCaptionsVisible(bool closedCaptionsVisible)
{
    m_private->setClosedCaptionsVisible(closedCaptionsVisible);
}

double MediaPlayer::rate() const
{
    return m_rate;
}

void MediaPlayer::setRate(double rate)
{
    m_rate = rate;
    m_private->setRateDouble(rate);
}

bool MediaPlayer::preservesPitch() const
{
    return m_preservesPitch;
}

void MediaPlayer::setPreservesPitch(bool preservesPitch)
{
    m_preservesPitch = preservesPitch;
    m_private->setPreservesPitch(preservesPitch);
}

std::unique_ptr<PlatformTimeRanges> MediaPlayer::buffered()
{
    return m_private->buffered();
}

std::unique_ptr<PlatformTimeRanges> MediaPlayer::seekable()
{
    return m_private->seekable();
}

MediaTime MediaPlayer::maxTimeSeekable()
{
    return m_private->maxMediaTimeSeekable();
}

MediaTime MediaPlayer::minTimeSeekable()
{
    return m_private->minMediaTimeSeekable();
}

bool MediaPlayer::didLoadingProgress()
{
    return m_private->didLoadingProgress();
}

void MediaPlayer::setSize(const IntSize& size)
{ 
    m_size = size;
    m_private->setSize(size);
}

bool MediaPlayer::visible() const
{
    return m_visible;
}

void MediaPlayer::setVisible(bool b)
{
    m_visible = b;
    m_private->setVisible(b);
}

MediaPlayer::Preload MediaPlayer::preload() const
{
    return m_preload;
}

void MediaPlayer::setPreload(MediaPlayer::Preload preload)
{
    m_preload = preload;
    m_private->setPreload(preload);
}

void MediaPlayer::paint(GraphicsContext* p, const IntRect& r)
{
    m_private->paint(p, r);
}

void MediaPlayer::paintCurrentFrameInContext(GraphicsContext* p, const IntRect& r)
{
    m_private->paintCurrentFrameInContext(p, r);
}

bool MediaPlayer::copyVideoTextureToPlatformTexture(GraphicsContext3D* context, Platform3DObject texture, GC3Dint level, GC3Denum type, GC3Denum internalFormat, bool premultiplyAlpha, bool flipY)
{
    return m_private->copyVideoTextureToPlatformTexture(context, texture, level, type, internalFormat, premultiplyAlpha, flipY);
}

PassNativeImagePtr MediaPlayer::nativeImageForCurrentTime()
{
    return m_private->nativeImageForCurrentTime();
}

MediaPlayer::SupportsType MediaPlayer::supportsType(const MediaEngineSupportParameters& parameters, const MediaPlayerSupportsTypeClient* client)
{
    // 4.8.10.3 MIME types - The canPlayType(type) method must return the empty string if type is a type that the 
    // user agent knows it cannot render or is the type "application/octet-stream"
    if (parameters.type == applicationOctetStream())
        return IsNotSupported;

    MediaPlayerFactory* engine = bestMediaEngineForSupportParameters(parameters);
    if (!engine)
        return IsNotSupported;

#if PLATFORM(COCOA)
    // YouTube will ask if the HTMLMediaElement canPlayType video/webm, then
    // video/x-flv, then finally video/mp4, and will then load a URL of the first type
    // in that list which returns "probably". When Perian is installed,
    // MediaPlayerPrivateQTKit claims to support both video/webm and video/x-flv, but
    // due to a bug in Perian, loading media in these formats will sometimes fail on
    // slow connections. <https://bugs.webkit.org/show_bug.cgi?id=86409>
    if (client && client->mediaPlayerNeedsSiteSpecificHacks()) {
        String host = client->mediaPlayerDocumentHost();
        if ((host.endsWith(".youtube.com", false) || equalIgnoringCase("youtube.com", host))
            && (parameters.type.startsWith("video/webm", false) || parameters.type.startsWith("video/x-flv", false)))
            return IsNotSupported;
    }
#else
    UNUSED_PARAM(client);
#endif

    return engine->supportsTypeAndCodecs(parameters);
}

void MediaPlayer::getSupportedTypes(HashSet<String>& types)
{
    Vector<MediaPlayerFactory*>& engines = installedMediaEngines();
    if (engines.isEmpty())
        return;

    unsigned count = engines.size();
    for (unsigned ndx = 0; ndx < count; ndx++)
        engines[ndx]->getSupportedTypes(types);
} 

bool MediaPlayer::isAvailable()
{
    return !installedMediaEngines().isEmpty();
} 

#if USE(NATIVE_FULLSCREEN_VIDEO)
void MediaPlayer::enterFullscreen()
{
    m_private->enterFullscreen();
}

void MediaPlayer::exitFullscreen()
{
    m_private->exitFullscreen();
}
#endif

#if ENABLE(IOS_AIRPLAY)
bool MediaPlayer::isCurrentPlaybackTargetWireless() const
{
    return m_private->isCurrentPlaybackTargetWireless();
}

String MediaPlayer::wirelessPlaybackTargetName() const
{
    return m_private->wirelessPlaybackTargetName();
}

MediaPlayer::WirelessPlaybackTargetType MediaPlayer::wirelessPlaybackTargetType() const
{
    return m_private->wirelessPlaybackTargetType();
}

void MediaPlayer::showPlaybackTargetPicker()
{
    m_private->showPlaybackTargetPicker();
}

bool MediaPlayer::hasWirelessPlaybackTargets() const
{
    return m_private->hasWirelessPlaybackTargets();
}

bool MediaPlayer::wirelessVideoPlaybackDisabled() const
{
    return m_private->wirelessVideoPlaybackDisabled();
}

void MediaPlayer::setWirelessVideoPlaybackDisabled(bool disabled)
{
    m_private->setWirelessVideoPlaybackDisabled(disabled);
}

void MediaPlayer::setHasPlaybackTargetAvailabilityListeners(bool hasListeners)
{
    m_private->setHasPlaybackTargetAvailabilityListeners(hasListeners);
}

void MediaPlayer::currentPlaybackTargetIsWirelessChanged()
{
    if (m_mediaPlayerClient)
        m_mediaPlayerClient->mediaPlayerCurrentPlaybackTargetIsWirelessChanged(this);
}

void MediaPlayer::playbackTargetAvailabilityChanged()
{
    if (m_mediaPlayerClient)
        m_mediaPlayerClient->mediaPlayerPlaybackTargetAvailabilityChanged(this);
}
#endif

double MediaPlayer::maxFastForwardRate() const
{
    return m_private->maxFastForwardRate();
}

double MediaPlayer::minFastReverseRate() const
{
    return m_private->minFastReverseRate();
}

#if USE(NATIVE_FULLSCREEN_VIDEO)
bool MediaPlayer::canEnterFullscreen() const
{
    return m_private->canEnterFullscreen();
}
#endif

void MediaPlayer::acceleratedRenderingStateChanged()
{
    m_private->acceleratedRenderingStateChanged();
}

bool MediaPlayer::supportsAcceleratedRendering() const
{
    return m_private->supportsAcceleratedRendering();
}

bool MediaPlayer::shouldMaintainAspectRatio() const
{
    return m_private->shouldMaintainAspectRatio();
}

void MediaPlayer::setShouldMaintainAspectRatio(bool maintainAspectRatio)
{
    m_private->setShouldMaintainAspectRatio(maintainAspectRatio);
}

bool MediaPlayer::hasSingleSecurityOrigin() const
{
    return m_private->hasSingleSecurityOrigin();
}

bool MediaPlayer::didPassCORSAccessCheck() const
{
    return m_private->didPassCORSAccessCheck();
}

MediaPlayer::MovieLoadType MediaPlayer::movieLoadType() const
{
    return m_private->movieLoadType();
}

MediaTime MediaPlayer::mediaTimeForTimeValue(const MediaTime& timeValue) const
{
    return m_private->mediaTimeForTimeValue(timeValue);
}

double MediaPlayer::maximumDurationToCacheMediaTime() const
{
    return m_private->maximumDurationToCacheMediaTime();
}

unsigned MediaPlayer::decodedFrameCount() const
{
    return m_private->decodedFrameCount();
}

unsigned MediaPlayer::droppedFrameCount() const
{
    return m_private->droppedFrameCount();
}

unsigned MediaPlayer::audioDecodedByteCount() const
{
    return m_private->audioDecodedByteCount();
}

unsigned MediaPlayer::videoDecodedByteCount() const
{
    return m_private->videoDecodedByteCount();
}

void MediaPlayer::reloadTimerFired(Timer<MediaPlayer>&)
{
    m_private->cancelLoad();
    loadWithNextMediaEngine(m_currentMediaEngine);
}

void MediaPlayer::getSitesInMediaCache(Vector<String>& sites)
{
    Vector<MediaPlayerFactory*>& engines = installedMediaEngines();
    unsigned size = engines.size();
    for (unsigned i = 0; i < size; i++) {
        if (!engines[i]->getSitesInMediaCache)
            continue;
        Vector<String> engineSites;
        engines[i]->getSitesInMediaCache(engineSites);
        sites.appendVector(engineSites);
    }
}

void MediaPlayer::clearMediaCache()
{
    Vector<MediaPlayerFactory*>& engines = installedMediaEngines();
    unsigned size = engines.size();
    for (unsigned i = 0; i < size; i++) {
        if (engines[i]->clearMediaCache)
            engines[i]->clearMediaCache();
    }
}

void MediaPlayer::clearMediaCacheForSite(const String& site)
{
    Vector<MediaPlayerFactory*>& engines = installedMediaEngines();
    unsigned size = engines.size();
    for (unsigned i = 0; i < size; i++) {
        if (engines[i]->clearMediaCacheForSite)
            engines[i]->clearMediaCacheForSite(site);
    }
}

bool MediaPlayer::supportsKeySystem(const String& keySystem, const String& mimeType)
{
    for (auto& engine : installedMediaEngines()) {
        if (engine->supportsKeySystem && engine->supportsKeySystem(keySystem, mimeType))
            return true;
    }
    return false;
}

void MediaPlayer::setPrivateBrowsingMode(bool privateBrowsingMode)
{
    m_privateBrowsing = privateBrowsingMode;
    m_private->setPrivateBrowsingMode(m_privateBrowsing);
}

// Client callbacks.
void MediaPlayer::networkStateChanged()
{
    // If more than one media engine is installed and this one failed before finding metadata,
    // let the next engine try.
    if (m_private->networkState() >= FormatError
        && m_private->readyState() < HaveMetadata
        && installedMediaEngines().size() > 1) {
        if (m_contentMIMEType.isEmpty() || nextBestMediaEngine(m_currentMediaEngine)) {
            m_reloadTimer.startOneShot(0);
            return;
        }
    }
    if (m_mediaPlayerClient)
        m_mediaPlayerClient->mediaPlayerNetworkStateChanged(this);
}

void MediaPlayer::readyStateChanged()
{
    if (m_mediaPlayerClient)
        m_mediaPlayerClient->mediaPlayerReadyStateChanged(this);
}

void MediaPlayer::volumeChanged(double newVolume)
{
#if PLATFORM(IOS)
    UNUSED_PARAM(newVolume);
    m_volume = m_private->volume();
#else
    m_volume = newVolume;
#endif
    if (m_mediaPlayerClient)
        m_mediaPlayerClient->mediaPlayerVolumeChanged(this);
}

void MediaPlayer::muteChanged(bool newMuted)
{
    m_muted = newMuted;
    if (m_mediaPlayerClient)
        m_mediaPlayerClient->mediaPlayerMuteChanged(this);
}

void MediaPlayer::timeChanged()
{
    if (m_mediaPlayerClient)
        m_mediaPlayerClient->mediaPlayerTimeChanged(this);
}

void MediaPlayer::sizeChanged()
{
    if (m_mediaPlayerClient)
        m_mediaPlayerClient->mediaPlayerSizeChanged(this);
}

void MediaPlayer::repaint()
{
    if (m_mediaPlayerClient)
        m_mediaPlayerClient->mediaPlayerRepaint(this);
}

void MediaPlayer::durationChanged()
{
    if (m_mediaPlayerClient)
        m_mediaPlayerClient->mediaPlayerDurationChanged(this);
}

void MediaPlayer::rateChanged()
{
    if (m_mediaPlayerClient)
        m_mediaPlayerClient->mediaPlayerRateChanged(this);
}

void MediaPlayer::playbackStateChanged()
{
    if (m_mediaPlayerClient)
        m_mediaPlayerClient->mediaPlayerPlaybackStateChanged(this);
}

void MediaPlayer::firstVideoFrameAvailable()
{
    if (m_mediaPlayerClient)
        m_mediaPlayerClient->mediaPlayerFirstVideoFrameAvailable(this);
}

void MediaPlayer::characteristicChanged()
{
    if (m_mediaPlayerClient)
        m_mediaPlayerClient->mediaPlayerCharacteristicChanged(this);
}

#if ENABLE(WEB_AUDIO)
AudioSourceProvider* MediaPlayer::audioSourceProvider()
{
    return m_private->audioSourceProvider();
}
#endif // WEB_AUDIO

#if ENABLE(ENCRYPTED_MEDIA)
void MediaPlayer::keyAdded(const String& keySystem, const String& sessionId)
{
    if (m_mediaPlayerClient)
        m_mediaPlayerClient->mediaPlayerKeyAdded(this, keySystem, sessionId);
}

void MediaPlayer::keyError(const String& keySystem, const String& sessionId, MediaPlayerClient::MediaKeyErrorCode errorCode, unsigned short systemCode)
{
    if (m_mediaPlayerClient)
        m_mediaPlayerClient->mediaPlayerKeyError(this, keySystem, sessionId, errorCode, systemCode);
}

void MediaPlayer::keyMessage(const String& keySystem, const String& sessionId, const unsigned char* message, unsigned messageLength, const URL& defaultURL)
{
    if (m_mediaPlayerClient)
        m_mediaPlayerClient->mediaPlayerKeyMessage(this, keySystem, sessionId, message, messageLength, defaultURL);
}

bool MediaPlayer::keyNeeded(const String& keySystem, const String& sessionId, const unsigned char* initData, unsigned initDataLength)
{
    if (m_mediaPlayerClient)
        return m_mediaPlayerClient->mediaPlayerKeyNeeded(this, keySystem, sessionId, initData, initDataLength);
    return false;
}
#endif

#if ENABLE(ENCRYPTED_MEDIA_V2)
bool MediaPlayer::keyNeeded(Uint8Array* initData)
{
    if (m_mediaPlayerClient)
        return m_mediaPlayerClient->mediaPlayerKeyNeeded(this, initData);
    return false;
}
#endif

String MediaPlayer::referrer() const
{
    if (!m_mediaPlayerClient)
        return String();

    return m_mediaPlayerClient->mediaPlayerReferrer();
}

String MediaPlayer::userAgent() const
{
    if (!m_mediaPlayerClient)
        return String();
    
    return m_mediaPlayerClient->mediaPlayerUserAgent();
}

String MediaPlayer::engineDescription() const
{
    if (!m_private)
        return String();

    return m_private->engineDescription();
}

#if PLATFORM(WIN) && USE(AVFOUNDATION)
GraphicsDeviceAdapter* MediaPlayer::graphicsDeviceAdapter() const
{
    if (!m_mediaPlayerClient)
        return 0;
    
    return m_mediaPlayerClient->mediaPlayerGraphicsDeviceAdapter(this);
}
#endif

CachedResourceLoader* MediaPlayer::cachedResourceLoader()
{
    if (!m_mediaPlayerClient)
        return 0;

    return m_mediaPlayerClient->mediaPlayerCachedResourceLoader();
}

#if ENABLE(VIDEO_TRACK)
void MediaPlayer::addAudioTrack(PassRefPtr<AudioTrackPrivate> track)
{
    if (!m_mediaPlayerClient)
        return;

    m_mediaPlayerClient->mediaPlayerDidAddAudioTrack(track);
}

void MediaPlayer::removeAudioTrack(PassRefPtr<AudioTrackPrivate> track)
{
    if (!m_mediaPlayerClient)
        return;

    m_mediaPlayerClient->mediaPlayerDidRemoveAudioTrack(track);
}

void MediaPlayer::addTextTrack(PassRefPtr<InbandTextTrackPrivate> track)
{
    if (!m_mediaPlayerClient)
        return;

    m_mediaPlayerClient->mediaPlayerDidAddTextTrack(track);
}

void MediaPlayer::removeTextTrack(PassRefPtr<InbandTextTrackPrivate> track)
{
    if (!m_mediaPlayerClient)
        return;

    m_mediaPlayerClient->mediaPlayerDidRemoveTextTrack(track);
}

void MediaPlayer::addVideoTrack(PassRefPtr<VideoTrackPrivate> track)
{
    if (!m_mediaPlayerClient)
        return;

    m_mediaPlayerClient->mediaPlayerDidAddVideoTrack(track);
}

void MediaPlayer::removeVideoTrack(PassRefPtr<VideoTrackPrivate> track)
{
    if (!m_mediaPlayerClient)
        return;

    m_mediaPlayerClient->mediaPlayerDidRemoveVideoTrack(track);
}

bool MediaPlayer::requiresTextTrackRepresentation() const
{
    return m_private->requiresTextTrackRepresentation();
}

void MediaPlayer::setTextTrackRepresentation(TextTrackRepresentation* representation)
{
    m_private->setTextTrackRepresentation(representation);
}

void MediaPlayer::syncTextTrackBounds()
{
    m_private->syncTextTrackBounds();
}

#if ENABLE(AVF_CAPTIONS)
void MediaPlayer::notifyTrackModeChanged()
{
    if (m_private)
        m_private->notifyTrackModeChanged();
}

Vector<RefPtr<PlatformTextTrack>> MediaPlayer::outOfBandTrackSources()
{
    if (!m_mediaPlayerClient)
        return Vector<RefPtr<PlatformTextTrack>> ();
    
    return m_mediaPlayerClient->outOfBandTrackSources();
}
#endif

#endif // ENABLE(VIDEO_TRACK)

#if USE(PLATFORM_TEXT_TRACK_MENU)
bool MediaPlayer::implementsTextTrackControls() const
{
    return m_private->implementsTextTrackControls();
}

PassRefPtr<PlatformTextTrackMenuInterface> MediaPlayer::textTrackMenu()
{
    return m_private->textTrackMenu();
}
#endif // USE(PLATFORM_TEXT_TRACK_MENU)

void MediaPlayer::resetMediaEngines()
{
    installedMediaEngines(ResetEngines);
}

#if USE(GSTREAMER)
void MediaPlayer::simulateAudioInterruption()
{
    if (!m_private)
        return;

    m_private->simulateAudioInterruption();
}
#endif

String MediaPlayer::languageOfPrimaryAudioTrack() const
{
    if (!m_private)
        return emptyString();
    
    return m_private->languageOfPrimaryAudioTrack();
}

size_t MediaPlayer::extraMemoryCost() const
{
    if (!m_private)
        return 0;

    return m_private->extraMemoryCost();
}

unsigned long long MediaPlayer::fileSize() const
{
    if (!m_private)
        return 0;
    
    return m_private->fileSize();
}

#if ENABLE(MEDIA_SOURCE)
unsigned long MediaPlayer::totalVideoFrames()
{
    if (!m_private)
        return 0;

    return m_private->totalVideoFrames();
}

unsigned long MediaPlayer::droppedVideoFrames()
{
    if (!m_private)
        return 0;

    return m_private->droppedVideoFrames();
}

unsigned long MediaPlayer::corruptedVideoFrames()
{
    if (!m_private)
        return 0;

    return m_private->corruptedVideoFrames();
}

MediaTime MediaPlayer::totalFrameDelay()
{
    if (!m_private)
        return MediaTime::zeroTime();

    return m_private->totalFrameDelay();
}
#endif

bool MediaPlayer::shouldWaitForResponseToAuthenticationChallenge(const AuthenticationChallenge& challenge)
{
    if (!m_mediaPlayerClient)
        return false;

    return m_mediaPlayerClient->mediaPlayerShouldWaitForResponseToAuthenticationChallenge(challenge);
}
    
void MediaPlayer::handlePlaybackCommand(MediaSession::RemoteControlCommandType command)
{
    if (!m_mediaPlayerClient)
        return;
    
    m_mediaPlayerClient->mediaPlayerHandlePlaybackCommand(command);
}

String MediaPlayer::sourceApplicationIdentifier() const
{
    if (!m_mediaPlayerClient)
        return emptyString();

    return m_mediaPlayerClient->mediaPlayerSourceApplicationIdentifier();
}

void MediaPlayerFactorySupport::callRegisterMediaEngine(MediaEngineRegister registerMediaEngine)
{
    registerMediaEngine(addMediaEngine);
}

bool MediaPlayer::doesHaveAttribute(const AtomicString& attribute, AtomicString* value) const
{
    if (!m_mediaPlayerClient)
        return false;
    
    return m_mediaPlayerClient->doesHaveAttribute(attribute, value);
}

#if PLATFORM(IOS)
String MediaPlayer::mediaPlayerNetworkInterfaceName() const
{
    if (!m_mediaPlayerClient)
        return emptyString();

    return m_mediaPlayerClient->mediaPlayerNetworkInterfaceName();
}

bool MediaPlayer::getRawCookies(const URL& url, Vector<Cookie>& cookies) const
{
    if (!m_mediaPlayerClient)
        return false;

    return m_mediaPlayerClient->mediaPlayerGetRawCookies(url, cookies);
}
#endif

}

#endif
