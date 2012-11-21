/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#ifndef InbandTextTrack_h
#define InbandTextTrack_h

#if ENABLE(VIDEO_TRACK)

#include "TextTrack.h"
#include <wtf/RefPtr.h>

namespace WebCore {

class Document;
class InbandTextTrackPrivate;
class MediaPlayer;
class TextTrackCue;

class InbandTextTrackClient {
public:
    virtual ~InbandTextTrackClient() { }

    virtual void addCue(InbandTextTrackPrivate*, double /*start*/, double /*end*/, const String& /*id*/, const String& /*content*/, const String& /*settings*/) { }
};

class InbandTextTrack : public TextTrack, public InbandTextTrackClient {
public:
    static PassRefPtr<InbandTextTrack> create(ScriptExecutionContext* context, TextTrackClient* client, PassRefPtr<InbandTextTrackPrivate> playerPrivate)
    {
        return adoptRef(new InbandTextTrack(context, client, playerPrivate));
    }
    virtual ~InbandTextTrack();

    virtual void setMode(const AtomicString&) OVERRIDE;
    size_t inbandTrackIndex();

private:
    InbandTextTrack(ScriptExecutionContext*, TextTrackClient*, PassRefPtr<InbandTextTrackPrivate>);

    virtual void addCue(InbandTextTrackPrivate*, double, double, const String&, const String&, const String&);

    RefPtr<InbandTextTrackPrivate> m_private;
};

} // namespace WebCore

#endif
#endif
