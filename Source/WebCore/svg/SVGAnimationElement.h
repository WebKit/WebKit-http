/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Cameron McCormack <cam@mcc.id.au>
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
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

#ifndef SVGAnimationElement_h
#define SVGAnimationElement_h

#if ENABLE(SVG)
#include "ElementTimeControl.h"
#include "Path.h"
#include "SMILTime.h"
#include "SVGAnimatedBoolean.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGSMILElement.h"
#include "SVGStringList.h"
#include "SVGTests.h"
#include "UnitBezier.h"

namespace WebCore {

enum AnimationMode {
    NoAnimation,
    FromToAnimation,
    FromByAnimation,
    ToAnimation,
    ByAnimation,
    ValuesAnimation,
    PathAnimation // Used by AnimateMotion.
};

enum CalcMode {
    CalcModeDiscrete,
    CalcModeLinear,
    CalcModePaced,
    CalcModeSpline
};

class ConditionEventListener;
class TimeContainer;

class SVGAnimationElement : public SVGSMILElement,
                            public SVGTests,
                            public SVGExternalResourcesRequired,
                            public ElementTimeControl {
public:
    // SVGAnimationElement
    float getStartTime() const;
    float getCurrentTime() const;
    float getSimpleDuration(ExceptionCode&) const;

    // ElementTimeControl
    virtual void beginElement();
    virtual void beginElementAt(float offset);
    virtual void endElement();
    virtual void endElementAt(float offset);

    static bool isTargetAttributeCSSProperty(SVGElement*, const QualifiedName&);

    bool isAdditive() const;
    bool isAccumulated() const;
    AnimationMode animationMode() const;
    CalcMode calcMode() const;

protected:
    SVGAnimationElement(const QualifiedName&, Document*);

    bool isSupportedAttribute(const QualifiedName&);
    virtual void parseMappedAttribute(Attribute*);

    enum AttributeType {
        AttributeTypeCSS,
        AttributeTypeXML,
        AttributeTypeAuto
    };
    AttributeType attributeType() const;

    String toValue() const;
    String byValue() const;
    String fromValue() const;

    String targetAttributeBaseValue() const;
    void setTargetAttributeAnimatedValue(const String&);

    // from SVGSMILElement
    virtual void startedActiveInterval();
    virtual void updateAnimation(float percent, unsigned repeat, SVGSMILElement* resultElement);
    virtual void endedActiveInterval();

private:
    virtual void attributeChanged(Attribute*, bool preserveDecls);

    virtual bool calculateFromAndToValues(const String& fromString, const String& toString) = 0;
    virtual bool calculateFromAndByValues(const String& fromString, const String& byString) = 0;
    virtual void calculateAnimatedValue(float percentage, unsigned repeat, SVGSMILElement* resultElement) = 0;
    virtual float calculateDistance(const String& /*fromString*/, const String& /*toString*/) { return -1.f; }
    virtual Path animationPath() const { return Path(); }

    void currentValuesForValuesAnimation(float percent, float& effectivePercent, String& from, String& to);
    void calculateKeyTimesForCalcModePaced();
    float calculatePercentFromKeyPoints(float percent) const;
    void currentValuesFromKeyPoints(float percent, float& effectivePercent, String& from, String& to) const;
    float calculatePercentForSpline(float percent, unsigned splineIndex) const;
    float calculatePercentForFromTo(float percent) const;
    unsigned calculateKeyTimesIndex(float percent) const;

    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGAnimationElement)
        DECLARE_ANIMATED_BOOLEAN(ExternalResourcesRequired, externalResourcesRequired)
    END_DECLARE_ANIMATED_PROPERTIES

    // SVGTests
    virtual void synchronizeRequiredFeatures() { SVGTests::synchronizeRequiredFeatures(this); }
    virtual void synchronizeRequiredExtensions() { SVGTests::synchronizeRequiredExtensions(this); }
    virtual void synchronizeSystemLanguage() { SVGTests::synchronizeSystemLanguage(this); }

    bool m_animationValid;

    Vector<String> m_values;
    Vector<float> m_keyTimes;
    Vector<float> m_keyPoints;
    Vector<UnitBezier> m_keySplines;
    String m_lastValuesAnimationFrom;
    String m_lastValuesAnimationTo;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGAnimationElement_h
