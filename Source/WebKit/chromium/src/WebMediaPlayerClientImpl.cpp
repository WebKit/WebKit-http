// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#include "WebMediaPlayerClientImpl.h"

#if ENABLE(VIDEO)

#include "AudioSourceProvider.h"
#include "AudioSourceProviderClient.h"
#include "Frame.h"
#include "GraphicsContext.h"
#include "HTMLMediaElement.h"
#include "IntSize.h"
#include "KURL.h"
#include "MediaPlayer.h"
#include "NotImplemented.h"
#include "RenderView.h"
#include "TimeRanges.h"
#include "VideoLayerChromium.h"

#if USE(ACCELERATED_COMPOSITING)
#include "RenderLayerCompositor.h"
#endif

#include "VideoFrameChromium.h"
#include "VideoFrameChromiumImpl.h"
#include "WebAudioSourceProvider.h"
#include "WebCanvas.h"
#include "WebCString.h"
#include "WebFrameClient.h"
#include "WebFrameImpl.h"
#include "WebKit.h"
#include "WebKitPlatformSupport.h"
#include "WebMediaElement.h"
#include "WebMediaPlayer.h"
#include "WebMimeRegistry.h"
#include "WebRect.h"
#include "WebSize.h"
#include "WebString.h"
#include "WebURL.h"
#include "WebViewImpl.h"

// WebCommon.h defines WEBKIT_USING_SKIA so this has to be included last.
#if WEBKIT_USING_SKIA
#include "PlatformContextSkia.h"
#endif

#include <wtf/Assertions.h>
#include <wtf/text/CString.h>

using namespace WebCore;

namespace WebKit {

static PassOwnPtr<WebMediaPlayer> createWebMediaPlayer(WebMediaPlayerClient* client, Frame* frame)
{
    WebFrameImpl* webFrame = WebFrameImpl::fromFrame(frame);

    if (!webFrame->client())
        return nullptr;
    return adoptPtr(webFrame->client()->createMediaPlayer(webFrame, client));
}

bool WebMediaPlayerClientImpl::m_isEnabled = false;

bool WebMediaPlayerClientImpl::isEnabled()
{
    return m_isEnabled;
}

void WebMediaPlayerClientImpl::setIsEnabled(bool isEnabled)
{
    m_isEnabled = isEnabled;
}

void WebMediaPlayerClientImpl::registerSelf(MediaEngineRegistrar registrar)
{
    if (m_isEnabled) {
        registrar(WebMediaPlayerClientImpl::create,
                  WebMediaPlayerClientImpl::getSupportedTypes,
                  WebMediaPlayerClientImpl::supportsType,
                  0,
                  0,
                  0);
    }
}

WebMediaPlayerClientImpl* WebMediaPlayerClientImpl::fromMediaElement(const WebMediaElement* element)
{
    PlatformMedia pm = element->constUnwrap<HTMLMediaElement>()->platformMedia();
    return static_cast<WebMediaPlayerClientImpl*>(pm.media.chromiumMediaPlayer);
}

WebMediaPlayer* WebMediaPlayerClientImpl::mediaPlayer() const
{
    return m_webMediaPlayer.get();
}

// WebMediaPlayerClient --------------------------------------------------------

WebMediaPlayerClientImpl::~WebMediaPlayerClientImpl()
{
    // VideoLayerChromium may outlive this object so clear the back pointer.
#if USE(ACCELERATED_COMPOSITING)
    if (m_videoLayer)
        m_videoLayer->releaseProvider();
#endif
}

void WebMediaPlayerClientImpl::networkStateChanged()
{
    ASSERT(m_mediaPlayer);
    m_mediaPlayer->networkStateChanged();
}

void WebMediaPlayerClientImpl::readyStateChanged()
{
    ASSERT(m_mediaPlayer);
    m_mediaPlayer->readyStateChanged();
#if USE(ACCELERATED_COMPOSITING)
    if (hasVideo() && supportsAcceleratedRendering() && !m_videoLayer)
        m_videoLayer = VideoLayerChromium::create(0, this);
#endif
}

void WebMediaPlayerClientImpl::volumeChanged(float newVolume)
{
    ASSERT(m_mediaPlayer);
    m_mediaPlayer->volumeChanged(newVolume);
}

void WebMediaPlayerClientImpl::muteChanged(bool newMute)
{
    ASSERT(m_mediaPlayer);
    m_mediaPlayer->muteChanged(newMute);
}

void WebMediaPlayerClientImpl::timeChanged()
{
    ASSERT(m_mediaPlayer);
    m_mediaPlayer->timeChanged();
}

void WebMediaPlayerClientImpl::repaint()
{
    ASSERT(m_mediaPlayer);
#if USE(ACCELERATED_COMPOSITING)
    if (m_videoLayer && supportsAcceleratedRendering())
        m_videoLayer->setNeedsDisplay(IntRect(0, 0, m_videoLayer->bounds().width(), m_videoLayer->bounds().height()));
#endif
    m_mediaPlayer->repaint();
}

void WebMediaPlayerClientImpl::durationChanged()
{
    ASSERT(m_mediaPlayer);
    m_mediaPlayer->durationChanged();
}

void WebMediaPlayerClientImpl::rateChanged()
{
    ASSERT(m_mediaPlayer);
    m_mediaPlayer->rateChanged();
}

void WebMediaPlayerClientImpl::sizeChanged()
{
    ASSERT(m_mediaPlayer);
    m_mediaPlayer->sizeChanged();
}

void WebMediaPlayerClientImpl::sawUnsupportedTracks()
{
    ASSERT(m_mediaPlayer);
    m_mediaPlayer->mediaPlayerClient()->mediaPlayerSawUnsupportedTracks(m_mediaPlayer);
}

float WebMediaPlayerClientImpl::volume() const
{
    if (m_mediaPlayer)
        return m_mediaPlayer->volume();
    return 0.0f;
}

void WebMediaPlayerClientImpl::playbackStateChanged()
{
    ASSERT(m_mediaPlayer);
    m_mediaPlayer->playbackStateChanged();
}

WebMediaPlayer::Preload WebMediaPlayerClientImpl::preload() const
{
    if (m_mediaPlayer)
        return static_cast<WebMediaPlayer::Preload>(m_mediaPlayer->preload());
    return static_cast<WebMediaPlayer::Preload>(m_preload);
}

void WebMediaPlayerClientImpl::sourceOpened()
{
#if ENABLE(MEDIA_SOURCE)
    ASSERT(m_mediaPlayer);
    m_mediaPlayer->sourceOpened();
#endif
}

WebKit::WebURL WebMediaPlayerClientImpl::sourceURL() const
{
#if ENABLE(MEDIA_SOURCE)
    ASSERT(m_mediaPlayer);
    return KURL(ParsedURLString, m_mediaPlayer->sourceURL());
#else
    return KURL();
#endif
}

// MediaPlayerPrivateInterface -------------------------------------------------

void WebMediaPlayerClientImpl::load(const String& url)
{
    m_url = url;

    if (m_preload == MediaPlayer::None) {
        m_webMediaPlayer.clear();
        m_delayingLoad = true;
    } else
        loadInternal();
}

void WebMediaPlayerClientImpl::loadInternal()
{
    Frame* frame = static_cast<HTMLMediaElement*>(m_mediaPlayer->mediaPlayerClient())->document()->frame();
    m_webMediaPlayer = createWebMediaPlayer(this, frame);
    if (m_webMediaPlayer) {
#if ENABLE(WEB_AUDIO)
        // Make sure if we create/re-create the WebMediaPlayer that we update our wrapper.
        m_audioSourceProvider.wrap(m_webMediaPlayer->audioSourceProvider());
#endif
        m_webMediaPlayer->load(KURL(ParsedURLString, m_url));
    }
}

void WebMediaPlayerClientImpl::cancelLoad()
{
    if (m_webMediaPlayer)
        m_webMediaPlayer->cancelLoad();
}

#if USE(ACCELERATED_COMPOSITING)
PlatformLayer* WebMediaPlayerClientImpl::platformLayer() const
{
    ASSERT(m_supportsAcceleratedCompositing);
    return m_videoLayer.get();
}
#endif

PlatformMedia WebMediaPlayerClientImpl::platformMedia() const
{
    PlatformMedia pm;
    pm.type = PlatformMedia::ChromiumMediaPlayerType;
    pm.media.chromiumMediaPlayer = const_cast<WebMediaPlayerClientImpl*>(this);
    return pm;
}

void WebMediaPlayerClientImpl::play()
{
    if (m_webMediaPlayer)
        m_webMediaPlayer->play();
}

void WebMediaPlayerClientImpl::pause()
{
    if (m_webMediaPlayer)
        m_webMediaPlayer->pause();
}

#if ENABLE(MEDIA_SOURCE)
bool WebMediaPlayerClientImpl::sourceAppend(const unsigned char* data, unsigned length)
{
    if (m_webMediaPlayer)
        return m_webMediaPlayer->sourceAppend(data, length);
    return false;
}

void WebMediaPlayerClientImpl::sourceEndOfStream(WebCore::MediaPlayer::EndOfStreamStatus status)
{
    if (m_webMediaPlayer)
        m_webMediaPlayer->sourceEndOfStream(static_cast<WebMediaPlayer::EndOfStreamStatus>(status));
}
#endif

void WebMediaPlayerClientImpl::prepareToPlay()
{
    if (m_delayingLoad)
        startDelayedLoad();
}

IntSize WebMediaPlayerClientImpl::naturalSize() const
{
    if (m_webMediaPlayer)
        return m_webMediaPlayer->naturalSize();
    return IntSize();
}

bool WebMediaPlayerClientImpl::hasVideo() const
{
    if (m_webMediaPlayer)
        return m_webMediaPlayer->hasVideo();
    return false;
}

bool WebMediaPlayerClientImpl::hasAudio() const
{
    if (m_webMediaPlayer)
        return m_webMediaPlayer->hasAudio();
    return false;
}

void WebMediaPlayerClientImpl::setVisible(bool visible)
{
    if (m_webMediaPlayer)
        m_webMediaPlayer->setVisible(visible);
}

float WebMediaPlayerClientImpl::duration() const
{
    if (m_webMediaPlayer)
        return m_webMediaPlayer->duration();
    return 0.0f;
}

float WebMediaPlayerClientImpl::currentTime() const
{
    if (m_webMediaPlayer)
        return m_webMediaPlayer->currentTime();
    return 0.0f;
}

void WebMediaPlayerClientImpl::seek(float time)
{
    if (m_webMediaPlayer)
        m_webMediaPlayer->seek(time);
}

bool WebMediaPlayerClientImpl::seeking() const
{
    if (m_webMediaPlayer)
        return m_webMediaPlayer->seeking();
    return false;
}

void WebMediaPlayerClientImpl::setEndTime(float time)
{
    if (m_webMediaPlayer)
        m_webMediaPlayer->setEndTime(time);
}

void WebMediaPlayerClientImpl::setRate(float rate)
{
    if (m_webMediaPlayer)
        m_webMediaPlayer->setRate(rate);
}

bool WebMediaPlayerClientImpl::paused() const
{
    if (m_webMediaPlayer)
        return m_webMediaPlayer->paused();
    return false;
}

bool WebMediaPlayerClientImpl::supportsFullscreen() const
{
    if (m_webMediaPlayer)
        return m_webMediaPlayer->supportsFullscreen();
    return false;
}

bool WebMediaPlayerClientImpl::supportsSave() const
{
    if (m_webMediaPlayer)
        return m_webMediaPlayer->supportsSave();
    return false;
}

void WebMediaPlayerClientImpl::setVolume(float volume)
{
    if (m_webMediaPlayer)
        m_webMediaPlayer->setVolume(volume);
}

MediaPlayer::NetworkState WebMediaPlayerClientImpl::networkState() const
{
    if (m_webMediaPlayer)
        return static_cast<MediaPlayer::NetworkState>(m_webMediaPlayer->networkState());
    return MediaPlayer::Empty;
}

MediaPlayer::ReadyState WebMediaPlayerClientImpl::readyState() const
{
    if (m_webMediaPlayer)
        return static_cast<MediaPlayer::ReadyState>(m_webMediaPlayer->readyState());
    return MediaPlayer::HaveNothing;
}

float WebMediaPlayerClientImpl::maxTimeSeekable() const
{
    if (m_webMediaPlayer)
        return m_webMediaPlayer->maxTimeSeekable();
    return 0.0f;
}

PassRefPtr<TimeRanges> WebMediaPlayerClientImpl::buffered() const
{
    if (m_webMediaPlayer) {
        const WebTimeRanges& webRanges = m_webMediaPlayer->buffered();

        // FIXME: Save the time ranges in a member variable and update it when needed.
        RefPtr<TimeRanges> ranges = TimeRanges::create();
        for (size_t i = 0; i < webRanges.size(); ++i)
            ranges->add(webRanges[i].start, webRanges[i].end);
        return ranges.release();
    }
    return TimeRanges::create();
}

int WebMediaPlayerClientImpl::dataRate() const
{
    if (m_webMediaPlayer)
        return m_webMediaPlayer->dataRate();
    return 0;
}

bool WebMediaPlayerClientImpl::totalBytesKnown() const
{
    if (m_webMediaPlayer)
        return m_webMediaPlayer->totalBytesKnown();
    return false;
}

unsigned WebMediaPlayerClientImpl::totalBytes() const
{
    if (m_webMediaPlayer)
        return static_cast<unsigned>(m_webMediaPlayer->totalBytes());
    return 0;
}

unsigned WebMediaPlayerClientImpl::bytesLoaded() const
{
    if (m_webMediaPlayer)
        return static_cast<unsigned>(m_webMediaPlayer->bytesLoaded());
    return 0;
}

void WebMediaPlayerClientImpl::setSize(const IntSize& size)
{
    if (m_webMediaPlayer)
        m_webMediaPlayer->setSize(WebSize(size.width(), size.height()));
}

void WebMediaPlayerClientImpl::paint(GraphicsContext* context, const IntRect& rect)
{
#if USE(ACCELERATED_COMPOSITING)
    // If we are using GPU to render video, ignore requests to paint frames into
    // canvas because it will be taken care of by VideoLayerChromium.
    if (acceleratedRenderingInUse())
        return;
#endif
    paintCurrentFrameInContext(context, rect);
}

void WebMediaPlayerClientImpl::paintCurrentFrameInContext(GraphicsContext* context, const IntRect& rect)
{
    // Normally GraphicsContext operations do nothing when painting is disabled.
    // Since we're accessing platformContext() directly we have to manually
    // check.
    if (m_webMediaPlayer && !context->paintingDisabled()) {
#if WEBKIT_USING_SKIA
        PlatformGraphicsContext* platformContext = context->platformContext();
        WebCanvas* canvas = platformContext->canvas();

        canvas->saveLayerAlpha(0, platformContext->getNormalizedAlpha());

        m_webMediaPlayer->paint(canvas, rect);

        canvas->restore();
#elif WEBKIT_USING_CG
        m_webMediaPlayer->paint(context->platformContext(), rect);
#else
        notImplemented();
#endif
    }
}

void WebMediaPlayerClientImpl::setPreload(MediaPlayer::Preload preload)
{
    m_preload = preload;

    if (m_webMediaPlayer)
        m_webMediaPlayer->setPreload(static_cast<WebMediaPlayer::Preload>(preload));

    if (m_delayingLoad && m_preload != MediaPlayer::None)
        startDelayedLoad();
}

bool WebMediaPlayerClientImpl::hasSingleSecurityOrigin() const
{
    if (m_webMediaPlayer)
        return m_webMediaPlayer->hasSingleSecurityOrigin();
    return false;
}

MediaPlayer::MovieLoadType WebMediaPlayerClientImpl::movieLoadType() const
{
    if (m_webMediaPlayer)
        return static_cast<MediaPlayer::MovieLoadType>(
            m_webMediaPlayer->movieLoadType());
    return MediaPlayer::Unknown;
}

float WebMediaPlayerClientImpl::mediaTimeForTimeValue(float timeValue) const
{
    if (m_webMediaPlayer)
        return m_webMediaPlayer->mediaTimeForTimeValue(timeValue);
    return timeValue;
}

unsigned WebMediaPlayerClientImpl::decodedFrameCount() const
{
    if (m_webMediaPlayer)
        return m_webMediaPlayer->decodedFrameCount();
    return 0;
}

unsigned WebMediaPlayerClientImpl::droppedFrameCount() const
{
    if (m_webMediaPlayer)
        return m_webMediaPlayer->droppedFrameCount();
    return 0;
}

unsigned WebMediaPlayerClientImpl::audioDecodedByteCount() const
{
    if (m_webMediaPlayer)
        return m_webMediaPlayer->audioDecodedByteCount();
    return 0;
}

unsigned WebMediaPlayerClientImpl::videoDecodedByteCount() const
{
    if (m_webMediaPlayer)
        return m_webMediaPlayer->videoDecodedByteCount();
    return 0;
}

#if ENABLE(WEB_AUDIO)
AudioSourceProvider* WebMediaPlayerClientImpl::audioSourceProvider()
{
    return &m_audioSourceProvider;
}
#endif

#if USE(ACCELERATED_COMPOSITING)
bool WebMediaPlayerClientImpl::supportsAcceleratedRendering() const
{
    return m_supportsAcceleratedCompositing;
}

bool WebMediaPlayerClientImpl::acceleratedRenderingInUse()
{
    return m_videoLayer && m_videoLayer->layerTreeHost();
}

VideoFrameChromium* WebMediaPlayerClientImpl::getCurrentFrame()
{
    ASSERT(!m_currentVideoFrame);
    if (m_webMediaPlayer && !m_currentVideoFrame) {
        WebVideoFrame* webkitVideoFrame = m_webMediaPlayer->getCurrentFrame();
        if (webkitVideoFrame)
            m_currentVideoFrame = adoptPtr(new VideoFrameChromiumImpl(webkitVideoFrame));
    }
    return m_currentVideoFrame.get();
}

void WebMediaPlayerClientImpl::putCurrentFrame(VideoFrameChromium* videoFrame)
{
    if (videoFrame && videoFrame == m_currentVideoFrame) {
        if (m_webMediaPlayer) {
            m_webMediaPlayer->putCurrentFrame(
                VideoFrameChromiumImpl::toWebVideoFrame(videoFrame));
        }
        ASSERT(videoFrame == m_currentVideoFrame);
        m_currentVideoFrame.clear();
    }
}
#endif

PassOwnPtr<MediaPlayerPrivateInterface> WebMediaPlayerClientImpl::create(MediaPlayer* player)
{
    OwnPtr<WebMediaPlayerClientImpl> client = adoptPtr(new WebMediaPlayerClientImpl());
    client->m_mediaPlayer = player;

#if USE(ACCELERATED_COMPOSITING)
    Frame* frame = static_cast<HTMLMediaElement*>(
        client->m_mediaPlayer->mediaPlayerClient())->document()->frame();

    // This does not actually check whether the hardware can support accelerated
    // compositing, but only if the flag is set. However, this is checked lazily
    // in WebViewImpl::setIsAcceleratedCompositingActive() and will fail there
    // if necessary.
    client->m_supportsAcceleratedCompositing =
        frame->contentRenderer()->compositor()->hasAcceleratedCompositing();
#endif

    return client.release();
}

void WebMediaPlayerClientImpl::getSupportedTypes(HashSet<String>& supportedTypes)
{
    // FIXME: integrate this list with WebMediaPlayerClientImpl::supportsType.
    notImplemented();
}

MediaPlayer::SupportsType WebMediaPlayerClientImpl::supportsType(const String& type,
                                                                 const String& codecs)
{
    WebMimeRegistry::SupportsType supportsType = webKitPlatformSupport()->mimeRegistry()->supportsMediaMIMEType(type, codecs);

    switch (supportsType) {
    default:
        ASSERT_NOT_REACHED();
    case WebMimeRegistry::IsNotSupported:
        return MediaPlayer::IsNotSupported;
    case WebMimeRegistry::IsSupported:
        return MediaPlayer::IsSupported;
    case WebMimeRegistry::MayBeSupported:
        return MediaPlayer::MayBeSupported;
    }
    return MediaPlayer::IsNotSupported;
}

void WebMediaPlayerClientImpl::startDelayedLoad()
{
    ASSERT(m_delayingLoad);
    ASSERT(!m_webMediaPlayer);

    m_delayingLoad = false;

    loadInternal();
}

WebMediaPlayerClientImpl::WebMediaPlayerClientImpl()
    : m_mediaPlayer(0)
    , m_delayingLoad(false)
    , m_preload(MediaPlayer::MetaData)
#if USE(ACCELERATED_COMPOSITING)
    , m_videoLayer(0)
    , m_supportsAcceleratedCompositing(false)
#endif
{
}

#if ENABLE(WEB_AUDIO)
void WebMediaPlayerClientImpl::AudioSourceProviderImpl::wrap(WebAudioSourceProvider* provider)
{
    if (m_webAudioSourceProvider && m_webAudioSourceProvider != provider)
        m_webAudioSourceProvider->setClient(0);
    m_webAudioSourceProvider = provider;
    if (m_webAudioSourceProvider)
        m_webAudioSourceProvider->setClient(&m_client);
}

void WebMediaPlayerClientImpl::AudioSourceProviderImpl::setClient(AudioSourceProviderClient* client)
{
    m_client.wrap(client);
    if (m_webAudioSourceProvider)
        m_webAudioSourceProvider->setClient(&m_client);
}

void WebMediaPlayerClientImpl::AudioSourceProviderImpl::provideInput(AudioBus* bus, size_t framesToProcess)
{
    ASSERT(bus);
    if (!bus)
        return;

    if (!m_webAudioSourceProvider) {
        bus->zero();
        return;
    }

    // Wrap the AudioBus channel data using WebVector.
    size_t n = bus->numberOfChannels();
    WebVector<float*> webAudioData(n);
    for (size_t i = 0; i < n; ++i)
        webAudioData[i] = bus->channel(i)->data();

    m_webAudioSourceProvider->provideInput(webAudioData, framesToProcess);
}

void WebMediaPlayerClientImpl::AudioClientImpl::setFormat(size_t numberOfChannels, float sampleRate)
{
    if (m_client)
        m_client->setFormat(numberOfChannels, sampleRate);
}

#endif

} // namespace WebKit

#endif  // ENABLE(VIDEO)
