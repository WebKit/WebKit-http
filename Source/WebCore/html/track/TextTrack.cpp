/*
 * Copyright (C) 2011 Google Inc.  All rights reserved.
 * Copyright (C) 2011 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(VIDEO_TRACK)

#include "TextTrack.h"

#include "Event.h"
#include "ExceptionCode.h"
#include "HTMLMediaElement.h"
#include "TextTrackCueList.h"
#include "TextTrackList.h"
#include "TrackBase.h"

namespace WebCore {

static const int invalidTrackIndex = -1;

const AtomicString& TextTrack::subtitlesKeyword()
{
    DEFINE_STATIC_LOCAL(const AtomicString, subtitles, ("subtitles", AtomicString::ConstructFromLiteral));
    return subtitles;
}

const AtomicString& TextTrack::captionsKeyword()
{
    DEFINE_STATIC_LOCAL(const AtomicString, captions, ("captions", AtomicString::ConstructFromLiteral));
    return captions;
}

const AtomicString& TextTrack::descriptionsKeyword()
{
    DEFINE_STATIC_LOCAL(const AtomicString, descriptions, ("descriptions", AtomicString::ConstructFromLiteral));
    return descriptions;
}

const AtomicString& TextTrack::chaptersKeyword()
{
    DEFINE_STATIC_LOCAL(const AtomicString, chapters, ("chapters", AtomicString::ConstructFromLiteral));
    return chapters;
}

const AtomicString& TextTrack::metadataKeyword()
{
    DEFINE_STATIC_LOCAL(const AtomicString, metadata, ("metadata", AtomicString::ConstructFromLiteral));
    return metadata;
}

const AtomicString& TextTrack::disabledKeyword()
{
    DEFINE_STATIC_LOCAL(const AtomicString, open, ("disabled", AtomicString::ConstructFromLiteral));
    return open;
}

const AtomicString& TextTrack::hiddenKeyword()
{
    DEFINE_STATIC_LOCAL(const AtomicString, closed, ("hidden", AtomicString::ConstructFromLiteral));
    return closed;
}

const AtomicString& TextTrack::showingKeyword()
{
    DEFINE_STATIC_LOCAL(const AtomicString, ended, ("showing", AtomicString::ConstructFromLiteral));
    return ended;
}

TextTrack::TextTrack(ScriptExecutionContext* context, TextTrackClient* client, const String& kind, const String& label, const String& language, TextTrackType type)
    : TrackBase(context, TrackBase::TextTrack)
    , m_cues(0)
    , m_mediaElement(0)
    , m_label(label)
    , m_language(language)
    , m_mode(disabledKeyword())
    , m_client(client)
    , m_trackType(type)
    , m_readinessState(NotLoaded)
    , m_showingByDefault(false)
    , m_trackIndex(invalidTrackIndex)
{
    setKind(kind);
}

TextTrack::~TextTrack()
{
    if (m_client && m_cues)
        m_client->textTrackRemoveCues(this, m_cues.get());
    clearClient();
}

bool TextTrack::isValidKindKeyword(const String& value)
{
    if (equalIgnoringCase(value, subtitlesKeyword()))
        return true;
    if (equalIgnoringCase(value, captionsKeyword()))
        return true;
    if (equalIgnoringCase(value, descriptionsKeyword()))
        return true;
    if (equalIgnoringCase(value, chaptersKeyword()))
        return true;
    if (equalIgnoringCase(value, metadataKeyword()))
        return true;

    return false;
}

void TextTrack::setKind(const String& kind)
{
    String oldKind = m_kind;

    if (isValidKindKeyword(kind))
        m_kind = kind;
    else
        m_kind = subtitlesKeyword();

    if (m_client && oldKind != m_kind)
        m_client->textTrackKindChanged(this);
}

void TextTrack::setMode(const String& mode)
{
    // On setting, if the new value isn't equal to what the attribute would currently
    // return, the new value must be processed as follows ...
    if (mode != disabledKeyword() && mode != hiddenKeyword() && mode != showingKeyword())
        return;

    if (m_mode == mode)
        return;

    // If mode changes to disabled, remove this track's cues from the client
    // because they will no longer be accessible from the cues() function.
    if (mode == disabledKeyword() && m_client && m_cues)
        m_client->textTrackRemoveCues(this, m_cues.get());
         
    if (mode != showingKeyword() && m_cues)
        for (size_t i = 0; i < m_cues->length(); ++i)
            m_cues->item(i)->removeDisplayTree();

    //  ... Note: If the mode had been showing by default, this will change it to showing, 
    // even though the value of mode would appear not to change.
    m_mode = mode;
    setShowingByDefault(false);

    if (m_client)
        m_client->textTrackModeChanged(this);
}

String TextTrack::mode() const
{
    // The text track "showing" and "showing by default" modes return the string "showing".
    if (m_showingByDefault)
        return showingKeyword();
    return m_mode;
}

TextTrackCueList* TextTrack::cues()
{
    // 4.8.10.12.5 If the text track mode ... is not the text track disabled mode,
    // then the cues attribute must return a live TextTrackCueList object ...
    // Otherwise, it must return null. When an object is returned, the
    // same object must be returned each time.
    // http://www.whatwg.org/specs/web-apps/current-work/#dom-texttrack-cues
    if (m_mode != disabledKeyword())
        return ensureTextTrackCueList();
    return 0;
}

TextTrackCueList* TextTrack::activeCues() const
{
    // 4.8.10.12.5 If the text track mode ... is not the text track disabled mode,
    // then the activeCues attribute must return a live TextTrackCueList object ...
    // ... whose active flag was set when the script started, in text track cue
    // order. Otherwise, it must return null. When an object is returned, the
    // same object must be returned each time.
    // http://www.whatwg.org/specs/web-apps/current-work/#dom-texttrack-activecues
    if (m_cues && m_mode != disabledKeyword())
        return m_cues->activeCues();
    return 0;
}

void TextTrack::addCue(PassRefPtr<TextTrackCue> prpCue, ExceptionCode& ec)
{
    if (!prpCue)
        return;

    RefPtr<TextTrackCue> cue = prpCue;

    // TODO(93143): Add spec-compliant behavior for negative time values.
    if (isnan(cue->startTime()) || isnan(cue->endTime()) || cue->startTime() < 0 || cue->endTime() < 0)
        return;

    // 4.8.10.12.4 Text track API

    // The addCue(cue) method of TextTrack objects, when invoked, must run the following steps:

    // 1. If the given cue is already associated with a text track other than 
    // the method's TextTrack object's text track, then throw an InvalidStateError
    // exception and abort these steps.
    TextTrack* cueTrack = cue->track();
    if (cueTrack && cueTrack != this) {
        ec = INVALID_STATE_ERR;
        return;
    }

    // 2. Associate cue with the method's TextTrack object's text track, if it is 
    // not currently associated with a text track.
    cue->setTrack(this);

    // 3. If the given cue is already listed in the method's TextTrack object's text
    // track's text track list of cues, then throw an InvalidStateError exception.
    // 4. Add cue to the method's TextTrack object's text track's text track list of cues.
    if (!ensureTextTrackCueList()->add(cue)) {
        ec = INVALID_STATE_ERR;
        return;
    }
    
    if (m_client)
        m_client->textTrackAddCue(this, cue.get());
}

void TextTrack::removeCue(TextTrackCue* cue, ExceptionCode& ec)
{
    if (!cue)
        return;

    // 4.8.10.12.4 Text track API

    // The removeCue(cue) method of TextTrack objects, when invoked, must run the following steps:

    // 1. If the given cue is not associated with the method's TextTrack 
    // object's text track, then throw an InvalidStateError exception.
    if (cue->track() != this) {
        ec = INVALID_STATE_ERR;
        return;
    }
    
    // 2. If the given cue is not currently listed in the method's TextTrack 
    // object's text track's text track list of cues, then throw a NotFoundError exception.
    // 3. Remove cue from the method's TextTrack object's text track's text track list of cues.
    if (!m_cues || !m_cues->remove(cue)) {
        ec = INVALID_STATE_ERR;
        return;
    }

    cue->setTrack(0);
    if (m_client)
        m_client->textTrackRemoveCue(this, cue);
}

void TextTrack::cueWillChange(TextTrackCue* cue)
{
    if (!m_client)
        return;

    // The cue may need to be repositioned in the media element's interval tree, may need to
    // be re-rendered, etc, so remove it before the modification...
    m_client->textTrackRemoveCue(this, cue);
}

void TextTrack::cueDidChange(TextTrackCue* cue)
{
    if (!m_client)
        return;

    // ... and add it back again.
    m_client->textTrackAddCue(this, cue);
}

int TextTrack::trackIndex()
{
    ASSERT(m_mediaElement);

    if (m_trackIndex == invalidTrackIndex)
        m_trackIndex = m_mediaElement->textTracks()->getTrackIndex(this);

    return m_trackIndex;
}

void TextTrack::invalidateTrackIndex()
{
    m_trackIndex = invalidTrackIndex;
}

bool TextTrack::isRendered()
{
    if (m_kind != captionsKeyword() && m_kind != subtitlesKeyword())
        return false;

    if (m_mode != showingKeyword() && !m_showingByDefault)
        return false;

    return true;
}

TextTrackCueList* TextTrack::ensureTextTrackCueList()
{
    if (!m_cues)
        m_cues = TextTrackCueList::create();

    return m_cues.get();
}

} // namespace WebCore

#endif
