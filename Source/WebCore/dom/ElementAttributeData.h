/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Peter Kelly (pmk@post.com)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2008, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef ElementAttributeData_h
#define ElementAttributeData_h

#include "Attribute.h"
#include "SpaceSplitString.h"
#include "StylePropertySet.h"
#include <wtf/NotFound.h>

namespace WebCore {

class Element;

class ElementAttributeData {
public:
    ~ElementAttributeData();

    void clearClass() { m_classNames.clear(); }
    void setClass(const String& className, bool shouldFoldCase);
    const SpaceSplitString& classNames() const { return m_classNames; }

    const AtomicString& idForStyleResolution() const { return m_idForStyleResolution; }
    void setIdForStyleResolution(const AtomicString& newId) { m_idForStyleResolution = newId; }

    StylePropertySet* inlineStyleDecl() { return m_inlineStyleDecl.get(); }
    StylePropertySet* ensureInlineStyleDecl(StyledElement*);
    void destroyInlineStyleDecl(StyledElement* element);

    StylePropertySet* attributeStyle() const { return m_attributeStyle.get(); }
    void setAttributeStyle(PassRefPtr<StylePropertySet> style) { m_attributeStyle = style; }

    size_t length() const { return m_attributes.size(); }
    bool isEmpty() const { return m_attributes.isEmpty(); }

    // Internal interface.
    Attribute* attributeItem(unsigned index) const { return m_attributes[index].get(); }
    Attribute* getAttributeItem(const QualifiedName&) const;
    size_t getAttributeItemIndex(const QualifiedName&) const;

    // These functions do no error checking.
    void addAttribute(PassRefPtr<Attribute>, Element*);
    void removeAttribute(const QualifiedName&, Element*);
    void removeAttribute(size_t index, Element*);

private:
    friend class Element;
    friend class NamedNodeMap;

    ElementAttributeData()
    {
    }

    void detachAttributesFromElement();
    void copyAttributesToVector(Vector<RefPtr<Attribute> >&);
    Attribute* getAttributeItem(const String& name, bool shouldIgnoreAttributeCase) const;
    size_t getAttributeItemIndex(const String& name, bool shouldIgnoreAttributeCase) const;
    size_t getAttributeItemIndexSlowCase(const String& name, bool shouldIgnoreAttributeCase) const;
    void setAttributes(const ElementAttributeData& other, Element*);
    void clearAttributes();
    void replaceAttribute(size_t index, PassRefPtr<Attribute>, Element*);

    RefPtr<StylePropertySet> m_inlineStyleDecl;
    RefPtr<StylePropertySet> m_attributeStyle;
    SpaceSplitString m_classNames;
    AtomicString m_idForStyleResolution;
    Vector<RefPtr<Attribute>, 4> m_attributes;
};

inline void ElementAttributeData::removeAttribute(const QualifiedName& name, Element* element)
{
    size_t index = getAttributeItemIndex(name);
    if (index == notFound)
        return;

    removeAttribute(index, element);
}

inline Attribute* ElementAttributeData::getAttributeItem(const String& name, bool shouldIgnoreAttributeCase) const
{
    size_t index = getAttributeItemIndex(name, shouldIgnoreAttributeCase);
    if (index != notFound)
        return m_attributes[index].get();
    return 0;
}

inline Attribute* ElementAttributeData::getAttributeItem(const QualifiedName& name) const
{
    size_t index = getAttributeItemIndex(name);
    if (index != notFound)
        return m_attributes[index].get();
    return 0;
}

// We use a boolean parameter instead of calling shouldIgnoreAttributeCase so that the caller
// can tune the behavior (hasAttribute is case sensitive whereas getAttribute is not).
inline size_t ElementAttributeData::getAttributeItemIndex(const QualifiedName& name) const
{
    size_t len = length();
    for (unsigned i = 0; i < len; ++i) {
        if (m_attributes[i]->name().matches(name))
            return i;
    }
    return notFound;
}

inline size_t ElementAttributeData::getAttributeItemIndex(const String& name, bool shouldIgnoreAttributeCase) const
{
    unsigned len = length();
    bool doSlowCheck = shouldIgnoreAttributeCase;

    // Optimize for the case where the attribute exists and its name exactly matches.
    for (unsigned i = 0; i < len; ++i) {
        const QualifiedName& attrName = m_attributes[i]->name();
        if (!attrName.hasPrefix()) {
            if (name == attrName.localName())
                return i;
        } else
            doSlowCheck = true;
    }

    if (doSlowCheck)
        return getAttributeItemIndexSlowCase(name, shouldIgnoreAttributeCase);
    return notFound;
}

}

#endif // ElementAttributeData_h
