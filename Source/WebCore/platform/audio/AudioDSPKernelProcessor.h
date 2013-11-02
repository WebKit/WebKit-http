/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#ifndef AudioDSPKernelProcessor_h
#define AudioDSPKernelProcessor_h

#include "AudioBus.h"
#include "AudioProcessor.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class AudioBus;
class AudioDSPKernel;
class AudioProcessor;

// AudioDSPKernelProcessor processes one input -> one output (N channels each)
// It uses one AudioDSPKernel object per channel to do the processing, thus there is no cross-channel processing.
// Despite this limitation it turns out to be a very common and useful type of processor.

class AudioDSPKernelProcessor : public AudioProcessor {
public:
    // numberOfChannels may be later changed if object is not yet in an "initialized" state
    AudioDSPKernelProcessor(float sampleRate, unsigned numberOfChannels);

    // Subclasses create the appropriate type of processing kernel here.
    // We'll call this to create a kernel for each channel.
    virtual PassOwnPtr<AudioDSPKernel> createKernel() = 0;

    // AudioProcessor methods
    virtual void initialize() OVERRIDE;
    virtual void uninitialize() OVERRIDE;
    virtual void process(const AudioBus* source, AudioBus* destination, size_t framesToProcess) OVERRIDE;
    virtual void reset() OVERRIDE;
    virtual void setNumberOfChannels(unsigned) OVERRIDE;
    virtual unsigned numberOfChannels() const OVERRIDE { return m_numberOfChannels; }

    virtual double tailTime() const OVERRIDE;
    virtual double latencyTime() const OVERRIDE;

protected:
    Vector<OwnPtr<AudioDSPKernel>> m_kernels;
    bool m_hasJustReset;
};

} // namespace WebCore

#endif // AudioDSPKernelProcessor_h
