/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2008, 2012, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
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

#ifndef StylePropertySet_h
#define StylePropertySet_h

#include "CSSParserMode.h"
#include "CSSPrimitiveValue.h"
#include "CSSProperty.h"
#include "CSSPropertyNames.h"
#include "CSSValueKeywords.h"
#include <wtf/ListHashSet.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class CSSRule;
class CSSStyleDeclaration;
class ComputedStyleExtractor;
class ImmutableStylePropertySet;
class URL;
class MutableStylePropertySet;
class PropertySetCSSStyleDeclaration;
class StyledElement;
class StylePropertyShorthand;
class StyleSheetContents;

class StylePropertySet : public RefCounted<StylePropertySet> {
    friend class PropertyReference;
public:
    // Override RefCounted's deref() to ensure operator delete is called on
    // the appropriate subclass type.
    void deref();

    class PropertyReference {
    public:
        PropertyReference(const StylePropertySet& propertySet, unsigned index)
            : m_propertySet(propertySet)
            , m_index(index)
        {
        }

        CSSPropertyID id() const { return static_cast<CSSPropertyID>(propertyMetadata().m_propertyID); }
        CSSPropertyID shorthandID() const { return propertyMetadata().shorthandID(); }

        bool isImportant() const { return propertyMetadata().m_important; }
        bool isInherited() const { return propertyMetadata().m_inherited; }
        bool isImplicit() const { return propertyMetadata().m_implicit; }

        String cssName() const;
        String cssText() const;

        const CSSValue* value() const { return propertyValue(); }
        // FIXME: We should try to remove this mutable overload.
        CSSValue* value() { return const_cast<CSSValue*>(propertyValue()); }

        // FIXME: Remove this.
        CSSProperty toCSSProperty() const { return CSSProperty(propertyMetadata(), const_cast<CSSValue*>(propertyValue())); }
        const StylePropertyMetadata& propertyMetadata() const;

    private:
        const CSSValue* propertyValue() const;

        const StylePropertySet& m_propertySet;
        unsigned m_index;
    };

    unsigned propertyCount() const;
    bool isEmpty() const;
    PropertyReference propertyAt(unsigned index) const { return PropertyReference(*this, index); }

    PassRefPtr<CSSValue> getPropertyCSSValue(CSSPropertyID) const;
    String getPropertyValue(CSSPropertyID) const;
    bool propertyIsImportant(CSSPropertyID) const;
    String getPropertyShorthand(CSSPropertyID) const;
    bool isPropertyImplicit(CSSPropertyID) const;

    PassRef<MutableStylePropertySet> copyBlockProperties() const;

    CSSParserMode cssParserMode() const { return static_cast<CSSParserMode>(m_cssParserMode); }

    void addSubresourceStyleURLs(ListHashSet<URL>&, StyleSheetContents* contextStyleSheet) const;

    PassRef<MutableStylePropertySet> mutableCopy() const;
    PassRef<ImmutableStylePropertySet> immutableCopyIfNeeded() const;

    PassRef<MutableStylePropertySet> copyPropertiesInSet(const CSSPropertyID* set, unsigned length) const;
    
    String asText() const;

    bool isMutable() const { return m_isMutable; }
    bool hasCSSOMWrapper() const;

    bool hasFailedOrCanceledSubresources() const;

    static unsigned averageSizeInBytes();

#ifndef NDEBUG
    void showStyle();
#endif

    bool propertyMatches(CSSPropertyID, const CSSValue*) const;

protected:
    StylePropertySet(CSSParserMode cssParserMode)
        : m_cssParserMode(cssParserMode)
        , m_isMutable(true)
        , m_arraySize(0)
    { }

    StylePropertySet(CSSParserMode cssParserMode, unsigned immutableArraySize)
        : m_cssParserMode(cssParserMode)
        , m_isMutable(false)
        , m_arraySize(immutableArraySize)
    { }

    int findPropertyIndex(CSSPropertyID) const;

    unsigned m_cssParserMode : 2;
    mutable unsigned m_isMutable : 1;
    unsigned m_arraySize : 29;
    
private:
    String getShorthandValue(const StylePropertyShorthand&) const;
    String getCommonValue(const StylePropertyShorthand&) const;
    enum CommonValueMode { OmitUncommonValues, ReturnNullOnUncommonValues };
    String borderPropertyValue(CommonValueMode) const;
    String getLayeredShorthandValue(const StylePropertyShorthand&) const;
    String get4Values(const StylePropertyShorthand&) const;
    String borderSpacingValue(const StylePropertyShorthand&) const;
    String fontValue() const;
    void appendFontLonghandValueIfExplicit(CSSPropertyID, StringBuilder& result, String& value) const;

    friend class PropertySetCSSStyleDeclaration;
};

class ImmutableStylePropertySet : public StylePropertySet {
public:
    ~ImmutableStylePropertySet();
    static PassRef<ImmutableStylePropertySet> create(const CSSProperty* properties, unsigned count, CSSParserMode);

    unsigned propertyCount() const { return m_arraySize; }

    const CSSValue** valueArray() const;
    const StylePropertyMetadata* metadataArray() const;

    void* m_storage;

private:
    ImmutableStylePropertySet(const CSSProperty*, unsigned count, CSSParserMode);
};

inline const CSSValue** ImmutableStylePropertySet::valueArray() const
{
    return reinterpret_cast<const CSSValue**>(const_cast<const void**>((&(this->m_storage))));
}

inline const StylePropertyMetadata* ImmutableStylePropertySet::metadataArray() const
{
    return reinterpret_cast_ptr<const StylePropertyMetadata*>(&reinterpret_cast_ptr<const char*>(&(this->m_storage))[m_arraySize * sizeof(CSSValue*)]);
}

class MutableStylePropertySet : public StylePropertySet {
public:
    static PassRef<MutableStylePropertySet> create(CSSParserMode = CSSQuirksMode);
    static PassRef<MutableStylePropertySet> create(const CSSProperty* properties, unsigned count);

    ~MutableStylePropertySet();

    unsigned propertyCount() const { return m_propertyVector.size(); }

    PropertySetCSSStyleDeclaration* cssStyleDeclaration();

    void addParsedProperties(const Vector<CSSProperty>&);
    void addParsedProperty(const CSSProperty&);

    // These expand shorthand properties into multiple properties.
    bool setProperty(CSSPropertyID, const String& value, bool important = false, StyleSheetContents* contextStyleSheet = 0);
    void setProperty(CSSPropertyID, PassRefPtr<CSSValue>, bool important = false);

    // These do not. FIXME: This is too messy, we can do better.
    bool setProperty(CSSPropertyID, CSSValueID identifier, bool important = false);
    bool setProperty(CSSPropertyID, CSSPropertyID identifier, bool important = false);
    void appendPrefixingVariantProperty(const CSSProperty&);
    void setPrefixingVariantProperty(const CSSProperty&);
    void setProperty(const CSSProperty&, CSSProperty* slot = 0);

    bool removeProperty(CSSPropertyID, String* returnText = 0);
    void removePrefixedOrUnprefixedProperty(CSSPropertyID);
    void removeBlockProperties();
    bool removePropertiesInSet(const CSSPropertyID* set, unsigned length);

    // FIXME: These two can be moved to EditingStyle.cpp
    void removeEquivalentProperties(const StylePropertySet*);
    void removeEquivalentProperties(const ComputedStyleExtractor*);

    void mergeAndOverrideOnConflict(const StylePropertySet&);

    void clear();
    void parseDeclaration(const String& styleDeclaration, StyleSheetContents* contextStyleSheet);

    CSSStyleDeclaration* ensureCSSStyleDeclaration();
    CSSStyleDeclaration* ensureInlineCSSStyleDeclaration(StyledElement* parentElement);

    Vector<CSSProperty, 4> m_propertyVector;

private:
    explicit MutableStylePropertySet(CSSParserMode);
    explicit MutableStylePropertySet(const StylePropertySet&);
    MutableStylePropertySet(const CSSProperty* properties, unsigned count);

    bool removeShorthandProperty(CSSPropertyID);
    CSSProperty* findCSSPropertyWithID(CSSPropertyID);
    OwnPtr<PropertySetCSSStyleDeclaration> m_cssomWrapper;

    friend class StylePropertySet;
};

inline const StylePropertyMetadata& StylePropertySet::PropertyReference::propertyMetadata() const
{
    if (m_propertySet.isMutable())
        return static_cast<const MutableStylePropertySet&>(m_propertySet).m_propertyVector.at(m_index).metadata();
    return static_cast<const ImmutableStylePropertySet&>(m_propertySet).metadataArray()[m_index];
}

inline const CSSValue* StylePropertySet::PropertyReference::propertyValue() const
{
    if (m_propertySet.isMutable())
        return static_cast<const MutableStylePropertySet&>(m_propertySet).m_propertyVector.at(m_index).value();
    return static_cast<const ImmutableStylePropertySet&>(m_propertySet).valueArray()[m_index];
}

inline unsigned StylePropertySet::propertyCount() const
{
    if (m_isMutable)
        return static_cast<const MutableStylePropertySet*>(this)->m_propertyVector.size();
    return m_arraySize;
}

inline bool StylePropertySet::isEmpty() const
{
    return !propertyCount();
}

inline void StylePropertySet::deref()
{
    if (!derefBase())
        return;

    if (m_isMutable)
        delete static_cast<MutableStylePropertySet*>(this);
    else
        delete static_cast<ImmutableStylePropertySet*>(this);
}

} // namespace WebCore

#endif // StylePropertySet_h
