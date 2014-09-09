/*
 * Copyright (C) 2014 Samsung Electronics
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PlatformSpeechSynthesisProviderEfl_h
#define PlatformSpeechSynthesisProviderEfl_h

#if ENABLE(SPEECH_SYNTHESIS)

#include <speak_lib.h>
#include <wtf/PassRefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class PlatformSpeechSynthesizer;
class PlatformSpeechSynthesisUtterance;
class PlatformSpeechSynthesisVoice;

class PlatformSpeechSynthesisProviderEfl {
public:
    enum SpeechEvent {
        SpeechError,
        SpeechCancel,
        SpeechPause,
        SpeechResume,
        SpeechStart
    };

    explicit PlatformSpeechSynthesisProviderEfl(PlatformSpeechSynthesizer*);
    ~PlatformSpeechSynthesisProviderEfl();

    void initializeVoiceList(Vector<RefPtr<PlatformSpeechSynthesisVoice>>&);
    void pause();
    void resume();
    void speak(PassRefPtr<PlatformSpeechSynthesisUtterance>);
    void cancel();
private:
    bool engineInit();

    int convertRateToEspeakValue(float) const;
    int convertVolumeToEspeakValue(float) const;
    int convertPitchToEspeakValue(float) const;

    espeak_VOICE* currentVoice() const;
    String voiceName(PassRefPtr<PlatformSpeechSynthesisUtterance>) const;
    void fireSpeechEvent(SpeechEvent);

    bool m_isEngineStarted;
    PlatformSpeechSynthesizer* m_platformSpeechSynthesizer;
    RefPtr<PlatformSpeechSynthesisUtterance> m_utterance;
};

} // namespace WebCore

#endif // ENABLE(SPEECH_SYNTHESIS)

#endif // PlatformSpeechSynthesisProviderEfl_h
