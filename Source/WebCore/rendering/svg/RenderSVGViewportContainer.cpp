/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Google, Inc.
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

#include "config.h"

#if ENABLE(SVG)
#include "RenderSVGViewportContainer.h"

#include "GraphicsContext.h"
#include "RenderView.h"
#include "SVGNames.h"
#include "SVGSVGElement.h"

namespace WebCore {

RenderSVGViewportContainer::RenderSVGViewportContainer(SVGStyledElement* node)
    : RenderSVGContainer(node)
    , m_isLayoutSizeChanged(false)
    , m_needsTransformUpdate(true)
{
}

void RenderSVGViewportContainer::determineIfLayoutSizeChanged()
{
    if (!node()->hasTagName(SVGNames::svgTag))
        return;

    m_isLayoutSizeChanged = static_cast<SVGSVGElement*>(node())->hasRelativeLengths() && selfNeedsLayout();
}

void RenderSVGViewportContainer::applyViewportClip(PaintInfo& paintInfo)
{
    if (SVGRenderSupport::isOverflowHidden(this))
        paintInfo.context->clip(m_viewport);
}

void RenderSVGViewportContainer::calcViewport()
{
    SVGElement* element = static_cast<SVGElement*>(node());
    if (!element->hasTagName(SVGNames::svgTag))
        return;
    SVGSVGElement* svg = static_cast<SVGSVGElement*>(element);
    FloatRect oldViewport = m_viewport;

    SVGLengthContext lengthContext(element);
    m_viewport = FloatRect(svg->x().value(lengthContext), svg->y().value(lengthContext), svg->width().value(lengthContext), svg->height().value(lengthContext));

    if (oldViewport != m_viewport) {
        setNeedsBoundariesUpdate();
        setNeedsTransformUpdate();
    }
}

bool RenderSVGViewportContainer::calculateLocalTransform() 
{
    if (!m_needsTransformUpdate)
        return false;
    
    m_localToParentTransform = AffineTransform::translation(m_viewport.x(), m_viewport.y()) * viewportTransform();
    m_needsTransformUpdate = false;
    return true;
}

AffineTransform RenderSVGViewportContainer::viewportTransform() const
{
    if (node()->hasTagName(SVGNames::svgTag)) {
        SVGSVGElement* svg = static_cast<SVGSVGElement*>(node());
        return svg->viewBoxToViewTransform(m_viewport.width(), m_viewport.height());
    }

    return AffineTransform();
}

bool RenderSVGViewportContainer::pointIsInsideViewportClip(const FloatPoint& pointInParent)
{
    // Respect the viewport clip (which is in parent coords)
    if (!SVGRenderSupport::isOverflowHidden(this))
        return true;
    
    return m_viewport.contains(pointInParent);
}

}

#endif // ENABLE(SVG)
