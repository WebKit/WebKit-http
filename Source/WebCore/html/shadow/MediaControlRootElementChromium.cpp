/*
 * Copyright (C) 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#include "config.h"

#if ENABLE(VIDEO)
#include "MediaControlRootElementChromium.h"

#include "HTMLDivElement.h"
#include "HTMLMediaElement.h"
#include "HTMLNames.h"
#include "MediaControlElements.h"
#include "MouseEvent.h"
#include "Page.h"
#include "RenderTheme.h"
#include "Text.h"

#if ENABLE(VIDEO_TRACK)
#include "TextTrackCue.h"
#endif

using namespace std;

namespace WebCore {

MediaControlChromiumEnclosureElement::MediaControlChromiumEnclosureElement(Document* document)
    : HTMLDivElement(HTMLNames::divTag, document->document())
    , m_mediaController(0)
{
}

PassRefPtr<MediaControlChromiumEnclosureElement> MediaControlChromiumEnclosureElement::create(Document* document)
{
    return adoptRef(new MediaControlChromiumEnclosureElement(document));
}

const AtomicString& MediaControlChromiumEnclosureElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-enclosure"));
    return id;
}

MediaControlRootElementChromium::MediaControlRootElementChromium(Document* document)
    : MediaControls(document)
    , m_mediaController(0)
    , m_playButton(0)
    , m_currentTimeDisplay(0)
    , m_durationDisplay(0)
    , m_timeline(0)
    , m_panelMuteButton(0)
    , m_volumeSlider(0)
#if ENABLE(FULLSCREEN_MEDIA_CONTROLS)
    , m_fullscreenButton(0)
#endif
    , m_panel(0)
    , m_enclosure(0)
#if ENABLE(VIDEO_TRACK)
    , m_textDisplayContainer(0)
#endif
    , m_opaque(true)
    , m_isMouseOverControls(false)
{
}

PassRefPtr<MediaControls> MediaControls::create(Document* document)
{
    return MediaControlRootElementChromium::create(document);
}

PassRefPtr<MediaControlRootElementChromium> MediaControlRootElementChromium::create(Document* document)
{
    if (!document->page())
        return 0;

    RefPtr<MediaControlRootElementChromium> controls = adoptRef(new MediaControlRootElementChromium(document));

    // Create an enclosing element for the panel so we can visually offset the controls correctly.
    RefPtr<MediaControlChromiumEnclosureElement> enclosure = MediaControlChromiumEnclosureElement::create(document);

    RefPtr<MediaControlPanelElement> panel = MediaControlPanelElement::create(document);

    ExceptionCode ec;

    RefPtr<MediaControlPlayButtonElement> playButton = MediaControlPlayButtonElement::create(document);
    controls->m_playButton = playButton.get();
    panel->appendChild(playButton.release(), ec, true);
    if (ec)
        return 0;

    RefPtr<MediaControlTimelineElement> timeline = MediaControlTimelineElement::create(document, controls.get());
    controls->m_timeline = timeline.get();
    panel->appendChild(timeline.release(), ec, true);
    if (ec)
        return 0;

    RefPtr<MediaControlCurrentTimeDisplayElement> currentTimeDisplay = MediaControlCurrentTimeDisplayElement::create(document);
    controls->m_currentTimeDisplay = currentTimeDisplay.get();
    controls->m_currentTimeDisplay->hide();
    panel->appendChild(currentTimeDisplay.release(), ec, true);
    if (ec)
        return 0;

    RefPtr<MediaControlTimeRemainingDisplayElement> durationDisplay = MediaControlTimeRemainingDisplayElement::create(document);
    controls->m_durationDisplay = durationDisplay.get();
    panel->appendChild(durationDisplay.release(), ec, true);
    if (ec)
        return 0;

    RefPtr<MediaControlPanelMuteButtonElement> panelMuteButton = MediaControlPanelMuteButtonElement::create(document, controls.get());
    controls->m_panelMuteButton = panelMuteButton.get();
    panel->appendChild(panelMuteButton.release(), ec, true);
    if (ec)
        return 0;

    RefPtr<MediaControlVolumeSliderElement> slider = MediaControlVolumeSliderElement::create(document);
    controls->m_volumeSlider = slider.get();
    panel->appendChild(slider.release(), ec, true);
    if (ec)
        return 0;

#if ENABLE(FULLSCREEN_MEDIA_CONTROLS)
    RefPtr<MediaControlFullscreenButtonElement> fullscreenButton = MediaControlFullscreenButtonElement::create(document, controls.get());
    controls->m_fullscreenButton = fullscreenButton.get();
    panel->appendChild(fullscreenButton.release(), ec, true);
    if (ec)
        return 0;
#endif

    controls->m_panel = panel.get();
    enclosure->appendChild(panel.release(), ec, true);
    if (ec)
        return 0;

    controls->m_enclosure = enclosure.get();
    controls->appendChild(enclosure.release(), ec, true);
    if (ec)
        return 0;

    return controls.release();
}

void MediaControlRootElementChromium::setMediaController(MediaControllerInterface* controller)
{
    if (m_mediaController == controller)
        return;
    m_mediaController = controller;

    if (m_playButton)
        m_playButton->setMediaController(controller);
    if (m_currentTimeDisplay)
        m_currentTimeDisplay->setMediaController(controller);
    if (m_durationDisplay)
        m_durationDisplay->setMediaController(controller);
    if (m_timeline)
        m_timeline->setMediaController(controller);
    if (m_panelMuteButton)
        m_panelMuteButton->setMediaController(controller);
    if (m_volumeSlider)
        m_volumeSlider->setMediaController(controller);
#if ENABLE(FULLSCREEN_MEDIA_CONTROLS)
    if (m_fullscreenButton)
        m_fullscreenButton->setMediaController(controller);
#endif
    if (m_panel)
        m_panel->setMediaController(controller);
    if (m_enclosure)
        m_enclosure->setMediaController(controller);
#if ENABLE(VIDEO_TRACK)
    if (m_textDisplayContainer)
        m_textDisplayContainer->setMediaController(controller);
#endif
    reset();
}

void MediaControlRootElementChromium::show()
{
    m_panel->setIsDisplayed(true);
    m_panel->show();
}

void MediaControlRootElementChromium::hide()
{
    m_panel->setIsDisplayed(false);
    m_panel->hide();
}

void MediaControlRootElementChromium::makeOpaque()
{
    m_panel->makeOpaque();
}

void MediaControlRootElementChromium::makeTransparent()
{
    m_panel->makeTransparent();
}

void MediaControlRootElementChromium::reset()
{
    Page* page = document()->page();
    if (!page)
        return;

    updateStatusDisplay();

    float duration = m_mediaController->duration();
    m_timeline->setDuration(duration);
    m_timeline->show();

    m_durationDisplay->setInnerText(page->theme()->formatMediaControlsTime(duration), ASSERT_NO_EXCEPTION);
    m_durationDisplay->setCurrentValue(duration);

    m_timeline->setPosition(m_mediaController->currentTime());
    updateTimeDisplay();

    m_panelMuteButton->show();

    if (m_volumeSlider)
        m_volumeSlider->setVolume(m_mediaController->volume());

#if ENABLE(FULLSCREEN_MEDIA_CONTROLS)
    m_fullscreenButton->show();
#endif
    makeOpaque();
}

void MediaControlRootElementChromium::playbackStarted()
{
    m_playButton->updateDisplayType();
    m_timeline->setPosition(m_mediaController->currentTime());
    m_currentTimeDisplay->show();
    m_durationDisplay->hide();
    updateTimeDisplay();
}

void MediaControlRootElementChromium::playbackProgressed()
{
    m_timeline->setPosition(m_mediaController->currentTime());
    updateTimeDisplay();

    if (!m_isMouseOverControls && m_mediaController->hasVideo())
        makeTransparent();
}

void MediaControlRootElementChromium::playbackStopped()
{
    m_playButton->updateDisplayType();
    m_timeline->setPosition(m_mediaController->currentTime());
    updateTimeDisplay();
    makeOpaque();
}

void MediaControlRootElementChromium::updateTimeDisplay()
{
    float now = m_mediaController->currentTime();
    float duration = m_mediaController->duration();

    Page* page = document()->page();
    if (!page)
        return;

    // After seek, hide duration display and show current time.
    if (now > 0) {
        m_currentTimeDisplay->show();
        m_durationDisplay->hide();
    }

    // Allow the theme to format the time.
    ExceptionCode ec;
    m_currentTimeDisplay->setInnerText(page->theme()->formatMediaControlsCurrentTime(now, duration), ec);
    m_currentTimeDisplay->setCurrentValue(now);
}

void MediaControlRootElementChromium::reportedError()
{
    Page* page = document()->page();
    if (!page)
        return;

    m_timeline->hide();
    m_panelMuteButton->hide();
    m_volumeSlider->hide();
#if ENABLE(FULLSCREEN_MEDIA_CONTROLS)
    m_fullscreenButton->hide();
#endif
}

void MediaControlRootElementChromium::updateStatusDisplay()
{
}

bool MediaControlRootElementChromium::shouldHideControls()
{
    return !m_panel->hovered();
}

void MediaControlRootElementChromium::loadedMetadata()
{
    reset();
}

bool MediaControlRootElementChromium::containsRelatedTarget(Event* event)
{
    if (!event->isMouseEvent())
        return false;
    EventTarget* relatedTarget = static_cast<MouseEvent*>(event)->relatedTarget();
    if (!relatedTarget)
        return false;
    return contains(relatedTarget->toNode());
}

void MediaControlRootElementChromium::defaultEventHandler(Event* event)
{
    MediaControls::defaultEventHandler(event);

    if (event->type() == eventNames().mouseoverEvent) {
        if (!containsRelatedTarget(event)) {
            m_isMouseOverControls = true;
            if (!m_mediaController->canPlay())
                makeOpaque();
        }
    } else if (event->type() == eventNames().mouseoutEvent) {
        if (!containsRelatedTarget(event))
            m_isMouseOverControls = false;
    }
}

void MediaControlRootElementChromium::changedClosedCaptionsVisibility()
{
}

void MediaControlRootElementChromium::changedMute()
{
    m_panelMuteButton->changedMute();
}

void MediaControlRootElementChromium::changedVolume()
{
    m_volumeSlider->setVolume(m_mediaController->volume());
}

void MediaControlRootElementChromium::enteredFullscreen()
{
}

void MediaControlRootElementChromium::exitedFullscreen()
{
}

void MediaControlRootElementChromium::showVolumeSlider()
{
    if (!m_mediaController->hasAudio())
        return;

    m_volumeSlider->show();
}

#if ENABLE(VIDEO_TRACK)
void MediaControlRootElementChromium::createTextTrackDisplay()
{
    if (m_textDisplayContainer)
        return;

    RefPtr<MediaControlTextTrackContainerElement> textDisplayContainer = MediaControlTextTrackContainerElement::create(document());
    m_textDisplayContainer = textDisplayContainer.get();

    // Insert it before the first controller element so it always displays behind the controls.
    insertBefore(textDisplayContainer.release(), m_enclosure, ASSERT_NO_EXCEPTION, true);
}

void MediaControlRootElementChromium::showTextTrackDisplay()
{
    if (!m_textDisplayContainer)
        createTextTrackDisplay();
    m_textDisplayContainer->show();
}

void MediaControlRootElementChromium::hideTextTrackDisplay()
{
    if (!m_textDisplayContainer)
        createTextTrackDisplay();
    m_textDisplayContainer->hide();
}

void MediaControlRootElementChromium::updateTextTrackDisplay()
{
    if (!m_textDisplayContainer)
        createTextTrackDisplay();

    m_textDisplayContainer->updateDisplay();
}
#endif

const AtomicString& MediaControlRootElementChromium::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls"));
    return id;
}

void MediaControlRootElementChromium::bufferingProgressed()
{
    // We only need to update buffering progress when paused, during normal
    // playback playbackProgressed() will take care of it.
    if (m_mediaController->paused())
        m_timeline->setPosition(m_mediaController->currentTime());
}

}

#endif
