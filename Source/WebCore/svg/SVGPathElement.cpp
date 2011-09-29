/*
 * Copyright (C) 2004, 2005, 2006, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
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
#include "SVGPathElement.h"

#include "Attribute.h"
#include "RenderSVGPath.h"
#include "RenderSVGResource.h"
#include "SVGElementInstance.h"
#include "SVGNames.h"
#include "SVGPathParserFactory.h"
#include "SVGPathSegArc.h"
#include "SVGPathSegClosePath.h"
#include "SVGPathSegCurvetoCubic.h"
#include "SVGPathSegCurvetoCubicSmooth.h"
#include "SVGPathSegCurvetoQuadratic.h"
#include "SVGPathSegCurvetoQuadraticSmooth.h"
#include "SVGPathSegLineto.h"
#include "SVGPathSegLinetoHorizontal.h"
#include "SVGPathSegLinetoVertical.h"
#include "SVGPathSegList.h"
#include "SVGPathSegListBuilder.h"
#include "SVGPathSegListPropertyTearOff.h"
#include "SVGPathSegMoveto.h"
#include "SVGSVGElement.h"

namespace WebCore {

// Define custom animated property 'd'.
const SVGPropertyInfo* SVGPathElement::dPropertyInfo()
{
    static const SVGPropertyInfo* s_propertyInfo = 0;
    if (!s_propertyInfo) {
        s_propertyInfo = new SVGPropertyInfo(AnimatedPath,
                                             SVGNames::dAttr,
                                             SVGNames::dAttr.localName(),
                                             &SVGPathElement::synchronizeD,
                                             &SVGPathElement::lookupOrCreateDWrapper);
    }
    return s_propertyInfo;
}

// Animated property definitions
DEFINE_ANIMATED_NUMBER(SVGPathElement, SVGNames::pathLengthAttr, PathLength, pathLength)
DEFINE_ANIMATED_BOOLEAN(SVGPathElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGPathElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(d)
    REGISTER_LOCAL_ANIMATED_PROPERTY(pathLength)
    REGISTER_LOCAL_ANIMATED_PROPERTY(externalResourcesRequired)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGStyledTransformableElement)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGTests)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGPathElement::SVGPathElement(const QualifiedName& tagName, Document* document)
    : SVGStyledTransformableElement(tagName, document)
    , m_pathByteStream(SVGPathByteStream::create())
    , m_pathSegList(PathSegUnalteredRole)
{
    ASSERT(hasTagName(SVGNames::pathTag));
    registerAnimatedPropertiesForSVGPathElement();
}

PassRefPtr<SVGPathElement> SVGPathElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGPathElement(tagName, document));
}

float SVGPathElement::getTotalLength()
{
    float totalLength = 0;
    SVGPathParserFactory::self()->getTotalLengthOfSVGPathByteStream(m_pathByteStream.get(), totalLength);
    return totalLength;
}

FloatPoint SVGPathElement::getPointAtLength(float length)
{
    FloatPoint point;
    SVGPathParserFactory::self()->getPointAtLengthOfSVGPathByteStream(m_pathByteStream.get(), length, point);
    return point;
}

unsigned long SVGPathElement::getPathSegAtLength(float length)
{
    unsigned long pathSeg = 0;
    SVGPathParserFactory::self()->getSVGPathSegAtLengthFromSVGPathByteStream(m_pathByteStream.get(), length, pathSeg);
    return pathSeg;
}

PassRefPtr<SVGPathSegClosePath> SVGPathElement::createSVGPathSegClosePath(SVGPathSegRole role)
{
    return SVGPathSegClosePath::create(this, role);
}

PassRefPtr<SVGPathSegMovetoAbs> SVGPathElement::createSVGPathSegMovetoAbs(float x, float y, SVGPathSegRole role)
{
    return SVGPathSegMovetoAbs::create(this, role, x, y);
}

PassRefPtr<SVGPathSegMovetoRel> SVGPathElement::createSVGPathSegMovetoRel(float x, float y, SVGPathSegRole role)
{
    return SVGPathSegMovetoRel::create(this, role, x, y);
}

PassRefPtr<SVGPathSegLinetoAbs> SVGPathElement::createSVGPathSegLinetoAbs(float x, float y, SVGPathSegRole role)
{
    return SVGPathSegLinetoAbs::create(this, role, x, y);
}

PassRefPtr<SVGPathSegLinetoRel> SVGPathElement::createSVGPathSegLinetoRel(float x, float y, SVGPathSegRole role)
{
    return SVGPathSegLinetoRel::create(this, role, x, y);
}

PassRefPtr<SVGPathSegCurvetoCubicAbs> SVGPathElement::createSVGPathSegCurvetoCubicAbs(float x, float y, float x1, float y1, float x2, float y2, SVGPathSegRole role)
{
    return SVGPathSegCurvetoCubicAbs::create(this, role, x, y, x1, y1, x2, y2);
}

PassRefPtr<SVGPathSegCurvetoCubicRel> SVGPathElement::createSVGPathSegCurvetoCubicRel(float x, float y, float x1, float y1, float x2, float y2, SVGPathSegRole role)
{
    return SVGPathSegCurvetoCubicRel::create(this, role, x, y, x1, y1, x2, y2);
}

PassRefPtr<SVGPathSegCurvetoQuadraticAbs> SVGPathElement::createSVGPathSegCurvetoQuadraticAbs(float x, float y, float x1, float y1, SVGPathSegRole role)
{
    return SVGPathSegCurvetoQuadraticAbs::create(this, role, x, y, x1, y1);
}

PassRefPtr<SVGPathSegCurvetoQuadraticRel> SVGPathElement::createSVGPathSegCurvetoQuadraticRel(float x, float y, float x1, float y1, SVGPathSegRole role)
{
    return SVGPathSegCurvetoQuadraticRel::create(this, role, x, y, x1, y1);
}

PassRefPtr<SVGPathSegArcAbs> SVGPathElement::createSVGPathSegArcAbs(float x, float y, float r1, float r2, float angle, bool largeArcFlag, bool sweepFlag, SVGPathSegRole role)
{
    return SVGPathSegArcAbs::create(this, role, x, y, r1, r2, angle, largeArcFlag, sweepFlag);
}

PassRefPtr<SVGPathSegArcRel> SVGPathElement::createSVGPathSegArcRel(float x, float y, float r1, float r2, float angle, bool largeArcFlag, bool sweepFlag, SVGPathSegRole role)
{
    return SVGPathSegArcRel::create(this, role, x, y, r1, r2, angle, largeArcFlag, sweepFlag);
}

PassRefPtr<SVGPathSegLinetoHorizontalAbs> SVGPathElement::createSVGPathSegLinetoHorizontalAbs(float x, SVGPathSegRole role)
{
    return SVGPathSegLinetoHorizontalAbs::create(this, role, x);
}

PassRefPtr<SVGPathSegLinetoHorizontalRel> SVGPathElement::createSVGPathSegLinetoHorizontalRel(float x, SVGPathSegRole role)
{
    return SVGPathSegLinetoHorizontalRel::create(this, role, x);
}

PassRefPtr<SVGPathSegLinetoVerticalAbs> SVGPathElement::createSVGPathSegLinetoVerticalAbs(float y, SVGPathSegRole role)
{
    return SVGPathSegLinetoVerticalAbs::create(this, role, y);
}

PassRefPtr<SVGPathSegLinetoVerticalRel> SVGPathElement::createSVGPathSegLinetoVerticalRel(float y, SVGPathSegRole role)
{
    return SVGPathSegLinetoVerticalRel::create(this, role, y);
}

PassRefPtr<SVGPathSegCurvetoCubicSmoothAbs> SVGPathElement::createSVGPathSegCurvetoCubicSmoothAbs(float x, float y, float x2, float y2, SVGPathSegRole role)
{
    return SVGPathSegCurvetoCubicSmoothAbs::create(this, role, x, y, x2, y2);
}

PassRefPtr<SVGPathSegCurvetoCubicSmoothRel> SVGPathElement::createSVGPathSegCurvetoCubicSmoothRel(float x, float y, float x2, float y2, SVGPathSegRole role)
{
    return SVGPathSegCurvetoCubicSmoothRel::create(this, role, x, y, x2, y2);
}

PassRefPtr<SVGPathSegCurvetoQuadraticSmoothAbs> SVGPathElement::createSVGPathSegCurvetoQuadraticSmoothAbs(float x, float y, SVGPathSegRole role)
{
    return SVGPathSegCurvetoQuadraticSmoothAbs::create(this, role, x, y);
}

PassRefPtr<SVGPathSegCurvetoQuadraticSmoothRel> SVGPathElement::createSVGPathSegCurvetoQuadraticSmoothRel(float x, float y, SVGPathSegRole role)
{
    return SVGPathSegCurvetoQuadraticSmoothRel::create(this, role, x, y);
}

bool SVGPathElement::isSupportedAttribute(const QualifiedName& attrName)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, supportedAttributes, ());
    if (supportedAttributes.isEmpty()) {
        SVGTests::addSupportedAttributes(supportedAttributes);
        SVGLangSpace::addSupportedAttributes(supportedAttributes);
        SVGExternalResourcesRequired::addSupportedAttributes(supportedAttributes);
        supportedAttributes.add(SVGNames::dAttr);
        supportedAttributes.add(SVGNames::pathLengthAttr);
    }
    return supportedAttributes.contains<QualifiedName, SVGAttributeHashTranslator>(attrName);
}

void SVGPathElement::parseMappedAttribute(Attribute* attr)
{
    if (!isSupportedAttribute(attr->name())) {
        SVGStyledTransformableElement::parseMappedAttribute(attr);
        return;
    }

    if (attr->name() == SVGNames::dAttr) {
        SVGPathParserFactory* factory = SVGPathParserFactory::self();
        if (!factory->buildSVGPathByteStreamFromString(attr->value(), m_pathByteStream, UnalteredParsing))
            document()->accessSVGExtensions()->reportError("Problem parsing d=\"" + attr->value() + "\"");
        return;
    }

    if (attr->name() == SVGNames::pathLengthAttr) {
        setPathLengthBaseValue(attr->value().toFloat());
        if (pathLengthBaseValue() < 0)
            document()->accessSVGExtensions()->reportError("A negative value for path attribute <pathLength> is not allowed");
        return;
    }

    if (SVGTests::parseMappedAttribute(attr))
        return;
    if (SVGLangSpace::parseMappedAttribute(attr))
        return;
    if (SVGExternalResourcesRequired::parseMappedAttribute(attr))
        return;

    ASSERT_NOT_REACHED();
}

void SVGPathElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGStyledTransformableElement::svgAttributeChanged(attrName);
        return;
    }

    SVGElementInstance::InvalidationGuard invalidationGuard(this);
    
    if (SVGTests::handleAttributeChange(this, attrName))
        return;

    RenderSVGPath* renderer = static_cast<RenderSVGPath*>(this->renderer());

    if (attrName == SVGNames::dAttr) {
        if (SVGAnimatedProperty::lookupWrapper<SVGPathElement, SVGAnimatedPathSegListPropertyTearOff, true>(this, dPropertyInfo())) {
            SVGPathSegList newList(PathSegUnalteredRole);
            SVGPathParserFactory* factory = SVGPathParserFactory::self();
            factory->buildSVGPathSegListFromByteStream(m_pathByteStream.get(), this, newList, UnalteredParsing);
            m_pathSegList.value = newList;
        }

        if (renderer)
            renderer->setNeedsPathUpdate();
    }

    if (renderer)
        RenderSVGResource::markForLayoutAndParentResourceInvalidation(renderer);
}

PassRefPtr<SVGAnimatedProperty> SVGPathElement::lookupOrCreateDWrapper(void* contextElement)
{
    ASSERT(contextElement);
    SVGPathElement* ownerType = static_cast<SVGPathElement*>(contextElement);

    SVGPathParserFactory* factory = SVGPathParserFactory::self();
    factory->buildSVGPathSegListFromByteStream(ownerType->m_pathByteStream.get(), ownerType, ownerType->m_pathSegList.value, UnalteredParsing);

    return SVGAnimatedProperty::lookupOrCreateWrapper<SVGPathElement, SVGAnimatedPathSegListPropertyTearOff, SVGPathSegList, true>
           (ownerType, dPropertyInfo(), ownerType->m_pathSegList.value);
}

void SVGPathElement::synchronizeD(void* contextElement)
{
    ASSERT(contextElement);
    SVGPathElement* ownerType = static_cast<SVGPathElement*>(contextElement);
    if (!ownerType->m_pathSegList.shouldSynchronize)
        return;
    SVGAnimatedPropertySynchronizer<true>::synchronize(ownerType, dPropertyInfo()->attributeName, ownerType->m_pathSegList.value.valueAsString());
}

SVGPathSegListPropertyTearOff* SVGPathElement::pathSegList()
{
    m_pathSegList.shouldSynchronize = true;
    if (!m_animatablePathSegList)
        m_animatablePathSegList = static_pointer_cast<SVGAnimatedPathSegListPropertyTearOff>(lookupOrCreateDWrapper(this));
    return static_cast<SVGPathSegListPropertyTearOff*>(m_animatablePathSegList->baseVal(PathSegUnalteredRole));
}

SVGPathSegListPropertyTearOff* SVGPathElement::normalizedPathSegList()
{
    // FIXME: https://bugs.webkit.org/show_bug.cgi?id=15412 - Implement normalized path segment lists!
    return 0;
}

SVGPathSegListPropertyTearOff* SVGPathElement::animatedPathSegList()
{
    m_pathSegList.shouldSynchronize = true;
    if (!m_animatablePathSegList)
        m_animatablePathSegList = static_pointer_cast<SVGAnimatedPathSegListPropertyTearOff>(lookupOrCreateDWrapper(this));
    return static_cast<SVGPathSegListPropertyTearOff*>(m_animatablePathSegList->animVal(PathSegUnalteredRole));
}

SVGPathSegListPropertyTearOff* SVGPathElement::animatedNormalizedPathSegList()
{
    // FIXME: https://bugs.webkit.org/show_bug.cgi?id=15412 - Implement normalized path segment lists!
    return 0;
}

void SVGPathElement::toPathData(Path& path) const
{
    ASSERT(path.isEmpty());

    SVGPathParserFactory* factory = SVGPathParserFactory::self();
    factory->buildPathFromByteStream(m_pathByteStream.get(), path);
}

void SVGPathElement::pathSegListChanged(SVGPathSegRole role)
{
    SVGPathParserFactory* factory = SVGPathParserFactory::self();

    switch (role) {
    case PathSegNormalizedRole:
        // FIXME: https://bugs.webkit.org/show_bug.cgi?id=15412 - Implement normalized path segment lists!
        break;
    case PathSegUnalteredRole:
        m_pathByteStream->clear();
        factory->buildSVGPathByteStreamFromSVGPathSegList(m_pathSegList.value, m_pathByteStream, UnalteredParsing);
        break;
    case PathSegUndefinedRole:
        return;
    }

    invalidateSVGAttributes();

    RenderSVGPath* renderer = static_cast<RenderSVGPath*>(this->renderer());
    if (!renderer)
        return;

    renderer->setNeedsPathUpdate();
    RenderSVGResource::markForLayoutAndParentResourceInvalidation(renderer);
}

}

#endif // ENABLE(SVG)
