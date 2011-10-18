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
#include "TraceEvent.h"
#include "cc/CCInputHandler.h"
#include "cc/CCLayerTreeHost.h"
#include "cc/CCMainThreadTask.h"
#include "cc/CCScheduler.h"
#include "cc/CCScrollController.h"
#include "cc/CCThreadTask.h"
#include <wtf/CurrentTime.h>
#include <wtf/MainThread.h>

using namespace WTF;

namespace WebCore {

CCThread* CCThreadProxy::s_ccThread = 0;

void CCThreadProxy::setThread(CCThread* ccThread)
{
    s_ccThread = ccThread;
#ifndef NDEBUG
    CCProxy::setImplThread(s_ccThread->threadID());
#endif
}

class CCThreadProxySchedulerClient : public CCSchedulerClient {
public:
    static PassOwnPtr<CCThreadProxySchedulerClient> create(CCThreadProxy* proxy)
    {
        return adoptPtr(new CCThreadProxySchedulerClient(proxy));
    }
    virtual ~CCThreadProxySchedulerClient() { }

    virtual void scheduleBeginFrameAndCommit()
    {
        CCMainThread::postTask(m_proxy->createBeginFrameAndCommitTaskOnCCThread());
    }

    virtual void scheduleDrawAndPresent()
    {
        m_proxy->drawLayersAndPresentOnCCThread();
    }

private:
    explicit CCThreadProxySchedulerClient(CCThreadProxy* proxy)
    {
        m_proxy = proxy;
    }

    CCThreadProxy* m_proxy;
};

class CCThreadProxyScrollControllerAdapter : public CCScrollController {
public:
    static PassOwnPtr<CCThreadProxyScrollControllerAdapter> create(CCThreadProxy* proxy)
    {
        return adoptPtr(new CCThreadProxyScrollControllerAdapter(proxy));
    }
    virtual ~CCThreadProxyScrollControllerAdapter() { }

    virtual void scrollRootLayer(const IntSize& offset)
    {
        m_proxy->m_layerTreeHostImpl->scrollRootLayer(offset);
        m_proxy->setNeedsRedrawOnCCThread();
        m_proxy->setNeedsCommitOnCCThread();
    }

private:
    explicit CCThreadProxyScrollControllerAdapter(CCThreadProxy* proxy)
    {
        m_proxy = proxy;
    }

    CCThreadProxy* m_proxy;
};

PassOwnPtr<CCProxy> CCThreadProxy::create(CCLayerTreeHost* layerTreeHost)
{
    return adoptPtr(new CCThreadProxy(layerTreeHost));
}

CCThreadProxy::CCThreadProxy(CCLayerTreeHost* layerTreeHost)
    : m_commitRequested(false)
    , m_layerTreeHost(layerTreeHost)
    , m_compositorIdentifier(-1)
    , m_started(false)
    , m_lastExecutedBeginFrameAndCommitSequenceNumber(-1)
    , m_numBeginFrameAndCommitsIssuedOnCCThread(0)
{
    TRACE_EVENT("CCThreadProxy::CCThreadProxy", this, 0);
    ASSERT(isMainThread());
}

CCThreadProxy::~CCThreadProxy()
{
    TRACE_EVENT("CCThreadProxy::~CCThreadProxy", this, 0);
    ASSERT(isMainThread());
    ASSERT(!m_started);
}

bool CCThreadProxy::compositeAndReadback(void *pixels, const IntRect& rect)
{
    TRACE_EVENT("CCThreadPRoxy::compositeAndReadback", this, 0);
    ASSERT(isMainThread());
    ASSERT(m_layerTreeHost);

    // If a commit is pending, perform the commit first.
    if (m_commitRequested)  {
        // This bit of code is uglier than it should be because returning
        // pointers via the CCThread task model is really messy. Effectively, we
        // are making a blocking call to createBeginFrameAndCommitTaskOnCCThread,
        // and trying to get the CCMainThread::Task it returns so we can run it.
        OwnPtr<CCMainThread::Task> beginFrameAndCommitTask;
        {
            CCMainThread::Task* taskPtr = 0;
            CCCompletionEvent completion;
            s_ccThread->postTask(createCCThreadTask(this, &CCThreadProxy::obtainBeginFrameAndCommitTaskFromCCThread, AllowCrossThreadAccess(&completion), AllowCrossThreadAccess(&taskPtr)));
            completion.wait();
            beginFrameAndCommitTask = adoptPtr(taskPtr);
        }

        beginFrameAndCommitTask->performTask();
    }

    // Draw using the new tree and read back the results.
    bool success = false;
    CCCompletionEvent completion;
    s_ccThread->postTask(createCCThreadTask(this, &CCThreadProxy::drawLayersAndReadbackOnCCThread, AllowCrossThreadAccess(&completion), AllowCrossThreadAccess(&success), AllowCrossThreadAccess(pixels), rect));
    completion.wait();
    return success;
}

void CCThreadProxy::drawLayersAndReadbackOnCCThread(CCCompletionEvent* completion, bool* success, void* pixels, const IntRect& rect)
{
    ASSERT(CCProxy::isImplThread());
    if (!m_layerTreeHostImpl) {
        *success = false;
        completion->signal();
        return;
    }
    drawLayersOnCCThread();
    m_layerTreeHostImpl->readback(pixels, rect);
    *success = m_layerTreeHostImpl->isContextLost();
    completion->signal();
}

GraphicsContext3D* CCThreadProxy::context()
{
    return 0;
}

void CCThreadProxy::finishAllRendering()
{
    ASSERT(CCProxy::isMainThread());

    // Make sure all GL drawing is finished on the impl thread.
    CCCompletionEvent completion;
    s_ccThread->postTask(createCCThreadTask(this, &CCThreadProxy::finishAllRenderingOnCCThread, AllowCrossThreadAccess(&completion)));
    completion.wait();
}

bool CCThreadProxy::isStarted() const
{
    ASSERT(CCProxy::isMainThread());
    return m_started;
}

bool CCThreadProxy::initializeLayerRenderer()
{
    TRACE_EVENT("CCThreadProxy::initializeLayerRenderer", this, 0);
    RefPtr<GraphicsContext3D> context = m_layerTreeHost->createLayerTreeHostContext3D();
    if (!context)
        return false;
    ASSERT(context->hasOneRef());

    // Leak the context pointer so we can transfer ownership of it to the other side...
    GraphicsContext3D* contextPtr = context.release().leakRef();
    ASSERT(contextPtr->hasOneRef());

    // Make a blocking call to initializeLayerRendererOnCCThread. The results of that call
    // are pushed into the initializeSucceeded and capabilities local variables.
    CCCompletionEvent completion;
    bool initializeSucceeded = false;
    LayerRendererCapabilities capabilities;
    s_ccThread->postTask(createCCThreadTask(this, &CCThreadProxy::initializeLayerRendererOnCCThread,
                                          AllowCrossThreadAccess(contextPtr), AllowCrossThreadAccess(&completion),
                                          AllowCrossThreadAccess(&initializeSucceeded), AllowCrossThreadAccess(&capabilities),
                                          AllowCrossThreadAccess(&m_compositorIdentifier)));
    completion.wait();

    if (initializeSucceeded)
        m_layerRendererCapabilitiesMainThreadCopy = capabilities;
    return initializeSucceeded;
}

int CCThreadProxy::compositorIdentifier() const
{
    ASSERT(isMainThread());
    return m_compositorIdentifier;
}

const LayerRendererCapabilities& CCThreadProxy::layerRendererCapabilities() const
{
    return m_layerRendererCapabilitiesMainThreadCopy;
}

void CCThreadProxy::loseCompositorContext(int numTimes)
{
    ASSERT_NOT_REACHED();
}

void CCThreadProxy::setNeedsCommit()
{
    ASSERT(isMainThread());
    if (m_commitRequested)
        return;

    TRACE_EVENT("CCThreadProxy::setNeedsCommit", this, 0);
    m_commitRequested = true;
    s_ccThread->postTask(createCCThreadTask(this, &CCThreadProxy::setNeedsCommitOnCCThread));
}

void CCThreadProxy::setNeedsCommitThenRedraw()
{
    ASSERT(isMainThread());
    m_redrawAfterCommit = true;
    setNeedsCommit();
}

void CCThreadProxy::setNeedsCommitOnCCThread()
{
    ASSERT(isImplThread());
    TRACE_EVENT("CCThreadProxy::setNeedsCommitOnCCThread", this, 0);
    m_schedulerOnCCThread->requestCommit();
}

void CCThreadProxy::setNeedsRedraw()
{
    ASSERT(isMainThread());
    TRACE_EVENT("CCThreadProxy::setNeedsRedraw", this, 0);
    s_ccThread->postTask(createCCThreadTask(this, &CCThreadProxy::setNeedsRedrawOnCCThread));
}

void CCThreadProxy::setNeedsRedrawOnCCThread()
{
    ASSERT(isImplThread());
    TRACE_EVENT("CCThreadProxy::setNeedsRedrawOnCCThread", this, 0);
    m_schedulerOnCCThread->requestRedraw();
}

void CCThreadProxy::start()
{
    ASSERT(isMainThread());
    ASSERT(s_ccThread);
    // Create LayerTreeHostImpl.
    CCCompletionEvent completion;
    s_ccThread->postTask(createCCThreadTask(this, &CCThreadProxy::initializeImplOnCCThread, AllowCrossThreadAccess(&completion)));
    completion.wait();

    m_started = true;
}

void CCThreadProxy::stop()
{
    TRACE_EVENT("CCThreadProxy::stop", this, 0);
    ASSERT(isMainThread());
    ASSERT(m_started);

    // Synchronously deletes the impl.
    CCCompletionEvent completion;
    s_ccThread->postTask(createCCThreadTask(this, &CCThreadProxy::layerTreeHostClosedOnCCThread, AllowCrossThreadAccess(&completion)));
    completion.wait();

    ASSERT(!m_layerTreeHostImpl); // verify that the impl deleted.
    m_layerTreeHost = 0;
    m_started = false;
}

void CCThreadProxy::finishAllRenderingOnCCThread(CCCompletionEvent* completion)
{
    TRACE_EVENT("CCThreadProxy::finishAllRenderingOnCCThread", this, 0);
    ASSERT(isImplThread());
    if (m_schedulerOnCCThread->redrawPending()) {
        drawLayersOnCCThread();
        m_layerTreeHostImpl->present();
        m_schedulerOnCCThread->didDraw();
    }
    m_layerTreeHostImpl->finishAllRendering();
    completion->signal();
}

void CCThreadProxy::obtainBeginFrameAndCommitTaskFromCCThread(CCCompletionEvent* completion, CCMainThread::Task** taskPtr)
{
    OwnPtr<CCMainThread::Task> task = createBeginFrameAndCommitTaskOnCCThread();
    *taskPtr = task.leakPtr();
    completion->signal();
}

PassOwnPtr<CCMainThread::Task> CCThreadProxy::createBeginFrameAndCommitTaskOnCCThread()
{
    TRACE_EVENT("CCThreadProxy::createBeginFrameAndCommitTaskOnCCThread", this, 0);
    ASSERT(isImplThread());
    double frameBeginTime = currentTime();

    // NOTE, it is possible to receieve a request for a
    // beginFrameAndCommitOnCCThread from finishAllRendering while a
    // beginFrameAndCommitOnCCThread is enqueued. Since CCMainThread doesn't
    // provide a threadsafe way to cancel tasks, it is important that
    // beginFrameAndCommit be structured to understand that it may get called at
    // a point that it shouldn't. We do this by assigning a sequence number to
    // every new beginFrameAndCommit task. Then, beginFrameAndCommit tracks the
    // last executed sequence number, dropping beginFrameAndCommit with sequence
    // numbers below the last executed one.
    int thisTaskSequenceNumber = m_numBeginFrameAndCommitsIssuedOnCCThread;
    m_numBeginFrameAndCommitsIssuedOnCCThread++;
    OwnPtr<CCScrollUpdateSet> scrollInfo = m_layerTreeHostImpl->processScrollDeltas();
    return createMainThreadTask(this, &CCThreadProxy::beginFrameAndCommit, thisTaskSequenceNumber, frameBeginTime, scrollInfo.release());
}

void CCThreadProxy::beginFrameAndCommit(int sequenceNumber, double frameBeginTime, PassOwnPtr<CCScrollUpdateSet> scrollInfo)
{
    TRACE_EVENT("CCThreadProxy::beginFrameAndCommit", this, 0);
    ASSERT(isMainThread());
    if (!m_layerTreeHost)
        return;

    // Scroll deltas need to be applied even if the commit will be dropped.
    m_layerTreeHost->applyScrollDeltas(*scrollInfo.get());

    // Drop beginFrameAndCommit calls that occur out of sequence. See createBeginFrameAndCommitTaskOnCCThread for
    // an explanation of how out-of-sequence beginFrameAndCommit tasks can occur.
    if (sequenceNumber < m_lastExecutedBeginFrameAndCommitSequenceNumber) {
        TRACE_EVENT("EarlyOut_StaleBeginFrameAndCommit", this, 0);
        return;
    }
    m_lastExecutedBeginFrameAndCommitSequenceNumber = sequenceNumber;

    // FIXME: recreate the context if it was requested by the impl thread
    {
        TRACE_EVENT("CCLayerTreeHost::animateAndLayout", this, 0);
        m_layerTreeHost->animateAndLayout(frameBeginTime);
    }

    ASSERT(m_lastExecutedBeginFrameAndCommitSequenceNumber == sequenceNumber);

    // Clear the commit flag after animateAndLayout here --- objects that only
    // layout when painted will trigger another setNeedsCommit inside
    // updateLayers.
    m_commitRequested = false;

    m_layerTreeHost->updateLayers();

    {
        // Blocking call to CCThreadProxy::commitOnCCThread
        TRACE_EVENT("commit", this, 0);
        CCCompletionEvent completion;
        s_ccThread->postTask(createCCThreadTask(this, &CCThreadProxy::commitOnCCThread, AllowCrossThreadAccess(&completion)));
        completion.wait();
    }

    m_layerTreeHost->commitComplete();

    if (m_redrawAfterCommit)
        setNeedsRedraw();
    m_redrawAfterCommit = false;

    ASSERT(m_lastExecutedBeginFrameAndCommitSequenceNumber == sequenceNumber);
}

void CCThreadProxy::commitOnCCThread(CCCompletionEvent* completion)
{
    TRACE_EVENT("CCThreadProxy::beginFrameAndCommitOnCCThread", this, 0);
    ASSERT(isImplThread());
    ASSERT(m_schedulerOnCCThread->commitPending());
    if (!m_layerTreeHostImpl) {
        completion->signal();
        return;
    }
    m_layerTreeHostImpl->beginCommit();
    m_layerTreeHost->commitToOnCCThread(m_layerTreeHostImpl.get());
    m_layerTreeHostImpl->commitComplete();

    completion->signal();

    m_schedulerOnCCThread->didCommit();
}

void CCThreadProxy::drawLayersAndPresentOnCCThread()
{
    TRACE_EVENT("CCThreadProxy::drawLayersOnCCThread", this, 0);
    ASSERT(isImplThread());
    if (!m_layerTreeHostImpl)
        return;

    drawLayersOnCCThread();
    m_layerTreeHostImpl->present();
    m_schedulerOnCCThread->didDraw();
}

void CCThreadProxy::drawLayersOnCCThread()
{
    TRACE_EVENT("CCThreadProxy::drawLayersOnCCThread", this, 0);
    ASSERT(isImplThread());
    ASSERT(m_layerTreeHostImpl);

    m_layerTreeHostImpl->drawLayers();
    ASSERT(!m_layerTreeHostImpl->isContextLost());
}

void CCThreadProxy::initializeImplOnCCThread(CCCompletionEvent* completion)
{
    TRACE_EVENT("CCThreadProxy::initializeImplOnCCThread", this, 0);
    ASSERT(isImplThread());
    m_layerTreeHostImpl = m_layerTreeHost->createLayerTreeHostImpl();
    m_schedulerClientOnCCThread = CCThreadProxySchedulerClient::create(this);
    m_schedulerOnCCThread = CCScheduler::create(m_schedulerClientOnCCThread.get());
    completion->signal();
}

void CCThreadProxy::initializeLayerRendererOnCCThread(GraphicsContext3D* contextPtr, CCCompletionEvent* completion, bool* initializeSucceeded, LayerRendererCapabilities* capabilities, int* compositorIdentifier)
{
    TRACE_EVENT("CCThreadProxy::initializeLayerRendererOnCCThread", this, 0);
    ASSERT(isImplThread());
    RefPtr<GraphicsContext3D> context(adoptRef(contextPtr));
    *initializeSucceeded = m_layerTreeHostImpl->initializeLayerRenderer(context);
    if (*initializeSucceeded)
        *capabilities = m_layerTreeHostImpl->layerRendererCapabilities();

    m_scrollControllerAdapterOnCCThread = CCThreadProxyScrollControllerAdapter::create(this);
    m_inputHandlerOnCCThread = CCInputHandler::create(m_scrollControllerAdapterOnCCThread.get());
    *compositorIdentifier = m_inputHandlerOnCCThread->identifier();

    completion->signal();
}

void CCThreadProxy::layerTreeHostClosedOnCCThread(CCCompletionEvent* completion)
{
    TRACE_EVENT("CCThreadProxy::layerTreeHostClosedOnCCThread", this, 0);
    ASSERT(isImplThread());
    m_layerTreeHost->deleteContentsTexturesOnCCThread(m_layerTreeHostImpl->contentsTextureAllocator());
    m_layerTreeHostImpl.clear();
    m_inputHandlerOnCCThread.clear();
    m_scrollControllerAdapterOnCCThread.clear();
    completion->signal();
}

} // namespace WebCore
