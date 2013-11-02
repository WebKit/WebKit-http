/*
 * Copyright (C) 2004 Zack Rusin <zack@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2008, 2012, 2013 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef CSSComputedStyleDeclaration_h
#define CSSComputedStyleDeclaration_h

#include "CSSStyleDeclaration.h"
#include "RenderStyleConstants.h"
#include <wtf/RefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class CSSPrimitiveValue;
class CSSValueList;
class Color;
class FilterOperations;
class MutableStylePropertySet;
class Node;
class RenderObject;
class RenderStyle;
class SVGPaint;
class ShadowData;
class StylePropertySet;
class StylePropertyShorthand;

#if ENABLE(CSS_SHADERS)
class CustomFilterNumberParameter;
class CustomFilterParameter;
#endif

enum EUpdateLayout { DoNotUpdateLayout = false, UpdateLayout = true };

enum AdjustPixelValuesForComputedStyle { AdjustPixelValues, DoNotAdjustPixelValues };

class ComputedStyleExtractor {
public:
    ComputedStyleExtractor(PassRefPtr<Node>, bool allowVisitedStyle = false, PseudoId = NOPSEUDO);

    PassRefPtr<CSSValue> propertyValue(CSSPropertyID, EUpdateLayout = UpdateLayout) const;

    // Helper methods for HTML editing.
    PassRef<MutableStylePropertySet> copyPropertiesInSet(const CSSPropertyID* set, unsigned length) const;
    PassRef<MutableStylePropertySet> copyProperties() const;
    PassRefPtr<CSSPrimitiveValue> getFontSizeCSSValuePreferringKeyword() const;
    bool useFixedFontDefaultSize() const;
    bool propertyMatches(CSSPropertyID, const CSSValue*) const;

#if ENABLE(CSS_FILTERS)
    static PassRefPtr<CSSValue> valueForFilter(const RenderObject*, const RenderStyle*, const FilterOperations&, AdjustPixelValuesForComputedStyle = AdjustPixelValues);
#endif

private:
    // The styled node is either the node passed into computedPropertyValue, or the
    // PseudoElement for :before and :after if they exist.
    // FIXME: This should be styledElement since in JS getComputedStyle only works
    // on Elements, but right now editing creates these for text nodes. We should fix that.
    Node* styledNode() const;

#if ENABLE(SVG)
    PassRefPtr<CSSValue> svgPropertyValue(CSSPropertyID, EUpdateLayout) const;
    PassRefPtr<SVGPaint> adjustSVGPaintForCurrentColor(PassRefPtr<SVGPaint>, RenderStyle*) const;
#endif

    static PassRefPtr<CSSValue> valueForShadow(const ShadowData*, CSSPropertyID, const RenderStyle*, AdjustPixelValuesForComputedStyle = AdjustPixelValues);
    PassRefPtr<CSSPrimitiveValue> currentColorOrValidColor(RenderStyle*, const Color&) const;

    PassRefPtr<CSSValueList> getCSSPropertyValuesForShorthandProperties(const StylePropertyShorthand&) const;
    PassRefPtr<CSSValueList> getCSSPropertyValuesForSidesShorthand(const StylePropertyShorthand&) const;
    PassRefPtr<CSSValueList> getBackgroundShorthandValue() const;
    PassRefPtr<CSSValueList> getCSSPropertyValuesForGridShorthand(const StylePropertyShorthand&) const;

    RefPtr<Node> m_node;
    PseudoId m_pseudoElementSpecifier;
    bool m_allowVisitedStyle;
};

class CSSComputedStyleDeclaration : public CSSStyleDeclaration {
public:
    static PassRefPtr<CSSComputedStyleDeclaration> create(PassRefPtr<Node> node, bool allowVisitedStyle = false, const String& pseudoElementName = String())
    {
        return adoptRef(new CSSComputedStyleDeclaration(node, allowVisitedStyle, pseudoElementName));
    }
    virtual ~CSSComputedStyleDeclaration();

    virtual void ref() OVERRIDE;
    virtual void deref() OVERRIDE;

    String getPropertyValue(CSSPropertyID) const;

private:
    CSSComputedStyleDeclaration(PassRefPtr<Node>, bool allowVisitedStyle, const String&);

    // CSSOM functions. Don't make these public.
    virtual CSSRule* parentRule() const OVERRIDE;
    virtual unsigned length() const OVERRIDE;
    virtual String item(unsigned index) const OVERRIDE;
    virtual PassRefPtr<CSSValue> getPropertyCSSValue(const String& propertyName) OVERRIDE;
    virtual String getPropertyValue(const String& propertyName) OVERRIDE;
    virtual String getPropertyPriority(const String& propertyName) OVERRIDE;
    virtual String getPropertyShorthand(const String& propertyName) OVERRIDE;
    virtual bool isPropertyImplicit(const String& propertyName) OVERRIDE;
    virtual void setProperty(const String& propertyName, const String& value, const String& priority, ExceptionCode&) OVERRIDE;
    virtual String removeProperty(const String& propertyName, ExceptionCode&) OVERRIDE;
    virtual String cssText() const OVERRIDE;
    virtual void setCssText(const String&, ExceptionCode&) OVERRIDE;
    virtual PassRefPtr<CSSValue> getPropertyCSSValueInternal(CSSPropertyID) OVERRIDE;
    virtual String getPropertyValueInternal(CSSPropertyID) OVERRIDE;
    virtual void setPropertyInternal(CSSPropertyID, const String& value, bool important, ExceptionCode&) OVERRIDE;
    virtual PassRef<MutableStylePropertySet> copyProperties() const OVERRIDE;

    PassRefPtr<CSSValue> getPropertyCSSValue(CSSPropertyID, EUpdateLayout = UpdateLayout) const;

    RefPtr<Node> m_node;
    PseudoId m_pseudoElementSpecifier;
    bool m_allowVisitedStyle;
    unsigned m_refCount;
};

} // namespace WebCore

#endif // CSSComputedStyleDeclaration_h
