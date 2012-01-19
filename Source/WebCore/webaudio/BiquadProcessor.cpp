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

#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "BiquadProcessor.h"

#include "BiquadDSPKernel.h"

namespace WebCore {
    
BiquadProcessor::BiquadProcessor(float sampleRate, size_t numberOfChannels, bool autoInitialize)
    : AudioDSPKernelProcessor(sampleRate, numberOfChannels)
    , m_type(LowPass)
    , m_parameter1(0)
    , m_parameter2(0)
    , m_parameter3(0)
    , m_filterCoefficientsDirty(true)
{
    double nyquist = 0.5 * this->sampleRate();

    // Create parameters for BiquadFilterNode.
    m_parameter1 = AudioParam::create("frequency", 350.0, 10.0, nyquist);
    m_parameter2 = AudioParam::create("Q", 1, 0.0001, 1000.0);
    m_parameter3 = AudioParam::create("gain", 0.0, -40, 40);

    if (autoInitialize)
        initialize();
}

BiquadProcessor::BiquadProcessor(FilterType type, float sampleRate, size_t numberOfChannels, bool autoInitialize)
    : AudioDSPKernelProcessor(sampleRate, numberOfChannels)
    , m_type(type)
    , m_parameter1(0)
    , m_parameter2(0)
    , m_parameter3(0)
    , m_filterCoefficientsDirty(true)
{
    double nyquist = 0.5 * this->sampleRate();
    
    // Handle the deprecated LowPass2FilterNode and HighPass2FilterNode.
    switch (type) {
    // Highpass and lowpass share the same parameters and only differ in filter type.
    case LowPass:
    case HighPass:
        m_parameter1 = AudioParam::create("frequency", 350.0, 20.0, nyquist);
        m_parameter2 = AudioParam::create("resonance", 0.0, -20.0, 20.0);
        m_parameter3 = AudioParam::create("unused", 0.0, 0.0, 1.0);
        break;

    default:
        ASSERT_NOT_REACHED();
    }

    if (autoInitialize)
        initialize();
}

BiquadProcessor::~BiquadProcessor()
{
    if (isInitialized())
        uninitialize();
}

PassOwnPtr<AudioDSPKernel> BiquadProcessor::createKernel()
{
    return adoptPtr(new BiquadDSPKernel(this));
}

void BiquadProcessor::checkForDirtyCoefficients()
{
    // Deal with smoothing / de-zippering. Start out assuming filter parameters are not changing.

    // The BiquadDSPKernel objects rely on this value to see if they need to re-compute their internal filter coefficients.
    m_filterCoefficientsDirty = false;
    
    if (m_hasJustReset) {
        // Snap to exact values first time after reset, then smooth for subsequent changes.
        m_parameter1->resetSmoothedValue();
        m_parameter2->resetSmoothedValue();
        m_parameter3->resetSmoothedValue();
        m_filterCoefficientsDirty = true;
        m_hasJustReset = false;
    } else {
        // Smooth all of the filter parameters. If they haven't yet converged to their target value then mark coefficients as dirty.
        bool isStable1 = m_parameter1->smooth();
        bool isStable2 = m_parameter2->smooth();
        bool isStable3 = m_parameter3->smooth();
        if (!(isStable1 && isStable2 && isStable3))
            m_filterCoefficientsDirty = true;
    }
}

void BiquadProcessor::process(const AudioBus* source, AudioBus* destination, size_t framesToProcess)
{
    if (!isInitialized()) {
        destination->zero();
        return;
    }
        
    checkForDirtyCoefficients();
            
    // For each channel of our input, process using the corresponding BiquadDSPKernel into the output channel.
    for (unsigned i = 0; i < m_kernels.size(); ++i)
        m_kernels[i]->process(source->channel(i)->data(), destination->channel(i)->mutableData(), framesToProcess);
}

void BiquadProcessor::setType(FilterType type)
{
    if (type != m_type) {
        m_type = type;
        reset(); // The filter state must be reset only if the type has changed.
    }
}

void BiquadProcessor::getFrequencyResponse(int nFrequencies,
                                           const float* frequencyHz,
                                           float* magResponse,
                                           float* phaseResponse)
{
    // Compute the frequency response on a separate temporary kernel
    // to avoid interfering with the processing running in the audio
    // thread on the main kernels.
    
    OwnPtr<BiquadDSPKernel> responseKernel = adoptPtr(new BiquadDSPKernel(this));

    responseKernel->getFrequencyResponse(nFrequencies, frequencyHz, magResponse, phaseResponse);
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
