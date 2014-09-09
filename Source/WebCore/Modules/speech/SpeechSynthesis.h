/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef SpeechSynthesis_h
#define SpeechSynthesis_h

#if ENABLE(SPEECH_SYNTHESIS)

#include "PlatformSpeechSynthesisUtterance.h"
#include "PlatformSpeechSynthesizer.h"
#include "SpeechSynthesisUtterance.h"
#include "SpeechSynthesisVoice.h"
#include <wtf/Deque.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {
    
class PlatformSpeechSynthesizerClient;
class SpeechSynthesisVoice;
    
class SpeechSynthesis : public PlatformSpeechSynthesizerClient, public RefCounted<SpeechSynthesis> {
public:
    static PassRefPtr<SpeechSynthesis> create();
    
    bool pending() const;
    bool speaking() const;
    bool paused() const;
    
    void speak(SpeechSynthesisUtterance*);
    void cancel();
    void pause();
    void resume();
    
    const Vector<RefPtr<SpeechSynthesisVoice>>& getVoices();
    
    // Used in testing to use a mock platform synthesizer
    void setPlatformSynthesizer(std::unique_ptr<PlatformSpeechSynthesizer>);
    
private:
    SpeechSynthesis();
    
    // PlatformSpeechSynthesizerClient override methods.
    virtual void voicesDidChange() override;
    virtual void didStartSpeaking(PassRefPtr<PlatformSpeechSynthesisUtterance>) override;
    virtual void didPauseSpeaking(PassRefPtr<PlatformSpeechSynthesisUtterance>) override;
    virtual void didResumeSpeaking(PassRefPtr<PlatformSpeechSynthesisUtterance>) override;
    virtual void didFinishSpeaking(PassRefPtr<PlatformSpeechSynthesisUtterance>) override;
    virtual void speakingErrorOccurred(PassRefPtr<PlatformSpeechSynthesisUtterance>) override;
    virtual void boundaryEventOccurred(PassRefPtr<PlatformSpeechSynthesisUtterance>, SpeechBoundary, unsigned charIndex) override;

    void startSpeakingImmediately(SpeechSynthesisUtterance*);
    void handleSpeakingCompleted(SpeechSynthesisUtterance*, bool errorOccurred);
    void fireEvent(const AtomicString& type, SpeechSynthesisUtterance*, unsigned long charIndex, const String& name);
    
#if PLATFORM(IOS)
    // Restrictions to change default behaviors.
    enum BehaviorRestrictionFlags {
        NoRestrictions = 0,
        RequireUserGestureForSpeechStartRestriction = 1 << 0,
    };
    typedef unsigned BehaviorRestrictions;
    
    bool userGestureRequiredForSpeechStart() const { return m_restrictions & RequireUserGestureForSpeechStartRestriction; }
    void removeBehaviorRestriction(BehaviorRestrictions restriction) { m_restrictions &= ~restriction; }
#endif
    std::unique_ptr<PlatformSpeechSynthesizer> m_platformSpeechSynthesizer;
    Vector<RefPtr<SpeechSynthesisVoice>> m_voiceList;
    SpeechSynthesisUtterance* m_currentSpeechUtterance;
    Deque<RefPtr<SpeechSynthesisUtterance>> m_utteranceQueue;
    bool m_isPaused;
#if PLATFORM(IOS)
    BehaviorRestrictions m_restrictions;
#endif
};
    
} // namespace WebCore

#endif // ENABLE(SPEECH_SYNTHESIS)

#endif // SpeechSynthesisEvent_h
