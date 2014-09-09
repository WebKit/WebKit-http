/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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

#ifndef RemoteScrollingCoordinatorProxy_h
#define RemoteScrollingCoordinatorProxy_h

#if ENABLE(ASYNC_SCROLLING)

#include "MessageReceiver.h"
#include "RemoteScrollingCoordinator.h"
#include "RemoteScrollingTree.h"
#include <wtf/Noncopyable.h>
#include <wtf/RefPtr.h>

namespace WebCore {
class FloatPoint;
class PlatformWheelEvent;
}

namespace WebKit {

class RemoteLayerTreeHost;
class RemoteScrollingCoordinatorTransaction;
class RemoteScrollingTree;
class WebPageProxy;

class RemoteScrollingCoordinatorProxy {
    WTF_MAKE_NONCOPYABLE(RemoteScrollingCoordinatorProxy);
public:
    explicit RemoteScrollingCoordinatorProxy(WebPageProxy&);
    virtual ~RemoteScrollingCoordinatorProxy();
    
    // Inform the web process that the scroll position changed (called from the scrolling tree)
    void scrollingTreeNodeDidScroll(WebCore::ScrollingNodeID, const WebCore::FloatPoint& newScrollPosition, WebCore::SetOrSyncScrollingLayerPosition);
    void scrollingTreeNodeRequestsScroll(WebCore::ScrollingNodeID, const WebCore::FloatPoint& scrollPosition, bool representsProgrammaticScroll);

    bool isPointInNonFastScrollableRegion(const WebCore::IntPoint&) const;

    // Called externally when native views move around.
    void viewportChangedViaDelegatedScrolling(WebCore::ScrollingNodeID, const WebCore::FloatRect& fixedPositionRect, double scale);

    // FIXME: expose the tree and pass this to that?
    bool handleWheelEvent(const WebCore::PlatformWheelEvent&);
    
    WebCore::ScrollingNodeID rootScrollingNodeID() const;

    const RemoteLayerTreeHost* layerTreeHost() const;

    struct RequestedScrollInfo {
        bool requestsScrollPositionUpdate { };
        bool requestIsProgrammaticScroll { };
        WebCore::FloatPoint requestedScrollPosition;
    };
    void updateScrollingTree(const RemoteScrollingCoordinatorTransaction&, RequestedScrollInfo&);

    void setPropagatesMainFrameScrolls(bool propagatesMainFrameScrolls) { m_propagatesMainFrameScrolls = propagatesMainFrameScrolls; }
    bool propagatesMainFrameScrolls() const { return m_propagatesMainFrameScrolls; }
    bool hasFixedOrSticky() const { return m_scrollingTree->hasFixedOrSticky(); }

#if PLATFORM(IOS)
    WebCore::FloatRect customFixedPositionRect() const;
    void scrollingTreeNodeWillStartPanGesture();
    void scrollingTreeNodeWillStartScroll();
    void scrollingTreeNodeDidEndScroll();
#endif

private:
    void connectStateNodeLayers(WebCore::ScrollingStateTree&, const RemoteLayerTreeHost&);

    WebPageProxy& m_webPageProxy;
    RefPtr<RemoteScrollingTree> m_scrollingTree;
    RequestedScrollInfo* m_requestedScrollInfo;
    bool m_propagatesMainFrameScrolls;
};

} // namespace WebKit

#endif // ENABLE(ASYNC_SCROLLING)

#endif // RemoteScrollingCoordinatorProxy_h
