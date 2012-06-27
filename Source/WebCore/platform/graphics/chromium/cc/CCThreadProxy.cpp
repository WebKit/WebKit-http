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

#include "config.h"

#include "cc/CCThreadProxy.h"

#include "GraphicsContext3D.h"
#include "SharedGraphicsContext3D.h"
#include "TraceEvent.h"
#include "cc/CCDelayBasedTimeSource.h"
#include "cc/CCDrawQuad.h"
#include "cc/CCFontAtlas.h"
#include "cc/CCFrameRateController.h"
#include "cc/CCGraphicsContext.h"
#include "cc/CCInputHandler.h"
#include "cc/CCLayerTreeHost.h"
#include "cc/CCRenderingStats.h"
#include "cc/CCScheduler.h"
#include "cc/CCScopedThreadProxy.h"
#include "cc/CCTextureUpdater.h"
#include "cc/CCThreadTask.h"
#include <wtf/CurrentTime.h>
#include <wtf/MainThread.h>

using namespace WTF;

namespace {

// Number of textures to update with each call to
// scheduledActionUpdateMoreResources().
static const size_t textureUpdatesPerFrame = 48;

// Measured in seconds.
static const double contextRecreationTickRate = 0.03;

} // anonymous namespace

namespace WebCore {

namespace {

// Type of texture uploader to use for texture updates.
static TextureUploaderOption textureUploader = ThrottledUploader;

} // anonymous namespace

PassOwnPtr<CCProxy> CCThreadProxy::create(CCLayerTreeHost* layerTreeHost)
{
    return adoptPtr(new CCThreadProxy(layerTreeHost));
}

CCThreadProxy::CCThreadProxy(CCLayerTreeHost* layerTreeHost)
    : m_animateRequested(false)
    , m_commitRequested(false)
    , m_forcedCommitRequested(false)
    , m_contextLost(false)
    , m_layerTreeHost(layerTreeHost)
    , m_compositorIdentifier(-1)
    , m_layerRendererInitialized(false)
    , m_started(false)
    , m_texturesAcquired(true)
    , m_inCompositeAndReadback(false)
    , m_mainThreadProxy(CCScopedThreadProxy::create(CCProxy::mainThread()))
    , m_beginFrameCompletionEventOnImplThread(0)
    , m_readbackRequestOnImplThread(0)
    , m_commitCompletionEventOnImplThread(0)
    , m_textureAcquisitionCompletionEventOnImplThread(0)
    , m_nextFrameIsNewlyCommittedFrameOnImplThread(false)
{
    TRACE_EVENT0("cc", "CCThreadProxy::CCThreadProxy");
    ASSERT(isMainThread());
}

CCThreadProxy::~CCThreadProxy()
{
    TRACE_EVENT0("cc", "CCThreadProxy::~CCThreadProxy");
    ASSERT(isMainThread());
    ASSERT(!m_started);
}

bool CCThreadProxy::compositeAndReadback(void *pixels, const IntRect& rect)
{
    TRACE_EVENT0("cc", "CCThreadPRoxy::compositeAndReadback");
    ASSERT(isMainThread());
    ASSERT(m_layerTreeHost);

    if (!m_layerTreeHost->initializeLayerRendererIfNeeded()) {
        TRACE_EVENT0("cc", "compositeAndReadback_EarlyOut_LR_Uninitialized");
        return false;
    }


    // Perform a synchronous commit.
    CCCompletionEvent beginFrameCompletion;
    CCProxy::implThread()->postTask(createCCThreadTask(this, &CCThreadProxy::forceBeginFrameOnImplThread, AllowCrossThreadAccess(&beginFrameCompletion)));
    beginFrameCompletion.wait();
    m_inCompositeAndReadback = true;
    beginFrame();
    m_inCompositeAndReadback = false;

    // Perform a synchronous readback.
    ReadbackRequest request;
    request.rect = rect;
    request.pixels = pixels;
    CCProxy::implThread()->postTask(createCCThreadTask(this, &CCThreadProxy::requestReadbackOnImplThread, AllowCrossThreadAccess(&request)));
    request.completion.wait();
    return request.success;
}

void CCThreadProxy::requestReadbackOnImplThread(ReadbackRequest* request)
{
    ASSERT(CCProxy::isImplThread());
    ASSERT(!m_readbackRequestOnImplThread);
    if (!m_layerTreeHostImpl) {
        request->success = false;
        request->completion.signal();
        return;
    }

    m_readbackRequestOnImplThread = request;
    m_schedulerOnImplThread->setNeedsRedraw();
    m_schedulerOnImplThread->setNeedsForcedRedraw();
}

void CCThreadProxy::startPageScaleAnimation(const IntSize& targetPosition, bool useAnchor, float scale, double duration)
{
    ASSERT(CCProxy::isMainThread());
    CCProxy::implThread()->postTask(createCCThreadTask(this, &CCThreadProxy::requestStartPageScaleAnimationOnImplThread, targetPosition, useAnchor, scale, duration));
}

void CCThreadProxy::requestStartPageScaleAnimationOnImplThread(IntSize targetPosition, bool useAnchor, float scale, double duration)
{
    ASSERT(CCProxy::isImplThread());
    if (m_layerTreeHostImpl)
        m_layerTreeHostImpl->startPageScaleAnimation(targetPosition, useAnchor, scale, monotonicallyIncreasingTime(), duration);
}

void CCThreadProxy::finishAllRendering()
{
    ASSERT(CCProxy::isMainThread());

    // Make sure all GL drawing is finished on the impl thread.
    CCCompletionEvent completion;
    CCProxy::implThread()->postTask(createCCThreadTask(this, &CCThreadProxy::finishAllRenderingOnImplThread, AllowCrossThreadAccess(&completion)));
    completion.wait();
}

bool CCThreadProxy::isStarted() const
{
    ASSERT(CCProxy::isMainThread());
    return m_started;
}

bool CCThreadProxy::initializeContext()
{
    TRACE_EVENT0("cc", "CCThreadProxy::initializeContext");
    OwnPtr<CCGraphicsContext> context = m_layerTreeHost->createContext();
    if (!context)
        return false;

    CCProxy::implThread()->postTask(createCCThreadTask(this, &CCThreadProxy::initializeContextOnImplThread,
                                                       AllowCrossThreadAccess(context.leakPtr())));
    return true;
}

void CCThreadProxy::setSurfaceReady()
{
    TRACE_EVENT0("cc", "CCThreadProxy::setSurfaceReady");
    CCProxy::implThread()->postTask(createCCThreadTask(this, &CCThreadProxy::setSurfaceReadyOnImplThread));
}

void CCThreadProxy::setSurfaceReadyOnImplThread()
{
    TRACE_EVENT0("cc", "CCThreadProxy::setSurfaceReadyOnImplThread");
    m_schedulerOnImplThread->setCanBeginFrame(true);
}

void CCThreadProxy::setVisible(bool visible)
{
    TRACE_EVENT0("cc", "CCThreadProxy::setVisible");
    CCCompletionEvent completion;
    CCProxy::implThread()->postTask(createCCThreadTask(this, &CCThreadProxy::setVisibleOnImplThread, AllowCrossThreadAccess(&completion), visible));
    completion.wait();
}

void CCThreadProxy::setVisibleOnImplThread(CCCompletionEvent* completion, bool visible)
{
    TRACE_EVENT0("cc", "CCThreadProxy::setVisibleOnImplThread");
    m_layerTreeHostImpl->setVisible(visible);
    m_schedulerOnImplThread->setVisible(visible);
    completion->signal();
}

bool CCThreadProxy::initializeLayerRenderer()
{
    TRACE_EVENT0("cc", "CCThreadProxy::initializeLayerRenderer");
    // Make a blocking call to initializeLayerRendererOnImplThread. The results of that call
    // are pushed into the initializeSucceeded and capabilities local variables.
    CCCompletionEvent completion;
    bool initializeSucceeded = false;
    LayerRendererCapabilities capabilities;
    CCProxy::implThread()->postTask(createCCThreadTask(this, &CCThreadProxy::initializeLayerRendererOnImplThread,
                                                       AllowCrossThreadAccess(&completion),
                                                       AllowCrossThreadAccess(&initializeSucceeded),
                                                       AllowCrossThreadAccess(&capabilities)));
    completion.wait();

    if (initializeSucceeded) {
        m_layerRendererInitialized = true;
        m_layerRendererCapabilitiesMainThreadCopy = capabilities;
    }
    return initializeSucceeded;
}

bool CCThreadProxy::recreateContext()
{
    TRACE_EVENT0("cc", "CCThreadProxy::recreateContext");
    ASSERT(isMainThread());

    // Try to create the context.
    OwnPtr<CCGraphicsContext> context = m_layerTreeHost->createContext();
    if (!context)
        return false;
    if (m_layerTreeHost->needsSharedContext() && !m_layerTreeHost->settings().forceSoftwareCompositing)
        if (!SharedGraphicsContext3D::createForImplThread())
            return false;

    // Make a blocking call to recreateContextOnImplThread. The results of that
    // call are pushed into the recreateSucceeded and capabilities local
    // variables.
    CCCompletionEvent completion;
    bool recreateSucceeded = false;
    LayerRendererCapabilities capabilities;
    CCProxy::implThread()->postTask(createCCThreadTask(this, &CCThreadProxy::recreateContextOnImplThread,
                                                       AllowCrossThreadAccess(&completion),
                                                       AllowCrossThreadAccess(context.leakPtr()),
                                                       AllowCrossThreadAccess(&recreateSucceeded),
                                                       AllowCrossThreadAccess(&capabilities)));
    completion.wait();

    if (recreateSucceeded)
        m_layerRendererCapabilitiesMainThreadCopy = capabilities;
    return recreateSucceeded;
}

int CCThreadProxy::compositorIdentifier() const
{
    ASSERT(isMainThread());
    return m_compositorIdentifier;
}

void CCThreadProxy::implSideRenderingStats(CCRenderingStats& stats)
{
    ASSERT(isMainThread());

    CCCompletionEvent completion;
    CCProxy::implThread()->postTask(createCCThreadTask(this, &CCThreadProxy::implSideRenderingStatsOnImplThread,
                                                       AllowCrossThreadAccess(&completion),
                                                       AllowCrossThreadAccess(&stats)));
    completion.wait();
}

const LayerRendererCapabilities& CCThreadProxy::layerRendererCapabilities() const
{
    ASSERT(m_layerRendererInitialized);
    return m_layerRendererCapabilitiesMainThreadCopy;
}

void CCThreadProxy::loseContext()
{
    CCProxy::implThread()->postTask(createCCThreadTask(this, &CCThreadProxy::didLoseContextOnImplThread));
}

void CCThreadProxy::setNeedsAnimate()
{
    ASSERT(isMainThread());
    if (m_animateRequested)
        return;

    TRACE_EVENT0("cc", "CCThreadProxy::setNeedsAnimate");
    m_animateRequested = true;
    setNeedsCommit();
}

void CCThreadProxy::setNeedsCommit()
{
    ASSERT(isMainThread());
    if (m_commitRequested)
        return;

    TRACE_EVENT0("cc", "CCThreadProxy::setNeedsCommit");
    m_commitRequested = true;
    CCProxy::implThread()->postTask(createCCThreadTask(this, &CCThreadProxy::setNeedsCommitOnImplThread));
}

void CCThreadProxy::didLoseContextOnImplThread()
{
    ASSERT(isImplThread());
    TRACE_EVENT0("cc", "CCThreadProxy::didLoseContextOnImplThread");
    m_schedulerOnImplThread->didLoseContext();
}

void CCThreadProxy::onSwapBuffersCompleteOnImplThread()
{
    ASSERT(isImplThread());
    TRACE_EVENT0("cc", "CCThreadProxy::onSwapBuffersCompleteOnImplThread");
    m_schedulerOnImplThread->didSwapBuffersComplete();
    m_mainThreadProxy->postTask(createCCThreadTask(this, &CCThreadProxy::didCompleteSwapBuffers));
}

void CCThreadProxy::setNeedsCommitOnImplThread()
{
    ASSERT(isImplThread());
    TRACE_EVENT0("cc", "CCThreadProxy::setNeedsCommitOnImplThread");
    m_schedulerOnImplThread->setNeedsCommit();
}

void CCThreadProxy::setNeedsForcedCommitOnImplThread()
{
    ASSERT(isImplThread());
    TRACE_EVENT0("cc", "CCThreadProxy::setNeedsForcedCommitOnImplThread");
    m_schedulerOnImplThread->setNeedsCommit();
    m_schedulerOnImplThread->setNeedsForcedCommit();
}

void CCThreadProxy::postAnimationEventsToMainThreadOnImplThread(PassOwnPtr<CCAnimationEventsVector> events, double wallClockTime)
{
    ASSERT(isImplThread());
    TRACE_EVENT0("cc", "CCThreadProxy::postAnimationEventsToMainThreadOnImplThread");
    m_mainThreadProxy->postTask(createCCThreadTask(this, &CCThreadProxy::setAnimationEvents, events, wallClockTime));
}

void CCThreadProxy::setNeedsRedraw()
{
    ASSERT(isMainThread());
    TRACE_EVENT0("cc", "CCThreadProxy::setNeedsRedraw");
    CCProxy::implThread()->postTask(createCCThreadTask(this, &CCThreadProxy::setFullRootLayerDamageOnImplThread));
    CCProxy::implThread()->postTask(createCCThreadTask(this, &CCThreadProxy::setNeedsRedrawOnImplThread));
}

bool CCThreadProxy::commitRequested() const
{
    ASSERT(isMainThread());
    return m_commitRequested;
}

void CCThreadProxy::setNeedsRedrawOnImplThread()
{
    ASSERT(isImplThread());
    TRACE_EVENT0("cc", "CCThreadProxy::setNeedsRedrawOnImplThread");
    m_schedulerOnImplThread->setNeedsRedraw();
}

void CCThreadProxy::start()
{
    ASSERT(isMainThread());
    ASSERT(CCProxy::implThread());
    // Create LayerTreeHostImpl.
    CCCompletionEvent completion;
    CCProxy::implThread()->postTask(createCCThreadTask(this, &CCThreadProxy::initializeImplOnImplThread, AllowCrossThreadAccess(&completion)));
    completion.wait();

    m_started = true;
}

void CCThreadProxy::stop()
{
    TRACE_EVENT0("cc", "CCThreadProxy::stop");
    ASSERT(isMainThread());
    ASSERT(m_started);

    // Synchronously deletes the impl.
    {
        DebugScopedSetMainThreadBlocked mainThreadBlocked;

        CCCompletionEvent completion;
        CCProxy::implThread()->postTask(createCCThreadTask(this, &CCThreadProxy::layerTreeHostClosedOnImplThread, AllowCrossThreadAccess(&completion)));
        completion.wait();
    }

    m_mainThreadProxy->shutdown(); // Stop running tasks posted to us.

    ASSERT(!m_layerTreeHostImpl); // verify that the impl deleted.
    m_layerTreeHost = 0;
    m_started = false;
}

void CCThreadProxy::forceSerializeOnSwapBuffers()
{
    CCCompletionEvent completion;
    CCProxy::implThread()->postTask(createCCThreadTask(this, &CCThreadProxy::forceSerializeOnSwapBuffersOnImplThread, AllowCrossThreadAccess(&completion)));
    completion.wait();
}

void CCThreadProxy::forceSerializeOnSwapBuffersOnImplThread(CCCompletionEvent* completion)
{
    if (m_layerRendererInitialized)
        m_layerTreeHostImpl->layerRenderer()->doNoOp();
    completion->signal();
}


void CCThreadProxy::finishAllRenderingOnImplThread(CCCompletionEvent* completion)
{
    TRACE_EVENT0("cc", "CCThreadProxy::finishAllRenderingOnImplThread");
    ASSERT(isImplThread());
    m_layerTreeHostImpl->finishAllRendering();
    completion->signal();
}

void CCThreadProxy::forceBeginFrameOnImplThread(CCCompletionEvent* completion)
{
    TRACE_EVENT0("cc", "CCThreadProxy::forceBeginFrameOnImplThread");
    ASSERT(!m_beginFrameCompletionEventOnImplThread);

    if (m_schedulerOnImplThread->commitPending()) {
        completion->signal();
        return;
    }

    m_beginFrameCompletionEventOnImplThread = completion;
    setNeedsForcedCommitOnImplThread();
}

void CCThreadProxy::scheduledActionBeginFrame()
{
    TRACE_EVENT0("cc", "CCThreadProxy::scheduledActionBeginFrame");
    ASSERT(!m_pendingBeginFrameRequest);
    m_pendingBeginFrameRequest = adoptPtr(new BeginFrameAndCommitState());
    m_pendingBeginFrameRequest->monotonicFrameBeginTime = monotonicallyIncreasingTime();
    m_pendingBeginFrameRequest->scrollInfo = m_layerTreeHostImpl->processScrollDeltas();
    m_currentTextureUpdaterOnImplThread = adoptPtr(new CCTextureUpdater);
    m_pendingBeginFrameRequest->updater = m_currentTextureUpdaterOnImplThread.get();
    m_pendingBeginFrameRequest->contentsTexturesWereDeleted = m_layerTreeHostImpl->contentsTexturesWerePurgedSinceLastCommit();
    m_pendingBeginFrameRequest->memoryAllocationLimitBytes = m_layerTreeHostImpl->memoryAllocationLimitBytes();

    m_mainThreadProxy->postTask(createCCThreadTask(this, &CCThreadProxy::beginFrame));

    if (m_beginFrameCompletionEventOnImplThread) {
        m_beginFrameCompletionEventOnImplThread->signal();
        m_beginFrameCompletionEventOnImplThread = 0;
    }
}

void CCThreadProxy::beginFrame()
{
    TRACE_EVENT0("cc", "CCThreadProxy::beginFrame");
    ASSERT(isMainThread());
    if (!m_layerTreeHost)
        return;

    if (!m_pendingBeginFrameRequest) {
        TRACE_EVENT0("cc", "EarlyOut_StaleBeginFrameMessage");
        return;
    }

    if (m_layerTreeHost->needsSharedContext() && !m_layerTreeHost->settings().forceSoftwareCompositing && !SharedGraphicsContext3D::haveForImplThread())
        SharedGraphicsContext3D::createForImplThread();

    OwnPtr<BeginFrameAndCommitState> request(m_pendingBeginFrameRequest.release());

    // Do not notify the impl thread of commit requests that occur during
    // the apply/animate/layout part of the beginFrameAndCommit process since
    // those commit requests will get painted immediately. Once we have done
    // the paint, m_commitRequested will be set to false to allow new commit
    // requests to be scheduled.
    m_commitRequested = true;

    // On the other hand, the animationRequested flag needs to be cleared
    // here so that any animation requests generated by the apply or animate
    // callbacks will trigger another frame.
    m_animateRequested = false;

    // FIXME: technically, scroll deltas need to be applied for dropped commits as well.
    // Re-do the commit flow so that we don't send the scrollInfo on the BFAC message.
    m_layerTreeHost->applyScrollAndScale(*request->scrollInfo);

    if (!m_inCompositeAndReadback && !m_layerTreeHost->visible()) {
        m_commitRequested = false;
        m_forcedCommitRequested = false;

        TRACE_EVENT0("cc", "EarlyOut_NotVisible");
        CCProxy::implThread()->postTask(createCCThreadTask(this, &CCThreadProxy::beginFrameAbortedOnImplThread));
        return;
    }

    m_layerTreeHost->willBeginFrame();

    m_layerTreeHost->updateAnimations(request->monotonicFrameBeginTime);
    m_layerTreeHost->layout();

    // Clear the commit flag after updating animations and layout here --- objects that only
    // layout when painted will trigger another setNeedsCommit inside
    // updateLayers.
    m_commitRequested = false;
    m_forcedCommitRequested = false;

    if (!m_layerTreeHost->initializeLayerRendererIfNeeded())
        return;

    if (request->contentsTexturesWereDeleted)
        m_layerTreeHost->evictAllContentTextures();

    m_layerTreeHost->updateLayers(*request->updater, request->memoryAllocationLimitBytes);

    // Once single buffered layers are committed, they cannot be modified until
    // they are drawn by the impl thread.
    m_texturesAcquired = false;

    m_layerTreeHost->willCommit();
    // Before applying scrolls and calling animate, we set m_animateRequested to false.
    // If it is true now, it means setNeedAnimate was called again. Call setNeedsCommit
    // now so that we get begin frame when this one is done.
    if (m_animateRequested)
        setNeedsCommit();

    // Notify the impl thread that the beginFrame has completed. This will
    // begin the commit process, which is blocking from the main thread's
    // point of view, but asynchronously performed on the impl thread,
    // coordinated by the CCScheduler.
    {
        TRACE_EVENT0("cc", "commit");
        DebugScopedSetMainThreadBlocked mainThreadBlocked;

        CCCompletionEvent completion;
        CCProxy::implThread()->postTask(createCCThreadTask(this, &CCThreadProxy::beginFrameCompleteOnImplThread, AllowCrossThreadAccess(&completion)));
        completion.wait();
    }

    m_layerTreeHost->commitComplete();
    m_layerTreeHost->didBeginFrame();
}

void CCThreadProxy::beginFrameCompleteOnImplThread(CCCompletionEvent* completion)
{
    TRACE_EVENT0("cc", "CCThreadProxy::beginFrameCompleteOnImplThread");
    ASSERT(!m_commitCompletionEventOnImplThread);
    ASSERT(isImplThread());
    ASSERT(m_schedulerOnImplThread);
    ASSERT(m_schedulerOnImplThread->commitPending());

    if (!m_layerTreeHostImpl) {
        completion->signal();
        return;
    }

    m_commitCompletionEventOnImplThread = completion;

    m_schedulerOnImplThread->beginFrameComplete();
}

void CCThreadProxy::beginFrameAbortedOnImplThread()
{
    TRACE_EVENT0("cc", "CCThreadProxy::beginFrameAbortedOnImplThread");
    ASSERT(isImplThread());
    ASSERT(m_schedulerOnImplThread);
    ASSERT(m_schedulerOnImplThread->commitPending());

    m_schedulerOnImplThread->beginFrameAborted();
}

bool CCThreadProxy::hasMoreResourceUpdates() const
{
    if (!m_currentTextureUpdaterOnImplThread)
        return false;
    return m_currentTextureUpdaterOnImplThread->hasMoreUpdates();
}

bool CCThreadProxy::canDraw()
{
    ASSERT(isImplThread());
    if (!m_layerTreeHostImpl)
        return false;
    return m_layerTreeHostImpl->canDraw();
}

void CCThreadProxy::scheduledActionUpdateMoreResources()
{
    TRACE_EVENT0("cc", "CCThreadProxy::scheduledActionUpdateMoreResources");
    ASSERT(m_currentTextureUpdaterOnImplThread);
    m_currentTextureUpdaterOnImplThread->update(m_layerTreeHostImpl->context(), m_layerTreeHostImpl->contentsTextureAllocator(), m_layerTreeHostImpl->layerRenderer()->textureCopier(), m_layerTreeHostImpl->layerRenderer()->textureUploader(), textureUpdatesPerFrame);
}

void CCThreadProxy::scheduledActionCommit()
{
    TRACE_EVENT0("cc", "CCThreadProxy::scheduledActionCommit");
    ASSERT(isImplThread());
    ASSERT(m_currentTextureUpdaterOnImplThread);
    ASSERT(!m_currentTextureUpdaterOnImplThread->hasMoreUpdates());
    ASSERT(m_commitCompletionEventOnImplThread);

    m_currentTextureUpdaterOnImplThread.clear();

    m_layerTreeHostImpl->beginCommit();

    m_layerTreeHost->beginCommitOnImplThread(m_layerTreeHostImpl.get());
    m_layerTreeHost->finishCommitOnImplThread(m_layerTreeHostImpl.get());

    m_layerTreeHostImpl->commitComplete();

    m_nextFrameIsNewlyCommittedFrameOnImplThread = true;

    m_commitCompletionEventOnImplThread->signal();
    m_commitCompletionEventOnImplThread = 0;

    // SetVisible kicks off the next scheduler action, so this must be last.
    m_schedulerOnImplThread->setVisible(m_layerTreeHostImpl->visible());
}

void CCThreadProxy::scheduledActionBeginContextRecreation()
{
    ASSERT(isImplThread());
    m_mainThreadProxy->postTask(createCCThreadTask(this, &CCThreadProxy::beginContextRecreation));
}

CCScheduledActionDrawAndSwapResult CCThreadProxy::scheduledActionDrawAndSwapInternal(bool forcedDraw)
{
    TRACE_EVENT0("cc", "CCThreadProxy::scheduledActionDrawAndSwap");
    CCScheduledActionDrawAndSwapResult result;
    result.didDraw = false;
    result.didSwap = false;
    ASSERT(isImplThread());
    ASSERT(m_layerTreeHostImpl);
    if (!m_layerTreeHostImpl)
        return result;

    ASSERT(m_layerTreeHostImpl->layerRenderer());
    if (!m_layerTreeHostImpl->layerRenderer())
        return result;

    // FIXME: compute the frame display time more intelligently
    double monotonicTime = monotonicallyIncreasingTime();
    double wallClockTime = currentTime();

    m_inputHandlerOnImplThread->animate(monotonicTime);
    m_layerTreeHostImpl->animate(monotonicTime, wallClockTime);

    // This method is called on a forced draw, regardless of whether we are able to produce a frame,
    // as the calling site on main thread is blocked until its request completes, and we signal
    // completion here. If canDraw() is false, we will indicate success=false to the caller, but we
    // must still signal completion to avoid deadlock.

    // We guard prepareToDraw() with canDraw() because it always returns a valid frame, so can only
    // be used when such a frame is possible. Since drawLayers() depends on the result of
    // prepareToDraw(), it is guarded on canDraw() as well.

    CCLayerTreeHostImpl::FrameData frame;
    bool drawFrame = m_layerTreeHostImpl->canDraw() && (m_layerTreeHostImpl->prepareToDraw(frame) || forcedDraw);
    if (drawFrame) {
        m_layerTreeHostImpl->drawLayers(frame);
        result.didDraw = true;
    }
    m_layerTreeHostImpl->didDrawAllLayers(frame);

    // Check for a pending compositeAndReadback.
    if (m_readbackRequestOnImplThread) {
        m_readbackRequestOnImplThread->success = false;
        if (drawFrame) {
            m_layerTreeHostImpl->readback(m_readbackRequestOnImplThread->pixels, m_readbackRequestOnImplThread->rect);
            m_readbackRequestOnImplThread->success = !m_layerTreeHostImpl->isContextLost();
        }
        m_readbackRequestOnImplThread->completion.signal();
        m_readbackRequestOnImplThread = 0;
    } else if (drawFrame)
        result.didSwap = m_layerTreeHostImpl->swapBuffers();

    // Tell the main thread that the the newly-commited frame was drawn.
    if (m_nextFrameIsNewlyCommittedFrameOnImplThread) {
        m_nextFrameIsNewlyCommittedFrameOnImplThread = false;
        m_mainThreadProxy->postTask(createCCThreadTask(this, &CCThreadProxy::didCommitAndDrawFrame));
    }

    return result;
}

void CCThreadProxy::acquireLayerTextures()
{
    // Called when the main thread needs to modify a layer texture that is used
    // directly by the compositor.
    // This method will block until the next compositor draw if there is a
    // previously committed frame that is still undrawn. This is necessary to
    // ensure that the main thread does not monopolize access to the textures.
    ASSERT(isMainThread());

    if (m_texturesAcquired)
        return;

    TRACE_EVENT0("cc", "CCThreadProxy::acquireLayerTextures");
    CCCompletionEvent completion;
    CCProxy::implThread()->postTask(createCCThreadTask(this, &CCThreadProxy::acquireLayerTexturesForMainThreadOnImplThread, AllowCrossThreadAccess(&completion)));
    completion.wait(); // Block until it is safe to write to layer textures from the main thread.

    m_texturesAcquired = true;
}

void CCThreadProxy::acquireLayerTexturesForMainThreadOnImplThread(CCCompletionEvent* completion)
{
    ASSERT(isImplThread());
    ASSERT(!m_textureAcquisitionCompletionEventOnImplThread);

    m_textureAcquisitionCompletionEventOnImplThread = completion;
    m_schedulerOnImplThread->setMainThreadNeedsLayerTextures();
}

void CCThreadProxy::scheduledActionAcquireLayerTexturesForMainThread()
{
    ASSERT(m_textureAcquisitionCompletionEventOnImplThread);
    m_textureAcquisitionCompletionEventOnImplThread->signal();
    m_textureAcquisitionCompletionEventOnImplThread = 0;
}

CCScheduledActionDrawAndSwapResult CCThreadProxy::scheduledActionDrawAndSwapIfPossible()
{
    return scheduledActionDrawAndSwapInternal(false);
}

CCScheduledActionDrawAndSwapResult CCThreadProxy::scheduledActionDrawAndSwapForced()
{
    return scheduledActionDrawAndSwapInternal(true);
}

void CCThreadProxy::didCommitAndDrawFrame()
{
    ASSERT(isMainThread());
    if (!m_layerTreeHost)
        return;
    m_layerTreeHost->didCommitAndDrawFrame();
}

void CCThreadProxy::didCompleteSwapBuffers()
{
    ASSERT(isMainThread());
    if (!m_layerTreeHost)
        return;
    m_layerTreeHost->didCompleteSwapBuffers();
}

void CCThreadProxy::setAnimationEvents(PassOwnPtr<CCAnimationEventsVector> events, double wallClockTime)
{
    TRACE_EVENT0("cc", "CCThreadProxy::setAnimationEvents");
    ASSERT(isMainThread());
    if (!m_layerTreeHost)
        return;
    m_layerTreeHost->setAnimationEvents(events, wallClockTime);
}

class CCThreadProxyContextRecreationTimer : public CCTimer, CCTimerClient {
public:
    static PassOwnPtr<CCThreadProxyContextRecreationTimer> create(CCThreadProxy* proxy) { return adoptPtr(new CCThreadProxyContextRecreationTimer(proxy)); }

    virtual void onTimerFired() OVERRIDE
    {
        m_proxy->tryToRecreateContext();
    }

private:
    explicit CCThreadProxyContextRecreationTimer(CCThreadProxy* proxy)
        : CCTimer(CCProxy::mainThread(), this)
        , m_proxy(proxy)
    {
    }

    CCThreadProxy* m_proxy;
};

void CCThreadProxy::beginContextRecreation()
{
    TRACE_EVENT0("cc", "CCThreadProxy::beginContextRecreation");
    ASSERT(isMainThread());
    ASSERT(!m_contextRecreationTimer);
    m_contextRecreationTimer = CCThreadProxyContextRecreationTimer::create(this);
    m_layerTreeHost->didLoseContext();
    m_contextRecreationTimer->startOneShot(contextRecreationTickRate);
}

void CCThreadProxy::tryToRecreateContext()
{
    ASSERT(isMainThread());
    ASSERT(m_layerTreeHost);
    CCLayerTreeHost::RecreateResult result = m_layerTreeHost->recreateContext();
    if (result == CCLayerTreeHost::RecreateFailedButTryAgain)
        m_contextRecreationTimer->startOneShot(contextRecreationTickRate);
    else if (result == CCLayerTreeHost::RecreateSucceeded)
        m_contextRecreationTimer.clear();
}

void CCThreadProxy::initializeImplOnImplThread(CCCompletionEvent* completion)
{
    TRACE_EVENT0("cc", "CCThreadProxy::initializeImplOnImplThread");
    ASSERT(isImplThread());
    m_layerTreeHostImpl = m_layerTreeHost->createLayerTreeHostImpl(this);
    const double displayRefreshInterval = 1.0 / 60.0;
    OwnPtr<CCFrameRateController> frameRateController = adoptPtr(new CCFrameRateController(CCDelayBasedTimeSource::create(displayRefreshInterval, CCProxy::implThread())));
    m_schedulerOnImplThread = CCScheduler::create(this, frameRateController.release());
    m_schedulerOnImplThread->setVisible(m_layerTreeHostImpl->visible());

    m_inputHandlerOnImplThread = CCInputHandler::create(m_layerTreeHostImpl.get());
    m_compositorIdentifier = m_inputHandlerOnImplThread->identifier();

    completion->signal();
}

void CCThreadProxy::initializeContextOnImplThread(CCGraphicsContext* context)
{
    TRACE_EVENT0("cc", "CCThreadProxy::initializeContextOnImplThread");
    ASSERT(isImplThread());
    m_contextBeforeInitializationOnImplThread = adoptPtr(context);
}

void CCThreadProxy::initializeLayerRendererOnImplThread(CCCompletionEvent* completion, bool* initializeSucceeded, LayerRendererCapabilities* capabilities)
{
    TRACE_EVENT0("cc", "CCThreadProxy::initializeLayerRendererOnImplThread");
    ASSERT(isImplThread());
    ASSERT(m_contextBeforeInitializationOnImplThread);
    *initializeSucceeded = m_layerTreeHostImpl->initializeLayerRenderer(m_contextBeforeInitializationOnImplThread.release(), textureUploader);
    if (*initializeSucceeded) {
        *capabilities = m_layerTreeHostImpl->layerRendererCapabilities();
        if (capabilities->usingSwapCompleteCallback)
            m_schedulerOnImplThread->setMaxFramesPending(2);
    }

    completion->signal();
}

void CCThreadProxy::layerTreeHostClosedOnImplThread(CCCompletionEvent* completion)
{
    TRACE_EVENT0("cc", "CCThreadProxy::layerTreeHostClosedOnImplThread");
    ASSERT(isImplThread());
    if (!m_layerTreeHostImpl->contentsTexturesWerePurgedSinceLastCommit())
        m_layerTreeHost->deleteContentsTexturesOnImplThread(m_layerTreeHostImpl->contentsTextureAllocator());
    m_inputHandlerOnImplThread.clear();
    m_layerTreeHostImpl.clear();
    m_schedulerOnImplThread.clear();
    completion->signal();
}

void CCThreadProxy::setFullRootLayerDamageOnImplThread()
{
    ASSERT(isImplThread());
    m_layerTreeHostImpl->setFullRootLayerDamage();
}

size_t CCThreadProxy::maxPartialTextureUpdates() const
{
    return textureUpdatesPerFrame;
}

void CCThreadProxy::setFontAtlas(PassOwnPtr<CCFontAtlas> fontAtlas)
{
    ASSERT(isMainThread());
    CCProxy::implThread()->postTask(createCCThreadTask(this, &CCThreadProxy::setFontAtlasOnImplThread, fontAtlas));

}

void CCThreadProxy::setFontAtlasOnImplThread(PassOwnPtr<CCFontAtlas> fontAtlas)
{
    ASSERT(isImplThread());
    m_layerTreeHostImpl->setFontAtlas(fontAtlas);
}

void CCThreadProxy::recreateContextOnImplThread(CCCompletionEvent* completion, CCGraphicsContext* contextPtr, bool* recreateSucceeded, LayerRendererCapabilities* capabilities)
{
    TRACE_EVENT0("cc", "CCThreadProxy::recreateContextOnImplThread");
    ASSERT(isImplThread());
    m_layerTreeHost->deleteContentsTexturesOnImplThread(m_layerTreeHostImpl->contentsTextureAllocator());
    *recreateSucceeded = m_layerTreeHostImpl->initializeLayerRenderer(adoptPtr(contextPtr), textureUploader);
    if (*recreateSucceeded) {
        *capabilities = m_layerTreeHostImpl->layerRendererCapabilities();
        m_schedulerOnImplThread->didRecreateContext();
    }
    completion->signal();
}

void CCThreadProxy::implSideRenderingStatsOnImplThread(CCCompletionEvent* completion, CCRenderingStats* stats)
{
    ASSERT(isImplThread());
    stats->numFramesSentToScreen = m_layerTreeHostImpl->sourceAnimationFrameNumber();
    completion->signal();
}

} // namespace WebCore
