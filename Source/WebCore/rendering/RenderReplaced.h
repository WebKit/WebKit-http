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
    RenderReplaced(Node*);
    RenderReplaced(Node*, const IntSize& intrinsicSize);
    virtual ~RenderReplaced();

    virtual LayoutUnit computeReplacedLogicalWidth(bool includeMaxWidth = true) const;
    virtual LayoutUnit computeReplacedLogicalHeight() const;

    bool hasReplacedLogicalWidth() const;
    bool hasReplacedLogicalHeight() const;

protected:
    virtual void willBeDestroyed();

    virtual void layout();

    virtual IntSize intrinsicSize() const { return m_intrinsicSize; }
    virtual void computeIntrinsicRatioInformation(FloatSize& intrinsicSize, double& intrinsicRatio, bool& isPercentageIntrinsicSize) const;

    virtual int minimumReplacedHeight() const { return 0; }

    virtual void setSelectionState(SelectionState);

    bool isSelected() const;

    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);

    void setIntrinsicSize(const IntSize& intrinsicSize) { m_intrinsicSize = intrinsicSize; }
    virtual void intrinsicSizeChanged();

    virtual void paint(PaintInfo&, const LayoutPoint&);
    bool shouldPaint(PaintInfo&, const LayoutPoint&);
    LayoutRect localSelectionRect(bool checkWhetherSelected = true) const; // This is in local coordinates, but it's a physical rect (so the top left corner is physical top left).

private:
    virtual RenderBox* embeddedContentBox() const { return 0; }
    virtual const char* renderName() const { return "RenderReplaced"; }

    virtual bool canHaveChildren() const { return false; }

    LayoutUnit computeMaxPreferredLogicalWidth() const;
    virtual void computePreferredLogicalWidths();
    virtual void paintReplaced(PaintInfo&, const LayoutPoint&) { }

    virtual LayoutRect clippedOverflowRectForRepaint(RenderBoxModelObject* repaintContainer) const;

    virtual VisiblePosition positionForPoint(const LayoutPoint&);
    
    virtual bool canBeSelectionLeaf() const { return true; }

    virtual LayoutRect selectionRectForRepaint(RenderBoxModelObject* repaintContainer, bool clipToVisibleContent = true);
    void computeAspectRatioInformationForRenderBox(RenderBox*, FloatSize& constrainedSize, double& intrinsicRatio, bool& isPercentageIntrinsicSize) const;

    mutable IntSize m_intrinsicSize;
};

}

#endif
