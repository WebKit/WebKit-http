/*
 * Copyright (C) 2006 Oliver Hunt <oliver@nerget.com>
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

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "SVGFEDisplacementMapElement.h"

#include "Attribute.h"
#include "FilterEffect.h"
#include "SVGFilterBuilder.h"
#include "SVGNames.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_STRING(SVGFEDisplacementMapElement, SVGNames::inAttr, In1, in1)
DEFINE_ANIMATED_STRING(SVGFEDisplacementMapElement, SVGNames::in2Attr, In2, in2)
DEFINE_ANIMATED_ENUMERATION(SVGFEDisplacementMapElement, SVGNames::xChannelSelectorAttr, XChannelSelector, xChannelSelector, ChannelSelectorType)
DEFINE_ANIMATED_ENUMERATION(SVGFEDisplacementMapElement, SVGNames::yChannelSelectorAttr, YChannelSelector, yChannelSelector, ChannelSelectorType)
DEFINE_ANIMATED_NUMBER(SVGFEDisplacementMapElement, SVGNames::scaleAttr, Scale, scale)

inline SVGFEDisplacementMapElement::SVGFEDisplacementMapElement(const QualifiedName& tagName, Document* document)
    : SVGFilterPrimitiveStandardAttributes(tagName, document)
    , m_xChannelSelector(CHANNEL_A)
    , m_yChannelSelector(CHANNEL_A)
{
    ASSERT(hasTagName(SVGNames::feDisplacementMapTag));
}

PassRefPtr<SVGFEDisplacementMapElement> SVGFEDisplacementMapElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGFEDisplacementMapElement(tagName, document));
}

void SVGFEDisplacementMapElement::parseMappedAttribute(Attribute* attr)
{
    const String& value = attr->value();
    if (attr->name() == SVGNames::xChannelSelectorAttr) {
        ChannelSelectorType propertyValue = SVGPropertyTraits<ChannelSelectorType>::fromString(value);
        if (propertyValue > 0)
            setXChannelSelectorBaseValue(propertyValue);
    } else if (attr->name() == SVGNames::yChannelSelectorAttr) {
        ChannelSelectorType propertyValue = SVGPropertyTraits<ChannelSelectorType>::fromString(value);
        if (propertyValue > 0)
            setYChannelSelectorBaseValue(propertyValue);
    } else if (attr->name() == SVGNames::inAttr)
        setIn1BaseValue(value);
    else if (attr->name() == SVGNames::in2Attr)
        setIn2BaseValue(value);
    else if (attr->name() == SVGNames::scaleAttr)
        setScaleBaseValue(value.toFloat());
    else
        SVGFilterPrimitiveStandardAttributes::parseMappedAttribute(attr);
}

bool SVGFEDisplacementMapElement::setFilterEffectAttribute(FilterEffect* effect, const QualifiedName& attrName)
{
    FEDisplacementMap* displacementMap = static_cast<FEDisplacementMap*>(effect);
    if (attrName == SVGNames::xChannelSelectorAttr)
        return displacementMap->setXChannelSelector(xChannelSelector());
    if (attrName == SVGNames::yChannelSelectorAttr)
        return displacementMap->setYChannelSelector(yChannelSelector());
    if (attrName == SVGNames::scaleAttr)
        return displacementMap->setScale(scale());

    ASSERT_NOT_REACHED();
    return false;
}

void SVGFEDisplacementMapElement::svgAttributeChanged(const QualifiedName& attrName)
{
    SVGFilterPrimitiveStandardAttributes::svgAttributeChanged(attrName);

    if (attrName == SVGNames::xChannelSelectorAttr || attrName == SVGNames::yChannelSelectorAttr || attrName == SVGNames::scaleAttr)
        primitiveAttributeChanged(attrName);
    else if (attrName == SVGNames::inAttr || attrName == SVGNames::in2Attr)
        invalidate();
}

void SVGFEDisplacementMapElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGFilterPrimitiveStandardAttributes::synchronizeProperty(attrName);

    if (attrName == anyQName()) {
        synchronizeXChannelSelector();
        synchronizeYChannelSelector();
        synchronizeIn1();
        synchronizeIn2();
        synchronizeScale();
        return;
    }

    if (attrName == SVGNames::xChannelSelectorAttr)
        synchronizeXChannelSelector();
    else if (attrName == SVGNames::yChannelSelectorAttr)
        synchronizeYChannelSelector();
    else if (attrName == SVGNames::inAttr)
        synchronizeIn1();
    else if (attrName == SVGNames::in2Attr)
        synchronizeIn2();
    else if (attrName == SVGNames::scaleAttr)
        synchronizeScale();
}

AttributeToPropertyTypeMap& SVGFEDisplacementMapElement::attributeToPropertyTypeMap()
{
    DEFINE_STATIC_LOCAL(AttributeToPropertyTypeMap, s_attributeToPropertyTypeMap, ());
    return s_attributeToPropertyTypeMap;
}

void SVGFEDisplacementMapElement::fillAttributeToPropertyTypeMap()
{
    AttributeToPropertyTypeMap& attributeToPropertyTypeMap = this->attributeToPropertyTypeMap();

    SVGFilterPrimitiveStandardAttributes::fillPassedAttributeToPropertyTypeMap(attributeToPropertyTypeMap);
    attributeToPropertyTypeMap.set(SVGNames::inAttr, AnimatedString);
    attributeToPropertyTypeMap.set(SVGNames::in2Attr, AnimatedString);
    attributeToPropertyTypeMap.set(SVGNames::xChannelSelectorAttr, AnimatedEnumeration);
    attributeToPropertyTypeMap.set(SVGNames::yChannelSelectorAttr, AnimatedEnumeration);
    attributeToPropertyTypeMap.set(SVGNames::scaleAttr, AnimatedNumber);
}

PassRefPtr<FilterEffect> SVGFEDisplacementMapElement::build(SVGFilterBuilder* filterBuilder, Filter* filter)
{
    FilterEffect* input1 = filterBuilder->getEffectById(in1());
    FilterEffect* input2 = filterBuilder->getEffectById(in2());
    
    if (!input1 || !input2)
        return 0;

    RefPtr<FilterEffect> effect = FEDisplacementMap::create(filter, xChannelSelector(), yChannelSelector(), scale());
    FilterEffectVector& inputEffects = effect->inputEffects();
    inputEffects.reserveCapacity(2);
    inputEffects.append(input1);
    inputEffects.append(input2);    
    return effect.release();
}

}

#endif // ENABLE(SVG)
