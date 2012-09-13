/*
 * Copyright (C) 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "InRegionScroller.h"

#include "BackingStoreClient.h"
#include "Frame.h"
#include "HTMLFrameOwnerElement.h"
#include "HitTestResult.h"
#include "InRegionScrollableArea.h"
#include "InRegionScroller_p.h"
#include "LayerCompositingThread.h"
#include "LayerWebKitThread.h"
#include "Page.h"
#include "RenderBox.h"
#include "RenderLayer.h"
#include "RenderLayerBacking.h"
#include "RenderObject.h"
#include "RenderView.h"
#include "SelectionHandler.h"
#include "WebPage_p.h"

using namespace WebCore;

namespace BlackBerry {
namespace WebKit {

static bool canScrollInnerFrame(Frame*);
static RenderLayer* parentLayer(RenderLayer*);
static bool isNonRenderViewFixedPositionedContainer(RenderLayer*);

InRegionScroller::InRegionScroller(WebPagePrivate* webPagePrivate)
    : d(new InRegionScrollerPrivate(webPagePrivate))
{
    ASSERT(webPagePrivate);
}

InRegionScroller::~InRegionScroller()
{
    delete d;
}

bool InRegionScroller::setScrollPositionCompositingThread(unsigned camouflagedLayer, const Platform::IntPoint& scrollPosition)
{
    ASSERT(Platform::userInterfaceThreadMessageClient()->isCurrentThread());

    // FIXME: Negative values won't work with map{To,From}Transform methods.
    return d->setScrollPositionCompositingThread(camouflagedLayer, d->m_webPage->mapFromTransformed(scrollPosition));
}

bool InRegionScroller::setScrollPositionWebKitThread(unsigned camouflagedLayer, const Platform::IntPoint& scrollPosition, bool supportsAcceleratedScrolling)
{
    ASSERT(Platform::webKitThreadMessageClient()->isCurrentThread());

    // FIXME: Negative values won't work with map{To,From}Transform methods.
    return d->setScrollPositionWebKitThread(camouflagedLayer, d->m_webPage->mapFromTransformed(scrollPosition), supportsAcceleratedScrolling);
}

InRegionScrollerPrivate::InRegionScrollerPrivate(WebPagePrivate* webPagePrivate)
    : m_webPage(webPagePrivate)
    , m_needsActiveScrollableAreaCalculation(false)
{
}

void InRegionScrollerPrivate::reset()
{
    m_needsActiveScrollableAreaCalculation = false;
    for (size_t i = 0; i < m_activeInRegionScrollableAreas.size(); ++i)
        delete m_activeInRegionScrollableAreas[i];
    m_activeInRegionScrollableAreas.clear();
}

bool InRegionScrollerPrivate::isActive() const
{
    return m_activeInRegionScrollableAreas.size() > 0;
}

void InRegionScrollerPrivate::clearDocumentData(const Document* documentGoingAway)
{
    if (m_needsActiveScrollableAreaCalculation) {
        reset();
        return;
    }

    InRegionScrollableArea* scrollableArea = static_cast<InRegionScrollableArea*>(m_activeInRegionScrollableAreas[0]);
    ASSERT(scrollableArea);
    Node* node = scrollableArea->layer()->enclosingElement();
    if (node && node->document() == documentGoingAway)
        reset();
}

bool InRegionScrollerPrivate::setScrollPositionCompositingThread(unsigned camouflagedLayer, const WebCore::IntPoint& scrollPosition)
{
    LayerCompositingThread* scrollLayer = reinterpret_cast<LayerWebKitThread*>(camouflagedLayer)->layerCompositingThread();

    // FIXME: Clamp maximum and minimum scroll positions as a last attempt to fix round errors.
    FloatPoint anchor;
    if (scrollLayer->override()->isAnchorPointSet())
        anchor = scrollLayer->override()->anchorPoint();
    else
        anchor = scrollLayer->anchorPoint();

    FloatSize bounds;
    if (scrollLayer->override()->isBoundsSet())
        bounds = scrollLayer->override()->bounds();
    else
        bounds = scrollLayer->bounds();

    // Position is offset on the layer by the layer anchor point.
    FloatPoint layerPosition(-scrollPosition.x() + anchor.x() * bounds.width(),
                             -scrollPosition.y() + anchor.y() * bounds.height());

    scrollLayer->override()->setPosition(FloatPoint(layerPosition.x(), layerPosition.y()));

    // The client is going to blitVisibleContens, which allow us benefit from "defer blits" technique.
    return true;
}

bool InRegionScrollerPrivate::setScrollPositionWebKitThread(unsigned camouflagedLayer, const WebCore::IntPoint& scrollPosition, bool supportsAcceleratedScrolling)
{
    RenderLayer* layer = 0;

    if (supportsAcceleratedScrolling) {
        LayerWebKitThread* layerWebKitThread = reinterpret_cast<LayerWebKitThread*>(camouflagedLayer);
        ASSERT(layerWebKitThread);
        if (layerWebKitThread->owner()) {
            GraphicsLayer* graphicsLayer = layerWebKitThread->owner();
            RenderLayerBacking* backing = static_cast<RenderLayerBacking*>(graphicsLayer->client());
            layer = backing->owningLayer();
        }
    } else {
        Node* node = reinterpret_cast<Node*>(camouflagedLayer);
        ASSERT(node);
        if (!node->renderer())
            return false;

        layer = node->renderer()->enclosingLayer();
    }

    if (!layer)
        return false;

    calculateActiveAndShrinkCachedScrollableAreas(layer);

    // FIXME: Clamp maximum and minimum scroll positions as a last attempt to fix round errors.
    return setLayerScrollPosition(layer, scrollPosition);
}

void InRegionScrollerPrivate::calculateActiveAndShrinkCachedScrollableAreas(RenderLayer* layer)
{
    if (!m_needsActiveScrollableAreaCalculation)
        return;

    ASSERT(layer);
    std::vector<Platform::ScrollViewBase*>::iterator end = m_activeInRegionScrollableAreas.end();
    std::vector<Platform::ScrollViewBase*>::iterator it = m_activeInRegionScrollableAreas.begin();
    while (it != end) {
        InRegionScrollableArea* curr = static_cast<InRegionScrollableArea*>(*it);

        if (layer == curr->layer()) {
            ++it;
            continue;
        }

        delete *it;
        it = m_activeInRegionScrollableAreas.erase(it);
        // ::erase invalidates the iterators.
        end = m_activeInRegionScrollableAreas.end();
    }

    ASSERT(m_activeInRegionScrollableAreas.size() == 1);
    m_needsActiveScrollableAreaCalculation = false;
}

void InRegionScrollerPrivate::calculateInRegionScrollableAreasForPoint(const WebCore::IntPoint& point)
{
    ASSERT(m_activeInRegionScrollableAreas.empty());
    m_needsActiveScrollableAreaCalculation = false;

    HitTestResult result = m_webPage->m_mainFrame->eventHandler()->hitTestResultAtPoint(m_webPage->mapFromViewportToContents(point), false /*allowShadowContent*/);
    Node* node = result.innerNonSharedNode();
    if (!node || !node->renderer())
        return;

    RenderLayer* layer = node->renderer()->enclosingLayer();
    do {
        RenderObject* renderer = layer->renderer();

        if (renderer->isRenderView()) {
            if (RenderView* renderView = toRenderView(renderer)) {
                FrameView* view = renderView->frameView();
                if (!view) {
                    reset();
                    return;
                }

                if (canScrollInnerFrame(view->frame())) {
                    pushBackInRegionScrollable(new InRegionScrollableArea(m_webPage, layer));
                    continue;
                }
            }
        } else if (canScrollRenderBox(layer->renderBox())) {
            pushBackInRegionScrollable(new InRegionScrollableArea(m_webPage, layer));
            continue;
        }

        // If we run into a fix positioned layer, set the last scrollable in-region object
        // as not able to propagate scroll to its parent scrollable.
        if (isNonRenderViewFixedPositionedContainer(layer) && m_activeInRegionScrollableAreas.size()) {
            Platform::ScrollViewBase* end = m_activeInRegionScrollableAreas.back();
            end->setCanPropagateScrollingToEnclosingScrollable(false);
        }

    } while (layer = parentLayer(layer));

    if (m_activeInRegionScrollableAreas.empty())
        return;

    m_needsActiveScrollableAreaCalculation = true;

    // Post-calculate the visible window rects in reverse hit test order so
    // we account for all and any clipping rects.
    WebCore::IntRect recursiveClippingRect(WebCore::IntPoint::zero(), m_webPage->transformedViewportSize());

    std::vector<Platform::ScrollViewBase*>::reverse_iterator rend = m_activeInRegionScrollableAreas.rend();
    for (std::vector<Platform::ScrollViewBase*>::reverse_iterator rit = m_activeInRegionScrollableAreas.rbegin(); rit != rend; ++rit) {

        InRegionScrollableArea* curr = static_cast<InRegionScrollableArea*>(*rit);
        RenderLayer* layer = curr->layer();

        if (layer && layer->renderer()->isRenderView()) { // #document case
            FrameView* view = toRenderView(layer->renderer())->frameView();
            ASSERT(view);
            ASSERT(canScrollInnerFrame(view->frame()));

            WebCore::IntRect frameWindowRect = m_webPage->mapToTransformed(m_webPage->getRecursiveVisibleWindowRect(view));
            frameWindowRect.intersect(recursiveClippingRect);
            curr->setVisibleWindowRect(frameWindowRect);
            recursiveClippingRect = frameWindowRect;

        } else { // RenderBox-based elements case (scrollable boxes (div's, p's, textarea's, etc)).

            RenderBox* box = layer->renderBox();
            ASSERT(box);
            ASSERT(canScrollRenderBox(box));

            WebCore::IntRect visibleWindowRect = enclosingIntRect(box->absoluteClippedOverflowRect());
            visibleWindowRect = box->frame()->view()->contentsToWindow(visibleWindowRect);
            visibleWindowRect = m_webPage->mapToTransformed(visibleWindowRect);
            visibleWindowRect.intersect(recursiveClippingRect);

            curr->setVisibleWindowRect(visibleWindowRect);
            recursiveClippingRect = visibleWindowRect;
        }
    }
}

const std::vector<Platform::ScrollViewBase*>& InRegionScrollerPrivate::activeInRegionScrollableAreas() const
{
    return m_activeInRegionScrollableAreas;
}

bool InRegionScrollerPrivate::setLayerScrollPosition(RenderLayer* layer, const IntPoint& scrollPosition)
{
    RenderObject* layerRenderer = layer->renderer();
    ASSERT(layerRenderer);

    if (layerRenderer->isRenderView()) { // #document case.
        FrameView* view = toRenderView(layerRenderer)->frameView();
        ASSERT(view);

        Frame* frame = view->frame();
        ASSERT_UNUSED(frame, frame);
        ASSERT(canScrollInnerFrame(frame));

        view->setCanBlitOnScroll(false);

        BackingStoreClient* backingStoreClient = m_webPage->backingStoreClientForFrame(view->frame());
        if (backingStoreClient) {
            backingStoreClient->setIsClientGeneratedScroll(true);
            backingStoreClient->setIsScrollNotificationSuppressed(true);
        }

        view->setScrollPosition(scrollPosition);

        if (backingStoreClient) {
            backingStoreClient->setIsClientGeneratedScroll(false);
            backingStoreClient->setIsScrollNotificationSuppressed(false);
        }

    } else {

        // RenderBox-based elements case (scrollable boxes (div's, p's, textarea's, etc)).
        layer->scrollToOffset(toSize(scrollPosition));
    }

    m_webPage->m_selectionHandler->selectionPositionChanged();
    // FIXME: We have code in place to handle scrolling and clipping tap highlight
    // on in-region scrolling. As soon as it is fast enough (i.e. we have it backed by
    // a backing store), we can reliably make use of it in the real world.
    // m_touchEventHandler->drawTapHighlight();
    return true;
}

void InRegionScrollerPrivate::adjustScrollDelta(const WebCore::IntPoint& maxOffset, const WebCore::IntPoint& currentOffset, WebCore::IntSize& delta) const
{
    if (currentOffset.x() + delta.width() > maxOffset.x())
        delta.setWidth(std::min(maxOffset.x() - currentOffset.x(), delta.width()));

    if (currentOffset.x() + delta.width() < 0)
        delta.setWidth(std::max(-currentOffset.x(), delta.width()));

    if (currentOffset.y() + delta.height() > maxOffset.y())
        delta.setHeight(std::min(maxOffset.y() - currentOffset.y(), delta.height()));

    if (currentOffset.y() + delta.height() < 0)
        delta.setHeight(std::max(-currentOffset.y(), delta.height()));
}

static bool canScrollInnerFrame(Frame* frame)
{
    if (!frame || !frame->view())
        return false;

    // Not having an owner element means that we are on the mainframe.
    if (!frame->ownerElement())
        return false;

    ASSERT(frame != frame->page()->mainFrame());

    IntSize visibleSize = frame->view()->visibleContentRect().size();
    IntSize contentsSize = frame->view()->contentsSize();

    bool canBeScrolled = contentsSize.height() > visibleSize.height() || contentsSize.width() > visibleSize.width();

    // Lets also consider the 'overflow-{x,y} property set directly to the {i}frame tag.
    return canBeScrolled && (frame->ownerElement()->scrollingMode() != ScrollbarAlwaysOff);
}

// The RenderBox::canbeScrolledAndHasScrollableArea method returns true for the
// following scenario, for example:
// (1) a div that has a vertical overflow but no horizontal overflow
//     with overflow-y: hidden and overflow-x: auto set.
// The version below fixes it.
// FIXME: Fix RenderBox::canBeScrolledAndHasScrollableArea method instead.
bool InRegionScrollerPrivate::canScrollRenderBox(RenderBox* box)
{
    if (!box)
        return false;

    // We use this to make non-overflown contents layers to actually
    // be overscrollable.
    if (box->layer() && box->layer()->usesCompositedScrolling()) {
        if (box->style()->overflowScrolling() == OSBlackberryTouch)
            return true;
    }

    if (!box->hasOverflowClip())
        return false;

    if (box->scrollHeight() == box->clientHeight() && box->scrollWidth() == box->clientWidth())
        return false;

    if (box->scrollsOverflowX() && (box->scrollWidth() != box->clientWidth())
        || box->scrollsOverflowY() && (box->scrollHeight() != box->clientHeight()))
        return true;

    Node* node = box->node();
    return node && (node->rendererIsEditable() || node->isDocumentNode());
}

static RenderLayer* parentLayer(RenderLayer* layer)
{
    ASSERT(layer);
    if (layer->parent())
        return layer->parent();

    RenderObject* renderer = layer->renderer();
    if (renderer->document() && renderer->document()->ownerElement() && renderer->document()->ownerElement()->renderer())
        return renderer->document()->ownerElement()->renderer()->enclosingLayer();

    return 0;
}

static bool isNonRenderViewFixedPositionedContainer(RenderLayer* layer)
{
    RenderObject* o = layer->renderer();
    if (o->isRenderView())
        return false;

    return o->isOutOfFlowPositioned() && o->style()->position() == FixedPosition;
}

void InRegionScrollerPrivate::pushBackInRegionScrollable(InRegionScrollableArea* scrollableArea)
{
    ASSERT(!scrollableArea->isNull());

    scrollableArea->setCanPropagateScrollingToEnclosingScrollable(!isNonRenderViewFixedPositionedContainer(scrollableArea->layer()));
    m_activeInRegionScrollableAreas.push_back(scrollableArea);
}

}
}
