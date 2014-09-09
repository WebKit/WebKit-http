/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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

#import "config.h"

#if PLATFORM(IOS)
#import "WebVideoFullscreenModelVideoElement.h"

#import "DOMEventInternal.h"
#import "Logging.h"
#import "MediaControlsHost.h"
#import "WebVideoFullscreenInterface.h"
#import <QuartzCore/CoreAnimation.h>
#import <WebCore/DOMEventListener.h>
#import <WebCore/Event.h>
#import <WebCore/EventListener.h>
#import <WebCore/EventNames.h>
#import <WebCore/HTMLElement.h>
#import <WebCore/HTMLVideoElement.h>
#import <WebCore/Page.h>
#import <WebCore/PageGroup.h>
#import <WebCore/SoftLinking.h>
#import <WebCore/TextTrackList.h>
#import <WebCore/TimeRanges.h>
#import <WebCore/WebCoreThreadRun.h>
#import <wtf/NeverDestroyed.h>
#import <wtf/RetainPtr.h>


using namespace WebCore;

WebVideoFullscreenModelVideoElement::WebVideoFullscreenModelVideoElement()
    : EventListener(EventListener::CPPEventListenerType)
    , m_isListening(false)
{
}

WebVideoFullscreenModelVideoElement::~WebVideoFullscreenModelVideoElement()
{
}

void WebVideoFullscreenModelVideoElement::setVideoElement(HTMLVideoElement* videoElement)
{
    if (m_videoElement == videoElement)
        return;

    if (m_videoElement && m_isListening) {
        for (auto eventName : observedEventNames())
            m_videoElement->removeEventListener(eventName, this, false);
    }
    m_isListening = false;

    m_videoElement = videoElement;

    if (!m_videoElement)
        return;

    for (auto eventName : observedEventNames())
        m_videoElement->addEventListener(eventName, this, false);
    m_isListening = true;

    updateForEventName(eventNameAll());

    m_videoFullscreenInterface->setVideoDimensions(true, videoElement->videoWidth(), videoElement->videoHeight());
}

void WebVideoFullscreenModelVideoElement::handleEvent(WebCore::ScriptExecutionContext*, WebCore::Event* event)
{
    LOG(Media, "handleEvent %s", event->type().characters8());
    updateForEventName(event->type());
}

void WebVideoFullscreenModelVideoElement::updateForEventName(const WTF::AtomicString& eventName)
{
    if (!m_videoElement || !m_videoFullscreenInterface)
        return;
    
    bool all = eventName == eventNameAll();

    if (all
        || eventName == eventNames().durationchangeEvent) {
        m_videoFullscreenInterface->setDuration(m_videoElement->duration());
        // These is no standard event for minFastReverseRateChange; duration change is a reasonable proxy for it.
        // It happens every time a new item becomes ready to play.
        m_videoFullscreenInterface->setCanPlayFastReverse(m_videoElement->minFastReverseRate() < 0.0);
    }

    if (all
        || eventName == eventNames().pauseEvent
        || eventName == eventNames().playEvent
        || eventName == eventNames().ratechangeEvent)
        m_videoFullscreenInterface->setRate(!m_videoElement->paused(), m_videoElement->playbackRate());

    if (all
        || eventName == eventNames().timeupdateEvent) {
        m_videoFullscreenInterface->setCurrentTime(m_videoElement->currentTime(), [[NSProcessInfo processInfo] systemUptime]);
        // FIXME: 130788 - find a better event to update seekable ranges from.
        m_videoFullscreenInterface->setSeekableRanges(*m_videoElement->seekable());
    }

    if (all
        || eventName == eventNames().addtrackEvent
        || eventName == eventNames().removetrackEvent)
        updateLegibleOptions();

    if (all
        || eventName == eventNames().webkitcurrentplaybacktargetiswirelesschangedEvent) {
        bool enabled = m_videoElement->mediaSession().currentPlaybackTargetIsWireless(*m_videoElement);
        WebVideoFullscreenInterface::ExternalPlaybackTargetType targetType = WebVideoFullscreenInterface::TargetTypeNone;
        String localizedDeviceName;

        if (m_videoElement->mediaControlsHost()) {
            DEPRECATED_DEFINE_STATIC_LOCAL(String, airplay, (ASCIILiteral("airplay")));
            DEPRECATED_DEFINE_STATIC_LOCAL(String, tvout, (ASCIILiteral("tvout")));
            
            String type = m_videoElement->mediaControlsHost()->externalDeviceType();
            if (type == airplay)
                targetType = WebVideoFullscreenInterface::TargetTypeAirPlay;
            else if (type == tvout)
                targetType = WebVideoFullscreenInterface::TargetTypeTVOut;
            localizedDeviceName = m_videoElement->mediaControlsHost()->externalDeviceDisplayName();
        }
        m_videoFullscreenInterface->setExternalPlayback(enabled, targetType, localizedDeviceName);
    }
}

void WebVideoFullscreenModelVideoElement::setVideoFullscreenLayer(PlatformLayer* videoLayer)
{
    if (m_videoFullscreenLayer == videoLayer)
        return;
    
    m_videoFullscreenLayer = videoLayer;
    [m_videoFullscreenLayer setFrame:m_videoFrame];
    
    __block RefPtr<WebVideoFullscreenModelVideoElement> protect(this);
    WebThreadRun(^{
        if (m_videoElement)
            m_videoElement->setVideoFullscreenLayer(m_videoFullscreenLayer.get());
        protect.clear();
    });
}

void WebVideoFullscreenModelVideoElement::play()
{
    __block RefPtr<WebVideoFullscreenModelVideoElement> protect(this);
    WebThreadRun(^{
        if (m_videoElement)
            m_videoElement->play();
        protect.clear();
    });
}

void WebVideoFullscreenModelVideoElement::pause()
{
    __block RefPtr<WebVideoFullscreenModelVideoElement> protect(this);
    WebThreadRun(^{
        if (m_videoElement)
            m_videoElement->pause();
        protect.clear();
    });
}

void WebVideoFullscreenModelVideoElement::togglePlayState()
{
    __block RefPtr<WebVideoFullscreenModelVideoElement> protect(this);
    WebThreadRun(^{
        if (m_videoElement)
            m_videoElement->togglePlayState();
        protect.clear();
    });
}

void WebVideoFullscreenModelVideoElement::beginScrubbing()
{
    __block RefPtr<WebVideoFullscreenModelVideoElement> protect(this);
    WebThreadRun(^{
        if (m_videoElement)
            m_videoElement->beginScrubbing();
        protect.clear();
    });
}

void WebVideoFullscreenModelVideoElement::endScrubbing()
{
    __block RefPtr<WebVideoFullscreenModelVideoElement> protect(this);
    WebThreadRun(^{
        if (m_videoElement)
            m_videoElement->endScrubbing();
        protect.clear();
    });
}

void WebVideoFullscreenModelVideoElement::seekToTime(double time)
{
    __block RefPtr<WebVideoFullscreenModelVideoElement> protect(this);
    WebThreadRun(^{
        if (m_videoElement)
            m_videoElement->setCurrentTime(time);
        protect.clear();
    });
}

void WebVideoFullscreenModelVideoElement::fastSeek(double time)
{
    __block RefPtr<WebVideoFullscreenModelVideoElement> protect(this);
    WebThreadRun(^{
        if (m_videoElement)
            m_videoElement->fastSeek(time);
        protect.clear();
    });
}

void WebVideoFullscreenModelVideoElement::beginScanningForward()
{
    __block RefPtr<WebVideoFullscreenModelVideoElement> protect(this);
    WebThreadRun(^{
        if (m_videoElement)
            m_videoElement->beginScanning(MediaControllerInterface::Forward);
        protect.clear();
    });
}

void WebVideoFullscreenModelVideoElement::beginScanningBackward()
{
    __block RefPtr<WebVideoFullscreenModelVideoElement> protect(this);
    WebThreadRun(^{
        if (m_videoElement)
            m_videoElement->beginScanning(MediaControllerInterface::Backward);
        protect.clear();
    });
}

void WebVideoFullscreenModelVideoElement::endScanning()
{
    __block RefPtr<WebVideoFullscreenModelVideoElement> protect(this);
    WebThreadRun(^{
        if (m_videoElement)
            m_videoElement->endScanning();
        protect.clear();
    });
}

void WebVideoFullscreenModelVideoElement::requestExitFullscreen()
{
    if (!m_videoElement)
        return;

    __block RefPtr<WebVideoFullscreenModelVideoElement> protect(this);
    WebThreadRun(^{
        if (m_videoElement && m_videoElement->isFullscreen())
            m_videoElement->exitFullscreen();
        protect.clear();
    });
}

void WebVideoFullscreenModelVideoElement::setVideoLayerFrame(FloatRect rect)
{
    m_videoFrame = rect;
    [m_videoFullscreenLayer setFrame:CGRect(rect)];
    m_videoElement->setVideoFullscreenFrame(rect);
}

void WebVideoFullscreenModelVideoElement::setVideoLayerGravity(WebVideoFullscreenModel::VideoGravity gravity)
{
    MediaPlayer::VideoGravity videoGravity = MediaPlayer::VideoGravityResizeAspect;
    if (gravity == WebVideoFullscreenModel::VideoGravityResize)
        videoGravity = MediaPlayer::VideoGravityResize;
    else if (gravity == WebVideoFullscreenModel::VideoGravityResizeAspect)
        videoGravity = MediaPlayer::VideoGravityResizeAspect;
    else if (gravity == WebVideoFullscreenModel::VideoGravityResizeAspectFill)
        videoGravity = MediaPlayer::VideoGravityResizeAspectFill;
    else
        ASSERT_NOT_REACHED();
    
    m_videoElement->setVideoFullscreenGravity(videoGravity);
}

void WebVideoFullscreenModelVideoElement::selectAudioMediaOption(uint64_t)
{
    // FIXME: 131236 Implement audio track selection.
}

void WebVideoFullscreenModelVideoElement::selectLegibleMediaOption(uint64_t index)
{
    TextTrack* textTrack = nullptr;
    
    if (index < m_legibleTracksForMenu.size())
        textTrack = m_legibleTracksForMenu[static_cast<size_t>(index)].get();
    
    m_videoElement->setSelectedTextTrack(textTrack);
}

void WebVideoFullscreenModelVideoElement::updateLegibleOptions()
{
    TextTrackList* trackList = m_videoElement->textTracks();
    if (!trackList || !m_videoElement->document().page() || !m_videoElement->mediaControlsHost())
        return;
    
    WTF::AtomicString displayMode = m_videoElement->mediaControlsHost()->captionDisplayMode();
    TextTrack* offItem = m_videoElement->mediaControlsHost()->captionMenuOffItem();
    TextTrack* automaticItem = m_videoElement->mediaControlsHost()->captionMenuAutomaticItem();
    CaptionUserPreferences& captionPreferences = *m_videoElement->document().page()->group().captionPreferences();
    m_legibleTracksForMenu = captionPreferences.sortedTrackListForMenu(trackList);
    Vector<String> trackDisplayNames;
    uint64_t selectedIndex = 0;
    uint64_t offIndex = 0;
    bool trackMenuItemSelected = false;
    
    for (size_t index = 0; index < m_legibleTracksForMenu.size(); index++) {
        auto& track = m_legibleTracksForMenu[index];
        trackDisplayNames.append(captionPreferences.displayNameForTrack(track.get()));
        
        if (track == offItem)
            offIndex = index;
        
        if (track == automaticItem && displayMode == MediaControlsHost::automaticKeyword()) {
            selectedIndex = index;
            trackMenuItemSelected = true;
        }
        
        if (displayMode != MediaControlsHost::automaticKeyword() && track->mode() == TextTrack::showingKeyword()) {
            selectedIndex = index;
            trackMenuItemSelected = true;
        }
    }
    
    if (offIndex && !trackMenuItemSelected && displayMode == MediaControlsHost::forcedOnlyKeyword()) {
        selectedIndex = offIndex;
        trackMenuItemSelected = true;
    }
    
    m_videoFullscreenInterface->setLegibleMediaSelectionOptions(trackDisplayNames, selectedIndex);
}

const Vector<AtomicString>& WebVideoFullscreenModelVideoElement::observedEventNames()
{
    static NeverDestroyed<Vector<AtomicString>> sEventNames;

    if (!sEventNames.get().size()) {
        sEventNames.get().append(eventNames().durationchangeEvent);
        sEventNames.get().append(eventNames().pauseEvent);
        sEventNames.get().append(eventNames().playEvent);
        sEventNames.get().append(eventNames().ratechangeEvent);
        sEventNames.get().append(eventNames().timeupdateEvent);
        sEventNames.get().append(eventNames().addtrackEvent);
        sEventNames.get().append(eventNames().removetrackEvent);
        sEventNames.get().append(eventNames().webkitcurrentplaybacktargetiswirelesschangedEvent);
    }
    return sEventNames.get();
}

const AtomicString& WebVideoFullscreenModelVideoElement::eventNameAll()
{
    static NeverDestroyed<AtomicString> sEventNameAll = "allEvents";
    return sEventNameAll;
}

#endif
