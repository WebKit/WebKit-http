/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "AudioParamTimeline.h"

#include "AudioUtilities.h"
#include "FloatConversion.h"
#include <algorithm>
#include <wtf/MathExtras.h>

using namespace std;

namespace WebCore {

void AudioParamTimeline::setValueAtTime(float value, float time)
{
    insertEvent(ParamEvent(ParamEvent::SetValue, value, time, 0, 0, 0));
}

void AudioParamTimeline::linearRampToValueAtTime(float value, float time)
{
    insertEvent(ParamEvent(ParamEvent::LinearRampToValue, value, time, 0, 0, 0));
}

void AudioParamTimeline::exponentialRampToValueAtTime(float value, float time)
{
    insertEvent(ParamEvent(ParamEvent::ExponentialRampToValue, value, time, 0, 0, 0));
}

void AudioParamTimeline::setTargetValueAtTime(float targetValue, float time, float timeConstant)
{
    insertEvent(ParamEvent(ParamEvent::SetTargetValue, targetValue, time, timeConstant, 0, 0));
}

void AudioParamTimeline::setValueCurveAtTime(Float32Array* curve, float time, float duration)
{
    insertEvent(ParamEvent(ParamEvent::SetValueCurve, 0, time, 0, duration, curve));
}

static bool isValidNumber(float x)
{
    return !isnan(x) && !isinf(x);
}

void AudioParamTimeline::insertEvent(const ParamEvent& event)
{
    // Sanity check the event. Be super careful we're not getting infected with NaN or Inf.
    bool isValid = event.type() < ParamEvent::LastType
        && isValidNumber(event.value())
        && isValidNumber(event.time())
        && isValidNumber(event.timeConstant())
        && isValidNumber(event.duration())
        && event.duration() >= 0;

    ASSERT(isValid);
    if (!isValid)
        return;
        
    MutexLocker locker(m_eventsLock);
    
    unsigned i = 0;
    float insertTime = event.time();
    for (i = 0; i < m_events.size(); ++i) {
        // Overwrite same event type and time.
        if (m_events[i].time() == insertTime && m_events[i].type() == event.type()) {
            m_events[i] = event;
            return;
        }

        if (m_events[i].time() > insertTime)
            break;
    }

    m_events.insert(i, event);
}

void AudioParamTimeline::cancelScheduledValues(float startTime)
{
    MutexLocker locker(m_eventsLock);

    // Remove all events starting at startTime.
    for (unsigned i = 0; i < m_events.size(); ++i) {
        if (m_events[i].time() >= startTime) {
            m_events.remove(i, m_events.size() - i);
            break;
        }
    }
}

float AudioParamTimeline::valueForContextTime(AudioContext* context, float defaultValue, bool& hasValue)
{
    ASSERT(context);

    if (!context || !m_events.size() || context->currentTime() < m_events[0].time()) {
        hasValue = false;
        return defaultValue;
    }

    // Ask for just a single value.
    float value;
    float sampleRate = context->sampleRate();
    float startTime = narrowPrecisionToFloat(context->currentTime());
    float endTime = startTime + 1.1f / sampleRate; // time just beyond one sample-frame
    float controlRate = sampleRate / AudioNode::ProcessingSizeInFrames; // one parameter change per render quantum
    value = valuesForTimeRange(startTime, endTime, defaultValue, &value, 1, sampleRate, controlRate);

    hasValue = true;
    return value;
}

float AudioParamTimeline::valuesForTimeRange(float startTime,
                                             float endTime,
                                             float defaultValue,
                                             float* values,
                                             unsigned numberOfValues,
                                             float sampleRate,
                                             float controlRate)
{
    // We can't contend the lock in the realtime audio thread.
    if (!m_eventsLock.tryLock()) {
        if (values) {
            for (unsigned i = 0; i < numberOfValues; ++i)
                values[i] = defaultValue;
        }
        return defaultValue;
    }

    float value = valuesForTimeRangeImpl(startTime, endTime, defaultValue, values, numberOfValues, sampleRate, controlRate);
    m_eventsLock.unlock();

    return value;
}

// Returns the rounded down integer sample-frame for the time and sample-rate.
static unsigned timeToSampleFrame(double time, float sampleRate)
{
    double k = 0.5 / sampleRate;
    return static_cast<unsigned>((time + k) * sampleRate);
}

float AudioParamTimeline::valuesForTimeRangeImpl(float startTime,
                                                 float endTime,
                                                 float defaultValue,
                                                 float* values,
                                                 unsigned numberOfValues,
                                                 float sampleRate,
                                                 float controlRate)
{
    ASSERT(values);
    if (!values)
        return defaultValue;

    // Return default value if there are no events matching the desired time range.
    ASSERT(m_events.size());
    if (!m_events.size() || endTime <= m_events[0].time()) {
        for (unsigned i = 0; i < numberOfValues; ++i)
            values[i] = defaultValue;
        return defaultValue;
    }

    // Maintain a running time and index for writing the values buffer.
    float currentTime = startTime;
    unsigned writeIndex = 0;

    // If first event is after startTime then fill initial part of values buffer with defaultValue
    // until we reach the first event time.
    float firstEventTime = m_events[0].time();
    if (firstEventTime > startTime) {
        float fillToTime = min(endTime, firstEventTime);
        unsigned fillToFrame = timeToSampleFrame(fillToTime - startTime, sampleRate);
        fillToFrame = min(fillToFrame, numberOfValues);
        for (; writeIndex < fillToFrame; ++writeIndex)
            values[writeIndex] = defaultValue;

        currentTime = fillToTime;
    }

    float value = defaultValue;

    // Go through each event and render the value buffer where the times overlap,
    // stopping when we've rendered all the requested values.
    // FIXME: could try to optimize by avoiding having to iterate starting from the very first event
    // and keeping track of a "current" event index.
    int n = m_events.size();
    for (int i = 0; i < n && writeIndex < numberOfValues; ++i) {
        ParamEvent& event = m_events[i];
        ParamEvent* nextEvent = i < n - 1 ? &(m_events[i + 1]) : 0;

        // Wait until we get a more recent event.
        if (nextEvent && nextEvent->time() < currentTime)
            continue;

        float value1 = event.value();
        float time1 = event.time();
        float value2 = nextEvent ? nextEvent->value() : value1;
        float time2 = nextEvent ? nextEvent->time() : endTime + 1;

        float deltaTime = time2 - time1;
        float k = deltaTime > 0 ? 1 / deltaTime : 0;
        float sampleFrameTimeIncr = 1 / sampleRate;

        float fillToTime = min(endTime, time2);
        unsigned fillToFrame = timeToSampleFrame(fillToTime - startTime, sampleRate);
        fillToFrame = min(fillToFrame, numberOfValues);

        ParamEvent::Type nextEventType = nextEvent ? static_cast<ParamEvent::Type>(nextEvent->type()) : ParamEvent::LastType /* unknown */;

        // First handle linear and exponential ramps which require looking ahead to the next event.
        if (nextEventType == ParamEvent::LinearRampToValue) {
            for (; writeIndex < fillToFrame; ++writeIndex) {
                float x = (currentTime - time1) * k;
                value = (1 - x) * value1 + x * value2;
                values[writeIndex] = value;
                currentTime += sampleFrameTimeIncr;
            }
        } else if (nextEventType == ParamEvent::ExponentialRampToValue) {
            if (value1 <= 0 || value2 <= 0) {
                // Handle negative values error case by propagating previous value.
                for (; writeIndex < fillToFrame; ++writeIndex)
                    values[writeIndex] = value;
            } else {
                // Interpolate in log space.
                value1 = log2f(value1);
                value2 = log2f(value2);

                // FIXME: optimize to not use pow() in inner loop, this is just a simple exponential ramp.
                for (; writeIndex < fillToFrame; ++writeIndex) {
                    float x = (currentTime - time1) * k;
                    value = (1 - x) * value1 + x * value2;
                    value = powf(2.0f, value);
                    values[writeIndex] = value;
                    currentTime += sampleFrameTimeIncr;
                }
            }
        } else {
            // Handle event types not requiring looking ahead to the next event.
            switch (event.type()) {
            case ParamEvent::SetValue:
            case ParamEvent::LinearRampToValue:
            case ParamEvent::ExponentialRampToValue:
                {
                    currentTime = fillToTime;

                    // Simply stay at a constant value.
                    value = event.value();
                    for (; writeIndex < fillToFrame; ++writeIndex)
                        values[writeIndex] = value;

                    break;
                }

            case ParamEvent::SetTargetValue:
                {
                    currentTime = fillToTime;

                    // Exponential approach to target value with given time constant.
                    float targetValue = event.value();
                    float timeConstant = event.timeConstant();
                    float discreteTimeConstant = AudioUtilities::discreteTimeConstantForSampleRate(timeConstant, controlRate);

                    for (; writeIndex < fillToFrame; ++writeIndex) {
                        values[writeIndex] = value;
                        value += (targetValue - value) * discreteTimeConstant;
                    }

                    break;
                }

            case ParamEvent::SetValueCurve:
                {
                    Float32Array* curve = event.curve();
                    float* curveData = curve ? curve->data() : 0;
                    unsigned numberOfCurvePoints = curve ? curve->length() : 0;

                    // Curve events have duration, so don't just use next event time.
                    float duration = event.duration();
                    float durationFrames = duration * sampleRate;
                    float curvePointsPerFrame = static_cast<float>(numberOfCurvePoints) / durationFrames;

                    if (!curve || !curveData || !numberOfCurvePoints || duration <= 0 || sampleRate <= 0) {
                        // Error condition - simply propagate previous value.
                        currentTime = fillToTime;
                        for (; writeIndex < fillToFrame; ++writeIndex)
                            values[writeIndex] = value;
                        break;
                    }

                    // Save old values and recalculate information based on the curve's duration
                    // instead of the next event time.
                    unsigned nextEventFillToFrame = fillToFrame;
                    float nextEventFillToTime = fillToTime;
                    fillToTime = min(endTime, time1 + duration);
                    fillToFrame = timeToSampleFrame(fillToTime - startTime, sampleRate);
                    fillToFrame = min(fillToFrame, numberOfValues);

                    // Index into the curve data using a floating-point value.
                    // We're scaling the number of curve points by the duration (see curvePointsPerFrame).
                    float curveVirtualIndex = 0;
                    if (time1 < currentTime) {
                        // Index somewhere in the middle of the curve data.
                        // Don't use timeToSampleFrame() since we want the exact floating-point frame.
                        float frameOffset = (currentTime - time1) * sampleRate;
                        curveVirtualIndex = curvePointsPerFrame * frameOffset;
                    }

                    // Render the stretched curve data using nearest neighbor sampling.
                    // Oversampled curve data can be provided if smoothness is desired.
                    for (; writeIndex < fillToFrame; ++writeIndex) {
                        unsigned curveIndex = static_cast<unsigned>(curveVirtualIndex);
                        curveVirtualIndex += curvePointsPerFrame;

                        // Bounds check.
                        if (curveIndex < numberOfCurvePoints)
                            value = curveData[curveIndex];

                        values[writeIndex] = value;
                    }

                    // If there's any time left after the duration of this event and the start
                    // of the next, then just propagate the last value.
                    for (; writeIndex < nextEventFillToFrame; ++writeIndex)
                        values[writeIndex] = value;

                    // Re-adjust current time
                    currentTime = nextEventFillToTime;

                    break;
                }
            }
        }
    }

    // If there's any time left after processing the last event then just propagate the last value
    // to the end of the values buffer.
    for (; writeIndex < numberOfValues; ++writeIndex)
        values[writeIndex] = value;

    return value;
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
