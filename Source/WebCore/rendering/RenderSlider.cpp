/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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

#include "config.h"
#include "RenderSlider.h"

#include "CSSPropertyNames.h"
#include "Document.h"
#include "Event.h"
#include "EventHandler.h"
#include "EventNames.h"
#include "Frame.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "MediaControlElements.h"
#include "MouseEvent.h"
#include "Node.h"
#include "RenderLayer.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "ShadowRoot.h"
#include "SliderThumbElement.h"
#include "StepRange.h"
#include "StyleResolver.h"
#include <wtf/MathExtras.h>
#include <wtf/StackStats.h>

namespace WebCore {

const int RenderSlider::defaultTrackLength = 129;

RenderSlider::RenderSlider(HTMLInputElement& element, PassRef<RenderStyle> style)
    : RenderFlexibleBox(element, std::move(style))
{
    // We assume RenderSlider works only with <input type=range>.
    ASSERT(element.isRangeControl());
}

RenderSlider::~RenderSlider()
{
}

HTMLInputElement& RenderSlider::element() const
{
    return toHTMLInputElement(nodeForNonAnonymous());
}

bool RenderSlider::canBeReplacedWithInlineRunIn() const
{
    return false;
}

int RenderSlider::baselinePosition(FontBaseline, bool /*firstLine*/, LineDirectionMode, LinePositionMode) const
{
    // FIXME: Patch this function for writing-mode.
    return height() + marginTop();
}

void RenderSlider::computeIntrinsicLogicalWidths(LayoutUnit& minLogicalWidth, LayoutUnit& maxLogicalWidth) const
{
    maxLogicalWidth = defaultTrackLength * style().effectiveZoom();
    if (!style().width().isPercent())
        minLogicalWidth = maxLogicalWidth;
}

void RenderSlider::computePreferredLogicalWidths()
{
    m_minPreferredLogicalWidth = 0;
    m_maxPreferredLogicalWidth = 0;

    if (style().width().isFixed() && style().width().value() > 0)
        m_minPreferredLogicalWidth = m_maxPreferredLogicalWidth = adjustContentBoxLogicalWidthForBoxSizing(style().width().value());
    else
        computeIntrinsicLogicalWidths(m_minPreferredLogicalWidth, m_maxPreferredLogicalWidth);

    if (style().minWidth().isFixed() && style().minWidth().value() > 0) {
        m_maxPreferredLogicalWidth = std::max(m_maxPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(style().minWidth().value()));
        m_minPreferredLogicalWidth = std::max(m_minPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(style().minWidth().value()));
    }

    if (style().maxWidth().isFixed()) {
        m_maxPreferredLogicalWidth = std::min(m_maxPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(style().maxWidth().value()));
        m_minPreferredLogicalWidth = std::min(m_minPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(style().maxWidth().value()));
    }

    LayoutUnit toAdd = borderAndPaddingWidth();
    m_minPreferredLogicalWidth += toAdd;
    m_maxPreferredLogicalWidth += toAdd;

    setPreferredLogicalWidthsDirty(false); 
}

void RenderSlider::layout()
{
    StackStats::LayoutCheckPoint layoutCheckPoint;

    // FIXME: Find a way to cascade appearance. http://webkit.org/b/62535
    RenderBox* thumbBox = element().sliderThumbElement()->renderBox();
    if (thumbBox && thumbBox->isSliderThumb())
        static_cast<RenderSliderThumb*>(thumbBox)->updateAppearance(&style());

    RenderFlexibleBox::layout();
}

bool RenderSlider::inDragMode() const
{
    return element().sliderThumbElement()->active();
}

} // namespace WebCore
