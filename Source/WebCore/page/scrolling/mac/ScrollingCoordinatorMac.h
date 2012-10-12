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

#ifndef ScrollingCoordinatorMac_h
#define ScrollingCoordinatorMac_h

#if ENABLE(THREADED_SCROLLING)

#include "ScrollingCoordinator.h"

namespace WebCore {

class ScrollingCoordinatorMac : public ScrollingCoordinator {
public:
    explicit ScrollingCoordinatorMac(Page*);
    virtual ~ScrollingCoordinatorMac();

    virtual void pageDestroyed();

    virtual ScrollingTree* scrollingTree() const;
    virtual void commitTreeStateIfNeeded();

    // Should be called whenever the given frame view has been laid out.
    virtual void frameViewLayoutUpdated(FrameView*);

    // Should be called whenever the root layer for the given frame view changes.
    virtual void frameViewRootLayerDidChange(FrameView*);

    // Should be called whenever the horizontal scrollbar layer for the given frame view changes.
    virtual void frameViewHorizontalScrollbarLayerDidChange(FrameView*, GraphicsLayer* horizontalScrollbarLayer);

    // Should be called whenever the vertical scrollbar layer for the given frame view changes.
    virtual void frameViewVerticalScrollbarLayerDidChange(FrameView*, GraphicsLayer* verticalScrollbarLayer);

    // Requests that the scrolling coordinator updates the scroll position of the given frame view. If this function returns true, it means that the
    // position will be updated asynchronously. If it returns false, the caller should update the scrolling position itself.
    virtual bool requestScrollPositionUpdate(FrameView*, const IntPoint&);

    // Handle the wheel event on the scrolling thread. Returns whether the event was handled or not.
    virtual bool handleWheelEvent(FrameView*, const PlatformWheelEvent&);

    // Dispatched by the scrolling tree whenever the main frame scroll position changes and the scroll layer position needs to be updated as well.
    virtual void updateMainFrameScrollPositionAndScrollLayerPosition();

    // These functions are used to indicate that a layer should be (or should not longer be) represented by a node
    // in the scrolling tree.
    virtual ScrollingNodeID attachToStateTree(ScrollingNodeID);
    virtual void detachFromStateTree(ScrollingNodeID);

    // This function wipes out the current tree.
    virtual void clearStateTree();

private:
    virtual void recomputeWheelEventHandlerCountForFrameView(FrameView*);
    virtual void setShouldUpdateScrollLayerPositionOnMainThread(MainThreadScrollingReasons);

    void ensureRootStateNodeForFrameView(FrameView*);
    ScrollingStateScrollingNode* stateNodeForID(ScrollingNodeID);

    struct ScrollParameters {
        ScrollElasticity horizontalScrollElasticity;
        ScrollElasticity verticalScrollElasticity;

        bool hasEnabledHorizontalScrollbar;
        bool hasEnabledVerticalScrollbar;

        ScrollbarMode horizontalScrollbarMode;
        ScrollbarMode verticalScrollbarMode;

        IntPoint scrollOrigin;

        IntRect viewportRect;
        IntSize contentsSize;
    };

    void setScrollParametersForNode(const ScrollParameters&, ScrollingStateScrollingNode*);
    void setScrollLayerForNode(GraphicsLayer*, ScrollingStateNode*);
    void setNonFastScrollableRegionForNode(const Region&, ScrollingStateScrollingNode*);
    void setWheelEventHandlerCountForNode(unsigned, ScrollingStateScrollingNode*);

    void updateMainFrameScrollLayerPosition();

    void scheduleTreeStateCommit();

    void scrollingStateTreeCommitterTimerFired(Timer<ScrollingCoordinatorMac>*);
    void commitTreeState();

    OwnPtr<ScrollingStateTree> m_scrollingStateTree;
    RefPtr<ScrollingTree> m_scrollingTree;
    Timer<ScrollingCoordinatorMac> m_scrollingStateTreeCommitterTimer;

    HashMap<ScrollingNodeID, ScrollingStateNode*> m_stateNodeMap;
};

} // namespace WebCore

#endif // ENABLE(THREADED_SCROLLING)

#endif // ScrollingCoordinatorMac_h
