/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
#include "DrawingAreaProxyImpl.h"

#include "DrawingAreaMessages.h"
#include "DrawingAreaProxyMessages.h"
#include "LayerTreeContext.h"
#include "UpdateInfo.h"
#include "WebPageProxy.h"
#include "WebProcessProxy.h"
#include <WebCore/Region.h>

using namespace WebCore;

namespace WebKit {

PassOwnPtr<DrawingAreaProxyImpl> DrawingAreaProxyImpl::create(WebPageProxy* webPageProxy)
{
    return adoptPtr(new DrawingAreaProxyImpl(webPageProxy));
}

DrawingAreaProxyImpl::DrawingAreaProxyImpl(WebPageProxy* webPageProxy)
    : DrawingAreaProxy(DrawingAreaTypeImpl, webPageProxy)
    , m_currentBackingStoreStateID(0)
    , m_nextBackingStoreStateID(0)
    , m_isWaitingForDidUpdateBackingStoreState(false)
    , m_hasReceivedFirstUpdate(false)
    , m_isBackingStoreDiscardable(true)
    , m_discardBackingStoreTimer(RunLoop::current(), this, &DrawingAreaProxyImpl::discardBackingStore)
{
}

DrawingAreaProxyImpl::~DrawingAreaProxyImpl()
{
#if USE(ACCELERATED_COMPOSITING)
    // Make sure to exit accelerated compositing mode.
    if (isInAcceleratedCompositingMode())
        exitAcceleratedCompositingMode();
#endif
}

void DrawingAreaProxyImpl::paint(BackingStore::PlatformGraphicsContext context, const IntRect& rect, Region& unpaintedRegion)
{
    unpaintedRegion = rect;

    if (isInAcceleratedCompositingMode())
        return;

    ASSERT(m_currentBackingStoreStateID <= m_nextBackingStoreStateID);
    if (m_currentBackingStoreStateID < m_nextBackingStoreStateID) {
        // Tell the web process to do a full backing store update now, in case we previously told
        // it about our next state but didn't request an immediate update.
        sendUpdateBackingStoreState(RespondImmediately);

        // If we haven't yet received our first bits from the WebProcess then don't paint anything.
        if (!m_hasReceivedFirstUpdate)
            return;        
        
        if (m_isWaitingForDidUpdateBackingStoreState) {
            // Wait for a DidUpdateBackingStoreState message that contains the new bits before we paint
            // what's currently in the backing store.
            waitForAndDispatchDidUpdateBackingStoreState();
        }

        // Dispatching DidUpdateBackingStoreState (either beneath sendUpdateBackingStoreState or
        // beneath waitForAndDispatchDidUpdateBackingStoreState) could destroy our backing store or
        // change the compositing mode.
        if (!m_backingStore || isInAcceleratedCompositingMode())
            return;
    } else {
        ASSERT(!m_isWaitingForDidUpdateBackingStoreState);
        if (!m_backingStore) {
            // The view has asked us to paint before the web process has painted anything. There's
            // nothing we can do.
            return;
        }
    }

    m_backingStore->paint(context, rect);
    unpaintedRegion.subtract(IntRect(IntPoint(), m_backingStore->size()));

    discardBackingStoreSoon();
}

bool DrawingAreaProxyImpl::paint(const WebCore::IntRect&, PlatformDrawingContext)
{
    ASSERT_NOT_REACHED();
    return false;
}

void DrawingAreaProxyImpl::sizeDidChange()
{
    backingStoreStateDidChange(RespondImmediately);
}

void DrawingAreaProxyImpl::visibilityDidChange()
{
    if (!m_webPageProxy->isViewVisible()) {
        // Suspend painting.
        m_webPageProxy->process()->send(Messages::DrawingArea::SuspendPainting(), m_webPageProxy->pageID());
        return;
    }

    // Resume painting.
    m_webPageProxy->process()->send(Messages::DrawingArea::ResumePainting(), m_webPageProxy->pageID());

#if USE(ACCELERATED_COMPOSITING)
    // If we don't have a backing store, go ahead and mark the backing store as being changed so
    // that when paint we'll actually wait for something to paint and not flash white.
    if (!m_backingStore && m_layerTreeContext.isEmpty())
        backingStoreStateDidChange(DoNotRespondImmediately);
#endif
}

void DrawingAreaProxyImpl::setPageIsVisible(bool)
{
}

void DrawingAreaProxyImpl::setBackingStoreIsDiscardable(bool isBackingStoreDiscardable)
{
    if (m_isBackingStoreDiscardable == isBackingStoreDiscardable)
        return;

    m_isBackingStoreDiscardable = isBackingStoreDiscardable;
    if (m_isBackingStoreDiscardable)
        discardBackingStoreSoon();
    else
        m_discardBackingStoreTimer.stop();
}

void DrawingAreaProxyImpl::waitForBackingStoreUpdateOnNextPaint()
{
    m_hasReceivedFirstUpdate = true;
}

void DrawingAreaProxyImpl::update(uint64_t backingStoreStateID, const UpdateInfo& updateInfo)
{
    ASSERT_ARG(backingStoreStateID, backingStoreStateID <= m_currentBackingStoreStateID);
    if (backingStoreStateID < m_currentBackingStoreStateID)
        return;

    // FIXME: Handle the case where the view is hidden.

    incorporateUpdate(updateInfo);
    m_webPageProxy->process()->send(Messages::DrawingArea::DidUpdate(), m_webPageProxy->pageID());
}

void DrawingAreaProxyImpl::didUpdateBackingStoreState(uint64_t backingStoreStateID, const UpdateInfo& updateInfo, const LayerTreeContext& layerTreeContext)
{
    ASSERT_ARG(backingStoreStateID, backingStoreStateID <= m_nextBackingStoreStateID);
    ASSERT_ARG(backingStoreStateID, backingStoreStateID > m_currentBackingStoreStateID);
    m_currentBackingStoreStateID = backingStoreStateID;

    m_isWaitingForDidUpdateBackingStoreState = false;

    // Stop the responsiveness timer that was started in sendUpdateBackingStoreState.
    m_webPageProxy->process()->responsivenessTimer()->stop();

    if (m_nextBackingStoreStateID != m_currentBackingStoreStateID)
        sendUpdateBackingStoreState(RespondImmediately);
    else
        m_hasReceivedFirstUpdate = true;

#if USE(ACCELERATED_COMPOSITING)
    if (layerTreeContext != m_layerTreeContext) {
        if (!m_layerTreeContext.isEmpty()) {
            exitAcceleratedCompositingMode();
            ASSERT(m_layerTreeContext.isEmpty());
        }

        if (!layerTreeContext.isEmpty()) {
            enterAcceleratedCompositingMode(layerTreeContext);
            ASSERT(layerTreeContext == m_layerTreeContext);
        }            
    }

    if (isInAcceleratedCompositingMode()) {
        ASSERT(!m_backingStore);
        return;
    }
#endif

    // If we have a backing store the right size, reuse it.
    if (m_backingStore && (m_backingStore->size() != updateInfo.viewSize || m_backingStore->scaleFactor() != updateInfo.scaleFactor))
        m_backingStore = nullptr;
    incorporateUpdate(updateInfo);
}

void DrawingAreaProxyImpl::enterAcceleratedCompositingMode(uint64_t backingStoreStateID, const LayerTreeContext& layerTreeContext)
{
    ASSERT_ARG(backingStoreStateID, backingStoreStateID <= m_currentBackingStoreStateID);
    if (backingStoreStateID < m_currentBackingStoreStateID)
        return;

#if USE(ACCELERATED_COMPOSITING)
    enterAcceleratedCompositingMode(layerTreeContext);
#endif
}

void DrawingAreaProxyImpl::exitAcceleratedCompositingMode(uint64_t backingStoreStateID, const UpdateInfo& updateInfo)
{
    ASSERT_ARG(backingStoreStateID, backingStoreStateID <= m_currentBackingStoreStateID);
    if (backingStoreStateID < m_currentBackingStoreStateID)
        return;

#if USE(ACCELERATED_COMPOSITING)
    exitAcceleratedCompositingMode();
#endif

    incorporateUpdate(updateInfo);
}

void DrawingAreaProxyImpl::incorporateUpdate(const UpdateInfo& updateInfo)
{
    ASSERT(!isInAcceleratedCompositingMode());

    if (updateInfo.updateRectBounds.isEmpty())
        return;

    if (!m_backingStore)
        m_backingStore = BackingStore::create(updateInfo.viewSize, updateInfo.scaleFactor, m_webPageProxy);

    m_backingStore->incorporateUpdate(updateInfo);

    bool shouldScroll = !updateInfo.scrollRect.isEmpty();

    if (shouldScroll)
        m_webPageProxy->scrollView(updateInfo.scrollRect, updateInfo.scrollOffset);

    for (size_t i = 0; i < updateInfo.updateRects.size(); ++i)
        m_webPageProxy->setViewNeedsDisplay(updateInfo.updateRects[i]);

    if (WebPageProxy::debugPaintFlags() & kWKDebugFlashBackingStoreUpdates)
        m_webPageProxy->flashBackingStoreUpdates(updateInfo.updateRects);

    if (shouldScroll)
        m_webPageProxy->displayView();
}

void DrawingAreaProxyImpl::backingStoreStateDidChange(RespondImmediatelyOrNot respondImmediatelyOrNot)
{
    ++m_nextBackingStoreStateID;
    sendUpdateBackingStoreState(respondImmediatelyOrNot);
}

void DrawingAreaProxyImpl::sendUpdateBackingStoreState(RespondImmediatelyOrNot respondImmediatelyOrNot)
{
    ASSERT(m_currentBackingStoreStateID < m_nextBackingStoreStateID);

    if (!m_webPageProxy->isValid())
        return;

    if (m_isWaitingForDidUpdateBackingStoreState)
        return;

    if (m_webPageProxy->viewSize().isEmpty())
        return;

    m_isWaitingForDidUpdateBackingStoreState = respondImmediatelyOrNot == RespondImmediately;

    m_webPageProxy->process()->send(Messages::DrawingArea::UpdateBackingStoreState(m_nextBackingStoreStateID, respondImmediatelyOrNot == RespondImmediately, m_size, m_scrollOffset), m_webPageProxy->pageID());
    m_scrollOffset = IntSize();

    if (m_isWaitingForDidUpdateBackingStoreState) {
        // Start the responsiveness timer. We will stop it when we hear back from the WebProcess
        // in didUpdateBackingStoreState.
        m_webPageProxy->process()->responsivenessTimer()->start();
    }

#if USE(ACCELERATED_COMPOSITING)
    if (m_isWaitingForDidUpdateBackingStoreState && !m_layerTreeContext.isEmpty()) {
        // Wait for the DidUpdateBackingStoreState message. Normally we do this in DrawingAreaProxyImpl::paint, but that
        // function is never called when in accelerated compositing mode.
        waitForAndDispatchDidUpdateBackingStoreState();
    }
#endif
}

void DrawingAreaProxyImpl::waitForAndDispatchDidUpdateBackingStoreState()
{
    ASSERT(m_isWaitingForDidUpdateBackingStoreState);

    if (!m_webPageProxy->isValid())
        return;
    if (m_webPageProxy->process()->isLaunching())
        return;

#if USE(ACCELERATED_COMPOSITING)
    // FIXME: waitForAndDispatchImmediately will always return the oldest DidUpdateBackingStoreState message that
    // hasn't yet been processed. But it might be better to skip ahead to some other DidUpdateBackingStoreState
    // message, if multiple DidUpdateBackingStoreState messages are waiting to be processed. For instance, we could
    // choose the most recent one, or the one that is closest to our current size.

    // The timeout, in seconds, we use when waiting for a DidUpdateBackingStoreState message when we're asked to paint.
    static const double didUpdateBackingStoreStateTimeout = 0.5;
    m_webPageProxy->process()->connection()->waitForAndDispatchImmediately<Messages::DrawingAreaProxy::DidUpdateBackingStoreState>(m_webPageProxy->pageID(), didUpdateBackingStoreStateTimeout);
#endif
}

#if USE(ACCELERATED_COMPOSITING)
void DrawingAreaProxyImpl::enterAcceleratedCompositingMode(const LayerTreeContext& layerTreeContext)
{
    ASSERT(!isInAcceleratedCompositingMode());

    m_backingStore = nullptr;
    m_layerTreeContext = layerTreeContext;
    m_webPageProxy->enterAcceleratedCompositingMode(layerTreeContext);
}

void DrawingAreaProxyImpl::exitAcceleratedCompositingMode()
{
    ASSERT(isInAcceleratedCompositingMode());

    m_layerTreeContext = LayerTreeContext();    
    m_webPageProxy->exitAcceleratedCompositingMode();
}
#endif

void DrawingAreaProxyImpl::discardBackingStoreSoon()
{
    if (!m_isBackingStoreDiscardable || m_discardBackingStoreTimer.isActive())
        return;

    // We'll wait this many seconds after the last paint before throwing away our backing store to save memory.
    // FIXME: It would be smarter to make this delay based on how expensive painting is. See <http://webkit.org/b/55733>.
    static const double discardBackingStoreDelay = 2;

    m_discardBackingStoreTimer.startOneShot(discardBackingStoreDelay);
}

void DrawingAreaProxyImpl::discardBackingStore()
{
    m_backingStore = nullptr;
    backingStoreStateDidChange(DoNotRespondImmediately);
}

} // namespace WebKit
