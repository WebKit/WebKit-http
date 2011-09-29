/*
 * Copyright (C) 2004, 2005, 2006, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008 Dirk Schulze <krit@webkit.org>
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
#include "SVGRadialGradientElement.h"

#include "Attribute.h"
#include "FloatConversion.h"
#include "FloatPoint.h"
#include "RadialGradientAttributes.h"
#include "RenderSVGResourceRadialGradient.h"
#include "SVGElementInstance.h"
#include "SVGNames.h"
#include "SVGStopElement.h"
#include "SVGTransform.h"
#include "SVGTransformList.h"
#include "SVGUnitTypes.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_LENGTH(SVGRadialGradientElement, SVGNames::cxAttr, Cx, cx)
DEFINE_ANIMATED_LENGTH(SVGRadialGradientElement, SVGNames::cyAttr, Cy, cy)
DEFINE_ANIMATED_LENGTH(SVGRadialGradientElement, SVGNames::rAttr, R, r)
DEFINE_ANIMATED_LENGTH(SVGRadialGradientElement, SVGNames::fxAttr, Fx, fx)
DEFINE_ANIMATED_LENGTH(SVGRadialGradientElement, SVGNames::fyAttr, Fy, fy)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGRadialGradientElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(cx)
    REGISTER_LOCAL_ANIMATED_PROPERTY(cy)
    REGISTER_LOCAL_ANIMATED_PROPERTY(r)
    REGISTER_LOCAL_ANIMATED_PROPERTY(fx)
    REGISTER_LOCAL_ANIMATED_PROPERTY(fy)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGGradientElement)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGRadialGradientElement::SVGRadialGradientElement(const QualifiedName& tagName, Document* document)
    : SVGGradientElement(tagName, document)
    , m_cx(LengthModeWidth, "50%")
    , m_cy(LengthModeHeight, "50%")
    , m_r(LengthModeOther, "50%")
    , m_fx(LengthModeWidth)
    , m_fy(LengthModeHeight)
{
    // Spec: If the cx/cy/r attribute is not specified, the effect is as if a value of "50%" were specified.
    ASSERT(hasTagName(SVGNames::radialGradientTag));
    registerAnimatedPropertiesForSVGRadialGradientElement();
}

PassRefPtr<SVGRadialGradientElement> SVGRadialGradientElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGRadialGradientElement(tagName, document));
}

bool SVGRadialGradientElement::isSupportedAttribute(const QualifiedName& attrName)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, supportedAttributes, ());
    if (supportedAttributes.isEmpty()) {
        supportedAttributes.add(SVGNames::cxAttr);
        supportedAttributes.add(SVGNames::cyAttr);
        supportedAttributes.add(SVGNames::fxAttr);
        supportedAttributes.add(SVGNames::fyAttr);
        supportedAttributes.add(SVGNames::rAttr);
    }
    return supportedAttributes.contains<QualifiedName, SVGAttributeHashTranslator>(attrName);
}

void SVGRadialGradientElement::parseMappedAttribute(Attribute* attr)
{
    SVGParsingError parseError = NoError;

    if (!isSupportedAttribute(attr->name()))
        SVGGradientElement::parseMappedAttribute(attr);
    else if (attr->name() == SVGNames::cxAttr)
        setCxBaseValue(SVGLength::construct(LengthModeWidth, attr->value(), parseError));
    else if (attr->name() == SVGNames::cyAttr)
        setCyBaseValue(SVGLength::construct(LengthModeHeight, attr->value(), parseError));
    else if (attr->name() == SVGNames::rAttr)
        setRBaseValue(SVGLength::construct(LengthModeOther, attr->value(), parseError, ForbidNegativeLengths));
    else if (attr->name() == SVGNames::fxAttr)
        setFxBaseValue(SVGLength::construct(LengthModeWidth, attr->value(), parseError));
    else if (attr->name() == SVGNames::fyAttr)
        setFyBaseValue(SVGLength::construct(LengthModeHeight, attr->value(), parseError));
    else
        ASSERT_NOT_REACHED();

    reportAttributeParsingError(parseError, attr);
}

void SVGRadialGradientElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGGradientElement::svgAttributeChanged(attrName);
        return;
    }

    SVGElementInstance::InvalidationGuard invalidationGuard(this);
    
    updateRelativeLengthsInformation();
        
    if (RenderObject* object = renderer())
        object->setNeedsLayout(true);
}

RenderObject* SVGRadialGradientElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderSVGResourceRadialGradient(this);
}

bool SVGRadialGradientElement::collectGradientAttributes(RadialGradientAttributes& attributes)
{
    HashSet<SVGGradientElement*> processedGradients;

    bool isRadial = true;
    SVGGradientElement* current = this;

    while (current) {
        if (!current->renderer())
            return false;

        if (!attributes.hasSpreadMethod() && current->hasAttribute(SVGNames::spreadMethodAttr))
            attributes.setSpreadMethod(current->spreadMethod());

        if (!attributes.hasBoundingBoxMode() && current->hasAttribute(SVGNames::gradientUnitsAttr))
            attributes.setBoundingBoxMode(current->gradientUnits() == SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX);

        if (!attributes.hasGradientTransform() && current->hasAttribute(SVGNames::gradientTransformAttr)) {
            AffineTransform transform;
            current->gradientTransform().concatenate(transform);
            attributes.setGradientTransform(transform);
        }

        if (!attributes.hasStops()) {
            const Vector<Gradient::ColorStop>& stops(current->buildStops());
            if (!stops.isEmpty())
                attributes.setStops(stops);
        }

        if (isRadial) {
            SVGRadialGradientElement* radial = static_cast<SVGRadialGradientElement*>(current);

            if (!attributes.hasCx() && current->hasAttribute(SVGNames::cxAttr))
                attributes.setCx(radial->cx());

            if (!attributes.hasCy() && current->hasAttribute(SVGNames::cyAttr))
                attributes.setCy(radial->cy());

            if (!attributes.hasR() && current->hasAttribute(SVGNames::rAttr))
                attributes.setR(radial->r());

            if (!attributes.hasFx() && current->hasAttribute(SVGNames::fxAttr))
                attributes.setFx(radial->fx());

            if (!attributes.hasFy() && current->hasAttribute(SVGNames::fyAttr))
                attributes.setFy(radial->fy());
        }

        processedGradients.add(current);

        // Respect xlink:href, take attributes from referenced element
        Node* refNode = SVGURIReference::targetElementFromIRIString(current->href(), document());
        if (refNode && (refNode->hasTagName(SVGNames::radialGradientTag) || refNode->hasTagName(SVGNames::linearGradientTag))) {
            current = static_cast<SVGGradientElement*>(refNode);

            // Cycle detection
            if (processedGradients.contains(current)) {
                current = 0;
                break;
            }

            isRadial = current->hasTagName(SVGNames::radialGradientTag);
        } else
            current = 0;
    }

    // Handle default values for fx/fy
    if (!attributes.hasFx())
        attributes.setFx(attributes.cx());

    if (!attributes.hasFy())
        attributes.setFy(attributes.cy());

    return true;
}

void SVGRadialGradientElement::calculateFocalCenterPointsAndRadius(const RadialGradientAttributes& attributes, FloatPoint& focalPoint, FloatPoint& centerPoint, float& radius)
{
    // Determine gradient focal/center points and radius
    if (attributes.boundingBoxMode()) {
        focalPoint = FloatPoint(attributes.fx().valueAsPercentage(), attributes.fy().valueAsPercentage());
        centerPoint = FloatPoint(attributes.cx().valueAsPercentage(), attributes.cy().valueAsPercentage());
        radius = attributes.r().valueAsPercentage();
    } else {
        focalPoint = FloatPoint(attributes.fx().value(this), attributes.fy().value(this));
        centerPoint = FloatPoint(attributes.cx().value(this), attributes.cy().value(this));
        radius = attributes.r().value(this);
    }

    // Eventually adjust focal points, as described below
    float deltaX = focalPoint.x() - centerPoint.x();
    float deltaY = focalPoint.y() - centerPoint.y();
    float radiusMax = 0.99f * radius;

    // Spec: If (fx, fy) lies outside the circle defined by (cx, cy) and r, set
    // (fx, fy) to the point of intersection of the line through (fx, fy) and the circle.
    // We scale the radius by 0.99 to match the behavior of FireFox.
    if (sqrt(deltaX * deltaX + deltaY * deltaY) > radiusMax) {
        float angle = atan2f(deltaY, deltaX);

        deltaX = cosf(angle) * radiusMax;
        deltaY = sinf(angle) * radiusMax;
        focalPoint = FloatPoint(deltaX + centerPoint.x(), deltaY + centerPoint.y());
    }
}

bool SVGRadialGradientElement::selfHasRelativeLengths() const
{
    return cx().isRelative()
        || cy().isRelative()
        || r().isRelative()
        || fx().isRelative()
        || fy().isRelative();
}

}

#endif // ENABLE(SVG)
