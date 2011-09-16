/*
 * Copyright 2011 Adobe Systems Incorporated. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER “AS IS” AND ANY
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

#ifndef RenderFlowThread_h
#define RenderFlowThread_h


#include "RenderBlock.h"
#include <wtf/HashCountedSet.h>
#include <wtf/ListHashSet.h>
#include <wtf/PassRefPtr.h>
#include <wtf/UnusedParam.h>
#include <wtf/text/AtomicString.h>

namespace WebCore {

class RenderFlowThread;
class RenderStyle;
class RenderRegion;

typedef ListHashSet<RenderFlowThread*> RenderFlowThreadList;
typedef HashCountedSet<RenderFlowThread*> RenderFlowThreadCountedSet;
typedef ListHashSet<RenderRegion*> RenderRegionList;

// RenderFlowThread is used to collect all the render objects that participate in a
// flow thread. It will also help in doing the layout. However, it will not render
// directly to screen. Instead, RenderRegion objects will redirect their paint 
// and nodeAtPoint methods to this object. Each RenderRegion will actually be a viewPort
// of the RenderFlowThread.

class RenderFlowThread: public RenderBlock {
public:
    RenderFlowThread(Node*, const AtomicString& flowThread);

    virtual bool isRenderFlowThread() const { return true; }

    virtual void layout();

    AtomicString flowThread() const { return m_flowThread; }

    // Always create a RenderLayer for the RenderFlowThread, so that we 
    // can easily avoid to draw it's children directly.
    virtual bool requiresLayer() const { return true; }

    RenderObject* nextRendererForNode(Node*) const;
    RenderObject* previousRendererForNode(Node*) const;
    
    void addFlowChild(RenderObject* newChild, RenderObject* beforeChild = 0);
    void removeFlowChild(RenderObject*);

    void addRegionToThread(RenderRegion*);
    void removeRegionFromThread(RenderRegion*);
    const RenderRegionList& renderRegionList() const { return m_regionList; }

    void computeLogicalWidth();
    void computeLogicalHeight();

    void paintIntoRegion(PaintInfo&, const LayoutRect& regionRect, const LayoutPoint& paintOffset);
    bool hitTestRegion(const LayoutRect& regionRect, const HitTestRequest&, HitTestResult&, const LayoutPoint& pointInContainer, const LayoutPoint& accumulatedOffset);

    bool hasRegions() const { return m_regionList.size(); }
    bool hasValidRegions() const { ASSERT(!m_regionsInvalidated); return m_hasValidRegions; }

    void invalidateRegions() { m_regionsInvalidated = true; setNeedsLayout(true); }

    static PassRefPtr<RenderStyle> createFlowThreadStyle(RenderStyle* parentStyle);

    void styleDidChange(StyleDifference, const RenderStyle* oldStyle);

    void pushDependencies(RenderFlowThreadList&);

    void repaintRectangleInRegions(const LayoutRect&, bool immediate);

    LayoutUnit regionLogicalWidthForLine(LayoutUnit position) const;
    LayoutUnit regionLogicalHeightForLine(LayoutUnit position) const;
    LayoutUnit regionRemainingLogicalHeightForLine(LayoutUnit position, bool includeBoundaryPoint = true) const;

    bool isRegionFittingEnabled() const { return !m_regionFittingDisableCount; }
    void disableRegionFitting() { m_regionFittingDisableCount++; }
    void enableRegionFitting() { ASSERT(m_regionFittingDisableCount > 0); m_regionFittingDisableCount--; }

    RenderRegion* mapFromFlowToRegion(TransformState&) const;

private:
    virtual const char* renderName() const { return "RenderFlowThread"; }

    RenderRegion* renderRegionForLine(LayoutUnit position, bool extendLastRegion = false) const;

    bool dependsOn(RenderFlowThread* otherRenderFlowThread) const;
    void addDependencyOnFlowThread(RenderFlowThread*);
    void removeDependencyOnFlowThread(RenderFlowThread*);
    void checkInvalidRegions();

    bool shouldRepaint(const LayoutRect&) const;

    typedef ListHashSet<RenderObject*> FlowThreadChildList;
    FlowThreadChildList m_flowThreadChildList;

    AtomicString m_flowThread;
    RenderRegionList m_regionList;

    // Observer flow threads have invalid regions that depend on the state of this thread
    // to re-validate their regions. Keeping a set of observer threads make it easy
    // to notify them when a region was removed from this flow.
    RenderFlowThreadCountedSet m_observerThreadsSet;

    // Some threads need to have a complete layout before we layout this flow.
    // That's because they contain a RenderRegion that should display this thread. The set makes it
    // easy to sort the order of threads layout.
    RenderFlowThreadCountedSet m_layoutBeforeThreadsSet;

    bool m_hasValidRegions;
    bool m_regionsInvalidated;
    unsigned m_regionFittingDisableCount;
};

inline RenderFlowThread* toRenderFlowThread(RenderObject* object)
{
    ASSERT(!object || object->isRenderFlowThread());
    return static_cast<RenderFlowThread*>(object);
}

inline const RenderFlowThread* toRenderFlowThread(const RenderObject* object)
{
    ASSERT(!object || object->isRenderFlowThread());
    return static_cast<const RenderFlowThread*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderFlowThread(const RenderFlowThread*);

class RegionFittingDisabler {
    WTF_MAKE_NONCOPYABLE(RegionFittingDisabler);
public:
    RegionFittingDisabler(RenderFlowThread* flowThread, bool disable)
    {
        if (flowThread && disable) {
            m_flowThread = flowThread;
            m_flowThread->disableRegionFitting();
        } else
            m_flowThread = 0;
    }

    ~RegionFittingDisabler()
    {
        if (m_flowThread)
            m_flowThread->enableRegionFitting();
    }
private:
    RenderFlowThread* m_flowThread;
};


} // namespace WebCore

#endif // RenderFlowThread_h
