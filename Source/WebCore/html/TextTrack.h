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

#ifndef TextTrack_h
#define TextTrack_h

#if ENABLE(VIDEO_TRACK)

#include "ExceptionCode.h"
#include "TrackBase.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class TextTrack;
class TextTrackCue;
class TextTrackCueList;

class TextTrackClient {
public:
    virtual ~TextTrackClient() { }
    virtual void textTrackReadyStateChanged(TextTrack*) = 0;
    virtual void textTrackKindChanged(TextTrack*) = 0;
    virtual void textTrackModeChanged(TextTrack*) = 0;
    virtual void textTrackAddCues(TextTrack*, const TextTrackCueList*) = 0;
    virtual void textTrackRemoveCues(TextTrack*, const TextTrackCueList*) = 0;
    virtual void textTrackAddCue(TextTrack*, PassRefPtr<TextTrackCue>) = 0;
    virtual void textTrackRemoveCue(TextTrack*, PassRefPtr<TextTrackCue>) = 0;
};

class TextTrack : public TrackBase {
public:
    static PassRefPtr<TextTrack> create(ScriptExecutionContext* context, TextTrackClient* client, const String& kind, const String& label, const String& language)
    {
        return adoptRef(new TextTrack(context, client, kind, label, language, AddTrack));
    }
    virtual ~TextTrack();

    String kind() const { return m_kind; }
    void setKind(const String&);

    static const AtomicString& subtitlesKeyword();
    static const AtomicString& captionsKeyword();
    static const AtomicString& descriptionsKeyword();
    static const AtomicString& chaptersKeyword();
    static const AtomicString& metadataKeyword();
    static bool isValidKindKeyword(const String&);

    String label() const { return m_label; }
    void setLabel(const String& label) { m_label = label; }

    String language() const { return m_language; }
    void setLanguage(const String& language) { m_language = language; }

    enum ReadyState { NONE = 0, LOADING = 1, LOADED = 2, HTML_ERROR = 3 };
    ReadyState readyState() const { return m_readyState; }

    enum Mode { DISABLED = 0, HIDDEN = 1, SHOWING = 2 };
    Mode mode() const { return m_mode; }
    void setMode(unsigned short, ExceptionCode&);

    TextTrackCueList* cues();
    TextTrackCueList* activeCues() const;

    void readyStateChanged();
    void modeChanged();

    virtual void clearClient() { m_client = 0; }
    TextTrackClient* client() { return m_client; }

    void addCue(PassRefPtr<TextTrackCue>, ExceptionCode&);
    void removeCue(TextTrackCue*, ExceptionCode&);
    
    virtual void fireCueChangeEvent();
    DEFINE_ATTRIBUTE_EVENT_LISTENER(cuechange);

    enum TextTrackType { TrackElement, AddTrack, InBand };
    TextTrackType trackType() const { return m_trackType; }

protected:
    TextTrack(ScriptExecutionContext*, TextTrackClient*, const String& kind, const String& label, const String& language, TextTrackType);

    void setReadyState(ReadyState);

    RefPtr<TextTrackCueList> m_cues;

private:
    String m_kind;
    String m_label;
    String m_language;
    TextTrack::ReadyState m_readyState;
    TextTrack::Mode m_mode;
    TextTrackClient* m_client;
    TextTrackType m_trackType;
};

} // namespace WebCore

#endif
#endif
