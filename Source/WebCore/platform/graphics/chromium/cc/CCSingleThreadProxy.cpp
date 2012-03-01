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

#include "cc/CCSingleThreadProxy.h"

#include "GraphicsContext3D.h"
#include "LayerRendererChromium.h"
#include "TraceEvent.h"
#include "cc/CCLayerTreeHost.h"
#include "cc/CCTextureUpdater.h"
#include <wtf/CurrentTime.h>

using namespace std;
using namespace WTF;

namespace WebCore {

PassOwnPtr<CCProxy> CCSingleThreadProxy::create(CCLayerTreeHost* layerTreeHost)
{
    return adoptPtr(new CCSingleThreadProxy(layerTreeHost));
}

CCSingleThreadProxy::CCSingleThreadProxy(CCLayerTreeHost* layerTreeHost)
    : m_layerTreeHost(layerTreeHost)
    , m_contextLost(false)
    , m_compositorIdentifier(-1)
    , m_layerRendererInitialized(false)
    , m_nextFrameIsNewlyCommittedFrame(false)
{
    TRACE_EVENT("CCSingleThreadProxy::CCSingleThreadProxy", this, 0);
    ASSERT(CCProxy::isMainThread());
}

void CCSingleThreadProxy::start()
{
    DebugScopedSetImplThread impl;
    m_layerTreeHostImpl = m_layerTreeHost->createLayerTreeHostImpl(this);
}

CCSingleThreadProxy::~CCSingleThreadProxy()
{
    TRACE_EVENT("CCSingleThreadProxy::~CCSingleThreadProxy", this, 0);
    ASSERT(CCProxy::isMainThread());
    ASSERT(!m_layerTreeHostImpl && !m_layerTreeHost); // make sure stop() got called.
}

bool CCSingleThreadProxy::compositeAndReadback(void *pixels, const IntRect& rect)
{
    TRACE_EVENT("CCSingleThreadProxy::compositeAndReadback", this, 0);
    ASSERT(CCProxy::isMainThread());

    if (!commitIfNeeded())
        return false;

    if (!doComposite())
        return false;

    m_layerTreeHostImpl->readback(pixels, rect);

    if (m_layerTreeHostImpl->isContextLost())
        return false;

    return true;
}

void CCSingleThreadProxy::startPageScaleAnimation(const IntSize& targetPosition, bool useAnchor, float scale, double durationSec)
{
    m_layerTreeHostImpl->startPageScaleAnimation(targetPosition, useAnchor, scale, monotonicallyIncreasingTime() * 1000.0, durationSec * 1000.0);
}

GraphicsContext3D* CCSingleThreadProxy::context()
{
    ASSERT(CCProxy::isMainThread());
    if (m_contextBeforeInitialization)
        return m_contextBeforeInitialization.get();
    DebugScopedSetImplThread impl;
    return m_layerTreeHostImpl->context();
}

void CCSingleThreadProxy::finishAllRendering()
{
    ASSERT(CCProxy::isMainThread());
    {
        DebugScopedSetImplThread impl;
        m_layerTreeHostImpl->finishAllRendering();
    }
}

bool CCSingleThreadProxy::isStarted() const
{
    ASSERT(CCProxy::isMainThread());
    return m_layerTreeHostImpl;
}

bool CCSingleThreadProxy::initializeContext()
{
    ASSERT(CCProxy::isMainThread());
    RefPtr<GraphicsContext3D> context = m_layerTreeHost->createContext();
    if (!context)
        return false;
    ASSERT(context->hasOneRef());
    m_contextBeforeInitialization = context;
    return true;
}

bool CCSingleThreadProxy::initializeLayerRenderer()
{
    ASSERT(CCProxy::isMainThread());
    ASSERT(m_contextBeforeInitialization);
    {
        DebugScopedSetImplThread impl;
        bool ok = m_layerTreeHostImpl->initializeLayerRenderer(m_contextBeforeInitialization.release());
        if (ok) {
            m_layerRendererInitialized = true;
            m_layerRendererCapabilitiesForMainThread = m_layerTreeHostImpl->layerRendererCapabilities();
        }
        return ok;
    }
}

bool CCSingleThreadProxy::recreateContext()
{
    TRACE_EVENT0("cc", "CCSingleThreadProxy::recreateContext");
    ASSERT(CCProxy::isMainThread());
    ASSERT(m_contextLost);

    RefPtr<GraphicsContext3D> context = m_layerTreeHost->createContext();
    if (!context)
        return false;

    ASSERT(context->hasOneRef());
    bool initialized;
    {
        DebugScopedSetImplThread impl;
        m_layerTreeHost->deleteContentsTexturesOnImplThread(m_layerTreeHostImpl->contentsTextureAllocator());
        initialized = m_layerTreeHostImpl->initializeLayerRenderer(context);
        if (initialized)
            m_layerRendererCapabilitiesForMainThread = m_layerTreeHostImpl->layerRendererCapabilities();
    }

    if (initialized)
        m_contextLost = false;

    return initialized;
}

const LayerRendererCapabilities& CCSingleThreadProxy::layerRendererCapabilities() const
{
    ASSERT(m_layerRendererInitialized);
    // Note: this gets called during the commit by the "impl" thread
    return m_layerRendererCapabilitiesForMainThread;
}

void CCSingleThreadProxy::loseContext()
{
    ASSERT(CCProxy::isMainThread());
    m_layerTreeHost->didLoseContext();
    m_contextLost = true;
}

void CCSingleThreadProxy::setNeedsAnimate()
{
    // CCThread-only feature
    ASSERT_NOT_REACHED();
}

void CCSingleThreadProxy::doCommit()
{
    ASSERT(CCProxy::isMainThread());
    // Commit immediately
    {
        DebugScopedSetImplThread impl;
        m_layerTreeHostImpl->beginCommit();

        m_layerTreeHost->beginCommitOnImplThread(m_layerTreeHostImpl.get());
        CCTextureUpdater updater(m_layerTreeHostImpl->contentsTextureAllocator());
        m_layerTreeHost->updateCompositorResources(m_layerTreeHostImpl->context(), updater);
        updater.update(m_layerTreeHostImpl->context(), numeric_limits<size_t>::max());
        ASSERT(!updater.hasMoreUpdates());
        m_layerTreeHostImpl->setVisible(m_layerTreeHost->visible());
        m_layerTreeHost->finishCommitOnImplThread(m_layerTreeHostImpl.get());

        m_layerTreeHostImpl->commitComplete();

#if !ASSERT_DISABLED
        // In the single-threaded case, the scroll deltas should never be
        // touched on the impl layer tree.
        OwnPtr<CCScrollAndScaleSet> scrollInfo = m_layerTreeHostImpl->processScrollDeltas();
        ASSERT(!scrollInfo->scrolls.size());
#endif
    }
    m_layerTreeHost->commitComplete();
    m_nextFrameIsNewlyCommittedFrame = true;
}

void CCSingleThreadProxy::setNeedsCommit()
{
    ASSERT(CCProxy::isMainThread());
    m_layerTreeHost->setNeedsCommit();
}

void CCSingleThreadProxy::setNeedsRedraw()
{
    // FIXME: Once we move render_widget scheduling into this class, we can
    // treat redraw requests more efficiently than commitAndRedraw requests.
    m_layerTreeHostImpl->setFullRootLayerDamage();
    setNeedsCommit();
}

void CCSingleThreadProxy::setVisible(bool visible)
{
    if (!visible) {
        DebugScopedSetImplThread impl;
        m_layerTreeHost->didBecomeInvisibleOnImplThread(m_layerTreeHostImpl.get());
        m_layerTreeHostImpl->setVisible(false);
        return;
    }
    setNeedsCommit();
}

void CCSingleThreadProxy::stop()
{
    TRACE_EVENT("CCSingleThreadProxy::stop", this, 0);
    ASSERT(CCProxy::isMainThread());
    {
        DebugScopedSetImplThread impl;
        m_layerTreeHost->deleteContentsTexturesOnImplThread(m_layerTreeHostImpl->contentsTextureAllocator());
        m_layerTreeHostImpl.clear();
    }
    m_layerTreeHost = 0;
}

void CCSingleThreadProxy::postAnimationEventsToMainThreadOnImplThread(PassOwnPtr<CCAnimationEventsVector> events)
{
    ASSERT(CCProxy::isImplThread());
    DebugScopedSetMainThread main;
    m_layerTreeHost->setAnimationEvents(events);
}

// Called by the legacy scheduling path (e.g. where render_widget does the scheduling)
void CCSingleThreadProxy::compositeImmediately()
{
    if (!commitIfNeeded())
        return;

    if (doComposite())
        m_layerTreeHostImpl->swapBuffers();
}

bool CCSingleThreadProxy::commitIfNeeded()
{
    ASSERT(CCProxy::isMainThread());

    if (!m_layerTreeHost->updateLayers())
        return false;

    doCommit();
    return true;
}

bool CCSingleThreadProxy::doComposite()
{
    ASSERT(!m_contextLost);
    {
      DebugScopedSetImplThread impl;
      double frameDisplayTimeMs = monotonicallyIncreasingTime() * 1000.0;
      m_layerTreeHostImpl->animate(frameDisplayTimeMs);
      m_layerTreeHostImpl->drawLayers();
    }

    if (m_layerTreeHostImpl->isContextLost()) {
        m_contextLost = true;
        m_layerTreeHost->didLoseContext();
        return false;
    }

    if (m_nextFrameIsNewlyCommittedFrame) {
        m_nextFrameIsNewlyCommittedFrame = false;
        m_layerTreeHost->didCommitAndDrawFrame();
    }

    return true;
}

}
