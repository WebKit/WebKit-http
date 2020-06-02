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

#pragma once

#if PLATFORM(COCOA) && ENABLE(GPU_PROCESS) && ENABLE(VIDEO_TRACK) && ENABLE(MEDIA_STREAM)

#include "MessageReceiver.h"
#include "SharedMemory.h"
#include <WebCore/CAAudioStreamDescription.h>
#include <wtf/MediaTime.h>

namespace WebCore {
class AudioMediaStreamTrackRenderer;
class CARingBuffer;
}

namespace WebKit {

class GPUConnectionToWebProcess;
class SharedRingBufferStorage;

class RemoteAudioMediaStreamTrackRenderer final : private IPC::MessageReceiver {
    WTF_MAKE_FAST_ALLOCATED;
public:
    RemoteAudioMediaStreamTrackRenderer();
    ~RemoteAudioMediaStreamTrackRenderer();

    void didReceiveMessage(IPC::Connection&, IPC::Decoder&) final;

private:
    // IPC::MessageReceiver
    void start();
    void stop();
    void clear();
    void setVolume(float);
    void audioSamplesStorageChanged(const SharedMemory::Handle&, const WebCore::CAAudioStreamDescription&, uint64_t numberOfFrames);
    void audioSamplesAvailable(MediaTime, uint64_t numberOfFrames, uint64_t startFrame, uint64_t endFrame);

    SharedRingBufferStorage& storage();

    std::unique_ptr<WebCore::AudioMediaStreamTrackRenderer> m_renderer;

    WebCore::CAAudioStreamDescription m_description;
    std::unique_ptr<WebCore::CARingBuffer> m_ringBuffer;
};

}

#endif // PLATFORM(COCOA) && ENABLE(GPU_PROCESS) && ENABLE(VIDEO_TRACK) && ENABLE(MEDIA_STREAM)
