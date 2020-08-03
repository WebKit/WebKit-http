/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2016 Apple Inc. All rights reserved.
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

#include "WebKitAudioContext.h"

#include "JSDOMPromiseDeferred.h"
#include "PeriodicWave.h"
#include "WebKitAudioPannerNode.h"
#include "WebKitOscillatorNode.h"
#include <wtf/IsoMallocInlines.h>

#if ENABLE(MEDIA_STREAM)
#include "MediaStream.h"
#include "MediaStreamAudioDestinationNode.h"
#include "MediaStreamAudioSource.h"
#include "MediaStreamAudioSourceNode.h"
#endif

#if ENABLE(VIDEO)
#include "HTMLMediaElement.h"
#include "MediaElementAudioSourceNode.h"
#endif

const unsigned MaxPeriodicWaveLength = 4096;

namespace WebCore {

WTF_MAKE_ISO_ALLOCATED_IMPL(WebKitAudioContext);

#define RELEASE_LOG_IF_ALLOWED(fmt, ...) RELEASE_LOG_IF(document() && document()->page() && document()->page()->isAlwaysOnLoggingAllowed(), Media, "%p - WebKitAudioContext::" fmt, this, ##__VA_ARGS__)

#if OS(WINDOWS)
// Don't allow more than this number of simultaneous AudioContexts talking to hardware.
constexpr unsigned maxHardwareContexts = 4;
#endif

ExceptionOr<Ref<WebKitAudioContext>> WebKitAudioContext::create(Document& document)
{
    ASSERT(isMainThread());
#if OS(WINDOWS)
    if (s_hardwareContextCount >= maxHardwareContexts)
        return Exception { QuotaExceededError };
#endif

    auto audioContext = adoptRef(*new WebKitAudioContext(document));
    audioContext->suspendIfNeeded();
    return audioContext;
}

// Constructor for rendering to the audio hardware.
WebKitAudioContext::WebKitAudioContext(Document& document)
    : BaseAudioContext(document)
{
}

// Constructor for offline (non-realtime) rendering.
WebKitAudioContext::WebKitAudioContext(Document& document, AudioBuffer* renderTarget)
    : BaseAudioContext(document, renderTarget)
{
}

const char* WebKitAudioContext::activeDOMObjectName() const
{
    return "WebKitAudioContext";
}

#if ENABLE(VIDEO)

ExceptionOr<Ref<MediaElementAudioSourceNode>> WebKitAudioContext::createMediaElementSource(HTMLMediaElement& mediaElement)
{
    ALWAYS_LOG(LOGIDENTIFIER);

    ASSERT(isMainThread());

    if (isStopped() || mediaElement.audioSourceNode())
        return Exception { InvalidStateError };

    lazyInitialize();

    auto node = MediaElementAudioSourceNode::create(*this, mediaElement);

    mediaElement.setAudioSourceNode(node.ptr());

    refNode(node.get()); // context keeps reference until node is disconnected
    return node;
}

#endif

#if ENABLE(MEDIA_STREAM)

ExceptionOr<Ref<MediaStreamAudioSourceNode>> WebKitAudioContext::createMediaStreamSource(MediaStream& mediaStream)
{
    ALWAYS_LOG(LOGIDENTIFIER);

    ASSERT(isMainThread());

    if (isStopped())
        return Exception { InvalidStateError };

    auto audioTracks = mediaStream.getAudioTracks();
    if (audioTracks.isEmpty())
        return Exception { InvalidStateError };

    MediaStreamTrack* providerTrack = nullptr;
    for (auto& track : audioTracks) {
        if (track->audioSourceProvider()) {
            providerTrack = track.get();
            break;
        }
    }
    if (!providerTrack)
        return Exception { InvalidStateError };

    lazyInitialize();

    auto node = MediaStreamAudioSourceNode::create(*this, mediaStream, *providerTrack);
    node->setFormat(2, sampleRate());

    refNode(node); // context keeps reference until node is disconnected
    return node;
}

ExceptionOr<Ref<MediaStreamAudioDestinationNode>> WebKitAudioContext::createMediaStreamDestination()
{
    if (isStopped())
        return Exception { InvalidStateError };

    // FIXME: Add support for an optional argument which specifies the number of channels.
    // FIXME: The default should probably be stereo instead of mono.
    return MediaStreamAudioDestinationNode::create(*this, 1);
}

#endif

ExceptionOr<Ref<WebKitAudioPannerNode>> WebKitAudioContext::createWebKitPanner()
{
    ALWAYS_LOG(LOGIDENTIFIER);

    ASSERT(isMainThread());
    if (isStopped())
        return Exception { InvalidStateError };

    lazyInitialize();
    return WebKitAudioPannerNode::create(*this, sampleRate());
}

ExceptionOr<Ref<WebKitOscillatorNode>> WebKitAudioContext::createWebKitOscillator()
{
    ALWAYS_LOG(LOGIDENTIFIER);

    ASSERT(isMainThread());
    if (isStopped())
        return Exception { InvalidStateError };

    lazyInitialize();

    auto node = WebKitOscillatorNode::create(*this);
    if (node.hasException())
        return node.releaseException();

    // Because this is an AudioScheduledSourceNode, the context keeps a reference until it has finished playing.
    // When this happens, AudioScheduledSourceNode::finish() calls BaseAudioContext::notifyNodeFinishedProcessing().
    auto nodeValue = node.releaseReturnValue();
    refNode(nodeValue);
    return nodeValue;
}

ExceptionOr<Ref<PeriodicWave>> WebKitAudioContext::createPeriodicWave(Float32Array& real, Float32Array& imaginary)
{
    ALWAYS_LOG(LOGIDENTIFIER);
    
    ASSERT(isMainThread());
    if (isStopped())
        return Exception { InvalidStateError };

    if (real.length() != imaginary.length() || real.length() > MaxPeriodicWaveLength || !real.length())
        return Exception { IndexSizeError };
    lazyInitialize();
    return PeriodicWave::create(sampleRate(), real, imaginary);
}

void WebKitAudioContext::close(DOMPromiseDeferred<void>&& promise)
{
    if (isOfflineContext() || isStopped()) {
        promise.reject(InvalidStateError);
        return;
    }

    if (state() == State::Closed || !destinationNode()) {
        promise.resolve();
        return;
    }

    addReaction(State::Closed, WTFMove(promise));

    lazyInitialize();

    destinationNode()->close([this, protectedThis = makeRef(*this)] {
        setState(State::Closed);
        uninitialize();
    });
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
