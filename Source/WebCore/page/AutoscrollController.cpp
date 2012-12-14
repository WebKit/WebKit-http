/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2012 Digia Plc. and/or its subsidiary(-ies)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "AutoscrollController.h"

#include "EventHandler.h"
#include "Frame.h"
#include "FrameView.h"
#include "HitTestResult.h"
#include "Page.h"
#include "RenderBox.h"
#include "ScrollView.h"

namespace WebCore {

// When the autoscroll or the panScroll is triggered when do the scroll every 0.05s to make it smooth
static const double autoscrollInterval = 0.05;

#if ENABLE(PAN_SCROLLING)
static Frame* getMainFrame(Frame* frame)
{
    Page* page = frame->page();
    return page ? page->mainFrame() : 0;
}
#endif

AutoscrollController::AutoscrollController()
    : m_autoscrollTimer(this, &AutoscrollController::autoscrollTimerFired)
    , m_autoscrollRenderer(0)
    , m_autoscrollType(NoAutoscroll)
#if ENABLE(PAN_SCROLLING)
    , m_panScrollButtonPressed(false)
    , m_springLoadedPanScrollInProgress(false)
#endif
{
}

RenderBox* AutoscrollController::autoscrollRenderer() const
{
    return m_autoscrollRenderer;
}

bool AutoscrollController::autoscrollInProgress() const
{
    return m_autoscrollType == AutoscrollForSelection;
}

void AutoscrollController::startAutoscrollForSelection(RenderObject* renderer)
{
    // We don't want to trigger the autoscroll or the panScroll if it's already active
    if (m_autoscrollTimer.isActive())
        return;
    RenderBox* scrollable = RenderBox::findAutoscrollable(renderer);
    if (!scrollable)
        return;
    m_autoscrollType = AutoscrollForSelection;
    m_autoscrollRenderer = scrollable;
    startAutoscrollTimer();
}

void AutoscrollController::stopAutoscrollTimer(bool rendererIsBeingDestroyed)
{
    RenderBox* scrollable = m_autoscrollRenderer;
    m_autoscrollTimer.stop();
    m_autoscrollType = NoAutoscroll;
    m_autoscrollRenderer = 0;
#if ENABLE(PAN_SCROLLING)
    m_panScrollButtonPressed = false;
    m_springLoadedPanScrollInProgress = false;
#endif

    if (!scrollable)
        return;

    Frame* frame = scrollable->frame();
    EventHandler* eventHandler = frame->eventHandler();
    if (autoscrollInProgress() && eventHandler->mouseDownWasInSubframe()) {
        if (Frame* subframe = eventHandler->subframeForTargetNode(eventHandler->mousePressNode()))
            subframe->eventHandler()->stopAutoscrollTimer(rendererIsBeingDestroyed);
        return;
    }

    if (!rendererIsBeingDestroyed)
        scrollable->stopAutoscroll();
#if ENABLE(PAN_SCROLLING)
    if (panScrollInProgress()) {
        if (FrameView* view = frame->view()) {
            view->removePanScrollIcon();
            view->setCursor(pointerCursor());
        }
    }
#endif

#if ENABLE(PAN_SCROLLING)
    // If we're not in the top frame we notify it that we are not doing a panScroll any more.
    if (Frame* mainFrame = getMainFrame(frame)) {
        if (frame != mainFrame)
            mainFrame->eventHandler()->didPanScrollStop();
    }
#endif
}

void AutoscrollController::updateAutoscrollRenderer()
{
    if (!m_autoscrollRenderer)
        return;

    RenderObject* renderer = m_autoscrollRenderer;

#if ENABLE(PAN_SCROLLING)
    HitTestResult hitTest = m_autoscrollRenderer->frame()->eventHandler()->hitTestResultAtPoint(m_panScrollStartPos, true);

    if (Node* nodeAtPoint = hitTest.innerNode())
        renderer = nodeAtPoint->renderer();
#endif

    while (renderer && !(renderer->isBox() && toRenderBox(renderer)->canAutoscroll()))
        renderer = renderer->parent();
    m_autoscrollRenderer = renderer && renderer->isBox() ? toRenderBox(renderer) : 0;
}

#if ENABLE(PAN_SCROLLING)
void AutoscrollController::didPanScrollStart()
{
    m_autoscrollType = AutoscrollForPan;
}

void AutoscrollController::didPanScrollStop()
{
    m_autoscrollType = NoAutoscroll;
}

void AutoscrollController::handleMouseReleaseEvent(const PlatformMouseEvent& mouseEvent)
{
    if (mouseEvent.button() == MiddleButton)
        m_panScrollButtonPressed = false;
    if (m_springLoadedPanScrollInProgress)
        stopAutoscrollTimer();
}

bool AutoscrollController::panScrollInProgress() const
{
    return m_autoscrollType == AutoscrollForPan;
}

void AutoscrollController::startPanScrolling(RenderBox* scrollable, const IntPoint& lastKnownMousePosition)
{
    // We don't want to trigger the autoscroll or the panScroll if it's already active
    if (m_autoscrollTimer.isActive())
        return;

    m_autoscrollType = AutoscrollForPan;
    m_autoscrollRenderer = scrollable;
    m_panScrollStartPos = lastKnownMousePosition;
    m_springLoadedPanScrollInProgress = false;

    if (FrameView* view = scrollable->frame()->view())
        view->addPanScrollIcon(lastKnownMousePosition);
    scrollable->frame()->eventHandler()->didPanScrollStart();
    startAutoscrollTimer();
}
#else
bool AutoscrollController::panScrollInProgress() const
{
    return false;
}
#endif

void AutoscrollController::autoscrollTimerFired(Timer<AutoscrollController>*)
{
    if (!m_autoscrollRenderer) {
        stopAutoscrollTimer();
        return;
    }

    Frame* frame = m_autoscrollRenderer->frame();
    switch (m_autoscrollType) {
    case AutoscrollForSelection:
        if (!frame->eventHandler()->mousePressed()) {
            stopAutoscrollTimer();
            return;
        }
        m_autoscrollRenderer->autoscroll();
        break;
    case NoAutoscroll:
        break;
#if ENABLE(PAN_SCROLLING)
    case AutoscrollForPan:
        // we verify that the main frame hasn't received the order to stop the panScroll
        if (Frame* mainFrame = getMainFrame(frame)) {
            if (!mainFrame->eventHandler()->panScrollInProgress()) {
                stopAutoscrollTimer();
                return;
            }
        }
        if (FrameView* view = frame->view())
            updatePanScrollState(view, frame->eventHandler()->lastKnownMousePosition());
        m_autoscrollRenderer->panScroll(m_panScrollStartPos);
        break;
#endif
    }
}

void AutoscrollController::startAutoscrollTimer()
{
    m_autoscrollTimer.startRepeating(autoscrollInterval);
}

#if ENABLE(PAN_SCROLLING)
void AutoscrollController::updatePanScrollState(FrameView* view, const IntPoint& lastKnownMousePosition)
{
    // At the original click location we draw a 4 arrowed icon. Over this icon there won't be any scroll
    // So we don't want to change the cursor over this area
    bool east = m_panScrollStartPos.x() < (lastKnownMousePosition.x() - ScrollView::noPanScrollRadius);
    bool west = m_panScrollStartPos.x() > (lastKnownMousePosition.x() + ScrollView::noPanScrollRadius);
    bool north = m_panScrollStartPos.y() > (lastKnownMousePosition.y() + ScrollView::noPanScrollRadius);
    bool south = m_panScrollStartPos.y() < (lastKnownMousePosition.y() - ScrollView::noPanScrollRadius);

    if ((east || west || north || south) && m_panScrollButtonPressed)
        m_springLoadedPanScrollInProgress = true;

    if (north) {
        if (east)
            view->setCursor(northEastPanningCursor());
        else if (west)
            view->setCursor(northWestPanningCursor());
        else
            view->setCursor(northPanningCursor());
    } else if (south) {
        if (east)
            view->setCursor(southEastPanningCursor());
        else if (west)
            view->setCursor(southWestPanningCursor());
        else
            view->setCursor(southPanningCursor());
    } else if (east)
        view->setCursor(eastPanningCursor());
    else if (west)
        view->setCursor(westPanningCursor());
    else
        view->setCursor(middlePanningCursor());
}
#endif

} // namespace WebCore
