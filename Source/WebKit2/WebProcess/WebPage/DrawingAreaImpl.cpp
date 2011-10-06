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
#include "DrawingAreaImpl.h"

#include "DrawingAreaProxyMessages.h"
#include "LayerTreeContext.h"
#include "ShareableBitmap.h"
#include "UpdateInfo.h"
#include "WebPage.h"
#include "WebPageCreationParameters.h"
#include "WebProcess.h"
#include <WebCore/GraphicsContext.h>
#include <WebCore/Page.h>
#include <WebCore/Settings.h>

using namespace WebCore;
using namespace std;

namespace WebKit {

PassOwnPtr<DrawingAreaImpl> DrawingAreaImpl::create(WebPage* webPage, const WebPageCreationParameters& parameters)
{
    return adoptPtr(new DrawingAreaImpl(webPage, parameters));
}

DrawingAreaImpl::~DrawingAreaImpl()
{
    if (m_layerTreeHost)
        m_layerTreeHost->invalidate();
}

DrawingAreaImpl::DrawingAreaImpl(WebPage* webPage, const WebPageCreationParameters& parameters)
    : DrawingArea(DrawingAreaTypeImpl, webPage)
    , m_backingStoreStateID(0)
    , m_inUpdateBackingStoreState(false)
    , m_shouldSendDidUpdateBackingStoreState(false)
    , m_isWaitingForDidUpdate(false)
    , m_compositingAccordingToProxyMessages(false)
    , m_layerTreeStateIsFrozen(false)
    , m_wantsToExitAcceleratedCompositingMode(false)
    , m_isPaintingSuspended(!parameters.isVisible)
    , m_alwaysUseCompositing(false)
    , m_shouldThrottleDisplay(true)
    , m_lastDisplayTime(0)
    , m_displayTimer(WebProcess::shared().runLoop(), this, &DrawingAreaImpl::displayTimerFired)
    , m_exitCompositingTimer(WebProcess::shared().runLoop(), this, &DrawingAreaImpl::exitAcceleratedCompositingMode)
{
    if (webPage->corePage()->settings()->acceleratedDrawingEnabled())
        m_alwaysUseCompositing = true;
        
    if (m_alwaysUseCompositing)
        enterAcceleratedCompositingMode(0);
}

void DrawingAreaImpl::setNeedsDisplay(const IntRect& rect)
{
    IntRect dirtyRect = rect;
    dirtyRect.intersect(m_webPage->bounds());

    if (dirtyRect.isEmpty())
        return;

    if (m_layerTreeHost) {
        ASSERT(m_dirtyRegion.isEmpty());

        m_layerTreeHost->setNonCompositedContentsNeedDisplay(dirtyRect);
        return;
    }
    
    if (m_webPage->mainFrameHasCustomRepresentation())
        return;

    m_dirtyRegion.unite(dirtyRect);
    scheduleDisplay();
}

void DrawingAreaImpl::scroll(const IntRect& scrollRect, const IntSize& scrollOffset)
{
    if (m_layerTreeHost) {
        ASSERT(m_scrollRect.isEmpty());
        ASSERT(m_scrollOffset.isEmpty());
        ASSERT(m_dirtyRegion.isEmpty());

        m_layerTreeHost->scrollNonCompositedContents(scrollRect, scrollOffset);
        return;
    }

    if (m_webPage->mainFrameHasCustomRepresentation())
        return;

    if (!m_scrollRect.isEmpty() && scrollRect != m_scrollRect) {
        unsigned scrollArea = scrollRect.width() * scrollRect.height();
        unsigned currentScrollArea = m_scrollRect.width() * m_scrollRect.height();

        if (currentScrollArea >= scrollArea) {
            // The rect being scrolled is at least as large as the rect we'd like to scroll.
            // Go ahead and just invalidate the scroll rect.
            setNeedsDisplay(scrollRect);
            return;
        }

        // Just repaint the entire current scroll rect, we'll scroll the new rect instead.
        setNeedsDisplay(m_scrollRect);
        m_scrollRect = IntRect();
        m_scrollOffset = IntSize();
    }

    // Get the part of the dirty region that is in the scroll rect.
    Region dirtyRegionInScrollRect = intersect(scrollRect, m_dirtyRegion);
    if (!dirtyRegionInScrollRect.isEmpty()) {
        // There are parts of the dirty region that are inside the scroll rect.
        // We need to subtract them from the region, move them and re-add them.
        m_dirtyRegion.subtract(scrollRect);

        // Move the dirty parts.
        Region movedDirtyRegionInScrollRect = intersect(translate(dirtyRegionInScrollRect, scrollOffset), scrollRect);

        // And add them back.
        m_dirtyRegion.unite(movedDirtyRegionInScrollRect);
    } 
    
    // Compute the scroll repaint region.
    Region scrollRepaintRegion = subtract(scrollRect, translate(scrollRect, scrollOffset));

    m_dirtyRegion.unite(scrollRepaintRegion);
    scheduleDisplay();

    m_scrollRect = scrollRect;
    m_scrollOffset += scrollOffset;
}

void DrawingAreaImpl::setLayerTreeStateIsFrozen(bool isFrozen)
{
    if (m_layerTreeStateIsFrozen == isFrozen)
        return;

    m_layerTreeStateIsFrozen = isFrozen;

    if (m_layerTreeHost)
        m_layerTreeHost->setLayerFlushSchedulingEnabled(!isFrozen);

    if (isFrozen)
        m_exitCompositingTimer.stop();
    else if (m_wantsToExitAcceleratedCompositingMode)
        exitAcceleratedCompositingModeSoon();
}

void DrawingAreaImpl::forceRepaint()
{
    setNeedsDisplay(m_webPage->bounds());

    m_webPage->layoutIfNeeded();

    if (m_layerTreeHost) {
        // FIXME: We need to do the same work as the layerHostDidFlushLayers function here,
        // but clearly it doesn't make sense to call the function with that name.
        // Consider refactoring and renaming it.
        if (m_compositingAccordingToProxyMessages)
            m_layerTreeHost->forceRepaint();
        else {
            // Call setShouldNotifyAfterNextScheduledLayerFlush(false) here to 
            // prevent layerHostDidFlushLayers() from being called a second time.
            m_layerTreeHost->setShouldNotifyAfterNextScheduledLayerFlush(false);
            layerHostDidFlushLayers();
        }
        return;
    }

    m_isWaitingForDidUpdate = false;
    display();
}

void DrawingAreaImpl::enableDisplayThrottling()
{
    m_shouldThrottleDisplay = true;
}

void DrawingAreaImpl::disableDisplayThrottling()
{
    m_shouldThrottleDisplay = false;
}

void DrawingAreaImpl::didInstallPageOverlay()
{
    if (m_layerTreeHost)
        m_layerTreeHost->didInstallPageOverlay();
}

void DrawingAreaImpl::didUninstallPageOverlay()
{
    if (m_layerTreeHost)
        m_layerTreeHost->didUninstallPageOverlay();

    setNeedsDisplay(m_webPage->bounds());
}

void DrawingAreaImpl::setPageOverlayNeedsDisplay(const IntRect& rect)
{
    if (m_layerTreeHost) {
        m_layerTreeHost->setPageOverlayNeedsDisplay(rect);
        return;
    }

    setNeedsDisplay(rect);
}

void DrawingAreaImpl::layerHostDidFlushLayers()
{
    ASSERT(m_layerTreeHost);

    m_layerTreeHost->forceRepaint();

    if (m_shouldSendDidUpdateBackingStoreState && !exitAcceleratedCompositingModePending()) {
        sendDidUpdateBackingStoreState();
        return;
    }

    if (!m_layerTreeHost)
        return;

#if USE(ACCELERATED_COMPOSITING)
    ASSERT(!m_compositingAccordingToProxyMessages);
    if (!exitAcceleratedCompositingModePending()) {
        m_webPage->send(Messages::DrawingAreaProxy::EnterAcceleratedCompositingMode(m_backingStoreStateID, m_layerTreeHost->layerTreeContext()));
        m_compositingAccordingToProxyMessages = true;
    }
#endif
}

void DrawingAreaImpl::setRootCompositingLayer(GraphicsLayer* graphicsLayer)
{
    // FIXME: Instead of using nested if statements, we should keep a compositing state
    // enum in the DrawingAreaImpl object and have a changeAcceleratedCompositingState function
    // that takes the new state.

    if (graphicsLayer) {
        if (!m_layerTreeHost) {
            // We're actually entering accelerated compositing mode.
            enterAcceleratedCompositingMode(graphicsLayer);
        } else {
            // We're already in accelerated compositing mode, but the root compositing layer changed.

            m_exitCompositingTimer.stop();
            m_wantsToExitAcceleratedCompositingMode = false;

            // If we haven't sent the EnterAcceleratedCompositingMode message, make sure that the
            // layer tree host calls us back after the next layer flush so we can send it then.
            if (!m_compositingAccordingToProxyMessages)
                m_layerTreeHost->setShouldNotifyAfterNextScheduledLayerFlush(true);

            m_layerTreeHost->setRootCompositingLayer(graphicsLayer);
        }
    } else {
        if (m_layerTreeHost) {
            m_layerTreeHost->setRootCompositingLayer(0);
            if (!m_alwaysUseCompositing) {
                // We'll exit accelerated compositing mode on a timer, to avoid re-entering
                // compositing code via display() and layout.
                // If we're leaving compositing mode because of a setSize, it is safe to
                // exit accelerated compositing mode right away.
                if (m_inUpdateBackingStoreState)
                    exitAcceleratedCompositingMode();
                else
                    exitAcceleratedCompositingModeSoon();
            }
        }
    }
}

void DrawingAreaImpl::scheduleCompositingLayerSync()
{
    if (!m_layerTreeHost)
        return;
    m_layerTreeHost->scheduleLayerFlush();
}

void DrawingAreaImpl::syncCompositingLayers()
{
}

void DrawingAreaImpl::updateBackingStoreState(uint64_t stateID, bool respondImmediately, float deviceScaleFactor, const WebCore::IntSize& size, const WebCore::IntSize& scrollOffset)
{
    ASSERT(!m_inUpdateBackingStoreState);
    m_inUpdateBackingStoreState = true;

    ASSERT_ARG(stateID, stateID >= m_backingStoreStateID);
    if (stateID != m_backingStoreStateID) {
        m_backingStoreStateID = stateID;
        m_shouldSendDidUpdateBackingStoreState = true;

        m_webPage->setDeviceScaleFactor(deviceScaleFactor);
        m_webPage->setSize(size);
        m_webPage->layoutIfNeeded();
        m_webPage->scrollMainFrameIfNotAtMaxScrollPosition(scrollOffset);

        if (m_layerTreeHost) {
            m_layerTreeHost->deviceScaleFactorDidChange();
            m_layerTreeHost->sizeDidChange(size);
        } else
            m_dirtyRegion = m_webPage->bounds();
    } else {
        ASSERT(size == m_webPage->size());
        if (!m_shouldSendDidUpdateBackingStoreState) {
            // We've already sent a DidUpdateBackingStoreState message for this state. We have nothing more to do.
            m_inUpdateBackingStoreState = false;
            return;
        }
    }

    // The UI process has updated to a new backing store state. Any Update messages we sent before
    // this point will be ignored. We wait to set this to false until after updating the page's
    // size so that any displays triggered by the relayout will be ignored. If we're supposed to
    // respond to the UpdateBackingStoreState message immediately, we'll do a display anyway in
    // sendDidUpdateBackingStoreState; otherwise we shouldn't do one right now.
    m_isWaitingForDidUpdate = false;

    if (respondImmediately) {
        // Make sure to resume painting if we're supposed to respond immediately, otherwise we'll just
        // send back an empty UpdateInfo struct.
        if (m_isPaintingSuspended)
            resumePainting();

        sendDidUpdateBackingStoreState();
    }

    m_inUpdateBackingStoreState = false;
}

void DrawingAreaImpl::sendDidUpdateBackingStoreState()
{
    ASSERT(!m_isWaitingForDidUpdate);
    ASSERT(m_shouldSendDidUpdateBackingStoreState);

    m_shouldSendDidUpdateBackingStoreState = false;

    UpdateInfo updateInfo;

    if (!m_isPaintingSuspended && !m_layerTreeHost)
        display(updateInfo);

    LayerTreeContext layerTreeContext;

    if (m_isPaintingSuspended || m_layerTreeHost) {
        updateInfo.viewSize = m_webPage->size();
        updateInfo.deviceScaleFactor = m_webPage->corePage()->deviceScaleFactor();

        if (m_layerTreeHost) {
            layerTreeContext = m_layerTreeHost->layerTreeContext();

            // We don't want the layer tree host to notify after the next scheduled
            // layer flush because that might end up sending an EnterAcceleratedCompositingMode
            // message back to the UI process, but the updated layer tree context
            // will be sent back in the DidUpdateBackingStoreState message.
            m_layerTreeHost->setShouldNotifyAfterNextScheduledLayerFlush(false);
            m_layerTreeHost->forceRepaint();
        }
    }

    m_webPage->send(Messages::DrawingAreaProxy::DidUpdateBackingStoreState(m_backingStoreStateID, updateInfo, layerTreeContext));
    m_compositingAccordingToProxyMessages = !layerTreeContext.isEmpty();
}

void DrawingAreaImpl::didUpdate()
{
    // We might get didUpdate messages from the UI process even after we've
    // entered accelerated compositing mode. Ignore them.
    if (m_layerTreeHost)
        return;

    m_isWaitingForDidUpdate = false;

    // Display if needed. We call displayTimerFired here since it will throttle updates to 60fps.
    displayTimerFired();
}

void DrawingAreaImpl::suspendPainting()
{
    ASSERT(!m_isPaintingSuspended);

    if (m_layerTreeHost)
        m_layerTreeHost->pauseRendering();

    m_isPaintingSuspended = true;
    m_displayTimer.stop();
}

void DrawingAreaImpl::resumePainting()
{
    if (!m_isPaintingSuspended) {
        // FIXME: We can get a call to resumePainting when painting is not suspended.
        // This happens when sending a synchronous message to create a new page. See <rdar://problem/8976531>.
        return;
    }
    
    if (m_layerTreeHost)
        m_layerTreeHost->resumeRendering();
        
    m_isPaintingSuspended = false;

    // FIXME: We shouldn't always repaint everything here.
    setNeedsDisplay(m_webPage->bounds());
}

void DrawingAreaImpl::enterAcceleratedCompositingMode(GraphicsLayer* graphicsLayer)
{
    m_exitCompositingTimer.stop();
    m_wantsToExitAcceleratedCompositingMode = false;

    ASSERT(!m_layerTreeHost);

    m_layerTreeHost = LayerTreeHost::create(m_webPage);
    if (!m_inUpdateBackingStoreState)
        m_layerTreeHost->setShouldNotifyAfterNextScheduledLayerFlush(true);

    m_layerTreeHost->setRootCompositingLayer(graphicsLayer);
    
    // Non-composited content will now be handled exclusively by the layer tree host.
    m_dirtyRegion = Region();
    m_scrollRect = IntRect();
    m_scrollOffset = IntSize();
    m_displayTimer.stop();
    m_isWaitingForDidUpdate = false;
}

void DrawingAreaImpl::exitAcceleratedCompositingMode()
{
    if (m_alwaysUseCompositing)
        return;

    ASSERT(!m_layerTreeStateIsFrozen);

    m_exitCompositingTimer.stop();
    m_wantsToExitAcceleratedCompositingMode = false;

    ASSERT(m_layerTreeHost);

    m_layerTreeHost->invalidate();
    m_layerTreeHost = nullptr;
    m_dirtyRegion = m_webPage->bounds();

    if (m_inUpdateBackingStoreState)
        return;

    if (m_shouldSendDidUpdateBackingStoreState) {
        sendDidUpdateBackingStoreState();
        return;
    }

    UpdateInfo updateInfo;
    if (m_isPaintingSuspended) {
        updateInfo.viewSize = m_webPage->size();
        updateInfo.deviceScaleFactor = m_webPage->corePage()->deviceScaleFactor();
    } else
        display(updateInfo);

#if USE(ACCELERATED_COMPOSITING)
    // Send along a complete update of the page so we can paint the contents right after we exit the
    // accelerated compositing mode, eliminiating flicker.
    if (m_compositingAccordingToProxyMessages) {
        m_webPage->send(Messages::DrawingAreaProxy::ExitAcceleratedCompositingMode(m_backingStoreStateID, updateInfo));
        m_compositingAccordingToProxyMessages = false;
    } else {
        // If we left accelerated compositing mode before we sent an EnterAcceleratedCompositingMode message to the
        // UI process, we still need to let it know about the new contents, so send an Update message.
        m_webPage->send(Messages::DrawingAreaProxy::Update(m_backingStoreStateID, updateInfo));
    }
#endif
}

void DrawingAreaImpl::exitAcceleratedCompositingModeSoon()
{
    if (m_layerTreeStateIsFrozen) {
        m_wantsToExitAcceleratedCompositingMode = true;
        return;
    }

    if (exitAcceleratedCompositingModePending())
        return;

    m_exitCompositingTimer.startOneShot(0);
}

void DrawingAreaImpl::scheduleDisplay()
{
    ASSERT(!m_layerTreeHost);

    if (m_isWaitingForDidUpdate)
        return;

    if (m_isPaintingSuspended)
        return;

    if (m_dirtyRegion.isEmpty())
        return;

    if (m_displayTimer.isActive())
        return;

    m_displayTimer.startOneShot(0);
}

void DrawingAreaImpl::displayTimerFired()
{
#if PLATFORM(WIN)
    // For now we'll cap painting on Windows to 30fps because painting is much slower there for some reason.
    static const double minimumFrameInterval = 1.0 / 30.0;
#else
    static const double minimumFrameInterval = 1.0 / 60.0;
#endif

    if (m_shouldThrottleDisplay) {
        double timeSinceLastDisplay = currentTime() - m_lastDisplayTime;
        double timeUntilNextDisplay = minimumFrameInterval - timeSinceLastDisplay;

        if (timeUntilNextDisplay > 0) {
            m_displayTimer.startOneShot(timeUntilNextDisplay);
            return;
        }
    }

    display();
}

void DrawingAreaImpl::display()
{
    ASSERT(!m_layerTreeHost);
    ASSERT(!m_isWaitingForDidUpdate);
    ASSERT(!m_inUpdateBackingStoreState);

    if (m_isPaintingSuspended)
        return;

    if (m_dirtyRegion.isEmpty())
        return;

    if (m_shouldSendDidUpdateBackingStoreState) {
        sendDidUpdateBackingStoreState();
        return;
    }

    UpdateInfo updateInfo;
    display(updateInfo);

    if (m_layerTreeHost) {
        // The call to update caused layout which turned on accelerated compositing.
        // Don't send an Update message in this case.
        return;
    }

    m_webPage->send(Messages::DrawingAreaProxy::Update(m_backingStoreStateID, updateInfo));
    m_isWaitingForDidUpdate = true;
}

static bool shouldPaintBoundsRect(const IntRect& bounds, const Vector<IntRect>& rects)
{
    const size_t rectThreshold = 10;
    const double wastedSpaceThreshold = 0.75;

    if (rects.size() <= 1 || rects.size() > rectThreshold)
        return true;

    // Attempt to guess whether or not we should use the region bounds rect or the individual rects.
    // We do this by computing the percentage of "wasted space" in the bounds.  If that wasted space
    // is too large, then we will do individual rect painting instead.
    unsigned boundsArea = bounds.width() * bounds.height();
    unsigned rectsArea = 0;
    for (size_t i = 0; i < rects.size(); ++i)
        rectsArea += rects[i].width() * rects[i].height();

    double wastedSpace = 1 - (static_cast<double>(rectsArea) / boundsArea);

    return wastedSpace <= wastedSpaceThreshold;
}

#if !PLATFORM(WIN)
PassOwnPtr<GraphicsContext> DrawingAreaImpl::createGraphicsContext(ShareableBitmap* bitmap)
{
    return bitmap->createGraphicsContext();
}
#endif

void DrawingAreaImpl::display(UpdateInfo& updateInfo)
{
    ASSERT(!m_isPaintingSuspended);
    ASSERT(!m_layerTreeHost);
    ASSERT(!m_webPage->size().isEmpty());

    // FIXME: It would be better if we could avoid painting altogether when there is a custom representation.
    if (m_webPage->mainFrameHasCustomRepresentation()) {
        // ASSUMPTION: the custom representation will be painting the dirty region for us.
        m_dirtyRegion = Region();
        return;
    }

    m_webPage->layoutIfNeeded();

    // The layout may have put the page into accelerated compositing mode. If the LayerTreeHost is
    // in charge of displaying, we have nothing more to do.
    if (m_layerTreeHost)
        return;

    updateInfo.viewSize = m_webPage->size();
    updateInfo.deviceScaleFactor = m_webPage->corePage()->deviceScaleFactor();

    IntRect bounds = m_dirtyRegion.bounds();
    ASSERT(m_webPage->bounds().contains(bounds));

    IntSize bitmapSize = bounds.size();
    bitmapSize.scale(m_webPage->corePage()->deviceScaleFactor());
    RefPtr<ShareableBitmap> bitmap = ShareableBitmap::createShareable(bitmapSize, ShareableBitmap::SupportsAlpha);
    if (!bitmap)
        return;

    if (!bitmap->createHandle(updateInfo.bitmapHandle))
        return;

    Vector<IntRect> rects = m_dirtyRegion.rects();

    if (shouldPaintBoundsRect(bounds, rects)) {
        rects.clear();
        rects.append(bounds);
    }

    updateInfo.scrollRect = m_scrollRect;
    updateInfo.scrollOffset = m_scrollOffset;

    m_dirtyRegion = Region();
    m_scrollRect = IntRect();
    m_scrollOffset = IntSize();

    OwnPtr<GraphicsContext> graphicsContext = createGraphicsContext(bitmap.get());
    graphicsContext->scale(FloatSize(m_webPage->corePage()->deviceScaleFactor(), m_webPage->corePage()->deviceScaleFactor()));

    updateInfo.updateRectBounds = bounds;

    graphicsContext->translate(-bounds.x(), -bounds.y());

    for (size_t i = 0; i < rects.size(); ++i) {
        m_webPage->drawRect(*graphicsContext, rects[i]);
        if (m_webPage->hasPageOverlay())
            m_webPage->drawPageOverlay(*graphicsContext, rects[i]);
        updateInfo.updateRects.append(rects[i]);
    }

    // Layout can trigger more calls to setNeedsDisplay and we don't want to process them
    // until the UI process has painted the update, so we stop the timer here.
    m_displayTimer.stop();

    m_lastDisplayTime = currentTime();
}


} // namespace WebKit
