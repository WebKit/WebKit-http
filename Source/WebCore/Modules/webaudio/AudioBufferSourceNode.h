/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "AudioScheduledSourceNode.h"
#include <wtf/Lock.h>
#include <wtf/UniqueArray.h>

namespace WebCore {

class AudioBuffer;
class PannerNodeBase;

// AudioBufferSourceNode is an AudioNode representing an audio source from an in-memory audio asset represented by an AudioBuffer.
// It generally will be used for short sounds which require a high degree of scheduling flexibility (can playback in rhythmically perfect ways).

class AudioBufferSourceNode final : public AudioScheduledSourceNode {
    WTF_MAKE_ISO_ALLOCATED(AudioBufferSourceNode);
public:
    static Ref<AudioBufferSourceNode> create(BaseAudioContext&, float sampleRate);

    virtual ~AudioBufferSourceNode();

    // AudioNode
    void process(size_t framesToProcess) final;
    void reset() final;

    // setBuffer() is called on the main thread.  This is the buffer we use for playback.
    // returns true on success.
    void setBuffer(RefPtr<AudioBuffer>&&);
    AudioBuffer* buffer() { return m_buffer.get(); }

    // numberOfChannels() returns the number of output channels.  This value equals the number of channels from the buffer.
    // If a new buffer is set with a different number of channels, then this value will dynamically change.
    unsigned numberOfChannels();

    // Play-state
    ExceptionOr<void> startLater(double when, double grainOffset, Optional<double> grainDuration);

    // Note: the attribute was originally exposed as .looping, but to be more consistent in naming with <audio>
    // and with how it's described in the specification, the proper attribute name is .loop
    // The old attribute is kept for backwards compatibility.
    bool loop() const { return m_isLooping; }
    void setLoop(bool looping) { m_isLooping = looping; }

    // Loop times in seconds.
    double loopStart() const { return m_loopStart; }
    double loopEnd() const { return m_loopEnd; }
    void setLoopStart(double loopStart) { m_loopStart = loopStart; }
    void setLoopEnd(double loopEnd) { m_loopEnd = loopEnd; }

    // Deprecated.
    bool looping();
    void setLooping(bool);

    AudioParam* gain() { return m_gain.get(); }
    AudioParam* playbackRate() { return m_playbackRate.get(); }

    // If a panner node is set, then we can incorporate doppler shift into the playback pitch rate.
    void setPannerNode(PannerNodeBase*);
    void clearPannerNode();

    // If we are no longer playing, propogate silence ahead to downstream nodes.
    bool propagatesSilence() const final;

    // AudioScheduledSourceNode
    void finish() final;

    const char* activeDOMObjectName() const override { return "AudioBufferSourceNode"; }

private:
    AudioBufferSourceNode(BaseAudioContext&, float sampleRate);

    double tailTime() const final { return 0; }
    double latencyTime() const final { return 0; }

    enum BufferPlaybackMode {
        Entire,
        Partial
    };

    ExceptionOr<void> startPlaying(BufferPlaybackMode, double when, double grainOffset, double grainDuration);

    // Returns true on success.
    bool renderFromBuffer(AudioBus*, unsigned destinationFrameOffset, size_t numberOfFrames);

    // Render silence starting from "index" frame in AudioBus.
    inline bool renderSilenceAndFinishIfNotLooping(AudioBus*, unsigned index, size_t framesToProcess);

    // m_buffer holds the sample data which this node outputs.
    RefPtr<AudioBuffer> m_buffer;

    // Pointers for the buffer and destination.
    UniqueArray<const float*> m_sourceChannels;
    UniqueArray<float*> m_destinationChannels;

    // Used for the "gain" and "playbackRate" attributes.
    RefPtr<AudioParam> m_gain;
    RefPtr<AudioParam> m_playbackRate;

    // If m_isLooping is false, then this node will be done playing and become inactive after it reaches the end of the sample data in the buffer.
    // If true, it will wrap around to the start of the buffer each time it reaches the end.
    bool m_isLooping;

    double m_loopStart;
    double m_loopEnd;

    // m_virtualReadIndex is a sample-frame index into our buffer representing the current playback position.
    // Since it's floating-point, it has sub-sample accuracy.
    double m_virtualReadIndex;

    // Granular playback
    bool m_isGrain;
    double m_grainOffset; // in seconds
    double m_grainDuration; // in seconds

    // totalPitchRate() returns the instantaneous pitch rate (non-time preserving).
    // It incorporates the base pitch rate, any sample-rate conversion factor from the buffer, and any doppler shift from an associated panner node.
    double totalPitchRate();

    // m_lastGain provides continuity when we dynamically adjust the gain.
    float m_lastGain;

    // We optionally keep track of a panner node which has a doppler shift that is incorporated into
    // the pitch rate. We manually manage ref-counting because we want to use RefTypeConnection.
    PannerNodeBase* m_pannerNode;

    // This synchronizes process() with setBuffer() which can cause dynamic channel count changes.
    mutable Lock m_processMutex;
};

} // namespace WebCore
