/*
 * Copyright (C) 2008, 2011, 2014 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef ScrollableArea_h
#define ScrollableArea_h

#include "Scrollbar.h"
#include <wtf/Vector.h>

namespace WebCore {

class FloatPoint;
class GraphicsContext;
class LayoutPoint;
class LayoutSize;
class PlatformTouchEvent;
class PlatformWheelEvent;
class ScrollAnimator;
class GraphicsLayer;
class TiledBacking;

class ScrollableArea {
public:
    bool scroll(ScrollDirection, ScrollGranularity, float multiplier = 1);
    void scrollToOffsetWithoutAnimation(const FloatPoint&);
    void scrollToOffsetWithoutAnimation(ScrollbarOrientation, float offset);

    // Should be called when the scroll position changes externally, for example if the scroll layer position
    // is updated on the scrolling thread and we need to notify the main thread.
    void notifyScrollPositionChanged(const IntPoint&);

    // Allows subclasses to handle scroll position updates themselves. If this member function
    // returns true, the scrollable area won't actually update the scroll position and instead
    // expect it to happen sometime in the future.
    virtual bool requestScrollPositionUpdate(const IntPoint&) { return false; }

    bool handleWheelEvent(const PlatformWheelEvent&);

#if ENABLE(TOUCH_EVENTS)
    virtual bool isTouchScrollable() const { return false; }
    virtual bool handleTouchEvent(const PlatformTouchEvent&);
#endif

#if PLATFORM(IOS)
    virtual bool isOverflowScroll() const { return false; }
    virtual void didStartScroll() { }
    virtual void didEndScroll() { }
    virtual void didUpdateScroll() { }
#endif
    virtual void setIsUserScroll(bool) { }

    // Functions for controlling if you can scroll past the end of the document.
    bool constrainsScrollingToContentEdge() const { return m_constrainsScrollingToContentEdge; }
    void setConstrainsScrollingToContentEdge(bool constrainsScrollingToContentEdge) { m_constrainsScrollingToContentEdge = constrainsScrollingToContentEdge; }

    void setVerticalScrollElasticity(ScrollElasticity scrollElasticity) { m_verticalScrollElasticity = scrollElasticity; }
    ScrollElasticity verticalScrollElasticity() const { return static_cast<ScrollElasticity>(m_verticalScrollElasticity); }

    void setHorizontalScrollElasticity(ScrollElasticity scrollElasticity) { m_horizontalScrollElasticity = scrollElasticity; }
    ScrollElasticity horizontalScrollElasticity() const { return static_cast<ScrollElasticity>(m_horizontalScrollElasticity); }

    bool inLiveResize() const { return m_inLiveResize; }
    virtual void willStartLiveResize();
    virtual void willEndLiveResize();

    void contentAreaWillPaint() const;
    void mouseEnteredContentArea() const;
    void mouseExitedContentArea() const;
    void mouseMovedInContentArea() const;
    void mouseEnteredScrollbar(Scrollbar*) const;
    void mouseExitedScrollbar(Scrollbar*) const;
    void contentAreaDidShow() const;
    void contentAreaDidHide() const;

    void lockOverlayScrollbarStateToHidden(bool shouldLockState) const;
    bool scrollbarsCanBeActive() const;

    virtual void didAddScrollbar(Scrollbar*, ScrollbarOrientation);
    virtual void willRemoveScrollbar(Scrollbar*, ScrollbarOrientation);

    virtual void contentsResized();

    bool hasOverlayScrollbars() const;
    virtual void setScrollbarOverlayStyle(ScrollbarOverlayStyle);
    ScrollbarOverlayStyle scrollbarOverlayStyle() const { return static_cast<ScrollbarOverlayStyle>(m_scrollbarOverlayStyle); }

    // This getter will create a ScrollAnimator if it doesn't already exist.
    ScrollAnimator* scrollAnimator() const;

    // This getter will return null if the ScrollAnimator hasn't been created yet.
    ScrollAnimator* existingScrollAnimator() const { return m_scrollAnimator.get(); }

    const IntPoint& scrollOrigin() const { return m_scrollOrigin; }
    bool scrollOriginChanged() const { return m_scrollOriginChanged; }

    virtual bool isActive() const = 0;
    virtual int scrollSize(ScrollbarOrientation) const = 0;
    virtual int scrollPosition(Scrollbar*) const = 0;
    virtual void invalidateScrollbar(Scrollbar*, const IntRect&);
    virtual bool isScrollCornerVisible() const = 0;
    virtual IntRect scrollCornerRect() const = 0;
    virtual void invalidateScrollCorner(const IntRect&);

    virtual bool updatesScrollLayerPositionOnMainThread() const = 0;

    virtual bool forceUpdateScrollbarsOnMainThreadForPerformanceTesting() const = 0;

    // Convert points and rects between the scrollbar and its containing view.
    // The client needs to implement these in order to be aware of layout effects
    // like CSS transforms.
    virtual IntRect convertFromScrollbarToContainingView(const Scrollbar* scrollbar, const IntRect& scrollbarRect) const
    {
        return scrollbar->Widget::convertToContainingView(scrollbarRect);
    }
    virtual IntRect convertFromContainingViewToScrollbar(const Scrollbar* scrollbar, const IntRect& parentRect) const
    {
        return scrollbar->Widget::convertFromContainingView(parentRect);
    }
    virtual IntPoint convertFromScrollbarToContainingView(const Scrollbar* scrollbar, const IntPoint& scrollbarPoint) const
    {
        return scrollbar->Widget::convertToContainingView(scrollbarPoint);
    }
    virtual IntPoint convertFromContainingViewToScrollbar(const Scrollbar* scrollbar, const IntPoint& parentPoint) const
    {
        return scrollbar->Widget::convertFromContainingView(parentPoint);
    }

    virtual Scrollbar* horizontalScrollbar() const { return 0; }
    virtual Scrollbar* verticalScrollbar() const { return 0; }

    virtual IntPoint scrollPosition() const;
    virtual IntPoint minimumScrollPosition() const;
    virtual IntPoint maximumScrollPosition() const;
    virtual bool scrolledToTop() const;
    virtual bool scrolledToBottom() const;
    virtual bool scrolledToLeft() const;
    virtual bool scrolledToRight() const;

    bool isScrolledProgrammatically() const { return m_scrolledProgrammatically; }
    void setScrolledProgrammatically(bool state) { m_scrolledProgrammatically = state; }

    enum VisibleContentRectIncludesScrollbars { ExcludeScrollbars, IncludeScrollbars };
    enum VisibleContentRectBehavior {
        ContentsVisibleRect,
#if PLATFORM(IOS)
        LegacyIOSDocumentViewRect,
        LegacyIOSDocumentVisibleRect = LegacyIOSDocumentViewRect
#else
        LegacyIOSDocumentVisibleRect = ContentsVisibleRect
#endif
    };

    IntRect visibleContentRect(VisibleContentRectBehavior = ContentsVisibleRect) const;
    IntRect visibleContentRectIncludingScrollbars(VisibleContentRectBehavior = ContentsVisibleRect) const;

    int visibleWidth() const { return visibleSize().width(); }
    int visibleHeight() const { return visibleSize().height(); }
    virtual IntSize visibleSize() const = 0;

    virtual IntSize contentsSize() const = 0;
    virtual IntSize overhangAmount() const { return IntSize(); }
    virtual IntPoint lastKnownMousePosition() const { return IntPoint(); }
    virtual bool isHandlingWheelEvent() const { return false; }

    virtual int headerHeight() const { return 0; }
    virtual int footerHeight() const { return 0; }

    // The totalContentsSize() is equivalent to the contentsSize() plus the header and footer heights.
    IntSize totalContentsSize() const;

    virtual bool shouldSuspendScrollAnimations() const { return true; }
    virtual void scrollbarStyleChanged(int /*newStyle*/, bool /*forceUpdate*/) { }
    virtual void setVisibleScrollerThumbRect(const IntRect&) { }
    
    // Note that this only returns scrollable areas that can actually be scrolled.
    virtual ScrollableArea* enclosingScrollableArea() const = 0;

    // Returns the bounding box of this scrollable area, in the coordinate system of the enclosing scroll view.
    virtual IntRect scrollableAreaBoundingBox() const = 0;

    virtual bool isRubberBandInProgress() const { return false; }

    virtual bool scrollAnimatorEnabled() const { return false; }

    // NOTE: Only called from Internals for testing.
    void setScrollOffsetFromInternals(const IntPoint&);

    static LayoutPoint constrainScrollPositionForOverhang(const LayoutRect& visibleContentRect, const LayoutSize& totalContentsSize, const LayoutPoint& scrollPosition, const LayoutPoint& scrollOrigin, int headerHeight, int footetHeight);
    LayoutPoint constrainScrollPositionForOverhang(const LayoutPoint& scrollPosition);

    // Computes the double value for the scrollbar's current position and the current overhang amount.
    // This function is static so that it can be called from the main thread or the scrolling thread.
    static void computeScrollbarValueAndOverhang(float currentPosition, float totalSize, float visibleSize, float& doubleValue, float& overhangAmount);

    // Let subclasses provide a way of asking for and servicing scroll
    // animations.
    virtual bool scheduleAnimation() { return false; }
    void serviceScrollAnimations();

#if PLATFORM(IOS)
    bool isHorizontalScrollerPinnedToMinimumPosition() const { return !horizontalScrollbar() || scrollPosition(horizontalScrollbar()) <= minimumScrollPosition().x(); }
    bool isHorizontalScrollerPinnedToMaximumPosition() const { return !horizontalScrollbar() || scrollPosition(horizontalScrollbar()) >= maximumScrollPosition().x(); }
    bool isVerticalScrollerPinnedToMinimumPosition() const { return !verticalScrollbar() || scrollPosition(verticalScrollbar()) <= minimumScrollPosition().y(); }
    bool isVerticalScrollerPinnedToMaximumPosition() const { return !verticalScrollbar() || scrollPosition(verticalScrollbar()) >= maximumScrollPosition().y(); } 

    bool isPinnedInBothDirections(const IntSize&) const; 
    bool isPinnedHorizontallyInDirection(int horizontalScrollDelta) const; 
    bool isPinnedVerticallyInDirection(int verticalScrollDelta) const;
#endif

    virtual TiledBacking* tiledBacking() const { return 0; }
    virtual bool usesCompositedScrolling() const { return false; }

    virtual GraphicsLayer* layerForHorizontalScrollbar() const { return 0; }
    virtual GraphicsLayer* layerForVerticalScrollbar() const { return 0; }

    bool hasLayerForHorizontalScrollbar() const;
    bool hasLayerForVerticalScrollbar() const;

    void verticalScrollbarLayerDidChange();
    void horizontalScrollbarLayerDidChange();

protected:
    ScrollableArea();
    virtual ~ScrollableArea();

    void setScrollOrigin(const IntPoint&);
    void resetScrollOriginChanged() { m_scrollOriginChanged = false; }

    virtual void invalidateScrollbarRect(Scrollbar*, const IntRect&) = 0;
    virtual void invalidateScrollCornerRect(const IntRect&) = 0;

    friend class ScrollingCoordinator;
    virtual GraphicsLayer* layerForScrolling() const { return 0; }
    virtual GraphicsLayer* layerForScrollCorner() const { return 0; }
#if ENABLE(RUBBER_BANDING)
    virtual GraphicsLayer* layerForOverhangAreas() const { return 0; }
#endif

    bool hasLayerForScrollCorner() const;

private:
    virtual IntRect visibleContentRectInternal(VisibleContentRectIncludesScrollbars, VisibleContentRectBehavior) const;
    void scrollPositionChanged(const IntPoint&);
    
    // NOTE: Only called from the ScrollAnimator.
    friend class ScrollAnimator;
    void setScrollOffsetFromAnimation(const IntPoint&);

    // This function should be overriden by subclasses to perform the actual
    // scroll of the content.
    virtual void setScrollOffset(const IntPoint&) = 0;

    mutable OwnPtr<ScrollAnimator> m_scrollAnimator;

    // There are 8 possible combinations of writing mode and direction. Scroll origin will be non-zero in the x or y axis
    // if there is any reversed direction or writing-mode. The combinations are:
    // writing-mode / direction     scrollOrigin.x() set    scrollOrigin.y() set
    // horizontal-tb / ltr          NO                      NO
    // horizontal-tb / rtl          YES                     NO
    // horizontal-bt / ltr          NO                      YES
    // horizontal-bt / rtl          YES                     YES
    // vertical-lr / ltr            NO                      NO
    // vertical-lr / rtl            NO                      YES
    // vertical-rl / ltr            YES                     NO
    // vertical-rl / rtl            YES                     YES
    IntPoint m_scrollOrigin;

    unsigned m_constrainsScrollingToContentEdge : 1;

    unsigned m_inLiveResize : 1;

    unsigned m_verticalScrollElasticity : 2; // ScrollElasticity
    unsigned m_horizontalScrollElasticity : 2; // ScrollElasticity

    unsigned m_scrollbarOverlayStyle : 2; // ScrollbarOverlayStyle

    unsigned m_scrollOriginChanged : 1;
    unsigned m_scrolledProgrammatically : 1;
};

} // namespace WebCore

#endif // ScrollableArea_h
