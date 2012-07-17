/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
#include "LayerChromium.h"
#include "Region.h"
#include "RenderLayerCompositor.h"
#include "RenderView.h"
#include "ScrollbarLayerChromium.h"
#include "ScrollbarTheme.h"
#include "cc/CCProxy.h"
#include <public/WebScrollableLayer.h>

using WebKit::WebLayer;
using WebKit::WebScrollableLayer;

namespace WebCore {

class ScrollingCoordinatorPrivate {
WTF_MAKE_NONCOPYABLE(ScrollingCoordinatorPrivate);
public:
    ScrollingCoordinatorPrivate() { }
    ~ScrollingCoordinatorPrivate() { }

    void setScrollLayer(WebScrollableLayer layer)
    {
        m_scrollLayer = layer;

        int id = layer.isNull() ? 0 : layer.unwrap<LayerChromium>()->id();
        if (!m_horizontalScrollbarLayer.isNull())
            m_horizontalScrollbarLayer.unwrap<ScrollbarLayerChromium>()->setScrollLayerId(id);
        if (!m_verticalScrollbarLayer.isNull())
            m_verticalScrollbarLayer.unwrap<ScrollbarLayerChromium>()->setScrollLayerId(id);
    }

    void setHorizontalScrollbarLayer(WebLayer layer)
    {
        m_horizontalScrollbarLayer = layer;
    }

    void setVerticalScrollbarLayer(WebLayer layer)
    {
        m_verticalScrollbarLayer = layer;
    }

    bool hasScrollLayer() const { return !m_scrollLayer.isNull(); }
    WebScrollableLayer scrollLayer() const { return m_scrollLayer; }

private:
    WebScrollableLayer m_scrollLayer;
    WebLayer m_horizontalScrollbarLayer;
    WebLayer m_verticalScrollbarLayer;
};

PassRefPtr<ScrollingCoordinator> ScrollingCoordinator::create(Page* page)
{
    RefPtr<ScrollingCoordinator> coordinator(adoptRef(new ScrollingCoordinator(page)));
    coordinator->m_private = new ScrollingCoordinatorPrivate;
    return coordinator.release();
}

ScrollingCoordinator::~ScrollingCoordinator()
{
    ASSERT(!m_page);
    delete m_private;
}

static GraphicsLayer* scrollLayerForFrameView(FrameView* frameView)
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
    return 0;
#endif
}

static WebLayer createScrollbarLayer(Scrollbar* scrollbar, WebScrollableLayer scrollLayer, GraphicsLayer* scrollbarGraphicsLayer, FrameView* frameView)
{
    ASSERT(scrollbar);
    ASSERT(scrollbarGraphicsLayer);

    if (scrollLayer.isNull()) {
        // FIXME: sometimes we get called before setScrollLayer, workaround by finding the scroll layout ourselves.
        scrollLayer = WebScrollableLayer(scrollLayerForFrameView(frameView)->platformLayer());
        ASSERT(!scrollLayer.isNull());
    }

    // Root layer non-overlay scrollbars should be marked opaque to disable
    // blending.
    bool isOpaqueRootScrollbar = !frameView->parent() && !scrollbar->isOverlayScrollbar();
    if (!scrollbarGraphicsLayer->contentsOpaque())
        scrollbarGraphicsLayer->setContentsOpaque(isOpaqueRootScrollbar);

    // FIXME: Mac scrollbar themes are not thread-safe to paint.
    // FIXME: Win scrollbars on XP Classic themes do not paint valid alpha
    // values due to GDI. This needs to be fixed in theme code before it
    // can be turned on here.
    bool platformSupported = true;
#if OS(DARWIN) || OS(WINDOWS)
    platformSupported = false;
#endif

    if (!platformSupported || scrollbar->isOverlayScrollbar()) {
        scrollbarGraphicsLayer->setContentsToMedia(0);
        scrollbarGraphicsLayer->setDrawsContent(true);
        return WebLayer();
    }

    RefPtr<ScrollbarLayerChromium> scrollbarLayer = ScrollbarLayerChromium::create(scrollbar, scrollLayer.unwrap<LayerChromium>()->id());
    scrollbarGraphicsLayer->setContentsToMedia(scrollbarLayer.get());
    scrollbarGraphicsLayer->setDrawsContent(false);
    scrollbarLayer->setOpaque(scrollbarGraphicsLayer->contentsOpaque());

    return WebLayer(scrollbarLayer.release());
}

void ScrollingCoordinator::frameViewHorizontalScrollbarLayerDidChange(FrameView* frameView, GraphicsLayer* horizontalScrollbarLayer)
{
    if (!horizontalScrollbarLayer || !coordinatesScrollingForFrameView(frameView))
        return;

    m_private->setHorizontalScrollbarLayer(createScrollbarLayer(frameView->horizontalScrollbar(), m_private->scrollLayer(), horizontalScrollbarLayer, frameView));
}

void ScrollingCoordinator::frameViewVerticalScrollbarLayerDidChange(FrameView* frameView, GraphicsLayer* verticalScrollbarLayer)
{
    if (!verticalScrollbarLayer || !coordinatesScrollingForFrameView(frameView))
        return;

    m_private->setVerticalScrollbarLayer(createScrollbarLayer(frameView->verticalScrollbar(), m_private->scrollLayer(), verticalScrollbarLayer, frameView));
}

void ScrollingCoordinator::setScrollLayer(GraphicsLayer* scrollLayer)
{
    m_private->setScrollLayer(WebScrollableLayer(scrollLayer ? scrollLayer->platformLayer() : 0));
}

void ScrollingCoordinator::setNonFastScrollableRegion(const Region& region)
{
    if (m_private->hasScrollLayer())
        m_private->scrollLayer().unwrap<LayerChromium>()->setNonFastScrollableRegion(region);
}

void ScrollingCoordinator::setScrollParameters(const ScrollParameters&)
{
    // FIXME: Implement!
}

void ScrollingCoordinator::setWheelEventHandlerCount(unsigned wheelEventHandlerCount)
{
    if (m_private->hasScrollLayer())
        m_private->scrollLayer().setHaveWheelEventHandlers(wheelEventHandlerCount > 0);
}

void ScrollingCoordinator::setShouldUpdateScrollLayerPositionOnMainThread(bool should)
{
    if (m_private->hasScrollLayer())
        m_private->scrollLayer().setShouldScrollOnMainThread(should);
}

bool ScrollingCoordinator::supportsFixedPositionLayers() const
{
    return true;
}

void ScrollingCoordinator::setLayerIsContainerForFixedPositionLayers(GraphicsLayer* layer, bool enable)
{
    if (LayerChromium* platformLayer = layer->platformLayer())
        platformLayer->setIsContainerForFixedPositionLayers(enable);
}

void ScrollingCoordinator::setLayerIsFixedToContainerLayer(GraphicsLayer* layer, bool enable)
{
    if (LayerChromium* platformLayer = layer->platformLayer())
        platformLayer->setFixedToContainerLayer(enable);
}

}
