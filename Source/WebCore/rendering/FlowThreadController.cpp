/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"

#include "FlowThreadController.h"

#include "NamedFlowCollection.h"
#include "RenderFlowThread.h"
#include "RenderLayer.h"
#include "RenderNamedFlowThread.h"
#include "StyleInheritedData.h"
#include "WebKitNamedFlow.h"
#include <wtf/text/AtomicString.h>

namespace WebCore {

PassOwnPtr<FlowThreadController> FlowThreadController::create(RenderView* view)
{
    return adoptPtr(new FlowThreadController(view));
}

FlowThreadController::FlowThreadController(RenderView* view)
    : m_view(view)
    , m_currentRenderFlowThread(0)
    , m_isRenderNamedFlowThreadOrderDirty(false)
    , m_flowThreadsWithAutoLogicalHeightRegions(0)
{
}

FlowThreadController::~FlowThreadController()
{
}

RenderNamedFlowThread& FlowThreadController::ensureRenderFlowThreadWithName(const AtomicString& name)
{
    if (!m_renderNamedFlowThreadList)
        m_renderNamedFlowThreadList = adoptPtr(new RenderNamedFlowThreadList());
    else {
        for (RenderNamedFlowThreadList::iterator iter = m_renderNamedFlowThreadList->begin(); iter != m_renderNamedFlowThreadList->end(); ++iter) {
            RenderNamedFlowThread* flowRenderer = *iter;
            if (flowRenderer->flowThreadName() == name)
                return *flowRenderer;
        }
    }

    NamedFlowCollection* namedFlows = m_view->document().namedFlows();

    // Sanity check for the absence of a named flow in the "CREATED" state with the same name.
    ASSERT(!namedFlows->flowByName(name));

    auto flowRenderer = new RenderNamedFlowThread(m_view->document(), RenderFlowThread::createFlowThreadStyle(&m_view->style()), namedFlows->ensureFlowWithName(name));
    flowRenderer->initializeStyle();
    m_renderNamedFlowThreadList->add(flowRenderer);

    // Keep the flow renderer as a child of RenderView.
    m_view->addChild(flowRenderer);

    setIsRenderNamedFlowThreadOrderDirty(true);

    return *flowRenderer;
}

void FlowThreadController::styleDidChange()
{
    RenderStyle& viewStyle = m_view->style();
    for (RenderNamedFlowThreadList::iterator iter = m_renderNamedFlowThreadList->begin(); iter != m_renderNamedFlowThreadList->end(); ++iter) {
        RenderNamedFlowThread* flowRenderer = *iter;
        flowRenderer->setStyle(RenderFlowThread::createFlowThreadStyle(&viewStyle));
    }
}

void FlowThreadController::layoutRenderNamedFlowThreads()
{
    updateFlowThreadsChainIfNecessary();

    for (RenderNamedFlowThreadList::iterator iter = m_renderNamedFlowThreadList->begin(); iter != m_renderNamedFlowThreadList->end(); ++iter) {
        RenderNamedFlowThread* flowRenderer = *iter;
        flowRenderer->layoutIfNeeded();
    }
}

void FlowThreadController::registerNamedFlowContentElement(Element& contentElement, RenderNamedFlowThread& namedFlow)
{
    ASSERT(!m_mapNamedFlowContentElement.contains(&contentElement));
    ASSERT(!namedFlow.hasContentElement(contentElement));
    m_mapNamedFlowContentElement.add(&contentElement, &namedFlow);
    namedFlow.registerNamedFlowContentElement(contentElement);
}

void FlowThreadController::unregisterNamedFlowContentElement(Element& contentElement)
{
    auto it = m_mapNamedFlowContentElement.find(&contentElement);
    ASSERT(it != m_mapNamedFlowContentElement.end());
    ASSERT(it->value);
    ASSERT(it->value->hasContentElement(contentElement));
    it->value->unregisterNamedFlowContentElement(contentElement);
    m_mapNamedFlowContentElement.remove(&contentElement);
}

void FlowThreadController::updateFlowThreadsChainIfNecessary()
{
    ASSERT(m_renderNamedFlowThreadList);
    ASSERT(isAutoLogicalHeightRegionsCountConsistent());

    // Remove the left-over flow threads.
    RenderNamedFlowThreadList toRemoveList;
    for (RenderNamedFlowThreadList::iterator iter = m_renderNamedFlowThreadList->begin(); iter != m_renderNamedFlowThreadList->end(); ++iter) {
        RenderNamedFlowThread* flowRenderer = *iter;
        if (flowRenderer->isMarkedForDestruction())
            toRemoveList.add(flowRenderer);
    }

    if (toRemoveList.size() > 0)
        setIsRenderNamedFlowThreadOrderDirty(true);

    for (RenderNamedFlowThreadList::iterator iter = toRemoveList.begin(); iter != toRemoveList.end(); ++iter) {
        RenderNamedFlowThread* flowRenderer = *iter;
        m_renderNamedFlowThreadList->remove(flowRenderer);
        flowRenderer->destroy();
    }

    if (isRenderNamedFlowThreadOrderDirty()) {
        // Arrange the thread list according to dependencies.
        RenderNamedFlowThreadList sortedList;
        for (RenderNamedFlowThreadList::iterator iter = m_renderNamedFlowThreadList->begin(); iter != m_renderNamedFlowThreadList->end(); ++iter) {
            RenderNamedFlowThread* flowRenderer = *iter;
            if (sortedList.contains(flowRenderer))
                continue;
            flowRenderer->pushDependencies(sortedList);
            sortedList.add(flowRenderer);
        }
        m_renderNamedFlowThreadList->swap(sortedList);
        setIsRenderNamedFlowThreadOrderDirty(false);
    }
}

bool FlowThreadController::updateFlowThreadsNeedingLayout()
{
    bool needsTwoPassLayout = false;

    for (RenderNamedFlowThreadList::iterator iter = m_renderNamedFlowThreadList->begin(); iter != m_renderNamedFlowThreadList->end(); ++iter) {
        RenderNamedFlowThread* flowRenderer = *iter;
        ASSERT(!flowRenderer->needsTwoPhasesLayout());
        ASSERT(flowRenderer->inMeasureContentLayoutPhase());
        if (flowRenderer->needsLayout() && flowRenderer->hasAutoLogicalHeightRegions())
            needsTwoPassLayout = true;
    }

    if (needsTwoPassLayout)
        resetFlowThreadsWithAutoHeightRegions();

    return needsTwoPassLayout;
}

bool FlowThreadController::updateFlowThreadsNeedingTwoStepLayout()
{
    bool needsTwoPassLayout = false;

    for (RenderNamedFlowThreadList::iterator iter = m_renderNamedFlowThreadList->begin(); iter != m_renderNamedFlowThreadList->end(); ++iter) {
        RenderNamedFlowThread* flowRenderer = *iter;
        if (flowRenderer->needsTwoPhasesLayout()) {
            needsTwoPassLayout = true;
            break;
        }
    }

    if (needsTwoPassLayout)
        resetFlowThreadsWithAutoHeightRegions();

    return needsTwoPassLayout;
}

void FlowThreadController::resetFlowThreadsWithAutoHeightRegions()
{
    for (RenderNamedFlowThreadList::iterator iter = m_renderNamedFlowThreadList->begin(); iter != m_renderNamedFlowThreadList->end(); ++iter) {
        RenderNamedFlowThread* flowRenderer = *iter;
        if (flowRenderer->hasAutoLogicalHeightRegions()) {
            flowRenderer->markAutoLogicalHeightRegionsForLayout();
            flowRenderer->invalidateRegions();
        }
    }
}

void FlowThreadController::updateFlowThreadsIntoConstrainedPhase()
{
    // Walk the flow chain in reverse order to update the auto-height regions and compute correct sizes for the containing regions. Only after this we can
    // set the flow in the constrained layout phase.
    for (RenderNamedFlowThreadList::reverse_iterator iter = m_renderNamedFlowThreadList->rbegin(); iter != m_renderNamedFlowThreadList->rend(); ++iter) {
        RenderNamedFlowThread* flowRenderer = *iter;
        ASSERT(!flowRenderer->hasRegions() || flowRenderer->hasValidRegionInfo());
        flowRenderer->layoutIfNeeded();
        if (flowRenderer->hasAutoLogicalHeightRegions()) {
            ASSERT(flowRenderer->needsTwoPhasesLayout());
            flowRenderer->markAutoLogicalHeightRegionsForLayout();
        }
        flowRenderer->setLayoutPhase(RenderFlowThread::LayoutPhaseConstrained);
        flowRenderer->clearNeedsTwoPhasesLayout();
    }
}

void FlowThreadController::updateFlowThreadsIntoOverflowPhase()
{
    for (RenderNamedFlowThreadList::reverse_iterator iter = m_renderNamedFlowThreadList->rbegin(); iter != m_renderNamedFlowThreadList->rend(); ++iter) {
        RenderNamedFlowThread* flowRenderer = *iter;
        ASSERT(!flowRenderer->hasRegions() || flowRenderer->hasValidRegionInfo());
        ASSERT(!flowRenderer->needsTwoPhasesLayout());

        // In the overflow computation phase the flow threads start in the constrained phase even though optimizations didn't set the state before.
        flowRenderer->setLayoutPhase(RenderFlowThread::LayoutPhaseConstrained);

        flowRenderer->layoutIfNeeded();
        flowRenderer->markRegionsForOverflowLayoutIfNeeded();
        flowRenderer->setLayoutPhase(RenderFlowThread::LayoutPhaseOverflow);
    }
}

void FlowThreadController::updateFlowThreadsIntoMeasureContentPhase()
{
    for (RenderNamedFlowThreadList::iterator iter = m_renderNamedFlowThreadList->begin(); iter != m_renderNamedFlowThreadList->end(); ++iter) {
        RenderNamedFlowThread* flowRenderer = *iter;
        ASSERT(flowRenderer->inFinalLayoutPhase());

        flowRenderer->setLayoutPhase(RenderFlowThread::LayoutPhaseMeasureContent);
    }
}

void FlowThreadController::updateFlowThreadsIntoFinalPhase()
{
    for (RenderNamedFlowThreadList::reverse_iterator iter = m_renderNamedFlowThreadList->rbegin(); iter != m_renderNamedFlowThreadList->rend(); ++iter) {
        RenderNamedFlowThread* flowRenderer = *iter;
        flowRenderer->layoutIfNeeded();
        if (flowRenderer->needsTwoPhasesLayout()) {
            flowRenderer->markRegionsForOverflowLayoutIfNeeded();
            flowRenderer->clearNeedsTwoPhasesLayout();
        }
        flowRenderer->setLayoutPhase(RenderFlowThread::LayoutPhaseFinal);
    }
}

#if USE(ACCELERATED_COMPOSITING)
void FlowThreadController::updateRenderFlowThreadLayersIfNeeded()
{
    // Walk the flow chain in reverse order because RenderRegions might become RenderLayers for the following flow threads.
    for (RenderNamedFlowThreadList::reverse_iterator iter = m_renderNamedFlowThreadList->rbegin(); iter != m_renderNamedFlowThreadList->rend(); ++iter) {
        RenderNamedFlowThread* flowRenderer = *iter;
        flowRenderer->updateAllLayerToRegionMappingsIfNeeded();
    }
}
#endif

bool FlowThreadController::isContentElementRegisteredWithAnyNamedFlow(const Element& contentElement) const
{
    return m_mapNamedFlowContentElement.contains(&contentElement);
}

// Collect the fixed positioned layers that have the named flows as containing block
// These layers are painted and hit-tested starting from RenderView not from regions.
void FlowThreadController::collectFixedPositionedLayers(Vector<RenderLayer*>& fixedPosLayers) const
{
    for (RenderNamedFlowThreadList::const_iterator iter = m_renderNamedFlowThreadList->begin(); iter != m_renderNamedFlowThreadList->end(); ++iter) {
        RenderNamedFlowThread* flowRenderer = *iter;

        // If the named flow does not have any regions attached, a fixed element should not be
        // displayed even if the fixed element is positioned/sized by the viewport.
        if (!flowRenderer->hasRegions())
            continue;

        // Iterate over the fixed positioned elements in the flow thread
        TrackedRendererListHashSet* positionedDescendants = flowRenderer->positionedObjects();
        if (positionedDescendants) {
            TrackedRendererListHashSet::iterator end = positionedDescendants->end();
            for (TrackedRendererListHashSet::iterator it = positionedDescendants->begin(); it != end; ++it) {
                RenderBox* box = *it;
                if (!box->fixedPositionedWithNamedFlowContainingBlock())
                    continue;
                fixedPosLayers.append(box->layer());
            }
        }
    }
}

#ifndef NDEBUG
bool FlowThreadController::isAutoLogicalHeightRegionsCountConsistent() const
{
    if (!hasRenderNamedFlowThreads())
        return !hasFlowThreadsWithAutoLogicalHeightRegions();

    for (RenderNamedFlowThreadList::iterator iter = m_renderNamedFlowThreadList->begin(); iter != m_renderNamedFlowThreadList->end(); ++iter) {
        if (!(*iter)->isAutoLogicalHeightRegionsCountConsistent())
            return false;
    }

    return true;
}
#endif

} // namespace WebCore
