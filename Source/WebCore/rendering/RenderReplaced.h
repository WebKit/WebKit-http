/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2009 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef RenderReplaced_h
#define RenderReplaced_h

#include "RenderBox.h"

namespace WebCore {

class RenderReplaced : public RenderBox {
public:
    virtual ~RenderReplaced();

    virtual LayoutUnit computeReplacedLogicalWidth(ShouldComputePreferred  = ComputeActual) const OVERRIDE;
    virtual LayoutUnit computeReplacedLogicalHeight() const OVERRIDE;

    LayoutRect replacedContentRect(const LayoutSize& intrinsicSize) const;

    bool hasReplacedLogicalWidth() const;
    bool hasReplacedLogicalHeight() const;

protected:
    RenderReplaced(Element&, PassRef<RenderStyle>);
    RenderReplaced(Element&, PassRef<RenderStyle>, const LayoutSize& intrinsicSize);
    RenderReplaced(Document&, PassRef<RenderStyle>, const LayoutSize& intrinsicSize);

    virtual void willBeDestroyed() OVERRIDE;

    virtual void layout() OVERRIDE;

    virtual LayoutSize intrinsicSize() const OVERRIDE FINAL { return m_intrinsicSize; }
    virtual void computeIntrinsicRatioInformation(FloatSize& intrinsicSize, double& intrinsicRatio, bool& isPercentageIntrinsicSize) const OVERRIDE;

    virtual void computeIntrinsicLogicalWidths(LayoutUnit& minLogicalWidth, LayoutUnit& maxLogicalWidth) const OVERRIDE FINAL;

    virtual LayoutUnit minimumReplacedHeight() const { return LayoutUnit(); }

    virtual void setSelectionState(SelectionState) OVERRIDE;

    bool isSelected() const;

    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle) OVERRIDE;

    void setIntrinsicSize(const LayoutSize& intrinsicSize) { m_intrinsicSize = intrinsicSize; }
    virtual void intrinsicSizeChanged();
    virtual bool hasRelativeIntrinsicLogicalWidth() const { return false; }

    virtual void paint(PaintInfo&, const LayoutPoint&) OVERRIDE;
    bool shouldPaint(PaintInfo&, const LayoutPoint&);
    LayoutRect localSelectionRect(bool checkWhetherSelected = true) const; // This is in local coordinates, but it's a physical rect (so the top left corner is physical top left).

private:
    virtual RenderBox* embeddedContentBox() const { return 0; }
    virtual const char* renderName() const OVERRIDE { return "RenderReplaced"; }

    virtual bool canHaveChildren() const OVERRIDE { return false; }

    virtual void computePreferredLogicalWidths() OVERRIDE FINAL;
    virtual void paintReplaced(PaintInfo&, const LayoutPoint&) { }

    virtual LayoutRect clippedOverflowRectForRepaint(const RenderLayerModelObject* repaintContainer) const OVERRIDE;

    virtual VisiblePosition positionForPoint(const LayoutPoint&) OVERRIDE FINAL;
    
    virtual bool canBeSelectionLeaf() const OVERRIDE { return true; }

    virtual LayoutRect selectionRectForRepaint(const RenderLayerModelObject* repaintContainer, bool clipToVisibleContent = true) OVERRIDE FINAL;
    void computeAspectRatioInformationForRenderBox(RenderBox*, FloatSize& constrainedSize, double& intrinsicRatio, bool& isPercentageIntrinsicSize) const;

    mutable LayoutSize m_intrinsicSize;
};

RENDER_OBJECT_TYPE_CASTS(RenderReplaced, isRenderReplaced())

}

#endif
