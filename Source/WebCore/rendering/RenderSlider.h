/*
 * Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef RenderSlider_h
#define RenderSlider_h

#include "RenderFlexibleBox.h"

namespace WebCore {

class HTMLInputElement;
class MouseEvent;
class SliderThumbElement;

class RenderSlider FINAL : public RenderFlexibleBox {
public:
    static const int defaultTrackLength;

    RenderSlider(HTMLInputElement&, PassRef<RenderStyle>);
    virtual ~RenderSlider();

    HTMLInputElement& element() const;

    bool inDragMode() const;

private:
    virtual const char* renderName() const OVERRIDE { return "RenderSlider"; }
    virtual bool isSlider() const OVERRIDE { return true; }
    virtual bool canBeReplacedWithInlineRunIn() const OVERRIDE;

    virtual int baselinePosition(FontBaseline, bool firstLine, LineDirectionMode, LinePositionMode = PositionOnContainingLine) const OVERRIDE;
    virtual void computeIntrinsicLogicalWidths(LayoutUnit& minLogicalWidth, LayoutUnit& maxLogicalWidth) const OVERRIDE;
    virtual void computePreferredLogicalWidths() OVERRIDE;
    virtual bool requiresForcedStyleRecalcPropagation() const OVERRIDE { return true; }
    virtual void layout() OVERRIDE;
};

RENDER_OBJECT_TYPE_CASTS(RenderSlider, isSlider())

} // namespace WebCore

#endif // RenderSlider_h
