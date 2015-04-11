/*
 * Copyright (C) 2015 Igalia S.L.
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

#ifndef TextureMapperPlatformLayerProxy_h
#define TextureMapperPlatformLayerProxy_h

#include "TextureMapper.h"
#include "TextureMapperPlatformLayerBuffer.h"
#include "TransformationMatrix.h"
#include <wtf/RunLoop.h>
#ifndef NDEBUG
#include <wtf/Threading.h>
#endif
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/ThreadingPrimitives.h>
#include <wtf/Vector.h>

namespace WebCore {

class TextureMapperLayer;
class TextureMapperPlatformLayerProxy;

class TextureMapperPlatformLayerProxyProvider {
public:
    virtual RefPtr<TextureMapperPlatformLayerProxy> proxy() const = 0;
};

class TextureMapperPlatformLayerProxy : public ThreadSafeRefCounted<TextureMapperPlatformLayerProxy> {
    WTF_MAKE_FAST_ALLOCATED();
public:
    class Compositor {
    public:
        virtual void onNewBufferAvailable() = 0;
    };

    TextureMapperPlatformLayerProxy();
    virtual ~TextureMapperPlatformLayerProxy();

    void pushNextBuffer(std::unique_ptr<TextureMapperPlatformLayerBuffer>);
    std::unique_ptr<TextureMapperPlatformLayerBuffer> getAvailableBuffer(const IntSize&, GC3Dint internalFormat = GraphicsContext3D::DONT_CARE);

    void setCompositor(Compositor*);
    void setTargetLayer(TextureMapperLayer*);

    void swapBuffer();

private:
    void scheduleReleaseUnusedBuffers();
    void releaseUnusedBuffersTimerFired();

    Compositor* m_compositor;
    TextureMapperLayer* m_targetLayer;

    std::unique_ptr<TextureMapperPlatformLayerBuffer> m_currentBuffer;
    std::unique_ptr<TextureMapperPlatformLayerBuffer> m_pendingBuffer;

    Mutex m_pushMutex;
    ThreadCondition m_pushCondition;

    Vector<std::unique_ptr<TextureMapperPlatformLayerBuffer>> m_usedBuffers;

    RunLoop& m_runLoop;
    RunLoop::Timer<TextureMapperPlatformLayerProxy> m_releaseUnusedBuffersTimer;
#ifndef NDEBUG
    ThreadIdentifier m_compositorThreadID;
#endif
};

};

#endif // TextureMapperPlatformLayerProxy_h
