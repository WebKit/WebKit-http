/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Peter Kelly (pmk@post.com)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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
 *
 */

#ifndef StyledElement_h
#define StyledElement_h

#include "Element.h"
#include "StylePropertySet.h"

namespace WebCore {

class Attribute;
struct PresentationAttributeCacheKey;

class StyledElement : public Element {
public:
    virtual ~StyledElement();

    virtual const StylePropertySet* additionalPresentationAttributeStyle() { return 0; }
    void invalidateStyleAttribute();

    const StylePropertySet* inlineStyle() const { return attributeData() ? attributeData()->m_inlineStyle.get() : 0; }
    StylePropertySet* ensureMutableInlineStyle();
    
    // Unlike StylePropertySet setters, these implement invalidation.
    bool setInlineStyleProperty(CSSPropertyID, int identifier, bool important = false);
    bool setInlineStyleProperty(CSSPropertyID, double value, CSSPrimitiveValue::UnitTypes, bool important = false);
    bool setInlineStyleProperty(CSSPropertyID, const String& value, bool important = false);
    bool removeInlineStyleProperty(CSSPropertyID);
    void removeAllInlineStyleProperties();
    
    virtual CSSStyleDeclaration* style() OVERRIDE;

    const StylePropertySet* presentationAttributeStyle();

    virtual void collectStyleForPresentationAttribute(const Attribute&, StylePropertySet*) { }

protected:
    StyledElement(const QualifiedName& name, Document* document, ConstructionType type)
        : Element(name, document, type)
    {
    }

    virtual void attributeChanged(const QualifiedName&, const AtomicString&) OVERRIDE;

    virtual bool isPresentationAttribute(const QualifiedName&) const { return false; }

    void addPropertyToPresentationAttributeStyle(StylePropertySet*, CSSPropertyID, int identifier);
    void addPropertyToPresentationAttributeStyle(StylePropertySet*, CSSPropertyID, double value, CSSPrimitiveValue::UnitTypes);
    void addPropertyToPresentationAttributeStyle(StylePropertySet*, CSSPropertyID, const String& value);

    virtual void addSubresourceAttributeURLs(ListHashSet<KURL>&) const;

private:
    void styleAttributeChanged(const AtomicString& newStyleString);

    virtual void updateStyleAttribute() const;
    void inlineStyleChanged();
    PropertySetCSSStyleDeclaration* inlineStyleCSSOMWrapper();
    void setInlineStyleFromString(const AtomicString&);

    void makePresentationAttributeCacheKey(PresentationAttributeCacheKey&) const;
    void rebuildPresentationAttributeStyle();
};

inline void StyledElement::invalidateStyleAttribute()
{
    ASSERT(attributeData());
    attributeData()->m_styleAttributeIsDirty = true;
}

inline const StylePropertySet* StyledElement::presentationAttributeStyle()
{
    if (!attributeData())
        return 0;
    if (attributeData()->m_presentationAttributeStyleIsDirty)
        rebuildPresentationAttributeStyle();
    return attributeData()->presentationAttributeStyle();
}

} //namespace

#endif
