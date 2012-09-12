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

#include "CCAnimationEvents.h"
#include "CCLayerTreeHostImpl.h"
#include "CCProxy.h"
#include <limits>
#include <wtf/OwnPtr.h>

namespace WebCore {

class CCLayerTreeHost;

class CCSingleThreadProxy : public CCProxy, CCLayerTreeHostImplClient {
public:
    static PassOwnPtr<CCProxy> create(CCLayerTreeHost*);
    virtual ~CCSingleThreadProxy();

    // CCProxy implementation
    virtual bool compositeAndReadback(void *pixels, const IntRect&) OVERRIDE;
    virtual void startPageScaleAnimation(const IntSize& targetPosition, bool useAnchor, float scale, double duration) OVERRIDE;
    virtual void finishAllRendering() OVERRIDE;
    virtual bool isStarted() const OVERRIDE;
    virtual bool initializeContext() OVERRIDE;
    virtual void setSurfaceReady() OVERRIDE;
    virtual void setVisible(bool) OVERRIDE;
    virtual bool initializeRenderer() OVERRIDE;
    virtual bool recreateContext() OVERRIDE;
    virtual void implSideRenderingStats(CCRenderingStats&) OVERRIDE;
    virtual const RendererCapabilities& rendererCapabilities() const OVERRIDE;
    virtual void loseContext() OVERRIDE;
    virtual void setNeedsAnimate() OVERRIDE;
    virtual void setNeedsCommit() OVERRIDE;
    virtual void setNeedsRedraw() OVERRIDE;
    virtual bool commitRequested() const OVERRIDE;
    virtual void didAddAnimation() OVERRIDE;
    virtual void start() OVERRIDE;
    virtual void stop() OVERRIDE;
    virtual size_t maxPartialTextureUpdates() const OVERRIDE { return std::numeric_limits<size_t>::max(); }
    virtual void acquireLayerTextures() OVERRIDE { }
    virtual void forceSerializeOnSwapBuffers() OVERRIDE;

    // CCLayerTreeHostImplClient implementation
    virtual void didLoseContextOnImplThread() OVERRIDE { }
    virtual void onSwapBuffersCompleteOnImplThread() OVERRIDE { ASSERT_NOT_REACHED(); }
    virtual void onVSyncParametersChanged(double monotonicTimebase, double intervalInSeconds) OVERRIDE { }
    virtual void onCanDrawStateChanged(bool canDraw) OVERRIDE { }
    virtual void setNeedsRedrawOnImplThread() OVERRIDE { m_layerTreeHost->scheduleComposite(); }
    virtual void setNeedsCommitOnImplThread() OVERRIDE { m_layerTreeHost->scheduleComposite(); }
    virtual void postAnimationEventsToMainThreadOnImplThread(PassOwnPtr<CCAnimationEventsVector>, double wallClockTime) OVERRIDE;
    virtual void releaseContentsTexturesOnImplThread() OVERRIDE;

    // Called by the legacy path where RenderWidget does the scheduling.
    void compositeImmediately();

private:
    explicit CCSingleThreadProxy(CCLayerTreeHost*);

    bool commitAndComposite();
    void doCommit(CCTextureUpdateQueue&);
    bool doComposite();
    void didSwapFrame();

    // Accessed on main thread only.
    CCLayerTreeHost* m_layerTreeHost;
    bool m_contextLost;

    // Holds on to the context between initializeContext() and initializeRenderer() calls. Shouldn't
    // be used for anything else.
    OwnPtr<CCGraphicsContext> m_contextBeforeInitialization;

    // Used on the CCThread, but checked on main thread during initialization/shutdown.
    OwnPtr<CCLayerTreeHostImpl> m_layerTreeHostImpl;
    bool m_rendererInitialized;
    RendererCapabilities m_RendererCapabilitiesForMainThread;

    bool m_nextFrameIsNewlyCommittedFrame;
};

// For use in the single-threaded case. In debug builds, it pretends that the
// code is running on the impl thread to satisfy assertion checks.
class DebugScopedSetImplThread {
public:
    DebugScopedSetImplThread()
    {
#if !ASSERT_DISABLED
        CCProxy::setCurrentThreadIsImplThread(true);
#endif
    }
    ~DebugScopedSetImplThread()
    {
#if !ASSERT_DISABLED
        CCProxy::setCurrentThreadIsImplThread(false);
#endif
    }
};

// For use in the single-threaded case. In debug builds, it pretends that the
// code is running on the main thread to satisfy assertion checks.
class DebugScopedSetMainThread {
public:
    DebugScopedSetMainThread()
    {
#if !ASSERT_DISABLED
        CCProxy::setCurrentThreadIsImplThread(false);
#endif
    }
    ~DebugScopedSetMainThread()
    {
#if !ASSERT_DISABLED
        CCProxy::setCurrentThreadIsImplThread(true);
#endif
    }
};

// For use in the single-threaded case. In debug builds, it pretends that the
// code is running on the impl thread and that the main thread is blocked to
// satisfy assertion checks
class DebugScopedSetImplThreadAndMainThreadBlocked {
private:
    DebugScopedSetImplThread m_implThread;
    DebugScopedSetMainThreadBlocked m_mainThreadBlocked;
};

} // namespace WebCore

#endif
