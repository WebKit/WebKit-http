/*
 * Copyright (C) 2012 Google Inc.  All rights reserved.
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

#ifndef MediaKeyEvent_h
#define MediaKeyEvent_h

#if ENABLE(ENCRYPTED_MEDIA)

#include "Event.h"
#include "MediaKeyError.h"

namespace WebCore {

struct MediaKeyEventInit : public EventInit {
    MediaKeyEventInit();

    String keySystem;
    String sessionId;
    RefPtr<Uint8Array> initData;
    RefPtr<Uint8Array> message;
    String defaultURL;
    RefPtr<MediaKeyError> errorCode;
    unsigned short systemCode;
};

class MediaKeyEvent : public Event {
public:
    virtual ~MediaKeyEvent();

    static PassRefPtr<MediaKeyEvent> create()
    {
        return adoptRef(new MediaKeyEvent);
    }

    static PassRefPtr<MediaKeyEvent> create(const AtomicString& type, const MediaKeyEventInit& initializer)
    {
        return adoptRef(new MediaKeyEvent(type, initializer));
    }

    virtual const AtomicString& interfaceName() const;

    String keySystem() const { return m_keySystem; }
    String sessionId() const { return m_sessionId; }
    Uint8Array* initData() const { return m_initData.get(); }
    Uint8Array* message() const { return m_message.get(); }
    String defaultURL() const { return m_defaultURL; }
    MediaKeyError* errorCode() const { return m_errorCode.get(); }
    unsigned short systemCode() const { return m_systemCode; }

private:
    MediaKeyEvent();
    MediaKeyEvent(const AtomicString& type, const MediaKeyEventInit& initializer);

    String m_keySystem;
    String m_sessionId;
    RefPtr<Uint8Array> m_initData;
    RefPtr<Uint8Array> m_message;
    String m_defaultURL;
    RefPtr<MediaKeyError> m_errorCode;
    unsigned short m_systemCode;
};

} // namespace WebCore

#endif
#endif
