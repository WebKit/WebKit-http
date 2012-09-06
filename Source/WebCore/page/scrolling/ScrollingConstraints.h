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

#ifndef ScrollingConstraints_h
#define ScrollingConstraints_h

#include "FloatRect.h"

namespace WebCore {

// ViewportConstraints classes encapsulate data and logic required to reposition elements whose layout
// depends on the viewport rect (positions fixed and sticky), when scrolling and zooming.
class ViewportConstraints {
public:
    enum ConstraintType {
        FixedPositionConstaint,
        StickyPositionConstraint
    };

    enum AnchorEdgeFlags {
        AnchorEdgeLeft = 1 << 0,
        AnchorEdgeRight = 1 << 1,
        AnchorEdgeTop = 1 << 2,
        AnchorEdgeBottom = 1 << 3
    };
    typedef unsigned AnchorEdges;
    
    ViewportConstraints()
        : m_anchorEdges(0)
    { }

    virtual ~ViewportConstraints() { }
    
    virtual ConstraintType constraintType() const = 0;
    
    AnchorEdges anchorEdges() const { return m_anchorEdges; }
    bool hasAnchorEdge(AnchorEdgeFlags flag) const { return m_anchorEdges & flag; }
    void addAnchorEdge(AnchorEdgeFlags edgeFlag) { m_anchorEdges |= edgeFlag; }
    
    FloatSize alignmentOffset() const { return m_alignmentOffset; }
    void setAlignmentOffset(const FloatSize& offset) { m_alignmentOffset = offset; }

protected:
    FloatSize m_alignmentOffset;
    AnchorEdges m_anchorEdges;
};

class StickyPositionViewportConstraints : public ViewportConstraints {
public:
    StickyPositionViewportConstraints()
        : m_leftOffset(0)
        , m_rightOffset(0)
        , m_topOffset(0)
        , m_bottomOffset(0)
    { }

    virtual ConstraintType constraintType() const OVERRIDE { return StickyPositionConstraint; };
    FloatSize computeStickyOffset(const FloatRect& viewportRect) const;

    const FloatSize stickyOffsetAtLastLayout() const { return m_stickyOffsetAtLastLayout; }
    void setStickyOffsetAtLastLayout(const FloatSize& offset) { m_stickyOffsetAtLastLayout = offset; }

    FloatPoint layerPositionForViewportRect(const FloatRect& viewportRect) const;

    const FloatPoint& layerPositionAtLastLayout() const { return m_layerPositionAtLastLayout; }
    void setLayerPositionAtLastLayout(const FloatPoint& point) { m_layerPositionAtLastLayout = point; }

    float leftOffset() const { return m_leftOffset; }
    float rightOffset() const { return m_rightOffset; }
    float topOffset() const { return m_topOffset; }
    float bottomOffset() const { return m_bottomOffset; }

    void setLeftOffset(float offset) { m_leftOffset = offset; }
    void setRightOffset(float offset) { m_rightOffset = offset; }
    void setTopOffset(float offset) { m_topOffset = offset; }
    void setBottomOffset(float offset) { m_bottomOffset = offset; }

    void setAbsoluteContainingBlockRect(const FloatRect& rect) { m_absoluteContainingBlockRect = rect; }
    void setAbsoluteStickyBoxRect(const FloatRect& rect) { m_absoluteStickyBoxRect = rect; }

private:
    float m_leftOffset;
    float m_rightOffset;
    float m_topOffset;
    float m_bottomOffset;
    FloatRect m_absoluteContainingBlockRect;
    FloatRect m_absoluteStickyBoxRect;
    FloatSize m_stickyOffsetAtLastLayout;
    FloatPoint m_layerPositionAtLastLayout;
};

} // namespace WebCore

#endif // ScrollingConstraints_h
