/*
 * Copyright (C) 2020 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "RemoteAudioMediaStreamTrackRenderer.h"

#if PLATFORM(COCOA) && ENABLE(GPU_PROCESS) && ENABLE(MEDIA_STREAM)

#include "SharedRingBufferStorage.h"
#include <WebCore/AudioMediaStreamTrackRenderer.h>
#include <WebCore/CARingBuffer.h>
#include <WebCore/WebAudioBufferList.h>

namespace WebKit {
using namespace WebCore;

#if !RELEASE_LOG_DISABLED
static const void* nextLogIdentifier()
{
    static uint64_t logIdentifier = cryptographicallyRandomNumber();
    return reinterpret_cast<const void*>(++logIdentifier);
}

static RefPtr<Logger>& nullLogger()
{
    static NeverDestroyed<RefPtr<Logger>> logger;
    return logger;
}
#endif

RemoteAudioMediaStreamTrackRenderer::RemoteAudioMediaStreamTrackRenderer()
    : m_renderer(WebCore::AudioMediaStreamTrackRenderer::create())
    , m_ringBuffer(makeUnique<CARingBuffer>(makeUniqueRef<SharedRingBufferStorage>(nullptr)))
{
    ASSERT(m_renderer);

#if !RELEASE_LOG_DISABLED
    if (!nullLogger().get()) {
        nullLogger() = Logger::create(this);
        nullLogger()->setEnabled(this, false);
    }
    m_renderer->setLogger(*nullLogger(), nextLogIdentifier());
#endif
}

RemoteAudioMediaStreamTrackRenderer::~RemoteAudioMediaStreamTrackRenderer()
{
    m_renderer->clear();
}

SharedRingBufferStorage& RemoteAudioMediaStreamTrackRenderer::storage()
{
    return static_cast<SharedRingBufferStorage&>(m_ringBuffer->storage());
}

void RemoteAudioMediaStreamTrackRenderer::start()
{
    m_renderer->start();
}

void RemoteAudioMediaStreamTrackRenderer::stop()
{
    m_renderer->stop();
}

void RemoteAudioMediaStreamTrackRenderer::clear()
{
    m_renderer->clear();
}

void RemoteAudioMediaStreamTrackRenderer::setVolume(float value)
{
    m_renderer->setVolume(value);
}

void RemoteAudioMediaStreamTrackRenderer::audioSamplesStorageChanged(const SharedMemory::Handle& handle, const WebCore::CAAudioStreamDescription& description, uint64_t numberOfFrames)
{
    ASSERT(m_ringBuffer);
    if (!m_ringBuffer)
        return;

    m_description = description;

    if (handle.isNull()) {
        m_ringBuffer->deallocate();
        storage().setReadOnly(false);
        storage().setStorage(nullptr);
        return;
    }

    auto memory = SharedMemory::map(handle, SharedMemory::Protection::ReadOnly);
    storage().setStorage(WTFMove(memory));
    storage().setReadOnly(true);

    m_ringBuffer->allocate(description, numberOfFrames);
}

void RemoteAudioMediaStreamTrackRenderer::audioSamplesAvailable(MediaTime time, uint64_t numberOfFrames, uint64_t startFrame, uint64_t endFrame)
{
    ASSERT(m_ringBuffer);
    if (!m_ringBuffer)
        return;

    m_ringBuffer->setCurrentFrameBounds(startFrame, endFrame);

    WebAudioBufferList audioData(m_description, numberOfFrames);
    m_ringBuffer->fetch(audioData.list(), numberOfFrames, time.timeValue());

    m_renderer->pushSamples(time, audioData, m_description, numberOfFrames);
}

}

#endif // PLATFORM(COCOA) && ENABLE(GPU_PROCESS) && ENABLE(MEDIA_STREAM)
