/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Research In Motion Limited. All rights reserved.
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

#define WTF_DEPRECATED_STRING_OPERATORS

#include "config.h"
#include "StylePropertySet.h"

#include "CSSParser.h"
#include "CSSValueKeywords.h"
#include "CSSValueList.h"
#include "CSSValuePool.h"
#include "Document.h"
#include "PropertySetCSSStyleDeclaration.h"
#include "StylePropertyShorthand.h"
#include "StyleSheetContents.h"
#include <wtf/BitArray.h>
#include <wtf/text/StringBuilder.h>

#ifndef NDEBUG
#include <stdio.h>
#include <wtf/ASCIICType.h>
#include <wtf/text/CString.h>
#endif

using namespace std;

namespace WebCore {

typedef HashMap<const StylePropertySet*, OwnPtr<PropertySetCSSStyleDeclaration> > PropertySetCSSOMWrapperMap;
static PropertySetCSSOMWrapperMap& propertySetCSSOMWrapperMap()
{
    DEFINE_STATIC_LOCAL(PropertySetCSSOMWrapperMap, propertySetCSSOMWrapperMapInstance, ());
    return propertySetCSSOMWrapperMapInstance;
}

static size_t immutableStylePropertySetSize(unsigned count)
{
    return sizeof(StylePropertySet) - sizeof(void*) + sizeof(CSSProperty) * count;
}

PassRefPtr<StylePropertySet> StylePropertySet::createImmutable(const CSSProperty* properties, unsigned count, CSSParserMode cssParserMode)
{
    void* slot = WTF::fastMalloc(immutableStylePropertySetSize(count));
    return adoptRef(new (slot) StylePropertySet(properties, count, cssParserMode, /* makeMutable */ false));
}

PassRefPtr<StylePropertySet> StylePropertySet::immutableCopyIfNeeded() const
{
    if (!isMutable())
        return const_cast<StylePropertySet*>(this);
    return createImmutable(m_mutablePropertyVector->data(), m_mutablePropertyVector->size(), cssParserMode());
}

StylePropertySet::StylePropertySet(CSSParserMode cssParserMode)
    : m_cssParserMode(cssParserMode)
    , m_ownsCSSOMWrapper(false)
    , m_isMutable(true)
    , m_arraySize(0)
    , m_mutablePropertyVector(new Vector<CSSProperty>)
{
}

StylePropertySet::StylePropertySet(const CSSProperty* properties, unsigned count, CSSParserMode cssParserMode, bool makeMutable)
    : m_cssParserMode(cssParserMode)
    , m_ownsCSSOMWrapper(false)
    , m_isMutable(makeMutable)
{
    if (makeMutable) {
        m_mutablePropertyVector = new Vector<CSSProperty>;
        m_mutablePropertyVector->reserveInitialCapacity(count);
        for (unsigned i = 0; i < count; ++i)
            m_mutablePropertyVector->uncheckedAppend(properties[i]);
    } else {
        m_arraySize = count;
        for (unsigned i = 0; i < m_arraySize; ++i)
            new (&array()[i]) CSSProperty(properties[i]);
    }
}

StylePropertySet::StylePropertySet(const StylePropertySet& other)
    : RefCounted<StylePropertySet>()
    , m_cssParserMode(other.m_cssParserMode)
    , m_ownsCSSOMWrapper(false)
    , m_isMutable(true)
    , m_arraySize(0)
    , m_mutablePropertyVector(new Vector<CSSProperty>)
{
    if (other.isMutable())
        *m_mutablePropertyVector = *other.m_mutablePropertyVector;
    else {
        m_mutablePropertyVector->clear();
        m_mutablePropertyVector->reserveCapacity(other.m_arraySize);
        for (unsigned i = 0; i < other.m_arraySize; ++i)
            m_mutablePropertyVector->uncheckedAppend(other.array()[i]);
    }
}

StylePropertySet::~StylePropertySet()
{
    ASSERT(!m_ownsCSSOMWrapper || propertySetCSSOMWrapperMap().contains(this));
    if (m_ownsCSSOMWrapper)
        propertySetCSSOMWrapperMap().remove(this);
    if (isMutable())
        delete m_mutablePropertyVector;
    else {
        for (unsigned i = 0; i < m_arraySize; ++i)
            array()[i].~CSSProperty();
    }
}

String StylePropertySet::getPropertyValue(CSSPropertyID propertyID) const
{
    RefPtr<CSSValue> value = getPropertyCSSValue(propertyID);
    if (value)
        return value->cssText();

    // Shorthand and 4-values properties
    switch (propertyID) {
    case CSSPropertyBorderSpacing:
        return borderSpacingValue(borderSpacingShorthand());
    case CSSPropertyBackgroundPosition:
        return getLayeredShorthandValue(backgroundPositionShorthand());
    case CSSPropertyBackgroundRepeat:
        return getLayeredShorthandValue(backgroundRepeatShorthand());
    case CSSPropertyBackground:
        return getLayeredShorthandValue(backgroundShorthand());
    case CSSPropertyBorder:
        return borderPropertyValue(OmitUncommonValues);
    case CSSPropertyBorderTop:
        return getShorthandValue(borderTopShorthand());
    case CSSPropertyBorderRight:
        return getShorthandValue(borderRightShorthand());
    case CSSPropertyBorderBottom:
        return getShorthandValue(borderBottomShorthand());
    case CSSPropertyBorderLeft:
        return getShorthandValue(borderLeftShorthand());
    case CSSPropertyOutline:
        return getShorthandValue(outlineShorthand());
    case CSSPropertyBorderColor:
        return get4Values(borderColorShorthand());
    case CSSPropertyBorderWidth:
        return get4Values(borderWidthShorthand());
    case CSSPropertyBorderStyle:
        return get4Values(borderStyleShorthand());
    case CSSPropertyWebkitFlex:
        return getShorthandValue(webkitFlexShorthand());
    case CSSPropertyWebkitFlexFlow:
        return getShorthandValue(webkitFlexFlowShorthand());
    case CSSPropertyFont:
        return fontValue();
    case CSSPropertyMargin:
        return get4Values(marginShorthand());
    case CSSPropertyOverflow:
        return getCommonValue(overflowShorthand());
    case CSSPropertyPadding:
        return get4Values(paddingShorthand());
    case CSSPropertyListStyle:
        return getShorthandValue(listStyleShorthand());
    case CSSPropertyWebkitMaskPosition:
        return getLayeredShorthandValue(webkitMaskPositionShorthand());
    case CSSPropertyWebkitMaskRepeat:
        return getLayeredShorthandValue(webkitMaskRepeatShorthand());
    case CSSPropertyWebkitMask:
        return getLayeredShorthandValue(webkitMaskShorthand());
    case CSSPropertyWebkitTransformOrigin:
        return getShorthandValue(webkitTransformOriginShorthand());
    case CSSPropertyWebkitTransition:
        return getLayeredShorthandValue(webkitTransitionShorthand());
    case CSSPropertyWebkitAnimation:
        return getLayeredShorthandValue(webkitAnimationShorthand());
#if ENABLE(CSS_EXCLUSIONS)
    case CSSPropertyWebkitWrap:
        return getShorthandValue(webkitWrapShorthand());
#endif
#if ENABLE(SVG)
    case CSSPropertyMarker: {
        RefPtr<CSSValue> value = getPropertyCSSValue(CSSPropertyMarkerStart);
        if (value)
            return value->cssText();
        return String();
    }
#endif
    case CSSPropertyBorderRadius:
        return get4Values(borderRadiusShorthand());
    default:
          return String();
    }
}

String StylePropertySet::borderSpacingValue(const StylePropertyShorthand& shorthand) const
{
    RefPtr<CSSValue> horizontalValue = getPropertyCSSValue(shorthand.properties()[0]);
    RefPtr<CSSValue> verticalValue = getPropertyCSSValue(shorthand.properties()[1]);

    // While standard border-spacing property does not allow specifying border-spacing-vertical without
    // specifying border-spacing-horizontal <http://www.w3.org/TR/CSS21/tables.html#separated-borders>,
    // -webkit-border-spacing-vertical can be set without -webkit-border-spacing-horizontal.
    if (!horizontalValue || !verticalValue)
        return String();

    String horizontalValueCSSText = horizontalValue->cssText();
    String verticalValueCSSText = verticalValue->cssText();
    if (horizontalValueCSSText == verticalValueCSSText)
        return horizontalValueCSSText;
    return horizontalValueCSSText + ' ' + verticalValueCSSText;
}

bool StylePropertySet::appendFontLonghandValueIfExplicit(CSSPropertyID propertyId, StringBuilder& result) const
{
    const CSSProperty* property = findPropertyWithId(propertyId);
    if (!property)
        return false; // All longhands must have at least implicit values if "font" is specified.
    if (property->isImplicit())
        return true;

    char prefix = '\0';
    switch (propertyId) {
    case CSSPropertyFontStyle:
        break; // No prefix.
    case CSSPropertyFontFamily:
    case CSSPropertyFontVariant:
    case CSSPropertyFontWeight:
        prefix = ' ';
        break;
    case CSSPropertyLineHeight:
        prefix = '/';
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    if (prefix && !result.isEmpty())
        result.append(prefix);
    result.append(property->value()->cssText());

    return true;
}

String StylePropertySet::fontValue() const
{
    const CSSProperty* fontSizeProperty = findPropertyWithId(CSSPropertyFontSize);
    if (!fontSizeProperty || fontSizeProperty->isImplicit())
        return emptyString();

    StringBuilder result;
    bool success = true;
    success &= appendFontLonghandValueIfExplicit(CSSPropertyFontStyle, result);
    success &= appendFontLonghandValueIfExplicit(CSSPropertyFontVariant, result);
    success &= appendFontLonghandValueIfExplicit(CSSPropertyFontWeight, result);
    if (!result.isEmpty())
        result.append(' ');
    result.append(fontSizeProperty->value()->cssText());
    success &= appendFontLonghandValueIfExplicit(CSSPropertyLineHeight, result);
    success &= appendFontLonghandValueIfExplicit(CSSPropertyFontFamily, result);
    if (!success) {
        // An invalid "font" value has been built (should never happen, as at least implicit values
        // for mandatory longhands are always found in the style), report empty value instead.
        ASSERT_NOT_REACHED();
        return emptyString();
    }
    return result.toString();
}

String StylePropertySet::get4Values(const StylePropertyShorthand& shorthand) const
{
    // Assume the properties are in the usual order top, right, bottom, left.
    const CSSProperty* top = findPropertyWithId(shorthand.properties()[0]);
    const CSSProperty* right = findPropertyWithId(shorthand.properties()[1]);
    const CSSProperty* bottom = findPropertyWithId(shorthand.properties()[2]);
    const CSSProperty* left = findPropertyWithId(shorthand.properties()[3]);

    // All 4 properties must be specified.
    if (!top || !top->value() || !right || !right->value() || !bottom || !bottom->value() || !left || !left->value())
        return String();
    if (top->value()->isInitialValue() || right->value()->isInitialValue() || bottom->value()->isInitialValue() || left->value()->isInitialValue())
        return String();
    if (top->isImportant() != right->isImportant() || right->isImportant() != bottom->isImportant() || bottom->isImportant() != left->isImportant())
        return String();

    bool showLeft = right->value()->cssText() != left->value()->cssText();
    bool showBottom = (top->value()->cssText() != bottom->value()->cssText()) || showLeft;
    bool showRight = (top->value()->cssText() != right->value()->cssText()) || showBottom;

    StringBuilder result;
    result.append(top->value()->cssText());
    if (showRight) {
        result.append(' ');
        result.append(right->value()->cssText());
    }
    if (showBottom) {
        result.append(' ');
        result.append(bottom->value()->cssText());
    }
    if (showLeft) {
        result.append(' ');
        result.append(left->value()->cssText());
    }
    return result.toString();
}

String StylePropertySet::getLayeredShorthandValue(const StylePropertyShorthand& shorthand) const
{
    StringBuilder result;

    const unsigned size = shorthand.length();
    // Begin by collecting the properties into an array.
    Vector< RefPtr<CSSValue> > values(size);
    size_t numLayers = 0;

    for (unsigned i = 0; i < size; ++i) {
        values[i] = getPropertyCSSValue(shorthand.properties()[i]);
        if (values[i]) {
            if (values[i]->isBaseValueList()) {
                CSSValueList* valueList = static_cast<CSSValueList*>(values[i].get());
                numLayers = max(valueList->length(), numLayers);
            } else
                numLayers = max<size_t>(1U, numLayers);
        }
    }

    // Now stitch the properties together.  Implicit initial values are flagged as such and
    // can safely be omitted.
    for (size_t i = 0; i < numLayers; i++) {
        StringBuilder layerResult;
        bool useRepeatXShorthand = false;
        bool useRepeatYShorthand = false;
        bool useSingleWordShorthand = false;
        bool foundBackgroundPositionYCSSProperty = false;
        for (unsigned j = 0; j < size; j++) {
            RefPtr<CSSValue> value;
            if (values[j]) {
                if (values[j]->isBaseValueList())
                    value = static_cast<CSSValueList*>(values[j].get())->item(i);
                else {
                    value = values[j];

                    // Color only belongs in the last layer.
                    if (shorthand.properties()[j] == CSSPropertyBackgroundColor) {
                        if (i != numLayers - 1)
                            value = 0;
                    } else if (i != 0) // Other singletons only belong in the first layer.
                        value = 0;
                }
            }

            // We need to report background-repeat as it was written in the CSS. If the property is implicit,
            // then it was written with only one value. Here we figure out which value that was so we can
            // report back correctly.
            if (shorthand.properties()[j] == CSSPropertyBackgroundRepeatX && isPropertyImplicit(shorthand.properties()[j])) {

                // BUG 49055: make sure the value was not reset in the layer check just above.
                if (j < size - 1 && shorthand.properties()[j + 1] == CSSPropertyBackgroundRepeatY && value) {
                    RefPtr<CSSValue> yValue;
                    RefPtr<CSSValue> nextValue = values[j + 1];
                    if (nextValue->isValueList())
                        yValue = static_cast<CSSValueList*>(nextValue.get())->itemWithoutBoundsCheck(i);
                    else
                        yValue = nextValue;

                    int xId = static_cast<CSSPrimitiveValue*>(value.get())->getIdent();
                    int yId = static_cast<CSSPrimitiveValue*>(yValue.get())->getIdent();
                    if (xId != yId) {
                        if (xId == CSSValueRepeat && yId == CSSValueNoRepeat) {
                            useRepeatXShorthand = true;
                            ++j;
                        } else if (xId == CSSValueNoRepeat && yId == CSSValueRepeat) {
                            useRepeatYShorthand = true;
                            continue;
                        }
                    } else {
                        useSingleWordShorthand = true;
                        ++j;
                    }
                }
            }

            if (value && !value->isImplicitInitialValue()) {
                if (!layerResult.isEmpty())
                    layerResult.append(' ');
                if (foundBackgroundPositionYCSSProperty && shorthand.properties()[j] == CSSPropertyBackgroundSize) 
                    layerResult.appendLiteral("/ ");
                if (!foundBackgroundPositionYCSSProperty && shorthand.properties()[j] == CSSPropertyBackgroundSize) 
                    continue;

                if (useRepeatXShorthand) {
                    useRepeatXShorthand = false;
                    layerResult.append(getValueName(CSSValueRepeatX));
                } else if (useRepeatYShorthand) {
                    useRepeatYShorthand = false;
                    layerResult.append(getValueName(CSSValueRepeatY));
                } else if (useSingleWordShorthand) {
                    useSingleWordShorthand = false;
                    layerResult.append(value->cssText());
                } else
                    layerResult.append(value->cssText());

                if (shorthand.properties()[j] == CSSPropertyBackgroundPositionY)
                    foundBackgroundPositionYCSSProperty = true;
            }
        }

        if (!layerResult.isEmpty()) {
            if (!result.isEmpty())
                result.appendLiteral(", ");
            result.append(layerResult);
        }
    }
    if (result.isEmpty())
        return String();
    return result.toString();
}

String StylePropertySet::getShorthandValue(const StylePropertyShorthand& shorthand) const
{
    StringBuilder result;
    for (unsigned i = 0; i < shorthand.length(); ++i) {
        if (!isPropertyImplicit(shorthand.properties()[i])) {
            RefPtr<CSSValue> value = getPropertyCSSValue(shorthand.properties()[i]);
            if (!value)
                return String();
            if (value->isInitialValue())
                continue;
            if (!result.isEmpty())
                result.append(' ');
            result.append(value->cssText());
        }
    }
    if (result.isEmpty())
        return String();
    return result.toString();
}

// only returns a non-null value if all properties have the same, non-null value
String StylePropertySet::getCommonValue(const StylePropertyShorthand& shorthand) const
{
    String res;
    bool lastPropertyWasImportant = false;
    for (unsigned i = 0; i < shorthand.length(); ++i) {
        RefPtr<CSSValue> value = getPropertyCSSValue(shorthand.properties()[i]);
        // FIXME: CSSInitialValue::cssText should generate the right value.
        if (!value)
            return String();
        String text = value->cssText();
        if (text.isNull())
            return String();
        if (res.isNull())
            res = text;
        else if (res != text)
            return String();

        bool currentPropertyIsImportant = propertyIsImportant(shorthand.properties()[i]);
        if (i && lastPropertyWasImportant != currentPropertyIsImportant)
            return String();
        lastPropertyWasImportant = currentPropertyIsImportant;
    }
    return res;
}

String StylePropertySet::borderPropertyValue(CommonValueMode valueMode) const
{
    const StylePropertyShorthand properties[3] = { borderWidthShorthand(), borderStyleShorthand(), borderColorShorthand() };
    StringBuilder result;
    for (size_t i = 0; i < WTF_ARRAY_LENGTH(properties); ++i) {
        String value = getCommonValue(properties[i]);
        if (value.isNull()) {
            if (valueMode == ReturnNullOnUncommonValues)
                return String();
            ASSERT(valueMode == OmitUncommonValues);
            continue;
        }
        if (value == "initial")
            continue;
        if (!result.isEmpty())
            result.append(' ');
        result.append(value);
    }
    return result.isEmpty() ? String() : result.toString();
}

PassRefPtr<CSSValue> StylePropertySet::getPropertyCSSValue(CSSPropertyID propertyID) const
{
    const CSSProperty* property = findPropertyWithId(propertyID);
    return property ? property->value() : 0;
}

bool StylePropertySet::removeShorthandProperty(CSSPropertyID propertyID)
{
    ASSERT(isMutable());
    StylePropertyShorthand shorthand = shorthandForProperty(propertyID);
    if (!shorthand.length())
        return false;
    return removePropertiesInSet(shorthand.properties(), shorthand.length());
}

bool StylePropertySet::removeProperty(CSSPropertyID propertyID, String* returnText)
{
    ASSERT(isMutable());
    if (removeShorthandProperty(propertyID)) {
        // FIXME: Return an equivalent shorthand when possible.
        if (returnText)
            *returnText = "";
        return true;
    }

    CSSProperty* foundProperty = findPropertyWithId(propertyID);
    if (!foundProperty) {
        if (returnText)
            *returnText = "";
        return false;
    }

    if (returnText)
        *returnText = foundProperty->value()->cssText();

    // A more efficient removal strategy would involve marking entries as empty
    // and sweeping them when the vector grows too big.
    m_mutablePropertyVector->remove(foundProperty - m_mutablePropertyVector->data());
    
    return true;
}

bool StylePropertySet::propertyIsImportant(CSSPropertyID propertyID) const
{
    const CSSProperty* property = findPropertyWithId(propertyID);
    if (property)
        return property->isImportant();

    StylePropertyShorthand shorthand = shorthandForProperty(propertyID);
    if (!shorthand.length())
        return false;

    for (unsigned i = 0; i < shorthand.length(); ++i) {
        if (!propertyIsImportant(shorthand.properties()[i]))
            return false;
    }
    return true;
}

CSSPropertyID StylePropertySet::getPropertyShorthand(CSSPropertyID propertyID) const
{
    const CSSProperty* property = findPropertyWithId(propertyID);
    return property ? property->shorthandID() : CSSPropertyInvalid;
}

bool StylePropertySet::isPropertyImplicit(CSSPropertyID propertyID) const
{
    const CSSProperty* property = findPropertyWithId(propertyID);
    return property ? property->isImplicit() : false;
}

bool StylePropertySet::setProperty(CSSPropertyID propertyID, const String& value, bool important, StyleSheetContents* contextStyleSheet)
{
    ASSERT(isMutable());
    // Setting the value to an empty string just removes the property in both IE and Gecko.
    // Setting it to null seems to produce less consistent results, but we treat it just the same.
    if (value.isEmpty()) {
        removeProperty(propertyID);
        return true;
    }

    // When replacing an existing property value, this moves the property to the end of the list.
    // Firefox preserves the position, and MSIE moves the property to the beginning.
    return CSSParser::parseValue(this, propertyID, value, important, cssParserMode(), contextStyleSheet);
}

void StylePropertySet::setProperty(CSSPropertyID propertyID, PassRefPtr<CSSValue> prpValue, bool important)
{
    ASSERT(isMutable());
    StylePropertyShorthand shorthand = shorthandForProperty(propertyID);
    if (!shorthand.length()) {
        setProperty(CSSProperty(propertyID, prpValue, important));
        return;
    }

    removePropertiesInSet(shorthand.properties(), shorthand.length());

    RefPtr<CSSValue> value = prpValue;
    for (unsigned i = 0; i < shorthand.length(); ++i)
        append(CSSProperty(shorthand.properties()[i], value, important));
}

void StylePropertySet::setProperty(const CSSProperty& property, CSSProperty* slot)
{
    ASSERT(isMutable());
    if (!removeShorthandProperty(property.id())) {
        CSSProperty* toReplace = slot ? slot : findPropertyWithId(property.id());
        if (toReplace) {
            *toReplace = property;
            return;
        }
    }
    append(property);
}

bool StylePropertySet::setProperty(CSSPropertyID propertyID, int identifier, bool important)
{
    ASSERT(isMutable());
    setProperty(CSSProperty(propertyID, cssValuePool().createIdentifierValue(identifier), important));
    return true;
}

void StylePropertySet::parseDeclaration(const String& styleDeclaration, StyleSheetContents* contextStyleSheet)
{
    ASSERT(isMutable());

    m_mutablePropertyVector->clear();

    CSSParserContext context(cssParserMode());
    if (contextStyleSheet) {
        context = contextStyleSheet->parserContext();
        context.mode = cssParserMode();
    }
    CSSParser parser(context);
    parser.parseDeclaration(this, styleDeclaration, 0, contextStyleSheet);
}

void StylePropertySet::addParsedProperties(const Vector<CSSProperty>& properties)
{
    ASSERT(isMutable());
    m_mutablePropertyVector->reserveCapacity(m_mutablePropertyVector->size() + properties.size());
    for (unsigned i = 0; i < properties.size(); ++i)
        addParsedProperty(properties[i]);
}

void StylePropertySet::addParsedProperty(const CSSProperty& property)
{
    ASSERT(isMutable());
    // Only add properties that have no !important counterpart present
    if (!propertyIsImportant(property.id()) || property.isImportant())
        setProperty(property);
}

String StylePropertySet::asText() const
{
    StringBuilder result;

    const CSSProperty* positionXProp = 0;
    const CSSProperty* positionYProp = 0;
    const CSSProperty* repeatXProp = 0;
    const CSSProperty* repeatYProp = 0;

    BitArray<numCSSProperties> shorthandPropertyUsed;
    BitArray<numCSSProperties> shorthandPropertyAppeared;

    unsigned size = propertyCount();
    unsigned numDecls = 0;
    for (unsigned n = 0; n < size; ++n) {
        const CSSProperty& prop = propertyAt(n);
        CSSPropertyID propertyID = prop.id();
        CSSPropertyID shorthandPropertyID = CSSPropertyInvalid;
        CSSPropertyID borderFallbackShorthandProperty = CSSPropertyInvalid;
        String value;

        switch (propertyID) {
#if ENABLE(CSS_VARIABLES)
        case CSSPropertyVariable:
            if (numDecls++)
                result.append(' ');
            result.append(prop.cssText());
            continue;
#endif
        case CSSPropertyBackgroundPositionX:
            positionXProp = &prop;
            continue;
        case CSSPropertyBackgroundPositionY:
            positionYProp = &prop;
            continue;
        case CSSPropertyBackgroundRepeatX:
            repeatXProp = &prop;
            continue;
        case CSSPropertyBackgroundRepeatY:
            repeatYProp = &prop;
            continue;
        case CSSPropertyBorderTopWidth:
        case CSSPropertyBorderRightWidth:
        case CSSPropertyBorderBottomWidth:
        case CSSPropertyBorderLeftWidth:
            if (!borderFallbackShorthandProperty)
                borderFallbackShorthandProperty = CSSPropertyBorderWidth;
        case CSSPropertyBorderTopStyle:
        case CSSPropertyBorderRightStyle:
        case CSSPropertyBorderBottomStyle:
        case CSSPropertyBorderLeftStyle:
            if (!borderFallbackShorthandProperty)
                borderFallbackShorthandProperty = CSSPropertyBorderStyle;
        case CSSPropertyBorderTopColor:
        case CSSPropertyBorderRightColor:
        case CSSPropertyBorderBottomColor:
        case CSSPropertyBorderLeftColor:
            if (!borderFallbackShorthandProperty)
                borderFallbackShorthandProperty = CSSPropertyBorderColor;

            // FIXME: Deal with cases where only some of border-(top|right|bottom|left) are specified.
            if (!shorthandPropertyAppeared.get(CSSPropertyBorder - firstCSSProperty)) {
                value = borderPropertyValue(ReturnNullOnUncommonValues);
                if (value.isNull())
                    shorthandPropertyAppeared.set(CSSPropertyBorder - firstCSSProperty);
                else
                    shorthandPropertyID = CSSPropertyBorder;
            } else if (shorthandPropertyUsed.get(CSSPropertyBorder - firstCSSProperty))
                shorthandPropertyID = CSSPropertyBorder;
            if (!shorthandPropertyID)
                shorthandPropertyID = borderFallbackShorthandProperty;
            break;
        case CSSPropertyWebkitBorderHorizontalSpacing:
        case CSSPropertyWebkitBorderVerticalSpacing:
            shorthandPropertyID = CSSPropertyBorderSpacing;
            break;
        case CSSPropertyFontFamily:
        case CSSPropertyLineHeight:
        case CSSPropertyFontSize:
        case CSSPropertyFontStyle:
        case CSSPropertyFontVariant:
        case CSSPropertyFontWeight:
            // Don't use CSSPropertyFont because old UAs can't recognize them but are important for editing.
            break;
        case CSSPropertyListStyleType:
        case CSSPropertyListStylePosition:
        case CSSPropertyListStyleImage:
            shorthandPropertyID = CSSPropertyListStyle;
            break;
        case CSSPropertyMarginTop:
        case CSSPropertyMarginRight:
        case CSSPropertyMarginBottom:
        case CSSPropertyMarginLeft:
            shorthandPropertyID = CSSPropertyMargin;
            break;
        case CSSPropertyOutlineWidth:
        case CSSPropertyOutlineStyle:
        case CSSPropertyOutlineColor:
            shorthandPropertyID = CSSPropertyOutline;
            break;
        case CSSPropertyOverflowX:
        case CSSPropertyOverflowY:
            shorthandPropertyID = CSSPropertyOverflow;
            break;
        case CSSPropertyPaddingTop:
        case CSSPropertyPaddingRight:
        case CSSPropertyPaddingBottom:
        case CSSPropertyPaddingLeft:
            shorthandPropertyID = CSSPropertyPadding;
            break;
        case CSSPropertyWebkitAnimationName:
        case CSSPropertyWebkitAnimationDuration:
        case CSSPropertyWebkitAnimationTimingFunction:
        case CSSPropertyWebkitAnimationDelay:
        case CSSPropertyWebkitAnimationIterationCount:
        case CSSPropertyWebkitAnimationDirection:
        case CSSPropertyWebkitAnimationFillMode:
            shorthandPropertyID = CSSPropertyWebkitAnimation;
            break;
        case CSSPropertyWebkitFlexDirection:
        case CSSPropertyWebkitFlexWrap:
            shorthandPropertyID = CSSPropertyWebkitFlexFlow;
            break;
        case CSSPropertyWebkitFlexBasis:
        case CSSPropertyWebkitFlexGrow:
        case CSSPropertyWebkitFlexShrink:
            shorthandPropertyID = CSSPropertyWebkitFlex;
            break;
        case CSSPropertyWebkitMaskPositionX:
        case CSSPropertyWebkitMaskPositionY:
        case CSSPropertyWebkitMaskRepeatX:
        case CSSPropertyWebkitMaskRepeatY:
        case CSSPropertyWebkitMaskImage:
        case CSSPropertyWebkitMaskRepeat:
        case CSSPropertyWebkitMaskAttachment:
        case CSSPropertyWebkitMaskPosition:
        case CSSPropertyWebkitMaskClip:
        case CSSPropertyWebkitMaskOrigin:
            shorthandPropertyID = CSSPropertyWebkitMask;
            break;
        case CSSPropertyWebkitTransformOriginX:
        case CSSPropertyWebkitTransformOriginY:
        case CSSPropertyWebkitTransformOriginZ:
            shorthandPropertyID = CSSPropertyWebkitTransformOrigin;
            break;
        case CSSPropertyWebkitTransitionProperty:
        case CSSPropertyWebkitTransitionDuration:
        case CSSPropertyWebkitTransitionTimingFunction:
        case CSSPropertyWebkitTransitionDelay:
            shorthandPropertyID = CSSPropertyWebkitTransition;
            break;
#if ENABLE(CSS_EXCLUSIONS)
        case CSSPropertyWebkitWrapFlow:
        case CSSPropertyWebkitWrapMargin:
        case CSSPropertyWebkitWrapPadding:
            shorthandPropertyID = CSSPropertyWebkitWrap;
            break;
#endif
        default:
            break;
        }

        unsigned shortPropertyIndex = shorthandPropertyID - firstCSSProperty;
        if (shorthandPropertyID) {
            if (shorthandPropertyUsed.get(shortPropertyIndex))
                continue;
            if (!shorthandPropertyAppeared.get(shortPropertyIndex) && value.isNull())
                value = getPropertyValue(shorthandPropertyID);
            shorthandPropertyAppeared.set(shortPropertyIndex);
        }

        if (!value.isNull()) {
            propertyID = shorthandPropertyID;
            shorthandPropertyUsed.set(shortPropertyIndex);
        } else
            value = prop.value()->cssText();

        if (value == "initial" && !CSSProperty::isInheritedProperty(propertyID))
            continue;

        if (numDecls++)
            result.append(' ');
        result.append(getPropertyName(propertyID));
        result.appendLiteral(": ");
        result.append(value);
        if (prop.isImportant())
            result.appendLiteral(" !important");
        result.append(';');
    }

    // FIXME: This is a not-so-nice way to turn x/y positions into single background-position in output.
    // It is required because background-position-x/y are non-standard properties and WebKit generated output
    // would not work in Firefox (<rdar://problem/5143183>)
    // It would be a better solution if background-position was CSS_PAIR.
    if (positionXProp && positionYProp && positionXProp->isImportant() == positionYProp->isImportant()) {
        if (numDecls++)
            result.append(' ');
        result.appendLiteral("background-position: ");
        if (positionXProp->value()->isValueList() || positionYProp->value()->isValueList())
            result.append(getLayeredShorthandValue(backgroundPositionShorthand()));
        else {
            result.append(positionXProp->value()->cssText());
            result.append(' ');
            result.append(positionYProp->value()->cssText());
        }
        if (positionXProp->isImportant())
            result.appendLiteral(" !important");
        result.append(';');
    } else {
        if (positionXProp) {
            if (numDecls++)
                result.append(' ');
            result.append(positionXProp->cssText());
        }
        if (positionYProp) {
            if (numDecls++)
                result.append(' ');
            result.append(positionYProp->cssText());
        }
    }

    // FIXME: We need to do the same for background-repeat.
    if (repeatXProp && repeatYProp && repeatXProp->isImportant() == repeatYProp->isImportant()) {
        if (numDecls++)
            result.append(' ');
        result.appendLiteral("background-repeat: ");
        if (repeatXProp->value()->isValueList() || repeatYProp->value()->isValueList())
            result.append(getLayeredShorthandValue(backgroundRepeatShorthand()));
        else {
            result.append(repeatXProp->value()->cssText());
            result.append(' ');
            result.append(repeatYProp->value()->cssText());
        }
        if (repeatXProp->isImportant())
            result.appendLiteral(" !important");
        result.append(';');
    } else {
        if (repeatXProp) {
            if (numDecls++)
                result.append(' ');
            result.append(repeatXProp->cssText());
        }
        if (repeatYProp) {
            if (numDecls++)
                result.append(' ');
            result.append(repeatYProp->cssText());
        }
    }

    ASSERT(!numDecls ^ !result.isEmpty());
    return result.toString();
}

void StylePropertySet::mergeAndOverrideOnConflict(const StylePropertySet* other)
{
    ASSERT(isMutable());
    unsigned size = other->propertyCount();
    for (unsigned n = 0; n < size; ++n) {
        const CSSProperty& toMerge = other->propertyAt(n);
        CSSProperty* old = findPropertyWithId(toMerge.id());
        if (old)
            setProperty(toMerge, old);
        else
            append(toMerge);
    }
}

void StylePropertySet::addSubresourceStyleURLs(ListHashSet<KURL>& urls, StyleSheetContents* contextStyleSheet) const
{
    unsigned size = propertyCount();
    for (unsigned i = 0; i < size; ++i)
        propertyAt(i).value()->addSubresourceStyleURLs(urls, contextStyleSheet);
}

bool StylePropertySet::hasFailedOrCanceledSubresources() const
{
    unsigned size = propertyCount();
    for (unsigned i = 0; i < size; ++i) {
        if (propertyAt(i).value()->hasFailedOrCanceledSubresources())
            return true;
    }
    return false;
}

// This is the list of properties we want to copy in the copyBlockProperties() function.
// It is the list of CSS properties that apply specially to block-level elements.
static const CSSPropertyID blockProperties[] = {
    CSSPropertyOrphans,
    CSSPropertyOverflow, // This can be also be applied to replaced elements
    CSSPropertyWebkitAspectRatio,
    CSSPropertyWebkitColumnCount,
    CSSPropertyWebkitColumnGap,
    CSSPropertyWebkitColumnRuleColor,
    CSSPropertyWebkitColumnRuleStyle,
    CSSPropertyWebkitColumnRuleWidth,
    CSSPropertyWebkitColumnBreakBefore,
    CSSPropertyWebkitColumnBreakAfter,
    CSSPropertyWebkitColumnBreakInside,
    CSSPropertyWebkitColumnWidth,
    CSSPropertyPageBreakAfter,
    CSSPropertyPageBreakBefore,
    CSSPropertyPageBreakInside,
#if ENABLE(CSS_REGIONS)
    CSSPropertyWebkitRegionBreakAfter,
    CSSPropertyWebkitRegionBreakBefore,
    CSSPropertyWebkitRegionBreakInside,
#endif
    CSSPropertyTextAlign,
    CSSPropertyTextIndent,
    CSSPropertyWidows
};

const unsigned numBlockProperties = WTF_ARRAY_LENGTH(blockProperties);

PassRefPtr<StylePropertySet> StylePropertySet::copyBlockProperties() const
{
    return copyPropertiesInSet(blockProperties, numBlockProperties);
}

void StylePropertySet::removeBlockProperties()
{
    removePropertiesInSet(blockProperties, numBlockProperties);
}

bool StylePropertySet::removePropertiesInSet(const CSSPropertyID* set, unsigned length)
{
    ASSERT(isMutable());
    if (m_mutablePropertyVector->isEmpty())
        return false;

    // FIXME: This is always used with static sets and in that case constructing the hash repeatedly is pretty pointless.
    HashSet<CSSPropertyID> toRemove;
    for (unsigned i = 0; i < length; ++i)
        toRemove.add(set[i]);

    Vector<CSSProperty> newProperties;
    newProperties.reserveInitialCapacity(m_mutablePropertyVector->size());

    unsigned size = m_mutablePropertyVector->size();
    for (unsigned n = 0; n < size; ++n) {
        const CSSProperty& property = m_mutablePropertyVector->at(n);
        // Not quite sure if the isImportant test is needed but it matches the existing behavior.
        if (!property.isImportant()) {
            if (toRemove.contains(property.id()))
                continue;
        }
        newProperties.append(property);
    }

    bool changed = newProperties.size() != m_mutablePropertyVector->size();
    *m_mutablePropertyVector = newProperties;
    return changed;
}

const CSSProperty* StylePropertySet::findPropertyWithId(CSSPropertyID propertyID) const
{
    for (int n = propertyCount() - 1 ; n >= 0; --n) {
        if (propertyID == propertyAt(n).id())
            return &propertyAt(n);
    }
    return 0;
}

CSSProperty* StylePropertySet::findPropertyWithId(CSSPropertyID propertyID)
{
    ASSERT(isMutable());
    for (int n = propertyCount() - 1 ; n >= 0; --n) {
        if (propertyID == propertyAt(n).id())
            return &propertyAt(n);
    }
    return 0;
}
    
bool StylePropertySet::propertyMatches(const CSSProperty* property) const
{
    RefPtr<CSSValue> value = getPropertyCSSValue(property->id());
    return value && value->cssText() == property->value()->cssText();
}
    
void StylePropertySet::removeEquivalentProperties(const StylePropertySet* style)
{
    ASSERT(isMutable());
    Vector<CSSPropertyID> propertiesToRemove;
    unsigned size = m_mutablePropertyVector->size();
    for (unsigned i = 0; i < size; ++i) {
        const CSSProperty& property = m_mutablePropertyVector->at(i);
        if (style->propertyMatches(&property))
            propertiesToRemove.append(property.id());
    }    
    // FIXME: This should use mass removal.
    for (unsigned i = 0; i < propertiesToRemove.size(); ++i)
        removeProperty(propertiesToRemove[i]);
}

void StylePropertySet::removeEquivalentProperties(const CSSStyleDeclaration* style)
{
    ASSERT(isMutable());
    Vector<CSSPropertyID> propertiesToRemove;
    unsigned size = m_mutablePropertyVector->size();
    for (unsigned i = 0; i < size; ++i) {
        const CSSProperty& property = m_mutablePropertyVector->at(i);
        if (style->cssPropertyMatches(&property))
            propertiesToRemove.append(property.id());
    }    
    // FIXME: This should use mass removal.
    for (unsigned i = 0; i < propertiesToRemove.size(); ++i)
        removeProperty(propertiesToRemove[i]);
}

PassRefPtr<StylePropertySet> StylePropertySet::copy() const
{
    return adoptRef(new StylePropertySet(*this));
}

PassRefPtr<StylePropertySet> StylePropertySet::copyPropertiesInSet(const CSSPropertyID* set, unsigned length) const
{
    Vector<CSSProperty, 256> list;
    list.reserveInitialCapacity(length);
    for (unsigned i = 0; i < length; ++i) {
        RefPtr<CSSValue> value = getPropertyCSSValue(set[i]);
        if (value)
            list.append(CSSProperty(set[i], value.release(), false));
    }
    return StylePropertySet::create(list.data(), list.size());
}

CSSStyleDeclaration* StylePropertySet::ensureCSSStyleDeclaration()
{
    ASSERT(isMutable());

    if (m_ownsCSSOMWrapper) {
        ASSERT(!static_cast<CSSStyleDeclaration*>(propertySetCSSOMWrapperMap().get(this))->parentRule());
        ASSERT(!propertySetCSSOMWrapperMap().get(this)->parentElement());
        return propertySetCSSOMWrapperMap().get(this);
    }
    m_ownsCSSOMWrapper = true;
    PropertySetCSSStyleDeclaration* cssomWrapper = new PropertySetCSSStyleDeclaration(const_cast<StylePropertySet*>(this));
    propertySetCSSOMWrapperMap().add(this, adoptPtr(cssomWrapper));
    return cssomWrapper;
}

CSSStyleDeclaration* StylePropertySet::ensureInlineCSSStyleDeclaration(const StyledElement* parentElement)
{
    ASSERT(isMutable());

    if (m_ownsCSSOMWrapper) {
        ASSERT(propertySetCSSOMWrapperMap().get(this)->parentElement() == parentElement);
        return propertySetCSSOMWrapperMap().get(this);
    }
    m_ownsCSSOMWrapper = true;
    PropertySetCSSStyleDeclaration* cssomWrapper = new InlineCSSStyleDeclaration(const_cast<StylePropertySet*>(this), const_cast<StyledElement*>(parentElement));
    propertySetCSSOMWrapperMap().add(this, adoptPtr(cssomWrapper));
    return cssomWrapper;
}

void StylePropertySet::clearParentElement(StyledElement* element)
{
    if (!m_ownsCSSOMWrapper)
        return;
    ASSERT_UNUSED(element, propertySetCSSOMWrapperMap().get(this)->parentElement() == element);
    propertySetCSSOMWrapperMap().get(this)->clearParentElement();
}

unsigned StylePropertySet::averageSizeInBytes()
{
    // Please update this if the storage scheme changes so that this longer reflects the actual size.
    return sizeof(StylePropertySet) + sizeof(CSSProperty) * 2;
}

void StylePropertySet::reportMemoryUsage(MemoryObjectInfo* memoryObjectInfo) const
{
    size_t actualSize = m_isMutable ? sizeof(StylePropertySet) : immutableStylePropertySetSize(m_arraySize);
    MemoryClassInfo info(memoryObjectInfo, this, WebCoreMemoryTypes::CSS, actualSize);
    if (m_isMutable)
        info.addVectorPtr(m_mutablePropertyVector);

    unsigned count = propertyCount();
    for (unsigned i = 0; i < count; ++i)
        info.addMember(propertyAt(i));
}

// See the function above if you need to update this.
struct SameSizeAsStylePropertySet : public RefCounted<SameSizeAsStylePropertySet> {
    unsigned bitfield;
    void* properties;
};
COMPILE_ASSERT(sizeof(StylePropertySet) == sizeof(SameSizeAsStylePropertySet), style_property_set_should_stay_small);

#ifndef NDEBUG
void StylePropertySet::showStyle()
{
    fprintf(stderr, "%s\n", asText().ascii().data());
}
#endif

inline void StylePropertySet::append(const CSSProperty& property)
{
    ASSERT(isMutable());
    m_mutablePropertyVector->append(property);
}

} // namespace WebCore
