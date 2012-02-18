/*
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

#if ENABLE(VIDEO_TRACK)
#include "HTMLTrackElement.h"

#include "ContentSecurityPolicy.h"
#include "Event.h"
#include "HTMLMediaElement.h"
#include "HTMLNames.h"
#include "Logging.h"
#include "RuntimeEnabledFeatures.h"
#include "ScriptEventListener.h"

using namespace std;

namespace WebCore {

using namespace HTMLNames;

#if !LOG_DISABLED
static String urlForLogging(const KURL& url)
{
    static const unsigned maximumURLLengthForLogging = 128;
    
    if (url.string().length() < maximumURLLengthForLogging)
        return url.string();
    return url.string().substring(0, maximumURLLengthForLogging) + "...";
}
#endif
    
inline HTMLTrackElement::HTMLTrackElement(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document)
    , m_hasBeenConfigured(false)
{
    LOG(Media, "HTMLTrackElement::HTMLTrackElement - %p", this);
    ASSERT(hasTagName(trackTag));
}

HTMLTrackElement::~HTMLTrackElement()
{
    if (m_track)
        m_track->clearClient();
}

PassRefPtr<HTMLTrackElement> HTMLTrackElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLTrackElement(tagName, document));
}

void HTMLTrackElement::insertedIntoDocument()
{
    HTMLElement::insertedIntoDocument();

    if (HTMLMediaElement* parent = mediaElement())
        parent->trackWasAdded(this);
}

void HTMLTrackElement::removedFromDocument()
{
    if (HTMLMediaElement* parent = mediaElement())
        parent->trackWasRemoved(this);

    HTMLElement::removedFromDocument();
}

void HTMLTrackElement::parseAttribute(Attribute* attribute)
{
    const QualifiedName& attrName = attribute->name();

    if (RuntimeEnabledFeatures::webkitVideoTrackEnabled()) {
        if (attrName == srcAttr) {
            if (!attribute->isEmpty() && mediaElement())
                scheduleLoad();
            // 4.8.10.12.3 Sourcing out-of-band text tracks
            // As the kind, label, and srclang attributes are set, changed, or removed, the text track must update accordingly...
        } else if (attrName == kindAttr)
            track()->setKind(attribute->value());
        else if (attrName == labelAttr)
            track()->setLabel(attribute->value());
        else if (attrName == srclangAttr)
            track()->setLanguage(attribute->value());
    }

    if (attrName == onloadAttr)
        setAttributeEventListener(eventNames().loadEvent, createAttributeEventListener(this, attribute));
    else if (attrName == onerrorAttr)
        setAttributeEventListener(eventNames().errorEvent, createAttributeEventListener(this, attribute));
    else
        HTMLElement::parseAttribute(attribute);
}

KURL HTMLTrackElement::src() const
{
    return document()->completeURL(getAttribute(srcAttr));
}

void HTMLTrackElement::setSrc(const String& url)
{
    setAttribute(srcAttr, url);
}

String HTMLTrackElement::kind()
{
    return track()->kind();
}

void HTMLTrackElement::setKind(const String& kind)
{
    setAttribute(kindAttr, kind);
}

String HTMLTrackElement::srclang() const
{
    return getAttribute(srclangAttr);
}

void HTMLTrackElement::setSrclang(const String& srclang)
{
    setAttribute(srclangAttr, srclang);
}

String HTMLTrackElement::label() const
{
    return getAttribute(labelAttr);
}

void HTMLTrackElement::setLabel(const String& label)
{
    setAttribute(labelAttr, label);
}

bool HTMLTrackElement::isDefault() const
{
    return fastHasAttribute(defaultAttr);
}

void HTMLTrackElement::setIsDefault(bool isDefault)
{
    setBooleanAttribute(defaultAttr, isDefault);
}

LoadableTextTrack* HTMLTrackElement::ensureTrack()
{
    if (!m_track) {
        // The kind attribute is an enumerated attribute, limited only to know values. It defaults to 'subtitles' if missing or invalid.
        String kind = getAttribute(kindAttr);
        if (!TextTrack::isValidKindKeyword(kind))
            kind = TextTrack::subtitlesKeyword();
        m_track = LoadableTextTrack::create(this, kind, label(), srclang(), isDefault());
    }
    return m_track.get();
}

TextTrack* HTMLTrackElement::track()
{
    return ensureTrack();
}

bool HTMLTrackElement::isURLAttribute(Attribute* attribute) const
{
    return attribute->name() == srcAttr || HTMLElement::isURLAttribute(attribute);
}

void HTMLTrackElement::scheduleLoad()
{
    if (!RuntimeEnabledFeatures::webkitVideoTrackEnabled())
        return;

    if (!mediaElement())
        return;

    if (!fastHasAttribute(srcAttr))
        return;

    // 4.8.10.12.3 Sourcing out-of-band text tracks

    // 1. Set the text track readiness state to loading.
    setReadyState(HTMLTrackElement::LOADING);

    KURL url = getNonEmptyURLAttribute(srcAttr);
    if (!canLoadUrl(url)) {
        didCompleteLoad(ensureTrack(), HTMLTrackElement::Failure);
        return;
    }

    ensureTrack()->scheduleLoad(url);
}

bool HTMLTrackElement::canLoadUrl(const KURL& url)
{
    if (!RuntimeEnabledFeatures::webkitVideoTrackEnabled())
        return false;

    HTMLMediaElement* parent = mediaElement();
    if (!parent)
        return false;

    // 4.8.10.12.3 Sourcing out-of-band text tracks

    // 4. Download: If URL is not the empty string, perform a potentially CORS-enabled fetch of URL, with the
    // mode being the state of the media element's crossorigin content attribute, the origin being the
    // origin of the media element's Document, and the default origin behaviour set to fail.
    if (url.isEmpty())
        return false;

    if (!document()->contentSecurityPolicy()->allowMediaFromSource(url)) {
        DEFINE_STATIC_LOCAL(String, consoleMessage, ("Text track load denied by Content Security Policy."));
        document()->addConsoleMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, consoleMessage);
        LOG(Media, "HTMLTrackElement::canLoadUrl(%s) -> rejected by Content Security Policy", urlForLogging(url).utf8().data());
        return false;
    }
    
    return dispatchBeforeLoadEvent(url.string());
}

void HTMLTrackElement::didCompleteLoad(LoadableTextTrack*, LoadStatus status)
{
    ExceptionCode ec = 0;

    // 4.8.10.12.3 Sourcing out-of-band text tracks (continued)
    
    // 4. Download: ...
    // If the fetching algorithm fails for any reason (network error, the server returns an error 
    // code, a cross-origin check fails, etc), or if URL is the empty string or has the wrong origin 
    // as determined by the condition at the start of this step, or if the fetched resource is not in
    // a supported format, then queue a task to first change the text track readiness state to failed
    // to load and then fire a simple event named error at the track element; and then, once that task
    // is queued, move on to the step below labeled monitoring.

    if (status == Failure) {
        setReadyState(HTMLTrackElement::TRACK_ERROR);
        dispatchEvent(Event::create(eventNames().errorEvent, false, false), ec);
        return;
    }

    // If the fetching algorithm does not fail, then the final task that is queued by the networking
    // task source must run the following steps:
    //     1. Change the text track readiness state to loaded.
    setReadyState(HTMLTrackElement::LOADED);

    //     2. If the file was successfully processed, fire a simple event named load at the 
    //        track element.
    dispatchEvent(Event::create(eventNames().loadEvent, false, false), ec);
}

// NOTE: The values in the TextTrack::ReadinessState enum must stay in sync with those in HTMLTrackElement::ReadyState.
COMPILE_ASSERT(HTMLTrackElement::NONE == static_cast<HTMLTrackElement::ReadyState>(TextTrack::NotLoaded), TextTrackEnumNotLoaded_Is_Wrong_Should_Be_HTMLTrackElementEnumNONE);
COMPILE_ASSERT(HTMLTrackElement::LOADING == static_cast<HTMLTrackElement::ReadyState>(TextTrack::Loading), TextTrackEnumLoadingIsWrong_ShouldBe_HTMLTrackElementEnumLOADING);
COMPILE_ASSERT(HTMLTrackElement::LOADED == static_cast<HTMLTrackElement::ReadyState>(TextTrack::Loaded), TextTrackEnumLoaded_Is_Wrong_Should_Be_HTMLTrackElementEnumLOADED);
COMPILE_ASSERT(HTMLTrackElement::TRACK_ERROR == static_cast<HTMLTrackElement::ReadyState>(TextTrack::FailedToLoad), TextTrackEnumFailedToLoad_Is_Wrong_Should_Be_HTMLTrackElementEnumTRACK_ERROR);

void HTMLTrackElement::setReadyState(ReadyState state)
{
    ensureTrack()->setReadinessState(static_cast<TextTrack::ReadinessState>(state));
    if (HTMLMediaElement* parent = mediaElement())
        return parent->textTrackReadyStateChanged(m_track.get());
}

HTMLTrackElement::ReadyState HTMLTrackElement::readyState() 
{
    return static_cast<ReadyState>(ensureTrack()->readinessState());
}

const AtomicString& HTMLTrackElement::mediaElementCrossOriginAttribute() const
{
    if (HTMLMediaElement* parent = mediaElement())
        return parent->fastGetAttribute(HTMLNames::crossoriginAttr);
    
    return nullAtom;
}

void HTMLTrackElement::textTrackKindChanged(TextTrack* track)
{
    if (HTMLMediaElement* parent = mediaElement())
        return parent->textTrackKindChanged(track);
}

void HTMLTrackElement::textTrackModeChanged(TextTrack* track)
{
    if (HTMLMediaElement* parent = mediaElement())
        return parent->textTrackModeChanged(track);
}

void HTMLTrackElement::textTrackAddCues(TextTrack* track, const TextTrackCueList* cues)
{
    if (HTMLMediaElement* parent = mediaElement())
        return parent->textTrackAddCues(track, cues);
}
    
void HTMLTrackElement::textTrackRemoveCues(TextTrack* track, const TextTrackCueList* cues)
{
    if (HTMLMediaElement* parent = mediaElement())
        return parent->textTrackAddCues(track, cues);
}
    
void HTMLTrackElement::textTrackAddCue(TextTrack* track, PassRefPtr<TextTrackCue> cue)
{
    if (HTMLMediaElement* parent = mediaElement())
        return parent->textTrackAddCue(track, cue);
}
    
void HTMLTrackElement::textTrackRemoveCue(TextTrack* track, PassRefPtr<TextTrackCue> cue)
{
    if (HTMLMediaElement* parent = mediaElement())
        return parent->textTrackRemoveCue(track, cue);
}

HTMLMediaElement* HTMLTrackElement::mediaElement() const
{
    Element* parent = parentElement();
    if (parent && parent->isMediaElement())
        return static_cast<HTMLMediaElement*>(parentNode());

    return 0;
}

#if ENABLE(MICRODATA)
String HTMLTrackElement::itemValueText() const
{
    return getURLAttribute(srcAttr);
}

void HTMLTrackElement::setItemValueText(const String& value, ExceptionCode&)
{
    setAttribute(srcAttr, value);
}
#endif

}

#endif
