/*
 * Copyright (C) 2006 Apple Computer, Inc.
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
 */

#ifndef RenderSVGBlock_h
#define RenderSVGBlock_h

#if ENABLE(SVG)
#include "RenderBlockFlow.h"
#include "SVGGraphicsElement.h"
#include "SVGRenderSupport.h"

namespace WebCore {

class SVGElement;

class RenderSVGBlock : public RenderBlockFlow {
public:
    virtual LayoutRect visualOverflowRect() const OVERRIDE FINAL;

    SVGGraphicsElement& graphicsElement() const { return toSVGGraphicsElement(nodeForNonAnonymous()); }

protected:
    RenderSVGBlock(SVGGraphicsElement&, PassRef<RenderStyle>);
    virtual void willBeDestroyed() OVERRIDE;

private:
    void element() const WTF_DELETED_FUNCTION;

    virtual void setStyle(PassRef<RenderStyle>) OVERRIDE FINAL;
    virtual void updateFromStyle() OVERRIDE FINAL;

    virtual bool isRenderSVGBlock() const OVERRIDE FINAL { return true; };

    virtual void absoluteRects(Vector<IntRect>&, const LayoutPoint& accumulatedOffset) const;

    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle) OVERRIDE FINAL;
};

}
#endif
#endif
