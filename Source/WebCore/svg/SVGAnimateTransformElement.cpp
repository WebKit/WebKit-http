/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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
#include "SVGAnimateTransformElement.h"

#include "AffineTransform.h"
#include "Attribute.h"
#include "RenderObject.h"
#include "RenderSVGResource.h"
#include "SVGAngle.h"
#include "SVGElementInstance.h"
#include "SVGGradientElement.h"
#include "SVGNames.h"
#include "SVGParserUtilities.h"
#include "SVGPatternElement.h"
#include "SVGSVGElement.h"
#include "SVGStyledTransformableElement.h"
#include "SVGTextElement.h"
#include "SVGTransform.h"
#include "SVGTransformList.h"
#include "SVGUseElement.h"
#include <wtf/MathExtras.h>

using namespace std;

namespace WebCore {

inline SVGAnimateTransformElement::SVGAnimateTransformElement(const QualifiedName& tagName, Document* document)
    : SVGAnimationElement(tagName, document)
    , m_type(SVGTransform::SVG_TRANSFORM_UNKNOWN)
    , m_baseIndexInTransformList(0)
{
    ASSERT(hasTagName(SVGNames::animateTransformTag));
}

PassRefPtr<SVGAnimateTransformElement> SVGAnimateTransformElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGAnimateTransformElement(tagName, document));
}

bool SVGAnimateTransformElement::hasValidAttributeType()
{
    SVGElement* targetElement = this->targetElement();
    if (!targetElement)
        return false;
    
    return determineAnimatedPropertyType(targetElement) == AnimatedTransformList;
}

AnimatedPropertyType SVGAnimateTransformElement::determineAnimatedPropertyType(SVGElement* targetElement) const
{
    ASSERT(targetElement);
    
    // Just transform lists can be animated with <animateTransform>
    // http://www.w3.org/TR/SVG/animate.html#AnimationAttributesAndProperties
    Vector<AnimatedPropertyType> propertyTypes;
    targetElement->animatedPropertyTypeForAttribute(attributeName(), propertyTypes);
    if (propertyTypes.isEmpty() || propertyTypes[0] != AnimatedTransformList)
        return AnimatedUnknown;

    ASSERT(propertyTypes.size() == 1);
    return AnimatedTransformList;
}

bool SVGAnimateTransformElement::isSupportedAttribute(const QualifiedName& attrName)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, supportedAttributes, ());
    if (supportedAttributes.isEmpty())
        supportedAttributes.add(SVGNames::typeAttr);
    return supportedAttributes.contains<QualifiedName, SVGAttributeHashTranslator>(attrName);
}

void SVGAnimateTransformElement::parseAttribute(Attribute* attr)
{
    if (!isSupportedAttribute(attr->name())) {
        SVGAnimationElement::parseAttribute(attr);
        return;
    }

    if (attr->name() == SVGNames::typeAttr) {
        const AtomicString& value = attr->value();
        if (value == "translate")
            m_type = SVGTransform::SVG_TRANSFORM_TRANSLATE;
        else if (value == "scale")
            m_type = SVGTransform::SVG_TRANSFORM_SCALE;
        else if (value == "rotate")
            m_type = SVGTransform::SVG_TRANSFORM_ROTATE;
        else if (value == "skewX")
            m_type = SVGTransform::SVG_TRANSFORM_SKEWX;
        else if (value == "skewY")
            m_type = SVGTransform::SVG_TRANSFORM_SKEWY;
        return;
    }

    ASSERT_NOT_REACHED();
}

static PassRefPtr<SVGAnimatedTransformList> animatedTransformListFor(SVGElement* element)
{
    ASSERT(element);
    if (element->isStyledTransformable())
        return static_cast<SVGStyledTransformableElement*>(element)->transformAnimated();
    if (element->hasTagName(SVGNames::textTag))
        return static_cast<SVGTextElement*>(element)->transformAnimated();
    if (element->hasTagName(SVGNames::linearGradientTag) || element->hasTagName(SVGNames::radialGradientTag))
        return static_cast<SVGGradientElement*>(element)->gradientTransformAnimated();
    if (element->hasTagName(SVGNames::patternTag))
        return static_cast<SVGPatternElement*>(element)->patternTransformAnimated();
    return 0;
}
    
void SVGAnimateTransformElement::resetToBaseValue(const String& baseValue)
{
    SVGElement* targetElement = this->targetElement();
    if (!targetElement || determineAnimatedPropertyType(targetElement) == AnimatedUnknown)
        return;

    // FIXME: This might not be correct for accumulated sum. Needs checking.
    if (targetElement->hasTagName(SVGNames::linearGradientTag) || targetElement->hasTagName(SVGNames::radialGradientTag)) {
        targetElement->setAttribute(SVGNames::gradientTransformAttr, baseValue.isEmpty() ? "matrix(1 0 0 1 0 0)" : baseValue);
        return;
    }
    if (targetElement->hasTagName(SVGNames::patternTag)) {
        targetElement->setAttribute(SVGNames::patternTransformAttr, baseValue.isEmpty() ? "matrix(1 0 0 1 0 0)" : baseValue);
        return;
    }
    
    if (baseValue.isEmpty()) {
        if (RefPtr<SVGAnimatedTransformList> list = animatedTransformListFor(targetElement)) {
            list->detachListWrappers(0);
            list->values().clear();
        }
    } else
        targetElement->setAttribute(SVGNames::transformAttr, baseValue);
}

void SVGAnimateTransformElement::calculateAnimatedValue(float percentage, unsigned repeat, SVGSMILElement*)
{
    SVGElement* targetElement = this->targetElement();
    if (!targetElement || determineAnimatedPropertyType(targetElement) == AnimatedUnknown)
        return;
    RefPtr<SVGAnimatedTransformList> animatedList = animatedTransformListFor(targetElement);
    ASSERT(animatedList);
    
    if (calcMode() == CalcModeDiscrete)
        percentage = percentage < 0.5 ? 0 : 1;

    if (!isAdditive()) {
        animatedList->detachListWrappers(0);
        animatedList->values().clear();
    }
    if (isAccumulated() && repeat)
        percentage += repeat;
    SVGTransform transform = SVGTransformDistance(m_fromTransform, m_toTransform).scaledDistance(percentage).addToSVGTransform(m_fromTransform);
    animatedList->values().append(transform);
    animatedList->wrappers().append(RefPtr<SVGPropertyTearOff<SVGTransform> >());
}
    
bool SVGAnimateTransformElement::calculateFromAndToValues(const String& fromString, const String& toString)
{
    m_fromTransform = parseTransformValue(fromString);
    if (!m_fromTransform.isValid())
        return false;
    m_toTransform = parseTransformValue(toString);
    return m_toTransform.isValid();
}

bool SVGAnimateTransformElement::calculateFromAndByValues(const String& fromString, const String& byString)
{
    m_fromTransform = parseTransformValue(fromString);
    if (!m_fromTransform.isValid())
        return false;
    m_toTransform = SVGTransformDistance::addSVGTransforms(m_fromTransform, parseTransformValue(byString));
    return m_toTransform.isValid();
}

SVGTransform SVGAnimateTransformElement::parseTransformValue(const String& value) const
{
    if (value.isEmpty())
        return SVGTransform(m_type);
    SVGTransform result;
    // FIXME: This is pretty dumb but parseTransformValue() wants those parenthesis.
    String parseString("(" + value + ")");
    const UChar* ptr = parseString.characters();
    SVGTransformable::parseTransformValue(m_type, ptr, ptr + parseString.length(), result); // ignoring return value
    return result;
}
    
void SVGAnimateTransformElement::applyResultsToTarget()
{
    SVGElement* targetElement = this->targetElement();
    if (!targetElement || determineAnimatedPropertyType(targetElement) == AnimatedUnknown)
        return;

    // We accumulate to the target element transform list so there is not much to do here.
    if (RenderObject* renderer = targetElement->renderer()) {
        renderer->setNeedsTransformUpdate();
        RenderSVGResource::markForLayoutAndParentResourceInvalidation(renderer);
    }

    // ...except in case where we have additional instances in <use> trees.
    RefPtr<SVGAnimatedTransformList> animatedList = animatedTransformListFor(targetElement);
    if (!animatedList)
        return;
    SVGTransformList* transformList = &animatedList->values();

    const HashSet<SVGElementInstance*>& instances = targetElement->instancesForElement();
    const HashSet<SVGElementInstance*>::const_iterator end = instances.end();
    for (HashSet<SVGElementInstance*>::const_iterator it = instances.begin(); it != end; ++it) {
        SVGElement* shadowTreeElement = (*it)->shadowTreeElement();
        ASSERT(shadowTreeElement);
        if (shadowTreeElement->isStyledTransformable())
            static_cast<SVGStyledTransformableElement*>(shadowTreeElement)->setTransformBaseValue(*transformList);
        else if (shadowTreeElement->hasTagName(SVGNames::textTag))
            static_cast<SVGTextElement*>(shadowTreeElement)->setTransformBaseValue(*transformList);
        else if (shadowTreeElement->hasTagName(SVGNames::linearGradientTag) || shadowTreeElement->hasTagName(SVGNames::radialGradientTag))
            static_cast<SVGGradientElement*>(shadowTreeElement)->setGradientTransformBaseValue(*transformList);
        else if (shadowTreeElement->hasTagName(SVGNames::patternTag))
            static_cast<SVGPatternElement*>(shadowTreeElement)->setPatternTransformBaseValue(*transformList);
        if (RenderObject* renderer = shadowTreeElement->renderer()) {
            renderer->setNeedsTransformUpdate();
            RenderSVGResource::markForLayoutAndParentResourceInvalidation(renderer);
        }
    }
}
    
float SVGAnimateTransformElement::calculateDistance(const String& fromString, const String& toString)
{
    // FIXME: This is not correct in all cases. The spec demands that each component (translate x and y for example) 
    // is paced separately. To implement this we need to treat each component as individual animation everywhere.
    SVGTransform from = parseTransformValue(fromString);
    if (!from.isValid())
        return -1;
    SVGTransform to = parseTransformValue(toString);
    if (!to.isValid() || from.type() != to.type())
        return -1;
    if (to.type() == SVGTransform::SVG_TRANSFORM_TRANSLATE) {
        FloatSize diff = to.translate() - from.translate();
        return sqrtf(diff.width() * diff.width() + diff.height() * diff.height());
    }
    if (to.type() == SVGTransform::SVG_TRANSFORM_ROTATE)
        return fabsf(to.angle() - from.angle());
    if (to.type() == SVGTransform::SVG_TRANSFORM_SCALE) {
        FloatSize diff = to.scale() - from.scale();
        return sqrtf(diff.width() * diff.width() + diff.height() * diff.height());
    }
    return -1;
}

}
#endif // ENABLE(SVG)
