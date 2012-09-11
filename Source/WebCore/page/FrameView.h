/*
   Copyright (C) 1997 Martin Jones (mjones@kde.org)
             (C) 1998 Waldo Bastian (bastian@kde.org)
             (C) 1998, 1999 Torben Weis (weis@kde.org)
             (C) 1999 Lars Knoll (knoll@kde.org)
             (C) 1999 Antti Koivisto (koivisto@kde.org)
   Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef FrameView_h
#define FrameView_h

#include "AdjustViewSizeOrNot.h"
#include "Color.h"
#include "Frame.h"
#include "LayoutTypes.h"
#include "Pagination.h"
#include "PaintPhase.h"
#include "ScrollView.h"
#include <wtf/Forward.h>
#include <wtf/OwnPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class Color;
class Element;
class Event;
class FloatSize;
class Frame;
class FrameActionScheduler;
class KURL;
class Node;
class Page;
class RenderBox;
class RenderEmbeddedObject;
class RenderLayer;
class RenderObject;
class RenderScrollbarPart;

Pagination::Mode paginationModeForRenderStyle(RenderStyle*);

typedef unsigned long long DOMTimeStamp;

class FrameView : public ScrollView {
public:
    friend class RenderView;
    friend class Internals;

    static PassRefPtr<FrameView> create(Frame*);
    static PassRefPtr<FrameView> create(Frame*, const IntSize& initialSize);

    virtual ~FrameView();

    virtual HostWindow* hostWindow() const;
    
    virtual void invalidateRect(const IntRect&);
    virtual void setFrameRect(const IntRect&);

#if ENABLE(REQUEST_ANIMATION_FRAME)
    virtual bool scheduleAnimation();
#endif

    Frame* frame() const { return m_frame.get(); }
    void clearFrame();

    int mapFromLayoutToCSSUnits(LayoutUnit);
    LayoutUnit mapFromCSSToLayoutUnits(int);

    LayoutUnit marginWidth() const { return m_margins.width(); } // -1 means default
    LayoutUnit marginHeight() const { return m_margins.height(); } // -1 means default
    void setMarginWidth(LayoutUnit);
    void setMarginHeight(LayoutUnit);

    virtual void setCanHaveScrollbars(bool);
    void updateCanHaveScrollbars();

    virtual PassRefPtr<Scrollbar> createScrollbar(ScrollbarOrientation);

    virtual bool avoidScrollbarCreation() const;

    virtual void setContentsSize(const IntSize&);

    void layout(bool allowSubtree = true);
    bool didFirstLayout() const;
    void layoutTimerFired(Timer<FrameView>*);
    void scheduleRelayout();
    void scheduleRelayoutOfSubtree(RenderObject*);
    void unscheduleRelayout();
    bool layoutPending() const;
    bool isInLayout() const { return m_inLayout; }

    RenderObject* layoutRoot(bool onlyDuringLayout = false) const;
    void clearLayoutRoot() { m_layoutRoot = 0; }
    int layoutCount() const { return m_layoutCount; }

    bool needsLayout() const;
    void setNeedsLayout();

    bool needsFullRepaint() const { return m_doFullRepaint; }

#if ENABLE(REQUEST_ANIMATION_FRAME)
    void serviceScriptedAnimations(DOMTimeStamp);
#endif

#if USE(ACCELERATED_COMPOSITING)
    void updateCompositingLayersAfterStyleChange();
    void updateCompositingLayersAfterLayout();
    bool syncCompositingStateForThisFrame(Frame* rootFrameForSync);

    void clearBackingStores();
    void restoreBackingStores();

    // Called when changes to the GraphicsLayer hierarchy have to be synchronized with
    // content rendered via the normal painting path.
    void setNeedsOneShotDrawingSynchronization();

    virtual TiledBacking* tiledBacking() OVERRIDE;
#endif

    bool hasCompositedContent() const;
    bool hasCompositedContentIncludingDescendants() const;
    bool hasCompositingAncestor() const;
    void enterCompositingMode();
    bool isEnclosedInCompositingLayer() const;

    // Only used with accelerated compositing, but outside the #ifdef to make linkage easier.
    // Returns true if the sync was completed.
    bool syncCompositingStateIncludingSubframes();

    // Returns true when a paint with the PaintBehaviorFlattenCompositingLayers flag set gives
    // a faithful representation of the content.
    bool isSoftwareRenderable() const;

    void didMoveOnscreen();
    void willMoveOffscreen();

    void resetScrollbars();
    void resetScrollbarsAndClearContentsSize();
    void detachCustomScrollbars();
    virtual void recalculateScrollbarOverlayStyle();

    void clear();

    bool isTransparent() const;
    void setTransparent(bool isTransparent);

    Color baseBackgroundColor() const;
    void setBaseBackgroundColor(const Color&);
    void updateBackgroundRecursively(const Color&, bool);

    bool shouldUpdateWhileOffscreen() const;
    void setShouldUpdateWhileOffscreen(bool);
    bool shouldUpdate(bool = false) const;

    void adjustViewSize();
    
    virtual IntRect windowClipRect(bool clipToContents = true) const;
    IntRect windowClipRectForFrameOwner(const HTMLFrameOwnerElement*, bool clipToLayerContents) const;

    virtual IntRect windowResizerRect() const;

    virtual void setFixedVisibleContentRect(const IntRect&) OVERRIDE;
    void setScrollPosition(const IntPoint&);
    void scrollPositionChangedViaPlatformWidget();
    virtual void repaintFixedElementsAfterScrolling();
    virtual void updateFixedElementsAfterScrolling();
    virtual bool shouldRubberBandInDirection(ScrollDirection) const;
    virtual bool requestScrollPositionUpdate(const IntPoint&) OVERRIDE;

    String mediaType() const;
    void setMediaType(const String&);
    void adjustMediaTypeForPrinting(bool printing);

    void setCannotBlitToWindow();
    void setIsOverlapped(bool);
    bool isOverlapped() const { return m_isOverlapped; }
    bool isOverlappedIncludingAncestors() const;
    void setContentIsOpaque(bool);

    void addSlowRepaintObject();
    void removeSlowRepaintObject();
    bool hasSlowRepaintObjects() const { return m_slowRepaintObjectCount; }

    // Includes fixed- and sticky-position objects.
    typedef HashSet<RenderObject*> ViewportConstrainedObjectSet;
    void addViewportConstrainedObject(RenderObject*);
    void removeViewportConstrainedObject(RenderObject*);
    const ViewportConstrainedObjectSet* viewportConstrainedObjects() const { return m_viewportConstrainedObjects.get(); }
    bool hasViewportConstrainedObjects() const { return m_viewportConstrainedObjects && m_viewportConstrainedObjects->size() > 0; }

    // Functions for querying the current scrolled position, negating the effects of overhang
    // and adjusting for page scale.
    IntSize scrollOffsetForFixedPosition() const;

    bool fixedElementsLayoutRelativeToFrame() const;

    void beginDeferredRepaints();
    void endDeferredRepaints();
    void checkFlushDeferredRepaintsAfterLoadComplete();
    void flushDeferredRepaints();
    void startDeferredRepaintTimer(double delay);
    void resetDeferredRepaintDelay();

    void beginDisableRepaints();
    void endDisableRepaints();
    bool repaintsDisabled() { return m_disableRepaints > 0; }

#if ENABLE(DASHBOARD_SUPPORT) || ENABLE(WIDGET_REGION)
    void updateDashboardRegions();
#endif
    void updateControlTints();

    void restoreScrollbar();

    void scheduleEvent(PassRefPtr<Event>, PassRefPtr<Node>);
    void pauseScheduledEvents();
    void resumeScheduledEvents();
    void postLayoutTimerFired(Timer<FrameView>*);

    bool wasScrolledByUser() const;
    void setWasScrolledByUser(bool);

    bool safeToPropagateScrollToParent() const { return m_safeToPropagateScrollToParent; }
    void setSafeToPropagateScrollToParent(bool isSafe) { m_safeToPropagateScrollToParent = isSafe; }

    void addWidgetToUpdate(RenderEmbeddedObject*);
    void removeWidgetToUpdate(RenderEmbeddedObject*);

    virtual void paintContents(GraphicsContext*, const IntRect& damageRect);
    void setPaintBehavior(PaintBehavior);
    PaintBehavior paintBehavior() const;
    bool isPainting() const;
    bool hasEverPainted() const { return m_lastPaintTime; }
    void setLastPaintTime(double lastPaintTime) { m_lastPaintTime = lastPaintTime; }
    void setNodeToDraw(Node*);

    enum SelectionInSnaphot { IncludeSelection, ExcludeSelection };
    enum CoordinateSpaceForSnapshot { DocumentCoordinates, ViewCoordinates };
    void paintContentsForSnapshot(GraphicsContext*, const IntRect& imageRect, SelectionInSnaphot shouldPaintSelection, CoordinateSpaceForSnapshot);

    virtual void paintOverhangAreas(GraphicsContext*, const IntRect& horizontalOverhangArea, const IntRect& verticalOverhangArea, const IntRect& dirtyRect);
    virtual void paintScrollCorner(GraphicsContext*, const IntRect& cornerRect);
    virtual void paintScrollbar(GraphicsContext*, Scrollbar*, const IntRect&) OVERRIDE;

    Color documentBackgroundColor() const;

    bool isInChildFrameWithFrameFlattening() const;

    static double currentPaintTimeStamp() { return sCurrentPaintTimeStamp; } // returns 0 if not painting
    
    void updateLayoutAndStyleIfNeededRecursive();

    void incrementVisuallyNonEmptyCharacterCount(unsigned);
    void incrementVisuallyNonEmptyPixelCount(const IntSize&);
    void setIsVisuallyNonEmpty() { m_isVisuallyNonEmpty = true; }
    bool isVisuallyNonEmpty() const { return m_isVisuallyNonEmpty; }
    void enableAutoSizeMode(bool enable, const IntSize& minSize, const IntSize& maxSize);

    void forceLayout(bool allowSubtree = false);
    void forceLayoutForPagination(const FloatSize& pageSize, const FloatSize& originalPageSize, float maximumShrinkFactor, AdjustViewSizeOrNot);

    // FIXME: This method is retained because of embedded WebViews in AppKit.  When a WebView is embedded inside
    // some enclosing view with auto-pagination, no call happens to resize the view.  The new pagination model
    // needs the view to resize as a result of the breaks, but that means that the enclosing view has to potentially
    // resize around that view.  Auto-pagination uses the bounds of the actual view that's being printed to determine
    // the edges of the print operation, so the resize is necessary if the enclosing view's bounds depend on the
    // web document's bounds.
    // 
    // This is already a problem if the view needs to be a different size because of printer fonts or because of print stylesheets.
    // Mail/Dictionary work around this problem by using the _layoutForPrinting SPI
    // to at least get print stylesheets and printer fonts into play, but since WebKit doesn't know about the page offset or
    // page size, it can't actually paginate correctly during _layoutForPrinting.
    //
    // We can eventually move Mail to a newer SPI that would let them opt in to the layout-time pagination model,
    // but that doesn't solve the general problem of how other AppKit views could opt in to the better model.
    //
    // NO OTHER PLATFORM BESIDES MAC SHOULD USE THIS METHOD.
    void adjustPageHeightDeprecated(float* newBottom, float oldTop, float oldBottom, float bottomLimit);

    bool scrollToFragment(const KURL&);
    bool scrollToAnchor(const String&);
    void maintainScrollPositionAtAnchor(Node*);
    void scrollElementToRect(Element*, const IntRect&);

    // Methods to convert points and rects between the coordinate space of the renderer, and this view.
    virtual IntRect convertFromRenderer(const RenderObject*, const IntRect&) const;
    virtual IntRect convertToRenderer(const RenderObject*, const IntRect&) const;
    virtual IntPoint convertFromRenderer(const RenderObject*, const IntPoint&) const;
    virtual IntPoint convertToRenderer(const RenderObject*, const IntPoint&) const;

    bool isFrameViewScrollCorner(RenderScrollbarPart* scrollCorner) const { return m_scrollCorner == scrollCorner; }

    enum ScrollbarModesCalculationStrategy { RulesFromWebContentOnly, AnyRule };
    void calculateScrollbarModesForLayout(ScrollbarMode& hMode, ScrollbarMode& vMode, ScrollbarModesCalculationStrategy = AnyRule);

    // Normal delay
    static void setRepaintThrottlingDeferredRepaintDelay(double p);
    // Negative value would mean that first few repaints happen without a delay
    static void setRepaintThrottlingnInitialDeferredRepaintDelayDuringLoading(double p);
    // The delay grows on each repaint to this maximum value
    static void setRepaintThrottlingMaxDeferredRepaintDelayDuringLoading(double p);
    // On each repaint the delay increses by this amount
    static void setRepaintThrottlingDeferredRepaintDelayIncrementDuringLoading(double p);

    virtual IntPoint currentMousePosition() const;

    virtual bool scrollbarsCanBeActive() const OVERRIDE;

    // FIXME: Remove this method once plugin loading is decoupled from layout.
    void flushAnyPendingPostLayoutTasks();

    virtual bool shouldSuspendScrollAnimations() const;
    virtual void scrollbarStyleChanged(int newStyle, bool forceUpdate);

    void setAnimatorsAreActive();

    RenderBox* embeddedContentBox() const;
    
    void setTracksRepaints(bool);
    bool isTrackingRepaints() const { return m_isTrackingRepaints; }
    void resetTrackedRepaints() { m_trackedRepaintRects.clear(); }
    const Vector<IntRect>& trackedRepaintRects() const { return m_trackedRepaintRects; }

    typedef HashSet<ScrollableArea*> ScrollableAreaSet;
    void addScrollableArea(ScrollableArea*);
    void removeScrollableArea(ScrollableArea*);
    bool containsScrollableArea(ScrollableArea*) const;
    const ScrollableAreaSet* scrollableAreas() const { return m_scrollableAreas.get(); }

    virtual void removeChild(Widget*) OVERRIDE;

    // This function exists for ports that need to handle wheel events manually.
    // On Mac WebKit1 the underlying NSScrollView just does the scrolling, but on most other platforms
    // we need this function in order to do the scroll ourselves.
    bool wheelEvent(const PlatformWheelEvent&);

    void setScrollingPerformanceLoggingEnabled(bool);

    // Page and FrameView both store a Pagination value. Page::pagination() is set only by API,
    // and FrameView::pagination() is set only by CSS. Page::pagination() will affect all
    // FrameViews in the page cache, but FrameView::pagination() only affects the current
    // FrameView. FrameView::pagination() will return m_pagination if it has been set. Otherwise,
    // it will return Page::pagination() since currently there are no callers that need to
    // distinguish between the two.
    const Pagination& pagination() const;
    void setPagination(const Pagination&);

protected:
    virtual bool scrollContentsFastPath(const IntSize& scrollDelta, const IntRect& rectToScroll, const IntRect& clipRect);
    virtual void scrollContentsSlowPath(const IntRect& updateRect);

    virtual bool isVerticalDocument() const;
    virtual bool isFlippedDocument() const;

private:
    explicit FrameView(Frame*);

    void reset();
    void init();

    virtual bool isFrameView() const;

    friend class RenderWidget;
    bool useSlowRepaints(bool considerOverlap = true) const;
    bool useSlowRepaintsIfNotOverlapped() const;
    void updateCanBlitOnScrollRecursively();
    bool contentsInCompositedLayer() const;

    void applyOverflowToViewport(RenderObject*, ScrollbarMode& hMode, ScrollbarMode& vMode);
    void applyPaginationToViewport();

    void updateOverflowStatus(bool horizontalOverflow, bool verticalOverflow);

    void paintControlTints();

    void forceLayoutParentViewIfNeeded();
    void performPostLayoutTasks();
    void autoSizeIfEnabled();

    virtual void repaintContentRectangle(const IntRect&, bool immediate);
    virtual void contentsResized() OVERRIDE;
    virtual void visibleContentsResized();

    virtual void delegatesScrollingDidChange();

    // Override ScrollView methods to do point conversion via renderers, in order to
    // take transforms into account.
    virtual IntRect convertToContainingView(const IntRect&) const OVERRIDE;
    virtual IntRect convertFromContainingView(const IntRect&) const OVERRIDE;
    virtual IntPoint convertToContainingView(const IntPoint&) const OVERRIDE;
    virtual IntPoint convertFromContainingView(const IntPoint&) const OVERRIDE;

    // ScrollableArea interface
    virtual void invalidateScrollbarRect(Scrollbar*, const IntRect&) OVERRIDE;
    virtual bool isActive() const OVERRIDE;
    virtual void getTickmarks(Vector<IntRect>&) const OVERRIDE;
    virtual void scrollTo(const IntSize&) OVERRIDE;
    virtual void setVisibleScrollerThumbRect(const IntRect&) OVERRIDE;
    virtual ScrollableArea* enclosingScrollableArea() const OVERRIDE;
    virtual IntRect scrollableAreaBoundingBox() const OVERRIDE;
    virtual bool scrollAnimatorEnabled() const OVERRIDE;
#if USE(ACCELERATED_COMPOSITING)
    virtual GraphicsLayer* layerForHorizontalScrollbar() const OVERRIDE;
    virtual GraphicsLayer* layerForVerticalScrollbar() const OVERRIDE;
    virtual GraphicsLayer* layerForScrollCorner() const OVERRIDE;
#if ENABLE(RUBBER_BANDING)
    virtual GraphicsLayer* layerForOverhangAreas() const OVERRIDE;
#endif
#endif

    void updateScrollableAreaSet();

    virtual void notifyPageThatContentAreaWillPaint() const;

    bool shouldUseLoadTimeDeferredRepaintDelay() const;
    void deferredRepaintTimerFired(Timer<FrameView>*);
    void doDeferredRepaints();
    void updateDeferredRepaintDelayAfterRepaint();
    double adjustedDeferredRepaintDelay() const;

    bool updateWidgets();
    void updateWidget(RenderEmbeddedObject*);
    void scrollToAnchor();
    void scrollPositionChanged();

    bool hasCustomScrollbars() const;

    virtual void updateScrollCorner();

    FrameView* parentFrameView() const;

    bool doLayoutWithFrameFlattening(bool allowSubtree);

    void setViewportConstrainedObjectsNeedLayout();

    virtual AXObjectCache* axObjectCache() const;
    void notifyWidgetsInAllFrames(WidgetNotification);
    
    static double sCurrentPaintTimeStamp; // used for detecting decoded resource thrash in the cache

    LayoutSize m_size;
    LayoutSize m_margins;
    
    typedef HashSet<RenderEmbeddedObject*> RenderEmbeddedObjectSet;
    OwnPtr<RenderEmbeddedObjectSet> m_widgetUpdateSet;
    RefPtr<Frame> m_frame;

    bool m_doFullRepaint;
    
    bool m_canHaveScrollbars;
    bool m_cannotBlitToWindow;
    bool m_isOverlapped;
    bool m_contentIsOpaque;
    unsigned m_slowRepaintObjectCount;
    int m_borderX;
    int m_borderY;

    Timer<FrameView> m_layoutTimer;
    bool m_delayedLayout;
    RenderObject* m_layoutRoot;
    
    bool m_layoutSchedulingEnabled;
    bool m_inLayout;
    bool m_inSynchronousPostLayout;
    int m_layoutCount;
    unsigned m_nestedLayoutCount;
    Timer<FrameView> m_postLayoutTasksTimer;
    bool m_firstLayoutCallbackPending;

    bool m_firstLayout;
    bool m_isTransparent;
    Color m_baseBackgroundColor;
    IntSize m_lastViewportSize;
    float m_lastZoomFactor;

    String m_mediaType;
    String m_mediaTypeWhenNotPrinting;

    OwnPtr<FrameActionScheduler> m_actionScheduler;

    bool m_overflowStatusDirty;
    bool m_horizontalOverflow;
    bool m_verticalOverflow;    
    RenderObject* m_viewportRenderer;

    Pagination m_pagination;

    bool m_wasScrolledByUser;
    bool m_inProgrammaticScroll;
    bool m_safeToPropagateScrollToParent;

    unsigned m_deferringRepaints;
    unsigned m_repaintCount;
    Vector<LayoutRect> m_repaintRects;
    Timer<FrameView> m_deferredRepaintTimer;
    double m_deferredRepaintDelay;
    double m_lastPaintTime;

    unsigned m_disableRepaints;

    bool m_isTrackingRepaints; // Used for testing.
    Vector<IntRect> m_trackedRepaintRects;

    bool m_shouldUpdateWhileOffscreen;

    unsigned m_deferSetNeedsLayouts;
    bool m_setNeedsLayoutWasDeferred;

    RefPtr<Node> m_nodeToDraw;
    PaintBehavior m_paintBehavior;
    bool m_isPainting;

    unsigned m_visuallyNonEmptyCharacterCount;
    unsigned m_visuallyNonEmptyPixelCount;
    bool m_isVisuallyNonEmpty;
    bool m_firstVisuallyNonEmptyLayoutCallbackPending;

    RefPtr<Node> m_maintainScrollPositionAnchor;

    // Renderer to hold our custom scroll corner.
    RenderScrollbarPart* m_scrollCorner;

    // If true, automatically resize the frame view around its content.
    bool m_shouldAutoSize;
    bool m_inAutoSize;
    // True if autosize has been run since m_shouldAutoSize was set.
    bool m_didRunAutosize;
    // The lower bound on the size when autosizing.
    IntSize m_minAutoSize;
    // The upper bound on the size when autosizing.
    IntSize m_maxAutoSize;

    OwnPtr<ScrollableAreaSet> m_scrollableAreas;
    OwnPtr<ViewportConstrainedObjectSet> m_viewportConstrainedObjects;

    static double s_normalDeferredRepaintDelay;
    static double s_initialDeferredRepaintDelayDuringLoading;
    static double s_maxDeferredRepaintDelayDuringLoading;
    static double s_deferredRepaintDelayIncrementDuringLoading;
};

inline void FrameView::incrementVisuallyNonEmptyCharacterCount(unsigned count)
{
    if (m_isVisuallyNonEmpty)
        return;
    m_visuallyNonEmptyCharacterCount += count;
    // Use a threshold value to prevent very small amounts of visible content from triggering didFirstVisuallyNonEmptyLayout.
    // The first few hundred characters rarely contain the interesting content of the page.
    static const unsigned visualCharacterThreshold = 200;
    if (m_visuallyNonEmptyCharacterCount > visualCharacterThreshold)
        setIsVisuallyNonEmpty();
}

inline void FrameView::incrementVisuallyNonEmptyPixelCount(const IntSize& size)
{
    if (m_isVisuallyNonEmpty)
        return;
    m_visuallyNonEmptyPixelCount += size.width() * size.height();
    // Use a threshold value to prevent very small amounts of visible content from triggering didFirstVisuallyNonEmptyLayout
    static const unsigned visualPixelThreshold = 32 * 32;
    if (m_visuallyNonEmptyPixelCount > visualPixelThreshold)
        setIsVisuallyNonEmpty();
}

inline int FrameView::mapFromLayoutToCSSUnits(LayoutUnit value)
{
    return value / (m_frame->pageZoomFactor() * m_frame->frameScaleFactor());
}

inline LayoutUnit FrameView::mapFromCSSToLayoutUnits(int value)
{
    return value * m_frame->pageZoomFactor() * m_frame->frameScaleFactor();
}

} // namespace WebCore

#endif // FrameView_h
