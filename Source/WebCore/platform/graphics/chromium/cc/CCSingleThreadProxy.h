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

#ifndef CCSingleThreadProxy_h
#define CCSingleThreadProxy_h

#include "cc/CCCompletionEvent.h"
#include "cc/CCLayerTreeHostImpl.h"
#include "cc/CCProxy.h"
#include <wtf/OwnPtr.h>

namespace WebCore {

class CCLayerTreeHost;

class CCSingleThreadProxy : public CCProxy {
public:
    static PassOwnPtr<CCProxy> create(CCLayerTreeHost*);
    virtual ~CCSingleThreadProxy();

    // CCProxy implementation
    virtual bool compositeAndReadback(void *pixels, const IntRect&);
    virtual GraphicsContext3D* context();
    virtual void finishAllRendering();
    virtual bool isStarted() const;
    virtual bool initializeLayerRenderer();
    virtual int compositorIdentifier() const { return m_compositorIdentifier; }
    virtual const LayerRendererCapabilities& layerRendererCapabilities() const;
    virtual void loseCompositorContext(int numTimes);
    virtual void setNeedsCommit();
    virtual void setNeedsCommitThenRedraw();
    virtual void setNeedsRedraw();
    virtual void start();
    virtual void stop();

    // Called by the legacy path where RenderWidget does the scheduling.
    void compositeImmediately();

private:
    explicit CCSingleThreadProxy(CCLayerTreeHost*);
    bool recreateContextIfNeeded();
    void commitIfNeeded();
    bool doComposite();

    // Accessed on main thread only.
    CCLayerTreeHost* m_layerTreeHost;
    int m_compositorIdentifier;

    // Used on the CCThread, but checked on main thread during initialization/shutdown.
    OwnPtr<CCLayerTreeHostImpl> m_layerTreeHostImpl;
    LayerRendererCapabilities m_layerRendererCapabilitiesForMainThread;

    int m_numFailedRecreateAttempts;
    bool m_graphicsContextLost;
    int m_timesRecreateShouldFail; // Used during testing.
};

// For use in the single-threaded case. In debug builds, it pretends that the
// code is running on the thread to satisfy assertion checks.
class DebugScopedSetImplThread {
public:
    DebugScopedSetImplThread()
    {
#if !ASSERT_DISABLED
        CCProxy::setImplThread(true);
#endif
    }
    ~DebugScopedSetImplThread()
    {
#if !ASSERT_DISABLED
        CCProxy::setImplThread(false);
#endif
    }
};

} // namespace WebCore

#endif
