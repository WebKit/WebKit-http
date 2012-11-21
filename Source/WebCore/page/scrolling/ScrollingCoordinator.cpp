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

#include "ScrollingCoordinator.h"

#include "Frame.h"
#include "FrameView.h"
#include "IntRect.h"
#include "Page.h"
#include "PlatformWheelEvent.h"
#include "PluginViewBase.h"
#include "Region.h"
#include "RenderView.h"
#include "ScrollAnimator.h"
#include <wtf/MainThread.h>

#if USE(ACCELERATED_COMPOSITING)
#include "RenderLayerCompositor.h"
#endif

#if ENABLE(THREADED_SCROLLING)
#include "ScrollingCoordinatorMac.h"
#endif

#if PLATFORM(CHROMIUM)
#include "ScrollingCoordinatorChromium.h"
#endif

namespace WebCore {

PassRefPtr<ScrollingCoordinator> ScrollingCoordinator::create(Page* page)
{
#if USE(ACCELERATED_COMPOSITING) && ENABLE(THREADED_SCROLLING)
    return adoptRef(new ScrollingCoordinatorMac(page));
#endif

#if PLATFORM(CHROMIUM)
    return adoptRef(new ScrollingCoordinatorChromium(page));
#endif

    return adoptRef(new ScrollingCoordinator(page));
}

static int fixedPositionScrollOffset(int scrollPosition, int maxValue, int scrollOrigin, float dragFactor)
{
    if (!maxValue)
        return 0;

    if (!scrollOrigin) {
        if (scrollPosition < 0)
            scrollPosition = 0;
        else if (scrollPosition > maxValue)
            scrollPosition = maxValue;
    } else {
        if (scrollPosition > 0)
            scrollPosition = 0;
        else if (scrollPosition < -maxValue)
            scrollPosition = -maxValue;
    }
    
    return scrollPosition * dragFactor;
}

IntSize scrollOffsetForFixedPosition(const IntRect& visibleContentRect, const IntSize& contentsSize, const IntPoint& scrollPosition, const IntPoint& scrollOrigin, float frameScaleFactor, bool fixedElementsLayoutRelativeToFrame)
{
    IntSize maxOffset(contentsSize.width() - visibleContentRect.width(), contentsSize.height() - visibleContentRect.height());
    
    FloatSize dragFactor = fixedElementsLayoutRelativeToFrame ? FloatSize(1, 1) : FloatSize(
        (contentsSize.width() - visibleContentRect.width() * frameScaleFactor) / maxOffset.width(),
        (contentsSize.height() - visibleContentRect.height() * frameScaleFactor) / maxOffset.height());

    int x = fixedPositionScrollOffset(scrollPosition.x(), maxOffset.width(), scrollOrigin.x(), dragFactor.width() / frameScaleFactor);
    int y = fixedPositionScrollOffset(scrollPosition.y(), maxOffset.height(), scrollOrigin.y(), dragFactor.height() / frameScaleFactor);

    return IntSize(x, y);
}

ScrollingCoordinator::ScrollingCoordinator(Page* page)
    : m_page(page)
    , m_updateMainFrameScrollPositionTimer(this, &ScrollingCoordinator::updateMainFrameScrollPositionTimerFired)
    , m_scheduledUpdateIsProgrammaticScroll(false)
    , m_scheduledScrollingLayerPositionAction(SyncScrollingLayerPosition)
    , m_forceMainThreadScrollLayerPositionUpdates(false)
{
}

ScrollingCoordinator::~ScrollingCoordinator()
{
    ASSERT(!m_page);
}

void ScrollingCoordinator::pageDestroyed()
{
    ASSERT(m_page);
    m_page = 0;
}

bool ScrollingCoordinator::coordinatesScrollingForFrameView(FrameView* frameView) const
{
    ASSERT(isMainThread());
    ASSERT(m_page);

    // We currently only handle the main frame.
    if (frameView->frame() != m_page->mainFrame())
        return false;

    // We currently only support composited mode.
#if USE(ACCELERATED_COMPOSITING)
    RenderView* renderView = m_page->mainFrame()->contentRenderer();
    if (!renderView)
        return false;
    return renderView->usesCompositing();
#else
    return false;
#endif
}

Region ScrollingCoordinator::computeNonFastScrollableRegion(Frame* frame, const IntPoint& frameLocation)
{
    Region nonFastScrollableRegion;
    FrameView* frameView = frame->view();
    if (!frameView)
        return nonFastScrollableRegion;

    IntPoint offset = frameLocation;
    offset.moveBy(frameView->frameRect().location());

    if (const FrameView::ScrollableAreaSet* scrollableAreas = frameView->scrollableAreas()) {
        for (FrameView::ScrollableAreaSet::const_iterator it = scrollableAreas->begin(), end = scrollableAreas->end(); it != end; ++it) {
            ScrollableArea* scrollableArea = *it;
#if USE(ACCELERATED_COMPOSITING)
            // Composited scrollable areas can be scrolled off the main thread.
            if (scrollableArea->usesCompositedScrolling())
                continue;
#endif
            IntRect box = scrollableArea->scrollableAreaBoundingBox();
            box.moveBy(offset);
            nonFastScrollableRegion.unite(box);
        }
    }

    if (const HashSet<RefPtr<Widget> >* children = frameView->children()) {
        for (HashSet<RefPtr<Widget> >::const_iterator it = children->begin(), end = children->end(); it != end; ++it) {
            if (!(*it)->isPluginViewBase())
                continue;

            PluginViewBase* pluginViewBase = static_cast<PluginViewBase*>((*it).get());
            if (pluginViewBase->wantsWheelEvents())
                nonFastScrollableRegion.unite(pluginViewBase->frameRect());
        }
    }

    FrameTree* tree = frame->tree();
    for (Frame* subFrame = tree->firstChild(); subFrame; subFrame = subFrame->tree()->nextSibling())
        nonFastScrollableRegion.unite(computeNonFastScrollableRegion(subFrame, offset));

    return nonFastScrollableRegion;
}

unsigned ScrollingCoordinator::computeCurrentWheelEventHandlerCount()
{
    unsigned wheelEventHandlerCount = 0;

    for (Frame* frame = m_page->mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        if (frame->document())
            wheelEventHandlerCount += frame->document()->wheelEventHandlerCount();
    }

    return wheelEventHandlerCount;
}

void ScrollingCoordinator::frameViewWheelEventHandlerCountChanged(FrameView* frameView)
{
    ASSERT(isMainThread());
    ASSERT(m_page);

    recomputeWheelEventHandlerCountForFrameView(frameView);
}

void ScrollingCoordinator::frameViewHasSlowRepaintObjectsDidChange(FrameView* frameView)
{
    ASSERT(isMainThread());
    ASSERT(m_page);

    if (!coordinatesScrollingForFrameView(frameView))
        return;

    updateShouldUpdateScrollLayerPositionOnMainThread();
}

void ScrollingCoordinator::frameViewFixedObjectsDidChange(FrameView* frameView)
{
    ASSERT(isMainThread());
    ASSERT(m_page);

    if (!coordinatesScrollingForFrameView(frameView))
        return;

    updateShouldUpdateScrollLayerPositionOnMainThread();
}

GraphicsLayer* ScrollingCoordinator::scrollLayerForFrameView(FrameView* frameView)
{
#if USE(ACCELERATED_COMPOSITING)
    Frame* frame = frameView->frame();
    if (!frame)
        return 0;

    RenderView* renderView = frame->contentRenderer();
    if (!renderView)
        return 0;
    return renderView->compositor()->scrollLayer();
#else
    UNUSED_PARAM(frameView);
    return 0;
#endif
}

void ScrollingCoordinator::frameViewRootLayerDidChange(FrameView* frameView)
{
    ASSERT(isMainThread());
    ASSERT(m_page);

    if (!coordinatesScrollingForFrameView(frameView))
        return;

    frameViewLayoutUpdated(frameView);
    recomputeWheelEventHandlerCountForFrameView(frameView);
    updateShouldUpdateScrollLayerPositionOnMainThread();
}

void ScrollingCoordinator::scheduleUpdateMainFrameScrollPosition(const IntPoint& scrollPosition, bool programmaticScroll, SetOrSyncScrollingLayerPosition scrollingLayerPositionAction)
{
    if (m_updateMainFrameScrollPositionTimer.isActive()) {
        if (m_scheduledUpdateIsProgrammaticScroll == programmaticScroll
            && m_scheduledScrollingLayerPositionAction == scrollingLayerPositionAction) {
            m_scheduledUpdateScrollPosition = scrollPosition;
            return;
        }
    
        // If the parameters don't match what was previosly scheduled, dispatch immediately.
        m_updateMainFrameScrollPositionTimer.stop();
        updateMainFrameScrollPosition(m_scheduledUpdateScrollPosition, m_scheduledUpdateIsProgrammaticScroll, m_scheduledScrollingLayerPositionAction);
        updateMainFrameScrollPosition(scrollPosition, programmaticScroll, scrollingLayerPositionAction);
        return;
    }

    m_scheduledUpdateScrollPosition = scrollPosition;
    m_scheduledUpdateIsProgrammaticScroll = programmaticScroll;
    m_scheduledScrollingLayerPositionAction = scrollingLayerPositionAction;
    m_updateMainFrameScrollPositionTimer.startOneShot(0);
}

void ScrollingCoordinator::updateMainFrameScrollPositionTimerFired(Timer<ScrollingCoordinator>*)
{
    updateMainFrameScrollPosition(m_scheduledUpdateScrollPosition, m_scheduledUpdateIsProgrammaticScroll, m_scheduledScrollingLayerPositionAction);
}

void ScrollingCoordinator::updateMainFrameScrollPosition(const IntPoint& scrollPosition, bool programmaticScroll, SetOrSyncScrollingLayerPosition scrollingLayerPositionAction)
{
    ASSERT(isMainThread());

    if (!m_page)
        return;

    FrameView* frameView = m_page->mainFrame()->view();
    if (!frameView)
        return;

    bool oldProgrammaticScroll = frameView->inProgrammaticScroll();
    frameView->setInProgrammaticScroll(programmaticScroll);

    frameView->setConstrainsScrollingToContentEdge(false);
    frameView->notifyScrollPositionChanged(scrollPosition);
    frameView->setConstrainsScrollingToContentEdge(true);

    frameView->setInProgrammaticScroll(oldProgrammaticScroll);

#if USE(ACCELERATED_COMPOSITING)
    if (GraphicsLayer* scrollLayer = scrollLayerForFrameView(frameView)) {
        if (programmaticScroll || scrollingLayerPositionAction == SetScrollingLayerPosition)
            scrollLayer->setPosition(-frameView->scrollPosition());
        else {
            scrollLayer->syncPosition(-frameView->scrollPosition());
            LayoutRect viewportRect = frameView->visibleContentRect();
            viewportRect.setLocation(toPoint(frameView->scrollOffsetForFixedPosition()));
            syncChildPositions(viewportRect);
        }
    }
#else
    UNUSED_PARAM(scrollingLayerPositionAction);
#endif
}

#if PLATFORM(MAC) || (PLATFORM(CHROMIUM) && OS(DARWIN))
void ScrollingCoordinator::handleWheelEventPhase(PlatformWheelEventPhase phase)
{
    ASSERT(isMainThread());

    if (!m_page)
        return;

    FrameView* frameView = m_page->mainFrame()->view();
    if (!frameView)
        return;

    frameView->scrollAnimator()->handleWheelEventPhase(phase);
}
#endif

bool ScrollingCoordinator::hasVisibleSlowRepaintFixedObjects(FrameView* frameView) const
{
    const FrameView::ViewportConstrainedObjectSet* viewportConstrainedObjects = frameView->viewportConstrainedObjects();
    if (!viewportConstrainedObjects)
        return false;

#if USE(ACCELERATED_COMPOSITING)
    for (FrameView::ViewportConstrainedObjectSet::const_iterator it = viewportConstrainedObjects->begin(), end = viewportConstrainedObjects->end(); it != end; ++it) {
        RenderObject* viewportConstrainedObject = *it;
        if (!viewportConstrainedObject->isBoxModelObject() || !viewportConstrainedObject->hasLayer())
            return true;
        RenderBoxModelObject* viewportConstrainedBoxModelObject = toRenderBoxModelObject(viewportConstrainedObject);
        if (!viewportConstrainedBoxModelObject->layer()->backing())
            return true;
    }
    return false;
#else
    return viewportConstrainedObjects->size();
#endif
}

MainThreadScrollingReasons ScrollingCoordinator::mainThreadScrollingReasons() const
{
    FrameView* frameView = m_page->mainFrame()->view();

    MainThreadScrollingReasons mainThreadScrollingReasons = (MainThreadScrollingReasons)0;

    if (m_forceMainThreadScrollLayerPositionUpdates)
        mainThreadScrollingReasons |= ForcedOnMainThread;
    if (frameView->hasSlowRepaintObjects())
        mainThreadScrollingReasons |= HasSlowRepaintObjects;
    if (!supportsFixedPositionLayers() && frameView->hasViewportConstrainedObjects())
        mainThreadScrollingReasons |= HasViewportConstrainedObjectsWithoutSupportingFixedLayers;
    if (supportsFixedPositionLayers() && hasVisibleSlowRepaintFixedObjects(frameView))
        mainThreadScrollingReasons |= HasNonLayerFixedObjects;
    if (m_page->mainFrame()->document()->isImageDocument())
        mainThreadScrollingReasons |= IsImageDocument;

    return mainThreadScrollingReasons;
}

void ScrollingCoordinator::updateShouldUpdateScrollLayerPositionOnMainThread()
{
    setShouldUpdateScrollLayerPositionOnMainThread(mainThreadScrollingReasons());
}

void ScrollingCoordinator::setForceMainThreadScrollLayerPositionUpdates(bool forceMainThreadScrollLayerPositionUpdates)
{
    if (m_forceMainThreadScrollLayerPositionUpdates == forceMainThreadScrollLayerPositionUpdates)
        return;

    m_forceMainThreadScrollLayerPositionUpdates = forceMainThreadScrollLayerPositionUpdates;
    updateShouldUpdateScrollLayerPositionOnMainThread();
}

ScrollingNodeID ScrollingCoordinator::uniqueScrollLayerID()
{
    static ScrollingNodeID uniqueScrollLayerID = 1;
    return uniqueScrollLayerID++;
}

String ScrollingCoordinator::scrollingStateTreeAsText() const
{
    return String();
}

} // namespace WebCore
