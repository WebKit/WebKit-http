/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(SCRIPTED_SPEECH)

#include "SpeechRecognitionEvent.h"

namespace WebCore {

SpeechRecognitionEventInit::SpeechRecognitionEventInit()
    : resultIndex(0)
{
}

PassRefPtr<SpeechRecognitionEvent> SpeechRecognitionEvent::create()
{
    return adoptRef(new SpeechRecognitionEvent());
}

PassRefPtr<SpeechRecognitionEvent> SpeechRecognitionEvent::create(const AtomicString& eventName, const SpeechRecognitionEventInit& initializer)
{
    return adoptRef(new SpeechRecognitionEvent(eventName, initializer));
}

PassRefPtr<SpeechRecognitionEvent> SpeechRecognitionEvent::createResult(PassRefPtr<SpeechRecognitionResult> result, short resultIndex, PassRefPtr<SpeechRecognitionResultList> resultHistory)
{
    return adoptRef(new SpeechRecognitionEvent(eventNames().resultEvent, result, resultIndex, resultHistory));
}

PassRefPtr<SpeechRecognitionEvent> SpeechRecognitionEvent::createNoMatch(PassRefPtr<SpeechRecognitionResult> result)
{
    return adoptRef(new SpeechRecognitionEvent(eventNames().nomatchEvent, result, 0, 0));
}

PassRefPtr<SpeechRecognitionEvent> SpeechRecognitionEvent::createResultDeleted(short resultIndex, PassRefPtr<SpeechRecognitionResultList> resultHistory)
{
    return adoptRef(new SpeechRecognitionEvent(eventNames().resultdeletedEvent, 0, resultIndex, resultHistory));
}

const AtomicString& SpeechRecognitionEvent::interfaceName() const
{
    return eventNames().interfaceForSpeechRecognitionEvent;
}

SpeechRecognitionEvent::SpeechRecognitionEvent()
    : m_resultIndex(0)
{
}

SpeechRecognitionEvent::SpeechRecognitionEvent(const AtomicString& eventName, const SpeechRecognitionEventInit& initializer)
    : Event(eventName, initializer)
    , m_result(initializer.result)
    , m_resultIndex(initializer.resultIndex)
    , m_resultHistory(initializer.resultHistory)
{
}

SpeechRecognitionEvent::SpeechRecognitionEvent(const AtomicString& eventName, PassRefPtr<SpeechRecognitionResult> result, short resultIndex, PassRefPtr<SpeechRecognitionResultList> resultHistory)
    : Event(eventName, /*canBubble=*/false, /*cancelable=*/false)
    , m_result(result)
    , m_resultIndex(resultIndex)
    , m_resultHistory(resultHistory)
{
}

} // namespace WebCore

#endif // ENABLE(SCRIPTED_SPEECH)
