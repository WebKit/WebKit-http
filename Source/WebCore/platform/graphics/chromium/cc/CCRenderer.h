/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef CCRenderer_h
#define CCRenderer_h

#include "FloatQuad.h"
#include "IntRect.h"
#include "cc/CCLayerTreeHost.h"
#include <wtf/Noncopyable.h>
#include <wtf/PassRefPtr.h>

namespace WebCore {

class CCRenderPass;
class TextureAllocator;
class TextureCopier;
class TextureManager;
class TextureUploader;

class CCRendererClient {
public:
    virtual const IntSize& deviceViewportSize() const = 0;
    virtual const CCLayerTreeSettings& settings() const = 0;
    virtual void didLoseContext() = 0;
    virtual void onSwapBuffersComplete() = 0;
    virtual void setFullRootLayerDamage() = 0;
    virtual void setContentsMemoryAllocationLimitBytes(size_t) = 0;
};

class CCRenderer {
    WTF_MAKE_NONCOPYABLE(CCRenderer);
public:
    virtual ~CCRenderer() { }

    virtual const LayerRendererCapabilities& capabilities() const = 0;

    const CCLayerTreeSettings& settings() const { return m_client->settings(); }

    const IntSize& viewportSize() { return m_client->deviceViewportSize(); }
    int viewportWidth() { return viewportSize().width(); }
    int viewportHeight() { return viewportSize().height(); }

    virtual void viewportChanged() = 0;

    const WebKit::WebTransformationMatrix& projectionMatrix() const { return m_projectionMatrix; }
    const WebKit::WebTransformationMatrix& windowMatrix() const { return m_windowMatrix; }

    virtual void beginDrawingFrame(const CCRenderPass* defaultRenderPass) = 0;
    virtual void drawRenderPass(const CCRenderPass*, const FloatRect& rootScissorRectInCurrentPass) = 0;
    virtual void finishDrawingFrame() = 0;

    virtual void drawHeadsUpDisplay(ManagedTexture*, const IntSize& hudSize) = 0;

    // waits for rendering to finish
    virtual void finish() = 0;

    virtual void doNoOp() = 0;
    // puts backbuffer onscreen
    virtual bool swapBuffers(const IntRect& subBuffer) = 0;

    virtual void getFramebufferPixels(void *pixels, const IntRect&) = 0;

    virtual TextureManager* implTextureManager() const = 0;
    virtual TextureCopier* textureCopier() const = 0;
    virtual TextureUploader* textureUploader() const = 0;
    virtual TextureAllocator* implTextureAllocator() const = 0;
    virtual TextureAllocator* contentsTextureAllocator() const = 0;

    virtual void setScissorToRect(const IntRect&) = 0;

    virtual bool isContextLost() = 0;

    virtual void setVisible(bool) = 0;

protected:
    explicit CCRenderer(CCRendererClient* client)
        : m_client(client)
    {
    }

    CCRendererClient* m_client;
    WebKit::WebTransformationMatrix m_projectionMatrix;
    WebKit::WebTransformationMatrix m_windowMatrix;
};

}

#endif // CCRenderer_h
