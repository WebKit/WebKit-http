/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#ifndef ScrollingCoordinatorChromium_h
#define ScrollingCoordinatorChromium_h

#include "ScrollingCoordinator.h"

namespace WebKit {
class WebScrollbarLayer;
class WebLayer;
}

namespace WebCore {

class Scrollbar;
class ScrollingCoordinatorPrivate;

class ScrollingCoordinatorChromium : public ScrollingCoordinator {
public:
    explicit ScrollingCoordinatorChromium(Page*);
    virtual ~ScrollingCoordinatorChromium();

    // Should be called whenever the given frame view has been laid out.
    virtual void frameViewLayoutUpdated(FrameView*);

    // Should be called whenever the root layer for the given frame view changes.
    virtual void frameViewRootLayerDidChange(FrameView*);

    // Should be called whenever the horizontal scrollbar layer for the given frame view changes.
    virtual void frameViewHorizontalScrollbarLayerDidChange(FrameView*, GraphicsLayer*);

    // Should be called whenever the vertical scrollbar layer for the given frame view changes.
    virtual void frameViewVerticalScrollbarLayerDidChange(FrameView*, GraphicsLayer*);

    // Return whether this scrolling coordinator can keep fixed position layers fixed to their
    // containers while scrolling.
    virtual bool supportsFixedPositionLayers() const { return true; }

    // Mark/unmark a layer as a container for fixed position layers.
    virtual void setLayerIsContainerForFixedPositionLayers(GraphicsLayer*, bool);

    // Attach/detach layer position to ancestor fixed position container.
    virtual void setLayerIsFixedToContainerLayer(GraphicsLayer*, bool);

    // Should be called whenever the scrollable layer for the given scroll area changes.
    virtual void scrollableAreaScrollLayerDidChange(ScrollableArea*, GraphicsLayer*);

private:
    virtual void recomputeWheelEventHandlerCountForFrameView(FrameView*);
    virtual void setShouldUpdateScrollLayerPositionOnMainThread(MainThreadScrollingReasons);

    void setScrollLayer(GraphicsLayer*);
    void setNonFastScrollableRegion(const Region&);
    void setWheelEventHandlerCount(unsigned);
    PassOwnPtr<WebKit::WebScrollbarLayer> createScrollbarLayer(Scrollbar*, WebKit::WebLayer* scrollLayer, GraphicsLayer* scrollbarGraphicsLayer, FrameView*);

    ScrollingCoordinatorPrivate* m_private;
};

} // namespace WebCore

#endif // ScrollingCoordinatorChromium_h
