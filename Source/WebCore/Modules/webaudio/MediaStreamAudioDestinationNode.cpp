/*
 * Copyright (C) 2012, Google Inc. All rights reserved.
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

#if ENABLE(WEB_AUDIO) && ENABLE(MEDIA_STREAM)

#include "MediaStreamAudioDestinationNode.h"

#include "AudioContext.h"
#include "AudioNodeInput.h"
#include "MediaStream.h"
#include "MediaStreamAudioSource.h"
#include "RTCPeerConnectionHandler.h"
#include <wtf/Locker.h>

namespace WebCore {

PassRefPtr<MediaStreamAudioDestinationNode> MediaStreamAudioDestinationNode::create(AudioContext* context, size_t numberOfChannels)
{
    return adoptRef(new MediaStreamAudioDestinationNode(context, numberOfChannels));
}

MediaStreamAudioDestinationNode::MediaStreamAudioDestinationNode(AudioContext* context, size_t numberOfChannels)
    : AudioBasicInspectorNode(context, context->sampleRate(), numberOfChannels)
    , m_mixBus(AudioBus::create(numberOfChannels, ProcessingSizeInFrames))
{
    setNodeType(NodeTypeMediaStreamAudioDestination);

    m_source = MediaStreamAudioSource::create();
    Vector<RefPtr<MediaStreamSource>> audioSources;
    audioSources.append(m_source);
    m_stream = MediaStream::create(*context->scriptExecutionContext(), MediaStreamDescriptor::create(audioSources, Vector<RefPtr<MediaStreamSource>>()));

    m_source->setAudioFormat(numberOfChannels, context->sampleRate());

    initialize();
}

MediaStreamSource* MediaStreamAudioDestinationNode::mediaStreamSource()
{
    return m_source.get();
}

MediaStreamAudioDestinationNode::~MediaStreamAudioDestinationNode()
{
    uninitialize();
}

void MediaStreamAudioDestinationNode::process(size_t numberOfFrames)
{
    m_mixBus->copyFrom(*input(0)->bus());
    m_source->consumeAudio(m_mixBus.get(), numberOfFrames);
}

void MediaStreamAudioDestinationNode::reset()
{
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO) && ENABLE(MEDIA_STREAM)
