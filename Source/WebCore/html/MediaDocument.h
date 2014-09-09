/*
 * Copyright (C) 2008,2009 Apple Inc. All Rights Reserved.
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

#ifndef MediaDocument_h
#define MediaDocument_h

#if ENABLE(VIDEO)

#include "HTMLDocument.h"

namespace WebCore {

class MediaDocument final : public HTMLDocument {
public:
    static PassRefPtr<MediaDocument> create(Frame* frame, const URL& url)
    {
        return adoptRef(new MediaDocument(frame, url));
    }
    virtual ~MediaDocument();

    void mediaElementSawUnsupportedTracks();

private:
    MediaDocument(Frame*, const URL&);

    virtual PassRefPtr<DocumentParser> createParser() override;

    virtual void defaultEventHandler(Event*) override;

    void replaceMediaElementTimerFired(Timer<MediaDocument>&);

    Timer<MediaDocument> m_replaceMediaElementTimer;
};

inline bool isMediaDocument(const Document& document) { return document.isMediaDocument(); }
void isMediaDocument(const MediaDocument&); // Catch unnecessary runtime check of type known at compile time.

DOCUMENT_TYPE_CASTS(MediaDocument)

}

#endif
#endif
