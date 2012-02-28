/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
 * Copyright (C) 2009 Google, Inc.  All rights reserved.
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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
#include "SVGLocatable.h"

#include "RenderObject.h"
#include "SVGException.h"
#include "SVGNames.h"
#include "SVGStyledLocatableElement.h"

namespace WebCore {

static bool isViewportElement(Node* node)
{
    return (node->hasTagName(SVGNames::svgTag)
        || node->hasTagName(SVGNames::symbolTag)
        || node->hasTagName(SVGNames::foreignObjectTag)
        || node->hasTagName(SVGNames::imageTag));
}

SVGElement* SVGLocatable::nearestViewportElement(const SVGElement* element)
{
    ASSERT(element);
    for (Element* current = element->parentOrHostElement(); current; current = current->parentOrHostElement()) {
        if (isViewportElement(current))
            return static_cast<SVGElement*>(current);
    }

    return 0;
}

SVGElement* SVGLocatable::farthestViewportElement(const SVGElement* element)
{
    ASSERT(element);
    SVGElement* farthest = 0;
    for (Element* current = element->parentOrHostElement(); current; current = current->parentOrHostElement()) {
        if (isViewportElement(current))
            farthest = static_cast<SVGElement*>(current);
    }
    return farthest;
}

FloatRect SVGLocatable::getBBox(SVGElement* element, StyleUpdateStrategy styleUpdateStrategy)
{
    ASSERT(element);
    if (styleUpdateStrategy == AllowStyleUpdate)
        element->document()->updateLayoutIgnorePendingStylesheets();

    // FIXME: Eventually we should support getBBox for detached elements.
    if (!element->renderer())
        return FloatRect();

    return element->renderer()->objectBoundingBox();
}

AffineTransform SVGLocatable::computeCTM(SVGElement* element, CTMScope mode, StyleUpdateStrategy styleUpdateStrategy)
{
    ASSERT(element);
    if (styleUpdateStrategy == AllowStyleUpdate)
        element->document()->updateLayoutIgnorePendingStylesheets();

    AffineTransform ctm;

    SVGElement* stopAtElement = mode == NearestViewportScope ? nearestViewportElement(element) : 0;
    for (Element* currentElement = element; currentElement; currentElement = currentElement->parentOrHostElement()) {
        if (!currentElement->isSVGElement())
            break;

        if (static_cast<SVGElement*>(currentElement)->isStyled())
            ctm = static_cast<SVGStyledElement*>(currentElement)->localCoordinateSpaceTransform(mode).multiply(ctm);

        // For getCTM() computation, stop at the nearest viewport element
        if (currentElement == stopAtElement)
            break;
    }

    return ctm;
}

AffineTransform SVGLocatable::getTransformToElement(SVGElement* target, ExceptionCode& ec, StyleUpdateStrategy styleUpdateStrategy)
{
    AffineTransform ctm = getCTM(styleUpdateStrategy);

    if (target && target->isStyledLocatable()) {
        AffineTransform targetCTM = static_cast<SVGStyledLocatableElement*>(target)->getCTM(styleUpdateStrategy);
        if (!targetCTM.isInvertible()) {
            ec = SVGException::SVG_MATRIX_NOT_INVERTABLE;
            return ctm;
        }
        ctm = targetCTM.inverse() * ctm;
    }

    return ctm;
}

}

#endif // ENABLE(SVG)
