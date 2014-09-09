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

#if ENABLE(VIDEO)

#include "HTMLMediaSession.h"

#include "Chrome.h"
#include "ChromeClient.h"
#include "Frame.h"
#include "HTMLMediaElement.h"
#include "HTMLNames.h"
#include "Logging.h"
#include "MediaSessionManager.h"
#include "Page.h"
#include "ScriptController.h"
#include "SourceBuffer.h"

#if PLATFORM(IOS)
#include "AudioSession.h"
#include "RuntimeApplicationChecksIOS.h"
#endif

namespace WebCore {

#if !LOG_DISABLED
static const char* restrictionName(HTMLMediaSession::BehaviorRestrictions restriction)
{
#define CASE(restriction) case HTMLMediaSession::restriction: return #restriction
    switch (restriction) {
    CASE(NoRestrictions);
    CASE(RequireUserGestureForLoad);
    CASE(RequireUserGestureForRateChange);
    CASE(RequireUserGestureForFullscreen);
    CASE(RequirePageConsentToLoadMedia);
    CASE(RequirePageConsentToResumeMedia);
#if ENABLE(IOS_AIRPLAY)
    CASE(RequireUserGestureToShowPlaybackTargetPicker);
    CASE(WirelessVideoPlaybackDisabled);
#endif
    }

    ASSERT_NOT_REACHED();
    return "";
}
#endif

std::unique_ptr<HTMLMediaSession> HTMLMediaSession::create(MediaSessionClient& client)
{
    return std::make_unique<HTMLMediaSession>(client);
}

HTMLMediaSession::HTMLMediaSession(MediaSessionClient& client)
    : MediaSession(client)
    , m_restrictions(NoRestrictions)
{
}

void HTMLMediaSession::addBehaviorRestriction(BehaviorRestrictions restriction)
{
    LOG(Media, "HTMLMediaSession::addBehaviorRestriction - adding %s", restrictionName(restriction));
    m_restrictions |= restriction;
}

void HTMLMediaSession::removeBehaviorRestriction(BehaviorRestrictions restriction)
{
    LOG(Media, "HTMLMediaSession::removeBehaviorRestriction - removing %s", restrictionName(restriction));
    m_restrictions &= ~restriction;
}

bool HTMLMediaSession::playbackPermitted(const HTMLMediaElement&) const
{
    if (m_restrictions & RequireUserGestureForRateChange && !ScriptController::processingUserGesture()) {
        LOG(Media, "HTMLMediaSession::playbackPermitted - returning FALSE");
        return false;
    }

    return true;
}

bool HTMLMediaSession::dataLoadingPermitted(const HTMLMediaElement&) const
{
    if (m_restrictions & RequireUserGestureForLoad && !ScriptController::processingUserGesture()) {
        LOG(Media, "HTMLMediaSession::dataLoadingPermitted - returning FALSE");
        return false;
    }

    return true;
}

bool HTMLMediaSession::fullscreenPermitted(const HTMLMediaElement&) const
{
    if (m_restrictions & RequireUserGestureForFullscreen && !ScriptController::processingUserGesture()) {
        LOG(Media, "HTMLMediaSession::fullscreenPermitted - returning FALSE");
        return false;
    }

    return true;
}

bool HTMLMediaSession::pageAllowsDataLoading(const HTMLMediaElement& element) const
{
    Page* page = element.document().page();
    if (m_restrictions & RequirePageConsentToLoadMedia && page && !page->canStartMedia()) {
        LOG(Media, "HTMLMediaSession::pageAllowsDataLoading - returning FALSE");
        return false;
    }

    return true;
}

bool HTMLMediaSession::pageAllowsPlaybackAfterResuming(const HTMLMediaElement& element) const
{
    Page* page = element.document().page();
    if (m_restrictions & RequirePageConsentToResumeMedia && page && !page->canStartMedia()) {
        LOG(Media, "HTMLMediaSession::pageAllowsPlaybackAfterResuming - returning FALSE");
        return false;
    }

    return true;
}

#if ENABLE(IOS_AIRPLAY)
bool HTMLMediaSession::showingPlaybackTargetPickerPermitted(const HTMLMediaElement& element) const
{
    if (m_restrictions & RequireUserGestureToShowPlaybackTargetPicker && !ScriptController::processingUserGesture()) {
        LOG(Media, "HTMLMediaSession::showingPlaybackTargetPickerPermitted - returning FALSE because of permissions");
        return false;
    }

    if (!element.document().page()) {
        LOG(Media, "HTMLMediaSession::showingPlaybackTargetPickerPermitted - returning FALSE because page is NULL");
        return false;
    }

    return !wirelessVideoPlaybackDisabled(element);
}

bool HTMLMediaSession::currentPlaybackTargetIsWireless(const HTMLMediaElement& element) const
{
    MediaPlayer* player = element.player();
    if (!player) {
        LOG(Media, "HTMLMediaSession::currentPlaybackTargetIsWireless - returning FALSE because player is NULL");
        return false;
    }

    bool isWireless = player->isCurrentPlaybackTargetWireless();
    LOG(Media, "HTMLMediaSession::currentPlaybackTargetIsWireless - returning %s", isWireless ? "TRUE" : "FALSE");

    return isWireless;
}

void HTMLMediaSession::showPlaybackTargetPicker(const HTMLMediaElement& element)
{
    LOG(Media, "HTMLMediaSession::showPlaybackTargetPicker");

    if (!showingPlaybackTargetPickerPermitted(element))
        return;

#if PLATFORM(IOS)
    element.document().frame()->page()->chrome().client().showPlaybackTargetPicker(element.hasVideo());
#endif
}

bool HTMLMediaSession::hasWirelessPlaybackTargets(const HTMLMediaElement& element) const
{
    UNUSED_PARAM(element);

    bool hasTargets = MediaSessionManager::sharedManager().hasWirelessTargetsAvailable();
    LOG(Media, "HTMLMediaSession::hasWirelessPlaybackTargets - returning %s", hasTargets ? "TRUE" : "FALSE");

    return hasTargets;
}

bool HTMLMediaSession::wirelessVideoPlaybackDisabled(const HTMLMediaElement& element) const
{
    Settings* settings = element.document().settings();
    if (!settings || !settings->mediaPlaybackAllowsAirPlay()) {
        LOG(Media, "HTMLMediaSession::wirelessVideoPlaybackDisabled - returning TRUE because of settings");
        return true;
    }

    if (element.fastHasAttribute(HTMLNames::webkitwirelessvideoplaybackdisabledAttr)) {
        LOG(Media, "HTMLMediaSession::wirelessVideoPlaybackDisabled - returning TRUE because of attribute");
        return true;
    }

#if PLATFORM(IOS)
    String legacyAirplayAttributeValue = element.fastGetAttribute(HTMLNames::webkitairplayAttr);
    if (equalIgnoringCase(legacyAirplayAttributeValue, "deny")) {
        LOG(Media, "HTMLMediaSession::wirelessVideoPlaybackDisabled - returning TRUE because of legacy attribute");
        return true;
    }
#endif

    MediaPlayer* player = element.player();
    if (!player)
        return false;

    bool disabled = player->wirelessVideoPlaybackDisabled();
    LOG(Media, "HTMLMediaSession::wirelessVideoPlaybackDisabled - returning %s because media engine says so", disabled ? "TRUE" : "FALSE");
    
    return disabled;
}

void HTMLMediaSession::setWirelessVideoPlaybackDisabled(const HTMLMediaElement& element, bool disabled)
{
    if (disabled)
        addBehaviorRestriction(WirelessVideoPlaybackDisabled);
    else
        removeBehaviorRestriction(WirelessVideoPlaybackDisabled);

    MediaPlayer* player = element.player();
    if (!player)
        return;

    LOG(Media, "HTMLMediaSession::setWirelessVideoPlaybackDisabled - disabled %s", disabled ? "TRUE" : "FALSE");
    player->setWirelessVideoPlaybackDisabled(disabled);
}

void HTMLMediaSession::setHasPlaybackTargetAvailabilityListeners(const HTMLMediaElement& element, bool hasListeners)
{
    LOG(Media, "HTMLMediaSession::setHasPlaybackTargetAvailabilityListeners - hasListeners %s", hasListeners ? "TRUE" : "FALSE");
    UNUSED_PARAM(element);

    if (hasListeners)
        MediaSessionManager::sharedManager().startMonitoringAirPlayRoutes();
    else
        MediaSessionManager::sharedManager().stopMonitoringAirPlayRoutes();
}
#endif

MediaPlayer::Preload HTMLMediaSession::effectivePreloadForElement(const HTMLMediaElement& element) const
{
    MediaSessionManager::SessionRestrictions restrictions = MediaSessionManager::sharedManager().restrictions(mediaType());
    MediaPlayer::Preload preload = element.preloadValue();

    if ((restrictions & MediaSessionManager::MetadataPreloadingNotPermitted) == MediaSessionManager::MetadataPreloadingNotPermitted)
        return MediaPlayer::None;

    if ((restrictions & MediaSessionManager::AutoPreloadingNotPermitted) == MediaSessionManager::AutoPreloadingNotPermitted) {
        if (preload > MediaPlayer::MetaData)
            return MediaPlayer::MetaData;
    }

    return preload;
}

bool HTMLMediaSession::requiresFullscreenForVideoPlayback(const HTMLMediaElement& element) const
{
    if (!MediaSessionManager::sharedManager().sessionRestrictsInlineVideoPlayback(*this))
        return false;

    Settings* settings = element.document().settings();
    if (!settings || !settings->mediaPlaybackAllowsInline())
        return true;

    if (element.fastHasAttribute(HTMLNames::webkit_playsinlineAttr))
        return false;

#if PLATFORM(IOS)
    if (applicationIsDumpRenderTree())
        return false;
#endif

    return true;
}

void HTMLMediaSession::applyMediaPlayerRestrictions(const HTMLMediaElement& element)
{
    LOG(Media, "HTMLMediaSession::applyMediaPlayerRestrictions");

#if ENABLE(IOS_AIRPLAY)
    setWirelessVideoPlaybackDisabled(element, m_restrictions & WirelessVideoPlaybackDisabled);
#else
    UNUSED_PARAM(element);
#endif
    
}

#if ENABLE(MEDIA_SOURCE)
const unsigned fiveMinutesOf1080PVideo = 290 * 1024 * 1024; // 290 MB is approximately 5 minutes of 8Mbps (1080p) content.
const unsigned fiveMinutesStereoAudio = 14 * 1024 * 1024; // 14 MB is approximately 5 minutes of 384kbps content.

size_t HTMLMediaSession::maximumMediaSourceBufferSize(const SourceBuffer& buffer) const
{
    // A good quality 1080p video uses 8,000 kbps and stereo audio uses 384 kbps, so assume 95% for video and 5% for audio.
    const float bufferBudgetPercentageForVideo = .95;
    const float bufferBudgetPercentageForAudio = .05;

    size_t maximum;
    Settings* settings = buffer.document().settings();
    if (settings)
        maximum = settings->maximumSourceBufferSize();
    else
        maximum = fiveMinutesOf1080PVideo + fiveMinutesStereoAudio;

    // Allow a SourceBuffer to buffer as though it is audio-only even if it doesn't have any active tracks (yet).
    size_t bufferSize = static_cast<size_t>(maximum * bufferBudgetPercentageForAudio);
    if (buffer.hasVideo())
        bufferSize += static_cast<size_t>(maximum * bufferBudgetPercentageForVideo);

    // FIXME: we might want to modify this algorithm to:
    // - decrease the maximum size for background tabs
    // - decrease the maximum size allowed for inactive elements when a process has more than one
    //   element, eg. so a page with many elements which are played one at a time doesn't keep
    //   everything buffered after an element has finished playing.

    return bufferSize;
}
#endif

}

#endif // ENABLE(VIDEO)
