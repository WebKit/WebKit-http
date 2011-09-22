/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
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

#ifndef CCThreadProxy_h
#define CCThreadProxy_h

#include "cc/CCCompletionEvent.h"
#include "cc/CCLayerTreeHostImpl.h"
#include "cc/CCProxy.h"
#include <wtf/OwnPtr.h>

namespace WebCore {

class CCLayerTreeHost;

class CCThreadProxy : public CCProxy {
public:
    static PassOwnPtr<CCProxy> create(CCLayerTreeHost*);

    virtual ~CCThreadProxy();

    // CCProxy implementation
    virtual bool compositeAndReadback(void *pixels, const IntRect&);
    virtual GraphicsContext3D* context();
    virtual void finishAllRendering();
    virtual bool isStarted() const;
    virtual bool initializeLayerRenderer();
    virtual const LayerRendererCapabilities& layerRendererCapabilities() const;
    virtual void loseCompositorContext(int numTimes);
    virtual void setNeedsCommit();
    virtual void setNeedsCommitAndRedraw();
    virtual void setNeedsRedraw();
    virtual void start();
    virtual void stop();

private:
    explicit CCThreadProxy(CCLayerTreeHost*);

    // Called on CCMainThread
    void beginFrameAndCommit(double frameBeginTime);

    // Called on CCThread
    void beginFrameAndCommitOnCCThread();
    void commitOnCCThread(CCCompletionEvent*);
    void drawLayersOnCCThread();
    void initializeImplOnCCThread(CCCompletionEvent*);
    void initializeLayerRendererOnCCThread(GraphicsContext3D*, CCCompletionEvent*, bool* initializeSucceeded, LayerRendererCapabilities*);
    void setNeedsCommitOnCCThread();
    void setNeedsCommitAndRedrawOnCCThread();
    void setNeedsRedrawOnCCThread();
    void layerTreeHostClosedOnCCThread(CCCompletionEvent*);

    // Used on main-thread only.
    bool m_commitPending;

    // Accessed on main thread only.
    CCLayerTreeHost* m_layerTreeHost;
    LayerRendererCapabilities m_layerRendererCapabilitiesMainThreadCopy;

    // Used on the CCThread, but checked on main thread during initialization/shutdown.
    OwnPtr<CCLayerTreeHostImpl> m_layerTreeHostImpl;
};

}

#endif
